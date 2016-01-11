///////////////////////////////////////////////////////////////////////////////
///
/// @file PrlQSettings.cpp
///
/// @author rsadovnikov
/// @owner sergeym
///
/// Copyright (c) 2005-2015 Parallels IP Holdings GmbH
///
/// This file is part of Virtuozzo Core Libraries. Virtuozzo Core
/// Libraries is free software; you can redistribute it and/or modify it
/// under the terms of the GNU Lesser General Public License as published
/// by the Free Software Foundation; either version 2.1 of the License, or
/// (at your option) any later version.
///
/// This library is distributed in the hope that it will be useful,
/// but WITHOUT ANY WARRANTY; without even the implied warranty of
/// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
/// Lesser General Public License for more details.
///
/// You should have received a copy of the GNU Lesser General Public
/// License along with this library.  If not, see
/// <http://www.gnu.org/licenses/> or write to Free Software Foundation,
/// 51 Franklin Street, Fifth Floor Boston, MA 02110, USA.
///
/// Our contact details: Parallels IP Holdings GmbH, Vordergasse 59, 8200
/// Schaffhausen, Switzerland.
///
///////////////////////////////////////////////////////////////////////////////

#include <QFile>
#include <QFileInfo>
#include <QDateTime>
#include <QDir>

#include <prlcommon/Interfaces/ParallelsQt.h>

#include <prlcommon/Logging/Logging.h>
#include <prlcommon/PrlUuid/Uuid.h>

#include "PrlQSettings.h"
#include <prlcommon/PrlCommonUtilsBase/ParallelsDirs.h>

bool PrlQSettings::backupAndRemoveQSettingsIfItsNotValid( QSettings& qSettings )
{
#ifdef _WIN_
	return false;
#endif

	//Try to remove unexistent key in order to check whether QSettings object valid.
	//We have to do this shit due QMacSettings implementation features - FormatError
	//setting for corrupted plist just in case to attempt remove some element
	qSettings.remove( Uuid::createUuid().toString() );
	if( qSettings.status() == QSettings::NoError )
		return false;

	QString backupSuffix = QString( ".BACKUP." ) + QDateTime::currentDateTime().toString( "yyyy-MM-dd_hh-mm-ss" );
	QString sFileName = qSettings.fileName();
	QString sFileBackup = sFileName + backupSuffix;

	QString warnMsg = QString( "QSettings file(%1) can't be loaded (status = %2) . \n"
		" Current QSettings file would be renamed and stored with suffix %3.\n "
		"configBackup = '%4', scope = %5\n"
		)
			.arg( sFileName )
			.arg( qSettings.status() )
			.arg( backupSuffix )
			.arg( sFileBackup )
			.arg( qSettings.scope() );

	WRITE_TRACE(DBG_FATAL, "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
	WRITE_TRACE(DBG_FATAL, "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
	WRITE_TRACE(DBG_FATAL, "%s", QSTR2UTF8( warnMsg ) );
	WRITE_TRACE(DBG_FATAL, "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
	WRITE_TRACE(DBG_FATAL, "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");

	bool bRemoved = false;

	bool bExisted = QFile::exists(sFileName);

	if( QFile::rename( sFileName, sFileBackup ) )
	{
		bRemoved = true;
	}
	else if( bExisted )
	{
		WRITE_TRACE(DBG_FATAL, "ERROR in QSettings file recovering: Can't rename '%s' ==> '%s'"
			, QSTR2UTF8( sFileName )
			, QSTR2UTF8( sFileBackup ) );

		if( QFile::remove(sFileName) )
			bRemoved = true;
		else
			WRITE_TRACE(DBG_FATAL, "ERROR in QSettings file recovering: Can't delete %s", QSTR2UTF8( sFileName ) );
	}

	if( bRemoved )
		WRITE_TRACE( DBG_FATAL, "Old file %s was successully backuped. Going to recreate new one."
			, QSTR2UTF8(sFileName));

	return true;
}

