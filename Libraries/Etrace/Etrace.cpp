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

#include "Libraries/Etrace/Etrace.h"
#include "Libraries/HostUtils/HostUtils.h"
#include "Interfaces/ParallelsQt.h"
#include "Libraries/Std/PrlTime.h"

#include "Libraries/Std/AtomicOps.h"

#include <QTextStream>
#include <QDateTime>
#include <errno.h>

#ifndef _WIN_
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#define closesocket(x)	close(x)
#else
#define socklen_t		int
#endif

#ifdef ETRACE

#define ETRACE_MIN_DUMP_SIZE	(48ll)

CEtrace::CEtrace()
{
	m_mem = NULL;
	m_time_conv = HostUtils::GetCPUMhz();
#ifndef _WIN_
	m_shmid = -1;
#else
	m_shmid = INVALID_HANDLE_VALUE;
	m_mapid = INVALID_HANDLE_VALUE;
#endif
	m_shmcreate = false;
	m_sock = -1;
	m_port = -1;
}

CEtrace::~CEtrace()
{
}

void CEtrace::register_mem(ETRACE_MEM *mem, UINT size)
{
	if (m_mem)
		WRITE_TRACE(DBG_FATAL, "Etrace: memory is already registered, overwriting...");
	m_mem = mem;
	if (m_mem) {
		m_mem->desc.size = (size - sizeof(ETRACE_DESC)) / sizeof(ETRACE_DATA);
		m_desc_size = m_mem->desc.size;
		m_size = size;
	} else {
		m_desc_size = 1;
		m_size = 0;
	}
}

#ifndef _WIN_
void CEtrace::register_shmem(const char *key, UINT size, bool create)
{
	if (m_shmid >= 0)
		WRITE_TRACE(DBG_FATAL, "Etrace: shared memory is already registered, overwriting...");

	int flags = create ? O_CREAT : 0;
	m_shmid = shm_open(key, flags | O_RDWR, S_IRWXU | S_IRWXG | S_IRWXO);
	if (m_shmid < 0) {
		WRITE_TRACE(DBG_FATAL, "Etrace: failed to create shared memory region: %s", strerror(errno));
		return;
	}
	if (create)
		ftruncate(m_shmid, size);
	ETRACE_MEM *mem = (ETRACE_MEM *)mmap(NULL, size, PROT_READ | PROT_WRITE,
			MAP_SHARED, m_shmid, 0);
	if (mem && create)
		memset(mem, 0, size);
	this->register_mem(mem, size);
	m_shmcreate = create;
	m_shmkey = key;
}

void CEtrace::unregister_shmem(void)
{
	if (m_shmid < 0)
		return;

	if (m_mem) {
		register_mem(NULL, 0);
		munmap(m_mem, m_size);
	}
	close(m_shmid);
	m_shmid = -1;
	if (m_shmcreate)
		shm_unlink(m_shmkey);
}
#else
static bool raise_privilege(void)
{
	TOKEN_PRIVILEGES tp;
	LUID luid;
	bool ret = false;

	if (!LookupPrivilegeValue(NULL, L"SeDebugPrivilege", &luid)) {
		WRITE_TRACE(DBG_FATAL, "LookupPrivilegeValue() failed");
		return ret;
	}

	tp.PrivilegeCount = 1;
	tp.Privileges[0].Luid = luid;
	tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

	HANDLE curProc = GetCurrentProcess();
	HANDLE procToken;
	if (!OpenProcessToken(curProc, TOKEN_ADJUST_PRIVILEGES, &procToken)) {
		WRITE_TRACE(DBG_FATAL, "OpenProcessToken() failed");
		CloseHandle(curProc);
		return ret;
	}

	if (!AdjustTokenPrivileges(procToken, FALSE, &tp, sizeof(tp),
				(PTOKEN_PRIVILEGES)NULL, (PDWORD)NULL))
		WRITE_TRACE(DBG_FATAL, "AdjustTokenPrivileges() failed");
	else
		ret = true;

	CloseHandle(procToken);
	CloseHandle(curProc);
	return ret;
}

void CEtrace::register_shmem(const char *key, UINT size, bool create)
{
	if (m_shmid != INVALID_HANDLE_VALUE)
		WRITE_TRACE(DBG_FATAL, "Etrace: shared memory is already registered, overwriting...");

	DWORD flags = create ? OPEN_ALWAYS : OPEN_EXISTING;
	char temp[256];
	char name[256];

	if (!create && !raise_privilege()) {
		WRITE_TRACE(DBG_FATAL, "Etrace: failed to set debug privilege, errno: %d",
				GetLastError());
		return;
	}

	unsigned int len = GetSystemWindowsDirectoryA(temp, 256);
	if (!len) {
		WRITE_TRACE(DBG_FATAL, "Etrace: GEtSystemWindowsDirectoryA() failed, error: %d",
				GetLastError());
		return;
	}
	_snprintf(name, 256, "%s\\Temp\\%s", temp, key);

	m_shmid = CreateFileA(name, GENERIC_READ | GENERIC_WRITE,
			FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
			flags, FILE_ATTRIBUTE_NORMAL, NULL);
	if (m_shmid == INVALID_HANDLE_VALUE) {
		WRITE_TRACE(DBG_FATAL, "Etrace: failed to %s file for shared memory, error: %d",
				create ? "create" : "open", GetLastError());
		return;
	}
	m_mapid = CreateFileMapping(m_shmid, NULL, PAGE_READWRITE, 0, size, NULL);
	CloseHandle(m_shmid);
	m_shmid = INVALID_HANDLE_VALUE;
	if (m_mapid == NULL) {
		WRITE_TRACE(DBG_FATAL, "Etrace: failed to create file mapping, error: %d",
				GetLastError());
		return;
	}
	ETRACE_MEM *mem = (ETRACE_MEM *)MapViewOfFile(m_mapid, FILE_MAP_ALL_ACCESS,
			0, 0, size);
	if (mem && create)
		memset(mem, 0, size);
	this->register_mem(mem, size);
	m_shmcreate = create;
	m_shmkey = key;
}

void CEtrace::unregister_shmem(void)
{
	if (m_mem) {
		register_mem(NULL, 0);
		UnmapViewOfFile(m_mem);
	}
	CloseHandle(m_mapid);
	m_mapid = INVALID_HANDLE_VALUE;
	if (m_shmcreate)
		DeleteFileA(m_shmkey);
}
#endif

void CEtrace::log(UINT src, UINT type, UINT64 val, UINT64 tsc)
{
	if (!m_mem)
		return;

	if (m_mem->desc.mask & (1ULL << type))
		__ETRACE_LOG(m_mem, src, type, val, tsc);
}

void CEtrace::set_mask(UINT64 mask)
{
	if (!m_mem) {
		WRITE_TRACE(DBG_FATAL, "Etrace is not initialized yet...");
		return;
	}

	if (!m_mem->desc.mask && mask) {
		if (!m_mem->desc.start_tsc) {
			m_mem->desc.start_tsc = ETRACE_UNPACK_TIME(ETRACE_PACK_TIME(HostUtils::GetTsc()));
			m_mem->desc.start_time = PrlGetTimeMonotonic();
		}
	}
	if (m_mem->desc.mask && !mask)
		if (!m_mem->desc.end_time)
			m_mem->desc.end_time = PrlGetTimeMonotonic();

	m_mem->desc.mask = mask;
}

UINT64 CEtrace::get_mask(void)
{
	if (!m_mem)
		return 0;

	return m_mem->desc.mask;
}

bool CEtrace::dump(char **mem, UINT *size)
{
	UINT sz = ETRACE_MIN_DUMP_SIZE;

	if (!m_mem) {
		WRITE_TRACE(DBG_FATAL, "Etrace is not initialized yet...");
		return false;
	}
	if (!mem || !size) {
		WRITE_TRACE(DBG_FATAL, "Either mem or size is NULL (%p, %p)", mem, size);
		return false;
	}

	/*
	 * Calculate how much memory we need to allocate:
	 *   - curr points to empty slot (time is 0) means we never overflowed
	 *     and not all buffer is filled
	 *   - curr points to filled slot (time is not 0) means we overflowed
	 *     and the whole buffer is filled
	 */
	UINT curr = m_mem->desc.curr % m_desc_size;
	UINT64 time = ETRACE_UNPACK_TIME(m_mem->data[curr].tst);
	if (!time)
		sz += curr * sizeof(ETRACE_DATA);
	else
		sz += (m_desc_size + 1) * sizeof(ETRACE_DATA);

	*mem = (char *)malloc(sz);
	if (!mem) {
		WRITE_TRACE(DBG_FATAL, "Failed to allocate memory");
		return false;
	}
	*size = sz;

	CMem2File memwr(*mem, sz);

	return __dump(memwr);
}

bool CEtrace::dump(QFile &file)
{
	bool ret;

	if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
		WRITE_TRACE(DBG_FATAL, "Failed to open %s to dump eTrace buffer",
				QSTR2UTF8(file.fileName()));
		return false;
	}

	ret = __dump(file);

	file.close();
	return ret;
}

void CEtrace::reset(void)
{
	set_mask(0);
	m_mem->desc.curr = 0;
	m_mem->desc.start_tsc = 0;
	m_mem->desc.start_time = 0;
	m_mem->desc.end_time = 0;
	memset(m_mem->data, 0, m_size - sizeof(ETRACE_DESC));
}

bool CEtrace::alias2mask(const QString &alias, UINT64 *mask)
{
	static struct {
		const char *alias;
		UINT64 mask;
	} aliases[] = {
		{
			"io",
			(1ULL << ETRACE_CP_GST_TST) |
		},
		{
			"ios",
			(1ULL << ETRACE_CP_IOSERVICE)
		},
		{
			"all",
			0xffffffffffffffffULL,
		},
	};

	if (!mask)
		return false;

	bool ok;
	UINT64 msk = alias.toULongLong(&ok, 16);
	if (ok) {
		*mask = msk;
		return ok;
	}

	for (unsigned i = 0; i < ARRAY_SIZE(aliases); i++) {
		if (alias == aliases[i].alias) {
			*mask = aliases[i].mask;
			return true;
		}
	}

	return false;
}

bool CEtrace::parse(const char *memin, const UINT32 sizein, QFile &fileout)
{
	CMem2File memrd((char *)memin, sizein);

	if (!fileout.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
		WRITE_TRACE(DBG_FATAL, "Failed to open %s to dump eTrace buffer",
				QSTR2UTF8(fileout.fileName()));
		return false;
	}

	return __parse(memrd, fileout);
}

bool CEtrace::parse(QFile &filein, QFile &fileout)
{
	bool ret;

	if (!filein.open(QIODevice::ReadOnly)) {
		WRITE_TRACE(DBG_FATAL, "Failed to open %s to dump eTrace buffer",
				QSTR2UTF8(filein.fileName()));
		return false;
	}
	if (!fileout.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
		WRITE_TRACE(DBG_FATAL, "Failed to open %s to dump eTrace buffer",
				QSTR2UTF8(fileout.fileName()));
		return false;
	}

	ret = __parse(filein, fileout);

	filein.close();
	fileout.close();
	return ret;
}

template <typename T>
bool CEtrace::__dump(T &storage)
{
	UINT n, end, found = 0;
	UINT64 last_tsc;

	if (!m_mem) {
		WRITE_TRACE(DBG_FATAL, "Etrace is not initialized yet...");
		return false;
	}

	UINT64 tmp = m_time_conv;
	storage.write((const char *)&tmp, sizeof(tmp));
	tmp = m_mem->desc.start_tsc;
	storage.write((const char *)&tmp, sizeof(tmp));
	tmp = m_mem->desc.start_time;
	storage.write((const char *)&tmp, sizeof(tmp));

	last_tsc = m_mem->desc.start_tsc;
	n = end = m_mem->desc.curr % m_desc_size;

	do {
		UINT64 time = ETRACE_UNPACK_TIME(m_mem->data[n].tst);
		if (time != 0) {
			BUILD_BUG_ON(sizeof(m_mem->data[n].tst) != sizeof(UINT64));
			storage.write((const char *)&m_mem->data[n].tst, sizeof(UINT64));
			storage.write((const char *)&m_mem->data[n].data, sizeof(UINT64));
			found++;
			last_tsc = time;
		}
		n = (n + 1) % m_desc_size;
	} while (n != end);

	if (found == m_desc_size) {
		ETRACE_DATA d;
		__ETRACE_FILL_DATA(d, ETRACE_CP_SPECIAL, 0, last_tsc, ETRACE_CP_SPECIAL_OVERFLOW);
		storage.write((const char *)&d.tst, sizeof(UINT64));
		storage.write((const char *)&d.data, sizeof(UINT64));
		WRITE_TRACE(DBG_FATAL, "WARNING: eTrace buffer overlow detected!");
	}

	tmp = 0;
	storage.write((const char *)&tmp, sizeof(tmp));
	storage.write((const char *)&tmp, sizeof(tmp));
	tmp = m_mem->desc.end_time;
	storage.write((const char *)&tmp, sizeof(tmp));

	return true;
}

int CEtrace::recv_data(int sock, char *buf, int len)
{
	if (recv(sock, buf, len, 0) != len) {
		WRITE_TRACE(DBG_FATAL, "recv() failed: %s", strerror(errno));
		closesocket(sock);
		closesocket(m_sock);
		return -1;
	}

	return 0;
}

int CEtrace::send_data(int sock, char *buf, int len)
{
	int ret;

	while (len) {
		ret = send(sock, buf, len, 0);
		if (ret == -1)
			return -1;
		len -= ret;
	}
	return 0;
}

void CEtrace::run()
{
	int quit = 0;

	if (!m_mem)
		return;

	while (!quit) {
		int sock;
		char cmd;
		sockaddr_in sin;
		socklen_t addrlen = sizeof(sockaddr_in);

		if ((sock = accept(m_sock, (struct sockaddr *)&sin, &addrlen)) == -1) {
			WRITE_TRACE(DBG_FATAL, "accept() failed: %s", strerror(errno));
			closesocket(m_sock);
			return;
		}

		if (recv_data(sock, &cmd, sizeof(cmd)))
			return;

		switch (cmd) {
			case ETRACE_CMD_SETMASK: {
				UINT64 mask;
				if (recv_data(sock, (char *)&mask, sizeof(mask)))
					return;
				set_mask(mask);
				break;
			}

			case ETRACE_CMD_GETDATA: {
				// temporary stop eTrace
				UINT64 mask = m_mem->desc.mask;
				m_mem->desc.mask = 0;

				UINT32 len;
				char *data;
				dump(&data, &len);

				if (send_data(sock, (char *)&len, sizeof(len)))
					return;
				if (send_data(sock, data, len))
					return;

				free(data);
				m_mem->desc.mask = mask;
				break;
			}

			case ETRACE_CMD_RESET:
				reset();
				break;

			case ETRACE_CMD_QUIT:
				quit = 1;
				break;

			default:
				WRITE_TRACE(DBG_FATAL, "Unknown command: %c", cmd);
				break;
		}
		closesocket(sock);
	}
}

void CEtrace::start_thread(UINT port)
{
	int on = 1;
	sockaddr_in sin;

	m_port = port;

	m_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (m_sock < 0) {
		WRITE_TRACE(DBG_FATAL, "Can't create socket");
		return;
	}

	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = INADDR_ANY;
	sin.sin_port = htons(port);

	setsockopt(m_sock, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on));

	if (bind(m_sock, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
		WRITE_TRACE(DBG_FATAL, "bind() failed: %s", strerror(errno));
		closesocket(m_sock);
		return;
	}

	if (listen(m_sock, 1) < 0) {
		WRITE_TRACE(DBG_FATAL, "listen() failed: %s", strerror(errno));
		closesocket(m_sock);
		return;
	}

	start();
}

int CEtrace::send_stop(void)
{
	int sock;
	sockaddr_in sin;
	char cmd = ETRACE_CMD_QUIT;

	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0)
		return -1;
	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = inet_addr("127.0.0.1");
	sin.sin_port = htons(m_port);

	if (::connect(sock, (struct sockaddr *)&sin, sizeof(sin)) < 0)
		return -1;
	if (send(sock, &cmd, sizeof(cmd), 0) != 1)
		return -1;
	closesocket(sock);

	return 0;
}


void CEtrace::stop_thread(void)
{
	if (isFinished())
		return;

	if (send_stop()) {
		WRITE_TRACE(DBG_FATAL, "Failed to properly stop eTrace thread...");
		return;
	}

	wait();
}

template <typename T>
static UINT64 inline read64(T &storage)
{
	UINT64 t;
	storage.read((char *)&t, sizeof(t));
	return t;
}

template <typename T>
static void inline write64(T &storage, UINT64 val)
{
	storage.write((const char *)&val, sizeof(val));
}

template <typename T>
bool CEtrace::__parse(T &storin, QFile &fileout)
{
	UINT64 time_conv, start_tsc, start_time, end_time;
	UINT64 time_max = 0, time_min = ~0ULL, time_prev = 0;
	UINT64 tdelta = 0;
	LONG64 size = storin.size();
	UINT found = 0, checked = 0;

	if (size < ETRACE_MIN_DUMP_SIZE) {
		WRITE_TRACE(DBG_FATAL, "Input file is too small, not a eTrace dump");
		return false;
	}

#ifdef _WIN_
	FILE *fd = _fdopen(fileout.handle(), "w");
#else
	FILE *fd = fdopen(fileout.handle(), "w");
#endif
	if (!fd) {
		WRITE_TRACE(DBG_FATAL, "Failed to reopen output file");
		return false;
	}


	time_conv = read64(storin);
	start_tsc = read64(storin);
	start_time = read64(storin);

	fprintf(fd, "========================= eTrace dump ==============================\n");
	if (!(time_conv & 0xffffffff00000000)) {
		// high bits are not set, this is really CPU MHz
		fprintf(fd, "CPU freq is %llu MHz\n", time_conv);
	} else {
		fprintf(fd, "CPU timebase, numer: %u, denom: %u\n",
				(UINT32)(time_conv & 0xffffffff), (UINT32)((time_conv >> 32) & 0xffffffff));
	}
	fprintf(fd, "Abs time (s)   Delta (us)\n");

	while (1) {
		UINT64 time;
		ETRACE_DATA data;

		data.tst = read64(storin);
		data.data = read64(storin);

		if (!data.tst)
			break;
		if (checked >= (size - ETRACE_MIN_DUMP_SIZE) / sizeof(ETRACE_DATA))
			break;

		time = ETRACE_UNPACK_TIME(data.tst);
		if (time != 0 && time > start_tsc) {
			time = CEtrace::abs2real(time - start_tsc, time_conv);
			if (time < time_min)
				time_min = time;
			if (time > time_max)
				time_max = time;

			parse_event(fd, &data, time, time_prev, time_conv);

			time_prev = time;
			found++;
		}
		checked++;
	}

	if (time_max > time_min)
		tdelta = time_max - time_min;

	fprintf(fd, "Total %d event found\n", found);
	fprintf(fd, "Log duration: according to TSC %llu.%06llu sec\n",
			tdelta / 1000000, tdelta % 1000000);

	end_time = read64(storin);
	tdelta = end_time - start_time;
	fprintf(fd, "                     real time %llu.%06llu sec\n",
			tdelta / 1000000, tdelta % 1000000);

	fflush(fd);
	return true;
}

/*
 * FIXME:
 * Copy-pasted from Vanderpool.h
 */
#define REASON_EXC_NMI			(0)
#define REASON_IRQ				(1)
#define REASON_TRIPLE_FAULT		(2)
#define REASON_INIT				(3)
#define REASON_SIPI				(4)
#define REASON_IRQ_WND			(7)
#define REASON_VIRTUAL_NMI_WND	(8)
#define REASON_TASK				(9)
#define REASON_CPUID			(10)
#define REASON_HLT				(12)
#define REASON_INVD				(13)
#define REASON_INVLPG			(14)
#define REASON_RDPMC			(15)
#define REASON_RDTSC			(16)
#define REASON_VMCALL			(18)
#define REASON_VMCLEAR			(19)
#define REASON_VMLAUNCH			(20)
#define REASON_VMPTRLD			(21)
#define REASON_VMPTRST			(22)
#define REASON_VMREAD			(23)
#define REASON_VMRESUME			(24)
#define REASON_VMWRITE			(25)
#define REASON_VMOFF			(26)
#define REASON_VMON				(27)
#define REASON_MOV_CR			(28)
#define REASON_MOV_DR			(29)
#define REASON_IO				(30)
#define REASON_RDMSR			(31)
#define REASON_WRMSR			(32)
#define REASON_VMENTRY_GUEST	(33)
#define REASON_VMENTRY_MSR		(34)
#define REASON_MWAIT			(36)
#define REASON_MONITOR			(39)
#define REASON_PAUSE			(40)
#define REASON_VMENTRY_MC		(41)
#define REASON_TPR_THRESHOLD	(43)
#define REASON_APIC_ACCESS		(44)
#define REASON_EPT_VIOLATION	(48)
#define REASON_EPT_MISCONFIG	(49)

/*
 * FIXME:
 * Copy-pasted from Ia32.h
 */
#define EXCEPTION_DE 0			/* Divide Error (fault) */
#define EXCEPTION_DB 1			/* Debug (fault/trap) */
#define EXCEPTION_BP 3			/* Breakpoint (trap) */
#define EXCEPTION_OF 4			/* Overflow (trap) */
#define EXCEPTION_BR 5			/* BOUND (fault) */
#define EXCEPTION_UD 6			/* UnDefined opcode (fault) */
#define EXCEPTION_NM 7			/* No Math coprocessor (fault) */
#define EXCEPTION_DF 8			/* Double Fault (abort,EC=0) */
#define EXCEPTION_TS 10			/* Task Segment (fault,EC) */
#define EXCEPTION_NP 11			/* Not Present (fault,EC) */
#define EXCEPTION_SS 12			/* Stack Segment (fault,EC) */
#define EXCEPTION_GP 13			/* General Protection (fault,EC) */
#define EXCEPTION_PF 14			/* Page Fault (fault,EC) */
#define EXCEPTION_MF 16			/* Math coprocessor Fault (fault) */
#define EXCEPTION_AC 17			/* Align Check (fault,EC=0) */
#define EXCEPTION_MC 18			/* Machine Check (abort,EC) */
#define EXCEPTION_XF 19			/* Xmm Fault (fault) */

/*
 * Definitions for VM-Entry interruption information field.
 */
#define IRQ_INFO_EXT_IRQ		(0 << 8)
#define IRQ_INFO_NMI			(2 << 8)
#define IRQ_INFO_HARD_EXC		(3 << 8)
#define IRQ_INFO_SOFT_IRQ		(4 << 8)
#define IRQ_INFO_PRIV_SOFT_EXC	(5 << 8)
#define IRQ_INFO_SOFT_EXC		(6 << 8)

#define IRQ_INFO_ERROR_VALID	(1 << 11)
#define IRQ_INFO_VALID			(1 << 31)

void CEtrace::parse_event(FILE *fd, ETRACE_DATA *data, UINT64 time, UINT64 prev_time,
		UINT64 cpu_mhz)
{
	(void)cpu_mhz;

	if (prev_time == 0)
		prev_time = time;

	fprintf(fd, "%llu.%06llu %7lld  ", time / 1000000, time % 1000000,
		(LONG64)(time - prev_time));

	ETRACE_DATA *p = data;
	UINT src = (UINT)ETRACE_UNPACK_SRC(p->tst);
	UINT type = ETRACE_UNPACK_TYPE(p->tst);

	switch(type) {
	case ETRACE_CP_IOSERVICE:
	{
		static const char *events[] = {
			"SrvListen", "SrvAccept", "SrvClose", "CliClose", "SrvConnect", "SrvConnectProxy",
			"CliConnect", "CliConnectProxy", "MngConnect", "SrvSetHs", "SrvSetHsSSL",
			"SrvSendHs", "SrvSendHsSSL", "SrvSendRsHsSSL", "SrvSendHsProxy", "CliSendHS",
			"CliSendHsSSL", "CliSendHsProxy", "MngSendHs", "MngRecvReqCmd", "MngRecvReqData",
			"MngRecvBRK_REQ_CMD", "MngRecvBRK_REQ_DATA", "MngSendRES", "MngSendBrkRes",
			"MngSendHbeat", "MngRecvHbeatChange", "RecvPkgSSL", "SendPkg", "SendPkgSSL",
			"Unknown" };

		unsigned evt = p->data & 0x3F;
		if (evt >= ARRAY_SIZE(events))
			evt = ARRAY_SIZE(events) - 1;

		char args[1024] = "";

		switch (evt) {
		case ETRACE_IOS_EVENT_SRV_ACCEPT:
			sprintf(args, ",   sock_id2:%10d", (int)(p->data >> 32));
			break;

		case ETRACE_IOS_EVENT_MNG_RECV_REQ_CMD:
		case ETRACE_IOS_EVENT_MNG_RECV_BRK_REQ_CMD:
			sprintf(args, ",        cmd:%10d,      size:%10d",
				(int)(p->data >> 32),
				(int)(p->data >> 10) & 0x3FFFFF);
			break;

		case ETRACE_IOS_EVENT_MNG_RECV_REQ_DATA:
		case ETRACE_IOS_EVENT_MNG_RECV_BRK_REQ_DATA:
		case ETRACE_IOS_EVENT_MNG_SEND_BRK_RES:
			sprintf(args, ",       size:%10d", (int)(p->data >> 10) & 0x3FFFFF);
			break;

		case ETRACE_IOS_EVENT_MNG_SEND_RES:
			sprintf(args, ",        cmd:%10d,      size:%10d",
				(int)(p->data >> 32),
				(int)(p->data >> 10) & 0x3FFFFF);
			break;

		case ETRACE_IOS_EVENT_MNG_RECV_HBEAT_CHNG:
			sprintf(args, ",    send_tm:%10d,   recv_tm:%10d",
				(int)(p->data >> 32),
				(int)(p->data >> 10) & 0x3FFFFF);
			break;

		case ETRACE_IOS_EVENT_RECV_PKG_SSL:
		case ETRACE_IOS_EVENT_SEND_PKG:
		case ETRACE_IOS_EVENT_SEND_PKG_SSL:
			sprintf(args, ",       type:%10d,      size:%10d,      cnt:%10d ",
				(int)(p->data >> 32),
				(int)(p->data >> 14) & 0x3FFFF,
				(int)(p->data >> 10) & 0xF);
			break;
		}

		static const char *senders[] = {
			"IOS_UNK", "IOS_VM", "IOS_DISP", "IOS_CLI",
			"IOS_IOCL", "IOS_FILE", "IOS_PTAG", "IOS_PTCL",
			"IOS_PMNG", "IOS_IOCT", "IOS_DCAG", "IOS_DCCL" };

		unsigned sender = (p->data >> 6) & 0xF;
		if (sender >= ARRAY_SIZE(senders))
			sender = 0;

		fprintf(fd, "IOS:%9s:%15s:I   sock_id1:%10d%s\n",
			senders[sender],
			events[evt],
			src,
			args);
		break;
	}

	case ETRACE_CP_SPECIAL:
		if (p->data == ETRACE_CP_SPECIAL_OVERFLOW)
			fprintf(fd, "%d: WARNING: eTrace buffer overflow detected!\n\n", src);
		else
			fprintf(fd, "unknown special event: data %llx\n", p->data);
		break;
	default:
		fprintf(fd, "unknown type %llx, src %llx, data %llx\n",
			ETRACE_UNPACK_TYPE(p->tst),
			ETRACE_UNPACK_SRC(p->tst),
			p->data);
	}
}

UINT64 CEtrace::abs2real(UINT64 abs_time, UINT64 time_conv)
{
	UINT32 denom, numer;

	if (!(time_conv & 0xffffffff00000000))
		return abs_time / time_conv;

	numer = time_conv & 0xffffffff;
	denom = (time_conv >> 32) & 0xffffffff;
	return abs_time * numer / (denom * 1000);
}

UINT64 CEtrace::real2abs(UINT64 real_time, UINT64 time_conv)
{
	UINT32 denom, numer;

	if (!(time_conv & 0xffffffff00000000))
		return real_time * time_conv;

	numer = time_conv & 0xffffffff;
	denom = (time_conv >> 32) & 0xffffffff;
	return real_time * 1000 * denom / numer;
}

struct etrace_merge_ctx
{
	QFile *f;
	UINT64 timestamp;
	UINT64 tsc;
	UINT64 cpu_mhz;
	ETRACE_DATA d;
};

static inline void swap_merge_ctx(etrace_merge_ctx **a)
{
	etrace_merge_ctx *x = a[0];
	a[0] = a[1];
	a[1] = x;
}

static inline void read_mergectx_data(etrace_merge_ctx *a)
{
	a->d.tst = read64(*a->f);
	a->d.data = read64(*a->f);
}

static inline void write_mergectx_data(QFile &f, etrace_merge_ctx **a, int min_idx, UINT64 diff)
{
	UINT64 tsc = CEtrace::abs2real(ETRACE_UNPACK_TIME(a[0]->d.tst), a[0]->cpu_mhz);
	if (min_idx)
		tsc -= diff;
	tsc = CEtrace::real2abs(tsc, a[min_idx]->cpu_mhz);
	a[0]->d.tst &= ~ETRACE_CYCLES_MASK;
	a[0]->d.tst |= ETRACE_PACK_TIME(tsc);
	write64(f, a[0]->d.tst);
	write64(f, a[0]->d.data);
}

static inline int compare_time(etrace_merge_ctx *a, etrace_merge_ctx *b, UINT64 diff)
{
	/* use relative timestamps only for dumps from different hosts */
	UINT64 tsc1 = ETRACE_UNPACK_TIME(a->d.tst) - a->tsc * !!diff;
	UINT64 tsc2 = ETRACE_UNPACK_TIME(b->d.tst) - b->tsc * !!diff;
	if (CEtrace::abs2real(tsc1, a->cpu_mhz) > CEtrace::abs2real(tsc2, b->cpu_mhz))
		return 1;
	return 0;
}

bool CEtrace::merge(QFile &filein1, QFile &filein2, QFile &fileout)
{
	etrace_merge_ctx info1;
	etrace_merge_ctx info2;
	info1.f = &filein1;
	info2.f = &filein2;

	etrace_merge_ctx *i[2] = { &info1, &info2 };

	if (!filein1.open(QIODevice::ReadOnly)) {
		WRITE_TRACE(DBG_FATAL, "Failed to open %s to dump eTrace buffer",
				QSTR2UTF8(filein1.fileName()));
		return false;
	}
	if (!filein2.open(QIODevice::ReadOnly)) {
		WRITE_TRACE(DBG_FATAL, "Failed to open %s to dump eTrace buffer",
				QSTR2UTF8(filein2.fileName()));
		return false;
	}
	if (!fileout.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
		WRITE_TRACE(DBG_FATAL, "Failed to open %s to dump eTrace buffer",
				QSTR2UTF8(fileout.fileName()));
		return false;
	}

	if ((filein1.size() < ETRACE_MIN_BUF_SIZE) ||
			(filein2.size() < ETRACE_MIN_BUF_SIZE)) {
		WRITE_TRACE(DBG_FATAL, "Input files are too small, not eTrace dumps");
		return false;
	}

	i[0]->cpu_mhz = read64(*i[0]->f);
	i[1]->cpu_mhz = read64(*i[1]->f);
	if (i[0]->cpu_mhz != i[1]->cpu_mhz) {
		WRITE_TRACE(DBG_FATAL,
				"Files were written on different hosts, cpu_mhz mismatch: %lld %lld.",
				i[0]->cpu_mhz, i[1]->cpu_mhz);
	}

	i[0]->tsc = read64(*i[0]->f);
	i[0]->timestamp = read64(*i[0]->f);
	i[1]->tsc = read64(*i[1]->f);
	i[1]->timestamp = read64(*i[1]->f);

	int min_idx;
	UINT64 time0 = CEtrace::abs2real(i[0]->tsc, i[0]->cpu_mhz);
	UINT64 time1 = CEtrace::abs2real(i[1]->tsc, i[1]->cpu_mhz);
	UINT64 time_diff;
	if (time0 > time1) {
		min_idx = 1;
		time_diff = time0 - time1;
	} else {
		min_idx = 0;
		time_diff = time1 - time0;
	}
	if (i[0]->cpu_mhz == i[1]->cpu_mhz)
		time_diff = 0;
	write64(fileout, i[min_idx]->cpu_mhz);
	write64(fileout, i[min_idx]->tsc);
	write64(fileout, i[min_idx]->timestamp);

	read_mergectx_data(i[0]);
	read_mergectx_data(i[1]);
	if (compare_time(i[0], i[1], time_diff) > 0) {
		swap_merge_ctx(i);
		min_idx = !min_idx;
	}

	while (i[0]->d.tst) {
		write_mergectx_data(fileout, i, min_idx, time_diff);
		read_mergectx_data(i[0]);
		if (compare_time(i[0], i[1], time_diff) > 0) {
			swap_merge_ctx(i);
			min_idx = !min_idx;
		}
	}

	swap_merge_ctx(i);
	min_idx = !min_idx;
	while(i[0]->d.tst) {
		write_mergectx_data(fileout, i, min_idx, time_diff);
		read_mergectx_data(i[0]);
	}

	write64(fileout, 0);
	write64(fileout, 0);

	UINT64 ts1 = read64(filein1);
	UINT64 ts2 = read64(filein2);
	if (ts1 > ts2)
		write64(fileout, ts1);
	else
		write64(fileout, ts2);

	filein1.close();
	filein2.close();
	fileout.close();

	return true;
}

CMem2File::CMem2File(char *mem, UINT size)
{
	m_mem = mem;
	m_size = size;
}

CMem2File::~CMem2File()
{
}

LONG64 CMem2File::write(const char *data, LONG64 size)
{
	if (!data)
		return -1;

	LONG64 sz = MIN(size, m_size);
	memcpy(m_mem, data, sz);
	m_mem += sz;
	m_size -= sz;

	return sz;
}

LONG64 CMem2File::read(char *data, LONG64 size)
{
	if (!data)
		return -1;

	LONG64 sz = MIN(size, m_size);
	memcpy(data, m_mem, sz);
	m_mem += sz;
	m_size -= sz;

	return sz;
}


CEtraceStatic* CEtraceStatic::s_etrace_static = 0;
CEtraceStatic* CEtraceStatic::get_instance()
{
	if (!s_etrace_static)
		s_etrace_static = new CEtraceStatic();
	return s_etrace_static;
}

void CEtraceStatic::log_static(UINT src, UINT type, UINT64 val)
{
	if (!s_etrace_static)
		return;
	if (!(s_etrace_static->get_mask() & (1ULL << type)))
		return;
	UINT64 tsc = HostUtils::GetTsc();

	ETRACE_MEM *mem = s_etrace_static->m_mem;
	UINT32 curr = EtrAtomicInc(&mem->desc.curr) % s_etrace_static->m_desc_size;
	__ETRACE_FILL_DATA(mem->data[curr], type, src, tsc, val);
}

CEtraceStatic::CEtraceStatic() : CEtrace()
{
	m_mem_buf = 0;
	m_ctrl_thread = false;
}

void CEtraceStatic::init(bool ctrl_thread)
{
	(void)ctrl_thread;

	register_shmem(ETRACE_SHMEM_PAX, ETRACE_DEFAULT_BUF_SIZE, true);

	if (m_ctrl_thread)
		start_thread(ETRACE_PORT_PAX);

	etrace_log_func = log_static;

	update_log_level();
}

void CEtraceStatic::deinit()
{
	etrace_log_func = 0;

	if (m_ctrl_thread)
		stop_thread();

	unregister_shmem();
}

bool CEtraceStatic::dump_and_parse(const char* path)
{
	UINT64 mask = get_mask();
	set_mask(0ULL);
	UINT sz = 0;
	char* buf = NULL;
	bool res = dump(&buf, &sz);
	set_mask(mask);
	if (!res) {
		WRITE_TRACE(DBG_FATAL, "Etrace dump failed");
		return false;
	}

	WRITE_TRACE(DBG_WARNING, "Etrace dump, path=\"%s\"", path);
	QFile file(path);
	res = parse(buf, sz, file);
	free(buf);
	return res;
}

void CEtraceStatic::update_log_level()
{
	set_log_level(::GetLogLevel());
}

void CEtraceStatic::set_log_level(int level)
{
	set_mask(level == DBG_DEBUG ?  0xffffffffffffffffULL : 0ULL);
	WRITE_TRACE(DBG_WARNING, "Etrace MASK %llx", get_mask());
}

int etrace_dump_and_parse(const char *path)
{
	return CEtraceStatic::get_instance()->dump_and_parse(path)? 0: -1;
}

#endif
