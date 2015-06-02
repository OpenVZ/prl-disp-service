///////////////////////////////////////////////////////////////////////////////
///
/// @file stdc++.cpp
///
/// Module to provide binary compatibilty with MacOS X 10.5 for code built
/// with SDK 10.6
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

#include <ostream>

namespace std {
	// Instantiating templates of some functions in order to get rid of
	// dependency on  symbols which are not provided by libstdc++ on
	// MacOS X 10.5.
	template ostream& __ostream_insert(ostream&, const char*, streamsize);
	template ostream& ostream::_M_insert(unsigned long);
	template ostream& ostream::_M_insert(unsigned long long);
}

