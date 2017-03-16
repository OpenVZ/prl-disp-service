///////////////////////////////////////////////////////////////////////////////
///
/// @file CDspSettingsWrap.h
///
/// Wrapper for dispatcher app settings.
///
/// @owner artemk@
/// @author dbrylev@
///
/// Copyright (c) 2013-2017, Parallels International GmbH
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
///////////////////////////////////////////////////////////////////////////////

#include "CDspSettingsWrap.h"

#include <prlcommon/Interfaces/ParallelsTypes.h>
#include <prlcommon/Interfaces/ParallelsQt.h>
#include <Libraries/PrlCommonUtils/PrlQSettings.h>
#include <prlcommon/Std/PrlAssert.h>

#include <QCoreApplication>
#include <QFile>

// #425177
#define PRL_DISPATCHER_QSETTINGS_SCOPE QSettings::SystemScope


CDspSettingsWrap::CDspSettingsWrap()
	: m_settingsMutex(QMutex::Recursive)
{}

void CDspSettingsWrap::init(const QString & name)
{
	m_name = name;
	m_pSettings.reset(createSettings());
}

CDspLockedPointer<QSettings> CDspSettingsWrap::getPtr()
{
	QMutexLocker lock(&m_settingsMutex);

	PRL_ASSERT( m_pSettings );

	if( m_pSettings->status() != QSettings::NoError &&
		PrlQSettings::backupAndRemoveQSettingsIfItsNotValid(*m_pSettings.getImpl()) )
	{
		// reinit QSettings
		m_pSettings.reset(createSettings());
	}

	PRL_ASSERT( m_pSettings );
	PRL_ASSERT( m_pSettings->status() == QSettings::NoError );

	return CDspLockedPointer<QSettings>(&m_settingsMutex, m_pSettings.getImpl());
}

QSettings * CDspSettingsWrap::createSettings()
{
#if defined(_WIN_) && defined(_64BIT_)
	// https://jira.sw.ru/browse/PM-10529
	// 32 and 64 bit dispatcher should use same storage on vista and higher.
	return new QSettings(QString("HKEY_LOCAL_MACHINE\\Software\\Wow6432Node\\%1\\%2")
							.arg(QCoreApplication::organizationName())
							.arg(m_name)
						 , QSettings::NativeFormat);
#else
	QScopedPointer<QSettings> s(new QSettings(PRL_DISPATCHER_QSETTINGS_SCOPE,
						 QCoreApplication::organizationName(),
						 m_name));
	if (!s)
		return NULL;
	QFile f(s->fileName());
	if (!f.exists())
	{
		// need to write something in order to create settings file
		s->setValue("auxiliaryField", "");
		s->sync();
		if (!f.exists())
		{
			WRITE_TRACE(DBG_FATAL, "could not create settings file %s",
				QSTR2UTF8(f.fileName()));
			return NULL;
		}
	}
	f.setPermissions(QFile::ReadOwner | QFile::WriteOwner);
	return s.take();
#endif
}
