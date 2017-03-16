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
///		TestDspCmdUserGetProfile.cpp
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
#include "TestDspCmdUserGetProfile.h"
#include <QDomDocument>

#include <QtTest>

#include <prlcommon/Interfaces/ParallelsQt.h>

#include "SDK/Handles/PveControl.h"
#include <prlxmlmodel/DispConfig/CDispatcherConfig.h>
#include <prlxmlmodel/DispConfig/CDispUsersPreferences.h>
#include <prlxmlmodel/DispConfig/CDispUser.h>
#include <prlcommon/Logging/Logging.h>
#include "Libraries/PrlCommonUtils/CAuthHelper.h"

#include "Tests/CommonTestsUtils.h"


TestDspCmdUserGetProfile::TestDspCmdUserGetProfile(void)
{
}

TestDspCmdUserGetProfile::~TestDspCmdUserGetProfile(void)
{
}

void TestDspCmdUserGetProfile::init()
{
	m_pPveControl = new CPveControl(false, m_pHandler);
	Login();
}

void TestDspCmdUserGetProfile::cleanup()
{
	Logoff();
	m_pPveControl->deleteLater();
	m_pPveControl=0;
}

namespace {
/**
 * Simple helper - eliminates home path info from user profile to prevent possibility
 * of failed comparision
 */
QString CleanupHomePath(const QString &sUserProfile)
{
	CDispUser _disp_user(sUserProfile);
	if (PRL_SUCCEEDED(_disp_user.m_uiRcInit))
		_disp_user.getUserWorkspace()->setUserHomeFolder("");
	return (_disp_user.toString());
}

}

void TestDspCmdUserGetProfile::Test()
{
	CALL_CMD(m_pPveControl->DspCmdUserGetProfile(), PVE::DspCmdUserGetProfile)
	CResult _result=m_pHandler->GetResult();
	QVERIFY(IS_OPERATION_SUCCEEDED(_result.getReturnCode()));

	QString sRes;
	QVERIFY(getResult(_result, sRes));
	sRes = CleanupHomePath(sRes);

	QString sExpected;
	CAuthHelper _auth(TestConfig::getUserLogin());
	QVERIFY(_auth.AuthUser(TestConfig::getUserPassword()));
	QVERIFY(getExpected(QString( "%1@%2" ).arg(_auth.getUserName()).arg(_auth.getUserDomain()), sExpected));
	QVERIFY(getExpected( _auth.getUserFullName() , sExpected));

	sExpected = CleanupHomePath(sExpected);

	QCOMPARE(sRes, sExpected);
}

bool TestDspCmdUserGetProfile::getResult(CResult& _result/*in*/, QString& out/*out*/)
{
	QString  errorMsg;
	int		errorLine, errorColumn;

	QString value = _result.m_hashResultSet[PVE::DspCmdUserGetProfile_strUserProfile];
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

	out=doc.toString();
	return true;
}


bool TestDspCmdUserGetProfile::getExpected(const QString& userName, QString& out)
{
	QString strDspConf=TestConfig::getPathToDispatcherConfig();
	QFile  fConfig(strDspConf);

   if(!fConfig.open(QIODevice::ReadOnly))
   {
      WRITE_TRACE(DBG_FATAL, "can't open file [%s]", strDspConf.toUtf8().data());
      return false;
   }

   QString  errorMsg;
   int      errorLine, errorColumn;

   QDomDocument doc;
   if(!doc.setContent(&fConfig, false, &errorMsg, &errorLine, &errorColumn ))
   {
      WRITE_TRACE(DBG_FATAL, "error of parsing file: [fname=%s], errorMsg=%s, line=%d, column=%d"
         , strDspConf.toUtf8().data()
         , errorMsg.toUtf8().data(), errorLine, errorColumn);
      return false;
   }

   QDomElement elUsersPref=doc.firstChildElement(XML_VM_DISP_EL_ROOT)
   .firstChildElement(XML_VM_DISP_EL_SETTINGS)
   .firstChildElement(XML_VM_DISP_EL_USERS_PREFERS);
   if (elUsersPref.isNull())
   {
      WRITE_TRACE(DBG_FATAL, "can't parse config");
      return false;
   }

   out="";
   for (QDomElement elUser=elUsersPref.firstChildElement(XML_VM_DISP_EL_USER_ITEM);
         !elUser.isNull();
         elUser=elUser.nextSiblingElement(XML_VM_DISP_EL_USER_ITEM)
      )
   {
		QDomElement userNameElement = elUser.firstChildElement(XML_VM_ND_NAME);
      QString userNameValue = userNameElement.text();
      if (userName!=userNameValue)
			continue;

		try
		{
			//replace uuid to path
			//FIXME: bug #2242 getUserProfile() return path intsead uuid of VmDirectory
			//  ==> we use direct access to dispatcher xml
			SmartPtr<CVmDirectory> pVmDir = GetUserVmDirectory();
			if ( !pVmDir )
				throw "!pVmDir";

			QString vmdir_path = pVmDir->getDefaultVmFolder();
			QDomElement elWorkspace = elUser.firstChildElement( XML_VM_DISP_EL_USER_WORKSPACE );
			if ( elWorkspace.isNull() )
				throw "elWorkspace.isNull()";

			QDomElement elUserVmDirectory = elWorkspace.firstChildElement( XML_VM_DISP_ND_USER_VM_DIR );
			if ( elUserVmDirectory.isNull() )
				throw "elUserVmDirectory.isNull()";
			QDomNode childTextNode = elUserVmDirectory.firstChild();
			if (childTextNode.isText())
				childTextNode.toText().setData(vmdir_path);
			else
				throw "Not correct XML model text element";

/*			LOG_MESSAGE (DBG_FATAL, "elUserVmDirectory = %s" , QSTR2UTF8( elUserVmDirectory.text() ));
			elUserVmDirectory.setNodeValue( vmdir_path );
			LOG_MESSAGE (DBG_FATAL, "elUserVmDirectory = %s" , QSTR2UTF8( elUserVmDirectory.text() ));

			LOG_MESSAGE (DBG_FATAL, "!!!!!!!! I CAN'T KNOW HOW CHANGE VALUE OF ELEMENT. sergeyt@" , QSTR2UTF8( elUserVmDirectory.text() ));*/

			QDomElement textValue = elUserVmDirectory.firstChildElement();

			/*
			{
				QDomElement my;


				// create default VM directory node
				QDomElement xmlNode_VmDir =
				QDomText xmlNode_VmDirValue =
					parent_doc->createTextNode( m_strVmDirectory );
				xmlNode_VmDir.appendChild( xmlNode_VmDirValue );
				parent_element->appendChild( xmlNode_VmDir );

			}
			*/

			LOG_MESSAGE (DBG_FATAL, "textValue = %s" , QSTR2UTF8( textValue.text() ));

			/*
			QDomText textValue = elUserVmDirectory.firstChildElement().toText();
			LOG_MESSAGE (DBG_FATAL, "textValue = %s" , QSTR2UTF8( textValue.text() ));
			textVal
				ue.setData( vmdir_path );
			*/

			LOG_MESSAGE (DBG_FATAL, "textValue = %s" , QSTR2UTF8( textValue.text() ));

			//replace uuid to path -- END

			//----------------------
			QTextStream stream(&out);
			stream << elUser;
			stream.flush();

			//set same indent as in getResult();
			QDomDocument doc2;
			if (!doc2.setContent(out, false))
				throw "error in parse";
			out=doc2.toString();
		}
		catch ( const char* err)
		{
			LOG_MESSAGE ( DBG_FATAL, "error catched: [ %s ]", err );
			Q_UNUSED(err);
			return false;
		}

      break;
   }//for
   return !out.isEmpty();
}
