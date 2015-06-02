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

#ifndef __LAZY_BB_H__
#define __LAZY_BB_H__

#include <Interfaces/ParallelsTypes.h>
#include <Interfaces/ParallelsAlloc.h>

#include <Libraries/HostUtils/HostUtils.h>
#include "Libraries/Std/PrlTime.h"

#define LAZY_BB_TIME_TO_DIE	5 /* time in seconds before bounce buffer dies without access */

class CLazyBB
{
public:
	CLazyBB() {
		m_ptr = NULL;
		m_size = 0;
	};
	~CLazyBB() {
		if (!m_ptr)
			return;
		prl_vfree(m_ptr);
		m_ptr = NULL;
	};
	void *bb_alloc(PRL_INT64 size) {
		m_last_access = PrlGetTimeMonotonic();
		if (m_ptr) {
			if (size <= m_size)
				return m_ptr;
			prl_vfree(m_ptr);
		}
		m_size = size;
		m_ptr = prl_valloc(size);
		return m_ptr;
	};
	void bb_free() {
		if (!m_ptr)
			return;
		if ((PrlGetTimeMonotonic() - m_last_access) < (LAZY_BB_TIME_TO_DIE * UL64(1000000)))
			return;
		prl_vfree(m_ptr);
		m_ptr = NULL;
	};
private:
	UINT64 m_last_access;	// last access timestamp
	void *m_ptr;		// currently allocated bounce buffer
	PRL_INT64 m_size;	// size of allocated bounce buffer
};

#endif /* __LAZY_BB_H__ */
