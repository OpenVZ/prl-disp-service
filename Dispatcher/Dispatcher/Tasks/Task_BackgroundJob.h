///////////////////////////////////////////////////////////////////////////////
///
/// @file Task_BackgroundJob.h
///
/// Dispatcher task for send question and wait answer from another thread
///
/// @author sergeyt
/// @owner sergeym@
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

#ifndef __Task_BackgroundJob_H_
#define __Task_BackgroundJob_H_

#include "CDspTaskHelper.h"
#include "Libraries/PrlCommonUtils/PrlProcess.h"
#include "CDspVm.h"

class CParallelsNetworkConfig;

class Task_BackgroundJob : public  CDspTaskHelper
{
	Q_OBJECT

public:
	/**
	* Class constructor
	* @param pointer to the user session object
	* @param pointer to request package
	*/
	Task_BackgroundJob(
	const SmartPtr<CDspClient> &pUser,
        const SmartPtr<IOPackage> &pRequestPkg
		);
	virtual ~Task_BackgroundJob() {}

protected:
	/**
	* Task body
	* @return result code of task completion
	*/
	virtual PRL_RESULT run_body();

	virtual void finalizeTask();

private:
	/**
	 * Template method - override it and do necessary actions at inheritances
	 */
	virtual PRL_RESULT ConcreteDoBackgroundJob();
};

class Task_StartVncServer : public Task_BackgroundJob
{
public:
	/**
	* Class constructor
	* @param pointer to the user session object
	* @param pointer to request package
	* @param pointer to the parent VM process maintainer object
	* @param sign whether request performing from VM process side
	*/
	Task_StartVncServer(SmartPtr<CDspClient> &pUser,
        const SmartPtr<IOPackage> &pRequestPkg,
		const SmartPtr<CDspVm> &pVm,
		bool bIsRequestFromVm
		);
	virtual ~Task_StartVncServer() {}

private:
	/**
	 * Overridden template method
	 */
	virtual PRL_RESULT ConcreteDoBackgroundJob();

protected:
	SmartPtr<CDspVm> m_pVm;
	bool m_bIsRequestFromVm;
};

class Task_StopVncServer : public Task_StartVncServer
{
public:
	/**
	* Class constructor
	* @param pointer to the user session object
	* @param pointer to request package
	* @param pointer to the parent VM process maintainer object
	* @param sign whether request performing from VM process side
	*/
	Task_StopVncServer(SmartPtr<CDspClient> &pUser,
        const SmartPtr<IOPackage> &pRequestPkg,
		const SmartPtr<CDspVm> &pVm,
		bool bIsRequestFromVm
		);

protected:
	virtual void cancelOperation(SmartPtr<CDspClient>, const SmartPtr<IOPackage>&);

private:
	/**
	 * Overridden template method
	 */
	PRL_RESULT ConcreteDoBackgroundJob();
};

class Task_AuthUserWithGuestSecurityDb : public Task_BackgroundJob
{
public:
	/**
	* Class constructor
	* @param pointer to the user session object
	* @param pointer to request package
	* @param pointer to the protocol command object
	*/
	Task_AuthUserWithGuestSecurityDb(const SmartPtr<CDspClient> &pUser,
        const SmartPtr<IOPackage> &pRequestPkg,
		const CProtoCommandPtr &pCmd
		);

private:
	/**
	 * Overridden template method
	 */
	PRL_RESULT ConcreteDoBackgroundJob();

private:
	/**
	 * Pointer to the protocol command object
	 */
	CProtoCommandPtr m_pCmd;
};

class Task_ApplyVMNetworking: public Task_BackgroundJob
{
public:
	/**
	* Class constructor
	* @param pointer to the user session object
	* @param pointer to the VM configuration object
	*/
	Task_ApplyVMNetworking(
		const SmartPtr<CDspClient> &pUser,
		const SmartPtr<IOPackage> &p,
		const SmartPtr<CVmConfiguration> &pVmConfig,
		bool bPauseMode
		);

private:
	/**
	 * Overridden template method
	 */
	PRL_RESULT ConcreteDoBackgroundJob();

private:
	/**
	 * Pointer to the VM configuration object
	 */
	SmartPtr<CVmConfiguration> m_pVmConfig;
	bool m_bPauseMode;
};

PRL_RESULT announceMacAddresses(SmartPtr<CVmConfiguration> &pVmConfig);


class CVmNetworkRates;
class Task_NetworkShapingManagement: public Task_BackgroundJob
{
public:
	Task_NetworkShapingManagement(
		const SmartPtr<CDspClient> &pUser,
		const SmartPtr<IOPackage> &p,
		const SmartPtr<CVmConfiguration> &pVmConfig
		);


	static PRL_RESULT getVmNetworkRates(const CVmConfiguration &config_, CVmNetworkRates &lstRates);
	static PRL_RESULT setNetworkRate(const CVmConfiguration &config_);
	static PRL_RESULT getDefaultNetworkRates(CVmNetworkRates *defRates);

private:

	/**
	 * Overridden template method
	 */
	PRL_RESULT ConcreteDoBackgroundJob();
	PRL_RESULT run_task(SmartPtr<CVmConfiguration> pVmConfig);


private:
	/**
	 * List of pointers to the VM configuration object
	 */
	SmartPtr<CVmConfiguration> m_pVmConfig;
};

class Task_CalcVmSize : public Task_BackgroundJob
{
public:
	Task_CalcVmSize(
		const SmartPtr<CDspClient> &pUser,
		const SmartPtr<IOPackage> &p
		);

	static PRL_RESULT getVmSize(
		const SmartPtr<CVmConfiguration> &pVmCfg,
		const SmartPtr<CDspClient> &pUser,
		PRL_UINT64 &nVmSize,
		PRL_UINT32 nFlags = 0
		);

private:
	/**
	 * Overridden template method
	 */
	PRL_RESULT ConcreteDoBackgroundJob();
	void finalizeTask();

private:
	/**
	 * Calculated VM HDDs size
	 */
	PRL_UINT64 m_nVmsHddsSize;
};


class Task_SendSnapshotTree : public Task_BackgroundJob
{
public:
	Task_SendSnapshotTree(
		const SmartPtr<CDspClient> &pUser,
		const SmartPtr<IOPackage> &p
		);
private:
	/**
	* Overridden template method
	*/
	PRL_RESULT ConcreteDoBackgroundJob();
};

class Task_AutoconnectUsbDevice : public Task_BackgroundJob
{
public:
	virtual ~Task_AutoconnectUsbDevice() {}

	/**
	* Class producer
	* @param pointer to the parent VM process maintainer object
	* @param system name of USB device to connect
	* @param friendly name of USB device to connect
	*/
	static Task_AutoconnectUsbDevice * createTask( SmartPtr<CDspVm> &pVm, QString id, QString name );
private:
	/**
	* Class constructor
	* @param pointer to the user session object
	* @param pointer to request package
	* @param pointer to the parent VM process maintainer object
	*/
	Task_AutoconnectUsbDevice(SmartPtr<CDspClient> &pUser,
		const SmartPtr<IOPackage> &pRequestPkg,
		const SmartPtr<CDspVm> &pVm
		);
	/**
	 * Overridden template method
	 */
	virtual PRL_RESULT ConcreteDoBackgroundJob();

protected:
	SmartPtr<CDspVm> m_pVm;
};

class Task_RunVmAction : public Task_BackgroundJob
{
public:
	Task_RunVmAction(
			SmartPtr<CDspClient>& pUser,
			const SmartPtr<IOPackage>& pRequestPkg,
			const SmartPtr<CDspVm> &pVm,
			PRL_VM_ACTION nAction,
			const QString &sUserName);

	virtual void cancelOperation(SmartPtr<CDspClient> pUserSession, const SmartPtr<IOPackage> &p);

private:
	PRL_RESULT RunAction(
			const QString &VmUuid,
			const QString &VmDirUuid,
			PRL_VM_ACTION nAction);
	/**
	 * Overridden template method
	 */
	virtual PRL_RESULT ConcreteDoBackgroundJob();
	void finalizeTask();

	PRL_RESULT RunScript(const QString &sScript, const QStringList &lstEnv);

private:
	SmartPtr<CDspVm> m_pVm;
	PRL_VM_ACTION m_nAction;
	QString m_sUserName;
	PrlProcess m_proc;
};

class Task_SendHostHardwareInfo: public Task_BackgroundJob
{
public:
	Task_SendHostHardwareInfo(
		const SmartPtr<CDspClient> &pUser,
		const SmartPtr<IOPackage> &p
		);
private:
	/**
	* Overridden template method
	*/
	virtual PRL_RESULT ConcreteDoBackgroundJob();
	// to skip send response here
	virtual void finalizeTask() {}
};

class Task_PendentClientRequest: public Task_BackgroundJob
{
public:
	Task_PendentClientRequest(
		const SmartPtr<CDspClient> &pUser,
		const SmartPtr<IOPackage> &p
		);
private:
	/**
	* Overridden template method
	*/
	virtual PRL_RESULT ConcreteDoBackgroundJob();
	// to skip send response here
	virtual void finalizeTask() {}
};

#endif //__Task_BackgroundJob_H_
