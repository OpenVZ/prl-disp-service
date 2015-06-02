/*
 * Copyright (c) 2015 Parallels IP Holdings GmbH
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
 * Our contact details: Parallels IP Holdings GmbH, Vordergasse 59, 8200
 * Schaffhausen, Switzerland.
 */


#include <QFile>
#include <QStringList>
#include <QCoreApplication>

#ifdef _MAC_
  #include <mach/mach.h>
  #include <crt_externs.h>
  static char** environ = *_NSGetEnviron();
#elif _WIN_
  #include <windows.h>
#elif _LIN_
  #include <unistd.h>
#endif

#include <stdlib.h>

#include "IOProtocol.h"
#include "Libraries/Logging/Logging.h"

using namespace IOService;

/*****************************************************************************/

int createProcess ( const QString& program,
                    const QStringList& args,
                    IOCommunication::SocketHandle sock )

{
#ifdef _WIN_
    // Windows

    STARTUPINFOA si = {0};
    si.cb = sizeof(si);
    si.dwFlags |= STARTF_USESTDHANDLES;

    PROCESS_INFORMATION pi = {0};

    QStringList arguments = args;
    arguments.prepend( QCoreApplication::applicationFilePath() );

    HANDLE pipeInWr = 0;

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
        }

        // Ensure the write handle to the pipe for STDIN is not inherited.
        if ( ! ::SetHandleInformation(pipeInWr, HANDLE_FLAG_INHERIT, 0) ) {
            WRITE_TRACE(DBG_FATAL, "Can't SetHandleInformation err='%d'!",
                        ::GetLastError());
        }

        // Set handle
        si.hStdInput = pipeInRd;
    }

    BOOL res = ::CreateProcessA( program.toAscii().data(),
                                 arguments.join(" ").toAscii().data(),
                                 0, 0, TRUE, 0, 0, 0, &si, &pi);

    if ( res && sock.isValid() ) {
        IOCommunication::SocketDuplicatedState dupState;
        if ( ! sock->duplicateSocket((int)pi.dwProcessId, dupState) ) {
            WRITE_TRACE(DBG_FATAL, "Can't duplicate socket err='%d'!",
                        ::WSAGetLastError());
        }
        quint32 stateSz = 0;
        void* stateBuff = dupState.getStateBuffer(stateSz);

        DWORD dwWritten = 0;
        DWORD bSuccess = ::WriteFile(pipeInWr, stateBuff, stateSz,
                                     &dwWritten, NULL);
        if ( ! bSuccess || dwWritten != stateSz ) {
            WRITE_TRACE(DBG_FATAL, "Can't write to duplicated socket to pipe!");
        }
        ::CloseHandle(pipeInWr);
    }

    if ( ! res )
        return 0;

    return pi.dwProcessId;

#else
    // Unix
    (void)sock;

    QStringList environment;

    int processPid = ::fork();

    if ( processPid == 0 ) {
        char **argv = reinterpret_cast<char**>(::calloc(args.count() + 2,
                                                        sizeof(char*)));
        argv[args.count() + 1] = 0;

        // add the program name
        argv[0] = reinterpret_cast<char*>(::malloc(program.size()+1));
        ::memcpy( argv[0], program.toAscii().constData(), program.size() );
        argv[0][program.size()] = 0;

        // add every argument to the list
        for ( int i = 0; i < args.count(); ++i ) {
            QString arg = args.at(i);
            argv[i + 1] = reinterpret_cast<char*>(::malloc(arg.size()+1));
            ::memcpy( argv[i + 1], arg.toAscii().constData(), arg.size() );
            argv[i + 1][arg.size()] = 0;
        }

        // add env
        if ( environment.isEmpty() ) {
            // Use environment list for temp storage
            for ( int i = 0; ::environ[i]; ++i )
                environment.append( QString(environ[i]) );

            ::execvp(argv[0], argv);
        }
        else {
#ifdef _MAC_
            static const char libraryPath[] = "DYLD_LIBRARY_PATH";
#else
            static const char libraryPath[] = "LD_LIBRARY_PATH";
#endif
            QStringList matches = environment.filter(
                                 QRegExp("^" + QByteArray(libraryPath) + "="));
            char* envLibraryPath = ::getenv( libraryPath );
            if ( matches.isEmpty() && envLibraryPath != 0 ) {
                QString entry = libraryPath;
                entry += "=";
                entry += envLibraryPath;
                environment << QString(libraryPath) +  "=" +
                               QString(envLibraryPath);
            }

            char **envp = new char *[environment.count() + 1];
            envp[environment.count()] = 0;

            for ( int j = 0; j < environment.count(); ++j ) {
                QString item = environment.at(j);
                envp[j] = reinterpret_cast<char*>(::malloc(item.size()+1));
                ::memcpy( envp[j], item.toAscii().constData(), item.size() );
                envp[j][item.size()] = 0;
            }

            ::execve(argv[0], argv, envp);
        }
        ::_exit(-1);
    }

    if ( processPid < 0 )
        return 0;

    return processPid;
#endif
}

/*****************************************************************************/
