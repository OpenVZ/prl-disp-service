///////////////////////////////////////////////////////////////////////////////
///
/// @file CDspVmInfoDatabase.cpp
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


//#define FORCE_LOGGING_ON
//#define FORCE_LOGGING_LEVEL		DBG_TRACE
#define FORCE_LOGGING_PREFIX	"VMINFO"


#include <QMutexLocker>
#include <QFile>
#include <Interfaces/ParallelsNamespace.h>
#include <Libraries/PrlCommonUtilsBase/ParallelsDirs.h>
#include "CDspVmInfoDatabase.h"


QMutex CDspVmInfoDatabase::ms_VmInfoMutex;


SmartPtr<CVmInfo> CDspVmInfoDatabase::readVmInfo( const QString& sPath )
{
	QMutexLocker locker( &ms_VmInfoMutex );

	SmartPtr<CVmInfo> pVmInfo( new CVmInfo() );
	int iRes = pVmInfo->loadFromFile( sPath );
	if ( 0 != iRes )
	{
		return SmartPtr<CVmInfo>( 0 );
	}
	return pVmInfo;
}


bool CDspVmInfoDatabase::writeVmInfo( const QString& sPath, SmartPtr<CVmInfo> pVmInfo )
{
	QMutexLocker locker( &ms_VmInfoMutex );
	QFile fVmInfo( sPath );
	return ( pVmInfo->saveToFile( &fVmInfo, true, true ) == PRL_ERR_SUCCESS );
}


bool CDspVmInfoDatabase::writeTisBackup( const QString& sVmHomeDir, const QString &sContent )
{
	QMutexLocker locker( &ms_VmInfoMutex );

	CVmGuestOsInformation *pVmGuestOsInfo = new CVmGuestOsInformation();
	if (pVmGuestOsInfo->fromString( sContent ) != 0)
	{
		delete pVmGuestOsInfo;
		WRITE_TRACE( DBG_WARNING, "Bad format of sContent" );
		return false;
	}

	SmartPtr<CVmInfo> pVmInfo ( new CVmInfo() );
	int iRes = pVmInfo->loadFromFile( ParallelsDirs::getVmInfoPath( sVmHomeDir ) );
	if ( iRes != PRL_ERR_SUCCESS )
	{
		pVmInfo = SmartPtr<CVmInfo> ( new CVmInfo() );
	}

	QFile fVmInfo( ParallelsDirs::getVmInfoPath( sVmHomeDir ) );
	pVmInfo->setGuestOsInformation( pVmGuestOsInfo );
	return ( pVmInfo->saveToFile( &fVmInfo, true, true ) == PRL_ERR_SUCCESS );
}


bool CDspVmInfoDatabase::saveVmInfoToSnapshot( const QString &strVmDir,
											   const QString &sSnapshotUuid )
{
	QMutexLocker locker( &ms_VmInfoMutex );
	QString strSourceFile = ParallelsDirs::getVmInfoPath( strVmDir );
	QString strDestFile = QString( "%1/%2/%3%4" )
							.arg( strVmDir )
							.arg( VM_GENERATED_WINDOWS_SNAPSHOTS_DIR )
							.arg( sSnapshotUuid )
							.arg( VM_INFO_FILE_SUFFIX );

	// Nothing to copy
	if ( !QFile::exists( strSourceFile ) )
	{
		return true;
	}
	return QFile::copy( strSourceFile, strDestFile );
}


bool CDspVmInfoDatabase::restoreVmInfoFromSnapshot( const QString &strVmDir,
												    const QString &sSnapshotUuid )
{
	QMutexLocker locker( &ms_VmInfoMutex );
	QString strSourceFile = QString( "%1/%2/%3%4" )
							.arg( strVmDir )
							.arg( VM_GENERATED_WINDOWS_SNAPSHOTS_DIR )
							.arg( sSnapshotUuid )
							.arg( VM_INFO_FILE_SUFFIX );
	QString strDestFile = ParallelsDirs::getVmInfoPath( strVmDir );

	// Delete dest file if it exists
	if ( QFile::exists( strDestFile ) )
	{
		if ( !QFile::remove( strDestFile ) )
		{
			return false;
		}
	}

	// Nothing to copy
	if ( !QFile::exists( strSourceFile ) )
	{
		WRITE_TRACE( DBG_WARNING, "Can't delete dest file" );
		return true;
	}

	return QFile::copy( strSourceFile, strDestFile );
}
