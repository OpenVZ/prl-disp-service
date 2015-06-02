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
///	CDspVmSuspendHelper.cpp
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


#include "XmlModel/DispConfig/CDispatcherConfig.h"
#include "XmlModel/VmConfig/CVmConfiguration.h"

#include "Libraries/Logging/Logging.h"
#include "Libraries/StatesUtils/StatesHelper.h"
#include "Libraries/Std/PrlAssert.h"
#include "Libraries/HostUtils/HostUtils.h"
#include "Interfaces/Debug.h"

#include "CDspService.h"
#include "CDspVmManager.h"
#include "CDspVmSuspendHelper.h"

using namespace Parallels;


CDspVmSuspendMounter::CDspVmSuspendMounter()
{

}


CDspVmSuspendMounter::~CDspVmSuspendMounter()
{
	CDspService::instance()->getVmManager().getSuspendHelper()->fastRebootUMount();
}


QString CDspVmSuspendMounter::getMountPoint()
{
	return CDspService::instance()->getVmManager().getSuspendHelper()->getPramMountPoint();
}


QString CDspVmSuspendMounter::getPath()
{
	return m_sPath;
}

void CDspVmSuspendMounter::setPath(QString path)
{
	m_sPath = path;
}


CDspVmSuspendHelper::CDspVmSuspendHelper()
{
	clear();
}


CDspVmSuspendHelper::~CDspVmSuspendHelper()
{

}


void CDspVmSuspendHelper::clear()
{
	m_sPramMountPoint.clear() ;
	m_sCtPramMountPoint.clear() ;
	m_uiReservedPramMemory = 0;
	m_uiAvialableMemory = 0;
	m_uiMountCounter = 0;
}


/*return available memory on host in bytes*/
bool CDspVmSuspendHelper::getAvailableMemory(UINT64 &memory)
{
	struct HostMemUsage memUsage;


	if (HostUtils::GetMemoryUsage(&memUsage)<0)
	{
		WRITE_TRACE(DBG_FATAL, "Failed to detect available memory on host");
		return false;
	}

	memory = memUsage.free + memUsage.cached;
	return true;
}


SmartPtr<CDspVmSuspendMounter> CDspVmSuspendHelper::prepareVmFastReboot(
	const SmartPtr<CVmConfiguration> pVmConfig)
{
	//detect free memory
	//we need ((RAM+VIDEO)/4096)*8 for stream
	//and 3Mb for sav file

	if (m_uiAvialableMemory == 0)
	{
		if (!getAvailableMemory(m_uiAvialableMemory)) //in bytes
			return SmartPtr<CDspVmSuspendMounter>();
	}

	UINT64 required;
	{
		UINT64 ram_required = pVmConfig->getVmHardwareList()->getMemory()->getRamSize() +
			pVmConfig->getVmHardwareList()->getVideo()->getMemorySize(); //in MB
		//convert to bytes
		ram_required *= (1024*1024); //in bytes
		ram_required = 8*ram_required/4096;
		UINT64 sav_required = 3*1024*1024;
		WRITE_TRACE(DBG_DEBUG, "Check fast reboot: available: %lld, ram_required %lld, sav_required %lld",
			m_uiAvialableMemory - m_uiReservedPramMemory, ram_required, sav_required);
		required = sav_required + ram_required;
	}

	if ((required + m_uiReservedPramMemory) >= m_uiAvialableMemory)
	{
		WRITE_TRACE(DBG_FATAL, "Check fast reboot: not enouth available memory");
		return SmartPtr<CDspVmSuspendMounter>();
	}

	m_uiReservedPramMemory += required;

	//TODO: we can make optimization if memory not enouth
	// 1 way. suspend 1 VM. wait for suspend. we got free memory and can suspend to PRAM others VMs
	// 2 way. if we have free ((RAM+VIDEO)/4096)*8. Suspend 1 VM to PRAM, but place sav file to disk.
	// we got free memory and can suspend to PRAM others VMs

	//mount
	SmartPtr<CDspVmSuspendMounter> pMounter = fastRebootMount(true);
	if (!pMounter)
		return pMounter;

	QString sSavHome = m_sPramMountPoint + "/" + pVmConfig->getVmIdentification()->getVmUuid() + ".tmp";
	pMounter->setPath( CStatesHelper(sSavHome).getSavFileName() );

	return pMounter;
}


SmartPtr<CDspVmSuspendMounter> CDspVmSuspendHelper::fastRebootMount(bool bMountedForSuspend)
{
	QString sMountPoint = CDspService::instance()->getDispConfigGuard()
		.getDispCommonPrefs()->getFastRebootPreferences()->getDefaultPramPath();

	QMutexLocker _lock(&m_MountMutex);
	if (!m_sPramMountPoint.isEmpty())
	{
		if (m_bMountedForSuspend != bMountedForSuspend)
		{
			if (m_bMountedForSuspend)
				WRITE_TRACE(DBG_FATAL, "FastReboot "
					"Error: PRAM is already mounted for suspend");
			else
				WRITE_TRACE(DBG_FATAL, "FastReboot "
					"Error: PRAM is already mounted for restore");
			return SmartPtr<CDspVmSuspendMounter>();
		}

		m_uiMountCounter ++;
		return SmartPtr<CDspVmSuspendMounter>(new CDspVmSuspendMounter);
	}

	//get path
	if (sMountPoint.isEmpty())
		sMountPoint = ParallelsDirs::getDefaultPramPath();
	if (sMountPoint.isEmpty())
	{
		WRITE_TRACE(DBG_FATAL, "Internal error: Mount point for PRAM is not specified");
		return SmartPtr<CDspVmSuspendMounter>();
	}

	//mount
	WRITE_TRACE(DBG_FATAL, "Mount PRAM: '%s'", QSTR2UTF8(sMountPoint));
	if (!HostUtils::MountPram(bMountedForSuspend, sMountPoint))
	{
		WRITE_TRACE(DBG_FATAL, "Failed to mount pram");
		return SmartPtr<CDspVmSuspendMounter>();
	}

	m_uiMountCounter ++;

	m_sPramMountPoint = sMountPoint;
	m_bMountedForSuspend = bMountedForSuspend;

	return SmartPtr<CDspVmSuspendMounter>(new CDspVmSuspendMounter);
}

const char *ct_mount_point = "/mnt/pram";

SmartPtr<CDspVmSuspendMounter> CDspVmSuspendHelper::prepareCtForResume()
{
	QMutexLocker _lock(&m_MountMutex);
	if (m_sCtPramMountPoint.isEmpty())
	{
		if (!HostUtils::MountPram(false, QString(ct_mount_point), true))
		{
			WRITE_TRACE(DBG_FATAL, "Failed to mount pram");
			return SmartPtr<CDspVmSuspendMounter>();
		}

		m_sCtPramMountPoint = ct_mount_point;
		m_bMountedForSuspend = false;
	}

	m_uiMountCounter++;
	return SmartPtr<CDspVmSuspendMounter>(new CDspVmSuspendMounter);
}

void CDspVmSuspendHelper::fastRebootUMount()
{
	QMutexLocker _lock(&m_MountMutex);
	PRL_ASSERT( m_uiMountCounter>0 );

	if ( -- m_uiMountCounter )
		return;

	HostUtils::UMountPram(m_sPramMountPoint);
	HostUtils::UMountPram(m_sCtPramMountPoint);

	clear();

	if ( ! m_bMountedForSuspend ) {
		_lock.unlock();
		freeFastRebootResources();
	}
}

QString CDspVmSuspendHelper::getPramMountPoint()
{
	return m_sPramMountPoint;
}


void CDspVmSuspendHelper::freeFastRebootResources()
{
#ifdef _LIN_
	if (!QFileInfo("/sys/kernel/pram").exists())
		return;

	QMutexLocker _lock(&m_MountMutex);
	if (m_uiMountCounter)
		return;

	WRITE_TRACE(DBG_WARNING, "Release PRAM");

	{
		QFile file("/sys/kernel/pram");

		if (file.open(QIODevice::WriteOnly))
			file.write("0", 1);
		else
			WRITE_TRACE(DBG_FATAL, "Failed to open /sys/kernel/pram");
	}

	{
		QFile file("/proc/sys/vm/prune_pswap");
		if (file.open(QIODevice::WriteOnly))
			file.write("1", 1);
		else
			WRITE_TRACE(DBG_FATAL, "Failed to open /proc/sys/vm/prune_pswap");
	}
#endif
}
