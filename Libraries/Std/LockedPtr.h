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

#ifndef H__LockedPtr__H
#define H__LockedPtr__H

#include <QMutex>
#include "SmartPtr.h"

template <class T>
class LockedPtrStorage
{
public:
	LockedPtrStorage ( QMutex* mutex, T* pointee ) :
		m_mutex(mutex),
		m_pointee(pointee),
		m_locksNum(0)
	{
		lock();
	}

	~LockedPtrStorage ()
	{
		unlock();
	}

	void lock ()
	{
		m_mutex->lock();
		m_locksNum = 1;
	}

	void unlock ()
	{
		volatile quint32 locksNum = m_locksNum;

		if ( locksNum ) {
			m_locksNum = 0;
			m_mutex->unlock();
		}
	}

	T* getPtr ()
	{
		return m_pointee;
	}

	const T* getPtr () const
	{
		return m_pointee;
	}

private:
	QMutex* m_mutex;
	T* m_pointee;
	volatile quint32 m_locksNum;
};

template <class T>
class LockedPtr
{
public:
	LockedPtr ( QMutex* mutex, T* pointer ) :
		m_smartStorage( new LockedPtrStorage<T>(mutex, pointer) )
	{}

	LockedPtr ( const LockedPtr<T>& p ) :
		m_smartStorage( p.m_smartStorage )
	{}

	LockedPtr<T>& operator= ( const LockedPtr<T>& other )
	{
		m_smartStorage = other.m_smartStorage;
		return *this;
	}

	~LockedPtr()
	{}

	void lock ()
	{
		m_smartStorage->lock();
	}

	void unlock()
	{
		m_smartStorage->unlock();
	}
	T* getPtr ()
	{
		return m_smartStorage->getPtr();
	}

	const T* getPtr () const
	{
		return m_smartStorage->getPtr();
	}

	T* operator->()
	{
		return m_smartStorage->getPtr();
	}

	const T& operator*() const
	{
		return *getPtr();
	}

	T& operator*()
	{
		return *getPtr();
	}

	operator bool () const
	{
		return isValid();
	}

	bool isValid () const
	{
		return m_smartStorage->getPtr() != 0;
	}

protected:
	LockedPtr( SmartPtr< LockedPtrStorage<T> >& storage ):
		m_smartStorage( storage )
	{

	}

protected:
	SmartPtr< LockedPtrStorage<T> > m_smartStorage;
};

#endif // H__LockedPtr__H
