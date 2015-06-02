///////////////////////////////////////////////////////////////////////////////
///
/// @file CDspSettingsWrap.h
///
/// Wrapper for dispatcher app settings.
///
/// @owner artemk@
/// @author dbrylev@
///
/// Copyright (c) 2013-2015 Parallels IP Holdings GmbH
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

#include "CDspSync.h"
#include <QSettings>
#include <QMutex>

class CDspSettingsWrap
{
public:
	/**
	 * Create uninitialized settings instance.
	 */
	CDspSettingsWrap();
	~CDspSettingsWrap() {}
	void init(const QString & name);
	CDspLockedPointer<QSettings> getPtr();
protected:
	QSettings * createSettings();
private:
	QMutex m_settingsMutex;
	SmartPtr<QSettings>	m_pSettings;
	QString m_name;
};
