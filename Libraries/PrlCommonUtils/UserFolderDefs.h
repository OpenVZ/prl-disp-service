///////////////////////////////////////////////////////////////////////////////
///
/// @file UserFoldersDefs.h
///
/// Enumerations used in user folders enumeration
///
/// @author vasilyz@
///
/// Copyright (c) 2005-2017, Parallels International GmbH
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
/// Our contact details: Parallels International GmbH, Vordergasse 59, 8200
/// Schaffhausen, Switzerland.
///
///////////////////////////////////////////////////////////////////////////////
#ifndef _USERFOLDERS_DEFS_DECLARED_
#define _USERFOLDERS_DEFS_DECLARED_

namespace UserFolder {
	/*
	 * User folders types. Those constants are used widely in host and guest.
	 * The new types must not change values of existing types or affect them
	 * anyhow. Changing may break compatibility!!!
	 */
	enum Type
	{
		DESKTOP = 1,
		MYDOCUMENTS = 2,
		MYPICTURES = 3,
		MYMUSIC = 4,
		MYVIDEOS = 5,
		DOWNLOADS = 6,
	};
};
#endif
