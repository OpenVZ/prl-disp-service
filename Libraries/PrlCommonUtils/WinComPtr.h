/*
* Copyright (c) 2005-2015 Parallels IP Holdings GmbH
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
 * Our contact details: Parallels IP Holdings GmbH, Vordergasse 59, 8200
 * Schaffhausen, Switzerland.
*/

#pragma once

#include <Libraries/Std/noncopyable.h>

template <class T>
class WinComPtr
{
public:
	WinComPtr(): m_obj(0) {}
	WinComPtr(T *obj): m_obj(obj) {}
	WinComPtr(const WinComPtr &other) : m_obj(other.m_obj)
	{
		if (m_obj) m_obj->AddRef();
	}
	~WinComPtr()
	{
		reset(0);
	}
	void reset(T *obj)
	{
		if(0 != m_obj) m_obj->Release();
		m_obj = obj;
	}
	T** reset()
	{
		reset(0);
		return &m_obj;
	}
	T* ptr() { return m_obj; }
	T& operator *() const { return *m_obj; }
	T* operator ->() const { return m_obj; }
	WinComPtr &operator =(const WinComPtr &other)
	{
		m_obj = other.m_obj;
		if (m_obj) m_obj->AddRef();
		return *this;
	}

private:
	T *m_obj;
};
