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
///	CDspVmSuspendHelper.h
///
/// @brief
///	Definition of the class CDspVmSuspendHelper
///
/// @brief
///	This class implements help logic for VM susend
///
/// @author evg
///
////////////////////////////////////////////////////////////////////////////////

#ifndef __CDspVmSuspendHelper_H_
#define __CDspVmSuspendHelper_H_

#include <prlcommon/Interfaces/ParallelsNamespace.h>
#include "XmlModel/VmConfig/CVmConfiguration.h"
#include <prlcommon/Std/SmartPtr.h>


class CDspVmSuspendMounter
{
public:
	// constructor
	CDspVmSuspendMounter();

	// destructor
	~CDspVmSuspendMounter();

        QString getMountPoint();

        QString getPath();

        void setPath(QString path);

private:
	//path to dump file
	QString m_sPath;

}; // class CDspVmSuspendMounter

/**
* @brief This class implements VM Directory managing logic
* @author SergeyM
*/
class CDspVmSuspendHelper
{
public:
	// constructor
	CDspVmSuspendHelper ();

	// destructor
	~CDspVmSuspendHelper();

public:
	SmartPtr<CDspVmSuspendMounter> prepareVmFastReboot(const SmartPtr<CVmConfiguration> pVmConfig);
	SmartPtr<CDspVmSuspendMounter> prepareCtForResume();
	SmartPtr<CDspVmSuspendMounter> fastRebootMount(bool bMountedForSuspend);

	void fastRebootUMount();

	QString getPramMountPoint();

	void freeFastRebootResources();

private:
	bool getAvailableMemory(UINT64 &memory);
	void clear();

	QString m_sPramMountPoint;
	QString m_sCtPramMountPoint;
	UINT64 m_uiReservedPramMemory;
	UINT64 m_uiAvialableMemory;

	QMutex m_MountMutex;
	volatile uint m_uiMountCounter;
	/*
	 * for suspend or for restore
	 */
	bool m_bMountedForSuspend;

}; // class CDspVmSuspendHelper


#endif // __CDspVmSuspendHelper_H_
