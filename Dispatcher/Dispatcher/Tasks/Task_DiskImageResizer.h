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
#include <prlxmlmodel/VmConfig/CVmConfiguration.h>

enum {
	TASK_SKIP_LOCK		= 0x1,
	TASK_SKIP_SEND_RESPONSE	= 0x2
};

class Task_DiskImageResizer : public CDspTaskHelper
{
	Q_OBJECT
public:
	Task_DiskImageResizer(SmartPtr<CDspClient>& user,
				const SmartPtr<IOPackage>& p,
				PRL_UINT32 nOpFlags = 0);

	virtual QString getVmUuid();

protected:
	PRL_RESULT IsHasRightsForResize( CVmEvent& evtOutError );
	virtual PRL_RESULT prepareTask();
	virtual void finalizeTask();
	virtual PRL_RESULT run_body();

private:
	QString ConvertToFullPath(const QString &sPath);
	PRL_RESULT run_disk_tool();

public:
	void Notify(PRL_EVENT_TYPE evType);

public:
	PRL_UINT32 m_OpFlags;
	// Current progress
	PRL_UINT32 m_CurProgress;

private:
	PRL_BOOL m_bflLocked;

	QString m_DiskImage;
	PRL_UINT32 m_NewSize;
	PRL_UINT32 m_Flags;
	SmartPtr<CVmConfiguration> m_pVmConfig;
};

#endif //__Task_DiskImageResizer_H__
