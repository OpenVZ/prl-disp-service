/*
* Copyright (c) 2015 Parallels IP Holdings GmbH
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


template <typename T>
class ComPtr
{
	class _NoAddRefRelease: public T
	{
		_NoAddRefRelease();
	private:
		STDMETHOD_(ULONG, AddRef)()=0;
		STDMETHOD_(ULONG, Release)()=0;
	};

	template <typename Q>
	void _reset(const ComPtr<Q> &p)
	{
		m_obj = p.get();
		if (m_obj)
			m_obj->AddRef();
	}
public:
	ComPtr()
		: m_obj(0)
	{}

	explicit ComPtr(T *p)
		: m_obj(p)
	{}

	ComPtr(T *p, bool retain)
		: m_obj(p)
	{
		if (retain)
			m_obj->AddRef();
	}

	ComPtr(const ComPtr &p) {_reset(p);}

	template <typename Q>
	ComPtr(const ComPtr<Q> &p) {_reset(p);}

	~ComPtr()
	{
		if (m_obj)
			m_obj->Release();
	}

	const ComPtr &operator = (const ComPtr &copy)
	{
		ComPtr(copy).swap(*this);
		return *this;
	}

	template <typename Q>
	const ComPtr &operator = (const ComPtr<Q> &copy)
	{
		ComPtr(copy).swap(*this);
		return *this;
	}

	T **reset()
	{
		ComPtr().swap(*this);
		return &m_obj;
	}

	void reset(T *p) {ComPtr(p).swap(*this);}

	T *get() const {return m_obj;}
	_NoAddRefRelease *operator->() const {return static_cast<_NoAddRefRelease *>(m_obj);}

	operator bool() const {return get() != 0;}

	void swap(ComPtr<T> &right)
	{
		T *p = right.m_obj;
		right.m_obj = m_obj;
		m_obj = p;
	}

private:
	T *m_obj;
};
