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
///	Task_CreateSnapshot.h
///
/// @brief
///	Definition of the class CDspTaskHelper
///
/// @brief
///	This class implements long running tasks helper class
///
/// @author sergeyt
///	Ilya@
///
////////////////////////////////////////////////////////////////////////////////

#ifndef __Task_CreateSnapshot_H_
#define __Task_CreateSnapshot_H_

#include "CDspVm.h"
#include "CDspTaskHelper.h"
#include <prlxmlmodel/VmConfig/CVmConfiguration.h>
//#include "Libraries/VirtualDisk/DiskStatesManager.h"  // VirtualDisk commented out by request from CP team

class Task_CreateSnapshot : public CDspTaskHelper
{
   Q_OBJECT
public:
   Task_CreateSnapshot (const SmartPtr<CDspClient>&, const SmartPtr<IOPackage>&, VIRTUAL_MACHINE_STATE initialVmState);

	~Task_CreateSnapshot();

   virtual QString getVmUuid();

   /** Wait for wakeup from HDD callback */
   void waitTask ();

   /** Wakes up task from callback */
   void wakeTask ();

   /** if no error occurred return PRL_ERR_SUCCESS */
   PRL_RESULT getHddErrorCode();

   void setHddErrorCode( PRL_RESULT errCode );

   void PostProgressEvent(uint uiProgress);

	static PRL_RESULT CheckAvailableSize( VIRTUAL_MACHINE_STATE nVmState,
		const QString& qsSnapshotsDir, SmartPtr<CVmConfiguration> pVmConfig );

protected:

	virtual PRL_RESULT prepareTask();

	virtual void finalizeTask();

	virtual PRL_RESULT run_body();

	PRL_RESULT IsHasRightsForCreateSnapshot(SmartPtr<CVmConfiguration> pVmConfig, CVmEvent& evtOutError);

private:

	bool CopySuspendedVmFiles(const QString& strVmConfigPath);

	bool CreateDiskSnapshot();

	PRL_RESULT DeleteDiskSnapshot();

	PRL_RESULT sendCreateSnapshotPackageToVm();

	void UpdateVmState(SmartPtr<CDspVm> &pVm);

	bool isBootCampUsed();

	PRL_RESULT prepareSnapshotDirectory( QString& /* OUT */ sSnapshotsDir );

	void notifyVmUsers( PRL_EVENT_TYPE evt );
private:

	SmartPtr<CVmConfiguration> m_pVmConfig;

	QString	m_SnapshotUuid;

	VIRTUAL_MACHINE_STATE m_initialVmState;

// VirtualDisk commented out by request from CP team
//	CDSManager *m_pStatesManager;

	QSemaphore  m_semaphore;

	PRL_RESULT m_hddErrCode;

	QString m_SnapshotsDir;

	uint	m_uiProgress;

	quint32 m_nFlags;
	QString m_sPath;
};

#endif //__Task_CreateSnapshot_H_
