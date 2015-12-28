///////////////////////////////////////////////////////////////////////////////
///
/// @file CDspVmInfoDatabase.h
///
/// VM database implemetation
/// Used to store statistical VM information and information about VM state
///
/// @author kBagrinovskiy@
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
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <QMutex>
#include <QString>
#include <prlcommon/Std/SmartPtr.h>
#include <XmlModel/VmInfo/CVmInfo.h>

class CHwGenericDevice;
class CDspClient;

class CDspVmInfoDatabase
{
public:
	static SmartPtr<CVmInfo> readVmInfo( const QString& sPath );
	static bool writeVmInfo( const QString& sPath, SmartPtr<CVmInfo> pVmInfo );
	static bool writeTisBackup( const QString& sVmHomeDir, const QString &sContent );
	static bool saveVmInfoToSnapshot( const QString &strVmDir, const QString &sSnapshotUuid );
	static bool restoreVmInfoFromSnapshot( const QString& strVmDir, const QString &sSnapshotUuid );
private:
	static QMutex ms_VmInfoMutex;
};
