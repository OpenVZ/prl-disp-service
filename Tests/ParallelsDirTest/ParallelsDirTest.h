/////////////////////////////////////////////////////////////////////////////
///
/// Copyright (C) 2007-2015 Parallels IP Holdings GmbH
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
///		ParallelsDirTest.h
///
/// @author
///		sergeyt@
///
/// @brief
///		Tests fixture class for testing ParallelsDir class functionality.
///
/// @brief
///
/// last version of specification is available on http://wiki/index.php/Paths_to_Configuration_Files
///
/////////////////////////////////////////////////////////////////////////////

#ifndef ParallelsDirTest_H
#define ParallelsDirTest_H

#include <QtTest/QtTest>

class ParallelsDirTest : public QObject
{
Q_OBJECT
public:

private slots:
	void testGetDispatcherConfigDir();
	void testGetCallerUserPreferencesDir();
	void testGetDefaultVmCatalogue_serverMode();
	void testGetDefaultVmCatalogue_desktopMode();
	void testGetDefaultVmCatalogue_desktopMode2();
	void testGetToolsBaseImagePath();
	void testGetToolsImage();
	void testGetLinReconfigImage();
	void testGetProductPath();
private:
	enum OsType {osUnknown=-1, osWinXp, osWinVista, osLinux, osMac};

#	ifndef _WIN_
	uid_t m_last_euid;
#  endif

private:
	OsType getOsType();
	bool currentProcessHasRootPermission();

	void unixImpersonateTo(const QString& userName);
	void unixRevertToSelf();
	QString linuxGetUserHomePath(const QString& userName);
	QString winGetUserHomePathPrefix(class CAuthHelper *pUser);
};

#endif
