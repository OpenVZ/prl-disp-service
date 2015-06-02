//////////////////////////////////////////////////////////////////////////
///
/// @file std_time.h
///
/// @brief
/// Time specific stuff.
///
/// Copyright (c) 2012-2015 Parallels IP Holdings GmbH
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

#ifndef __STD_TIME_H__
#define __STD_TIME_H__

#define GUEST_TIME_MAX ((PRL_INT64)~((PRL_UINT64)1 << 63))

#define TIME_AFTER(a,b) \
	((PRL_INT64)(a) - (PRL_INT64)(b) > 0)
#define TIME_BEFORE(a,b) TIME_AFTER(b,a)

#define TIME_AFTER_EQ(a,b) \
	((PRL_INT64)(a) - (PRL_INT64)(b) >= 0)
#define TIME_BEFORE_EQ(a,b) TIME_AFTER_EQ(b,a)

#endif // __STD_TIME_H__
