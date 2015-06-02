//////////////////////////////////////////////////////////////////////////
///
/// Simple reference counting mech.
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
/////////////////////////////////////////////////////////////////////////
#pragma once

#include <Libraries/Std/AtomicOps.h>

/*
 * Smart pointers is good enough, but sometimes the very simple reference
 * counting mechanic needed. So, i decided to realize it here.
 */
class RefCounter
{
private:
	unsigned int m__reference_counter;
protected:
	/*
	 * Must be initialized to 0 in case AtomicDec returns
	 * previous value, so after last release counter will be
	 * MAXULONG and delete will called
	 */
	RefCounter() : m__reference_counter(0) { };
	// No any direct destructor calls allowed!
	virtual ~RefCounter() {};
	/*
	 * Tell should we kill ourselves or not, used when
	 * overloading Release()
	 */
	virtual unsigned long __decrement_counter() {
		return AtomicDec(&m__reference_counter);
	}
public:
	virtual RefCounter* addRef() {
		AtomicInc(&m__reference_counter);
		return this;
	}

	virtual void release() {
		if (__decrement_counter())
			return;
		delete this;
	}
};
