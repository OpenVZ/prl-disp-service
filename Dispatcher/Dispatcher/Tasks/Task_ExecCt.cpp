///////////////////////////////////////////////////////////////////////////////
///
/// @file Task_ExecCt.cpp
///
/// @author igor
///
/// Copyright (c) 2005-2015 Parallels IP Holdings GmbH
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
#include <unistd.h>
#include <fcntl.h>
#include <sys/select.h>
#include <poll.h>

//#define FORCE_LOGGING_ON
#include "Libraries/Logging/Logging.h"

#include "CDspService.h"
#include "CDspIOCtClientHandler.h"
#include "Task_ExecCt.h"
#include <prlsdk/PrlCommandsFlags.h>
#include "Libraries/Std/PrlAssert.h"
#include "Libraries/ProtoSerializer/CProtoSerializer.h"
#include "Libraries/PrlCommonUtils/CFirewallHelper.h"
#include "XmlModel/VmConfig/CVmConfiguration.h"
#include "Libraries/Virtuozzo/CVzHelper.h"
#include "Libraries/IOService/src/IOCommunication/Socket/Socket_p.h"

static int set_nonblock(int fd)
{
	int oldfl;

	if ((oldfl = fcntl(fd, F_GETFL)) == -1)
		return -1;
	return fcntl(fd, F_SETFL, oldfl | O_NONBLOCK);
}

template<>
void CDspTaskFuture<Task_ResponseProcessor>::doStop(Task_ResponseProcessor& task_)
{
	task_.quit();
}

Task_ExecCt::Task_ExecCt(const SmartPtr<CDspClient>& pClient,
		const SmartPtr<IOPackage>& p) :
	CDspTaskHelper(pClient, p),
	m_pResponseCmd(CProtoSerializer::CreateDspWsResponseCommand( getRequestPackage(), PRL_ERR_SUCCESS ))
{

	m_nTimeout = CDspService::instance()->getDispConfigGuard().
			getDispToDispPrefs()->getSendReceiveTimeout() * 1000;
	m_bCancelled = false;
	m_stdinfd[0] = -1;
	m_stdinfd[1] = -1;
	m_stdoutfd[0] = -1;
	m_stdoutfd[1] = -1;
	m_stderrfd[0] = -1;
	m_stderrfd[1] = -1;
	m_bFinProcessed = false;
}

Task_ExecCt::~Task_ExecCt()
{
	if (m_stdinfd[0] != -1)
		close(m_stdinfd[0]);
	if (m_stdinfd[1] !=  -1)
		close(m_stdinfd[1]);
	if (m_stdoutfd[0] != -1)
		close(m_stdoutfd[0]);
	if (m_stdoutfd[1] !=  -1)
		close(m_stdoutfd[1]);
	if (m_stderrfd[0] != -1)
		close(m_stderrfd[0]);
	if (m_stderrfd[1] != -1)
		close(m_stderrfd[1]);

	m_pResponseProcessor.stop();

	CDspService::instance()->getIoCtClientManager().unregisterResponseHandler(
			m_sSessionUuid, m_sGuestSessionUuid);
}

CProtoCommandDspWsResponse *Task_ExecCt::getResponseCmd()
{
	return CProtoSerializer::CastToProtoCommand<CProtoCommandDspWsResponse>( m_pResponseCmd );
}

void Task_ExecCt::cancelOperation(SmartPtr<CDspClient> pUserSession, const SmartPtr<IOPackage> &p)
{
	CDspTaskHelper::cancelOperation(pUserSession, p);

	m_exec.cancel();
	m_pResponseProcessor.abort(pUserSession, p);
	m_bCancelled = true;
	setLastErrorCode(PRL_ERR_OPERATION_WAS_CANCELED);
}

void Task_ExecCt::processStdin(const SmartPtr<IOPackage>& p)
{
	if (p->header.type == PET_IO_STDIN_WAS_CLOSED) {
		LOG_MESSAGE(DBG_WARNING, " -> PET_IO_STDIN_WAS_CLOSED vmuid=%s session=%s",
				QSTR2UTF8(m_sVmUuid),
				QSTR2UTF8(Uuid::toString(p->header.parentUuid)));

		close(m_stdinfd[1]);
		m_stdinfd[1] = -1;
	} else if (p->header.type == PET_IO_STDIN_PORTION) {
		IOPackage::EncodingType type;
		SmartPtr<char> data;
		quint32 size;

		LOG_MESSAGE(DBG_WARNING, "-> PET_IO_STDIN_PORTION vmuuid=%s session=%s size=%d",
				QSTR2UTF8(m_sVmUuid),
				QSTR2UTF8(Uuid::toString(p->header.parentUuid)),
				QSTR2UTF8(m_sSessionUuid), size);
		p->getBuffer(0, type, data, size);

		if (write(m_stdinfd[1], data.getImpl(), size) != size) {
			WRITE_TRACE(DBG_FATAL, "Failed to write %d bytes to stdin (fd=%d): %m",
					size, m_stdinfd[1]);
			setLastErrorCode(PRL_ERR_OPERATION_FAILED);
		}
	}
}

int Task_ExecCt::sendEvent(int type)
{
	CVmEvent event((PRL_EVENT_TYPE) type,
			m_sVmUuid,
			PIE_VIRTUAL_MACHINE);
	SmartPtr<IOPackage> p =
		DispatcherPackage::createInstance(PVE::DspVmEvent, event,
				getRequestPackage());
	IOSendJob::Handle job = getClient()->sendPackage( p );
	if (CDspService::instance()->getIOServer().waitForSend(job, m_nTimeout) != IOSendJob::Success) {
		WRITE_TRACE(DBG_FATAL, "Package sending failure");
		return -1;
	}

	return 0;
}

int Task_ExecCt::sendToClient(int type, const char *data, int size)
{
	LOG_MESSAGE(DBG_WARNING, "-> sendToClient type=%d len=%d", type, size);

	if (!m_ioClient.isValid())
		return 0;

	CVmEvent event((PRL_EVENT_TYPE) type, m_sVmUuid, PIE_VIRTUAL_MACHINE);
	SmartPtr<IOPackage> p = DispatcherPackage::createInstance(
			(PRL_EVENT_TYPE) type, event, getRequestPackage());

	if (data != NULL && size > 0)
		p->fillBuffer(0, IOPackage::RawEncoding, (char *)(data), size);

	IOSendJob::Handle job = m_ioClient->sendPackage( p );
	if (CDspService::instance()->getIOServer().waitForSend(job, m_nTimeout) != IOSendJob::Success) {
		WRITE_TRACE(DBG_FATAL, "Task_ExecCt: package type=%d sending failure", type);
		return PRL_ERR_OPERATION_FAILED;
	}

	return PRL_ERR_SUCCESS;
}

int Task_ExecCt::getStdEvtTypeByFd(int fd)
{
	return fd == m_stdoutfd[0] ? PET_IO_STDOUT_PORTION : PET_IO_STDERR_PORTION;
}

int Task_ExecCt::sendStdData(int &fd)
{
	int len;
	char buf[10240];

	len = read(fd, buf, sizeof(buf));
	if (len < 0) {
		if (errno == EINTR || errno == EAGAIN)
			return 0;
		WRITE_TRACE(DBG_FATAL, "Task_ExecCt read(): %m");
		return -1;
	} else if (len == 0) {
		close(fd);
		fd = -1;
		return 0; // fd is closed
	}

	return sendToClient(getStdEvtTypeByFd(fd), buf, len);
}

PRL_RESULT Task_ExecCt::processStd()
{
	int n;

	close(m_stdinfd[0]);
	m_stdinfd[0] = -1;

	close(m_stdoutfd[1]);
	m_stdoutfd[1] = -1;

	close(m_stderrfd[1]);
	m_stderrfd[1] = -1;

	while (!m_bCancelled) {
		int nfds = 0;
		struct pollfd fds[2]; /* stdout stderr */

		if (PRL_FAILED(getLastErrorCode()))
			return getLastErrorCode();

		if (m_stdoutfd[0] != -1) {
			fds[nfds].fd = m_stdoutfd[0];
			fds[nfds].events = POLLIN;
			++nfds;
		}

		if (m_stderrfd[0] != -1) {
			fds[nfds].fd = m_stderrfd[0];
			fds[nfds].events = POLLIN;
			++nfds;
		}

		if (nfds == 0)
			break;

		n = poll(fds, nfds, 60);
		if (n > 0) {
			for (int i = 0; i < nfds; i++) {
				if (!fds[i].revents & POLLIN)
					continue;

				if (sendStdData(fds[i].fd == m_stdoutfd[0] ? m_stdoutfd[0] : m_stderrfd[0]))
					return PRL_ERR_OPERATION_FAILED;
			}
		} else if (n < 0 && errno != EINTR) {
			WRITE_TRACE(DBG_FATAL, "Task_ExecCt poll(): %m");
			return PRL_ERR_OPERATION_FAILED;
		}
	}

	return PRL_ERR_SUCCESS;
}

PRL_RESULT Task_ExecCt::startResponseProcessor()
{
	Task_ResponseProcessor* p = new Task_ResponseProcessor(getClient(), getRequestPackage(), this);
	p->moveToThread(p);
	m_pResponseProcessor = CDspService::instance()->getTaskManager().schedule(p);
	if (NULL == m_pResponseProcessor.getTask())
		return PRL_ERR_OPERATION_FAILED;

	p->waitForStart();

	return PRL_ERR_SUCCESS;
}

PRL_RESULT Task_ExecCt::execCmd()
{
	PRL_RESULT ret;
	int stdfd[3] = {-1, -1, -1};

	if (pipe(m_stdoutfd) == -1 || pipe(m_stderrfd) == -1 || pipe(m_stdinfd) == -1) {
		WRITE_TRACE(DBG_FATAL, "Task_ExecCt pipe(): %m");
		return PRL_ERR_OPERATION_FAILED;
	}

	if (m_nFlags & PFD_STDIN)
		stdfd[0] = m_stdinfd[0];

	if (m_nFlags & PFD_STDOUT) {
		set_nonblock(m_stdoutfd[0]);
		stdfd[1] = m_stdoutfd[1];
	}

	if (m_nFlags & PFD_STDERR) {
		set_nonblock(m_stderrfd[0]);
		stdfd[2] = m_stderrfd[1];
	}

	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(getRequestPackage());
	CProtoVmGuestRunProgramCommand *pRunCmd =
		CProtoSerializer::CastToProtoCommand<CProtoVmGuestRunProgramCommand>(pCmd);

	ret = m_exec.run_cmd(m_sVmUuid,
			pRunCmd->GetProgramName(),
			pRunCmd->GetProgramArguments(),
			pRunCmd->GetProgramEnvVars(),
			pRunCmd->GetCommandFlags(), stdfd);
	if (PRL_FAILED(ret))
		return PRL_ERR_OPERATION_FAILED;

	if (getRequestFlags() & PFD_STDIN)
		sendEvent(PET_IO_READY_TO_ACCEPT_STDIN_PKGS);

	ret = processStd();
	if (PRL_FAILED(ret))
		return ret;

	if (m_exec.wait()) {
		WRITE_TRACE(DBG_FATAL, "Task_ExecCt wait failed");
		return PRL_ERR_OPERATION_FAILED;
	}

	if (m_bCancelled)
		return PRL_ERR_OPERATION_WAS_CANCELED;

	if (sendToClient(PET_IO_FIN_TO_TRANSMIT_STDOUT_STDERR, NULL, 0))
		return PRL_ERR_OPERATION_FAILED;

	QMutexLocker _lock(&m_mutex);
	if (!m_bFinProcessed && !m_cond.wait(&m_mutex, m_nTimeout)) {
		WRITE_TRACE(DBG_FATAL, "Task_ExecCt failed to wait fin responce");
		return PRL_ERR_TIMEOUT;
	}

	return PRL_ERR_SUCCESS ;
}

PRL_RESULT Task_ExecCt::prepareTask()
{
	PRL_RESULT res = PRL_ERR_SUCCESS;

	do {
		CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(getRequestPackage());
		if (!pCmd->IsValid()) {
			res = PRL_ERR_UNRECOGNIZED_REQUEST;
			break;
		}

		CProtoVmGuestRunProgramCommand *pRunCmd =
			CProtoSerializer::CastToProtoCommand<CProtoVmGuestRunProgramCommand>(pCmd);
		if (!pRunCmd->IsValid()) {
			res =  PRL_ERR_UNRECOGNIZED_REQUEST;
			break;
		}

		m_sSessionUuid = pRunCmd->GetVmSessionUuid();
		m_sGuestSessionUuid = Uuid::toString(getRequestPackage()->header.uuid);

		m_ioClient = CDspService::instance()->getIoCtClientManager().
			getIOUserSession(m_sSessionUuid);
		if (!m_ioClient.isValid()) {
			WRITE_TRACE(DBG_FATAL, "Ct IO client is not found by id %s",
					QSTR2UTF8(pRunCmd->GetVmSessionUuid()));
			res = PRL_ERR_OPERATION_FAILED;
			break;
		}

		SmartPtr<CDspCtResponseHandler> hResponse = CDspService::instance()->getIoCtClientManager().
			registerResponseHandler(m_sSessionUuid, m_sGuestSessionUuid);
		if (!hResponse) {
			WRITE_TRACE(DBG_FATAL, "Unable to register response handle");
			res = PRL_ERR_OPERATION_FAILED;
			break;
		}

		res = startResponseProcessor();
		if (PRL_FAILED(res))
			break;

		m_sVmUuid = pCmd->GetVmUuid();
		m_nFlags = pCmd->GetCommandFlags();
	} while (0);

	setLastErrorCode( res );

	return res;
}

PRL_RESULT Task_ExecCt::run_body()
{
	PRL_RESULT ret = execCmd();
	if (PRL_FAILED(ret))
		WRITE_TRACE(DBG_FATAL, "Exec Action failed with code [%#x][%s]",
			ret, PRL_RESULT_TO_STRING( ret ) );

	setLastErrorCode( ret );

	return ret;
}

void Task_ExecCt::wakeUp()
{
	QMutexLocker _lock(&m_mutex);
	m_bFinProcessed = true;
	m_cond.wakeAll();
}

void Task_ExecCt::finalizeTask()
{
	PRL_RESULT res = getLastErrorCode();
	// Send response
	if ( PRL_FAILED( res ) ) {
		getClient()->sendResponseError( getLastError(), getRequestPackage() );
		return;
	}

	CVmEvent answer;
	answer.addEventParameter( new CVmEventParameter (
				PVE::UnsignedInt,
				QString::number(m_exec.get_retcode()),
				EVT_PARAM_VM_EXEC_APP_RET_CODE ) );
	getResponseCmd()->AddStandardParam(answer.toString());

	getClient()->sendResponse( m_pResponseCmd, getRequestPackage() );
}

/***************************************************************/
Task_ResponseProcessor::Task_ResponseProcessor(const SmartPtr<CDspClient>& pClient,
		const SmartPtr<IOPackage>& p, Task_ExecCt *pExec) :
	CDspTaskHelper(pClient, p),
	m_bStarted(false),
	m_sSessionUuid(pExec->getSessionUuid()),
	m_sGuestSessionUuid(pExec->getGuestSessionUuid()),
	m_pExec(pExec)
{
}

Task_ResponseProcessor::~Task_ResponseProcessor()
{
	QMutexLocker _lock(&m_mutex);
	m_cond.wakeAll();
}

void Task_ResponseProcessor::cancelOperation(SmartPtr<CDspClient> pClient,
		const SmartPtr<IOPackage> &p)
{
	CDspTaskHelper::cancelOperation(pClient, p);
	QThread::exit();
}

void Task_ResponseProcessor::waitForStart()
{
	QMutexLocker _lock(&m_mutex);
	if (!m_bStarted)
		m_cond.wait(&m_mutex);
}

void Task_ResponseProcessor::slotProcessStdin(const SmartPtr<IOPackage>& p)
{
	m_pExec->processStdin(p);
}

void Task_ResponseProcessor::slotProcessFin()
{
	m_pExec->wakeUp();
}

PRL_RESULT Task_ResponseProcessor::prepareTask()
{
	SmartPtr<CDspCtResponseHandler> hResponse = CDspService::instance()->getIoCtClientManager().
		getResponseHandler(m_sSessionUuid, m_sGuestSessionUuid);
	if (!hResponse) {
		WRITE_TRACE(DBG_FATAL, "Unable to get response handle");
		setLastErrorCode(PRL_ERR_OPERATION_FAILED);
		return PRL_ERR_OPERATION_FAILED;
	}

	bool bConnected;

	if (m_pExec->getRequestFlags() & PFD_STDIN) {
		bConnected = QObject::connect(hResponse.getImpl(),
				SIGNAL(onStdinPackageReceived(SmartPtr<IOPackage>)),
				this,
				SLOT(slotProcessStdin(const SmartPtr<IOPackage> &)),
				Qt::QueuedConnection);

		PRL_ASSERT(bConnected);
	}

	bConnected = QObject::connect(hResponse.getImpl(),
		SIGNAL(onFinPackageReceived()),
		this,
		SLOT(slotProcessFin()),
		Qt::QueuedConnection);

	PRL_ASSERT(bConnected);

	/* prepare done */
	QMutexLocker _lock(&m_mutex);
	m_bStarted = true;
	m_cond.wakeAll();

	return PRL_ERR_SUCCESS;
}

PRL_RESULT Task_ResponseProcessor::run_body()
{
	return QThread::exec();
}
