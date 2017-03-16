//////////////////////////////////////////////////////////////////////////
///
/// @file PerfCountersOut.cpp
///
/// @brief Performance counters output class helper implementation
///
/// @author maximk
///
/// Copyright (c) 2008-2017, Parallels International GmbH
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

#include <stdio.h>
#include <time.h>
#include <algorithm>

#ifdef _WIN_
	#define snprintf _snprintf
#else
#include <sys/time.h>
#endif

#include "PerfCountersOut.h"

/**
 * Number to the string conversion helper
 * Instead of '123456789' it prints '123,456,789'
 */
static string num2str(T_COUNTER val)
{
	std::string s = "";
	bool bfirst = true;

#define PRE_(i) \
	char s ## i[16]; \
	unsigned t ## i = (unsigned) (val % 1000); \
	val /= 1000; \
	snprintf( s ## i, sizeof(s ## i), val ? "%.3u" : "%u", t ## i );

	PRE_(0); PRE_(1); PRE_(2); PRE_(3); PRE_(4);

#define POST_(i) \
	if (t ## i) { \
		if (!bfirst) \
			s += "."; \
		s += s ## i; \
		bfirst = false; \
	}

	POST_(4); POST_(3); POST_(2); POST_(1); POST_(0);

	if (bfirst)
		s = "0";

	return s;
}

PerfCountersOut::PerfCountersOut()
: m_std_output(true), m_priv_data(0), m_call_dump_count(0), m_host(),
m_prev_dump_msec(0)
{
}

PerfCountersOut::~PerfCountersOut()
{
    Free();
}

bool PerfCountersOut::Free()
{
	m_output.clear();

    size_t size = m_storages.size();
    for (size_t i = 0; i < size; i++)
    {
        PerfStorage& storage = m_storages[i];
        perf_release_storage(&storage.storage, &m_lock);
    }
    m_storages.clear();
    return true;
}

bool PerfCountersOut::Sort()
{
    size_t size = m_storages.size();
    for (size_t i = 0; i < size; i++)
    {
        PerfStorage& storage = m_storages[i];
        std::sort( storage.counters.begin(), storage.counters.end(), SortCounters() );
    }

	return true;
}

bool PerfCountersOut::Fill(bool std_output)
{
	m_std_output = std_output;
    Free(); // when they fill us - first we need to cleanup
    perf_enum_storages(FillStorages, (void*)this, &m_lock );

	m_host.Fill();
	FillStorages(&m_host.m_desc, (void*)this );

	Sort();
    return true;
}

void PerfCountersOut::Dump(	string storage_mask,
							string counter_mask,
							bool delta,
							bool nozero,
							bool digits)
{
	m_host.Dump();

	++m_call_dump_count;

	char buf[128];
#define OUTPUT(print_func, args) do {	\
	print_func args;					\
	if (m_std_output) printf( "%s", buf );	\
	else m_output += buf;				\
} while (0)

	time_t t;
	tm* lt;
	UINT64 msec;

    bool use_counter_mask = (counter_mask != "");
    bool use_storage_mask = (storage_mask != "");

	t = time(NULL);
	lt = localtime( &t );

#ifdef _WIN_
	FILETIME tv;
	GetSystemTimeAsFileTime(&tv);
	msec = (((UINT64)tv.dwHighDateTime << 32) | tv.dwLowDateTime) / 10000;
#else
	struct timeval tv;
	gettimeofday(&tv, NULL);
	msec = (UINT64)tv.tv_sec * 1000 + tv.tv_usec / 1000;
#endif
	if (!m_prev_dump_msec)
		m_prev_dump_msec = msec;

    size_t size = m_storages.size();
	if (size == 0)
		return;

	SortCounters();

    for (size_t i = 0; i < size; i++)
    {
        PerfStorage& storage = m_storages[i];

        if (!storage.storage.storage || !perf_storage_alive(&storage.storage, &m_lock))
        {
            // release if not yet released
            storage.storage.storage && perf_release_storage(&storage.storage, &m_lock) ;
            continue ;
        }

        if ( use_storage_mask )
        {
            if ( string(storage.storage.storage->descriptor.name).find(storage_mask) == string::npos )
                continue;
        }

		bool bPrintHead = true;

        size_t size2 = storage.counters.size();
		if (size2 == 0)
		{
			OUTPUT(sprintf, (buf, "No performance counters found for storage '%s'\n",
				storage.storage.storage->descriptor.name ) );
		}

        for (size_t j = 0; j < size2; j++)
        {
			T_COUNTER val;
            PerfCounter& c = storage.counters[j];

			char* name = c.counter->name;
			int meta_idx = GetNameIdx(name);

            if (use_counter_mask)
                if ( string(c.counter->name).find(counter_mask) == string::npos )
                    continue;

            if (delta && strncmp(c.counter->name, PERF_COUNT_TYPE_INC,
						sizeof(PERF_COUNT_TYPE_INC) - 1) == 0 )
            {
				val = PERF_COUNT_GET(c.counter)- c.last_value;
                c.last_value = PERF_COUNT_GET(c.counter);
            }
            else
            {
				val = PERF_COUNT_GET(c.counter);
            }

			if (nozero && !val)
				continue;

			if ( bPrintHead )
			{
				bPrintHead = false;

				OUTPUT(sprintf, (buf, "%s - %.2u:%.2u:%.2u",
					storage.storage.storage->descriptor.name,
					lt->tm_hour, lt->tm_min, lt->tm_sec ) );

				if (m_prev_dump_msec == msec)
					OUTPUT(sprintf, (buf, "\n" ) );
				else
					OUTPUT(sprintf, (buf, "  +%lld.%02lld sec\n",
						(msec - m_prev_dump_msec) / 1000,
						(msec - m_prev_dump_msec) % 1000));
			}

			name = &name[meta_idx];

			if (digits)
			{
				OUTPUT(sprintf, (buf, "\t%-48s %16s\n", name, num2str(val).c_str() ) );
			}
			else
			{
				OUTPUT(sprintf, (buf, "\t%-48s %lld\n", name, val ) );
			}
        }
    }
	m_prev_dump_msec = msec; 
}

string PerfCountersOut::GetOutputString() const
{
	return m_output;
}

int PerfCountersOut::GetCallDumpCount() const
{
	return m_call_dump_count;
}

void PerfCountersOut::SetPrivateData(int pd)
{
	m_priv_data = pd;
}

int PerfCountersOut::GetPrivateData() const
{
	return m_priv_data;
}

void PerfCountersOut::PrintCustomTextInOutput(string text)
{
	if (m_std_output)
		printf( "%s", text.c_str() );
	else
		m_output += text;
}

int PerfCountersOut::FillCounters(counters_storage_t *, counter_t *counter, void *data)
{
    PerfStorage* pStorage = (PerfStorage*) data;

    PerfCounter c;
    c.counter = counter;
    c.last_value = PERF_COUNT_GET(counter);
    pStorage->counters.push_back( c );

    return ENUM_CONTINUE ;
}

int PerfCountersOut::FillStorages(const storage_descriptor_t *sd, void *data)
{
    PerfCountersOut* pCounter = (PerfCountersOut*) data;

    PerfStorage storage;
    storage.storage = *sd; // copying descriptor
    perf_enum_counters(sd->storage, FillCounters, (void*)&storage);

    pCounter->m_storages.push_back( storage );

    return ENUM_CONTINUE | ENUM_DONOT_RELEASE_STORAGE; // not releasing storage
}

int PerfCountersOut::GetNameIdx(const char* name)
{
	int len = (int)strlen(name);
	int meta_idx = 0;

	for ( int c = 0; c < len; c++ )
		if ( name[c] == '@' )
			meta_idx = c + 1;

	return meta_idx;
}
