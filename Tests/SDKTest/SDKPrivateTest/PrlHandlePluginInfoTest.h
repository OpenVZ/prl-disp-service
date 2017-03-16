/////////////////////////////////////////////////////////////////////////////
///
///	@file PrlHandlePluginInfoTest.h
///
///	This file is the part of parallels public SDK library private tests suite.
///	Tests fixture class for testing SDK API calls for plugin info handle.
///
///	@author myakhin
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
/////////////////////////////////////////////////////////////////////////////
#ifndef PrlHandlePluginInfoTest_H
#define PrlHandlePluginInfoTest_H

#include <QtTest/QtTest>
#include <prlsdk/Parallels.h>
#include "SDK/Wrappers/SdkWrap/SdkHandleWrap.h"


class PrlHandlePluginInfoTest : public QObject
{
Q_OBJECT
private slots:

	void init();
	void cleanup();
	void testPluginInfoBasicValidation();
	void testGetVendor();
	void testGetVendorOnWrongParams();
	void testGetCopyright();
	void testGetCopyrightOnWrongParams();
	void testGetShortDescription();
	void testGetShortDescriptionOnWrongParams();
	void testGetLongDescription();
	void testGetLongDescriptionOnWrongParams();
	void testGetVersion();
	void testGetVersionOnWrongParams();
	void testGetId();
	void testGetIdOnWrongParams();

private:

	SdkHandleWrap m_hPluginInfo;

};


#endif	// PrlHandlePluginInfoTest_H
