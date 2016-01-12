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
#include <prlxmlmodel/Messaging/CVmEvent.h>
#include <prlxmlmodel/Messaging/CVmEventParameter.h>
#include "CDspTaskHelper.h"
#include <prlxmlmodel/HostHardwareInfo/CHwFileSystemInfo.h>
#include "CDspClient.h"
#include <prlxmlmodel/VmConfig/CVmConfiguration.h>
#include "CDspService.h"
#include <prlxmlmodel/VmConfig/CVmHardware.h>
#include "CDspVmDirHelper.h"
#include "CDspUserHelper.h"
#include <prlxmlmodel/DispConfig/CDispatcherConfig.h>
#include "Libraries/PrlCommonUtils/CFileHelper.h"
#include "FloppyHeader.h"
#include "CConfigSearcher.h"
#include <prlxmlmodel/VmDirectory/CVmDirectory.h>
#include <prlxmlmodel/VmDirectory/CVmDirectoryItem.h>
#include "CDspVmDirHelper.h"
#include <prlcommon/Interfaces/ParallelsQt.h>
#include "CDspSync.h"
#include <prlcommon/PrlCommonUtilsBase/SysError.h>
#include <prlcommon/PrlCommonUtilsBase/PrlStringifyConsts.h>

#include <errno.h>
#include <QDir>

#ifndef _WIN_
#       include <sys/types.h>
#       include <sys/stat.h>
#endif

#include <prlcommon/Logging/Logging.h>

#endif
