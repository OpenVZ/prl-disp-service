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

#ifndef __LIB_ETRACE_H__
#define __LIB_ETRACE_H__

#ifdef __cplusplus

#include "Libraries/Std/Etrace.h"

#include <QString>
#include <QFile>
#include <QThread>

#ifdef ETRACE

#ifndef _WIN_
#define ETRACE_SHMEM_VMAPP	"/etrace_vmapp"
#define ETRACE_SHMEM_PAX	"/etrace_pax"
#else
#define ETRACE_SHMEM_VMAPP	"etrace_vmapp"
#define ETRACE_SHMEM_PAX	"etrace_pax"

#include <windows.h>
#endif

#define ETRACE_PORT_BASE	55655
#define ETRACE_PORT_PAX		(ETRACE_PORT_BASE + 1)

/*
 * Set eTrace mask
 *   in - UINT64 mask
 *   out - none
 */
#define ETRACE_CMD_SETMASK	'm'

/*
 * Get eTrace data buffer
 *   in - none
 *   out - UINT32 len, array - data
 */
#define ETRACE_CMD_GETDATA	'r'

/*
 * Reset eTrace to initial stage
 *   in - none
 *   out - none
 */
#define ETRACE_CMD_RESET	'x'

/*
 * Terminate eTrace thread
 *   in - none
 *   out - none
 */
#define ETRACE_CMD_QUIT		'q'

#ifndef _WIN_
#include <sys/mman.h>
#endif

class CMem2File {
public:
	CMem2File(char *mem, UINT size);
	~CMem2File();

	LONG64 write(const char *data, LONG64 size);
	LONG64 read(char *data, LONG64 size);
	LONG64 size(void) { return m_size; }
private:
	char *m_mem;
	UINT m_size;
};

class CEtrace : public QThread {
public:
	CEtrace();
	~CEtrace();

	/*
	 * @brief
	 *		Register memory for eTrace
	 *
	 * @param mem	- pointer to memory
	 * @param suze	- size in bytes
	 *
	 * Caller takes care of allocation/deallocation
	 */
	void register_mem(ETRACE_MEM *mem, UINT size);

	/*
	 * @brief
	 *		Register shared memory for eTrace
	 *
	 * @param key	- shmem key
	 * @param size	- size in bytes
	 * @param create- create shmem if not exists
	 */
	void register_shmem(const char *key, UINT size, bool create);

	/*
	 * @brief
	 *		Unregister eTrace shared memory
	 */
	void unregister_shmem(void);

	/*
	 * @brief
	 *		Log event into eTrace buffer
	 *
	 * @param src	- source of event (VCPU, disk id, etc)
	 * @param type	- eTrace checkpoint for this event
	 * @param val	- event data (up to 8 bytes)
	 * @param tsc	- event timestamp in CPU ticks
	 */
	void log(UINT src, UINT type, UINT64 val, UINT64 tsc);

	/*
	 * @brief
	 *		Set checkpoints mask
	 *
	 * @param mask	- new mask value
	 */
	void set_mask(UINT64 mask);

	/*
	 * @brief
	 *		Get current checkpoint mask
	 *
	 * @return checkpoint mask
	 */
	UINT64 get_mask(void);

	/*
	 * @brief
	 *		Dump eTrace buffer and required info into memory
	 *
	 * @param mem	- pointer to pointer to memory
	 * @param size	- size of allocated memory
	 *
	 * @return true if success, false otherwise
	 *
	 * Function allocates the required mount of memory and stores
	 * pointer to it in mem and size of it in size. Caller is responsible
	 * for freeing memory.
	 */
	bool dump(char **mem, UINT *size);

	/*
	 * @brief
	 *		Dump eTrace buffer and required info into file
	 *
	 * @param file	- valid QFile object
	 *
	 * @return true if success, false otherwise
	 */
	bool dump(QFile &file);

	/*
	 * @brief
	 *		Stops tracing, clears memory, e.g. resetes to intial state
	 */
	void reset(void);

	/*
	 * @brief
	 *		Start eTrace commands listening thread
	 *
	 * @param port	- port to listen to commands on
	 */
	void start_thread(UINT port);

	/*
	 * @brief
	 *		Stop eTrace command listening thread
	 */
	void stop_thread(void);

	/*
	 * @brief
	 *		Convert checkpoints set alias into mask
	 *
	 * @param alias	- checkpoint alias
	 * @param mask	- resulting mask
	 *
	 * @return true if parsed successfully, false otherwise (mask is not modified)
	 */
	static bool alias2mask(const QString &alias, UINT64 *mask);

	/*
	 * @brief
	 *		Parse eTrace binary data from memory
	 *
	 * @param memin		- pointer to memory with binary data
	 * @param sizein	- size of input memory
	 * @param fileout	- output file for parsed data
	 *
	 * @return true if success, false otherwise
	 */
	static bool parse(const char *memin, const UINT sizein, QFile &fileout);

	/*
	 * @brief
	 *		Parse eTrace binary data from file
	 *
	 * @param filein	- input file with binary data
	 * @param fileout	- output file for parsed data
	 *
	 * @return true if success, false otherwise
	 */
	static bool parse(QFile &filein, QFile &fileout);

	/**
	 * @brief
	 *		Merge two eTrace binary dumps into one
	 *
	 * @param filein1	- first input file with binary data
	 * @param fileint2	- second input file with binary data
	 * @param fileout	- output file for merged binary data
	 *
	 * @return true if success, false otherwise
	 */
	static bool merge(QFile &filein1, QFile &filein2, QFile &fileout);

protected:
	ETRACE_MEM *m_mem;
	UINT m_size;		// size of memory in bytes
	UINT m_desc_size;	// size of memory in  ETRACE_DATA (cache for desc.size)

	/*
	 * Used to convert eTrace time values to real-life time values:
	 *   - on x86 - CPU Mhz value
	 *   - on iOS - mach_timebase_info
	 */
	UINT64 m_time_conv;
#ifndef _WIN_
	int m_shmid;
#else
	HANDLE m_shmid;
	HANDLE m_mapid;
#endif
	const char *m_shmkey;
	bool m_shmcreate;

	int m_sock;
	int m_port;

	template <typename T>
	bool __dump(T &storage);

	int recv_data(int sock, char *buf, int len);
	int send_data(int sock, char *buf, int len);
	int send_stop(void);
	void run(void);

	template <typename T>
	static bool __parse(T &storage, QFile &fileout);
	static void parse_event(FILE *fd, ETRACE_DATA *data, UINT64 time, UINT64 prev_time,
			UINT64 cpu_mhz);

public:
	static UINT64 abs2real(UINT64 abs_time, UINT64 time_conv);
	static UINT64 real2abs(UINT64 real_time, UINT64 time_conv);
};


class CEtraceStatic : private CEtrace
{
public:
	static CEtraceStatic* get_instance();
	void init(bool ctrl_thread = false);
	void deinit();
	void set_log_level(int level);
	void update_log_level();
	bool dump_and_parse(const char *path);

private:
	static void log_static(UINT src, UINT type, UINT64 val);
	static CEtraceStatic* s_etrace_static;

	CEtraceStatic();
	char* m_mem_buf;
	bool m_ctrl_thread;
};

#endif /* ETRACE */

#endif /* __cplusplus */

#ifdef __cplusplus
extern "C" {
#endif
int etrace_dump_and_parse(const char *path);
#ifdef __cplusplus
}
#endif

#endif
