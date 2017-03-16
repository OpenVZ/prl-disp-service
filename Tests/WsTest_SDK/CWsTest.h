/*
 * Copyright (c) 2015-2017, Parallels International GmbH
 *
 * This file is part of Virtuozzo Core. Virtuozzo Core is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License as published by the Free Software Foundation;
 * either version 2 of the License, or (at your option) any later
 * version.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 * Our contact details: Parallels International GmbH, Vordergasse 59, 8200
 * Schaffhausen, Switzerland.
 */


#ifndef __CWSTEST_H__
#define __CWSTEST_H__

#include <Qt>
#include <QApplication>
#include <QTextDocument>
#include <QMainWindow>

#include "ui_WsTestDialog.h"
#include "SDK/Wrappers/SdkWrap/SdkWrap.h"

class CVmEvent;
class QStringList;
class CVmConfiguration;


class CWsTest : public QApplication
{
	Q_OBJECT

public:

	// Main window instance
	QMainWindow *m_pWnd;

	// Main window's UI
	Ui::MainWindow *m_pWndUI;

public:

	// Standard constructor
    CWsTest(int &argc, char **argv);

	~CWsTest();

    // Connect signals and slots
	void setupSignals();

	void setupGui();

	bool event(QEvent *pEvent);

public slots:

	// Exit button clicked
	void pbExit_Clicked();

	// Break button clicked
	void pbBreak_Clicked();

	// Clear button clicked
	void pbClear_Clicked();

	// Login button clicked
	void pbLogin_Clicked();

	// Logoff button clicked
	void pbLogoff_Clicked();

	// VM Start button clicked
	void pbVmStart_Clicked();

	// VM Stop button clicked
	void pbVmStop_Clicked();

	// Reserved buttons clicked
	void pbRegNewVm_Clicked();

	// Open button clicked
	void pbOpen_Clicked();

	// Get VM List button clicked
	void pbGetVmList_Clicked();

	// Get Host hardware info
	void pbGetHostInfo_Clicked();

	// Remote file open dialog clicked
	void pbRemoteFileOpen_Clicked();

	// Remote file save dialog clicked
	void pbRemoteFileSave_Clicked();

	// Get User Profile
	void pbGetUserProfile_Clicked();

	// Test Problem Report XML
	void pbProblemReportXml_Clicked();

	// Test Vm Reset
	void pbVmReset_Clicked();

	// Test Vm Pause
	void pbVmPause_Clicked();

	// Test Vm Supend
	void pbVmSupend_Clicked();

	// Test Vm Resume
	void pbVmResume_Clicked();

	// Test UnReg VM
	void pbUnRegVm_Clicked();

	// Test Clone VM
	void pbCloneVm_Clicked();

	// Test Edit VM
	void pbEditVm_Clicked();

	// Test Delete VM
	void pbDeleteVm_Clicked();

	// Test Validate VM
	void pbValidateVm_Clicked();

	// Test Create Disk Image
	void pbCreateDiskImage_Clicked();

	// Test Install Guest OS
	void pbInstallGuestOs_Clicked();

	// Test Cancel Request
	void pbCancelRequest_Clicked();

	// Test Connect device
	void pbConnectDevice_Clicked();

	// Test Disconnect device
	void pbDisconnectDevice_Clicked();

	// Test Get VM Config command
	void pbGetVmConfig_Clicked();

   // Test Get VM Info command
   void pbGetVmInfo_Clicked();

   // Test Get VM Statistics command
   void pbGetVMStatistics_Clicked();

   // Test Get Dispatcher RTInfo command
   void pbSMCGetRTInfo_Clicked();

   // Test pbGetHistoryByVm  command
   void pbGetHistoryByVm_Clicked();

   // Test pbGetHistoryByUser command
   void pbGetHistoryByUser_Clicked();

   void pbShutdownDisp_Clicked();
   void pbRestartDisp_Clicked();
   void pbDisconnectUser_Clicked();
   void pbDisconnectAllUsers_Clicked();
   void pbCancelUserRequest_Clicked();
   void pbShutdownVM_Clicked();
   void pbRestartVM_Clicked();

private:

	// Output area text
	QString m_strOutputText;

	// Parsed arguments
	QStringList m_lstArguments;

	QString m_strLastDirectory;

	CVmConfiguration* pVmConfig;

	QString m_strRequestUuid;

	QString m_strLastVmConfig;
	QString m_strLastVmUuid;

	PRL_HANDLE m_ServerHandle;
	PRL_HANDLE m_hVm;

private:

	// Parse arguments
	void parseArguments();

	// Put line to output area
	void logOut(QString txt_line);

	void WriteLog(const char *szForm, ...);

	void ShowEvent(CVmEvent *pE);

	void ShowComplexEvent(CVmEvent *pE);

   void ShowRTInfoEvent(CVmEvent*pE);

   void ShowCommandHistoryEvent (CVmEvent *pE);
   void PrintRequestParamsFromCData(const QString& xml);
// template <class T> QString ElementToString(T Element, const QString& tagName);

	// template <class T> bool StringToElement(T Element, const QString& source_string);
};

#endif // __CWSTEST_H__
