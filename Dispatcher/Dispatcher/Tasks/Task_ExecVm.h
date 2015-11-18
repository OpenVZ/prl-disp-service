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
///////////////////////////////////////////////////////////////////////////////
// class Task_ResponseProcessor

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


namespace Exec {

///////////////////////////////////////////////////////////////////////////////
// struct Stdin

struct Stdin: boost::static_visitor<PRL_RESULT>
{
	Stdin(const SmartPtr<IOPackage>& package_, Task_ExecVm& task_):
		m_task(&task_), m_package(package_)
	{
	}

	template <class T>
	PRL_RESULT operator()(T& mode_) const;

private:
	Task_ExecVm* m_task;
	SmartPtr<IOPackage> m_package;
};

///////////////////////////////////////////////////////////////////////////////
// struct Ct

struct Ct {
	Ct();
	~Ct();

	int sendStdData(Task_ExecVm*, int &fd, int type);
	PRL_RESULT processStd(Task_ExecVm*);
	PRL_RESULT runCommand(
		CProtoVmGuestRunProgramCommand* req, const QString& uuid, int flags);
	void closeStdin(Task_ExecVm*);
	PRL_RESULT processStdinData(const char * data, size_t size);
	CVzExecHelper& getExecer()
	{
		return m_exec;
	}

private:
	int m_stdinfd[2];
	int m_stdoutfd[2];
	int m_stderrfd[2];
	CVzExecHelper m_exec;
};

///////////////////////////////////////////////////////////////////////////////
// struct Vm

struct Vm
{
	typedef Libvirt::Tools::Agent::Vm::Exec::Future
		Future;
	typedef boost::optional<Libvirt::Tools::Agent::Vm::Exec::Result>
		Result;
	typedef Libvirt::Tools::Agent::Vm::Guest
		Guest;

	void closeStdin(Task_ExecVm*);
	PRL_RESULT processStdinData(const char * data, size_t size);
	const QByteArray& getStdin() const
	{
		return m_stdindata;
	}

private:
	QByteArray m_stdindata;
};

///////////////////////////////////////////////////////////////////////////////
// struct Run

struct Run: boost::static_visitor<PRL_RESULT>
{
	explicit Run(Task_ExecVm& task_): m_task(&task_)
	{
	}

	PRL_RESULT operator()(Exec::Ct& variant_) const;
	PRL_RESULT operator()(Exec::Vm& variant_) const;
	PRL_RESULT processVmResult(const Libvirt::Tools::Agent::Vm::Exec::Result& s) const;

private:
	Task_ExecVm* m_task;
};

typedef boost::variant<Exec::Vm, Exec::Ct> Mode;

} // namespace Exec

///////////////////////////////////////////////////////////////////////////////
// class Task_ExecVm

class Task_ExecVm : public CDspTaskHelper
{
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
	void setExitCode(int code) { m_exitcode = code; }
	bool waitForStage(const char* what, unsigned int timeout = 0);
	void wakeUpStage();

private:
	CProtoCommandDspWsResponse *getResponseCmd();
	virtual PRL_RESULT prepareTask();
	virtual PRL_RESULT run_body();
	virtual void finalizeTask();
	virtual void cancelOperation(SmartPtr<CDspClient>, const SmartPtr<IOPackage> &);
	PRL_RESULT startResponseProcessor();

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
};

#endif	// __Task_ExecVm_H__

