///////////////////////////////////////////////////////////////////////////////
///
/// @file ProcessWatcher.cpp
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

#if !defined(_WIN_)

#include "ProcessWatcher.h"
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>

//
// This file is intended to use in Unix-environment only
//

namespace Prl
{

/// Initializes watcher for the finished processes
void InitEndProcessWatcher()
{
}

/// Synchronously executes specified process and returns its finish status.
int ExecuteProcess(const char *command)
{
	return system(command);
}

} // namespace

#endif // !_WIN_
