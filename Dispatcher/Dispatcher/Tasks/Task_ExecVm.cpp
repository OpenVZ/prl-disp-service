///////////////////////////////////////////////////////////////////////////////
///
/// @file Task_ExecVm.cpp
///
/// @author igor
///
/// Copyright (c) 2005-2017, Parallels International GmbH
/// Copyright (c) 2017-2019 Virtuozzo International GmbH, All rights reserved.
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
/// Our contact details: Virtuozzo International GmbH, Vordergasse 59, 8200
/// Schaffhausen, Switzerland.
///
/////////////////////////////////////////////////////////////////////////////////
#include <unistd.h>
#include <fcntl.h>
#include <sys/select.h>
#include <poll.h>

//#define FORCE_LOGGING_ON
#include <prlcommon/Logging/Logging.h>

#include "CDspService.h"
#include "CDspIOCtClientHandler.h"
#include "Task_ExecVm.h"
#include <prlsdk/PrlCommandsFlags.h>
#include <prlcommon/Std/PrlAssert.h>
#include <prlcommon/ProtoSerializer/CProtoSerializer.h>
#include "Libraries/PrlCommonUtils/CFirewallHelper.h"
#include <prlxmlmodel/VmConfig/CVmConfiguration.h>
#include "Libraries/Virtuozzo/CVzHelper.h"
#include <prlcommon/IOService/IOCommunication/Socket/Socket_p.h>

enum
{
	GUEST_EXEC_MAX_IO_SIZE = 16*1024*1024
};

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

namespace Exec
{
///////////////////////////////////////////////////////////////////////////////
// struct Stdin

template <class T>
PRL_RESULT Stdin::operator()(T& mode_) const
{
	if (PET_IO_STDIN_WAS_CLOSED == m_package->header.type)
	{
		LOG_MESSAGE(DBG_WARNING, " -> PET_IO_STDIN_WAS_CLOSED vmuid=%s session=%s",
				QSTR2UTF8(m_task->getVmUuid()),
				QSTR2UTF8(Uuid::toString(m_package->header.parentUuid)));

		mode_.closeStdin();
	}
	else if (PET_IO_STDIN_PORTION == m_package->header.type)
	{
		IOPackage::EncodingType type;
		SmartPtr<char> data;
		quint32 size = 0;
		m_package->getBuffer(0, type, data, size);

		LOG_MESSAGE(DBG_WARNING, "-> PET_IO_STDIN_PORTION vmuuid=%s session=%s size=%d",
				QSTR2UTF8(m_task->getVmUuid()),
				QSTR2UTF8(Uuid::toString(m_package->header.parentUuid)),
				QSTR2UTF8(m_task->getSessionUuid()), size);

		return mode_.processStdin(data.getImpl(), size);
	}
	return PRL_ERR_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
// struct Run

PRL_RESULT Run::operator()(Exec::Ct& variant_) const
{
	PRL_RESULT ret;
	int flags = m_task->getRequestFlags();
	CProtoCommandPtr p = CProtoSerializer::ParseCommand(m_task->getRequestPackage());
	CProtoVmGuestRunProgramCommand* cmd = CProtoSerializer::CastToProtoCommand<CProtoVmGuestRunProgramCommand>(p);

	ret = variant_.runCommand(cmd, m_task->getVmUuid(), flags);
	if (PRL_FAILED(ret))
		return PRL_ERR_OPERATION_FAILED;

	if (flags & PFD_STDIN)
		m_task->sendEvent(PET_IO_READY_TO_ACCEPT_STDIN_PKGS);

	ret = variant_.processStdout(m_task);
	if (PRL_FAILED(ret))
		return ret;

	if (variant_.getExecer().wait()) {
		WRITE_TRACE(DBG_FATAL, "Task_ExecVm wait failed");
		return PRL_ERR_OPERATION_FAILED;
	}

	if (m_task->operationIsCancelled())
		return PRL_ERR_OPERATION_WAS_CANCELED;

	if (m_task->sendToClient(PET_IO_FIN_TO_TRANSMIT_STDOUT_STDERR, NULL, 0))
		return PRL_ERR_OPERATION_FAILED;

	// complete exec successfully even when fin timeouted
	// we have a valid exit code and guest process finished by now
	m_task->waitForStage("fin response");

	m_task->setExitCode(variant_.getExecer().get_retcode());

	return PRL_ERR_SUCCESS;
}

PRL_RESULT Run::processVmResult(const Libvirt::Instrument::Agent::Vm::Exec::Result& s) const
{
	int flags = m_task->getRequestFlags();

	m_task->setExitCode(s.exitcode);
	if (flags & PFD_STDOUT && s.stdOut.size()) {
		if (m_task->sendToClient(PET_IO_STDOUT_PORTION,
				s.stdOut.data(), s.stdOut.size()))
			return PRL_ERR_OPERATION_FAILED;
	}

	if (flags & PFD_STDERR && s.stdErr.size()) {
		if (m_task->sendToClient(PET_IO_STDERR_PORTION,
				 s.stdErr.data(), s.stdErr.size()))
			return PRL_ERR_OPERATION_FAILED;
	}

	if (m_task->sendToClient(PET_IO_FIN_TO_TRANSMIT_STDOUT_STDERR, NULL, 0))
		return PRL_ERR_OPERATION_FAILED;
	
	if (!m_task->waitForStage("fin response"))
		return PRL_ERR_TIMEOUT;

	return PRL_ERR_SUCCESS;
}

PRL_RESULT Run::operator()(Exec::Vm& variant_) const
{
	CProtoCommandPtr d = CProtoSerializer::ParseCommand(m_task->getRequestPackage());
	CProtoVmGuestRunProgramCommand* cmd = CProtoSerializer::CastToProtoCommand<CProtoVmGuestRunProgramCommand>(d);

	vm::Exec::Request r(cmd->GetProgramName(), cmd->GetProgramArguments());
	// check that the task is not cancelled
	if (PRL_FAILED(m_task->getLastErrorCode()))
		return m_task->getLastErrorCode();

	if (m_task->getRequestFlags() & PRPM_RUN_PROGRAM_IN_SHELL)
		r.setRunInShell();
	r.setEnvironment(cmd->GetProgramEnvVars());

	Prl::Expected<vm::Exec::Result, PRL_RESULT> x = variant_.handle(r, *m_task);
	if (x.isFailed())
		return x.error();

	if (m_task->operationIsCancelled())
		return PRL_ERR_OPERATION_WAS_CANCELED;

	return processVmResult(x.value());
}

///////////////////////////////////////////////////////////////////////////////
// struct Ct

Ct::Ct()
{
	m_stdinfd[0] = -1;
	m_stdinfd[1] = -1;
	m_stdoutfd[0] = -1;
	m_stdoutfd[1] = -1;
	m_stderrfd[0] = -1;
	m_stderrfd[1] = -1;
}

Ct::~Ct()
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
}

PRL_RESULT Ct::runCommand(
	CProtoVmGuestRunProgramCommand* cmd,
	const  QString& uuid,
	int flags)
{
	int stdfd[3] = {-1, -1, -1};

	if (pipe(m_stdoutfd) == -1 || pipe(m_stderrfd) == -1 || pipe(m_stdinfd) == -1) {
		WRITE_TRACE(DBG_FATAL, "Task_ExecVm pipe(): %m");
		return PRL_ERR_OPERATION_FAILED;
	}

	if (flags & PFD_STDIN)
		stdfd[0] = m_stdinfd[0];

	if (flags & PFD_STDOUT) {
		set_nonblock(m_stdoutfd[0]);
		stdfd[1] = m_stdoutfd[1];
	}

	if (flags & PFD_STDERR) {
		set_nonblock(m_stderrfd[0]);
		stdfd[2] = m_stderrfd[1];
	}

	return m_exec.run_cmd(uuid,
			cmd->GetProgramName(),
			cmd->GetProgramArguments(),
			cmd->GetProgramEnvVars(),
			cmd->GetCommandFlags(), stdfd);
}

PRL_RESULT Ct::sendStdData(Task_ExecVm* task, int &fd, int type)
{
	int len;
	char buf[10240];

	len = read(fd, buf, sizeof(buf));
	if (len < 0) {
		if (errno == EINTR || errno == EAGAIN)
			return 0;
		WRITE_TRACE(DBG_FATAL, "Task_ExecVm read(): %m");
		return PRL_ERR_OPERATION_FAILED;
	} else if (len == 0) {
		close(fd);
		fd = -1;
		return 0; // fd is closed
	}

	return task->sendToClient(type, buf, len);
}

PRL_RESULT Ct::processStdout(Task_ExecVm* task)
{
	int n;

	close(m_stdinfd[0]);
	m_stdinfd[0] = -1;

	close(m_stdoutfd[1]);
	m_stdoutfd[1] = -1;

	close(m_stderrfd[1]);
	m_stderrfd[1] = -1;

	while (!task->operationIsCancelled()) {
		int nfds = 0;
		struct pollfd fds[2]; /* stdout stderr */

		if (PRL_FAILED(task->getLastErrorCode()))
			return task->getLastErrorCode();

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
				if (sendStdData(
					task,
					fds[i].fd == m_stdoutfd[0] ?
						m_stdoutfd[0] : m_stderrfd[0],
					fds[i].fd == m_stdoutfd[0] ?
						PET_IO_STDOUT_PORTION : PET_IO_STDERR_PORTION))
					return PRL_ERR_OPERATION_FAILED;
			}
		} else if (n < 0 && errno != EINTR) {
			WRITE_TRACE(DBG_FATAL, "Task_ExecVm poll(): %m");
			return PRL_ERR_OPERATION_FAILED;
		}
	}

	return PRL_ERR_SUCCESS;
}

PRL_RESULT Ct::processStdin(const char * data, size_t size)
{
	if (write(m_stdinfd[1], data, size) != (ssize_t)size) {
		WRITE_TRACE(DBG_FATAL, "Failed to write %d bytes to stdin (fd=%d): %m",
				(int)size, m_stdinfd[1]);
		return PRL_ERR_OPERATION_FAILED;
	}
	return PRL_ERR_SUCCESS;
}

void Ct::closeStdin()
{
	close(m_stdinfd[1]);
	m_stdinfd[1] = -1;
}

///////////////////////////////////////////////////////////////////////////////
// struct Vm

PRL_RESULT Vm::processStdin(const char * data, size_t size)
{
	if (NULL == m_exec)
		return PRL_ERR_FAILURE;

	if (m_exec->getStdin()->write(data, size) < 0)
		return PRL_ERR_VM_GUEST_SESSION_EXPIRED;

	return PRL_ERR_SUCCESS;
}

void Vm::closeStdin()
{
	if (!m_exec.isNull())
		m_exec->getStdin()->close();
}

Prl::Expected<vm::Exec::Result, PRL_RESULT>
	Vm::handle(const vm::Exec::Request& request_, Task_ExecVm& task_)
{
	Prl::Expected<QSharedPointer<Vm::exec_type>, Error::Simple> x =
		Libvirt::Kit.vms().at(task_.getVmUuid()).getGuest().getExec();
	if (x.isFailed())
		return x.error().code();

	QEventLoop l;
	m_exec = x.value();

	Join j(l);
	j.add(new Mediator(task_, m_exec->getStdout(), PET_IO_STDOUT_PORTION));
	j.add(new Mediator(task_, m_exec->getStderr(), PET_IO_STDERR_PORTION));

	Libvirt::Result e = m_exec->start(request_);
	if (e.isFailed())
		return CDspTaskFailure(task_)(e.error().convertToEvent());

	if (task_.getRequestFlags() & PFD_STDIN)
		task_.sendEvent(PET_IO_READY_TO_ACCEPT_STDIN_PKGS);

	Poller p(l, *m_exec);
	p.startTimer(10000);

	l.exec();

	m_exec->getStdout()->close();
	m_exec->getStderr()->close();

	e = m_exec->wait();
	if (e.isFailed())
		return e.error().code();

	return x.value()->getResult();
}

///////////////////////////////////////////////////////////////////////////////
// struct Join

void Join::add(QObject *object_)
{
	bool x = QObject::connect(object_, SIGNAL(finished()), this, SLOT(slotFinished()),
		Qt::DirectConnection);
	PRL_ASSERT(x);
	m_objects.append(QSharedPointer<QObject>(object_));
}

void Join::slotFinished(void)
{
	foreach(QSharedPointer<QObject> p, m_objects) {
		if (p.data() == QObject::sender()) {
			m_objects.removeAll(p);
			break;
		}
	}
	if (m_objects.isEmpty())
		m_loop.quit();
}

///////////////////////////////////////////////////////////////////////////////
// struct Poller

void Poller::timerEvent(QTimerEvent *event)
{
	Q_UNUSED(event);
	Libvirt::Result r = m_exec.wait(0);
	if (r.isFailed())
	{
		if (PRL_ERR_TIMEOUT != r.error().code())
			m_loop.quit();
	}
	else if (m_sign.isSucceed())
		m_loop.quit();
	else
		m_sign = m_exec.getResult();
}

///////////////////////////////////////////////////////////////////////////////
// struct Mediator

void Mediator::slotSendData()
{
	forever {
		QByteArray a = m_object->readAll();
		if (a.isEmpty())
			break;
		m_task->sendToClient(m_iotype, a.constData(), a.size());
	}

	if (m_task->operationIsCancelled() || m_object->atEnd())
		emit finished();
}

void Mediator::slotEof()
{
	slotSendData();
}

///////////////////////////////////////////////////////////////////////////////
// struct Cancel

void Cancel::operator()(Exec::Ct& variant_) const
{
	variant_.getExecer().cancel();
}

void Cancel::operator()(Exec::Vm& variant_) const
{
	variant_.cancel();
}

} // namespace Exec

Task_ExecVm::Task_ExecVm(const SmartPtr<CDspClient>& pClient,
		const SmartPtr<IOPackage>& p, Exec::Mode mode) :
	CDspTaskHelper(pClient, p),
	m_pResponseCmd(CProtoSerializer::CreateDspWsResponseCommand(getRequestPackage(), PRL_ERR_SUCCESS)),
	m_stageFinished(false),
	m_mode(mode)
{
	bool x = QObject::connect(&CDspService::instance()->getIOServer(),
			SIGNAL(onClientDisconnected(IOSender::Handle)),
			SLOT(reactDisconnected(IOSender::Handle)),
			Qt::DirectConnection);
	PRL_ASSERT(x);
	m_nTimeout = CDspService::instance()->getDispConfigGuard().
		getDispToDispPrefs()->getSendReceiveTimeout() * 1000;
}

Task_ExecVm::~Task_ExecVm()
{
	m_pResponseProcessor.stop();

	CDspService::instance()->getIoCtClientManager().unregisterResponseHandler(
			m_sSessionUuid, m_sGuestSessionUuid);
}

void Task_ExecVm::reactDisconnected(IOSender::Handle h)
{
	SmartPtr<CDspClient> p = getClient();
	if (!p.isValid())
		return;
	if (p->getClientHandle() != h)
		return;
	cancelOperation(p, getRequestPackage());
}

CProtoCommandDspWsResponse *Task_ExecVm::getResponseCmd()
{
	return CProtoSerializer::CastToProtoCommand<CProtoCommandDspWsResponse>(m_pResponseCmd);
}

void Task_ExecVm::cancelOperation(SmartPtr<CDspClient> pUserSession, const SmartPtr<IOPackage> &p)
{
	if (operationIsCancelled())
		return;

	CDspTaskHelper::cancelOperation(pUserSession, p);

	boost::apply_visitor(Exec::Cancel(), m_mode);

	m_pResponseProcessor.abort(pUserSession, p);
	setLastErrorCode(PRL_ERR_OPERATION_WAS_CANCELED);
}

PRL_RESULT Task_ExecVm::processStdin(const SmartPtr<IOPackage>& p)
{
	PRL_RESULT e = boost::apply_visitor(Exec::Stdin(p, *this), m_mode);

	if (PRL_FAILED(e))
		setLastErrorCode(e);
	return e;
}

PRL_RESULT Task_ExecVm::sendEvent(int type)
{
	CVmEvent event((PRL_EVENT_TYPE)type, m_sVmUuid, PIE_VIRTUAL_MACHINE);
	SmartPtr<IOPackage> p =
		DispatcherPackage::createInstance(PVE::DspVmEvent, event,
				getRequestPackage());
	IOSendJob::Handle job = getClient()->sendPackage(p);
	if (CDspService::instance()->getIOServer().waitForSend(job, m_nTimeout) != IOSendJob::Success) {
		WRITE_TRACE(DBG_FATAL, "Package sending failure");
		return PRL_ERR_OPERATION_FAILED;
	}

	return PRL_ERR_SUCCESS;
}


PRL_RESULT Task_ExecVm::sendToClient(int type, const char *data, int size)
{
	if (!m_ioClient.isValid()) {
		cancelOperation(getClient(), getRequestPackage());
		return PRL_ERR_OPERATION_FAILED;
	}

	if (NULL == data)
		size = 0;
	do
	{
		CVmEvent event((PRL_EVENT_TYPE)type, m_sVmUuid, PIE_VIRTUAL_MACHINE);
		SmartPtr<IOPackage> p = DispatcherPackage::createInstance(
				(PRL_EVENT_TYPE)type, event, getRequestPackage());
		if (0 < size)
		{
			IOPackage::PODData d;
			d.bufferSize = size;
			d.bufferEncoding = IOPackage::RawEncoding;
			qint32 w = p->stowBuffer(0, (char*)(data), d);
			if (0 > w)
				return PRL_ERR_UNEXPECTED;
			size -= w;
			data += w;
		}
		IOSendJob::Handle job = m_ioClient->sendPackage(p);
		if (CDspService::instance()->getIOServer().waitForSend(job, m_nTimeout) != IOSendJob::Success) {
			WRITE_TRACE(DBG_FATAL, "Task_ExecVm: package type=%d sending failure", type);
			cancelOperation(getClient(), getRequestPackage());
			return PRL_ERR_OPERATION_FAILED;
		}
	} while(0 < size);

	return PRL_ERR_SUCCESS;
}

PRL_RESULT Task_ExecVm::startResponseProcessor()
{
	Task_ResponseProcessor* p = new Task_ResponseProcessor(getClient(), getRequestPackage(), this);
	p->moveToThread(p);
	m_pResponseProcessor = CDspService::instance()->getTaskManager().schedule(p);
	if (NULL == m_pResponseProcessor.getTask())
		return PRL_ERR_OPERATION_FAILED;

	p->waitForStart();

	return PRL_ERR_SUCCESS;
}

PRL_RESULT Task_ExecVm::prepareTask()
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
			WRITE_TRACE(DBG_FATAL, "IO client is not found by id %s",
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

PRL_RESULT Task_ExecVm::run_body()
{
	PRL_RESULT ret;
	
	ret = boost::apply_visitor(Exec::Run(*this), m_mode);

	if (PRL_FAILED(ret))
		WRITE_TRACE(DBG_FATAL, "Exec Action failed with code [%#x][%s]",
			ret, PRL_RESULT_TO_STRING( ret ) );

	setLastErrorCode(ret);
	return ret;
}

void Task_ExecVm::wakeUpStage()
{
	QMutexLocker _lock(&m_stageMutex);
	m_stageFinished = true;
	m_stageCond.wakeAll();
}

bool Task_ExecVm::waitForStage(const char* what, unsigned int timeout)
{
	QMutexLocker _lock(&m_stageMutex);
	WRITE_TRACE(DBG_DEBUG, "wait for: %s", what);
	if (!m_stageFinished && !m_stageCond.wait(&m_stageMutex, timeout ? timeout : m_nTimeout)) {
		WRITE_TRACE(DBG_FATAL, "Task_ExecVm failed to wait %s", what);
		return false;
	}
	// auto-reset
	m_stageFinished = false;
	WRITE_TRACE(DBG_DEBUG, "wait for: %s [COMPLETED]", what);
	return true;
}

void Task_ExecVm::finalizeTask()
{
	QObject::disconnect(
			&CDspService::instance()->getIOServer(),
			SIGNAL(onClientDisconnected(IOSender::Handle)),
			this,
			SLOT(reactDisconnected(IOSender::Handle)));

	PRL_RESULT res = getLastErrorCode();
	// Send response
	if ( PRL_FAILED( res ) ) {
		getClient()->sendResponseError( getLastError(), getRequestPackage() );
		return;
	}

	CVmEvent answer;
	answer.addEventParameter( new CVmEventParameter (
				PVE::UnsignedInt,
				QString::number(m_exitcode),
				EVT_PARAM_VM_EXEC_APP_RET_CODE ) );
	getResponseCmd()->AddStandardParam(answer.toString());

	getClient()->sendResponse( m_pResponseCmd, getRequestPackage() );
}

/***************************************************************/
Task_ResponseProcessor::Task_ResponseProcessor(const SmartPtr<CDspClient>& pClient,
		const SmartPtr<IOPackage>& p, Task_ExecVm *pExec) :
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
	PRL_RESULT e = m_pExec->processStdin(p);
	if (PRL_SUCCEEDED(e))
		return;

	WRITE_TRACE(DBG_DEBUG, "wakeUpStage from slotProcessStdin");
	m_pExec->wakeUpStage();
	QThread::exit(e);
}

void Task_ResponseProcessor::slotProcessFin()
{
	WRITE_TRACE(DBG_DEBUG, "wakeUpStage from slotProcessFin");
	m_pExec->wakeUpStage();
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
		if (!bConnected)
			WRITE_TRACE(DBG_FATAL, "stdin connect fail");
	}

	bConnected = QObject::connect(hResponse.getImpl(),
		SIGNAL(onFinPackageReceived()),
		this,
		SLOT(slotProcessFin()),
		Qt::QueuedConnection);
	PRL_ASSERT(bConnected);
	if (!bConnected)
		WRITE_TRACE(DBG_FATAL, "fin connect fail");

	/* prepare done */
	QMutexLocker _lock(&m_mutex);
	m_bStarted = true;
	m_cond.wakeAll();

	return PRL_ERR_SUCCESS;
}

PRL_RESULT Task_ResponseProcessor::run_body()
{
	setLastErrorCode(QThread::exec());
	return getLastErrorCode();
}
