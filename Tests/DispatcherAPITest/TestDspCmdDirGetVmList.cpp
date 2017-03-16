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
///		TestDspCmdDirGetVmList.cpp
///
/// @author
///		sergeyt
///
/// @brief
///		Tests fixture class for testing DspCmdDirGetVmList dispatcher API command functionality.
///
/// @brief
///		None.
///
/////////////////////////////////////////////////////////////////////////////
#include "TestDspCmdDirGetVmList.h"

#include <QtTest>

#include "SDK/Handles/PveControl.h"
#include <prlxmlmodel/VmDirectory/CVmDirectory.h>
#include <prlxmlmodel/VmConfig/CVmConfiguration.h>
#include <prlcommon/Logging/Logging.h>

#include "Libraries/PrlCommonUtils/CFileHelper.h"
#include "Libraries/PrlCommonUtils/CAuthHelper.h"
#include "Tests/CommonTestsUtils.h"

#include <prlcommon/Std/SmartPtr.h>

TestDspCmdDirGetVmList::TestDspCmdDirGetVmList()
{
}

TestDspCmdDirGetVmList::~TestDspCmdDirGetVmList()
{
}

void TestDspCmdDirGetVmList::init()
{
   m_pPveControl = new CPveControl(false, m_pHandler);
   Login();
}

void TestDspCmdDirGetVmList::cleanup()
{
   Logoff();
   m_pPveControl->deleteLater();
   m_pPveControl=0;
}

void TestDspCmdDirGetVmList::Test()
{
	QSKIP("Skiping test due access rights do not accounting at test implementation", SkipAll);
   QVERIFY (m_pPveControl->GetState()==IOSender::Connected);
   CALL_CMD(m_pPveControl->DspCmdDirGetVmList(), PVE::DspCmdDirGetVmList)
   CResult _result=m_pHandler->GetResult();
   CHECK_RET_CODE(_result.getReturnCode())

   //=====================
   QList<QDomDocument> listResult;
   QVERIFY(getResultVmList(listResult, _result));

   QList<QDomDocument> listExpected;
   QVERIFY(getExpectedVmList(listExpected));

   if(listResult.size()!=listExpected.size())
   {  WRITE_TRACE(DBG_FATAL, "listResult.size()=[%d]; listExpected.size()=[%d]",
         listResult.size(), listExpected.size());
      QVERIFY(listResult.size()==listExpected.size());
   }

   for (int i=0; i<listExpected.size(); i++)
   {
      bool flgFound=false;
      SmartPtr<CVmConfiguration> pExpectedVm( new CVmConfiguration(listExpected[i].toString()) );
      for (int j=0; !flgFound && j<listResult.size(); j++)
      {
         SmartPtr<CVmConfiguration> pActualVm( new CVmConfiguration(listResult[j].toString()) );
         if (pActualVm->getVmIdentification()->getVmUuid() == pExpectedVm->getVmIdentification()->getVmUuid())
            flgFound=true;
      }
			if (!flgFound)
			{
				WRITE_TRACE(DBG_FATAL, "VM config not found: [%s]", pExpectedVm->toString().toUtf8().data());
				foreach(QDomDocument _doc, listResult)
					WRITE_TRACE(DBG_FATAL, "result VM config: [%s]", _doc.toString().toUtf8().data());
			}
      QVERIFY(flgFound);
   }
}

bool TestDspCmdDirGetVmList::getResultVmList(QList<QDomDocument>& listResult, CResult& _result)
{
   QString  errorMsg;
   int      errorLine, errorColumn;

   listResult.clear();

   for (int i = 0; i < _result.GetParamsCount(); i++)
   {
      listResult.push_back(QDomDocument());
      if (!listResult[i].setContent(_result.GetParamToken(i), false, &errorMsg, &errorLine, &errorColumn ))
      {
         WRITE_TRACE(DBG_FATAL, "error in result: [idx=%d], errorMsg=%s, line=%d, column=%d", i
            , errorMsg.toUtf8().data(), errorLine, errorColumn);
         WRITE_TRACE(DBG_FATAL, "[%s]\n", _result.GetParamToken(i).toUtf8().data());
         listResult.clear();
         return false;
      }
   }
   return true;
}

bool TestDspCmdDirGetVmList::getExpectedVmList(QList<QDomDocument>& expectedList)
{
   QString  errorMsg;
   int      errorLine, errorColumn;

   expectedList.clear();

   CAuthHelper auth(TestConfig::getUserLogin());
   if (!auth.AuthUser(TestConfig::getUserPassword()))
   {
      WRITE_TRACE(DBG_FATAL, "can't auth user[%s] on localhost ", TestConfig::getUserLogin());
      return false;
   }

	// __asm int 3;
   SmartPtr<CVmDirectory> pVmDir = GetUserVmDirectory();
   if( !pVmDir )
   {
      WRITE_TRACE(DBG_FATAL, "can't get vm directory from ");
      return false;
   }

   for (int idx=0; idx< pVmDir->m_lstVmDirectoryItems.size(); idx++)
   {
      CVmDirectoryItem* pDirItem= pVmDir->m_lstVmDirectoryItems[idx];
      QString strVmHome=pDirItem->getVmHome();
      QString strChangedBy=pDirItem->getChangedBy();
      QString strChangeDateTime=pDirItem->getChangeDateTime().toString(XML_DATETIME_FORMAT);

      //FIXME: add checking access permission to vm.xml
      // fixed: when i started as test-user it doing automatically

      if (!CFileHelper::FileCanRead(strVmHome, &auth))
         continue;

      QFile vmConfig(strVmHome);
      if(!vmConfig.open(QIODevice::ReadOnly))
      {
         WRITE_TRACE(DBG_FATAL, "can't open file [%s]", strVmHome.toUtf8().data());
         break;
      }

      expectedList.push_back(QDomDocument());
      QDomDocument& doc=expectedList[expectedList.size()-1];
      if(!doc.setContent(&vmConfig, false, &errorMsg, &errorLine, &errorColumn ))
      {
         WRITE_TRACE(DBG_FATAL, "error of parsing file: [fname=%s], errorMsg=%s, line=%d, column=%d"
            , strVmHome.toUtf8().data()
            , errorMsg.toUtf8().data(), errorLine, errorColumn);
         expectedList.clear();
         return false;
      }

      addNodeToIdentityPart(doc, XML_VM_DIR_ND_VM_HOME, strVmHome);
      addNodeToIdentityPart(doc, XML_VM_DIR_ND_CHANGED_BY, strChangedBy);
      addNodeToIdentityPart(doc, XML_VM_DIR_ND_CHANGED_DATETIME, strChangeDateTime);
   }//for
   return (true);
}

void TestDspCmdDirGetVmList::addNodeToIdentityPart(QDomDocument& doc,
         const QString& nodeName, const QString& nodeValue )
{
   QDomElement identElement=doc.firstChildElement(XML_VM_CONFIG_EL_ROOT).firstChildElement(XML_VM_CONFIG_EL_VMIDENT);
   if (identElement.isNull())
      return;

   // remove excess elements
   QDomElement elem;
   while (!(elem=identElement.firstChildElement(nodeName)).isNull())
      identElement.removeChild(elem);

   QDomElement xmlNode = doc.createElement( nodeName );
   QDomText xmlText = doc.createTextNode(nodeValue);
   xmlNode.appendChild(xmlText);
   identElement.appendChild( xmlNode );
}
