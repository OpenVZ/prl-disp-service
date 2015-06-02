///////////////////////////////////////////////////////////////////////////////
///
/// @file CDspStarter.cpp
///
/// VM process start helper methods
///
/// @author sergeyt, sandro
///
/// Copyright (c) 2005-2015 Parallels IP Holdings GmbH
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
///////////////////////////////////////////////////////////////////////////////

// #define FORCE_LOGGING_LEVEL DBG_DEBUG

#include "CDspStarter.h"
#include "CDspService.h"

#include <Libraries/Std/ProcessWatcher.h>

#ifdef _WIN_
#include <windows.h>
#include <psapi.h>
#include <userenv.h>
#include "CAuth.h"
#include <QDir>
#else
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pwd.h>
#include <grp.h>
#include <unistd.h>
#include <errno.h>
#endif

#include <stdlib.h>

#include <QFile>

#include <Libraries/PrlCommonUtilsBase/ParallelsDirs.h>
#include "Libraries/Logging/Logging.h"

// By adding this interface we enable allocations tracing in the module
#include "Interfaces/Debug.h"

#include "CDspVzHelper.h"

#ifndef _WIN_
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#endif

#ifdef PSBM_MAC
#define PSBM_MAC_HW_DETECT "scripts/psbm-mac-hw-detect.py"
#endif

/**
 * Process functions
 */
void registerZombiWatcher()
{
#ifndef _WIN_
	Prl::InitEndProcessWatcher();
#endif
}

// Terminates proceess by pid. OS-independent variant
void terminateProcessByPid ( Q_PID pid )
{
#ifdef _WIN_
    ::TerminateProcess( pid->hProcess, 0 );
#else
    ::kill( pid_t(pid), SIGTERM );
#endif
}

#define SID_ARG_TAG		"--sid={"

static void freeArgs(char **args)
{
	if (args != NULL) {
		for (char **p = args; *p != NULL; p++)
			free(*p);
		free(args);
	}
}

static char **makeArgs(const QString &program,
                      const QStringList& arguments)
{
	int i = 0;
	char **args = reinterpret_cast<char**>(::calloc(arguments.count() + 2, sizeof(char*)));
	if( !args  )
	{
		WRITE_TRACE(DBG_FATAL, "Unable to allocate memory" );
		return NULL;
	}

	if (!program.isEmpty())
	{
		QByteArray encodedProg = QFile::encodeName(program);
		args[0] = reinterpret_cast<char*>(::malloc(encodedProg.size()+1));
		if( !args[0]  )
		{
			WRITE_TRACE(DBG_FATAL, "Unable to allocate memory" );
			goto err;
		}

		::memcpy( args[0], encodedProg.constData(), encodedProg.size() );
		args[0][encodedProg.size()] = 0;
		i++;
	}

	for (int j = 0; j < arguments.count(); ++j, ++i)
	{
		QByteArray arg = arguments.at(j).toLocal8Bit();
		args[i] = reinterpret_cast<char*>(::malloc(arg.size()+1));

		if( !args[i]  )
		{
			WRITE_TRACE(DBG_FATAL, "Unable to allocate memory" );
			goto err;
		}

		::memcpy( args[i], arg.constData(), arg.size() );
		args[i][arg.size()] = 0;
	}
	return args;

err:
	freeArgs(args);
	return NULL;
}

Q_PID createProcess ( const QString& program,
                      const QStringList& arguments,
                      const QString& workingDirectory,
                      const QStringList& env,
                      CAuthHelper& userAuthHelper,
                      const SmartPtr<CVmConfiguration> &pVmConfig,
                      const IOCommunication::SocketHandle& sock,
                      const enum PVE::VmBinaryMode binaryMode )
{

#ifdef _WIN_
	Q_UNUSED(pVmConfig);
	Q_UNUSED(binaryMode);
#else
	Q_UNUSED(sock);
#endif

	QStringList environment = env;
#ifdef _LIN_
	//https://bugzilla.sw.ru/show_bug.cgi?id=464634
#ifdef PSBM_MAC_HW_DETECT
	static char extmodel[20] = {'\0'};
	if (!strlen(extmodel))
	{
		QString sScriptAbsPath = QString( "%1/%2" ).arg( workingDirectory ).arg( PSBM_MAC_HW_DETECT );
		FILE *f = popen( QSTR2UTF8(sScriptAbsPath), "r" );
		if (f != NULL) {
			if (fgets(extmodel, sizeof(extmodel), f) != NULL)
				WRITE_TRACE(DBG_FATAL, "Detected model: '%s'", extmodel);
			else
			{
				int nError = errno;
				WRITE_TRACE(DBG_FATAL, "Fallback to default model. Script path: '%s' %d '%s'", QSTR2UTF8(sScriptAbsPath), nError, strerror(nError));
			}
			fclose(f);
		} else
			WRITE_TRACE(DBG_FATAL, "Error execing detection script");
	}
	if (strlen(extmodel))
		environment += QString("%1=%2").arg(PVS_APPLE_HARDWARE_MODEL_ENV).arg(QString(extmodel).trimmed());
#endif//PSBM_MAC_HW_DETECT
#endif//_LIN_
	QString args = create_commandline(program, arguments);
	QByteArray envlist = create_environment(environment);

	QString temp_args = args;
	int idx = temp_args.indexOf(SID_ARG_TAG);
	if (idx != -1)
	{
		QString qsFakeUuid = "XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX";
		temp_args.remove(idx + sizeof(SID_ARG_TAG)-1, qsFakeUuid.size());
		temp_args.insert(idx + sizeof(SID_ARG_TAG)-1, qsFakeUuid);
	}

	WRITE_TRACE(DBG_FATAL, "Creating process name '%s' dir '%s' environment '%s' arguments '%s'",
			QSTR2UTF8(program), QSTR2UTF8(workingDirectory), envlist.data(), QSTR2UTF8(temp_args) );

#ifdef _WIN_
	/**
	 * Duplicate token
	 */
	void *pToken = NULL;

	if (!DuplicateTokenEx(userAuthHelper.GetAuth()->GetTokenHandle(),
				MAXIMUM_ALLOWED, NULL, SecurityImpersonation, TokenPrimary, &pToken))
	{
		WRITE_TRACE(DBG_FATAL, "Couldn't duplicate user token. Error=%d", GetLastError());
		return 0;
	}

	//////////////////////////////////////////////////////////////////////////
	// set zero sessionId to allow execute Vm after local connected user make logoff from windows session
	// #460111  - disable for desktop products
	bool bEnableZeroSession = CDspService::isServerMode();
#if defined(_DEBUG)
	if( 0 != getenv("PRL_DISABLE_VM_ZERO_SESSION") )
	{
		if( bEnableZeroSession )
			WRITE_TRACE( DBG_FATAL, "NOTE: VM sessionId will be not zero by environment" );
		bEnableZeroSession = false;
	}
#endif
	// #PWE-3635 xp/w2k3 unable to start vm from different session than dispatcher session.
	QByteArray baVal = qgetenv("PRL_ENABLE_VM_ZERO_SESSION");
	if( !baVal.isEmpty() && baVal.toInt() == 1 )
	{
		if( !bEnableZeroSession )
			WRITE_TRACE( DBG_FATAL, "NOTE: VM sessionId will be set to ZERO by environment" );
		bEnableZeroSession = true;
	}

	if( bEnableZeroSession )
	{
		// Get dispatcher session id
		DWORD sessId = 0;
		BOOL res = ProcessIdToSessionId( GetCurrentProcessId(), &sessId );
		if ( ! res )
		{
			CloseHandle(pToken);
			WRITE_TRACE(DBG_FATAL, "Could not convert process id to session id. Error=%d",
					GetLastError());
			return 0;
		}

		// Set dispatcher session id
		res = SetTokenInformation( pToken, TokenSessionId,
				&sessId, sizeof(sessId) );
		if ( ! res ) {
			CloseHandle(pToken);
			WRITE_TRACE(DBG_FATAL, "Could not set token information for token. Error=%d",
					GetLastError());
			return 0;
		}
	}

	PROCESS_INFORMATION* pid = new PROCESS_INFORMATION;
	memset(pid, 0, sizeof(PROCESS_INFORMATION));

	DWORD dwCreationFlags = 0;
	if (!(QSysInfo::WindowsVersion & QSysInfo::WV_DOS_based))
		dwCreationFlags |= CREATE_NO_WINDOW;

	bool success = false;
	HANDLE pipeInWr = INVALID_HANDLE_VALUE;

	if (!(QSysInfo::WindowsVersion & QSysInfo::WV_DOS_based))
	{
		dwCreationFlags |= CREATE_UNICODE_ENVIRONMENT;
		STARTUPINFOW startupInfo = { sizeof( STARTUPINFO ), 0, L"", 0,
			(ulong)CW_USEDEFAULT,
			(ulong)CW_USEDEFAULT,
			(ulong)CW_USEDEFAULT,
			(ulong)CW_USEDEFAULT,
			0, 0, 0,
			STARTF_USESTDHANDLES,
			0, 0, 0,
			0, 0, 0
		};

		// If socket handle is valid:
		//   create pipes for child process STDIN
		if ( sock.isValid() ) {
			HANDLE pipeInRd = INVALID_HANDLE_VALUE;
			SECURITY_ATTRIBUTES saAttr;

			// Set the bInheritHandle flag so pipe handles are inherited.
			saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
			saAttr.bInheritHandle = TRUE;
			saAttr.lpSecurityDescriptor = NULL;

			// Create a pipe for the child process's STDIN.
			if ( ! ::CreatePipe(&pipeInRd, &pipeInWr, &saAttr, 0) ) {
				WRITE_TRACE(DBG_FATAL, "Can't create pipe err='%d'!",
						::GetLastError());
				goto done;
			}

			// Ensure the write handle to the pipe for STDIN is not inherited.
			if ( ! ::SetHandleInformation(pipeInWr, HANDLE_FLAG_INHERIT, 0) ) {
				WRITE_TRACE(DBG_FATAL, "Can't SetHandleInformation err='%d'!",
						::GetLastError());
				goto done;
			}

			// Set handle
			startupInfo.hStdInput = pipeInRd;
		}

		SECURITY_ATTRIBUTES saProcess, saThread;

		saProcess.nLength = sizeof(saProcess);
		saProcess.lpSecurityDescriptor = NULL;
		saProcess.bInheritHandle = FALSE;

		saThread.nLength = sizeof(saThread);
		saThread.lpSecurityDescriptor = NULL;
		saThread.bInheritHandle = FALSE;

		::SetLastError(ERROR_SUCCESS);
		success = ::CreateProcessAsUserW(pToken,0, (WCHAR*)args.utf16(),
				&saProcess, &saThread, TRUE, dwCreationFlags,
				environment.isEmpty() ? 0 : envlist.data(),
				workingDirectory.isEmpty() ? 0 : (WCHAR*)QDir::convertSeparators(workingDirectory).utf16(),
				&startupInfo, pid);

		if ( success && sock.isValid() ) {
			IOCommunication::SocketDuplicatedState dupState;
			if ( ! sock->duplicateSocket(pid->dwProcessId, dupState) ) {
				WRITE_TRACE(DBG_FATAL, "Can't duplicate socket err='%d'!",
						::WSAGetLastError());
				goto done;
			}
			quint32 stateSz = 0;
			void* stateBuff = dupState.getStateBuffer(stateSz);

			DWORD dwWritten = 0;
			DWORD bSuccess = ::WriteFile(pipeInWr, stateBuff, stateSz,
					&dwWritten, NULL);
			if ( ! bSuccess || dwWritten != stateSz ) {
				WRITE_TRACE(DBG_FATAL, "Can't write to duplicated socket to pipe!");
			}
		}
	}

done:
	DWORD dwErrNo = ::GetLastError();
	if ( pipeInWr != INVALID_HANDLE_VALUE )
		::CloseHandle(pipeInWr);
	::CloseHandle(pToken);
	WRITE_TRACE(DBG_FATAL, "CreateProcessAsUserW(): success=%d, errNp=%d pid=%d", success, dwErrNo, pid->dwProcessId);
	// fix bug#518 [ VM doesn't react on Reset, Suspend and Shut Down button on tool bar ]
	//// if ( !success || dwErrNo!=ERROR_SUCCESS ) //for fix bug#226 (CreateProcess() return success but GetLastError() return error 1307
	////                                              //(ERROR_INVALID_OWNER: This security ID may not be assigned as the owner of this object.) )
	if ( !success )
	{
		WRITE_TRACE(DBG_FATAL, "Cant't create process success=%d, errNp=%d", success, dwErrNo);
		delete pid;
		pid = 0;
	}

	return pid;

#else // _WIN_


	//Extract user info with reentrant system calls
	struct passwd *pResultUserInfo = NULL;
#ifdef _LIN_
	struct passwd userInfo;
	QByteArray _passwd_strings_buf;
	_passwd_strings_buf.resize(sysconf(_SC_GETPW_R_SIZE_MAX));

	::getpwnam_r(	userAuthHelper.getUserName().toUtf8().constData(), &userInfo, _passwd_strings_buf.data(), _passwd_strings_buf.size(),
			&pResultUserInfo );
#else
	//XXX: potentially trouble place under Mac OS: non thread safe code
	pResultUserInfo = getpwnam( userAuthHelper.getUserName().toUtf8().constData() );
#endif

	if ( !pResultUserInfo )
		return 0;

	// Prepare Argv parameters
	QString wrapper = ParallelsDirs::getVmStarterPath();

	QStringList wrapperArgs;
	wrapperArgs << UTF8_2QSTR( pResultUserInfo->pw_name );
	wrapperArgs << QString("%1").arg( pResultUserInfo->pw_gid);
	wrapperArgs << QString("%1").arg( pResultUserInfo->pw_uid);
	wrapperArgs << QString("%1").arg( (int)binaryMode );
 	wrapperArgs << workingDirectory;

	// CVzHelper::create_ctx() args:
	bool bCreateVzCtx = CDspService::isServerModePSBM() && pVmConfig;
	wrapperArgs << ( (!bCreateVzCtx)?"": pVmConfig->getVmIdentification()->getVmUuid() );
	bool bOvercommit =
		CDspService::instance()->getDispConfigGuard().getDispCommonPrefs()->getMemoryPreferences()->isOvercommit();
	wrapperArgs << ( (!bCreateVzCtx)?"": CVzHelper::get_cpu_mask(pVmConfig, bOvercommit) );
	wrapperArgs << ( (!bCreateVzCtx)?"": QString("%1").arg( pVmConfig->getVmHardwareList()->getCpu()->getNumber() ) );
	wrapperArgs << QString("%1").arg(CDspService::instance()->getDispConfigGuard().getDispConfig()->
			getDispatcherSettings()->getCommonPreferences()->
			getWorkspacePreferences()->getVmGuestCpuLimitType());

	wrapperArgs << program;
	wrapperArgs << arguments;
	char **argv = makeArgs(wrapper, wrapperArgs);
	if (argv == NULL)
		return 0;

	// Prepare Envp parameters
	// set env HOME for user.
	QString contId;
	if( contId.isEmpty() )
		environment << QString("HOME=") + QString(pResultUserInfo->pw_dir);
	else
	{
		environment << QString("HOME=") + qgetenv("HOME");

		environment << QString("APP_SANDBOX_CONTAINER_ID=") + contId;

		// #PDFM-38277 CFFIXED_USER_HOME is necessary to properly work NSHomeDirectory() call
		environment << QString("CFFIXED_USER_HOME=") + qgetenv("CFFIXED_USER_HOME");
	}
	// if LD_LIBRARY_PATH exists in the current environment, but
	// not in the environment list passed by the programmer, then
	// copy it over.
#if defined(Q_OS_MAC)
	static const char libraryPath[] = "DYLD_LIBRARY_PATH";
#else
	static const char libraryPath[] = "LD_LIBRARY_PATH";
#endif
	QStringList matches = environment.filter(QRegExp("^" + QByteArray(libraryPath) + "="));
	char *envLibraryPath = ::getenv(libraryPath);
	if (matches.isEmpty() && envLibraryPath != 0)
	{
		QString entry = libraryPath;
		entry += "=";
		entry += envLibraryPath;
		environment << QString(libraryPath) +  "=" + QString(envLibraryPath);
	}

	char **envp = makeArgs(QString(), environment);
	if (envp == NULL) {
		freeArgs(argv);
		return 0;
	}

    WRITE_TRACE( DBG_DEBUG, "Parallels VM Starter: " );
    for( int i=0; argv[i]; i++ )
        WRITE_TRACE( DBG_DEBUG, "argv[%d]='%s'", i, argv[i] );
    for( int i=0; envp[i]; i++ )
        WRITE_TRACE( DBG_DEBUG, "envp[%d]='%s'", i, envp[i] );

	Q_PID processPid = ::vfork();

	if ( processPid == 0 )
	{
		// DEVEL NOTES:
		// 1. All logic between vfork() and exec() should be eliminated !
		// 2. You can add all what you need to wrapper ( prl_vm_starter )
		// LOOK AT #PDFM-26379, #PSBM-6022 or #PSBM-5054 to get more information
		// #PSBM-5338
		// for release dispatcher stdin/stdout/stderror are alredy set to /dev/null
		// vm_app just inherits them
		for(int fd = getdtablesize(); fd >2; --fd)
		{
			// #PSBM-5054, #PSBM-5834
			int i = fcntl(fd, F_GETFD);
			if (i >= 0)
				fcntl(fd, F_SETFD, i | FD_CLOEXEC);
		}

		::execve(argv[0], argv, envp);

		//TODO: notify parent about error
		WRITE_TRACE( DBG_FATAL,
				"Failed to execve() - Parallels VM Starter fails to start - No error reported! errno = %d", errno );
		::_exit(-1);
	}
    else if ( processPid == -1 )
    {
        WRITE_TRACE( DBG_FATAL,
                    "Failed to vfork() - Parallels VM Starter fails to start - errno = %d", errno );
        // Reset pid to tell error to caller
        processPid = 0;
    }

	freeArgs(argv);
	freeArgs(envp);

	return processPid;

#endif // _WIN_
}

/**
 * Subsidiary functions
 *
 * Method is based on the Qt method that was not unfortunately exported
 * (qt/src/corelib/io/qprocess_win.cpp)
 * QT Commercial license
 * http://trolltech.com/products/qt/licenses/licensing
 */

QByteArray create_environment ( const QStringList& environment )
{
    QByteArray envlist;
    if (!environment.isEmpty()) {
        QStringList envStrings = environment;
        int pos = 0;
        // add PATH if necessary (for DLL loading)
        if (envStrings.filter(QRegExp("^PATH=",
                                      Qt::CaseInsensitive)).isEmpty()) {
            QByteArray path = qgetenv("PATH");
            if (!path.isEmpty())
                envStrings.prepend(QString(QLatin1String("PATH=%1")).
                                     arg(QString::fromLocal8Bit(path)));
        }
        // add systemroot if needed
        if (envStrings.filter(QRegExp("^SystemRoot=",
                                      Qt::CaseInsensitive)).isEmpty()) {
            QByteArray systemRoot = qgetenv("SystemRoot");
            if (!systemRoot.isEmpty())
                envStrings.prepend(QString(QLatin1String("SystemRoot=%1")).
                                     arg(QString::fromLocal8Bit(systemRoot)));
        }
#ifdef UNICODE
        if (!(QSysInfo::WindowsVersion & QSysInfo::WV_DOS_based)) {
            for (QStringList::ConstIterator it = envStrings.constBegin();
                 it != envStrings.constEnd(); it++ ) {
                QString tmp = *it;
                uint tmpSize = sizeof(TCHAR) * (tmp.length()+1);
                envlist.resize(envlist.size() + tmpSize);
                memcpy(envlist.data()+pos, tmp.utf16(), tmpSize);
                pos += tmpSize;
            }
            // add the 2 terminating 0 (actually 4,
                // just to be on the safe side)
            envlist.resize( envlist.size()+4 );
            envlist[pos++] = 0;
            envlist[pos++] = 0;
            envlist[pos++] = 0;
            envlist[pos++] = 0;
        } else
#endif // UNICODE
        {
            for (QStringList::ConstIterator it = envStrings.constBegin();
                 it != envStrings.constEnd(); it++) {
                QByteArray tmp = (*it).toLocal8Bit();
                uint tmpSize = tmp.length() + 1;
                envlist.resize(envlist.size() + tmpSize);
                memcpy(envlist.data()+pos, tmp.data(), tmpSize);
                pos += tmpSize;
            }
            // add the terminating 0 (actually 2, just to be on the safe side)
            envlist.resize(envlist.size()+2);
            envlist[pos++] = 0;
            envlist[pos++] = 0;
        }
    }
    return envlist;
}


/**
 * Subsidiary functions
 *
 * Method is based on the Qt method that was not unfortunately exported
 * (qt/src/corelib/io/qprocess_win.cpp)
 * QT Commercial license
 * http://trolltech.com/products/qt/licenses/licensing
 */
QString create_commandline ( const QString& program,
                             const QStringList& arguments )
{
    QString programName = program;
    if (!programName.startsWith(QLatin1Char('\"')) &&
        !programName.endsWith(QLatin1Char('\"')) && programName.contains(" "))
        programName = "\"" + programName + "\"";
    programName.replace("/", "\\");

    QString args;
    // add the prgram as the first arrg ... it works better
    args = programName + " ";
    for (int i=0; i<arguments.size(); ++i) {
        QString tmp = arguments.at(i);
        // in the case of \" already being in the string
        // the \ must also be escaped
        tmp.replace( "\\\"", "\\\\\"" );
        // escape a single " because the arguments will be parsed
        tmp.replace( "\"", "\\\"" );
        if (tmp.isEmpty() || tmp.contains(' ') || tmp.contains('\t')) {
            // The argument must not end with a \ since this would be
            // interpreted as escaping the quote -- rather put the \ behind
            // the quote: e.g. rather use "foo"\ than "foo\"
            QString endQuote("\"");
            int i = tmp.length();
            while (i>0 && tmp.at(i-1) == '\\') {
                --i;
                endQuote += "\\";
            }
            args += QString(" \"") + tmp.left(i) + endQuote;
        } else {
            args += ' ' + tmp;
        }
    }
    return args;
}

bool checkWhetherProcessAlive(quint32 nProcId)
{
#ifdef _WIN_
	QByteArray _buffer(BUFSIZ * sizeof(DWORD) , 0);
	DWORD cbNeeded = _buffer.size();
	do
	{
		if (!EnumProcesses((DWORD *)_buffer.data(), _buffer.size(), &cbNeeded))
		{
			WRITE_TRACE(DBG_FATAL, "Error occured on try to enumerate processes: %d", GetLastError());
			return (false);
		}
		else if (cbNeeded == _buffer.size())
		{
			LOG_MESSAGE( DBG_INFO, "EnumProcesses() need larger size than %d."
				"Multiple to 2 and trying again.", cbNeeded );

			cbNeeded = _buffer.size()*2;
			_buffer.resize( cbNeeded );
		}
	}while( cbNeeded == _buffer.size() );

	quint32 nProcessNumber = cbNeeded/sizeof(DWORD);
	DWORD *aProcesses = (DWORD *)_buffer.data();
	for (quint32 i = 0; i < nProcessNumber; ++i)
		if (nProcId == aProcesses[i])
			return (true);
#elif defined(_LIN_)
	if (QFile::exists(QString("/proc/%1/stat").arg(nProcId)))
		return (true);
#endif
	return false;
}

static void fillVmsWithAliveProcess(const char* args, unsigned int size,
									const QSet<CVmIdent>& setVms, QSet<CVmIdent>& setRunVms)
{
	QString qsArgs;
	QString qsVmUuid;
	QString qsVmDirUuid;

	for(unsigned int i = 0; i < size; i++)
	{
		if ( ! args[i] )
			qsArgs += ' ';
		else
		{
			qsArgs += args[i];
			if (args[i] != '{')
				continue;

			QString qsUuid = &args[i];
			if ( ! Uuid::isUuid(qsUuid) )
				continue;

			if (qsVmUuid.isEmpty())
				qsVmUuid = qsUuid;
			else
			{
				qsVmDirUuid = qsUuid;
				break;
			}
		}
	}

	if ( ! qsArgs.contains(VM_EXECUTABLE) )
		return;

	CVmIdent vmIdent = MakeVmIdent(qsVmUuid, qsVmDirUuid);
	if (setVms.contains(vmIdent))
		setRunVms.insert(vmIdent);
}

QSet<CVmIdent> getVmsWithAliveProcess(QSet<CVmIdent> setVms)
{
	QSet<CVmIdent> setRunVms;

#if defined(_LIN_)

	QDir proc("/proc");
	QStringList lstProcDirs = proc.entryList(QDir::AllDirs);

	foreach(QString qsProcDir, lstProcDirs)
	{
		QString qsFileName = "/proc/" + qsProcDir + "/cmdline";
		int fd = ::open(QSTR2UTF8(qsFileName), O_RDONLY);
		if ( fd == -1 )
			continue;

		QByteArray ba;
		char buf[128];
		ssize_t size = sizeof(buf);

		do
		{
			size = ::read(fd, buf, (size_t )size);
			if (size > 0)
				ba.append(buf, size);
		} while(size && size != -1);

		::close(fd);
		if (size == -1)
			continue;

		fillVmsWithAliveProcess(ba.constData(), (unsigned int )ba.size(), setVms, setRunVms);
	}

#elif defined(_WIN_)
	// TODO: implement process checking for Windows
#endif

	return setRunVms;
}

QStringList getUsersEnvironment(CAuthHelper& userAuthHelper)
{
	QStringList env;
#ifdef _WIN_
	HANDLE pToken = userAuthHelper.GetAuth()->GetTokenHandle();
	void *penv;
	if (!CreateEnvironmentBlock(&penv, pToken, FALSE)) {
		WRITE_TRACE(DBG_WARNING, "Couldn't create environment block. Error=%d", GetLastError());
		return env;
	}

	WCHAR *ws = (WCHAR *)penv;
	int len;
	while ((len = lstrlenW(ws)) != 0) {
		env.append(UTF16SZ_2QSTR(ws, len));
		ws += (len + 1);
	}

	DestroyEnvironmentBlock(penv);
#else
	Q_UNUSED(userAuthHelper);
#endif
	return env;
}
