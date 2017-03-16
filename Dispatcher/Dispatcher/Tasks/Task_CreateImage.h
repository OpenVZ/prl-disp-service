////////////////////////////////////////////////////////////////////////////////
///
/// Copyright (c) 2005-2017, Parallels International GmbH
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
/// Our contact details: Parallels International GmbH, Vordergasse 59, 8200
/// Schaffhausen, Switzerland.
///
/// @file
///	Task_CreateImage.h
///
/// @brief
///	Definition of the class CDspTaskHelper
///
/// @brief
///	This class implements long running tasks helper class
///
/// @author sergeyt
///	SergeyT@
///
////////////////////////////////////////////////////////////////////////////////

#ifndef __Task_CreateImage_H_
#define __Task_CreateImage_H_

#include "CDspTaskHelper.h"
#include "Mixin_CreateHddSupport.h"
#include <prlxmlmodel/VmConfig/CVmConfiguration.h>
#include <prlcommon/Std/SmartPtr.h>
//#include "Libraries/VirtualDisk/VirtualDisk.h"  // VirtualDisk commented out by request from CP team
//#include "Libraries/VirtualDisk/DiskConfigurator.h"  // VirtualDisk commented out by request from CP team
#include <prlsdk/PrlDisk.h>
//#include "Libraries/DiskImage/PartitionTable.h"  // DiskImage commented out by request from CP team

class Task_RegisterVm;

class Task_CreateImage : public Mixin_CreateHddSupport
{
	friend class Task_RegisterVm;

	// friend callback for control hard disk creation progress
    friend PRL_BOOL HddCallbackToCreateImageTask ( PRL_INT32 iDone, PRL_INT32 iTotal, PRL_VOID_PTR pUserData );

   Q_OBJECT
public:
   Task_CreateImage ( SmartPtr<CDspClient>&,
					  const SmartPtr<IOPackage>&,
					  const SmartPtr<CVmConfiguration> &pVmConfig,
					  const QString& image_config,
					  bool flgRecreateIsAllowed,
						bool bForceQuestionsSign );
	virtual QString getVmUuid();
protected:
   virtual PRL_RESULT run_body();

private:
	void setTaskParameters( const QString& strVmUuid,
		const QString& image_config,
		bool flgRecreateIsAllowed );
	/**
	 * Helper method that converts specified image path to absolute path
	 */
	QString ConvertToFullPath(const QString &sImagePath);

private:
// VirtualDisk commented out by request from CP team
//	// Initialize disk
//	PRL_RESULT InitializeDisk(IDisk* pDisk, unsigned int uiOSType, bool* NeedInit);
//	PRL_RESULT InitializeMacDisk(IDisk* Disk);
	// check FS and return error if cant create DI
	PRL_RESULT checkOnFATSupport(const QString strFullPath, PRL_UINT64 uiFileSizeMb);

private:
	PRL_RESULT createFdd( const CVmFloppyDisk& floppy_disk );
	PRL_RESULT createHdd(const CVmHardDisk& hard_disk );

// VirtualDisk commented out by request from CP team
//	PRL_RESULT hddStep1_RemoveExistingHdd( const CVmHardDisk& hard_disk );
	PRL_RESULT hddStep2_CheckConditions( const CVmHardDisk& hard_disk );
// VirtualDisk commented out by request from CP team
//	PRL_RESULT hddStep3_PrepareParameters( const CVmHardDisk& hard_disk
//		, PARALLELS_DISK_PARAMETERS& outDiskParameters );
//	PRL_RESULT hddStep4_CreateImage( const CVmHardDisk& hard_disk
//		, const PARALLELS_DISK_PARAMETERS& diskParameters );

	PRL_IMAGE_TYPE getHddDiskType( const CVmHardDisk& hard_disk );
	// Get padding value for disk
	PRL_UINT32 getPaddingValue();
	// Remove image
	PRL_RESULT removeImage(const QString& strPath);

private:
	/** Pointer to related VM configuration object */
	SmartPtr<CVmConfiguration> m_pVmConfig;
	QString	m_sImageConfig;
	bool	m_flgRecreateIsAllowed;
};


#endif //__Task_CreateImage_H_
