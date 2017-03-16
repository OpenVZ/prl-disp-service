//////////////////////////////////////////////////////////////////////////
///
/// @file PerfCountersOut.h
///
/// @brief Performance counters output class helper
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

#include <vector>
#include <string>
#include "PerfCounter.h"
#include "PerfCounterHost.h"

using namespace std;


/**
 * Performance counters class - allows to efficiently
 * retrieve and dump values of the performance counters
 */
class PerfCountersOut
{
public:

	PerfCountersOut();
    ~PerfCountersOut();

    bool Free();
	bool Sort();
    bool Fill(bool std_output = true);

    /**
     * Dump contents of the performance counters storage to the console
     * @param counter names mask
     * @param storage names mask
     */
    void Dump(string storage_mask = "",
			  string counter_mask = "",
			  bool delta = false,
			  bool nozero = false,
			  bool digits = false);

	string GetOutputString() const;

	int GetCallDumpCount() const;

	// Private data is data for store some id. For example, timer id
	void SetPrivateData(int pd);
	int GetPrivateData() const;

	void PrintCustomTextInOutput(string text);

private:

	/**
     * Single performance counter
     */
    struct PerfCounter
    {
        counter_t* counter;
        T_COUNTER last_value;
    };

    /**
     * Single storage descriptor
     */
    struct PerfStorage
    {
        storage_descriptor_t storage;
        vector<PerfCounter> counters;
    };

    /**
     * Functor used to sort the counters
     */
    struct SortCounters {
        bool operator() (PerfCounter const & lh, PerfCounter const& rh) {
			int idx1 = GetNameIdx(lh.counter->name);
			int idx2 = GetNameIdx(rh.counter->name);
            return strcmp(&lh.counter->name[idx1],&rh.counter->name[idx2]) < 0;
        }
    };

    vector<PerfStorage> m_storages;

    /**
     * Fill counters inside the storage callback
     */
    static int FillCounters(counters_storage_t *, counter_t *counter, void *data);

    /**
     * Fill storages data callback
     */
    static int FillStorages(const storage_descriptor_t *sd, void *data);

	/**
	 * Retrieve index, where real performance counter name starts
	 * @param string name
	 * @return index inside of that string
	 */
	static int GetNameIdx(const char* name);

	bool	m_std_output;
	string	m_output;
	int		m_priv_data;
	int		m_call_dump_count;
	lock_object_t m_lock;

	PerfCounterHost m_host;
	UINT64		m_prev_dump_msec;
};
