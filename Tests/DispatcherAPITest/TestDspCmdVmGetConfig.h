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
///		TestDspCmdVmGetConfig.h
///
/// @author
///		sergeyt
///
/// @brief
///		Tests fixture class for testing DspCmdVmGetConfig dispatcher API command functionality.
///
/// @brief
///		None.
///
/////////////////////////////////////////////////////////////////////////////
#ifndef H_TestDspCmdVmGetConfig_H
#define H_TestDspCmdVmGetConfig_H

#include <QDomDocument>

#include "TestDispatcherBase.h"
#include <prlcommon/Messaging/CResult.h>
#include <prlxmlmodel/VmDirectory/CVmDirectoryItem.h>


class TestDspCmdVmGetConfig :
   public TestDispatcherBase
{
   Q_OBJECT
public:
   TestDspCmdVmGetConfig(void);
   ~TestDspCmdVmGetConfig(void);

private slots:
   void init();
   void cleanup();

private slots:
   //void Test();
   void TestOfConfigIntegrity();
   void TestOfConfigPermission();
private:
   bool getResultVmConfig(CResult& _result/*in*/, QString& vmConfig/*out*/);
	bool getExpectedVmConfig(const QString& fname, const CVmDirectoryItem& vmDirItem, QString& vmConfig);

	void addNodeToIdentityPart(QDomDocument& doc, const QString& nodeName, const QString& nodeValue );
};

#endif //H_TestDspCmdVmGetConfig_H
