/*
 * Copyright (c) 2005-2017, Parallels International GmbH
 *
 * This file is part of Virtuozzo Core Libraries. Virtuozzo Core
 * Libraries is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <http://www.gnu.org/licenses/> or write to Free Software Foundation,
 * 51 Franklin Street, Fifth Floor Boston, MA 02110, USA.
 *
 * Our contact details: Parallels International GmbH, Vordergasse 59, 8200
 * Schaffhausen, Switzerland.
 */

#pragma once

#include <windows.h>
#include <prlcommon/Std/noncopyable.h>

class WinGDIObjectSelect: private noncopyable
{
public:
	WinGDIObjectSelect(const HDC dc, const HGDIOBJ obj):
		m_dc(dc),
		m_old(SelectObject(m_dc, obj)) {}
	~WinGDIObjectSelect() { SelectObject(m_dc, m_old); }
private:
	const HDC m_dc;
	const HGDIOBJ m_old;
};
