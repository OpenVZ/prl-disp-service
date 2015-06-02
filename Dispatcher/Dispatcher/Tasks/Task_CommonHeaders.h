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
///  Task_CommonHeaders.h
///
/// @brief
///  Definition of common headers
///
/// @brief
///  This class implements long running tasks helper class
///
/// @author sergeyt
///  SergeyT@
///
////////////////////////////////////////////////////////////////////////////////

#ifndef __Task_CommonHeaders_H_
#define __Task_CommonHeaders_H_

//#include "Libraries/DiskImage/DiskImage.h"  // DiskImage commented out by request from CP team
#include <prlsdk/PrlDisk.h>
#include "XmlModel/Messaging/CVmEvent.h"
#include "XmlModel/Messaging/CVmEventParameter.h"
#include "CDspTaskHelper.h"
#include "XmlModel/HostHardwareInfo/CHwFileSystemInfo.h"
#include "CDspClient.h"
#include "XmlModel/VmConfig/CVmConfiguration.h"
#include "CDspService.h"
#include "XmlModel/VmConfig/CVmHardware.h"
#include "CDspVmDirHelper.h"
#include "CDspUserHelper.h"
#include "XmlModel/DispConfig/CDispatcherConfig.h"
#include "Libraries/PrlCommonUtils/CFileHelper.h"
#include "FloppyHeader.h"
#include "CConfigSearcher.h"
#include "XmlModel/VmDirectory/CVmDirectory.h"
#include "XmlModel/VmDirectory/CVmDirectoryItem.h"
#include "CDspVmDirHelper.h"
#include "Interfaces/ParallelsQt.h"
#include "CDspSync.h"
#include "Libraries/PrlCommonUtilsBase/SysError.h"
#include "Libraries/PrlCommonUtilsBase/PrlStringifyConsts.h"

#include <errno.h>
#include <QDir>

#ifndef _WIN_
#       include <sys/types.h>
#       include <sys/stat.h>
#endif

#include "Libraries/Logging/Logging.h"

#endif
