///////////////////////////////////////////////////////////////////////////////
///
/// @file ProcessWatcher.h
/// @author sdmitry
///
/// Functions set to get rid of zombies and sync executing of processes
/// inside zombie-aware environment
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
#ifndef STD__ProcessWatcher_h__
#define STD__ProcessWatcher_h__


//
// This file is intended to use in Unix-environment only
//

#if !defined(_WIN_)

#include <unistd.h>

namespace Prl
{

/// Initializes watcher for the finished processes
void InitEndProcessWatcher();

/// Synchronously executes specified process and returns its finish status.
/// This is a wrapper against the 'system' call to fix signal disposition
/// for SIGCHLD before/after the call.
int ExecuteProcess( const char *command );

};

#endif // !_WIN_

#endif //STD__ProcessWatcher_h__
