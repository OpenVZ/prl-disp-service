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
///		CAuthHelper.h
///
/// @author
///		sergeyt
///
/// @brief
///		Tests fixture class for testing CAuthHelper class functionality.
/// 		To get more info look at bug 1963 http://bugzilla.parallels.ru/show_bug.cgi?id=1963
///
/// @brief
///		None.
///
/////////////////////////////////////////////////////////////////////////////
#ifndef CAuthHelperTest_H
#define CAuthHelperTest_H

#include <QtTest/QtTest>
#include "Libraries/PrlCommonUtils/CAuthHelper.h"
#include <QProcess>

class CAuthHelperTest : public QObject
{

Q_OBJECT

public:
	CAuthHelperTest();
	~CAuthHelperTest();

private slots:
	void cleanup();

	void TestLocalLogin_AsLocalUser();
	void TestLocalLogin_AsDomainUser();

	void TestRemoteLogin_AsLocalUser();
	void TestRemoteLogin_AsDomainUser();

	void TestRemoteLogin_AsDomainUserWithDomainName();
	void TestRemoteLogin_AsDomainUserWithDomainName_WithBackspash();

	void testGetOwnerOfFile();
	void testSetOwnerOfFile();
	void testIsOwnerOfFile();

	void testLoginAsTestUser();
	void testLoginAsTestUser_WithWrongLoginName();
	void testLoginAsTestUser_WithWrongPassword();
private:

	struct UserInfo
	{
		QString name;
		QString password;

		UserInfo(){}
		UserInfo (QString name, QString pwd) : name(name), password(pwd) {}
	};

		//return on windows host in domain - DomainName
		//			 without domain and on unix == getLocalDomain()
	QString getCurrentDomain();

		// return "."
	QString getLocalDomain();

	UserInfo getDomainUser();
	UserInfo getLocalUser();
	UserInfo getCurrentUser();

	bool isLocalUserName( const QString& userName);
	bool isHostInWindowsDomain();
	bool isWinOs();

	static Q_PID createSimpleProcessAsUser( CAuthHelper& auth );
	static QString create_commandline ( const QString& program,
		const QStringList& arguments );

	quint32 GetId_ToLocalLogin();
};


#endif
