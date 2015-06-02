/////////////////////////////////////////////////////////////////////////////
///
/// Copyright (C) 2006-2015 Parallels IP Holdings GmbH
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
///		CUrlParserTest.h
///
/// @author
///		sandro
///
/// @brief
///		Tests fixture class for testing CUrlParser class functionality
///
/// @brief
///		None.
///
/////////////////////////////////////////////////////////////////////////////
#ifndef CUrlParserTest_H
#define CUrlParserTest_H

#include <QtTest/QtTest>

class CUrlParserTest : public QObject
{

Q_OBJECT

private slots:
	void testCUrlParserOnAllComponentsPresent();
	void testCUrlParserOnProtocolAbsent();
	void testCUrlParserOnJustHostnamePort();
	void testCUrlParserOnJustHostname();
	void testCUrlParserOnHostnamePortWithProtocol();
	void testCUrlParserOnHostnameWithProtocol();
	void testCUrlParserSpecificTestCase();
};

#endif

