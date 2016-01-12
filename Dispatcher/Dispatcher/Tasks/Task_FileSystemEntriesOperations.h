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
///	Task_FileSystemEntriesOperations.h
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

#ifndef __Task_FileSystemEntriesOperations_H_
#define __Task_FileSystemEntriesOperations_H_

#include "CDspTaskHelper.h"
#include <prlxmlmodel/HostHardwareInfo/CHwFileSystemInfo.h>

class Task_FileSystemEntriesOperations : public  CDspTaskHelper
{
   Q_OBJECT
public:
   Task_FileSystemEntriesOperations( SmartPtr<CDspClient>&,
									 const SmartPtr<IOPackage>& );

   // Rename directory of file entry
   PRL_RESULT renameFsEntry(QString oldFileName, QString newFileName, CHwFileSystemInfo* fs_info);
   // Remove directory of file entry
   PRL_RESULT removeFsEntry(QString target, CHwFileSystemInfo* fs_info);

   PRL_RESULT getFileSystemEntries(const QString& target, CHwFileSystemInfo* fs_info);

   // Get list of disks
   PRL_RESULT getDisksEntries(CHwFileSystemInfo* fs_info);

   // Create directory entry
   PRL_RESULT createDirectoryEntry(QString target, CHwFileSystemInfo* fs_info);

   // Check if an user can create a file
   PRL_RESULT canCreateFile( const QString& qsFullPath );
#ifdef _WIN_
   // only for windows for network share!
   void entryInfoListOnNetworkShare( const QString & strDir, QFileInfoList & lstFileInfo);
#endif

protected:
   virtual PRL_RESULT run_body() {return PRL_ERR_OPERATION_FAILED;}
};



#endif //__Task_FileSystemEntriesOperations_H_
