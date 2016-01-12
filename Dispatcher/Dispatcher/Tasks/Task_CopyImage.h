///////////////////////////////////////////////////////////////////////////////
///
/// @file Task_CopyImage.h
///
/// Dispatcher task for doing copy image of virtual device.
///
/// @author myakhin@
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

#ifndef __Task_CopyImage_H__
#define __Task_CopyImage_H__


#include <prlxmlmodel/VmConfig/CVmConfiguration.h>
#include "CDspTaskHelper.h"


class Task_CopyImage : public CDspTaskHelper
{
	Q_OBJECT
public:

	Task_CopyImage( const SmartPtr<CDspClient>& pClient,
					const SmartPtr<IOPackage>& p);
	virtual ~Task_CopyImage();

private:

	virtual PRL_RESULT prepareTask();
	virtual PRL_RESULT run_body();
	virtual void finalizeTask();

	bool checkPathPermissions(QString qsPath);

	QString m_qsVmUuid;
	QString m_qsSourcePath;
	QString m_qsTargetPath;
	CVmHardDisk m_vmHardDisk;
	bool m_bExclusiveOpWasReg;

};


#endif	// __Task_CopyImage_H__
