///////////////////////////////////////////////////////////////////////////////
///
/// @file CWsTest.cpp
///
/// This is a test file intended to verify Parallels SDK
/// functionality in the real-life example
///
/// @author ilya
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
///////////////////////////////////////////////////////////////////////////////

#include <QMessageBox>
#include <QStringList>
#include <QFileDialog>
#include <QFile>
#include <QDateTime>
#include <QTextStream>
#include <prlcommon/PrlUuid/Uuid.h>
#include <QInputDialog>
#include <QSettings>

#include "CWsTest.h"
#include "CResult.h"
#include "CVmEvent.h"
#include "CVmEventParameter.h"
#include "CVmConfiguration.h"
#include "CHostHardwareInfo.h"
#include "CHwGenericDevice.h"
#include "CDispUser.h"
#include "CProblemReport.h"
#include "RemoteFileDialog.h"

#include <prlcommon/Logging/Logging.h>

#ifndef _WIN_
	#define _vsnprintf vsnprintf
#endif

#ifdef _WIN_
	#define PARALLELS_DIR_PREFIX "c:/Parallels Virtual Machines/"
#else
	#define PARALLELS_DIR_PREFIX "/usr/lib/Parallels Virtual Machines/"
#endif

static CWsTest* pSelf = NULL;


// Write debug info
void CWsTest::WriteLog(const char *szForm, ...)
{
    static char szBuffer[8192];
    va_list args;
	memset(szBuffer,0x00,sizeof(szBuffer));
	va_start(args, szForm);
    _vsnprintf(szBuffer, sizeof(szBuffer), szForm, args);
	va_end(args);

	logOut(szBuffer);
}

CWsTest::CWsTest(int &argc, char **argv) : QApplication(argc, argv)
{
	m_strOutputText = "";
	m_strRequestUuid = "";
	m_strLastDirectory = QString(PARALLELS_DIR_PREFIX);
	pVmConfig = NULL;
	m_strLastVmConfig = "";
	m_strLastVmUuid = "";

	pSelf = this;

	m_ServerHandle = PRL_INVALID_HANDLE;
	m_hVm = PRL_INVALID_HANDLE;

	// Initialize API library
	PrlApi_Init(PARALLELS_API_VER);
}

CWsTest::~CWsTest()
{
	PrlApi_Deinit();
}



static PRL_RESULT OnHaveEvent (PRL_HANDLE handle, void*)
{
	PRL_HANDLE_TYPE type;

	if ( PRL_FAILED(PrlHandle_GetType(handle, &type)) )
	{
		QMessageBox::information(NULL, "", "PrlHandle_GetType failed!!");
		return PRL_ERR_SUCCESS;
	}

	LOG_MESSAGE( DBG_FATAL, "Event type %X recieved", type );

	switch( type )
	{
		case PHT_EVENT:
		{
			// QMessageBox::information(NULL, "", "Incoming event handle!");
		}
		break; // case PHT_EVENT

		case PHT_JOB:
		{
			return 0;

			// QMessageBox::information(NULL, "", "Incoming job handle!");
			PRL_HANDLE hResult;

			if ( PRL_SUCCEEDED(PrlJob_GetResult(handle, &hResult)) )
			{
				// QMessageBox::information(NULL, "", "Incoming job handle!");
				PRL_UINT32 param_count;
				if ( PRL_SUCCEEDED(PrlResult_GetParamsCount(hResult, &param_count)) )
				{
					for (PRL_UINT32 i = 0; i < param_count; i++)
					{
						PRL_HANDLE hVm;
						if ( PRL_SUCCEEDED(PrlResult_GetParamByIndex(hResult, i, &hVm )) )
						{
							// Here we can work with phVm as VM HANDLE
							if ( PRL_SUCCEEDED(PrlVm_FromString(hVm, "some XML config...")) )
							{
							}
						}
					}
				}
			}

			PRL_VOID_PTR data_ptr;
			if ( PRL_SUCCEEDED(PrlJob_GetDataPtr(handle, &data_ptr)) )
			{
				((CWsTest *)pSelf)->event( (QEvent *)data_ptr );
			}
		}
		break; // case PHT_JOB

		case PHT_ERROR:
		{
			QMessageBox::information(NULL, "", "Incoming handle is invalid!");
		}
		break; // case PHT_ERROR

		default:
			;

	} // switch( type )


	return 0;
}

void CWsTest::setupSignals()
{
    connect( m_pWndUI->pbExit, SIGNAL( clicked() ), this, SLOT( pbExit_Clicked() ) );
    connect( m_pWndUI->pbBreak, SIGNAL( clicked() ), this, SLOT( pbBreak_Clicked() ) );
    connect( m_pWndUI->pbClearOutput, SIGNAL( clicked() ), this, SLOT( pbClear_Clicked() ) );
    connect( m_pWndUI->pbLogin, SIGNAL( clicked() ), this, SLOT( pbLogin_Clicked() ) );
    connect( m_pWndUI->pbLogoff, SIGNAL( clicked() ), this, SLOT( pbLogoff_Clicked() ) );
    connect( m_pWndUI->pbVmStart, SIGNAL( clicked() ), this, SLOT( pbVmStart_Clicked() ) );
    connect( m_pWndUI->pbVmStop, SIGNAL( clicked() ), this, SLOT( pbVmStop_Clicked() ) );
    connect( m_pWndUI->pbRegNewVm, SIGNAL( clicked() ), this, SLOT( pbRegNewVm_Clicked() ) );
    connect( m_pWndUI->pbOpen, SIGNAL( clicked() ), this, SLOT( pbOpen_Clicked() ) );
    connect( m_pWndUI->pbGetVmList, SIGNAL( clicked() ), this, SLOT( pbGetVmList_Clicked() ) );
    connect( m_pWndUI->pbGetHostInfo, SIGNAL( clicked() ), this, SLOT( pbGetHostInfo_Clicked() ) );
    connect( m_pWndUI->pbRemoteFileOpen, SIGNAL( clicked() ), this, SLOT( pbRemoteFileOpen_Clicked() ) );
    connect( m_pWndUI->pbRemoteFileSave, SIGNAL( clicked() ), this, SLOT( pbRemoteFileSave_Clicked() ) );
    connect( m_pWndUI->pbGetUserProfile, SIGNAL( clicked() ), this, SLOT( pbGetUserProfile_Clicked() ) );
    connect( m_pWndUI->pbProblemReportXml, SIGNAL( clicked() ), this, SLOT( pbProblemReportXml_Clicked() ) );
    connect( m_pWndUI->pbVmReset, SIGNAL( clicked() ), this, SLOT( pbVmReset_Clicked() ) );
    connect( m_pWndUI->pbVmPause, SIGNAL( clicked() ), this, SLOT( pbVmPause_Clicked() ) );
    connect( m_pWndUI->pbVmSuspend, SIGNAL( clicked() ), this, SLOT( pbVmSupend_Clicked() ) );
    connect( m_pWndUI->pbVmResume, SIGNAL( clicked() ), this, SLOT( pbVmResume_Clicked() ) );
    connect( m_pWndUI->pbUnRegVm, SIGNAL( clicked() ), this, SLOT( pbUnRegVm_Clicked() ) );
    connect( m_pWndUI->pbEditVm, SIGNAL( clicked() ), this, SLOT( pbEditVm_Clicked() ) );
    connect( m_pWndUI->pbCloneVm, SIGNAL( clicked() ), this, SLOT( pbCloneVm_Clicked() ) );
    connect( m_pWndUI->pbDeleteVm, SIGNAL( clicked() ), this, SLOT( pbDeleteVm_Clicked() ) );
    connect( m_pWndUI->pbValidateVm, SIGNAL( clicked() ), this, SLOT( pbValidateVm_Clicked() ) );
    connect( m_pWndUI->pbCancelRequest, SIGNAL( clicked() ), this, SLOT( pbCancelRequest_Clicked() ) );
    connect( m_pWndUI->pbConnectDevice, SIGNAL( clicked() ), this, SLOT( pbConnectDevice_Clicked() ) );
    connect( m_pWndUI->pbDisconnectDevice, SIGNAL( clicked() ), this, SLOT( pbDisconnectDevice_Clicked() ) );
    connect( m_pWndUI->pbCreateDiskImage, SIGNAL( clicked() ), this, SLOT( pbCreateDiskImage_Clicked() ) );
    connect( m_pWndUI->pbInstallGuestOS, SIGNAL( clicked() ), this, SLOT( pbInstallGuestOs_Clicked() ) );
    connect( m_pWndUI->pbGetVmConfig, SIGNAL( clicked() ), this, SLOT( pbGetVmConfig_Clicked() ) );
    connect( m_pWndUI->pbGetVmInfo, SIGNAL( clicked() ), this, SLOT( pbGetVmInfo_Clicked() ) );
	connect( m_pWndUI->pbGetVMStatistics, SIGNAL( clicked() ), this, SLOT( pbGetVMStatistics_Clicked() ) );
    connect( m_pWndUI->pbSMCGetRTInfo, SIGNAL( clicked() ), this, SLOT( pbSMCGetRTInfo_Clicked() ) );
    connect( m_pWndUI->pbGetHistoryByVm, SIGNAL( clicked() ), this, SLOT( pbGetHistoryByVm_Clicked() ) );
    connect( m_pWndUI->pbGetHistoryByUser, SIGNAL( clicked() ), this, SLOT( pbGetHistoryByUser_Clicked() ) );
    connect( m_pWndUI->pbShutdownDisp, SIGNAL( clicked() ), this, SLOT( pbShutdownDisp_Clicked() ) );
    connect( m_pWndUI->pbRestartDisp, SIGNAL( clicked() ), this, SLOT( pbRestartDisp_Clicked() ) );
    connect( m_pWndUI->pbDisconnectUser, SIGNAL( clicked() ), this, SLOT( pbDisconnectUser_Clicked() ) );
    connect( m_pWndUI->pbDisconnectAllUsers, SIGNAL( clicked() ), this, SLOT( pbDisconnectAllUsers_Clicked() ) );
    connect( m_pWndUI->pbCancelUserRequest, SIGNAL( clicked() ), this, SLOT( pbCancelUserRequest_Clicked() ) );
    connect( m_pWndUI->pbShutdownVM, SIGNAL( clicked() ), this, SLOT( pbShutdownVM_Clicked() ) );
    connect( m_pWndUI->pbRestartVM, SIGNAL( clicked() ), this, SLOT( pbRestartVM_Clicked() ) );

}

void CWsTest::setupGui()
{
   m_pWndUI->edtWebServicesHost->setText("localhost:64002");
//   m_pWndUI->edtCallArguments->setText( "guest 1q2w3e 111" );

   QCoreApplication::setOrganizationName( "Parallels Software" );
   QCoreApplication::setOrganizationDomain( "parallels.com" );
   QCoreApplication::setApplicationName( "Parallels Server" );
   QSettings prefSettings;

   QString argBuff=prefSettings.value( "tempLogin" ).toString() + QString(" ") + prefSettings.value( "tempPassword" ).toString() + " 1111";
   m_pWndUI->edtCallArguments->setText( argBuff );

}


void CWsTest::logOut(QString txt_line)
{
   m_strOutputText = QString( "%1<br>%2" ).arg( m_strOutputText ).arg( txt_line );
   m_pWndUI->textOutputArea->setHtml(
      QString( "<html><body>%1</body></html>" ).arg( m_strOutputText ) );

}


void CWsTest::parseArguments()
{
   m_lstArguments.clear();

   if( m_pWndUI->edtCallArguments->text().isEmpty()
      || m_pWndUI->edtCallArguments->text().isNull() )
   {
      logOut( ">>> No arguments specified for this call" );
      return;
   }

   m_lstArguments = m_pWndUI->edtCallArguments->text().trimmed().split( " " );

   logOut( ">>> Using arguments:" );

   for( int i=0; i < m_lstArguments.size(); i++ )
      logOut( QString( "arg%1 = %2" ).arg( i ).arg( m_lstArguments.at( i ) ) );

   logOut( ">>> End of arguments list" );

}


void CWsTest::pbExit_Clicked()
{
   exit( 0 );

}


void CWsTest::pbBreak_Clicked()
{
   logOut( "CWsTest::pbBreak_Clicked()" );

}


void CWsTest::pbClear_Clicked()
{
   m_strOutputText = "";
   m_pWndUI->textOutputArea->setHtml( m_strOutputText );

}


void CWsTest::pbLogin_Clicked()
{
	logOut( "CWsTest::pbLogin_Clicked()" );

	parseArguments();

	// Starting login operation to the server
	if ( PRL_SUCCEEDED(PrlSrv_Create(&m_ServerHandle) ) )
	{
		// Registering events handler with the server
		if ( PRL_SUCCEEDED(PrlSrv_RegEventHandler(m_ServerHandle, &OnHaveEvent, 0)) )
		   QMessageBox::information(NULL, "", "PrlSrv_RegEventHandler successful");

		// Initiating login operation
		PRL_HANDLE hJob = PrlSrv_Login(m_ServerHandle,
		   m_pWndUI->edtWebServicesHost->text().trimmed().toAscii().data(),
		   m_lstArguments.at(0).toAscii().data(),
		   m_lstArguments.at(1).toAscii().data(), 0);

		if ( PRL_SUCCEEDED( PrlJob_Wait(hJob, UINT_MAX) ) )
		{
			PRL_RESULT job_rc;

			if ( PRL_SUCCEEDED( PrlJob_GetRetCode(hJob, &job_rc) ) )
			{
				if ( PRL_SUCCEEDED(job_rc) )
					QMessageBox::information( NULL, "", "Login successful" );
				else
					QMessageBox::information( NULL, "", "Login failure" );
			}
		}

		// Don't forget to free the job handle after it's usage
		PrlHandle_Free( hJob );
	}
}


void CWsTest::pbLogoff_Clicked()
{

}


void CWsTest::pbVmStart_Clicked()
{
	// Try to start VM
	PRL_HANDLE hJob = PrlVm_Start(m_hVm);

	if ( PRL_SUCCEEDED( PrlJob_Wait(hJob, UINT_MAX) ) )
	{
		PRL_RESULT job_rc;

		if ( PRL_SUCCEEDED( PrlJob_GetRetCode(hJob, &job_rc) ) )
			if ( PRL_SUCCEEDED(job_rc) )
			{
				QMessageBox::information(NULL, "", "PrlVm_Start successful");
			}
	}
}

void CWsTest::pbGetVmConfig_Clicked()
{

}

void CWsTest::pbGetVmInfo_Clicked()
{

}

void CWsTest::pbGetVMStatistics_Clicked()
{
	qWarning(">>> pbGetVMStatistics_Clicked()");
	qWarning(">>> m_hVm [0x%X]", (unsigned int)m_hVm);

	//////////////////////////////////////////////////////////////////////////
	PrlSrv_GetStatistics(m_ServerHandle);
	return;
	//////////////////////////////////////////////////////////////////////////

	// Try to get VM statistic
	PRL_HANDLE hJob = PrlVm_GetStatistics(m_hVm);

	if ( PRL_FAILED( PrlJob_Wait(hJob, UINT_MAX) ) )
	{
		qWarning(">>> PrlJob_Wait failed");
		return;
	}

	PRL_RESULT job_rc;

	if ( PRL_FAILED( PrlJob_GetRetCode(hJob, &job_rc) ) )
	{
		qWarning(">>> PrlJob_GetRetCode failed");
		return;
	}

	if ( PRL_FAILED( job_rc) )
	{
		qWarning(">>> job_rc failed");
		return;
	}

	PRL_HANDLE hResult;
	if ( PRL_FAILED(PrlJob_GetResult(hJob, &hResult)) )
	{
		qWarning(">>> PrlJob_GetResult failed");
	}

	PRL_HANDLE hStat;
	if ( PRL_FAILED(PrlResult_GetParam(hResult, &hStat)) )
	{
		qWarning(">>> PrlResult_GetParam failed");
	}

	PRL_UINT32 nCount = 0;

	if ( PRL_FAILED(PrlStat_GetProcsStatsCount(hStat, &nCount)) )
	{
		qWarning(">>> PrlStat_GetProcsStatsCount failed");
		return;
	}

	qWarning(">>> nCount [%d]", nCount);



	/*
	PRL_UINT32 param_count = 0;
	if ( PRL_SUCCEEDED(PrlResult_GetParamsCount(hParam, &param_count)) )
	{
		qWarning(">>> param_count 2 [%u]", param_count);
	}
	else
	{
		qWarning(">>> PrlResult_GetParamsCount failed");
		return;
	}
	*/


	/*
	for (PRL_UINT32 i = 0; i < param_count; i++)
	{
		PRL_HANDLE hVm;
		if ( PRL_SUCCEEDED(PrlResult_GetParamByIndex(hResult, i, &hVm )) )
		{
			// Here we can work with phVm as VM HANDLE
			if ( PRL_SUCCEEDED(PrlVm_FromString(hVm, "some XML config...")) )
			{
			}
		}
	}
	*/
	//////////////////////////////////////////////////////////////////////////

	// GetParam(hResult, &hStat)

	//PrlStat_GetCpusStatsCount(hStat, &intVal)

	// PRL_HANDLE hCpuStat;

	// PrlStat_GetCpuStat(hStat, 0, &hCpuStat);

	//

	qWarning(">>> pbGetVMStatistics_Clicked() done");



}

void CWsTest::pbConnectDevice_Clicked()
{

}

void CWsTest::pbDisconnectDevice_Clicked()
{

}

void CWsTest::pbVmPause_Clicked()
{

}

void CWsTest::pbVmSupend_Clicked()
{

}

void CWsTest::pbVmResume_Clicked()
{

}

void CWsTest::pbUnRegVm_Clicked()
{

}

void CWsTest::pbEditVm_Clicked()
{

}

void CWsTest::pbCloneVm_Clicked()
{

}

void CWsTest::pbDeleteVm_Clicked()
{

}

void CWsTest::pbValidateVm_Clicked()
{

}

void CWsTest::pbCreateDiskImage_Clicked()
{

}

void CWsTest::pbInstallGuestOs_Clicked()
{

}

void CWsTest::pbCancelRequest_Clicked()
{

}

void CWsTest::pbVmReset_Clicked()
{

}


void CWsTest::pbVmStop_Clicked()
{

}


void CWsTest::pbRegNewVm_Clicked()
{

}


// Get VM List button clicked
void CWsTest::pbGetVmList_Clicked()
{
	// Try to get list of registered VM's
	PRL_HANDLE hJob = PrlSrv_GetVmList(m_ServerHandle);

	if ( PRL_SUCCEEDED( PrlJob_Wait(hJob, UINT_MAX) ) )
	{
		PRL_RESULT job_rc;

		if ( PRL_SUCCEEDED( PrlJob_GetRetCode(hJob, &job_rc) ) )
			if ( PRL_SUCCEEDED(job_rc) )
			{
				QMessageBox::information(NULL, "", "GetVmList successful");

				PRL_HANDLE hResult;

				if ( PRL_SUCCEEDED(PrlJob_GetResult(hJob, &hResult)) )
				{
					// QMessageBox::information(NULL, "", "Incoming job handle!");
					PRL_UINT32 param_count = 0;
					if ( PRL_SUCCEEDED(PrlResult_GetParamsCount(hResult, &param_count)) )
					{
						qWarning(">>> param_count 1 [%d]", param_count);

						if ( param_count > 0 )
						{
							if ( PRL_SUCCEEDED(PrlResult_GetParamByIndex(hResult, 0, &m_hVm )) )
							{
								qWarning(">>> PrlResult_GetParamByIndex successful");
							}
						}
					}
				}
			}
	}

	// Don't forget to free the job handle after it's usage
	PrlHandle_Free( hJob );
}


// Open button clicked
void CWsTest::pbOpen_Clicked()
{

}


// Get Host hardware info
void CWsTest::pbGetHostInfo_Clicked()
{


}

// Remote file open dialog clicked
void CWsTest::pbRemoteFileOpen_Clicked()
{

}

// Remote file save dialog clicked
void CWsTest::pbRemoteFileSave_Clicked()
{

}


// Get User Profile
void CWsTest::pbGetUserProfile_Clicked()
{

}

void CWsTest::pbSMCGetRTInfo_Clicked()
{

}

void CWsTest::pbGetHistoryByVm_Clicked()
{

}

void CWsTest::pbGetHistoryByUser_Clicked()
{

}

void CWsTest::pbShutdownDisp_Clicked()
{

}

void CWsTest::pbRestartDisp_Clicked()
{

}

void CWsTest::pbDisconnectUser_Clicked()
{

}

void CWsTest::pbDisconnectAllUsers_Clicked()
{

}

void CWsTest::pbCancelUserRequest_Clicked()
{

}

void CWsTest::pbShutdownVM_Clicked()
{

}

void CWsTest::pbRestartVM_Clicked()
{

}



bool CWsTest::event(QEvent *pEvent)
{

   // Check event type (event or operation result)
   if ( (PVE::VmEventClassType)pEvent->type() == PVE::CResultType)
   {
      WriteLog("################################################");

      CResult *pResult = (CResult *)pEvent;
      // Get operation type
      WriteLog("-- Have response from operation_type: %d", pResult->getOpCode());
      // Get executive server name
      WriteLog("\tExecutiveServer: %s", pResult->getExecutiveServer().toLatin1().constData());

      /**
       * Check operation result
       */
      if (IS_OPERATION_SUCCEEDED(pResult->getReturnCode()))
      {
         // Get operation code
         PVE::IDispatcherCommands op_code = pResult->getOpCode();

         if (op_code == PVE::DspCmdUserLogin)
         {
            QString access_token = pResult->m_hashResultSet[PVE::DspCmdUserLogin_strAccessToken];
            WriteLog("\tAccess token = %s", access_token.toLatin1().constData());
         }
         else if (op_code == PVE::DspCmdDirGetVmList)
         {
            // Get count of return parameters
            int nParamsCount = pResult->GetParamsCount();
            // Show values
            for (int i = 0; i < nParamsCount; i++)
            {
               QMessageBox::information(NULL, "", pResult->GetParamToken(i).toAscii().data());
            }
            // FIXME: Save last VM
            m_strLastVmConfig = pResult->GetParamToken(nParamsCount - 1);
            CVmConfiguration *pVm = new CVmConfiguration(m_strLastVmConfig);
            m_strLastVmUuid = pVm->getVmIdentification()->getVmUuid();
            delete pVm;
         }
         else
            if (op_code == PVE::DspCmdUserGetHostHwInfo)
            {
               QString strHostInfo = pResult->m_hashResultSet[PVE::DspCmdUserGetHostHwInfo_strHostHwInfo];

               /*				CHostHardwareInfo* hwinfo = new CHostHardwareInfo();
               hwinfo->fromString( strHostInfo );*/

               QMessageBox::information(NULL, "", strHostInfo.toAscii().data());

               /*				CHwGenericDevice* prt = (CHwGenericDevice*)hwinfo->m_lstPrinters.at(2);
               logOut( prt->getDeviceName() );

               logOut( ">>><<<" );*/
            }
            else
               if (op_code == PVE::DspCmdVmGetConfig)
               {
                  QString vm_config = pResult->m_hashResultSet[PVE::DspCmdVmGetConfig_strVmConfig];
                  QMessageBox::information(NULL, "", vm_config.toAscii().data());
               }
               else if (op_code == PVE::DspCmdGetVmInfo)
               {
                  QString container = pResult->m_hashResultSet[PVE::DspCmdGetVmInfo_strContainer];
                  CVmEvent evt(container);

                  WriteLog("vmInfo: ==========");
                  CVmEventParameter* pParam = evt.getEventParameter(EVT_PARAM_VMINFO_VM_STATE);
                  if (pParam)
                     WriteLog("%s = %d", EVT_PARAM_VMINFO_VM_STATE, pParam->getParamValue().toInt());
                  WriteLog("vmInfo: ========== end");
               }
               else
                  if (op_code == PVE::DspCmdFsGetDirectoryEntries)
                  {
                     QString strHostInfo = pResult->m_hashResultSet[PVE::DspCmdFsGetDirectoryEntries_strEntriesList];
                     QMessageBox::information(NULL, "", strHostInfo.toAscii().data());
                  }
                  else
                     if (op_code == PVE::DspCmdFsGetDiskList)
                     {
                        QString strHostInfo = pResult->m_hashResultSet[PVE::DspCmdFsGetDiskList_strDiskList];
                        QMessageBox::information(NULL, "", strHostInfo.toAscii().data());
                     }
                     else
                        if (op_code == PVE::DspCmdFsCreateDirectory)
                        {
                           QString strHostInfo = pResult->m_hashResultSet[PVE::DspCmdFsCreateDirectory_strDirEntries];
                           QMessageBox::information(NULL, "", strHostInfo.toAscii().data());
                        }
                        else
                        if (op_code == PVE::DspCmdUserGetProfile)
                        {
                           QString strProfile = pResult->m_hashResultSet[PVE::DspCmdUserGetProfile_strUserProfile];
                           QMessageBox::information(NULL, "", strProfile.toAscii().data());

                           //CDispUser* user_profile = new CDispUser();
                           //user_profile->fromString( strProfile );

                           CDispUser* user_profile = new CDispUser( strProfile );

                           QMessageBox::information( NULL, "", user_profile->getUserName().toAscii().data() );


                        }
                        else
                        if (op_code == PVE::DspCmdVmGetProblemReport)
                        {
                           QString strReport = pResult->m_hashResultSet[PVE::DspCmdVmGetProblemReport_strReport];
                           QMessageBox::information(NULL, "Problem report:", strReport.toAscii().data());
                        }
                        else
                        if (op_code == PVE::DspCmdSMCGetDispatcherRTInfo)
                        {
                           QString strReport = pResult->m_hashResultSet[PVE::DspCmdSMCGetDispatcherRTInfo_strEventContainer];
                           //logOut(strReport);
                           CVmEvent evt(strReport);
                           ShowRTInfoEvent (&evt);
                           //QMessageBox::information(NULL, "", strReport.toAscii().data());
                        }
      }
      /**
       * Error occurred
       */
      else
      {
         // Extract error information
         CVmEvent *pE = pResult->GetError();
         WriteLog("Ret code = %d", pResult->getReturnCode());
         WriteLog("Error code = %d", pE->getEventCode());
         WriteLog("Error source = %s", pE->getEventSource().toAscii().data());

         if (pE->getEventType() == PVE::EventTypeComplexError)
         {
            ShowComplexEvent(pE);
         }
         else if (pE->getEventType() == PVE::EventTypeError)
         {
            ShowEvent(pE);
         }



         // Show values of additional error parameters
         for (int i = 0; i < pE->m_lstEventParameters.size(); i++)
         {
            WriteLog("Error param [%d]: %s", i, pE->m_lstEventParameters.at(i)->getParamValue().toAscii().data());
         }
      }
   }
   else
      /**
       * Have incoming event?
       */
      if ( (PVE::VmEventClassType)pEvent->type() == PVE::CVmEventType)
      {
         WriteLog("################################################");

         CVmEvent *pVmEvent = (CVmEvent *)pEvent;

         WriteLog("-- Have incoming event");

         if (pVmEvent->getRespRequired() == PVE::EventRespRequired)
         {
            ShowEvent(pVmEvent);

            // Send empty answer
            CVmEvent *pAnswer = new CVmEvent();
            pAnswer->setEventCode(PET_VM_INF_UNINITIALIZED_EVENT_CODE);
            pAnswer->addEventParameter(new CVmEventParameter(PVE::String,
               pVmEvent->getEventIssuerId(),
               EVT_PARAM_VM_UUID));

            // FIXME: need uncomment m_pPveControl->DspCmdVmAnswer(pAnswer->toString().toAscii().data());
            return true;
         }

         switch(pVmEvent->getEventCode()) {

      case PET_DSP_EVT_VM_START_IO_FLOW:
         WriteLog("EVT_PARAM_IOFLOW_KEY = %s", pVmEvent->m_lstEventParameters.at(0)->getParamValue().toAscii().data());
         break;

      case PET_JOB_PROGRESS_CHANGED:

         CVmEventParameter *pParam;

			WriteLog("Initial request UUID [%s]", pVmEvent->getInitRequestId().toAscii().data());

         pParam = pVmEvent->getEventParameter(EVT_PARAM_PROGRESS_CHANGED);

         if (pParam)
            WriteLog("Current progress [%s]", pParam->getParamValue().toAscii().data());

         pParam = pVmEvent->getEventParameter(EVT_PARAM_DEVICE_TYPE);

         if (pParam)
            WriteLog("Device type [%s]", pParam->getParamValue().toAscii().data());

         pParam = pVmEvent->getEventParameter(EVT_PARAM_DEVICE_INDEX);

         if (pParam)
            WriteLog("Device index [%s]", pParam->getParamValue().toAscii().data());

         break;

      default:
         {
            WriteLog("Event code [%d]", pVmEvent->getEventCode());
            WriteLog("event_issuer_id [%s]", pVmEvent->getEventIssuerId().toAscii().data());
            WriteLog("initial_request_id [%s]", pVmEvent->getInitRequestId().toAscii().data());

            for (int i = 0; i < pVmEvent->m_lstEventParameters.size(); i++)
            {
               WriteLog("param_name [%s]", pVmEvent->m_lstEventParameters.at(i)->getParamName().toAscii().data());
               WriteLog("param_value [%s]", pVmEvent->m_lstEventParameters.at(i)->getParamValue().toAscii().data());
            }
         }

         }

         // Get executive server name
         CVmEventParameter *pExecutiveServer = pVmEvent->getEventParameter(EVT_PARAM_EXECUTIVE_SERVER);

         if (pExecutiveServer)
            WriteLog("\tExecutiveServer: %s", pExecutiveServer->getParamValue().toAscii().data());
      }

      return true;
}


// Test Problem Report XML
void CWsTest::pbProblemReportXml_Clicked()
{

}

void CWsTest::ShowEvent(CVmEvent *pE)
{
   PRL_RESULT nEventCode = pE->getEventCode();
   QString strEvent = PRL_RESULT_TO_STRING( nEventCode );

   for (int i = 0; i < pE->m_lstEventParameters.size(); i++)
   {
      strEvent = QString(strEvent).arg(pE->m_lstEventParameters.at(i)->getParamValue());
   }

   strEvent.replace(QRegExp("%\\d"), "");

   QMessageBox::information( NULL, "", strEvent.toAscii().data() );
}

void CWsTest::ShowComplexEvent(CVmEvent *pE)
{
   QList<CVmEventParameter*> ParamList = pE->m_lstEventParameters;
   foreach (CVmEventParameter *param, ParamList)
   {
      CVmEvent event(param->getParamValue());

      if (IS_OPERATION_SUCCEEDED( event.m_uiRcInit))
      {
         QList<CVmEventParameter*> ParamList2 = event.m_lstEventParameters;
         foreach (CVmEventParameter *param2, ParamList2)
         {
            CVmEvent cdata_event(param2->getCdata());
            if (IS_OPERATION_SUCCEEDED( cdata_event.m_uiRcInit))
            {
               if (cdata_event.getEventType() == PVE::EventTypeComplexError)
               {
                  ShowComplexEvent(&cdata_event);
               }
               else if (cdata_event.getEventType() == PVE::EventTypeError)
               {
                  ShowEvent(&cdata_event);
               }
            }
         }
      }
   }
}

void CWsTest::ShowRTInfoEvent(CVmEvent*pE)
{
   QList<CVmEventParameter*> ParamList = pE->m_lstEventParameters;
   if (ParamList.size()!=2)
   {
      WriteLog ("Error: bad incomming event.");
      return;
   }

   CVmEvent eventSessions(ParamList.at(0)->getParamValue());
   CVmEvent eventVMs(ParamList.at(1)->getParamValue());

   //////////////////////////
   // processing session info
   WriteLog("==========begin sessions info ===============");
   ParamList=eventSessions.m_lstEventParameters;
   foreach (CVmEventParameter *param, ParamList)
   {
      CVmEvent sessionEvent(param->getParamValue());
      if (!IS_OPERATION_SUCCEEDED( sessionEvent.m_uiRcInit))
         continue;

      WriteLog("-----------------------");
      CVmEventParameter *pParam;
      pParam = sessionEvent.getEventParameter(EVT_PARAM_SESSION_UUID);
      if (pParam)
         WriteLog("access token = [%s]", pParam->getParamValue().toAscii().data());

      pParam = sessionEvent.getEventParameter(EVT_PARAM_SESSION_USERNAME);
      if (pParam)
         WriteLog("user name = [%s]", pParam->getParamValue().toAscii().data());

      pParam = sessionEvent.getEventParameter(EVT_PARAM_SESSION_HOSTNAME);
      if (pParam)
         WriteLog("host = [%s]", pParam->getParamValue().toAscii().data());

      pParam = sessionEvent.getEventParameter(EVT_PARAM_SESSION_ACTIVITY_TIME);
      if (pParam)
         WriteLog("activity time = [%s]", pParam->getParamValue().toAscii().data());


      foreach (CVmEventParameter *param2, sessionEvent.m_lstEventParameters)
      {
         if (param2->getParamName()==EVT_PARAM_SESSION_ALLOWED_VM)
            WriteLog("allowed vm = [%s]", param2->getParamValue().toAscii().data());
      }
      //==============
   }
   WriteLog("==========end sessions info ===============");

   //////////////////////////
   // processing VMs info
   WriteLog("==========begin VMs info ===============");
   ParamList=eventVMs.m_lstEventParameters;
   foreach (CVmEventParameter *paramV, ParamList)
   {
      CVmEvent vmEvent(paramV->getParamValue());
      if (!IS_OPERATION_SUCCEEDED( vmEvent.m_uiRcInit))
         continue;
      WriteLog("-----------------------");

      CVmEventParameter* pParam;
      pParam = vmEvent.getEventParameter(EVT_PARAM_VM_UUID);
      if (pParam)
         WriteLog("vm_uuid = [ %s ]", pParam->getParamValue().toAscii().data());

      pParam = vmEvent.getEventParameter(EVT_PARAM_VM_RUNTIME_STATUS);
      if (pParam)
      {
         int vmState=pParam->getParamValue().toInt();
         const char * pState=0;
         switch(vmState){
               case PVE::VmIncorrect: pState="VmIncorrect"; break;
               case PVE::VmRunning: pState="VmRunning"; break;
               case PVE::VmStopped: pState="VmStopped"; break;
               case PVE::VmPaused: pState="VmPaused"; break;
               case PVE::VmSuspended: pState="VmSuspended"; break;
               case PVE::VmLocked: pState="VmLocked"; break;
               case PVE::VmStateUnknown: pState="VmStateUnknown"; break;
         default:
            pState="UNSUPPORTED STATE";
         }//switch

         WriteLog("vm state = [ %s ] (%d)", pState, vmState );
      }
   }//for
   WriteLog("==========end VMs info ===============");
}

void CWsTest::ShowCommandHistoryEvent (CVmEvent *pE)
{
   QList<CVmEventParameter*> ParamList = pE->m_lstEventParameters;
   foreach (CVmEventParameter *param, ParamList)
   {
      CVmEvent entry(param->getParamValue());
      if (!IS_OPERATION_SUCCEEDED( entry.m_uiRcInit))
         continue;
      WriteLog("-----------------------");
      CVmEventParameter *pParam;

      pParam = entry.getEventParameter(EVT_PARAM_CMDHISTORY_TIMESTAMP);
      if (pParam)
         WriteLog("timestamp = [%s]", pParam->getParamValue().toAscii().data());

      pParam = entry.getEventParameter(EVT_PARAM_CMDHISTORY_COMMAND_NUM);
      if (pParam)
      {
         PVE::IDispatcherCommands cmdNo=(PVE::IDispatcherCommands)pParam->getParamValue().toUInt();
		 WriteLog("command = [ %s ] (%d)", PVE::DispatcherCommandToString(cmdNo), cmdNo);


         if (PVE::DspVmRequest==cmdNo) do
         {
            CVmEventParameter* pParam2 = entry.getEventParameter(EVT_PARAM_CMDHISTORY_PARAMS_AS_XML);
            if (!pParam2)
               break;

            CVmEvent realEvt(pParam2->getParamValue());
            if (!IS_OPERATION_SUCCEEDED(realEvt.m_uiRcInit))
               break;

            PVE::IDispatcherCommands realCmdNo=(PVE::IDispatcherCommands)realEvt.getEventCode();
            WriteLog("real command = [ %s ] (%d)", PVE::DispatcherCommandToString(realCmdNo), realCmdNo);
         }while(0);
      }

      pParam = entry.getEventParameter(EVT_PARAM_CMDHISTORY_RESPONSED);
      if (pParam)
      {
         WriteLog("responsed = [%s]", pParam->getParamValue().toAscii().data());
         if (pParam->getParamValue().toInt()==1 )
         {
            pParam = entry.getEventParameter(EVT_PARAM_CMDHISTORY_RETURNCODE);
            if (pParam)
               WriteLog("returnCode = [%s]", pParam->getParamValue().toAscii().data());
         }
      }

      pParam = entry.getEventParameter(EVT_PARAM_CMDHISTORY_PARAMS_AS_XML);
      if (pParam)
      {
         //PrintRequestParamsFromCData (pParam->getCdata());
         PrintRequestParamsFromCData (pParam->getParamValue());
      }

   }//foreach
}

void CWsTest::PrintRequestParamsFromCData(const QString& xml)
{
   WriteLog("params as xml:  = [look to console log]"); //[\n%s\n]", pParam->getCdata().toAscii().data());
   LOG_MESSAGE( DBG_FATAL,"params as xml:  = [\n%s\n]", xml.toAscii().data());

   /*   EXAMPLE OF CODE FROM SERVER
         {
            CVmEvent  paramsContainer;

            QString errorMsg;
            int errorLine;
            int errorColumn;
            QDomDocument xmlSourceDoc;

            bool parse_rc = xmlSourceDoc.setContent( QString((char*)&pHist->pCmd->cmdData), false, &errorMsg, &errorLine, &errorColumn );
            if  (parse_rc)
            {
               // setup root element
               QDomElement xmlVmEventDocRoot = xmlSourceDoc.documentElement();
               QString  nodeName;//= xmlVmEventDocRoot.tagName();
               QDomNodeList Nodes=xmlVmEventDocRoot.childNodes();//xmlVmEventDocRoot.elementsByTagName(nodeName);
               bool flgBreak=false;
               for (int idx=0; idx<Nodes.count() && !flgBreak; idx++)
               {
                  QDomNode node=Nodes.item(idx);
                  if (!node.isElement())
                     continue;
                  if (node.nodeName()=="RequestIdentification") //is common part for any request
                     continue;

                  QDomNodeList paramsNodeList=node.childNodes();
                  for (int jdx=0; jdx<paramsNodeList.count(); jdx++)
                  {
                     QDomNode nodeParam=paramsNodeList.item(jdx);
                     if (!nodeParam.isElement())
                        continue;

                     QString value1=nodeParam.toElement().attribute(nodeParam.nodeName());
                     QString value2=node.toElement().attribute(nodeParam.nodeName());

                     paramsContainer.addEventParameter( new CVmEventParameter(PVE::String
                        , value2
                        , nodeParam.nodeName())
                        );
                  }//for jdx
                  flgBreak=true;

               }//for idx
               entry.addEventParameter( new CVmEventParameter(PVE::String
                  , paramsContainer.toString()
                  , EVT_PARAM_CMDHISTORY_PARAMS_AS_XML)
                  );
            }//if  (parse_rc)

         }// end of //parse command params
  */

}

int main(int argc, char **argv)
{
   CWsTest cTool(argc, argv);

   cTool.m_pWnd = new QMainWindow();
   Ui::MainWindow ui;
   ui.setupUi(cTool.m_pWnd);
   cTool.m_pWndUI = &ui;
   cTool.setupSignals();
   cTool.setupGui();

   cTool.m_pWnd->show();

   return cTool.exec();

}
