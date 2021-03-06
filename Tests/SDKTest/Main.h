/////////////////////////////////////////////////////////////////////////////
///
///	@file Main.h
///
///	This file is the part of Virtuozzo public SDK library tests suite.
///	SDK API tests entry point.
///
///	@author sandro
///
/// Copyright (c) 2005-2017, Parallels International GmbH
/// Copyright (c) 2017-2019 Virtuozzo International GmbH, All rights reserved.
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
/// Our contact details: Virtuozzo International GmbH, Vordergasse 59, 8200
/// Schaffhausen, Switzerland.
///
/////////////////////////////////////////////////////////////////////////////
#ifndef Main_H
#define Main_H

#include <QObject>

/** Special helper that lets to start tests execution in second events loop */
class TestsExecuter : public QObject
{
Q_OBJECT
public:
	TestsExecuter(int _argc, char **_argv) : argc(_argc), argv(_argv) {}

public slots:
	void PushTestsExecution();

private:
	int argc;
	char **argv;
	int nRet;
};

#endif
