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
///		TestDspCmdUserGetProfile.h
///
/// @author
///		sergeyt
///
/// @brief
///		Tests fixture class for testing DspCmdUserGetProfile dispatcher API command functionality.
///
/// @brief
///		None.
///
/////////////////////////////////////////////////////////////////////////////
#ifndef H_TestDspCmdUserGetProfile_H
#define H_TestDspCmdUserGetProfile_H

#include "TestDispatcherBase.h"
#include <prlcommon/Messaging/CResult.h>

class TestDspCmdUserGetProfile :
   public TestDispatcherBase
{
   Q_OBJECT
public:
   TestDspCmdUserGetProfile(void);
   ~TestDspCmdUserGetProfile(void);

private slots:
   void init();
   void cleanup();

private slots:
   void Test();
private:
   bool getResult(CResult& _result/*in*/, QString& res/*out*/);
   bool getExpected(const QString& userName, QString& res);

};

#endif //H_TestDspCmdUserGetProfile_H
