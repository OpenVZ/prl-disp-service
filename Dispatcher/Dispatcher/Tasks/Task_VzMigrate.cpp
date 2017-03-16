///////////////////////////////////////////////////////////////////////////////
///
/// @file Task_MigrateCtTarget.cpp
///
/// Target task for Vm migration
///
/// @author krasnov@
///
/// Copyright (c) 2010-2017, Parallels International GmbH
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
/// Our contact details: Parallels International GmbH, Vordergasse 59, 8200
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
#include <unistd.h>
#include <time.h>
#include <poll.h>
#include <sys/resource.h>

#include "Interfaces/Debug.h"
#include <prlcommon/Interfaces/ParallelsQt.h>
#include <prlcommon/Interfaces/ParallelsNamespace.h>

#include <prlcommon/Logging/Logging.h>

#include "Dispatcher/Dispatcher/Tasks/Task_DispToDispConnHelper.h"

#include "Task_VzMigrate.h"
#include "CDspService.h"
#include <prlcommon/Std/PrlAssert.h>
#include <prlcommon/Std/noncopyable.h>


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

namespace
{
namespace Migrate
{

inline void close_fd(int &fd)
{
	::close(fd);
	fd = -1;
}

struct Endpoint : private noncopyable
{
	explicit Endpoint(int fd)
		: m_fd(fd)
	{}

	~Endpoint()
	{
		close();
	}

	void dup(int fd) const
	{
		dup2(m_fd, fd);
	}

	void setCloseOnExec(bool close) const
	{
		fcntl(m_fd, F_SETFD, (close? FD_CLOEXEC : ~FD_CLOEXEC));
	}

	void close()
	{
		close_fd(m_fd);
	}

	QString toString() const
	{
		return QString::number(m_fd);
	}

	bool operator==(const Endpoint &e) const
	{
		return (e.m_fd == m_fd);
	}

	void setNonBlock() const
	{
		long flags;
		if ((flags = fcntl(m_fd, F_GETFL)) != -1)
			fcntl(m_fd, F_SETFL, flags | O_NONBLOCK);
	}

	int release()
	{
		int tmp = m_fd;
		m_fd = -1;
		return tmp;
	}

	ssize_t read(void *buffer, size_t count) const
	{
		ssize_t r;
		while ((r = ::read(m_fd, buffer, count)) < 0) {
			if (errno != EINTR)
				break;
		}
		return r;
	}

private:
	int m_fd;
};

struct EndpointPair
{
	static EndpointPair* createPipe()
	{
		int f[2] = {-1, -1};
		if (pipe(f) < 0) {
			WRITE_TRACE(DBG_FATAL, "pipe() : %m");
			return NULL;
		}

		return new (std::nothrow) EndpointPair(f);
	}

	static EndpointPair* createSocketPair()
	{
		int f[2] = {-1, -1};
		if (socketpair(AF_UNIX, SOCK_STREAM, 0, f) < 0) {
			WRITE_TRACE(DBG_FATAL, "socketpair() : %m");
			return NULL;
		}
		return new (std::nothrow) EndpointPair(f);
	}

	explicit EndpointPair(int (&fds)[2])
		: m_parent(fds[0]), m_child(fds[1])
	{}

	Endpoint& parent()
	{
		return m_parent;
	}

	Endpoint& child()
	{
		return m_child;
	}

private:
	Endpoint m_parent;
	Endpoint m_child;
};

void exec_cmd(const QStringList &args)
{
	QVector<char *> p(args.size() + 1, NULL);
	for (int i = 0; i < args.size(); ++i)
		p[i] = ::strdup(QSTR2UTF8(args[i]));

	::execvp(QSTR2UTF8(args[0]), p.data());
	WRITE_TRACE(DBG_FATAL, "execvp() error : %m");
	foreach (char *t, p)
		::free(t);
}

} // namespace Migrate
} // namespace anonymous

Task_VzMigrate::Task_VzMigrate(const SmartPtr<CDspClient> &client, const SmartPtr<IOPackage> &p)
:CDspTaskHelper(client, p),
	Task_DispToDispConnHelper(getLastError()),
	m_nFd(PRL_CT_MIGRATE_SWAP_FD + 1, -1),
	m_nBufferSize(PRL_DISP_IO_BUFFER_SIZE),
	m_pid(-1),
	m_pHandleDispPackageTask(NULL),
	m_pfnTermination(&Task_VzMigrate::setPidPolicy)
{
	/* alloc buffer for data from vzmigrate and link this buffer with IO package */
	m_pBuffer = SmartPtr<char>(new char[m_nBufferSize], SmartPtrPolicy::ArrayStorage);
}

Task_VzMigrate::~Task_VzMigrate()
{
	terminateVzMigrate();

	terminateHandleDispPackageTask();

	foreach (int i, m_nFd)
		::close(i);
}

PRL_RESULT Task_VzMigrate::readFromVzMigrate(quint16 nFdNum)
{
	PRL_RESULT nRetCode = PRL_ERR_SUCCESS;
	quint32 nSize = 0;
	ssize_t rc;

	PRL_ASSERT(nFdNum < m_nFd.size());

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
			Migrate::close_fd(m_nFd[nFdNum]);
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
	char pBuffer[BUFSIZ] = {};
	quint32 nSize = 0;
	ssize_t rc;

	while (1) {
		if (operationIsCancelled())
			return;

		errno = 0;
		rc = ::read(m_nFd[PRL_CT_MIGRATE_OUT_FD], pBuffer + nSize, (size_t)(sizeof(pBuffer) - 1 - nSize));
		if (rc > 0) {
			nSize += (quint32)rc;
			if (nSize >= sizeof(pBuffer) - 1)
			{
				WRITE_TRACE(DBG_INFO, "%s", pBuffer);
				nSize = 0;
			}
		} else if (rc == 0) {
			/* end of file - pipe was close, will check client exit code */
			pBuffer[nSize] = '\0';
			WRITE_TRACE(DBG_INFO, "%s", pBuffer);
			Migrate::close_fd(m_nFd[PRL_CT_MIGRATE_OUT_FD]);
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
PRL_RESULT Task_VzMigrate::startVzMigrate(const QString &sCmd, const QStringList &lstArgs,
	const CProgressHepler::callback_type& reporter_)
{
	QScopedPointer<Migrate::EndpointPair> out_fds(Migrate::EndpointPair::createPipe());
	QScopedPointer<Migrate::EndpointPair> err_pipe(Migrate::EndpointPair::createPipe());
	QScopedPointer<Migrate::EndpointPair> cmd_socks(Migrate::EndpointPair::createSocketPair());
	QScopedPointer<Migrate::EndpointPair> data_socks(Migrate::EndpointPair::createSocketPair());
	QScopedPointer<Migrate::EndpointPair> tmpl_data_socks(Migrate::EndpointPair::createSocketPair());
	QScopedPointer<Migrate::EndpointPair> swap_socks(Migrate::EndpointPair::createSocketPair());

	QScopedPointer<Migrate::EndpointPair> progressPipe;
	if (!reporter_.empty())
		progressPipe.reset(Migrate::EndpointPair::createPipe());

	m_sCmd = sCmd;

	if (out_fds.isNull() || cmd_socks.isNull() || data_socks.isNull() ||
		tmpl_data_socks.isNull() || swap_socks.isNull() || err_pipe.isNull())
		return PRL_ERR_CT_MIGRATE_INTERNAL_ERROR;

	pid_t p = ::fork();
	if (p < 0) {
		WRITE_TRACE(DBG_FATAL, "fork() : %m");
		return PRL_ERR_CT_MIGRATE_INTERNAL_ERROR;
	} else if (p == 0) {
		/* this f..ing child calls kill(0, signum) from signal handler */
		if (::setpgrp())
		{
			WRITE_TRACE(DBG_FATAL, "setpgrp() error: %m");
			_exit(-1);
		}

		QStringList args;
		args.append(m_sCmd);
		args.append("-ps");
		args.append(cmd_socks->child().toString());
		args.append(data_socks->child().toString());
		args.append(tmpl_data_socks->child().toString());
		args.append(swap_socks->child().toString());
		args.append(lstArgs);

		WRITE_TRACE(DBG_INFO, "Run migration command: '%s'", QSTR2UTF8(args.join(" ")));

		/* to close all unused descriptors */
		int fdnum = 1024;
		struct rlimit rlim;
		if (getrlimit(RLIMIT_NOFILE, &rlim) == 0)
			fdnum = (int)rlim.rlim_cur;

		for (int i = 3; i < fdnum; ++i) {
			Migrate::Endpoint e(i);
			if ((e == out_fds->child()) ||
				(e == cmd_socks->child()) ||
				(e == data_socks->child()) ||
				(e == tmpl_data_socks->child()) ||
				(e == swap_socks->child()) ||
				(e == err_pipe->child()) ||
				(progressPipe && e == progressPipe->child()))
				e.release();
		}

		/* try to redirect stdin to /dev/null */
		Migrate::Endpoint(::open("/dev/null", O_RDWR)).dup(STDIN_FILENO);

		out_fds->child().setCloseOnExec(false);
		out_fds->child().dup(STDOUT_FILENO);
		out_fds->child().dup(STDERR_FILENO);

		cmd_socks->child().setCloseOnExec(false);
		data_socks->child().setCloseOnExec(false);
		tmpl_data_socks->child().setCloseOnExec(false);
		swap_socks->child().setCloseOnExec(false);
		err_pipe->child().setCloseOnExec(true);

		cmd_socks->child().release();
		data_socks->child().release();
		tmpl_data_socks->child().release();
		swap_socks->child().release();
		err_pipe->child().release();
		out_fds->child().release();

		if (progressPipe)
		{
			progressPipe->child().setCloseOnExec(false);
			::setenv("VZ_PROGRESS_FD", QSTR2UTF8(QString::number(progressPipe->child().release())), 1);
		}

		Migrate::exec_cmd(args);
		_exit(-1);
	}

	err_pipe->child().close();
	int t;
	if (err_pipe->parent().read(&t, sizeof(t)) != 0) {
		WRITE_TRACE(DBG_FATAL, "read failed : %m");
		{
			QMutexLocker l(&m_terminateVzMigrateMutex);
			terminatePidPolicy(p);
		}
		return PRL_ERR_CT_MIGRATE_INTERNAL_ERROR;
	}
	{
		QMutexLocker l(&m_terminateVzMigrateMutex);
		(this->*m_pfnTermination)(p);
		p = m_pid;
	}

	if (p > 0) {
		pid_t pid;
		int status;
		while ((pid = ::waitpid(p, &status, WNOHANG)) == -1) {
			if (errno != EINTR)
				break;
		}

		if (pid < 0) {
			WRITE_TRACE(DBG_FATAL, "waitpid() error : %m");
			return PRL_ERR_CT_MIGRATE_INTERNAL_ERROR;
		}
	}

	out_fds->parent().setNonBlock();
	cmd_socks->parent().setNonBlock();
	data_socks->parent().setNonBlock();
	tmpl_data_socks->parent().setNonBlock();
	swap_socks->parent().setNonBlock();

	m_nFd[PRL_CT_MIGRATE_OUT_FD]      = out_fds->parent().release();
	m_nFd[PRL_CT_MIGRATE_CMD_FD]      = cmd_socks->parent().release();
	m_nFd[PRL_CT_MIGRATE_DATA_FD]     = data_socks->parent().release();
	m_nFd[PRL_CT_MIGRATE_TMPLDATA_FD] = tmpl_data_socks->parent().release();
	m_nFd[PRL_CT_MIGRATE_SWAP_FD]     = swap_socks->parent().release();

	if (progressPipe)
	{
		progressPipe->child().close();
		m_progress.reset(new CProgressHepler(reporter_, progressPipe->parent().release()));
		m_progress->start();
	}

	return PRL_ERR_SUCCESS;
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
	struct pollfd fds[m_nFd.size()];

	{
		QMutexLocker _lock(&m_terminateHandleDispPackageTaskMutex);
		/* start incoming dispatcher package handler */
		m_pHandleDispPackageTask = new Task_HandleDispPackage(pSendJobInterface, hJob, m_nFd);
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
		{
			QMutexLocker l(&m_terminateVzMigrateMutex);
			if (m_pid <= 0)
				return PRL_ERR_CT_MIGRATE_INTERNAL_ERROR;
			pid = ::waitpid(m_pid, &status, WNOHANG);
			if (pid < 0) {
				if (errno == EINTR)
					continue;
				m_pid = -1;
				WRITE_TRACE(DBG_FATAL, "waitpid() : %m");
				return PRL_ERR_CT_MIGRATE_INTERNAL_ERROR;
			} else if (pid == m_pid) {
				m_pid = -1;
				break;
			}
		}

		int nFd = 0;
		foreach (int fd, m_nFd) {
			if (fd == -1)
				continue;
			fds[nFd].fd = fd;
			fds[nFd].events = POLLIN;
			nFd++;
		}
		rc = ::poll(fds, nFd, -1);
		if (rc == -1) {
			/* will ignore poll() error and wait child exit */
			continue;
		}
/* TODO : progress */
		for (int i = 0; i < nFd; i++) {
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

	terminateHandleDispPackageTask();

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
	pid_t p;

	QMutexLocker l(&m_terminateVzMigrateMutex);
	if (m_pid <= 0) {
		m_pfnTermination = &Task_VzMigrate::terminatePidPolicy;
		return;
	}

	p = m_pid;
	m_pid = -1;
	terminatePidPolicy(p);
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
		QVector<int> &nFd)
:m_pSendJobInterface(pSendJobInterface),
	m_hJob(hJob),
	m_nFd(nFd),
	m_bActiveFd(nFd.size(), true)
{
}

PRL_RESULT Task_HandleDispPackage::writeToVzMigrate(quint16 nFdNum, char *data, quint32 size)
{
	int rc;
	size_t count;
	struct pollfd fds[1];

	if (nFdNum >= m_nFd.size()) {
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
			rc = ::write(m_nFd[nFdNum], data + count, (size_t)(size - count));
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
			rc = ::poll(fds, 1, -1);
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
				::shutdown(m_nFd[*pnFdNum], SHUT_RDWR);
			}
			if (PRL_FAILED(nRetCode))
				break;
		}
		if (PRL_FAILED(nRetCode))
			break;
	}
	emit onDispPackageHandlerFailed(nRetCode, sErrInfo);
}

void Task_VzMigrate::setPidPolicy(pid_t p)
{
	if (p > 0)
		m_pid = p;
	else
		WRITE_TRACE(DBG_FATAL, "Bad pid");
}

void Task_VzMigrate::terminatePidPolicy(pid_t p)
{
	int ret = -1;
	if (p <= 0) {
		WRITE_TRACE(DBG_FATAL, "Bad pid");
		return;
	}
	// give to vzmdest chance to complete undo operations during timeout of 300 sec
	WRITE_TRACE(DBG_FATAL, "Sending SIGTERM to %d...", p);
	::kill(p, SIGTERM);
	time_t t0 = ::time(NULL);
	while ((::time(NULL) - t0) < 300) {
		ret = ::waitpid(p, NULL, WNOHANG);
		if (ret == p)
			break;
		if (ret == -1)
			break;
		::sleep(1);
	}
	if (ret != p) {
		// vzmdest may have zombie with other PGID (rm, as sample)
		WRITE_TRACE(DBG_FATAL, "Sending SIGKILL to %d...", p);
		::kill(p, SIGKILL);
		while (::waitpid(p, NULL, 0) == -1)
			if (errno != EINTR)
				break;
	}
	foreach (int i, m_nFd)
		::shutdown(i, SHUT_RDWR);
}
