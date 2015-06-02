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
///		TestDspCmdVmGetConfig.cpp
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
#include "TestDspCmdVmGetConfig.h"
#include <QDomDocument>

#include <QtTest>

#include "SDK/Handles/PveControl.h"
#include "XmlModel/VmDirectory/CVmDirectory.h"
#include "XmlModel/VmConfig/CVmConfiguration.h"
#include "Libraries/Logging/Logging.h"

#include "Libraries/PrlCommonUtils/CFileHelper.h"
#include "Libraries/PrlCommonUtils/CAuthHelper.h"
#include "Tests/CommonTestsUtils.h"

#include "Libraries/Std/SmartPtr.h"


TestDspCmdVmGetConfig::TestDspCmdVmGetConfig(void)
{
}

TestDspCmdVmGetConfig::~TestDspCmdVmGetConfig(void)
{
}

void TestDspCmdVmGetConfig::init()
{
   m_pPveControl = new CPveControl(false, m_pHandler);
   Login();
}

void TestDspCmdVmGetConfig::cleanup()
{
   Logoff();
   m_pPveControl->deleteLater();
   m_pPveControl=0;
}



void TestDspCmdVmGetConfig::TestOfConfigIntegrity()
{
	QSKIP("Skiping test due access rights do not accounting at test implementation", SkipAll);
   QVERIFY (m_pPveControl->GetState()==IOSender::Connected);


   CAuthHelper auth(TestConfig::getUserLogin());
   QVERIFY(auth.AuthUser(TestConfig::getUserPassword()));

	SmartPtr<CVmDirectory> pVmDir = GetUserVmDirectory();
	QVERIFY ( pVmDir );

	if ( pVmDir->m_lstVmDirectoryItems.size()!=0) {

   for (int idx=0; idx< pVmDir->m_lstVmDirectoryItems.size(); idx++)
   {
      CVmDirectoryItem* pDirItem= pVmDir->m_lstVmDirectoryItems[idx];
      QString strVmHome=pDirItem->getVmHome();
      QString strVmUuid=pDirItem->getVmUuid();

      if (!CFileHelper::FileCanRead(strVmHome, &auth))
         continue;

      CALL_CMD(m_pPveControl->DspCmdVmGetConfig(strVmUuid.toUtf8().data()), PVE::DspCmdVmGetConfig)
      CResult _result=m_pHandler->GetResult();
      CHECK_RET_CODE(_result.getReturnCode())

      QString resultVmConfig;
      QVERIFY(getResultVmConfig(_result, resultVmConfig));

      QString expectedVmConfig;
      QVERIFY( getExpectedVmConfig( strVmHome, pDirItem, expectedVmConfig) );

	  SmartPtr<CVmConfiguration> pExpectedVm( new CVmConfiguration(expectedVmConfig) );
	  SmartPtr<CVmConfiguration> pActualVm( new CVmConfiguration(resultVmConfig) );

      QVERIFY (pExpectedVm->getVmIdentification()->getVmUuid() == pActualVm->getVmIdentification()->getVmUuid());
   }
	}
}

void TestDspCmdVmGetConfig::TestOfConfigPermission()
{
   QSKIP("NOT IMPLEMENTED YET", SkipSingle);
}

bool TestDspCmdVmGetConfig::getResultVmConfig(CResult& _result/*in*/, QString& vmConfig/*out*/)
{
   QString  errorMsg;
   int      errorLine, errorColumn;

   QString value = _result.m_hashResultSet[PVE::DspCmdVmGetConfig_strVmConfig];
   if (value.isEmpty())
   {
       WRITE_TRACE(DBG_FATAL, "error in result: result is empty");
       return false;
   }

   QDomDocument doc;
   if (!doc.setContent(value, false, &errorMsg, &errorLine, &errorColumn ))
   {
      WRITE_TRACE(DBG_FATAL, "error in result: errorMsg=%s, line=%d, column=%d",
         errorMsg.toUtf8().data(), errorLine, errorColumn);
      WRITE_TRACE(DBG_FATAL, "[%s]\n", value.toUtf8().data());
      return false;
   }

   vmConfig=doc.toString();
   return true;
}


bool TestDspCmdVmGetConfig::getExpectedVmConfig(const QString& fname, const CVmDirectoryItem& vmDirItem, QString& vmConfig)
{
   QString  errorMsg;
   int      errorLine, errorColumn;


   QFile fConfig(fname);
   if(!fConfig.open(QIODevice::ReadOnly))
   {
      WRITE_TRACE(DBG_FATAL, "can't open file [%s]", fname.toUtf8().data());
      return false;
   }

   QDomDocument doc;
   if(!doc.setContent(&fConfig, false, &errorMsg, &errorLine, &errorColumn ))
   {
      WRITE_TRACE(DBG_FATAL, "error of parsing file: [fname=%s], errorMsg=%s, line=%d, column=%d"
         , fname.toUtf8().data()
         , errorMsg.toUtf8().data(), errorLine, errorColumn);
      return false;
   }

	addNodeToIdentityPart(doc, XML_VM_DIR_ND_VM_HOME, vmDirItem.getVmHome() );
	addNodeToIdentityPart(doc, XML_VM_DIR_ND_CHANGED_BY, vmDirItem.getChangedBy() );
	addNodeToIdentityPart(doc, XML_VM_DIR_ND_CHANGED_DATETIME, vmDirItem.getChangeDateTime().toString(XML_DATETIME_FORMAT) );

   vmConfig=doc.toString();
   return true;
}

void TestDspCmdVmGetConfig::addNodeToIdentityPart(QDomDocument& doc,
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
