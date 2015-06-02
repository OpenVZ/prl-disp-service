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
///	Task_CommitUnfinishedDiskOp.h
///
/// @brief
///	Definition of the class Task_CommitUnfinishedDiskOp
///
/// @brief
///	This class implements long running tasks helper class
///
/// @author sergeyt
///	Ilya@
///
////////////////////////////////////////////////////////////////////////////////

#ifndef __Task_CommitUnfinishedDiskOp_H_
#define __Task_CommitUnfinishedDiskOp_H_

#include "CDspVm.h"
#include "CDspTaskHelper.h"
#include "XmlModel/VmConfig/CVmConfiguration.h"
//#include "Libraries/VirtualDisk/DiskStatesManager.h"  // VirtualDisk commented out by request from CP team

class Task_CommitUnfinishedDiskOp : public CDspTaskHelper
{

   Q_OBJECT

public:

   Task_CommitUnfinishedDiskOp(SmartPtr<CDspClient>&, const SmartPtr<IOPackage>&, bool bNeedStartVm = false);

   ~Task_CommitUnfinishedDiskOp();

   virtual QString getVmUuid();

   /** Wait for wakeup from HDD callback */
   void waitTask ();

   /** Wakes up task from callback */
   void wakeTask ();

   /** if no error occurred return PRL_ERR_SUCCESS */
   PRL_RESULT getHddErrorCode();

   void setHddErrorCode( PRL_RESULT errCode );

   void postProgressEvent(uint uiProgress);

   /** Commit unfinished disk operation in synchronous mode */
   static PRL_RESULT commitDiskOpSync(SmartPtr<CDspClient>, const SmartPtr<IOPackage> &);

	// Commit unfinished disk operations inside a running VM.
	// return task to wait and get result
	static CDspTaskFuture<CDspTaskHelper> pushVmCommit(const CDspVm& vm_);

	static void removeHalfDeletedSnapshots(
		const SmartPtr<CVmConfiguration>& pVmConfig, const QSet<QString>& snapshotIds);
protected:

	virtual PRL_RESULT prepareTask();

	virtual void finalizeTask();

	virtual PRL_RESULT run_body();

private:

	PRL_RESULT commitUnfinishedDiskOp();

	void postEventToClient(PRL_EVENT_TYPE evtType);

	PRL_RESULT prepareSnapshotDirectory( QString& /* OUT */ sSnapshotsDir );

	PRL_RESULT getDevices();
private:

	SmartPtr<CVmConfiguration> m_pVmConfig;

// VirtualDisk commented out by request from CP team
//	CDSManager *m_pStatesManager;

	QSemaphore  m_semaphore;

	PRL_RESULT m_hddErrCode;

	QString m_SnapshotsDir;

	bool m_bNeedStartVm;

	QSet<QString> m_deletedSnapshots;

	QList<CVmHardDisk* > m_devices;
};

#endif //__Task_CommitUnfinishedDiskOp_H_
