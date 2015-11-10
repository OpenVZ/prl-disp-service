///////////////////////////////////////////////////////////////////////////////
///
/// @file Task_ExecVm.h
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

#ifndef __Task_ExecVm_H__
#define __Task_ExecVm_H__

#include "CDspService.h"
#include "CDspVzHelper.h"
#include "CDspTaskHelper.h"
#include "XmlModel/VmDirectory/CVmDirectory.h"
#include "Libraries/ProtoSerializer/CProtoCommands.h"

#include <boost/variant.hpp>

using namespace Parallels;

class Task_ExecVm;
class Task_ResponseProcessor : public CDspTaskHelper
{
	Q_OBJECT

public:
	Task_ResponseProcessor(const SmartPtr<CDspClient>& pClient,
			const SmartPtr<IOPackage>& p, Task_ExecVm *pExec);
	virtual ~Task_ResponseProcessor();
	void waitForStart();

private:
	virtual PRL_RESULT prepareTask();
	virtual PRL_RESULT run_body();
	virtual void cancelOperation(SmartPtr<CDspClient>, const SmartPtr<IOPackage> &);

public slots:
	void slotProcessStdin(const SmartPtr<IOPackage>& p);
	void slotProcessFin();

private:
	bool m_bStarted;
	QString m_sSessionUuid;
	QString m_sGuestSessionUuid;
	Task_ExecVm *m_pExec;
	QMutex m_mutex;
	QWaitCondition m_cond;
};


class Task_ExecVm;

namespace Exec {

struct Ct {
	Ct();
	~Ct();
	int sendStdData(Task_ExecVm*, int &fd, int type);
	PRL_RESULT processStd(Task_ExecVm*);
	PRL_RESULT runCommand(
		CProtoVmGuestRunProgramCommand* req, const QString& uuid, int flags);
	void closeStdin();
	PRL_RESULT processStdinData(const char * data, size_t size);
	CVzExecHelper& getExecer() { return m_exec; }

	int m_stdinfd[2];
	int m_stdoutfd[2];
	int m_stderrfd[2];
	CVzExecHelper m_exec;
};

struct Vm {
	Vm() : m_pid(-1) {}
	PRL_RESULT runCommand(
		CProtoVmGuestRunProgramCommand* req, const QString& uuid, int flags);
	void closeStdin();
	PRL_RESULT processStdinData(const char * data, size_t size);
	int calculateTimeout(int i) const;
	bool checkCmdFinished(Task_ExecVm*);

	int m_pid;
	QByteArray m_stdindata;
	boost::optional<Libvirt::Tools::Agent::Vm::Guest::ExitStatus> m_exitStatus;
};

struct Run;

typedef boost::variant<Exec::Vm, Exec::Ct> Mode;

} // namespace Exec


class Task_ExecVm : public CDspTaskHelper
{
	friend Exec::Run;
public:
	Task_ExecVm(const SmartPtr<CDspClient>& pClient,
		const SmartPtr<IOPackage>& p, Exec::Mode mode);
	virtual ~Task_ExecVm();
	virtual QString getVmUuid() { return m_sVmUuid; }

	PRL_RESULT sendEvent(int type);
	void processStdin(const SmartPtr<IOPackage>& p);
	const QString &getSessionUuid() const  { return m_sSessionUuid; }
	const QString &getGuestSessionUuid() const  { return m_sGuestSessionUuid; }
	PRL_RESULT sendToClient(int type, const char *data, int size);

private:
	CProtoCommandDspWsResponse *getResponseCmd();
private:
	virtual PRL_RESULT prepareTask();
	virtual PRL_RESULT run_body();
	virtual void finalizeTask();
	virtual void cancelOperation(SmartPtr<CDspClient>, const SmartPtr<IOPackage> &);
	PRL_RESULT startResponseProcessor();

private:
	unsigned int m_nFlags;
	unsigned int m_nTimeout;
	CProtoCommandPtr m_pResponseCmd;
	QMutex m_stageMutex;
	QWaitCondition m_stageCond;
	bool m_stageFinished;
	SmartPtr<CDspClient> m_ioClient;
	CDspTaskFuture<Task_ResponseProcessor> m_pResponseProcessor;
	QString m_sVmUuid;
	QString m_sSessionUuid;
	QString m_sGuestSessionUuid;
	int m_exitcode;
	Exec::Mode m_mode;

public:
	void setExitCode(int code) { m_exitcode = code; }
	bool waitForStage(const char* what, unsigned int timeout = 0);
	void wakeUpStage();
};

#endif	// __Task_ExecVm_H__

