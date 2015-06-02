//////////////////////////////////////////////////////////////////////////
///
/// @file PerfCountersHost_mac.cpp
///
/// @brief collect Mac host CPU/disk/memory statistics
///
/// @author Alexander Andreev (aandreev@)
///
/// Copyright (c) 2010-2015 Parallels IP Holdings GmbH
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

#include "PerfCounter.h"
#include "PerfCounterHost.h"

#include <sys/sysctl.h>

#include <mach/mach_init.h>
#include <mach/mach_host.h>
#include <mach/processor_info.h>
#include <limits.h>

#define IOKIT 1
#include <device/device_types.h>
#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/storage/IOBlockStorageDriver.h>

struct perf_counter_host {
	int			initialized;
	host_t		host_self;
	mach_port_t	master_port;
	vm_size_t	pagesize_kb;
};

void PerfCounterHost::Fill()
{
	if (!m_priv)
		Init();
}

void PerfCounterHost::Init()
{
	struct perf_counter_host *p;
	vm_size_t pagesize;

	if (m_priv)
		return;

	p = (struct perf_counter_host *)malloc(sizeof(struct perf_counter_host ));
	if (!p)
		return;

	p->host_self = mach_host_self();
	if (p->host_self == MACH_PORT_NULL)
		goto err;

	if (IOMasterPort(bootstrap_port, &p->master_port))
		goto err;

	host_page_size(p->master_port, &pagesize);
	p->pagesize_kb = pagesize / 1024;

	m_priv = p;

	/* Save initial counters */
	Dump();
	return;

err:
	free(p);
	return;
}

void PerfCounterHost::Deinit()
{
	if (m_priv)
		free(m_priv);
	m_priv = NULL;
}

#define __PERF_COUNT_SET(NAME, VAL)	PERF_COUNT_SET(&m_storage.counters[NAME], VAL)
#define __PERF_COUNT_ADD(NAME, VAL)	PERF_COUNT_ADD(&m_storage.counters[NAME], VAL)

void PerfCounterHost::Dump()
{
	vm_statistics_data_t		vm_stat;
	processor_cpu_load_info_data_t	cpu_stat;
	mach_msg_type_number_t		msg_type_info_count;
	io_iterator_t			drive_list;
	struct perf_counter_host	*p;

	if (!m_priv)
		return;

	p = (struct perf_counter_host *)m_priv;
	if (!p)
		return;

	/*
	 * CPU usage statistics
	 */
	msg_type_info_count = HOST_CPU_LOAD_INFO_COUNT;
	if(host_statistics(p->host_self,
			HOST_CPU_LOAD_INFO,
			(host_info_t)&cpu_stat,
			&msg_type_info_count) == KERN_SUCCESS)
	{
		__PERF_COUNT_SET(PERFC_HOST_CPU_USER, cpu_stat.cpu_ticks[CPU_STATE_USER]);
		__PERF_COUNT_SET(PERFC_HOST_CPU_SYST, cpu_stat.cpu_ticks[CPU_STATE_SYSTEM]);
		__PERF_COUNT_SET(PERFC_HOST_CPU_IDLE, cpu_stat.cpu_ticks[CPU_STATE_IDLE]);

#ifdef CPU_STATE_NICE
		__PERF_COUNT_SET(PERFC_HOST_CPU_NICE, cpu_stat.cpu_ticks[CPU_STATE_NICE]);
#else
		__PERF_COUNT_SET(PERFC_HOST_CPU_NICE, 0);
#endif

#ifdef CPU_STATE_WAIT
		__PERF_COUNT_SET(PERFC_HOST_CPU_WAIT, cpu_stat.cpu_ticks[CPU_STATE_WAIT]);
#else
		__PERF_COUNT_SET(PERFC_HOST_CPU_WAIT, 0);
#endif
	}

	/*
	 * Memory usage statistics
	 */
	/* wired, active, inactive, free */
	msg_type_info_count = sizeof(vm_stat) / sizeof(natural_t);
	if (host_statistics(p->host_self,
			HOST_VM_INFO,
			(host_info_t)&vm_stat,
			&msg_type_info_count) == KERN_SUCCESS)
	{
		/* Swap usage */
		struct xsw_usage xsu;
		int mib[2] = { CTL_VM, VM_SWAPUSAGE };
		int miblen = 2;
		size_t len = sizeof(xsu);
		if (!sysctl(mib, miblen, &xsu, &len, NULL, 0))
			__PERF_COUNT_SET(PERFC_HOST_MEM_SWAP, xsu.xsu_used / 1024);

		__PERF_COUNT_SET(PERFC_HOST_MEM_FREE,		vm_stat.free_count * p->pagesize_kb);
		__PERF_COUNT_SET(PERFC_HOST_MEM_ACTIVE, 	vm_stat.active_count * p->pagesize_kb);
		__PERF_COUNT_SET(PERFC_HOST_MEM_INACTIVE,	vm_stat.inactive_count * p->pagesize_kb);
		/* 
		 * FIXME
		 * actually:
		 *	wired = wire_count + stolen,
		 * where:
		 * 	stolen = machdep.memmap.Reserved + Unusable + Other
		 */
		__PERF_COUNT_SET(PERFC_HOST_MEM_WIRED,		vm_stat.wire_count * p->pagesize_kb);
	}

	/*
	 * Disk usage statistics
	 */

	/* Get the list of all drive objects. */
	if (!IOServiceGetMatchingServices(p->master_port,
		    IOServiceMatching("IOBlockStorageDriver"), &drive_list))
	{
		io_registry_entry_t	drive;
		CFNumberRef		number;
		CFDictionaryRef		properties, statistics;
		UInt64			value;

		/* Reset all counters */
		__PERF_COUNT_SET(PERFC_HOST_DISK_READ_REQ, 0);
		__PERF_COUNT_SET(PERFC_HOST_DISK_READ_BYTES, 0);
		__PERF_COUNT_SET(PERFC_HOST_DISK_WRITE_REQ, 0);
		__PERF_COUNT_SET(PERFC_HOST_DISK_WRITE_BYTES, 0);

		/* Calculate sum of counters from all drives */
		while ((drive = IOIteratorNext(drive_list)) != 0) {
			number = 0;
			properties = 0;
			statistics = 0;
			value = 0;

			/* Obtain the properties for this drive object. */
			if (IORegistryEntryCreateCFProperties(drive,
				    (CFMutableDictionaryRef *)&properties, kCFAllocatorDefault,
				    kNilOptions))
				break;

			if (properties != 0) {
				/* Obtain the statistics from the drive properties. */
				statistics
				    = (CFDictionaryRef)CFDictionaryGetValue(properties,
				    CFSTR(kIOBlockStorageDriverStatisticsKey));

#define READ_DISK_STAT(MACH_NAME, NAME)												\
	number = (CFNumberRef)CFDictionaryGetValue(statistics, CFSTR(MACH_NAME));	\
	if (number != 0) {															\
		CFNumberGetValue(number, kCFNumberSInt64Type, &value);					\
		__PERF_COUNT_ADD(NAME, value);											\
	}

				if (statistics != 0) {
					READ_DISK_STAT(kIOBlockStorageDriverStatisticsReadsKey,
							PERFC_HOST_DISK_READ_REQ);
					READ_DISK_STAT(kIOBlockStorageDriverStatisticsBytesReadKey,
							PERFC_HOST_DISK_READ_BYTES);
					READ_DISK_STAT(kIOBlockStorageDriverStatisticsWritesKey,
							PERFC_HOST_DISK_WRITE_REQ);
					READ_DISK_STAT(kIOBlockStorageDriverStatisticsBytesWrittenKey,
							PERFC_HOST_DISK_WRITE_BYTES);
				}
				CFRelease(properties);
			}
			IOObjectRelease(drive);
		}
		IOIteratorReset(drive_list);
		IOObjectRelease(drive_list);
	}
}
