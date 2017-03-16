//////////////////////////////////////////////////////////////////////////
///
/// @file PerfCountersHost.h
///
/// @brief host performance statistics container class
///
/// @author Alexander Andreev (aandreev@)
///
/// Copyright (c) 2010-2017, Parallels International GmbH
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
//////////////////////////////////////////////////////////////////////////

#ifndef __PREF_COUNTER_HOST_H__
#define __PREF_COUNTER_HOST_H__

#include <prlcommon/Interfaces/ParallelsTypes.h>
#include <stdio.h>

enum {
	PERFC_HOST_DISK_READ_REQ = 0,
	PERFC_HOST_DISK_READ_BYTES,
	PERFC_HOST_DISK_WRITE_REQ,
	PERFC_HOST_DISK_WRITE_BYTES,
	PERFC_HOST_CPU_USER,
	PERFC_HOST_CPU_NICE,
	PERFC_HOST_CPU_SYST,
	PERFC_HOST_CPU_IDLE,
	PERFC_HOST_CPU_WAIT,
	PERFC_HOST_CPU_INTER,
	PERFC_HOST_MEM_FREE,
	PERFC_HOST_MEM_SWAP,
	PERFC_HOST_MEM_ACTIVE,
	PERFC_HOST_MEM_INACTIVE,
	PERFC_HOST_MEM_WIRED,
	PERFC_HOST_MEM_DIRTY,
	PERFC_HOST_COUNTER_LAST,
};

#ifdef _WIN_
#define snprintf _snprintf
#endif

#define HOST_ADD_COUNTER(IDX, NAME)	\
	snprintf(m_storage.counters[IDX].name, PERF_COUNT_NAME_LENGTH - 1, \
		PERF_COUNT_TYPE_INC NAME)

class PerfCounterHost {
public:
	storage_descriptor_t	m_desc;
	counters_storage_t		m_storage;

	PerfCounterHost() {
		memset(&m_storage, 0, sizeof(m_storage));
		m_storage.local = 1;
		m_desc.storage = &m_storage;
		sprintf(m_storage.descriptor.name, "host");
		PERF_COUNT_SET(&m_storage.descriptor, PERFC_HOST_COUNTER_LAST);
		HOST_ADD_COUNTER(PERFC_HOST_DISK_READ_REQ,		"disk.read.req");
		HOST_ADD_COUNTER(PERFC_HOST_DISK_READ_BYTES,	"disk.read.bytes");
		HOST_ADD_COUNTER(PERFC_HOST_DISK_WRITE_REQ,		"disk.write.req");
		HOST_ADD_COUNTER(PERFC_HOST_DISK_WRITE_BYTES,	"disk.write.bytes");
		HOST_ADD_COUNTER(PERFC_HOST_CPU_USER,			"cpu.user");
		HOST_ADD_COUNTER(PERFC_HOST_CPU_NICE,			"cpu.nice");
		HOST_ADD_COUNTER(PERFC_HOST_CPU_SYST,			"cpu.system");
		HOST_ADD_COUNTER(PERFC_HOST_CPU_IDLE,			"cpu.idle");
		HOST_ADD_COUNTER(PERFC_HOST_CPU_WAIT,			"cpu.wait");
		HOST_ADD_COUNTER(PERFC_HOST_CPU_INTER,			"cpu.inter");
		HOST_ADD_COUNTER(PERFC_HOST_MEM_FREE,			"mem.free");
		HOST_ADD_COUNTER(PERFC_HOST_MEM_SWAP,			"mem.swap");
		HOST_ADD_COUNTER(PERFC_HOST_MEM_ACTIVE,			"mem.active");
		HOST_ADD_COUNTER(PERFC_HOST_MEM_INACTIVE,		"mem.inactive");
		HOST_ADD_COUNTER(PERFC_HOST_MEM_WIRED,			"mem.wired");
		HOST_ADD_COUNTER(PERFC_HOST_MEM_DIRTY,			"mem.dirty");
		m_priv = NULL;
	}

	~PerfCounterHost() {
		Deinit();
	}

	/* Called in PerfCountersOut::Fill */
	void Fill();

	/* Called in PerfCountersOut::Dump */
	void Dump();
private:
	void Init();
	void Deinit();
	void *m_priv;
};
#endif //__PREF_COUNTER_H__
