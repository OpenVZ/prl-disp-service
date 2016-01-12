///////////////////////////////////////////////////////////////////////////////
///
/// @file CDspStarter.h
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

#ifndef __DSPSTARTER_H__
#define __DSPSTARTER_H__

#include <QString>
#include <QStringList>
#include <QProcess>

#include "CDspClient.h"
#include <prlcommon/Std/SmartPtr.h>
#include <prlxmlmodel/VmConfig/CVmConfiguration.h>
#include <prlcommon/Interfaces/ParallelsNamespace.h>

/**
 * Terminates proceess by pid. OS-independent variant
 */
void terminateProcessByPid ( Q_PID pid );

/**
 * Create process under specified user
 * @param starting VM process command (basically absolute path to VM process executable file)
 * @param arguments list that will be passed to the starting process
 * @param path to the working directory
 * @param environment variables list
 * @param pointer to the user session that starting process
 * @return
 */
Q_PID createProcess ( const QString& program,
                      const QStringList& arguments,
                      const QString& workingDirectory,
                      const QStringList& environment,
                      CAuthHelper& userAuthHelper,
		      const SmartPtr<CVmConfiguration> &pVmConfig = SmartPtr<CVmConfiguration>(),
                      const IOCommunication::SocketHandle& sock = IOCommunication::SocketHandle(),
                      const enum PVE::VmBinaryMode = PVE::VBMODE_DEFAULT);

/**
 * Regstries zombi watcher: method that prevents zombi processes appearing
 */
void registerZombiWatcher();

/**
 * Helper function. Creates C-string with process environment
 * @param list with environment variables
 * @return ready string with environment variables
 */
QByteArray create_environment ( const QStringList& environment );
/**
 * Helper function. Creates string with command line for starting process
 * @param path to executable binary file
 * @param list of arguments that will be passed to starting process
 * @return ready process start command line
 */
QString create_commandline ( const QString& program,
                             const QStringList& arguments );

/**
 * Helper function. Let to determine whether specified process alive
 * @param process identifier structure
 * @return sign whether process currently alive
 */
bool checkWhetherProcessAlive(quint32 procId);


/**
 * Helper function checks VM process alive state
 * @param VM list to check
 * @return VM list of VMs with alive process
 */
QSet<CVmIdent> getVmsWithAliveProcess(QSet<CVmIdent> setVms);


/**
 * Helper function. Obtains environment for a given user.
 * @param pointer to the user session
 * @return list of environment variables
 */
QStringList getUsersEnvironment(CAuthHelper& userAuthHelper);


#endif // __DSPSTARTER_H__
