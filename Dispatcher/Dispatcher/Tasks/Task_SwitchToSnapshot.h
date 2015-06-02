////////////////////////////////////////////////////////////////////////////////
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
/// @file
///	Task_SwitchToSnapshot.h
///
/// @brief
///	Definition of the class Task_SwitchToSnapshot
///
/// @brief
///	This class implements long running tasks helper class
///
/// @author sergeyt
///	Ilya@
///
////////////////////////////////////////////////////////////////////////////////

#ifndef __Task_SwitchToSnapshot_H_
#define __Task_SwitchToSnapshot_H_

#include "CDspVm.h"
#include "CDspTaskHelper.h"
#include "XmlModel/VmConfig/CVmConfiguration.h"
#include "XmlModel/VmInfo/CVmInfo.h"
#include "CDspVmSnapshotInfrastructure.h"
//#include "Libraries/VirtualDisk/DiskStatesManager.h"  // VirtualDisk commented out by request from CP team

class Task_SwitchToSnapshot : public CDspTaskHelper
{
   Q_OBJECT
public:

   Task_SwitchToSnapshot ( SmartPtr<CDspClient>&,
						   const SmartPtr<IOPackage>&,
						   VIRTUAL_MACHINE_STATE initialVmState);

	~Task_SwitchToSnapshot();

   virtual QString getVmUuid();

   /** Wait for wakeup */
   void waitTask ();

   /** Wakes up task */
   void wakeTask ();

   /** if no error occurred return PRL_ERR_SUCCESS */
   PRL_RESULT getHddErrorCode();

   void setHddErrorCode( PRL_RESULT errCode );

   void handleVmEvents(const SmartPtr<IOPackage> &p);

   void handleClientDisconnected( /* OUT */ bool &bNeedUnregisterVmObject );

   bool willTaskUnregisterVmFromMap();

	void PostProgressEvent(uint uiProgress);

protected:

	virtual PRL_RESULT prepareTask();

	virtual void finalizeTask();

	virtual PRL_RESULT run_body();

	PRL_RESULT IsHasRightsForSwitchToSnapshot(SmartPtr<CVmConfiguration> pVmConfig, CVmEvent& evtOutError);

private:

	bool isSnapshotedVmRunning(void);

	bool RestoreSuspendedVmFiles(const QString& strVmConfigPath);

	void DropSuspendedVmFiles(const QString &strVmConfigPath);

	bool RevertDiskToSnapshot(const QString& strVmConfigPath);

	PRL_RESULT sendPackageToVm(const SmartPtr<IOPackage> &pRequestPkg);

	Snapshot::Revert::Command* makeRevertPackage();

	SmartPtr<IOPackage> makeStopPackage();

	void UpdateVmState(SmartPtr<CDspVm> &pVm);

	PRL_RESULT RestoreVmConfig(const QString &sVmConfigPath);

	void releaseDiskManager();

	PRL_RESULT offlineRevertToSnapshot( bool& /* in|out */ flgImpersonated );

	PRL_RESULT CheckAvailableSize(const QString& strVmConfigPath);

	void NotifyConfigChanged();

	PRL_RESULT changeFirewallSettings();

private:

	/** Current VM configuration */
	SmartPtr<CVmConfiguration> m_pVmConfig;
	/** Saved VM configuration */
	SmartPtr<CVmConfiguration> m_pSavedVmConfig;

	SmartPtr<CVmInfo> m_pVmInfo;

	QString	m_SnapshotUuid;

	VIRTUAL_MACHINE_STATE m_initialVmState;
	PVE::SnapshotedVmState m_snapshotedVmState;

// VirtualDisk commented out by request from CP team
//	CDSManager *m_pStatesManager;

	QSemaphore  m_semaphore;

	PRL_RESULT m_hddErrCode;

	QString m_VmConfigPath;

	bool m_bVmStartedByTask;

	QMutex m_mtxVmConnectionWasClosed;
	bool m_bVmConnectionWasClosed;
	bool m_bTaskWasFinished;

	uint	m_uiProgress;
	Snapshot::Revert::Note m_note;
};

#endif //__Task_SwitchToSnapshot_H_
