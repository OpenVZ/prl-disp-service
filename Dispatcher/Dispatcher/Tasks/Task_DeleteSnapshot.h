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
///	Task_DeleteSnapshot.h
///
/// @brief
///	Definition of the class Task_DeleteSnapshot
///
/// @brief
///	This class implements long running tasks helper class
///
///
////////////////////////////////////////////////////////////////////////////////

#ifndef __Task_DeleteSnapshot_H_
#define __Task_DeleteSnapshot_H_

#include "CDspVm.h"
#include "CDspTaskHelper.h"
#include "XmlModel/VmConfig/CVmConfiguration.h"
//#include "Libraries/VirtualDisk/DiskStatesManager.h"  // VirtualDisk commented out by request from CP team

class Task_DeleteSnapshot : public CDspTaskHelper
{
	Q_OBJECT
public:

	Task_DeleteSnapshot ( SmartPtr<CDspClient>&, const SmartPtr<IOPackage>&, VIRTUAL_MACHINE_STATE initialVmState);
	~Task_DeleteSnapshot();

	virtual QString getVmUuid();

	/** Wakes up task from callback */
	void wakeTask ();
	void setHddErrorCode( PRL_RESULT errCode );

	void PostProgressEvent(uint uiProgress);

protected:

	/** Wait for wakeup from HDD callback */
	void waitTask ();

	/** if no error occurred return PRL_ERR_SUCCESS */
	PRL_RESULT getHddErrorCode();

	virtual PRL_RESULT prepareTask();

	virtual void finalizeTask();

	virtual PRL_RESULT run_body();

	PRL_RESULT deleteSnapshot(const QString& strSnapshotUuid, bool bMerge);
	PRL_RESULT switchAndDeleteState(const QString& strSnapshotUuid, bool bMerge, bool bUseOnlyDiskSnapshot);
	PRL_RESULT sendPackageToVm(const QString& strSnapshotUuid, bool bMerge);

	void UpdateVmState(SmartPtr<CDspVm> &pVm);

	SmartPtr<CVmConfiguration> getSavedVmConfigBySnapshotUuid(const QString& uuid, PRL_RESULT& error);

	bool isSnapshotDeletedByCommitUnfinished();

private:

	/** Current VM configuration */
	SmartPtr<CVmConfiguration> m_pVmConfig;

	QString	m_SnapshotUuid;
	VIRTUAL_MACHINE_STATE m_initialVmState;
	QString m_sVmConfigPath;
	QString m_sVmUuid;
	bool m_flgChild;
	bool m_bVmRunning;
	bool m_bSnapLock;

// VirtualDisk commented out by request from CP team
//	CDSManager *m_pStatesManager;

	QSemaphore  m_semaphore;

	PRL_RESULT m_hddErrCode;

	uint	m_uiProgress;
	uint	m_uiStepsCount;
	uint	m_uiStep;

	quint32 m_nFlags;

	bool m_bChildrenExist;
};

#endif //__Task_DeleteSnapshot_H_
