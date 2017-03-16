/////////////////////////////////////////////////////////////////////////////
///
/// Copyright (c) 2006-2017, Parallels International GmbH
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
/// @file
///		TestDispatcherBase.h
///
/// @author
///		sergeyt
///
/// @brief
///		Base tests fixture class for all tests suites of dispatcher API commands functionality.
///
/// @brief
///		None.
///
/////////////////////////////////////////////////////////////////////////////
#ifndef H_TestDispatcherBase_H
#define H_TestDispatcherBase_H

#include <qobject.h>
#include "Tests/CMockPveEventsHandler.h"

#include <prlxmlmodel/VmDirectory/CVmDirectory.h>
#include <prlxmlmodel/DispConfig/CDispUser.h>
#include <prlcommon/Std/SmartPtr.h>

class CPveControl;

class TestDispatcherBase : public QObject
{
   Q_OBJECT
public:
   TestDispatcherBase();
   virtual ~TestDispatcherBase();

protected:
   virtual bool IsValid();
   bool Login();
   bool Logoff();
    QString QueryUserVmDirPath();

    SmartPtr<CVmDirectory> GetUserVmDirectory();

protected:
   CPveControl       *m_pPveControl;
   CMockPveEventsHandler *m_pHandler;

private:
    SmartPtr<CDispUser> GetUserProfile();
};

#endif //H_TestDispatcherBase_H
