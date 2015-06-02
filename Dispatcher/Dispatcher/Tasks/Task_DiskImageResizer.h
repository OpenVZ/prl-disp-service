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
/// @author sergeyt
///	igor@
///
////////////////////////////////////////////////////////////////////////////////

#ifndef __Task_DiskImageResizer_H__
#define __Task_DiskImageResizer_H__

#include "CDspVm.h"
#include "CDspTaskHelper.h"
#include "XmlModel/VmConfig/CVmConfiguration.h"
// #include "Libraries/IPCMemory/IPCMemory.h"
//#include "Libraries/VirtualDisk/VirtualDisk.h"  // VirtualDisk commented out by request from CP team

enum {
	TASK_SKIP_LOCK		= 0x1,
	TASK_SKIP_SEND_RESPONSE	= 0x2
};

// struct ImageToolSharedInfo;
class Task_DiskImageResizer : public CDspTaskHelper
{
	Q_OBJECT
public:
	Task_DiskImageResizer(SmartPtr<CDspClient>& user,
				const SmartPtr<IOPackage>& p,
				PRL_UINT32 nOpFlags = 0);

	~Task_DiskImageResizer();

	virtual QString getVmUuid();

protected:
	PRL_RESULT IsHasRightsForResize( CVmEvent& evtOutError );
	virtual PRL_RESULT prepareTask();
	virtual void finalizeTask();
	virtual PRL_RESULT run_body();

private:
	QString ConvertToFullPath(const QString &sPath);
	PRL_RESULT run_disk_tool();
//	PRL_RESULT InitIPCMemory();
//	void DeinitIPCMemory();

	PRL_RESULT updateSplittedState( bool& isSplitted );

public:
	void Notify(PRL_EVENT_TYPE evType);
	void Cancel(unsigned int uErrorCode);

public:
	PRL_UINT32 m_OpFlags;
	// Current progress
	PRL_UINT32 m_CurProgress;

private:
// VirtualDisk commented out by request from CP team
//	IDisk *m_pDisk;
	// IPC data allocated by IPCMemory::Attach()
//	struct ImageToolSharedInfo *m_ImageToolInfo;
	PRL_BOOL m_bflLocked;
	// Shared memory object
// 	IPCMemory* m_pMemory;
	// IPC memory name
	QString m_IPCName;

	QString m_DiskImage;
	PRL_UINT32 m_NewSize;
	PRL_UINT32 m_Flags;
	SmartPtr<CVmConfiguration> m_pVmConfig;
};

#endif //__Task_DiskImageResizer_H__
