///////////////////////////////////////////////////////////////////////////////
///
/// @file Task_MountVm.h
///
/// Dispatcher task for mounting Hdd files
///
/// @author andreydanin@
/// @owner lenkor@
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

#ifndef __Task_MountVm_H_
#define __Task_MountVm_H_

#include "CDspTaskHelper.h"
#include "CDspVmMounter.h"
#include "XmlModel/VmConfig/CVmConfiguration.h"

class Task_MountVm : public  CDspTaskHelper
{
	Q_OBJECT

public:
	/**
	* Class constructor
	* @param pointer to the user session object
	* @param pointer to request package
	* @param pointer to the list with paths of old VM hdd
	*/
	Task_MountVm(SmartPtr<CDspClient> &pUser,
		const SmartPtr<IOPackage> &p, PVE::IDispatcherCommands nCmd,
		SmartPtr<CDspVmMountRegistry> &registry);

	/**
	* Class destructor
	*/
	virtual ~Task_MountVm() {}

protected:
	/**
	* Returns result code which specifies whether all prestart conditions correct
	*/
	virtual PRL_RESULT prepareTask();
	/**
	* Task body
	* @return result code of task completion
	*/
	virtual PRL_RESULT run_body();
	/**
	* Makes all necessary actions (sending answer and etc.) on task comletion
	*/
	virtual void finalizeTask();

	QString getVmUuid();
	void MountVmInfo();
	PRL_RESULT MountVm();
	PRL_RESULT UmountVm();

private:
	SmartPtr<CVmConfiguration>	m_pVmConfig;
	/** mount point */
	QString m_sMountPoint;
	PVE::IDispatcherCommands m_nCmd;
	quint32 m_nFlags;
	QString m_sMountInfo;
	SmartPtr<CDspVmMountRegistry> m_registry;
};

#endif //__Task_MountVm_H__
