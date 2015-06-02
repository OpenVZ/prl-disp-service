//////////////////////////////////////////////////////////////////////////
///
/// @file pollset_private.h
///
/// @author sdmitry
///
/// Private data and structure of pollset.
/// Must not be included by anyone except pollset_xxx.cpp
///
/// Copyright (c) 2011-2015 Parallels IP Holdings GmbH
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
//////////////////////////////////////////////////////////////////////////
#ifndef prl_pollset_private_h__
#define prl_pollset_private_h__

// initial num of entries in prl_pollset
#define PRL_POLLSET_INITIAL_SIZE 8
#define PRL_POLLSET_SIZE_INCREMENT 16

// internal function. Scans timers and invoke callbacks.
// return interval in milliseconds of the next timeout
int __pollset_poll_timers(pollset_t *pollset);

// Internal function. Refreshes timestamp on pollset.
void __pollset_refresh_time(pollset_t *pollset);

#endif // prl_pollset_private_h__
