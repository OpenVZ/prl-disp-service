///////////////////////////////////////////////////////////////////////////////
///
/// @file PrlNetLibrary.cpp
/// @author sdmitry
///
/// Functions implementations for Windows
///
/// Copyright (c) 2005-2015 Parallels IP Holdings GmbH
///
/// This file is part of Virtuozzo Core Libraries. Virtuozzo Core
/// Libraries is free software; you can redistribute it and/or modify it
/// under the terms of the GNU Lesser General Public License as published
/// by the Free Software Foundation; either version 2.1 of the License, or
/// (at your option) any later version.
///
/// This library is distributed in the hope that it will be useful,
/// but WITHOUT ANY WARRANTY; without even the implied warranty of
/// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
/// Lesser General Public License for more details.
///
/// You should have received a copy of the GNU Lesser General Public
/// License along with this library.  If not, see
/// <http://www.gnu.org/licenses/> or write to Free Software Foundation,
/// 51 Franklin Street, Fifth Floor Boston, MA 02110, USA.
///
/// Our contact details: Parallels IP Holdings GmbH, Vordergasse 59, 8200
/// Schaffhausen, Switzerland.
///
///////////////////////////////////////////////////////////////////////////////

#include "../PrlNetLibrary.h"
#include "../win/ethlist.h"
#include <winsock2.h>
#include <WS2tcpip.h>
#include "Libraries/PrlNetEnum/prl_net_enum.h"
#include "System/Network/services/ParallelsNetDaemon.h" // for WIN32_PARALLELS_NETSERVICE_NAME and WIN32_SERVICE_FILE_NAME
#include "ServiceControl.h"
#include <Libraries/Logging/Logging.h>
#include <Libraries/Std/scoped_array.h>
#include <Libraries/PrlNetworking/netconfig.h>
#include <Libraries/Std/scoped_mem.h>
#include "WinNetworkHelpers.h"
#include "../PrlNetInternal.h"

#include <QProcess>
#include <QFile>
#include <QStringList>
#include <QHostAddress.h>
#include <QObject>
#include <QtAlgorithms>
#include <cassert>
#include <set>

#define PARALLELS_NET_INSTALL_NAME "prl_net_inst.exe"
#define PARALLELS_VNIC_INF_FILE "prl_vnic.inf"
#define PARALLELS_VNIC_CONNECTION_NAME L"Parallels Virtual Adapter %d"
#define PARALLELS_NETSERVICE_NAME L"Parallels Networking Service"
#define PARALLELS_NETBRIDGE_INF_FILE "prl_net.inf"
#define PARALLLELS_NETBRIDGE_HARDW_ID "PARALLLELS_PVSNET"
#define WIN32_PARALLELS_NETBRIDGE_SERVICE_NAME L"pvsnet"
#define PARALLELS_MAXIMUM_ADAPTER_INDEX 6

// 30 secs of timeout to give time to the Process finished in startProcess()
#define PRL_PROCESS_EXECUTE_TIMEOUT 30000

// last system error
static unsigned int s_LastSystemError;
static void inline MODULE_STORE_SYSTEM_ERROR()
{
	s_LastSystemError = ::GetLastError();
}


int		   PrlNet::getMaximumAdapterIndex()
{
	return PARALLELS_MAXIMUM_ADAPTER_INDEX;
}


// returns true if deviceId1 matches deviceId2. If bAllowPartialMatch is true,
// device will also match if deviceId1 = deviceId2 + someX ( prlvnic2 will match prlvnic )
static bool MatchDeviceId( IN LPCWSTR deviceId1, IN LPCWSTR deviceId2, IN BOOL bAllowPartialMatch )
{
	if( !bAllowPartialMatch )
	{
		return 0 == _wcsicmp(deviceId1, deviceId2);
	}
	else
	{
		int len1 = wcslen(deviceId1);
		int len2 = wcslen(deviceId2);
		if( len2 > len1 )
			return false;

		return ( 0 == _wcsnicmp(deviceId1, deviceId2, len2) );
	}
}


//
// searches multistring searchList for presence stringToSearch
// if bAllowPartitialMatch is true, substring will be searched
static const WCHAR *searchList( const WCHAR *searchList, DWORD bufSize, const WCHAR *stringToSearch, bool bAllowPartialMatch )
{
	//
	// Compare each entry in the buffer multi-sz list with our HardwareID.
	//
	for ( const WCHAR *p=searchList;*p&&(p<&searchList[bufSize]);p+=wcslen(p) + 1 )
	{
		// No garbage output!
		//_tprintf(TEXT("Compare device ID: [%s]\n"),p);

		if( MatchDeviceId(p, stringToSearch, bAllowPartialMatch ) )
		{
			return p;
		}
	}
	return NULL;
}

PRL_RESULT PrlNet::makePrlAdaptersList( PrlNet::EthAdaptersList &adaptersList )
{
	adaptersList.clear();

	NetAdapterFindHandle hFindAdapter = ::FindFirstNetAdapter();
	if( NULL == hFindAdapter )
	{
		MODULE_STORE_SYSTEM_ERROR();

		WRITE_TRACE(DBG_FATAL, "[PrlNet]  Error creating list of Parallels Adapters: %d",
			s_LastSystemError );
		return PRL_NET_SYSTEM_ERROR;
	}

	BOOL bContinue = TRUE;
	while(bContinue)
	{
		WCHAR *szCompDescription = NULL;
		WCHAR *szHardwIdsList = NULL;
		DWORD dwHardwIdsListSize = 0;
		DWORD dwSize = 0;

		if( !::GetAdapterHardwareIds(hFindAdapter, &szHardwIdsList, &dwHardwIdsListSize) )
		{
			// error obtaining adapter hardware Ids.. skip adapter.
			bContinue = ::FindNextAdapter(hFindAdapter);
			continue;
		}

		::GetAdapterDescription( hFindAdapter, &szCompDescription, &dwSize );

		const WCHAR *szId = searchList(szHardwIdsList, dwHardwIdsListSize, L"*prlvnic", true);
		if( NULL != szId )
		{
			// last digit in id - index.
			int idx = -1;
			WCHAR wc = szId[wcslen(szId) - 1];
			if( wc >= '0' && wc <= '9' )
				idx = wc - '0';

			BOOL bAdapterEnabled = TRUE;
			::AdapterIsEnabled( hFindAdapter, &bAdapterEnabled );


			WCHAR szAdapterName[PRLNET_MAX_ADAPTER_NAME_LEN];
			HRESULT hrGetNameResult = HrGetNetAdapterName( szId, NC_NetAdapter, szAdapterName );

			PrlNet::EthernetAdapter ethAdapter;
			ethAdapter._name = QString::fromWCharArray( SUCCEEDED(hrGetNameResult) ? szAdapterName : L"error" );
			ethAdapter._systemName = QString::fromWCharArray( szCompDescription ? szCompDescription : L"error" );
			ethAdapter._adapterIndex = idx | PRL_ADAPTER_START_INDEX;
			ethAdapter._adapterGuid = "{does-not-matter}";
			ethAdapter._bEnabled = bAdapterEnabled ? true:false;
			ethAdapter._bParallelsAdapter = true;
			ethAdapter._vlanTag = PRL_INVALID_VLAN_TAG;
			memset( ethAdapter._macAddr, 0, 6 );
			adaptersList.push_back(ethAdapter);
		}

		::LocalFree(szHardwIdsList);
		::LocalFree(szCompDescription);

		bContinue = ::FindNextAdapter(hFindAdapter);
		continue;
	}

	::FindCloseAdapter(hFindAdapter);

	qSort(adaptersList); // sort adapters

	return PRL_ERR_SUCCESS;
}


PRL_RESULT PrlNet::enablePrlAdapter( int adapterIndex, bool bEnable )
{
	WCHAR szAdapterId[16];
	wsprintf(szAdapterId, L"*prlvnic%d", GET_PRL_ADAPTER_NUMBER(adapterIndex) );

	NetAdapterFindHandle hFindAdapter = ::FindFirstNetAdapter();
	if( NULL == hFindAdapter )
	{
		MODULE_STORE_SYSTEM_ERROR();

		WRITE_TRACE(DBG_FATAL, "[PrlNet] Error enabling adapter: search failed %d",
					s_LastSystemError );
		return PRL_NET_SYSTEM_ERROR;
	}

	BOOL bContinue = TRUE;
	bool bFound = false;
	DWORD dwErr = 0;

	while(bContinue)
	{
		WCHAR *szHardwIdsList = NULL;
		DWORD dwHardwIdsListSize = 0;

		if( !::GetAdapterHardwareIds(hFindAdapter, &szHardwIdsList, &dwHardwIdsListSize) )
		{
			// error obtaining adapter hardware Ids.. skip adapter.
			bContinue = ::FindNextAdapter(hFindAdapter);
			continue;
		}

		const WCHAR *szId = searchList(szHardwIdsList, dwHardwIdsListSize, szAdapterId, false);
		if( NULL != szId )
		{
			bFound = true;

			BOOL bAdapterEnabled = TRUE;
			::AdapterIsEnabled( hFindAdapter, &bAdapterEnabled );
			if( bEnable && bAdapterEnabled
				|| !bEnable && !bAdapterEnabled )
			{
				::LocalFree(szHardwIdsList);
				break;
			}

			if( !::EnableAdapter(hFindAdapter, bEnable ? TRUE:FALSE) )
			{
				MODULE_STORE_SYSTEM_ERROR();

				dwErr = s_LastSystemError;
				WRITE_TRACE(DBG_FATAL, "[PrlNet] Error enabling adapter: %d",
					s_LastSystemError );
			}
			::LocalFree(szHardwIdsList);
			break;
		}

		::LocalFree(szHardwIdsList);

		bContinue = ::FindNextAdapter(hFindAdapter);
	}

	::FindCloseAdapter(hFindAdapter);

	::SetLastError(dwErr);
	MODULE_STORE_SYSTEM_ERROR();

	if( !bFound )
	{
		WRITE_TRACE(DBG_FATAL, "[PrlNet] Error enabling adapter: Adapter %d is not installed",
			GET_PRL_ADAPTER_NUMBER(adapterIndex) );
		return PRL_NET_ADAPTER_NOT_EXIST;
	}

	if( dwErr )
	{
		return PRL_NET_SYSTEM_ERROR;
	}

	return PRL_ERR_SUCCESS;
}


bool PrlNet::isWIFIAdapter(const EthernetAdapter& ethAdapter)
{
	// always false for Windows.
	UNUSED_PARAM(ethAdapter);

	return false;
}


bool PrlNet::isVirtualAdapter(const EthernetAdapter& ethAdapter)
{
	// always false for Windows.
	UNUSED_PARAM(ethAdapter);

	return false;
}


PRL_RESULT PrlNet::renamePrlAdapter(
    const QString &prlDriversDir,
    int adapterIndex,
    bool bHiddenAdapter,
    const QString &newName )
{
	UNUSED_PARAM(prlDriversDir);
	UNUSED_PARAM(bHiddenAdapter);

	WCHAR szAdapterId[16];
	wsprintf(szAdapterId, L"*prlvnic%d", GET_PRL_ADAPTER_NUMBER(adapterIndex) );
	HRESULT hr = HrStoreNetAdapterName( szAdapterId, NC_NetAdapter, (WCHAR *)newName.utf16() );
	if( S_OK == hr )
	{
		return PRL_ERR_SUCCESS;
	}
	else
	{
		WRITE_TRACE(DBG_FATAL, "Failed to change name of the adapter %d: error 0x%x", hr );
		return PRL_NET_RENAME_FAILED;
	}
}

/// returns TRUE if we are 32-bit process which is running on the 64-bit OS
static BOOL prlIsWow64Process()
{
	BOOL bIsWow64 = FALSE;
	typedef BOOL (WINAPI *LPFN_ISWOW64PROCESS) (HANDLE, PBOOL);
	LPFN_ISWOW64PROCESS fnIsWow64Process = (LPFN_ISWOW64PROCESS)GetProcAddress(
		GetModuleHandle(TEXT("kernel32")),"IsWow64Process");
	if( NULL != fnIsWow64Process )
	{
		fnIsWow64Process( GetCurrentProcess(), &bIsWow64 );
	}
	return bIsWow64;

}
// returns path to the PARALLELS_NET_INSTALL
static bool getPrlNetInstallPath(const QString &prlDriversDir, QString &prl_net_install)
{
	// find out correct path to the PARALLELS_NET_INSTALL_NAME
	prl_net_install = prlDriversDir + "\\" + PARALLELS_NET_INSTALL_NAME;
	if( !prlIsWow64Process() )
	{
		prl_net_install = prlDriversDir + "\\" + PARALLELS_NET_INSTALL_NAME;
		return QFile::exists(prl_net_install);
	}

	// we are running on the 64-bit machine.
	// since we don't know whether end-user version or development version is running now,
	// we must find out correct way to the prl_net_install.
	prl_net_install = prlDriversDir + "\\AMD64\\" + PARALLELS_NET_INSTALL_NAME;
	bool bFileExists = QFile::exists(prl_net_install);
	if( !bFileExists )
	{
		prl_net_install = prlDriversDir + "\\" + PARALLELS_NET_INSTALL_NAME;
		bFileExists = QFile::exists(prl_net_install);
	}

	return bFileExists;
}


// Encloses program-name to "" and converts slashes to windows-way
static QString normalizeProgramName(const QString &program)
{
	QString programName = program;
	if (!programName.startsWith(QLatin1Char('\"')) &&
			!programName.endsWith(QLatin1Char('\"')) &&
			programName.contains(QLatin1String(" ")))
		programName = QLatin1String("\"") + programName + QLatin1String("\"");
	programName.replace(QLatin1String("/"), QLatin1String("\\"));
	return programName;
}


/**
 * Method is based on the Qt method that was not unfortunately exported
 * (qt/src/corelib/io/qprocess_win.cpp)
 * QT Commercial license
 * http://trolltech.com/products/qt/licenses/licensing
 */
static QString qt_create_commandline(const QString &program, const QStringList &arguments)
{
	QString programName = normalizeProgramName(program);

	QString args;
	// add the prgram as the first arrg ... it works better
	args = programName + QLatin1String(" ");
	for (int i=0; i<arguments.size(); ++i) {
		QString tmp = arguments.at(i);
		// in the case of \" already being in the string the \ must also be escaped
		tmp.replace( QLatin1String("\\\""), QLatin1String("\\\\\"") );
		// escape a single " because the arguments will be parsed
		tmp.replace( QLatin1String("\""), QLatin1String("\\\"") );
		if (tmp.isEmpty() || tmp.contains(QLatin1Char(' ')) || tmp.contains(QLatin1Char('\t'))) {
			// The argument must not end with a \ since this would be interpreted
			// as escaping the quote -- rather put the \ behind the quote: e.g.
			// rather use "foo"\ than "foo\"
			QString endQuote(QLatin1String("\""));
			int i = tmp.length();
			while (i>0 && tmp.at(i-1) == QLatin1Char('\\')) {
				--i;
				endQuote += QLatin1String("\\");
			}
			args += QLatin1String(" \"") + tmp.left(i) + endQuote;
		} else {
			args += QLatin1Char(' ') + tmp;
		}
	}
	return args;
}


static PRL_RESULT __startProcessWithCmdLine(
	const QString &commandLine,
	HANDLE hUserToken,
	DWORD &dwProcessExitCode )
{
	dwProcessExitCode = 0;

	DWORD dwCreationFlags = CREATE_NO_WINDOW;
	STARTUPINFOW si;
	memset( &si, 0, sizeof(si) );
	si.cb = sizeof( STARTUPINFO );

	PROCESS_INFORMATION pi;
	memset( &pi, 0, sizeof(pi) );

	BOOL bResult = FALSE;

	if (NULL != hUserToken)
	{
		bResult = CreateProcessAsUserW(
			hUserToken,
			0, (WCHAR*)commandLine.utf16(),
			0, 0, TRUE, dwCreationFlags,
			NULL,
			NULL,
			&si, &pi );
	}
	else
	{
		bResult = CreateProcessW(
			0, (WCHAR*)commandLine.utf16(),
			0, 0, TRUE, dwCreationFlags,
			NULL,
			NULL,
			&si, &pi );
	}

	if( !bResult )
	{
		MODULE_STORE_SYSTEM_ERROR();

		WRITE_TRACE(DBG_FATAL, "Failed to execute %s: error %x",
			QSTR2UTF8(commandLine), s_LastSystemError);
		return PRL_NET_SYSTEM_ERROR;
	}

	BOOL bProcessFinished = (::WaitForSingleObject(pi.hProcess, PRL_PROCESS_EXECUTE_TIMEOUT ) == WAIT_OBJECT_0);
	::GetExitCodeProcess(pi.hProcess, &dwProcessExitCode);

	::CloseHandle(pi.hProcess);
	::CloseHandle(pi.hThread);

	if( bProcessFinished )
	{
		return PRL_ERR_SUCCESS;
	}
	else
	{
		WRITE_TRACE(DBG_FATAL,
			"Timeout waiting while command %s is completed.",
			QSTR2UTF8(commandLine));
		return PRL_NET_INSTALL_TIMEOUT;
	}
}


static PRL_RESULT startProcess(
	const QString &processName,
	const QStringList &args,
	HANDLE hUserToken,
	DWORD &dwProcessExitCode )
{
	dwProcessExitCode = 0;

	QString commandLine = qt_create_commandline(processName, args);
	return __startProcessWithCmdLine(commandLine, hUserToken, dwProcessExitCode);
}


static PRL_RESULT startProcessWithCmdLine(
	const QString &processName,
	const QString &args,
	HANDLE hUserToken,
	DWORD &dwProcessExitCode )
{
	dwProcessExitCode = 0;

	QString programName = normalizeProgramName(processName);

	// add the prgram as the first arrg ... it works better
	QString commandLine;
	commandLine = programName + QLatin1String(" ") + args;

	return __startProcessWithCmdLine(commandLine, hUserToken, dwProcessExitCode);
}


static PRL_RESULT installPrlNetbridgeDriver( const QString &prlDriversDir )
{
	// find out correct path to the PARALLELS_NET_INSTALL_NAME
	QString prl_net_install;
	if( !getPrlNetInstallPath(prlDriversDir, prl_net_install) )
	{
		return PRL_ERR_INSTALLATION_PROBLEM;
	}

	QString prlNetbridgeInfFile = prlDriversDir + "\\" + PARALLELS_NETBRIDGE_INF_FILE;
	QStringList args;
	args<<"-InstallNet"<<prlNetbridgeInfFile<<PARALLLELS_NETBRIDGE_HARDW_ID;

	DWORD dwResult = 0;

	PRL_RESULT prlResult = startProcess( prl_net_install, args, NULL, dwResult );
	if( !PRL_SUCCEEDED(prlResult) )
	{
		return prlResult;
	}

	if( dwResult == 2 )
	{
		WRITE_TRACE(DBG_FATAL, "prl_net_install returned FAILURE while installing net adapter");
		return PRL_NET_INSTALL_FAILED;
	}

	CServiceControl scm;
	if( !scm )
	{
		MODULE_STORE_SYSTEM_ERROR();

		WRITE_TRACE(DBG_FATAL, "[installPrlNetbridgeDriver] Error starting Service: ServiceManager open failed: %d",
			s_LastSystemError );
		return PRL_NET_WINSCM_OPEN_ERROR;
	}

	scm.StartService( WIN32_PARALLELS_NETBRIDGE_SERVICE_NAME );

	return PRL_ERR_SUCCESS;
}

PRL_RESULT PrlNet::installPrlAdapter(
	void *hUserToken,
	const QString &prlDriversDir,
	int adapterIndex,
	bool bHiddenAdapter,
	const QString &adapterName,
	PrlNet::EthernetAdapter &adapter)
{
	UNUSED_PARAM(bHiddenAdapter);
	// find out correct path to the PARALLELS_NET_INSTALL_NAME
	QString prl_net_install;
	if( !getPrlNetInstallPath(prlDriversDir, prl_net_install) )
	{
		return PRL_ERR_INSTALLATION_PROBLEM;
	}

	QString prlVNicInfFile = prlDriversDir + "\\" + PARALLELS_VNIC_INF_FILE;
	QString adapterId;
	adapterId.sprintf("*prlvnic%d", GET_PRL_ADAPTER_NUMBER(adapterIndex));

	QStringList args;
	args<<"-InstallNet"<<prlVNicInfFile<<adapterId<<adapterName;

	DWORD dwResult = 0;

	PRL_RESULT prlResult = startProcess( prl_net_install, args, hUserToken, dwResult );
	if( !PRL_SUCCEEDED(prlResult) )
	{
		return prlResult;
	}

	if( dwResult == 2 )
	{
		WRITE_TRACE(DBG_FATAL, "prl_net_install returned FAILURE while installing net adapter");
		return PRL_NET_INSTALL_FAILED;
	}

	//
	// Fill resulting adapter to return to caller
	//
	adapterIndex |= PRL_ADAPTER_START_INDEX;
	PrlNet::EthAdaptersList ethList;
	makePrlAdaptersList(ethList);
	for( PrlNet::EthAdaptersList::iterator it = ethList.begin(); it != ethList.end(); ++it )
	{
		if( it->_adapterIndex == adapterIndex )
		{
			adapter = *it;
			break;
		}
	}

	return PRL_ERR_SUCCESS;
}


PRL_RESULT PrlNet::uninstallPrlAdapter( const QString &prlDriversDir, int adapterIndex )
{
	// find out correct path to the PARALLELS_NET_INSTALL_NAME
	QString prl_net_install;
	if( !getPrlNetInstallPath(prlDriversDir, prl_net_install) )
	{
		return PRL_ERR_INSTALLATION_PROBLEM;
	}

	QString prlVNicInfFile = prlDriversDir + "\\" + PARALLELS_VNIC_INF_FILE;
	QString adapterId;
	adapterId.sprintf("*prlvnic%d", GET_PRL_ADAPTER_NUMBER(adapterIndex));

	QStringList args;
	args<<"-RemoveNet"<<adapterId;

	DWORD dwResult = 0;

	PRL_RESULT prlResult = startProcess( prl_net_install, args, NULL, dwResult );
	if( !PRL_SUCCEEDED(prlResult) )
	{
		return prlResult;
	}

	if( dwResult == 2 )
	{
		WRITE_TRACE(DBG_FATAL, "prl_net_install returned FAILURE while uninstalling net adapter");
		return PRL_NET_UNINSTALL_FAILED;
	}

	return PRL_ERR_SUCCESS;
}

//
// Service section
//


PRL_RESULT PrlNet::installPrlService( const QString &parallelsDir )
{
	CServiceControl scm;
	if( !scm )
	{
		MODULE_STORE_SYSTEM_ERROR();

		WRITE_TRACE(DBG_FATAL, "[PrlNet] Error Installing Service: ServiceManager open failed: %d",
			s_LastSystemError );
		return PRL_NET_WINSCM_OPEN_ERROR;
	}

	// form name
	std::wstring serviceFile = parallelsDir.toStdWString();
	serviceFile += L"\\";
	serviceFile += WIN32_PARALLELS_NETSERVICE_FILE_NAME;

	// silently ignore start/install request if file does not exists:
	// dispatcher is installed standalone to work with CT only - VZWIN
	if (!QFile::exists(UTF16_2QSTR(serviceFile.c_str())))
		return PRL_ERR_SUCCESS;

	BOOL bRes = scm.InstallService(
		WIN32_PARALLELS_NETSERVICE_NAME,
		serviceFile.c_str(),
		WIN32_PARALLELS_NETSERVICE_DESCRIPTION,
		SERVICE_WIN32_OWN_PROCESS,
		SERVICE_AUTO_START );

	if( !bRes )
	{
		MODULE_STORE_SYSTEM_ERROR();

		WRITE_TRACE(DBG_FATAL, "Error Installing Parallels Networking service: %d",
			s_LastSystemError );
		return PRL_NET_SYSTEM_ERROR;
	}
	else
	{
		scm.RegisterApplicationLog( WIN32_PARALLELS_NETSERVICE_NAME, serviceFile.c_str() );
		scm.StartService(WIN32_PARALLELS_NETSERVICE_NAME);
	}

	return PRL_ERR_SUCCESS;
}


PRL_RESULT PrlNet::uninstallPrlService( )
{
	CServiceControl scm;
	if( !scm )
	{
		MODULE_STORE_SYSTEM_ERROR();

		WRITE_TRACE(DBG_FATAL, "[PrlNet] Error uninstalling Service: ServiceManager open failed: %d",
			 s_LastSystemError );
		return PRL_NET_WINSCM_OPEN_ERROR;
	}

	scm.StopService(WIN32_PARALLELS_NETSERVICE_NAME);
	scm.DeregisterApplicationLog(WIN32_PARALLELS_NETSERVICE_NAME);
	scm.RemoveService(WIN32_PARALLELS_NETSERVICE_NAME);

	return PRL_ERR_SUCCESS;
}


PRL_RESULT PrlNet::startPrlNetService(const QString &parallelsDir,  PrlNet::SrvAction::Action action )
{
	UNUSED_PARAM(parallelsDir);

	CServiceControl scm;
	if( !scm )
	{
		MODULE_STORE_SYSTEM_ERROR();

		WRITE_TRACE(DBG_FATAL, "[PrlNet] Error starting Service: ServiceManager open failed: %d",
			s_LastSystemError );
		return PRL_NET_WINSCM_OPEN_ERROR;
	}

	PrlNet::SrvStatus::Status srvStatus;
	PRL_RESULT prlResult = getPrlNetServiceStatus(&srvStatus);
	if( PRL_ERR_SUCCESS != prlResult )
	{
		return prlResult;
	}

	switch(action)
	{
	case PrlNet::SrvAction::Start:
		if( PrlNet::SrvStatus::Stopped == srvStatus )
		{
			scm.StartService(WIN32_PARALLELS_NETSERVICE_NAME);
		}
		else if (PrlNet::SrvStatus::NotInstalled == srvStatus )
		{
			prlResult = PrlNet::installPrlService(parallelsDir);
		}
		break;

	case PrlNet::SrvAction::Stop:
		if( PrlNet::SrvStatus::Stopped == srvStatus || PrlNet::SrvStatus::Started == srvStatus )
		{
			scm.StopService(WIN32_PARALLELS_NETSERVICE_NAME);
		}
		break;

	default:
		assert(0);
		MODULE_STORE_SYSTEM_ERROR();

		WRITE_TRACE(DBG_FATAL, "Parallels Networking service: unknown operation requested: %d",
			 s_LastSystemError );
		return PRL_NET_SRV_NOTIFY_ERROR;
	}

	return prlResult;
}


PRL_RESULT PrlNet::getPrlNetServiceStatus(PrlNet::SrvStatus::Status *pStatus)
{
	CServiceControl scm;
	if( !scm )
	{
		MODULE_STORE_SYSTEM_ERROR();

		WRITE_TRACE(DBG_FATAL, "[PrlNet] Error obtaining service status: ServiceManager open failed: %d",
			 s_LastSystemError );
		return PRL_NET_WINSCM_OPEN_ERROR;
	}

	::SERVICE_STATUS serviceStatus;
	if( !scm.QueryServiceStatus(WIN32_PARALLELS_NETSERVICE_NAME, &serviceStatus) )
	{
		*pStatus = PrlNet::SrvStatus::NotInstalled;
		return PRL_ERR_SUCCESS;
	}

	switch(serviceStatus.dwCurrentState)
	{
	case SERVICE_START_PENDING:
	case SERVICE_RUNNING:
		*pStatus = PrlNet::SrvStatus::Started;
		return PRL_ERR_SUCCESS;
	case SERVICE_STOPPED:
	case SERVICE_STOP_PENDING:
		*pStatus = PrlNet::SrvStatus::Stopped;
		return PRL_ERR_SUCCESS;
	default:
		assert(0);
		*pStatus = PrlNet::SrvStatus::Stopped; // in hope that soon all will come to the normal state
		return PRL_ERR_SUCCESS;
	}
}


PRL_RESULT PrlNet::notifyPrlNetService(const QString &parallelsDir)
{
	UNUSED_PARAM(parallelsDir);
	CServiceControl scm;
	if( !scm )
	{
		MODULE_STORE_SYSTEM_ERROR();

		WRITE_TRACE(DBG_FATAL, "[PrlNet] Error notifying Service: ServiceManager open failed: %d",
			s_LastSystemError );
		return PRL_NET_WINSCM_OPEN_ERROR;
	}

	PrlNet::SrvStatus::Status srvStatus;
	PRL_RESULT prlResult = getPrlNetServiceStatus(&srvStatus);
	if( PRL_ERR_SUCCESS != prlResult )
	{
		return prlResult;
	}

	if( PrlNet::SrvStatus::Started == srvStatus )
	{
		if( !scm.ControlService(WIN32_PARALLELS_NETSERVICE_NAME, WIN32_PARALLELS_NETSERVICE_NOTIFY_CODE) )
		{
			MODULE_STORE_SYSTEM_ERROR();

			LOG_MESSAGE(DBG_FATAL, "[PrlNet::notifyPrlNetService] Error notifying NAPT daemon: %d",
				s_LastSystemError );
			return PRL_NET_SYSTEM_ERROR;
		}

		return PRL_ERR_SUCCESS;
	}
	else
	{
		return PrlNet::startPrlNetService(parallelsDir, PrlNet::SrvAction::Start);
	}
}

unsigned long PrlNet::getSysError()
{
	return s_LastSystemError;
}


void PrlNet::setSysError(unsigned long err)
{
	s_LastSystemError = err;
}


QString PrlNet::getSysErrorText()
{
	char tmpBuf[512];
	DWORD dwErr = s_LastSystemError;
	if (!FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		0, dwErr, 0, tmpBuf, _countof(tmpBuf), 0) )
	{
		sprintf_s(tmpBuf, _countof(tmpBuf),
			"Unable to obtain system message text. Original error: %d", dwErr);
	}

	return tmpBuf;
}


void	PrlNet::getDefaultDhcpParams(
	int 	adapterIndex,
	quint32 &dhcpScopeStartIp,
	quint32 &dhcpScopeEndIp,
	quint32 &dhcpScopeMask)
{
	PrlNet::GetDefaultDhcpParams(
		GET_PRL_ADAPTER_NUMBER(adapterIndex),
		dhcpScopeStartIp,
		dhcpScopeEndIp,
		dhcpScopeMask );
}


PRL_RESULT PrlNet::stopNetworking(const QString &parallelsDir)
{
	UNUSED_PARAM(parallelsDir);
	return PRL_ERR_SUCCESS;
}


// returns windows-guid of parallels adapter
static PRL_RESULT
GetPrlAdapterGuid(int adapterIndex, QString &adapterGuid)
{
	// find out the parallels adapter guid.
	EthIfaceList ethList;
	if ( !::makeEthIfacesList( ethList, true ) )
	{
		MODULE_STORE_SYSTEM_ERROR();
		WRITE_TRACE(DBG_FATAL, "[setPrlAdapterIpAddress]  makeEthIfacesList returned error: %d",
			s_LastSystemError );
		return PRL_NET_ETHLIST_CREATE_ERROR;
	}

	for ( EthIfaceList::iterator it = ethList.begin();
		it != ethList.end();
		++it )
	{
		// consider only real ethernet adapters
		if ( !IS_PRL_ADAPTER_INDEX(it->_nAdapter) )
			continue;

		if ( GET_PRL_ADAPTER_NUMBER(it->_nAdapter) == GET_PRL_ADAPTER_NUMBER(adapterIndex)
			&& it->_adapterGuid.length() > 8 )
		{
			adapterGuid = it->_adapterGuid.toAscii().constData() + 8; // skip "\DEVICE\" prefix
			return PRL_ERR_SUCCESS;
		}
	}
	WRITE_TRACE(DBG_FATAL, "[GetPrlAdapterGuid] Adapter %x was not found", adapterIndex);
	return PRL_ERR_FAILURE;
}


static PRL_RESULT exec_netsh(const QString &cmdline)
{
	WRITE_TRACE(DBG_DEBUG, "exec_netsh: %s", QSTR2UTF8(cmdline));

	WCHAR systemDirectoryBuf[MAX_PATH];
	if (!GetSystemDirectoryW(systemDirectoryBuf, ARRAY_SIZE(systemDirectoryBuf))) {
		MODULE_STORE_SYSTEM_ERROR();

		WRITE_TRACE(DBG_FATAL, "GetSystemDirectory() failed: %d",
			s_LastSystemError );
		return PRL_NET_SYSTEM_ERROR;
	}

	QString netsh_cmd = QString::fromUtf16(systemDirectoryBuf);
	netsh_cmd += "\\netsh.exe";

	DWORD dwResult;
	PRL_RESULT res = startProcessWithCmdLine(netsh_cmd, cmdline, NULL, dwResult);
	if (PRL_FAILED(res))
		return res;

	if (0 != dwResult)
		WRITE_TRACE(DBG_FATAL, "netsh.exe finished with %d: %s",
			dwResult, QSTR2UTF8(cmdline));
	return PRL_ERR_SUCCESS;
}


static PRL_RESULT EnableIPv6RouterDiscovery(int iface_idx, BOOL enable)
{
	QString cmd;
	cmd.sprintf("interface ipv6 set interface %d routerdiscovery=%s",
		iface_idx, enable ? "enable" : "disable");
	return exec_netsh(cmd);
}


PRL_RESULT PrlNet::DisablePrlIPv6RouterDiscovery(int prlAdapterIndex)
{
	int idx;
	if (!PrlNetInternal::GetPrlAdapterSysIndex(prlAdapterIndex, idx)) {
		WRITE_TRACE(DBG_FATAL, "Failed to obtain system-index of parallels-adapter %d",
			prlAdapterIndex);
		return PRL_ERR_FAILURE;
	}
	return EnableIPv6RouterDiscovery(idx, FALSE);
}


static bool compare_ip6_addrmask(
	const PrlNetInternal::IP6AddrMask &a,
	const PrlNetInternal::IP6AddrMask &b)
{
	if (a == b)
		return true;
	if (a.first != b.first)
		return false;
	return a.second == 0 || b.second == 0;
}


static PRL_RESULT del_ip6_address(
	int iface_sysidx, const QHostAddress &ip6)
{
	QString cmd;
	cmd.sprintf("interface ipv6 del address %d %s",
		iface_sysidx, QSTR2UTF8(ip6.toString()));
	return exec_netsh(cmd);
}


static PRL_RESULT set_ip6_address(
	int iface_sysidx, const QHostAddress &ip6, int prefixLen)
{
	QString cmd;
	cmd.sprintf("interface ipv6 set address %d %s/%d",
		iface_sysidx, QSTR2UTF8(ip6.toString()), prefixLen);
	return exec_netsh(cmd);
}


static PRL_RESULT setPrlAdapterIPv6Address(
	int adapterIndex,
	const QHostAddress &ip6, int prefixLen)
{
	int iface_sysidx;
	if (!PrlNetInternal::GetPrlAdapterSysIndex(adapterIndex, iface_sysidx)) {
		WRITE_TRACE(DBG_FATAL, "Failed to obtain system-index of parallels-adapter %d",
			adapterIndex);
		return PRL_ERR_FAILURE;
	}

	// unfortunately it is too dangerous to leave RouterDiscovery
	// enabled if it is mistakenly set by user or third-party
	// software.
	PRL_RESULT res = EnableIPv6RouterDiscovery(iface_sysidx, FALSE);
	if (PRL_FAILED(res))
		return res;

	// Check whether address is already set
	QList<PrlNetInternal::IP6AddrMask> ipv6List;
	PrlNetInternal::getAdapterIPv6List(iface_sysidx, ipv6List);
	PrlNetInternal::IP6AddrMask need(ip6, prefixLen);
	foreach (PrlNetInternal::IP6AddrMask a, ipv6List)
		if (compare_ip6_addrmask(a, need))
			return PRL_ERR_SUCCESS;

	foreach (PrlNetInternal::IP6AddrMask a, ipv6List) {
		if (IN6_IS_ADDR_LINKLOCAL((const in6_addr *)a.first.toIPv6Address().c))
			continue;
		del_ip6_address(iface_sysidx, a.first);
	}

	return set_ip6_address(iface_sysidx, ip6, prefixLen);
}


typedef DWORD (WINAPI *SetAdapterIpAddress_t )( const char *szAdapterGUID,
											  DWORD dwDHCP,
											  DWORD dwNewIP,
											  DWORD dwMask,
											  DWORD dwGateway
											  );

static PRL_RESULT setPrlAdapterIPv4Address(
	int adapterIndex,
	bool bHiddenAdapter,
	quint32 ipAddress,
	quint32 netMask )
{
	UNUSED_PARAM(bHiddenAdapter);

	QString adapterGuid;
	PRL_RESULT prlResult = GetPrlAdapterGuid(adapterIndex, adapterGuid);
	if (PRL_FAILED(prlResult))
		return prlResult;

	// Check current adapter parameters.
	unsigned int prevIpAddr = 0;
	unsigned int prevNetMask = 0;
	prlResult = PrlNetInternal::getPrlSystemAdapterParams( adapterIndex, prevIpAddr, prevNetMask);

	if ( PRL_SUCCEEDED(prlResult) )
	{
		if (prevIpAddr == ipAddress && prevNetMask == netMask)
		{
			// Adapter already has this ip address.
			return PRL_ERR_SUCCESS;
		}
	}

	HINSTANCE lib = (HINSTANCE) LoadLibrary( TEXT("iphlpapi.dll") );
	if (!lib)
	{
		WRITE_TRACE(DBG_FATAL, "[setPrlAdapterIpAddress] Failed to open IpHlpApi.dll");
		return PRL_ERR_FAILURE;
	}

	SetAdapterIpAddress_t SetAdapterIpAddress = (SetAdapterIpAddress_t) GetProcAddress( lib, "SetAdapterIpAddress" );
	if (NULL == SetAdapterIpAddress)
	{
		WRITE_TRACE(DBG_FATAL, "[setPrlAdapterIpAddress] Failed to open IpHlpApi.dll entry point.");
		FreeLibrary(lib);
		return PRL_ERR_FAILURE;
	}

	DWORD dwErr = SetAdapterIpAddress(adapterGuid.toAscii(), 0, htonl(ipAddress), htonl(netMask), 0);
	if ( 0 != dwErr )
	{
		s_LastSystemError = dwErr;
		WRITE_TRACE(DBG_FATAL, "[setPrlAdapterIpAddress] Failed to setup ip addres: error 0x%08x.", dwErr);
		prlResult = PRL_NET_SYSTEM_ERROR;
	}
	else
	{
		prlResult = PRL_ERR_SUCCESS;
	}

	FreeLibrary(lib);

	return prlResult;
}

PRL_RESULT PrlNet::setPrlAdapterIpAddresses(
	int adapterIndex,
	bool bHiddenAdapter,
	const CHostOnlyNetwork *pNetwork)
{
	PRL_RESULT res4;
	PRL_RESULT res6 = PRL_ERR_SUCCESS;
	if (!pNetwork)
		return PRL_ERR_INVALID_ARG;

	const QHostAddress& ipAddress = pNetwork->getHostIPAddress();
	const QHostAddress& netMask = pNetwork->getIPNetMask();
	res4 = setPrlAdapterIPv4Address(adapterIndex, bHiddenAdapter,
			ipAddress.toIPv4Address(), netMask.toIPv4Address());

	if (PrlNet::isIPv6Enabled()) {
		const QHostAddress& ip6 = pNetwork->getHostIP6Address();
		const QHostAddress& ip6_mask = pNetwork->getIP6NetMask();
		int prefix_len = PrlNet::getIPv6PrefixFromMask(&ip6_mask.toIPv6Address());
		res6 = setPrlAdapterIPv6Address(adapterIndex, ip6, prefix_len);
		if (PRL_FAILED(res6)) {
			WRITE_TRACE(DBG_FATAL,
				"Failed to config IPv6 on vnic %d. Disabling IPv6 for SharedNetworking", adapterIndex);
			PrlNet::setIPv6Enabled(false);
		}
	}

	// return worse result
	return PRL_FAILED(res4) ? res4 : res6;
}

PRL_RESULT PrlNet::delPrlAdapterIpAddresses(int adapterIndex,
	bool bHiddenAdapter, const CHostOnlyNetwork *pNetwork)
{
	UNUSED_PARAM(adapterIndex);
	UNUSED_PARAM(bHiddenAdapter);
	UNUSED_PARAM(pNetwork);
	return PRL_ERR_SUCCESS;
}


PRL_RESULT PrlNet::VZSyncConfig(CParallelsNetworkConfig *pConfig, bool *pbConfigChanged)
{
    *pbConfigChanged = false;
    UNUSED_PARAM(pConfig);
    return PRL_ERR_SUCCESS;
}


bool PrlNet::updateArp(const QString &ip, const QString &src_hwaddr, const QString &iface)
{
	UNUSED_PARAM(ip);
	UNUSED_PARAM(src_hwaddr);
	UNUSED_PARAM(iface);
	return true;
}


bool PrlNet::IsIPv6DefaultRoutePresent()
{
	SmartPtr<PrlNetInternal::MIB_IPFORWARD_TABLE2> table = PrlNetInternal::getIpForwardTable2();
	if (!table)
		return false;
	for (ULONG i = 0;i < table->NumEntries; ++i) {
		if (isDefaultIpv6Route(&table->Table[i]))
			return true;
	}

	return false;
}
