///////////////////////////////////////////////////////////////////////////////
///
/// @file Task_MigrateCtTarget.cpp
///
/// Target task for Vm migration
///
/// @author krasnov@
///
/// Copyright (c) 2010-2015 Parallels IP Holdings GmbH
///
/// This file is part of Virtuozzo Core. Virtuozzo Core is free
/// software; you can redistribute it and/or modify it under the terms
/// of the GNU General Public License as published by the Free Software
/// Foundation; either version 2 of the License, or (at your option) any
/// later version.
/// 
/// This program is distributed in the hope that it will be useful,
/// but WITHOUT ANY WARRANTY; without even the implied warranty of
/// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
/// GNU General Public License for more details.
/// 
/// You should have received a copy of the GNU General Public License
/// along with this program; if not, write to the Free Software
/// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
/// 02110-1301, USA.
///
/// Our contact details: Parallels IP Holdings GmbH, Vordergasse 59, 8200
/// Schaffhausen, Switzerland.
///
/////////////////////////////////////////////////////////////////////////////////

//#define LOGGING_ON
//#define FORCE_LOGGING_LEVEL DBG_DEBUG

#include <limits.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syslog.h>
#include <unistd.h>
#include <time.h>
#include <poll.h>
#include <sys/resource.h>

#include "Interfaces/Debug.h"
#include "Interfaces/ParallelsQt.h"
#include "Interfaces/ParallelsNamespace.h"

#include "Libraries/Logging/Logging.h"

#include "Dispatcher/Dispatcher/Tasks/Task_DispToDispConnHelper.h"

#include "Task_VzMigrate.h"
#include "CDspService.h"
#include "Libraries/Std/PrlAssert.h"


/*
 vzmigrate requires 4 interchanges sockets:
   command socket - for command/reply and rsync data
   data socket - for tar data (private area)
   tmpl data socket - for tar data (template area)
   swap socket - for memory pages transmission.
 Will retransmit data between sockets/remote dispatcher by 3 theads:
  main thread will read sockets and send packages as reply on start package
  and child thread will recieve packages and write to sockets.
 To close child thread will use urgentResponseWakeUp() method.
 In main thread will use waitpid(WNOHANG) and poll()
*/


Task_VzMigrate::Task_VzMigrate(const SmartPtr<CDspClient> &client, const SmartPtr<IOPackage> &p)
:CDspTaskHelper(client, p),
Task_DispToDispConnHelper(getLastError())
{
	m_nFdSize = sizeof(m_nFd)/sizeof(int);
	for (int i = 0; i < m_nFdSize; i++)
		m_nFd[i] = -1;

	m_nBufferSize = PRL_DISP_IO_BUFFER_SIZE;
	m_pid = -1;
	m_pHandleDispPackageTask = NULL;

	/* alloc buffer for data from vzmigrate and link this buffer with IO package */
	m_pBuffer = SmartPtr<char>(new char[m_nBufferSize], SmartPtrPolicy::ArrayStorage);
}

Task_VzMigrate::~Task_VzMigrate()
{
	terminateVzMigrate();

	terminateHandleDispPackageTask();

	for (int i = 0; i < m_nFdSize; i++)
		if (m_nFd[i] != -1)
			close(m_nFd[i]);
}

PRL_RESULT Task_VzMigrate::readFromVzMigrate(quint16 nFdNum)
{
	PRL_RESULT nRetCode = PRL_ERR_SUCCESS;
	quint32 nSize = 0;
	ssize_t rc;

	PRL_ASSERT( nFdNum < m_nFdSize );

	/* read data */
	while (1) {
		errno = 0;
		rc = ::read(m_nFd[nFdNum], m_pBuffer.getImpl() + nSize, (size_t)(m_nBufferSize - nSize));
		if (rc > 0) {
			nSize += (quint32)rc;
			/* if buffer is full, send it */
			if (nSize >= m_nBufferSize) {
				nRetCode = sendDispPackage(nFdNum, nSize);
				if (PRL_FAILED(nRetCode))
					return nRetCode;
				nSize = 0;
			}
		} else if ((rc == 0) || (errno == ECONNRESET)) {
			/* end of file - pipe was close, will check client exit code */
			WRITE_TRACE(DBG_DEBUG, "EOF");
			close(m_nFd[nFdNum]);
			m_nFd[nFdNum] = -1;
			nRetCode = sendDispPackage(nFdNum, nSize);
			if (PRL_FAILED(nRetCode))
				return nRetCode;
			return sendDispPackage(nFdNum, 0);
		} else if (errno == EAGAIN) {
			/* no data to read - send buffer and exit */
			return sendDispPackage(nFdNum, nSize);
		} else if (errno == EINTR) {
			/* signal - send buffer and exit - to check waitpid() */
			return sendDispPackage(nFdNum, nSize);
		} else {
			WRITE_TRACE(DBG_FATAL, "read() : %m");
			return PRL_ERR_CT_MIGRATE_INTERNAL_ERROR;
		}
	}
	return PRL_ERR_SUCCESS;
}

void Task_VzMigrate::readOutFromVzMigrate()
{
	char pBuffer[BUFSIZ];
	quint32 nSize = 0;
	ssize_t rc;

	while (1) {
		if (operationIsCancelled())
			return;

		errno = 0;
		rc = ::read(m_nFd[PRL_CT_MIGRATE_OUT_FD], pBuffer + nSize, (size_t)(sizeof(pBuffer) - nSize));
		if (rc > 0) {
			nSize += (quint32)rc;
			if (nSize >= sizeof(pBuffer) - 1) {
				pBuffer[nSize] = '\0';
				WRITE_TRACE(DBG_INFO, "%s", pBuffer);
				nSize = 0;
			}
		} else if (rc == 0) {
			/* end of file - pipe was close, will check client exit code */
			pBuffer[nSize] = '\0';
			WRITE_TRACE(DBG_INFO, "%s", pBuffer);
			close(m_nFd[PRL_CT_MIGRATE_OUT_FD]);
			m_nFd[PRL_CT_MIGRATE_OUT_FD] = -1;
			return;
		} else if ((errno == EAGAIN) || (errno == EINTR)) {
			pBuffer[nSize] = '\0';
			WRITE_TRACE(DBG_INFO, "%s", pBuffer);
			return;
		} else {
			WRITE_TRACE(DBG_FATAL, "read() : %m");
			return;
		}
	}
}

/* send package with migration data to target dispatcher */
PRL_RESULT Task_VzMigrate::sendDispPackage(quint16 nType, quint32 nSize)
{
	m_pPackage->fillBuffer(0, IOPackage::RawEncoding, &nType, sizeof(nType));
	m_pPackage->setBuffer(1, IOPackage::RawEncoding, m_pBuffer, nSize);
	return sendDispPackage(m_pPackage);
}

/*
 start vzmigrate with three interchanges sockets:
   command socket - for command/reply and rsync data
   data socket - for tar data (private area)
   tmpl data socket - for tar data (template area)
   swap socket - for memory pages transmission
  send this sockets to vzmigrate as parameters.
*/
PRL_RESULT Task_VzMigrate::startVzMigrate(const QString &sCmd, const QStringList &lstArgs)
{
	PRL_RESULT nRetCode = PRL_ERR_SUCCESS;
	int out_fds[2], cmd_socks[2], data_socks[2], tmpl_data_socks[2], swap_socks[2];
	int status;
	pid_t pid;
	char buffer[11];
	long flags;
	char **pArgv = NULL;
	size_t nArgs, ndx;
	int i;
	QString sMgrtCmd;

	m_sCmd = sCmd;

	nArgs = lstArgs.size()+7;
	pArgv = (char **)calloc(nArgs, sizeof(char *));
	if (pArgv == NULL) {
		WRITE_TRACE(DBG_FATAL, "calloc() : %m");
		return PRL_ERR_CT_MIGRATE_INTERNAL_ERROR;
	}
	if (pipe(out_fds) < 0) {
		WRITE_TRACE(DBG_FATAL, "pipe() : %m");
		nRetCode = PRL_ERR_CT_MIGRATE_INTERNAL_ERROR;
		goto cleanup_0;
	}
	if (socketpair(AF_UNIX, SOCK_STREAM, 0, cmd_socks) < 0) {
		WRITE_TRACE(DBG_FATAL, "socketpair() : %m");
		nRetCode = PRL_ERR_CT_MIGRATE_INTERNAL_ERROR;
		goto cleanup_1;
	}
	if (socketpair(AF_UNIX, SOCK_STREAM, 0, data_socks) < 0) {
		WRITE_TRACE(DBG_FATAL, "socketpair() : %m");
		nRetCode = PRL_ERR_CT_MIGRATE_INTERNAL_ERROR;
		goto cleanup_2;
	}
	if (socketpair(AF_UNIX, SOCK_STREAM, 0, tmpl_data_socks) < 0) {
		WRITE_TRACE(DBG_FATAL, "socketpair() : %m");
		nRetCode = PRL_ERR_CT_MIGRATE_INTERNAL_ERROR;
		goto cleanup_3;
	}
	if (socketpair(AF_UNIX, SOCK_STREAM, 0, swap_socks) < 0) {
		WRITE_TRACE(DBG_FATAL, "socketpair() : %m");
		nRetCode = PRL_ERR_CT_MIGRATE_INTERNAL_ERROR;
		goto cleanup_4;
	}

	ndx = 0;
	pArgv[ndx++] = strdup(QSTR2UTF8(m_sCmd));
	pArgv[ndx++] = strdup("-ps");
	snprintf(buffer, sizeof(buffer), "%d", cmd_socks[1]);
	pArgv[ndx++] = strdup(buffer);
	snprintf(buffer, sizeof(buffer), "%d", data_socks[1]);
	pArgv[ndx++] = strdup(buffer);
	snprintf(buffer, sizeof(buffer), "%d", tmpl_data_socks[1]);
	pArgv[ndx++] = strdup(buffer);
	snprintf(buffer, sizeof(buffer), "%d", swap_socks[1]);
	pArgv[ndx++] = strdup(buffer);

	for(i = 0; i < lstArgs.size(); i++, ndx++) {
		pArgv[ndx] = strdup(QSTR2UTF8(lstArgs.at(i)));
		PRL_ASSERT(nArgs > ndx-1);
	}
	pArgv[ndx] = NULL;

	// Print final cmd line to log
	for(i = 0; i < (int)ndx; i++)
		sMgrtCmd.append(QString(" \'%1\'").arg(pArgv[i]));
	WRITE_TRACE(DBG_INFO, "Run migration command: %s", QSTR2UTF8(sMgrtCmd));

	if ((flags = fcntl(out_fds[0], F_GETFL)) != -1)
		fcntl(out_fds[0], F_SETFL, flags | O_NONBLOCK);
	if ((flags = fcntl(cmd_socks[0], F_GETFL)) != -1)
		fcntl(cmd_socks[0], F_SETFL, flags | O_NONBLOCK);
	if ((flags = fcntl(swap_socks[0], F_GETFL)) != -1)
		fcntl(swap_socks[0], F_SETFL, flags | O_NONBLOCK);

	m_pid = fork();
	if (m_pid < 0) {
		WRITE_TRACE(DBG_FATAL, "fork() : %m");
		nRetCode = PRL_ERR_CT_MIGRATE_INTERNAL_ERROR;
		goto cleanup_5;
	} else if (m_pid == 0) {
		/* this f..ing child calls kill(0, signum) from signal handler */
		if( setpgrp() )
		{
			WRITE_TRACE(DBG_FATAL, "setpgrp() error: %m");
			_exit(-1);
		}
		/* to close all unused descriptors */
		int fdnum;
		struct rlimit rlim;
		if (getrlimit(RLIMIT_NOFILE, &rlim) == 0)
			fdnum = (int)rlim.rlim_cur;
		else
			fdnum = 1024;
		for (i = 3; i < fdnum; ++i) {
			if (	(i == out_fds[1]) ||
				(i == cmd_socks[1]) ||
				(i == data_socks[1]) ||
				(i == tmpl_data_socks[1]) ||
				(i == swap_socks[1]))
				continue;
			close(i);
		}

		/* try to redirect stdin to /dev/null */
		int fd = open("/dev/null", O_RDWR);
		if (fd != -1)
			dup2(fd, STDIN_FILENO);
		/* redirect stderr & stdout to out_fds[1] */
		fcntl(out_fds[1], F_SETFD, ~FD_CLOEXEC);
		dup2(out_fds[1], STDOUT_FILENO);
		dup2(out_fds[1], STDERR_FILENO);

		fcntl(cmd_socks[1], F_SETFD, ~FD_CLOEXEC);
		fcntl(data_socks[1], F_SETFD, ~FD_CLOEXEC);
		fcntl(tmpl_data_socks[1], F_SETFD, ~FD_CLOEXEC);
		fcntl(swap_socks[1], F_SETFD, ~FD_CLOEXEC);
		execvp(QSTR2UTF8(m_sCmd), (char* const*)pArgv);
		WRITE_TRACE(DBG_FATAL, "execvp() error : %m");
		_exit(-1);
	}
	close(out_fds[1]);
	close(cmd_socks[1]);
	close(data_socks[1]);
	close(tmpl_data_socks[1]);
	close(swap_socks[1]);

	while ((pid = waitpid(m_pid, &status, WNOHANG)) == -1)
		if (errno != EINTR)
			break;

	if (pid < 0) {
		WRITE_TRACE(DBG_FATAL, "waitpid() error : %m");
		nRetCode = PRL_ERR_CT_MIGRATE_INTERNAL_ERROR;
		goto cleanup_5;
	}
	m_nFd[PRL_CT_MIGRATE_OUT_FD] = out_fds[0];
	m_nFd[PRL_CT_MIGRATE_CMD_FD] = cmd_socks[0];
	m_nFd[PRL_CT_MIGRATE_DATA_FD] = data_socks[0];
	m_nFd[PRL_CT_MIGRATE_TMPLDATA_FD] = tmpl_data_socks[0];
	m_nFd[PRL_CT_MIGRATE_SWAP_FD] = swap_socks[0];

	for (i = 0; pArgv[i]; ++i)
		free(pArgv[i]);
	delete pArgv;

	return PRL_ERR_SUCCESS;

cleanup_5:
	close(swap_socks[0]);
	close(swap_socks[1]);
cleanup_4:
	close(tmpl_data_socks[0]);
	close(tmpl_data_socks[1]);
cleanup_3:
	close(data_socks[0]);
	close(data_socks[1]);
cleanup_2:
	close(cmd_socks[0]);
	close(cmd_socks[1]);
cleanup_1:
	close(out_fds[0]);
	close(out_fds[1]);
cleanup_0:
	for (i = 0; pArgv[i]; ++i)
		free(pArgv[i]);
	delete pArgv;

	return nRetCode;
}

/*
   So vzmigrate in most of cases send messages only via one socket,
   will read() from socket thus far data for reading exists in this socket.
   After this will go to next socket or to poll().
   Will close socket after EOF and break loop if all sockets are close.
 */
PRL_RESULT Task_VzMigrate::execVzMigrate(
			SmartPtr<IOPackage> pParentPackage,
			IOSendJobInterface *pSendJobInterface,
			IOSendJob::Handle &hJob)
{
	PRL_RESULT nRetCode = PRL_ERR_SUCCESS;
	pid_t pid;
	int status;
	int rc;
	int nFd;
	struct pollfd fds[sizeof(m_nFd)/sizeof(int)];
	int i;

	{
		QMutexLocker _lock(&m_terminateHandleDispPackageTaskMutex);
		/* start incoming dispatcher package handler */
		m_pHandleDispPackageTask = new Task_HandleDispPackage(pSendJobInterface, hJob, m_nFd, m_nFdSize);
		bool bConnected = QObject::connect(m_pHandleDispPackageTask,
				SIGNAL(onDispPackageHandlerFailed(PRL_RESULT, const QString &)),
				SLOT(handleDispPackageHandlerFailed(PRL_RESULT, const QString &)),
				Qt::DirectConnection);
		PRL_ASSERT(bConnected);
		m_pHandleDispPackageTask->start();
	}

	/* will send all packages as response to pParentPackage package */
	m_pPackage = IOPackage::createInstance(CtMigrateCmd, 2, pParentPackage, false);

	do {
		PRL_ASSERT(m_pid != -1);
		pid = waitpid(m_pid, &status, WNOHANG);
		if (pid < 0) {
			if (errno == EINTR)
				continue;
			m_pid = -1;
			if (errno != ECHILD)
				terminateVzMigrate();
			WRITE_TRACE(DBG_FATAL, "waitpid() : %m");
			return PRL_ERR_CT_MIGRATE_INTERNAL_ERROR;
		} else if (pid == m_pid) {
			m_pid = -1;
			terminateHandleDispPackageTask();
			break;
		}

		for (i = 0, nFd = 0; i < m_nFdSize; i++) {
			if (m_nFd[i] == -1)
				continue;
			fds[nFd].fd = m_nFd[i];
			fds[nFd].events = POLLIN;
			nFd++;
		}
		rc = poll(fds, nFd, -1);
		if (rc == -1) {
			/* will ignore poll() error and wait child exit */
			continue;
		}
/* TODO : progress */
		for (i = 0; i < nFd; i++) {
			if (!(fds[i].revents & POLLIN))
				continue;

			if (fds[i].fd == m_nFd[PRL_CT_MIGRATE_OUT_FD]) {
				readOutFromVzMigrate();
			} else if (fds[i].fd == m_nFd[PRL_CT_MIGRATE_CMD_FD]) {
				nRetCode = readFromVzMigrate(PRL_CT_MIGRATE_CMD_FD);
			} else if (fds[i].fd == m_nFd[PRL_CT_MIGRATE_DATA_FD]) {
				nRetCode = readFromVzMigrate(PRL_CT_MIGRATE_DATA_FD);
			} else if (fds[i].fd == m_nFd[PRL_CT_MIGRATE_TMPLDATA_FD]) {
				nRetCode = readFromVzMigrate(PRL_CT_MIGRATE_TMPLDATA_FD);
			} else if (fds[i].fd == m_nFd[PRL_CT_MIGRATE_SWAP_FD]) {
				nRetCode = readFromVzMigrate(PRL_CT_MIGRATE_SWAP_FD);
			} else {
				WRITE_TRACE(DBG_FATAL, "invalid fd in poll(read) : %d", fds[i].fd);
				PRL_ASSERT(false);
			}
			if (PRL_FAILED(nRetCode)) {
				/* it's really trouble - kill child and exit */
				terminateHandleDispPackageTask();
				terminateVzMigrate();
				return nRetCode;
			}
		}
	} while(true);

	if (WIFEXITED(status)) {
		if ((rc = WEXITSTATUS(status))) {
			if ((m_sCmd == PRL_COPT_CT_TEMPLATE_CLIENT) || (m_sCmd == PRL_CT_MIGRATE_CLIENT)) {
				switch (rc) {
				case 9:
					WRITE_TRACE(DBG_FATAL, "CT or CT private or CT root "
							"already exists on the target node");
					nRetCode = PRL_ERR_CT_MIGRATE_TARGET_ALREADY_EXISTS;
					break;
				case 10:
					WRITE_TRACE(DBG_FATAL, "Template not found");
					nRetCode = PRL_ERR_TEMPLATE_NOT_FOUND;
					break;
				case 56:
					WRITE_TRACE(DBG_FATAL, "Insufficient cpu capabilities on destination node");
					nRetCode = PRL_ERR_VM_MIGRATE_NON_COMPATIBLE_CPU_ON_TARGET_SHORT;
					break;
				case 57:
					WRITE_TRACE(DBG_FATAL, "CT has unsupported features");
					nRetCode = PRL_ERR_VM_MIGRATE_NON_COMPATIBLE_CPU_ON_TARGET_SHORT;
					break;
				case 72:
					WRITE_TRACE(DBG_FATAL, "External process in CT");
					nRetCode = PRL_ERR_CT_MIGRATE_EXTERNAL_PROCESS;
					break;
				default:
					WRITE_TRACE(DBG_FATAL, "%s exited with code %d", QSTR2UTF8(m_sCmd), rc);
					nRetCode = PRL_ERR_CT_MIGRATE_INTERNAL_ERROR;
				}
			} else {
				WRITE_TRACE(DBG_FATAL, "%s exited with code %d", QSTR2UTF8(m_sCmd), rc);
				nRetCode = PRL_ERR_CT_MIGRATE_INTERNAL_ERROR;
			}
		}
	} else if (WIFSIGNALED(status)) {
		WRITE_TRACE(DBG_FATAL, "%s got signal %d", QSTR2UTF8(m_sCmd), WTERMSIG(status));
		nRetCode = PRL_ERR_CT_MIGRATE_INTERNAL_ERROR;
	} else {
		WRITE_TRACE(DBG_FATAL, "%s exited with status %d", QSTR2UTF8(m_sCmd), status);
		nRetCode = PRL_ERR_CT_MIGRATE_INTERNAL_ERROR;
	}
	return nRetCode;
}

void Task_VzMigrate::terminateVzMigrate()
{
	pid_t pid = m_pid;

	if (pid == -1)
		return;

	time_t t0 = time(NULL);
	int ret = -1;

	QMutexLocker _lock(&m_terminateVzMigrateMutex);
	// give to vzmdest chance to complete undo operations during timeout of 300 sec
	WRITE_TRACE(DBG_FATAL, "Sending SIGTERM to %d...", pid);
	::kill(pid, SIGTERM);
	while ((time(NULL) - t0) < 300) {
		ret = waitpid(pid, NULL, WNOHANG);
		if (ret == pid)
			break;
		if (ret == -1)
			break;
		sleep(1);
	}
	if (ret != pid) {
		// vzmdest may have zombie with other PGID (rm, as sample)
		WRITE_TRACE(DBG_FATAL, "Sending SIGKILL to %d...", pid);
		::kill(pid, SIGKILL);
		while (waitpid(pid, NULL, 0) == -1)
			if (errno != EINTR)
				break;
	}
	for (int i = 0; i < m_nFdSize; i++)
		shutdown(m_nFd[i], SHUT_RDWR);
}

void Task_VzMigrate::terminateHandleDispPackageTask()
{
	QMutexLocker _lock(&m_terminateHandleDispPackageTaskMutex);

	Task_HandleDispPackage *pHandleDispPackageTask = m_pHandleDispPackageTask;
	if (pHandleDispPackageTask == NULL)
		return;
	m_pHandleDispPackageTask = NULL;

	QObject::disconnect(pHandleDispPackageTask,
		SIGNAL(onDispPackageHandlerFailed(PRL_RESULT, const QString &)),
		this,
		SLOT(handleDispPackageHandlerFailed(PRL_RESULT, const QString &)));

	pHandleDispPackageTask->urgentResponseWakeUp();
	if (!pHandleDispPackageTask->wait(m_nTimeout)) {
		WRITE_TRACE(DBG_FATAL, "wake up of responce waiting failed, task will terminate");
		pHandleDispPackageTask->terminate();
		pHandleDispPackageTask->wait();
	}
	delete pHandleDispPackageTask;
}

/* handler of Task_HandleDispPackage failure */
void Task_VzMigrate::handleDispPackageHandlerFailed(PRL_RESULT nRetCode, const QString &sErrInfo)
{
	terminateVzMigrate();
	setLastErrorCode(nRetCode);
	getLastError()->fromString(sErrInfo);
}

Task_HandleDispPackage::Task_HandleDispPackage(
		IOSendJobInterface *pSendJobInterface,
		IOSendJob::Handle &hJob,
		int *nFd,
		int nFdSize)
:m_pSendJobInterface(pSendJobInterface),
m_hJob(hJob)
{
	m_nFd = nFd;
	m_nFdSize = nFdSize;
	m_bActiveFd = new bool[nFdSize];
	for (int i = 0; i < nFdSize; ++i)
		m_bActiveFd[i] = true;
}

PRL_RESULT Task_HandleDispPackage::writeToVzMigrate(quint16 nFdNum, char *data, quint32 size)
{
	int rc;
	size_t count;
	struct pollfd fds[1];

	if (nFdNum >= m_nFdSize) {
		WRITE_TRACE( DBG_FATAL, "Invalid FD type : %d", nFdNum);
		return PRL_ERR_CT_MIGRATE_INTERNAL_ERROR;
	}
	if (!m_bActiveFd[nFdNum]) {
		/* we could end up here when there're several buffered packets, but
		   the fd is already closed, thus we can't write the data from the packets */
		WRITE_TRACE(DBG_INFO, "fd#%d already disconnected, discarding data", m_nFd[nFdNum]);
		return PRL_ERR_SUCCESS;
	}
	if (m_nFd[nFdNum] == -1) {
		WRITE_TRACE(DBG_FATAL, "write() to closed socket");
		if (nFdNum == PRL_CT_MIGRATE_SWAP_FD) {
			/* swap fd reader already closed the swap fd, not an error */
			m_bActiveFd[nFdNum] = false;
			return PRL_ERR_SUCCESS;
		}
		return PRL_ERR_CT_MIGRATE_INTERNAL_ERROR;
	}
	if (size == 0)
		return PRL_ERR_SUCCESS;

	fds[0].fd = m_nFd[nFdNum];
	fds[0].events = POLLOUT;
	count = 0;
	while (1) {
		while (1) {
			errno = 0;
			rc = write(m_nFd[nFdNum], data + count, (size_t)(size - count));
			if (rc > 0) {
				count += rc;
				if (count >= size)
					return PRL_ERR_SUCCESS;
				continue;
			}
			if (errno == EAGAIN) {
				break;
			} else if (errno == EINTR) {
				continue;
			} else {
				WRITE_TRACE(DBG_FATAL, "write() : %m");
				return PRL_ERR_CT_MIGRATE_INTERNAL_ERROR;
			}
		}

		while (true) {
			rc = poll(fds, 1, -1);
			if (rc < 0) {
				WRITE_TRACE(DBG_FATAL, "poll(write) : %m");
				return PRL_ERR_CT_MIGRATE_INTERNAL_ERROR;
			}
			/* If the writer poll() returns before reader poll(), then we'll have
			 * POLLHUP for the swap fd here. Otherwise - POLLNVAL.
			 */
			if (nFdNum == PRL_CT_MIGRATE_SWAP_FD
					&& (fds[0].revents & POLLHUP || fds[0].revents & POLLNVAL)) {
				WRITE_TRACE(DBG_INFO, "poll(write): connection closed for fd#%d", fds[0].fd);
				m_bActiveFd[nFdNum] = false;
				return PRL_ERR_SUCCESS;
			}
			if (fds[0].revents & POLLOUT) {
				break;
			} else if (fds[0].revents) {
				WRITE_TRACE(DBG_FATAL, "poll(write) : revent = %d", fds[0].revents);
				return PRL_ERR_CT_MIGRATE_INTERNAL_ERROR;
			}
		}
	}

	/* but we never should be here */
	return PRL_ERR_CT_MIGRATE_INTERNAL_ERROR;
}

void Task_HandleDispPackage::run()
{
	PRL_RESULT nRetCode = PRL_ERR_SUCCESS;
	SmartPtr<IOPackage> p;
	IOSendJob::Response pResponse;
	IOPackage::EncodingType nEnc;
	SmartPtr<char> pBuffer;
	quint32 nSize;
	quint16 *pnFdNum;
	QString sErrInfo;
	IOSendJob::Result nResult;

	setTerminationEnabled(true);

	while (nRetCode == PRL_ERR_SUCCESS)
	{
		nResult = m_pSendJobInterface->waitForResponse(m_hJob);
		if (nResult == IOSendJob::UrgentlyWaked) {
			WRITE_TRACE(DBG_DEBUG, "IOSendJob::waitForResponse() was waked, task completed");
			return;
		} else if (nResult != IOSendJob::Success) {
			WRITE_TRACE(DBG_FATAL, "IOSendJob::waitForResponse() failure");
			nRetCode = PRL_ERR_CT_MIGRATE_INTERNAL_ERROR;
			break;
		}
		pResponse = m_pSendJobInterface->takeResponse(m_hJob);
		if (pResponse.responseResult != IOSendJob::Success) {
			WRITE_TRACE(DBG_FATAL, "IOSendJob::takeResponse() failure");
			nRetCode = PRL_ERR_CT_MIGRATE_INTERNAL_ERROR;
			break;
		}

		foreach(p, pResponse.responsePackages) {
			PRL_ASSERT(p->buffers[0].getImpl());
			if (p->header.type == DispToDispResponseCmd) {
				/* handle error message */
				CDispToDispCommandPtr pCmd =
					CDispToDispProtoSerializer::ParseCommand(
						DispToDispResponseCmd,
						UTF8_2QSTR(p->buffers[0].getImpl()));
				CDispToDispResponseCommand *pRespCmd =
					CDispToDispProtoSerializer::
						CastToDispToDispCommand<CDispToDispResponseCommand>(pCmd);
				nRetCode = pRespCmd->GetRetCode();
				sErrInfo = pRespCmd->GetErrorInfo()->toString();
				break;
			} else if (p->header.type != CtMigrateCmd) {
				WRITE_TRACE(DBG_FATAL, "Invalid package type : %d", p->header.type);
				nRetCode = PRL_ERR_CT_MIGRATE_INTERNAL_ERROR;
				break;
			}
			pnFdNum = (quint16 *)p->buffers[0].getImpl();
			p->getBuffer(1, nEnc, pBuffer, nSize);

			if (nSize) {
				PRL_ASSERT(pBuffer.getImpl());
				/* write to command channel */
				nRetCode = writeToVzMigrate(*pnFdNum, pBuffer.getImpl(), nSize);
			} else {
				/* empty package indicate that this descriptor closed on remote side.
				   close this descriptor on our side too */
				shutdown(m_nFd[*pnFdNum], SHUT_RDWR);
			}
			if (PRL_FAILED(nRetCode))
				break;
		}
		if (PRL_FAILED(nRetCode))
			break;
	}
	emit onDispPackageHandlerFailed(nRetCode, sErrInfo);
}
