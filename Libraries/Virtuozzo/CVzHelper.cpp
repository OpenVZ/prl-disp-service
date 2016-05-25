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

#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <syscall.h>
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/file.h>
#include <dlfcn.h>
#include <asm/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <linux/netlink.h>
#include <string.h>
#include <dirent.h>
#include <numeric>

#include <QThread>
#include <QString>
#include <QByteArray>
#include <prlcommon/Logging/Logging.h>
#include "Libraries/PrlCommonUtils/CFileHelper.h"
#include <prlsdk/PrlOses.h>
#include <prlsdk/PrlIOStructs.h>
#include <prlcommon/PrlUuid/Uuid.h>
#include "UuidMap.h"
#include "CVzHelper.h"
#include "CVzNetworkShaping.h"
#include "Libraries/HostInfo/CHostInfo.h"
#include <prlcommon/Interfaces/ParallelsNamespace.h>
#include <prlxmlmodel/HostHardwareInfo/CHostHardwareInfo.h>
#include <prlcommon/HostUtils/HostUtils.h>
#include <prlcommon/PrlCommonUtilsBase/StringUtils.h>

#include <vzctl/libvzctl.h>
#include <vzctl/vzctl_param.h>

#ifndef FAIRSCHED_SET_RATE
	#define FAIRSCHED_SET_RATE      0
#endif
#ifndef FAIRSCHED_DROP_RATE
	#define FAIRSCHED_DROP_RATE     1
#endif

#define VZCTLLIB	"libvzctl2.so.0"
#define QUOTENAME(x) #x

#define BIN_VZCTL	"/usr/sbin/vzctl"
#define FINDMNT		"/usr/bin/findmnt"

QMutex CVzHelper::s_mtxEnvUuidMap;
QHash< QString, QString > CVzHelper::s_envUuidMap;

CNumaNode CVzHelper::s_numanodes;
QMutex CVzHelper::s_mtxNodemask(QMutex::Recursive);

static void reserve_hugepages(int node, unsigned long RamMb, unsigned long *reserved);

class VzctlHandleWrap
{
private:
	vzctl_env_handle_ptr m_h;

public:
	VzctlHandleWrap() : m_h(NULL)
	{}

	VzctlHandleWrap(vzctl_env_handle_ptr h) : m_h(h)
	{}
	~VzctlHandleWrap()
	{
		if (m_h != NULL) {
			vzctl2_env_close(m_h);
			m_h = NULL;
		}
	}
	operator vzctl_env_handle_ptr () const
	{
		return (m_h);
	}
	void reset(vzctl_env_handle_ptr env_)
	{
		if (m_h != NULL) {
			vzctl2_env_close(m_h);
		}
		m_h = env_;
	}
};

class VzctlParamWrap
{
private:
	vzctl_env_param_ptr m_p;

public:
	VzctlParamWrap(vzctl_env_param_ptr p) : m_p(p)
	{}

	~VzctlParamWrap()
	{
		if (m_p != NULL) {
			vzctl2_free_env_param(m_p);
			m_p = NULL;
		}
	}
	operator vzctl_env_param_ptr () const
	{
		return (m_p);
	}
};

CVzHelper::CVzHelper()
{
	init_lib();
}

CVzHelper::~CVzHelper()
{
}

int CVzHelper::init_lib()
{
	vzctl2_init_log("prl_disp_service");
	vzctl2_set_log_quiet(1);
	vzctl2_set_flags(VZCTL_FLAG_DONT_SEND_EVT);
	if (vzctl2_lib_init()) {
		WRITE_TRACE(DBG_FATAL, "vzctl2_lib_init: %s",
				vzctl2_get_last_error());
		return -1;
	}

	return 0;
}


#define BITS_PER_LONG		(sizeof(unsigned long) * 8)
#define BITS_PER_CPUMASK	(CPUMASK_SIZE * BITS_PER_LONG)

#define DIV_ROUND_UP(n,d)	(((n) + (d) - 1) / (d))
#define BITS_TO_LONGS(nr)	DIV_ROUND_UP((nr), BITS_PER_LONG)
#define BIT_MASK(nr)		(1UL << ((nr) % BITS_PER_LONG))
#define BIT_WORD(nr)		((nr) / BITS_PER_LONG)

static inline int test_bit(int nr, const unsigned long *map)
{
	return (map[BIT_WORD(nr)] & BIT_MASK(nr)) != 0;
}

static inline void bitmap_set_bit(int nr, unsigned long *map)
{
	map[BIT_WORD(nr)] |= BIT_MASK(nr);
}

static inline void bitmap_zero(unsigned long *map, int nmaskbits)
{
	memset(map, 0, BITS_TO_LONGS(nmaskbits) * sizeof(long));
}

static int bitmap_empty(const unsigned long *map, int size)
{
	int i, len = size / sizeof(unsigned long);

	for (i = 0; i < len; i++)
		if (map[i])
			return 0;
	return 1;
}

int bitmap_andnot(unsigned long *dst, const unsigned long *map1,
		const unsigned long *map2, int size)
{
	int i, len = size / sizeof(unsigned long);
	unsigned long result = 0;

	for (i = 0; i < len; i++)
		result |= (dst[i] = map1[i] & ~map2[i]);

	return result != 0;
}

int bitmap_or(unsigned long *dst, const unsigned long *map1,
		const unsigned long *map2, int size)
{
	int i, len = size / sizeof(unsigned long);
	unsigned long result = 0;

	for (i = 0; i < len; i++)
		result |= (dst[i] = map1[i] | map2[i]);

	return result != 0;
}

static int bitmap_find_first_bit(const unsigned long *map, int nmaskbits)
{
	int i, n;
	int nmasklongs = BITS_TO_LONGS(nmaskbits);

	for (i = 0; i < nmasklongs; i++) {
		if (map[i] != 0)
			break;
	}
	if (i == nmasklongs)
		return nmaskbits;
	i *= BITS_PER_LONG;
	n = i + BITS_PER_LONG;
	n = MIN(n, nmaskbits);
	do {
		if (test_bit(i, map))
			break;
	} while (++i < n);
	return i;
}

static int bitmap_find_bit(const unsigned long *map, unsigned nmaskbits, unsigned offset)
{
	unsigned n;

	if (offset % BITS_PER_LONG != 0) {
		n = (BIT_WORD(offset) + 1) * BITS_PER_LONG;
		n = MIN(n, nmaskbits);
		while (offset < n) {
			if (test_bit(offset, map))
				return offset;
			offset++;
		}
	}
	if (offset >= nmaskbits)
		return nmaskbits;

	return offset + bitmap_find_first_bit(map + BIT_WORD(offset),
			nmaskbits - offset);;
}

static int bitmap_find_first_zero_bit(const unsigned long *map, unsigned nmaskbits)
{
	unsigned i, n;
	unsigned nmasklongs = BITS_TO_LONGS(nmaskbits);

	for (i = 0; i < nmasklongs; i++) {
		if (~map[i] != 0)
			break;
	}
	if (i == nmasklongs)
		return nmaskbits;
	i *= BITS_PER_LONG;
	n = i + BITS_PER_LONG;
	n = MIN(n, nmaskbits);
	do {
		if (!test_bit(i, map))
			break;
	} while (++i < n);
	return i;
}

static int bitmap_find_zero_bit(const unsigned long *map, unsigned nmaskbits, unsigned offset)
{
        unsigned n;

        if (offset % BITS_PER_LONG != 0) {
                n = (BIT_WORD(offset) + 1) * BITS_PER_LONG;
                n = MIN(n, nmaskbits);
                while (offset < n) {
                        if (!test_bit(offset, map))
                                return offset;
                        offset++;
                }
        }
        if (offset >= nmaskbits)
                return nmaskbits;
        return offset + bitmap_find_first_zero_bit(map + BIT_WORD(offset),
                                            nmaskbits - offset);
}

static inline int print_range(char *buf, unsigned int buflen, int a, int b)
{
	if (a == b)
		return snprintf(buf, buflen, "%d", a);
	return snprintf(buf, buflen, "%d-%d", a, b);
}

int bitmap_snprintf(char *buf, unsigned int buflen,
		const unsigned long *map, int nmaskbits)
{
	int a, b;
	unsigned int len = 0;

	if (buflen > 0)
		buf[0] = '\0';
	a = bitmap_find_bit(map, nmaskbits, 0);
	while (a < nmaskbits) {
		b = bitmap_find_zero_bit(map, nmaskbits, a + 1) - 1;
		if (len > 0)
			len += snprintf(buf + len,
					buflen > len ? buflen - len : 0, ",");
		len += print_range(buf + len,
				buflen > len ? buflen - len : 0, a, b);
		a = bitmap_find_bit(map, nmaskbits, b + 1);
	}
	return len;
}

static inline int parse_range(const char *str, int *a, int *b, char **endptr)
{
	if (!isdigit(*str))
		return -1;
	*a = *b = strtol(str, endptr, 10);
	if (**endptr == '-') {
		str = *endptr + 1;
		if (!isdigit(*str))
			return -1;
		*b = strtol(str, endptr, 10);
		if (*a > *b)
			return -1;
	}
	return 0;
}

static int bitmap_parse(const char *str, unsigned long *maskp, int nmaskbits)
{
	int a, b;
	char *endptr;

	bitmap_zero(maskp, nmaskbits);
	do {
		if (parse_range(str, &a, &b, &endptr) != 0) {
			errno = EINVAL;
			return -1;
		}
		if (b >= nmaskbits) {
			errno = ERANGE;
			return -1;
		}
		for (; a <= b; a++)
			bitmap_set_bit(a, maskp);
		if (*endptr == ',')
			endptr++;
		str = endptr;
	} while (*str != '\0');
	return 0;
}

int file_read_bitmap(const char *path, unsigned long *maskp, int nmaskbits)
{
	FILE *fp;
	char buf[4096];
	char *s;
	int success = 0;

	fp = fopen(path, "r");
	if (!fp)
		return -1;
	if (!fgets(buf, sizeof(buf), fp))
		goto out;
	s = strchr(buf, '\n');
	if (!s)
		goto out;
	if (s == buf) {
		bitmap_zero(maskp, nmaskbits);
		success = 1;
	} else {
		*s = '\0';
		if (bitmap_parse(buf, maskp, nmaskbits) == 0)
			success = 1;
	}
out:
	fclose(fp);
	if (success)
		return 0;
	errno = EIO;
	return -1;
}

static int numa_node_to_cpu(int nid, unsigned long *cpumask, int nmaskbits)
{
	DIR *d;
	struct dirent *de;
	char path[64];
	char *endp;

	sprintf(path, "/sys/devices/system/node/node%d", nid);
	d = opendir(path);
	if (!d) {
		WRITE_TRACE(DBG_FATAL, "NUMA: Failed to open %s: %m",	path);
		return -1;
	}

	while ((de = readdir(d)) != NULL) {
		int cpu;

		if (strncmp(de->d_name, "cpu", 3))
			continue;
		cpu = strtoul(de->d_name + 3, &endp, 10);
		if (!*endp && cpu >= 0 && cpu < nmaskbits)
//				test_bit(cpu, cpu_possible_mask))
			bitmap_set_bit(cpu, cpumask);
	}
	closedir(d);
	return 0;
}

static QString& remove_brackets_from_uuid(QString& uuid)
{
	uuid.remove(QChar('{'));
	uuid.remove(QChar('}'));
	return uuid;
}

int CNodeMask::get_free_cpu()
{
	unsigned long cpumask[CPUMASK_SIZE];
	unsigned int n, count = 0;

	bitmap_andnot(cpumask, avail_cpumask, used_cpumask, sizeof(cpumask));

	for (n = 0; n < sizeof(cpumask) * 8; n++)
		if (test_bit(n, cpumask))
			count++;

	return count;
}

#define MAX_NR_NODES 128
static int numa_init(NodemaskMap &nodes)
{
	int n;
	char buf[1024];
	unsigned long nodemap[MAX_NR_NODES/sizeof(unsigned long)];

	bzero(nodemap, sizeof(nodemap));
	if (file_read_bitmap("/sys/devices/system/node/possible", nodemap, MAX_NR_NODES)) {
		WRITE_TRACE(DBG_FATAL, "NUMA_init: Failed to get list of possible nodes");
		return -1;
	}

	for (n = 0; n < MAX_NR_NODES; n++) {
		if (!test_bit(n, nodemap))
			continue;

		struct CNodeMask node;
		bzero(&node, sizeof(node));

		if (numa_node_to_cpu(n, node.avail_cpumask, BITS_PER_CPUMASK))
			continue;

		bitmap_snprintf(buf, sizeof(buf), node.avail_cpumask, BITS_PER_CPUMASK);
		WRITE_TRACE(DBG_FATAL, "NUMA: register node=%d cpumask=%s", n, buf);
		nodes.insert(n, node);
	}
	return 0;
}

int CVzHelper::numa_calculate_cpumask(unsigned envid, unsigned ncpu,
		struct CNodeMask &node, struct CEnvCpumask &env_cpumask,
		unsigned long *out, int size)
{
	unsigned nbits, n;

	for (nbits = 0, n = 0; n < BITS_PER_CPUMASK; n++) {
		if (!test_bit(n, node.avail_cpumask))
			continue;
		if (test_bit(n, node.used_cpumask))
			continue;
		bitmap_set_bit(n, env_cpumask.cpumask);
		nbits++;
		if (nbits == ncpu)
			break;
	}

	if (nbits < ncpu) {
		WRITE_TRACE(DBG_FATAL, "NUMA: no free cpumask node=%u avail=%lx used=%lx nbits=%u ncpu=%u",
				 env_cpumask.nid, node.avail_cpumask[0], node.used_cpumask[0], n, ncpu);
		return -1;
	}
	bitmap_or(node.used_cpumask, node.used_cpumask, env_cpumask.cpumask,
			sizeof(node.used_cpumask));

	EnvCpumaskMap::iterator cit = s_numanodes.env_cpumask.find(envid);
	if (cit != s_numanodes.env_cpumask.end()) {
		WRITE_TRACE(DBG_FATAL, "NUMA: reset cpumask envid %u", envid);
		numa_release_cpu_mask(envid);
	}
	/* Assign all CPU on node to Vm */
	memcpy(out, node.avail_cpumask, size);

	WRITE_TRACE(DBG_FATAL, "NUMA: set cpumask envid %u node=%u usedmask=%lx cpumask=%lx",
			envid, env_cpumask.nid, node.used_cpumask[0], out[0]);

	return 0;
}

int CVzHelper::autocalculate_cpumask(unsigned envid, unsigned ncpu,
			unsigned long vm_ram, unsigned long *mask, int size)
{
	int free_cpu = 0;

	QMutexLocker locker(&s_mtxNodemask);
	if (s_numanodes.node.empty()) {
		if (numa_init(s_numanodes.node))
			return -1;
	}

	bzero(mask, size);
	NodemaskMap::const_iterator eit = s_numanodes.node.end();
	NodemaskMap::iterator nit = s_numanodes.node.end();
	for (NodemaskMap::iterator it = s_numanodes.node.begin(); it != eit; it++) {
		/* 1, select empty node */
		if (bitmap_empty(it->used_cpumask, sizeof(it->used_cpumask))) {
			nit = it;
			break;
		}
		/* 2 select node wit max  count of free cpu */
		int n = it->get_free_cpu();
		if (free_cpu < n) {
			free_cpu = n;
			nit = it;
		}
	}
	if (nit != s_numanodes.node.end()) {
		WRITE_TRACE(DBG_FATAL, "NUMA: envid %u node=%u avail=%lx used=%lx",
				envid, nit.key(), nit->avail_cpumask[0], nit->used_cpumask[0]);
		struct CEnvCpumask env_cpumask(nit.key());
		if (numa_calculate_cpumask(envid, ncpu, *nit, env_cpumask, mask, size) == 0) {
			unsigned long reserved = 0;

			reserve_hugepages(nit.key(), vm_ram, &reserved);
			env_cpumask.set_reserved_ram(reserved);

			s_numanodes.env_cpumask.insert(envid, env_cpumask);

			return 0;
		}
	}
	return -1;
}

static int read_ul(const char *fname, unsigned long long *bytes)
{
	FILE *fp;

	fp = fopen(fname, "r");
	if (fp == NULL) {
		WRITE_TRACE(DBG_FATAL, "Failed to open %s: %m", fname);
		return -1;
	}

	if (fscanf(fp, "%llu", bytes) != 1) {
		WRITE_TRACE(DBG_FATAL, "Failed to read from %s", fname);
		fclose(fp);
		return -1;
	}

	fclose(fp);
	return 0;
}

static int write_ul(const char *fname, unsigned long long bytes)
{
	FILE *fp;

	fp = fopen(fname, "w");
	if (fp == NULL) {
		WRITE_TRACE(DBG_FATAL, "Failed to open %s: %m", fname);
		return -1;
	}

	if (fprintf(fp, "%llu", bytes) < 0) {
		WRITE_TRACE(DBG_FATAL, "Failed to write to %s", fname);
		fclose(fp);
		return -1;
	}

	fclose(fp);
	return 0;

}

#define HUGEPAGES_LCK	"/var/lock/prl_hugepages.lck"
int take_hp_lock()
{
	int lckfd;
	mode_t mode = 0644;

	lckfd = open(HUGEPAGES_LCK, O_RDWR, mode);
	if (lckfd == -1 && errno == ENOENT)
		lckfd = open(HUGEPAGES_LCK, O_RDWR | O_CREAT, mode);

	if (lckfd == -1) {
		WRITE_TRACE(DBG_FATAL, "Unable to open the lock file %s: %m", HUGEPAGES_LCK);
		return -1;
	}

	if (flock(lckfd, LOCK_SH)) {
		WRITE_TRACE(DBG_FATAL, "Unable to take lock %s: %m", HUGEPAGES_LCK);
		close(lckfd);
		return -1;
	}

	return lckfd;
}

void release_hp_lock(int lckfd)
{
	if (lckfd >= 0)
		close(lckfd);
}


#define PROC_HUGEPAGES	"/sys/devices/system/node/node%1/hugepages/hugepages-2048kB/"
static void release_hugepages(int node, unsigned long reserved, bool skip_lock = false)
{
	unsigned long long nr_old, free_old, nr_new, free_new;
	int lckfd = -1;

	if (reserved == 0)
		return;

	if (!skip_lock)
		lckfd = take_hp_lock();

	if (read_ul(QSTR2UTF8(QString(PROC_HUGEPAGES "nr_hugepages").arg(node)), &nr_old) ||
			read_ul(QSTR2UTF8(QString(PROC_HUGEPAGES "free_hugepages").arg(node)), &free_old)) {
		release_hp_lock(lckfd);
		return;
	}

	if (reserved > nr_old)
		reserved = nr_old;

	write_ul(QSTR2UTF8(QString(PROC_HUGEPAGES "nr_hugepages").arg(node)), nr_old - reserved);

	if (read_ul(QSTR2UTF8(QString(PROC_HUGEPAGES "nr_hugepages").arg(node)), &nr_new) ||
			read_ul(QSTR2UTF8(QString(PROC_HUGEPAGES "free_hugepages").arg(node)), &free_new)) {
		release_hp_lock(lckfd);
		return;
	}
	release_hp_lock(lckfd);

	WRITE_TRACE(DBG_FATAL, "release hugepages: node=%d; need=%lu; old: nr=%llu, free=%llu; new: nr=%llu, free=%llu",
                                node, reserved, nr_old, free_old, nr_new, free_new);
}

static void reserve_hugepages(int node, unsigned long RamMb, unsigned long *reserved)
{
	struct stat st;
	unsigned long long nr_old, free_old, nr_new, free_new;
	unsigned long long need_pages = (((RamMb + 2) << 10) - 1) / (2 << 10);
	int lckfd = -1;

	*reserved = 0;

	/* hugepages support disabled */
	if (stat("/dev/hugetlbfs/parallels", &st)) {
		WRITE_TRACE(DBG_FATAL, "hugepages support is disabled");
		return;
	}

	lckfd = take_hp_lock();
	if (lckfd == -1)
		return;

	if (read_ul(QSTR2UTF8(QString(PROC_HUGEPAGES "nr_hugepages").arg(node)), &nr_old) ||
			read_ul(QSTR2UTF8(QString(PROC_HUGEPAGES "free_hugepages").arg(node)), &free_old))
		goto out;


	for (int i = 0; i < 10; i++) {

		/*
		 * FIXME:
		 *
		 * Never look at free pages, because if two VM's are started in parallel,
		 * then one VM can reserve some pages, but the second will think there are enough
		 * free pages already and will not reserve pages for itself.
		 */
		if (write_ul(QSTR2UTF8(QString(PROC_HUGEPAGES "nr_hugepages").arg(node)), (nr_old + need_pages)))
			break;

		if (read_ul(QSTR2UTF8(QString(PROC_HUGEPAGES "nr_hugepages").arg(node)), &nr_new) ||
				read_ul(QSTR2UTF8(QString(PROC_HUGEPAGES "free_hugepages").arg(node)), &free_new))
			break;

		WRITE_TRACE(DBG_FATAL, "reserve hugepages, try #%d: node=%d; need=%llu; old: nr=%llu, free=%llu; "
				"new: nr=%llu, free=%llu",
				i + 1, node, need_pages, nr_old, free_old, nr_new, free_new);

		*reserved = nr_new - nr_old;
		if (*reserved >= need_pages)
			goto out;
	}
	/* rollback */
	if (*reserved) {
		release_hugepages(node, *reserved, true);
		*reserved = 0;
	}

out:
	release_hp_lock(lckfd);
}

QString CVzHelper::get_cpu_mask(const SmartPtr<CVmConfiguration> &pVmConfig, bool bOvercommit)
{
	char buf[1024];
	unsigned long cpumask[CPUMASK_SIZE];

	if (!pVmConfig->getVmHardwareList()->getCpu()->getCpuMask().isEmpty())
		return pVmConfig->getVmHardwareList()->getCpu()->getCpuMask();
	if (bOvercommit)
		return QString();

	unsigned int id = Uuid::toVzid(pVmConfig->getVmIdentification()->getVmUuid());
	if (autocalculate_cpumask(id,
				pVmConfig->getVmHardwareList()->getCpu()->getNumber(),
				pVmConfig->getVmHardwareList()->getMemory()->getRamSize(),
				cpumask, sizeof(cpumask)))
		return QString();

	bitmap_snprintf(buf, sizeof(buf), cpumask, BITS_PER_CPUMASK);
	WRITE_TRACE(DBG_FATAL, "NUMA: autocalculate envid=%u cpumask=%s",
			id, buf);

	return QString(buf);
}

void CVzHelper::numa_release_cpu_mask(unsigned envid)
{
	EnvCpumaskMap::iterator cit = s_numanodes.env_cpumask.find(envid);
	if (cit == s_numanodes.env_cpumask.end())
		return;

	release_hugepages(cit->nid, cit->reserved_ram);

	NodemaskMap::iterator nit = s_numanodes.node.find(cit->nid);
	if (nit != s_numanodes.node.end()) {
		WRITE_TRACE(DBG_FATAL, "NUMA: release cpumask envid=%u node=%u cpumask=%lx",
				envid, cit->nid, cit->cpumask[0]);

		bitmap_andnot(nit->used_cpumask, nit->used_cpumask, cit->cpumask,
				sizeof(nit->used_cpumask));
	}
	s_numanodes.env_cpumask.erase(cit);
}

void CVzHelper::release_cpu_mask(const QString &uuid)
{
	QMutexLocker locker(&s_mtxNodemask);

	numa_release_cpu_mask(Uuid::toVzid(uuid));
}

namespace
{

namespace Network
{

///////////////////////////////////////////////////////////////////////////////
// struct Builder

struct Builder
{
	explicit Builder(const QString& uuid_);

	bool addv4();

	bool addv6();

	PRL_STAT_NET_TRAFFIC* getResult();

private:
	VzctlHandleWrap m_env;
	QScopedPointer<PRL_STAT_NET_TRAFFIC> m_result;

	bool fill(bool v6_);
};

Builder::Builder(const QString& uuid_) :
	m_result(new PRL_STAT_NET_TRAFFIC())
{
	int ret;
	m_env.reset(vzctl2_env_open(
		QSTR2UTF8(CVzHelper::get_ctid_by_uuid(uuid_)),
		VZCTL_CONF_SKIP_PARSE, &ret));
}

bool Builder::addv4()
{
	return fill(false);
}

bool Builder::addv6()
{
	return fill(true);
}

bool Builder::fill(bool v6_)
{
	struct vzctl_tc_netstat stat;
	if (vzctl2_get_env_tc_netstat(m_env, &stat, v6_) != 0)
	{
		WRITE_TRACE_RL(120, DBG_INFO, "Unable to get Container network statistics\n");
		return false;
	}

	for (unsigned int i = 0; i < PRL_TC_CLASS_MAX; i++)
	{
		m_result->incoming[i] += stat.incoming[i];
		m_result->outgoing[i] += stat.outgoing[i];
		m_result->incoming_pkt[i] += stat.incoming_pkt[i];
		m_result->outgoing_pkt[i] += stat.outgoing_pkt[i];
	}

	return true;
}

PRL_STAT_NET_TRAFFIC* Builder::getResult()
{
	return m_result.take();
}

} // namespace Network
} // namespace

PRL_STAT_NET_TRAFFIC *CVzHelper::get_net_stat(const QString &uuid)
{
	if (!is_vz_running())
		return NULL;

	Network::Builder b(uuid);

	if (!b.addv4() && !b.addv6())
		return NULL;

	return b.getResult();
}

int CVzHelper::update_network_classes_config(const CNetworkClassesConfig &conf)
{
	return CVzNetworkShaping::update_network_classes_config(conf);
}

int CVzHelper::get_network_classes_config(CNetworkClassesConfig &conf)
{
	return CVzNetworkShaping::get_network_classes_config(conf);
}

int CVzHelper::update_network_shaping_config(const CNetworkShapingConfig &conf)
{
	return CVzNetworkShaping::update_network_shaping_config(conf);
}

int CVzHelper::get_network_shaping_config(CNetworkShapingConfig &conf)
{
	return CVzNetworkShaping::get_network_shaping_config(conf);
}

int CVzHelper::set_rate(const QString &uuid, const CVmNetworkRates &lstRate)
{
        if (lstRate.m_lstNetworkRates.empty())
                return PRL_ERR_SUCCESS;

	return CVzNetworkShaping::set_rate(Uuid::toVzid(uuid), lstRate);
}

static int vz2prl_err(int vzret)
{
	static struct {
		int vzerr;
		int prlerr;
	} prl_error_map[] = {
		{91, PRL_ERR_VZ_OSTEMPLATE_NOT_FOUND},
		{32, PRL_ERR_CT_IS_RUNNING}
	};

	for (unsigned int i = 0; i < sizeof(prl_error_map)/sizeof(prl_error_map[0]); i++)
	{
		if (vzret == prl_error_map[i].vzerr)
			return prl_error_map[i].prlerr;
	}
	return PRL_ERR_VZ_OPERATION_FAILED;
}


void CProgressHepler::send_progress_evt(const QString &stage, int progress)
{
	if (m_notify && m_notify_obj)
		m_notify(m_notify_obj, PET_JOB_STAGE_PROGRESS_CHANGED, m_sUuid, stage, progress);
}

void CProgressHepler::process_progress_evt()
{
	FILE *fp;
	char buf[4096];
	int percent;
	char *stage, *p;

	if ((fp = fdopen(m_fd, "r")) == NULL) {
		WRITE_TRACE(DBG_FATAL, "fdopen failed: %m");
		return;
	}
	while (fgets(buf, sizeof(buf), fp) != NULL) {
		if (sscanf(buf, "percent=%d", &percent) != 1)
			continue;
		if ((stage = strstr(buf, "stage=")) == NULL)
			continue;
		stage += sizeof("stage");
		if ((p = strrchr(stage, '\n')) != NULL)
			*p = '\0';

		send_progress_evt(QString(stage), percent);
	}
}

CVzOperationHelper::~CVzOperationHelper()
{
	if (m_pProgressTask.isValid())
		m_pProgressTask->wait();

	release_snapshot_holder();
}

#define PRL_CMD_WORK_TIMEOUT  1000 * 60 * 60 * 24
#define PRL_CMD_TERMINATE_TIMEOUT 60 * 1000
PRL_RESULT CVzOperationHelper::run_prg(const char *name, const QStringList &lstArgs, bool quiet)
{
	int progress_fd[2] = {-1, -1};
	QProcess proc;
	QString args = quiet ? "" : lstArgs.join(" ").toUtf8().constData();

	if (!quiet)
		WRITE_TRACE(DBG_INFO, "%s %s", name, QSTR2UTF8(args));

	if (process_progress_evt()) {
		if (pipe(progress_fd)) {
			WRITE_TRACE(DBG_FATAL, "Can not creaet pipe %m");
			return PRL_ERR_OPERATION_FAILED;
		}
		fcntl(progress_fd[1], F_SETFD, ~FD_CLOEXEC);
		m_Envs.insert(QString("VZ_PROGRESS_FD"), QString("%1").arg(progress_fd[1]));
	}

	if (m_Envs.isEmpty()) {
		// remove LD_LIBRARY_PATH from environments of new process
		// https://jira.sw.ru/browse/PSBM-12493
		QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
		// QProcessEnvironment::remove does not works on linux
		env.insert(QString("LD_LIBRARY_PATH"), QString());
		proc.setProcessEnvironment(env);
	} else {
		proc.setProcessEnvironment(m_Envs);
		m_Envs.clear();
	}

	proc.start(name, lstArgs);
	if ( !proc.waitForStarted(-1)) {
		WRITE_TRACE(DBG_FATAL, "Can not start %s", name);
		close(progress_fd[0]);
		close(progress_fd[1]);

		return PRL_ERR_OPERATION_FAILED;
	}

	get_cleaner().add(CVzOperationCleaner::kill_process, proc.pid());

	if (process_progress_evt()) {
		close(progress_fd[1]);
		m_pProgressTask->set_param(get_uuid(), progress_fd[0]);
		m_pProgressTask->start();
	}

	bool bOk = proc.waitForFinished(PRL_CMD_WORK_TIMEOUT);
	get_cleaner().pop();

	if (!bOk)
	{
		WRITE_TRACE(DBG_FATAL, "%s tool not responding err=%d. Terminate it now.",
			name, proc.error());
		proc.terminate();
		if (!proc.waitForFinished(PRL_CMD_TERMINATE_TIMEOUT))
		{
			proc.kill();
			proc.waitForFinished(-1);
		}
		return PRL_ERR_OPERATION_FAILED;
	}
	if (proc.exitStatus() != QProcess::NormalExit) {
		WRITE_TRACE(DBG_FATAL, "'%s %s' command crashed",
			name, QSTR2UTF8(args));
		return PRL_ERR_OPERATION_FAILED;
	}
	m_Rc = proc.exitCode();
	if (m_Rc != 0)
	{
		m_sErrorMsg = proc.readAllStandardError();
		WRITE_TRACE(DBG_FATAL, "%s utility failed: %s %s [%d]\nout=%s\nerr=%s",
				name, name,
				QSTR2UTF8(args),
				proc.exitCode(),
				proc.readAllStandardOutput().data(),
				QSTR2UTF8(m_sErrorMsg));
		/* #PSBM-27689 report vzctl specific error code */
		return strcmp(name, BIN_VZCTL) == 0 ? PRL_ERR_VZCTL_OPERATION_FAILED :
				PRL_ERR_VZ_OPERATION_FAILED;
	}
	return PRL_ERR_SUCCESS;
}

int CVzHelper::get_envid_list(QStringList &lst)
{
	struct vzctl_ids *ctids = vzctl2_alloc_env_ids();
	if (ctids == NULL)
		return PRL_ERR_OUT_OF_MEMORY;

	int n = vzctl2_get_env_ids_by_state(ctids, ENV_STATUS_EXISTS);
	if (n < 0) {
		vzctl2_free_env_ids(ctids);
		return PRL_ERR_FAILURE;
	}

	for (int i = 0 ; i < n; i++) {	
		lst += ctids->ids[i];
		WRITE_TRACE(DBG_FATAL, "register CT: %s", ctids->ids[i]);
	}

	vzctl2_free_env_ids(ctids);

	return PRL_ERR_SUCCESS;
}

int CVzHelper::get_env_status_by_ctid(const QString &ctid, VIRTUAL_MACHINE_STATE &nState)
{
	vzctl_env_status_t status;
	int mask = ENV_STATUS_RUNNING | ENV_STATUS_SUSPENDED |
		ENV_STATUS_MOUNTED_FAST | ENV_STATUS_EXISTS;

	if (vzctl2_get_env_status(QSTR2UTF8(ctid), &status, mask)) {
		WRITE_TRACE(DBG_FATAL, "Failed to get Ct %s status: %s",
				QSTR2UTF8(ctid), vzctl2_get_last_error());
		return PRL_ERR_OPERATION_FAILED;
	}

	if (status.mask & ENV_STATUS_RUNNING)
		nState = VMS_RUNNING;
	else if (status.mask & ENV_STATUS_SUSPENDED)
		nState = VMS_SUSPENDED;
	else if (status.mask & ENV_STATUS_MOUNTED)
		nState = VMS_MOUNTED;
	else if (status.mask & ENV_STATUS_EXISTS)
		nState = VMS_STOPPED;
	else
		nState = VMS_UNKNOWN;

	return 0;
}

int CVzHelper::get_env_status(const QString &uuid, VIRTUAL_MACHINE_STATE &nState)
{
	QString ctid = CVzHelper::get_ctid_by_uuid(uuid);
	if (ctid.isEmpty())
		return PRL_ERR_CT_NOT_FOUND;

	return get_env_status_by_ctid(ctid, nState);
}

static unsigned int Ostemplate2Dist(const char *str)
{
	QString dist;

	// .name-ver-arch
	if (str[0] == '.')
		str++;
	dist = str;

	if (dist.startsWith("redhat-el7"))
		return PVS_GUEST_VER_LIN_REDHAT_7;
	else if (dist.startsWith("redhat"))
		return PVS_GUEST_VER_LIN_REDHAT;
	if (dist.startsWith("suse"))
		return PVS_GUEST_VER_LIN_SUSE;
	if (dist.startsWith("sles"))
		return PVS_GUEST_VER_LIN_SUSE;
	if (dist.startsWith("mandrake"))
		return PVS_GUEST_VER_LIN_MANDRAKE;
	if (dist.startsWith("debian"))
		return PVS_GUEST_VER_LIN_DEBIAN;
	if (dist.startsWith("fedora"))
		return PVS_GUEST_VER_LIN_FEDORA;
	if (dist.startsWith("xandros"))
		return PVS_GUEST_VER_LIN_XANDROS;
	if (dist.startsWith("ubuntu"))
		return PVS_GUEST_VER_LIN_UBUNTU;
	if (dist.startsWith("centos-7"))
		return PVS_GUEST_VER_LIN_CENTOS_7;
	else if (dist.startsWith("centos"))
		return PVS_GUEST_VER_LIN_CENTOS;
	if (dist.startsWith("vzlinux-7"))
		return PVS_GUEST_VER_LIN_VZLINUX_7;
	else if (dist.startsWith("vzlinux"))
		return PVS_GUEST_VER_LIN_VZLINUX;
	if (dist.startsWith("opensuse"))
		return PVS_GUEST_VER_LIN_OPENSUSE;
	return PVS_GUEST_VER_LIN_OTHER;
}

static QString get_env_xml_config_path(SmartPtr<CVmConfiguration> &pConfig)
{
	QString cfg = pConfig->getVmIdentification()->getHomePath();

	if (cfg.isEmpty())
		return QString();
	return (cfg + "/.ve.xml");
}

static CVmHardDisk *findDiskInList(CVmHardDisk *pHdd, QList<CVmHardDisk *> &lst)
{
	foreach(CVmHardDisk *p, lst)
		if (p->getUuid() == pHdd->getUuid())
			return p;
	return NULL;
}

static int merge_params(SmartPtr<CVmConfiguration> &pConfig)
{
	QString xml_conf = get_env_xml_config_path(pConfig);
	QFile file(xml_conf);
	if (!file.exists())
	        return 0;
	SmartPtr<CVmConfiguration> pVmConfig(new CVmConfiguration);

	PRL_RESULT res = pVmConfig->loadFromFile(&file, true);
	if (PRL_FAILED(res))
		return res;

	// Merge FireWalls
	QList<CVmGenericNetworkAdapter* > lstNet = pConfig->getVmHardwareList()->m_lstNetworkAdapters;
	foreach(CVmGenericNetworkAdapter* pNet, lstNet)
	{
		if (pNet->isVenetDevice())
			continue;
		foreach(CVmGenericNetworkAdapter* pVmNet, pVmConfig->getVmHardwareList()->m_lstNetworkAdapters)
		{
			if (pNet->getIndex() == pVmNet->getIndex()) {
				CVmNetFirewall* pFw = new CVmNetFirewall(pVmNet->getFirewall());
				pNet->setFirewall(pFw);
				break;
			}
		}
	}
	/* merge remote display settings (https://jira.sw.ru/browse/PSBM-11677) */
	CVmRemoteDisplay *pRemoteDisplay =
		new CVmRemoteDisplay(pVmConfig->getVmSettings()->getVmRemoteDisplay());
	pConfig->getVmSettings()->setVmRemoteDisplay(pRemoteDisplay);

	/* updete hdd indexes */
	foreach(CVmHardDisk *pVmHdd, pVmConfig->getVmHardwareList()->m_lstHardDisks) {
		CVmHardDisk *pHdd;

		if ((pHdd = findDiskInList(pVmHdd, pConfig->getVmHardwareList()->m_lstHardDisks))) {
			pHdd->setIndex(pVmHdd->getIndex());
			pHdd->setStackIndex(pVmHdd->getStackIndex());
			pHdd->setInterfaceType(pVmHdd->getInterfaceType());
		}
	}

	return 0;
}

static void conf_add_disk_entry(SmartPtr<CVmConfiguration> &pConfig,
		struct vzctl_disk_param &disk, int idx)
{
	CVmHardDisk *pDisk = new CVmHardDisk;
	// Kbytes -> Mbytes
	unsigned long long size = disk.size >> 10;
	if (size == 0)
		size = 1;

	pDisk->setIndex(idx);
	pDisk->setEnabled(disk.enabled != VZCTL_PARAM_OFF);
	pDisk->setSize(size);
	pDisk->setInterfaceType(PMS_UNKNOWN_DEVICE);

	if (disk.use_device) {
		pDisk->setEmulatedType(PDT_USE_REAL_DEVICE);
	} else {
		pDisk->setEmulatedType(pConfig->getCtSettings()->
			getLayout() == VZCTL_LAYOUT_5 ?
				PDT_USE_IMAGE_FILE : PDT_USE_FILE_SYSTEM);
	}
	pDisk->setDiskType(PHD_EXPANDING_HARD_DISK);
	pDisk->setUserFriendlyName(disk.path ? disk.path : "");
	pDisk->setSystemName(pDisk->getUserFriendlyName());
	pDisk->setUuid(disk.uuid);
	if (disk.mnt != NULL)
		 pDisk->setMountPoint(disk.mnt);
	pDisk->setAutoCompressEnabled(disk.autocompact != VZCTL_PARAM_OFF);
	if (disk.storage_url != NULL) {
		QUrl url(disk.storage_url);
		if (url.isValid())
			pDisk->setStorageURL(url);
	}
	pConfig->getVmHardwareList()->addHardDisk(pDisk);
}

static int conf_get_disk(SmartPtr<CVmConfiguration> &pConfig, vzctl_env_param_ptr env_param)
{
	struct vzctl_disk_param disk;

	if (pConfig->getCtSettings()->getLayout() == VZCTL_LAYOUT_5)
	{
		int idx = 0;

		vzctl_disk_iterator it = NULL;
		while ((it = vzctl2_env_get_disk(env_param, it)) != NULL) {
			vzctl2_env_get_disk_param(it, &disk, sizeof(disk));
			conf_add_disk_entry(pConfig, disk, idx++);
		}
	} else {
		vzctl_2UL_res diskspace;
		if (vzctl2_env_get_diskspace(env_param, &diskspace) == 0) {
			memset(&disk, 0, sizeof(disk));
			disk.size = diskspace.b;
			disk.enabled = VZCTL_PARAM_ON;

			conf_add_disk_entry(pConfig, disk, 0);
		}
	}
	return 0;
}

static QStringList getEnvRawParam()
{
	return (QStringList() << "BINDMOUNT" << "ORIGIN_SAMPLE" << "PCI"
		<< "NETDEV" << "DEVNODES" << "JOURNALED_QUOTA" << "TEMPLATES");
}

static int get_vm_config(vzctl_env_handle_ptr h,
		SmartPtr<CVmConfiguration> &pConfig)
{
	const char *data;
	int ret;
	unsigned long ul;
	int val;
	char buf[512];

	vzctl_env_param_ptr env_param = vzctl2_get_env_param(h);
	if (env_param == NULL) {
		WRITE_TRACE(DBG_FATAL, "vzctl2_get_env_param: %s",
				vzctl2_get_last_error());
		return PRL_ERR_OPERATION_FAILED;
	}

	pConfig->setVmType(PVT_CT);

	// VmIdentification
	{
		CVmIdentification *p = pConfig->getVmIdentification();

		p->setCtId(vzctl2_env_get_ctid(h));

		QString uuid;
		if (vzctl2_env_get_uuid(env_param, &data) == 0 && data != NULL)
			uuid = data;

		// Set uuid in the '{uuid}' format
		if (!uuid.isEmpty())
			p->setVmUuid(QString("{%1}").arg(uuid));
		// VE_PRIVATE
		if ((ret = vzctl2_env_get_ve_private_path(env_param, &data)) == 0)
			p->setHomePath(UTF8_2QSTR(data));
		// NAME
		if (vzctl2_get_name(h, &data) == 0)
			p->setVmName(UTF8_2QSTR(data));
		else
			p->setVmName(p->getCtId());
		// Uptime
		quint64 uptime, start_date;
		if ((ret = vzctl2_env_get_uptime(h, &uptime, &start_date)) == 0)
		{
			p->setVmUptimeInSeconds(uptime);
			p->setVmUptimeStartDateTime(QDateTime::fromTime_t(start_date));
		}
	}

	// VmSettings
	{
		CVmCommonOptions *pCo = pConfig->getVmSettings()->getVmCommonOptions();
		// DESCRIPTION
		if ((ret = vzctl2_env_get_description(env_param, &data)) == 0)
			pCo->setVmDescription(UTF8_2QSTR(data));

		if ((ret = vzctl2_env_get_ostemplate(env_param, &data)) == 0)
		{
			pCo->setOsType(PVS_GUEST_TYPE_LINUX);
			pCo->setOsVersion(Ostemplate2Dist(data));
		}
		// ONBOOT
		CVmStartupOptions *pSo = pConfig->getVmSettings()->getVmStartupOptions();
		int enabled;
		if ((ret = vzctl2_env_get_autostart(env_param, &enabled)) == 0)
			pSo->setAutoStart(enabled ? PAO_VM_START_ON_LOAD : PAO_VM_START_MANUAL);

		// BOOTORDER
		if (vzctl2_env_get_bootorder(env_param, &ul) == 0)
			 pSo->setBootOrderPrio(ul);

		// IOPRIO
		CVmRunTimeOptions *pRo = pConfig->getVmSettings()->getVmRuntimeOptions();
		int prio;
		if ((ret = vzctl2_env_get_ioprio(env_param, &prio)) == 0)
			pRo->setIoPriority(prio);

		// IOLIMIT
		unsigned int limit;
		if ((ret = vzctl2_env_get_iolimit(env_param, &limit)) == 0)
		{
			CVmIoLimit *pIoLimit = new CVmIoLimit(PRL_IOLIMIT_BS, limit);
			pRo->setIoLimit(pIoLimit);
		}
		// IOPS LIMIT
		if ((ret = vzctl2_env_get_iopslimit(env_param, &limit)) == 0)
			pRo->setIopsLimit(limit);

		vzctl_env_type type;
		// VE_TYPE
		if ((ret = vzctl2_env_get_type(env_param, &type)) == 0)
			pCo->setTemplate(type == VZCTL_ENV_TYPE_TEMPLATE ? true : false);
	}
	// CtSettings
	{
		// OSTEMPLATE
		CCtSettings *pCt = pConfig->getCtSettings();
		if (vzctl2_env_get_ostemplate(env_param, &data) == 0)
			pCt->setOsTemplate(UTF8_2QSTR(data));

		if (vzctl2_env_get_apptemplates(env_param, &data) == 0) {
			QStringList tmpl_set = UTF8_2QSTR(data).split(" ",
					QString::SkipEmptyParts);
			pCt->setAppTemplate(tmpl_set);
		}

		CVmMemory *pMem = pConfig->getVmHardwareList()->getMemory();
		if (vzctl2_env_get_ramsize(env_param, &ul) == 0)
			pMem->setRamSize(ul);

		struct vzctl_mem_guarantee g;
		if (vzctl2_env_get_memguarantee(env_param, &g) == 0) {
			pMem->setMemGuaranteeType(
				g.type == VZCTL_MEM_GUARANTEE_AUTO ?
					PRL_MEMGUARANTEE_AUTO :
					PRL_MEMGUARANTEE_PERCENTS
				);
			pMem->setMemGuarantee(g.value);
		}
		pMem->setEnableHotplug(true);

		/* Video memory is not used for Ct */
		pConfig->getVmHardwareList()->getVideo()->setMemorySize(0);
		// VE_ROOT
		if ((ret = vzctl2_env_get_ve_root_path(env_param, &data)) == 0)
			pCt->setMountPath(UTF8_2QSTR(data));
		if (vzctl2_env_get_layout(env_param, &val) == 0)
			pCt->setLayout(val);
		if (vzctl2_env_get_cap(env_param, &ul) == 0)
			pCt->setCapabilitiesMask(ul);

		unsigned mode;
		if (vzctl2_env_get_netfilter(env_param, &mode) == 0)
			pCt->setNetfilterMode( static_cast<PRL_NETFILTER_MODE>(mode) );

		struct vzctl_feature_param f;
		if (vzctl2_env_get_features(env_param, &f) == 0) {
			pCt->setFeaturesOnMask(f.on);
			pCt->setFeaturesOffMask(f.off);
		}
		// UB resources
		struct vzctl_2UL_res res;
		if (vzctl2_env_get_ub_resource(env_param, VZCTL_PARAM_SWAPPAGES, &res) == 0) {
			CCtResource *pRes = new CCtResource;

			pRes->setResourceId(PCR_SWAPPAGES);
			pRes->setBarrier(res.b);
			pRes->setLimit(res.l);
			pCt->addResource(pRes);
		}
		// QUOTAUGIDLIMIT
		if (vzctl2_env_get_quotaugidlimit(env_param, &ul) == 0)
		{
			CCtResource *pRes = new CCtResource;

			pRes->setResourceId(PCR_QUOTAUGIDLIMIT);
			pRes->setBarrier(ul);
			pRes->setLimit(ul);
			pCt->addResource(pRes);
		}

		QStringList l;
		foreach(QString s, getEnvRawParam()) {
			if (vzctl2_env_get_param(h, QSTR2UTF8(s), &data) == 0 && data != NULL)
				l += QString("%1=%2").arg(s).arg(data);
		}

		pCt->setRawParam(l);
	}
	// Hardware
	{
		struct vzctl_cpulimit_param res;

		CVmCpu *pCpu = pConfig->getVmHardwareList()->getCpu();
		// CPULIMIT
		if (vzctl2_env_get_cpulimit(env_param, &res) == 0) {
			PRL_CPULIMIT_DATA cpulimit;
			if (res.type == VZCTL_CPULIMIT_MHZ)
				cpulimit.type = PRL_CPULIMIT_MHZ;
			else
				cpulimit.type = PRL_CPULIMIT_PERCENTS;
			cpulimit.value = res.limit;

			pCpu->setCpuLimitData(&cpulimit);
		}
		pCpu->setEnableHotplug(true);

		// CPUUNITS
		if (vzctl2_env_get_cpuunits(env_param, &ul) == 0)
			pCpu->setCpuUnits(ul);
		// CPUS
		if (vzctl2_env_get_cpu_count(env_param, &ul) == 0 && (ul != 0))
			pCpu->setNumber(ul);
		else
			//zero mean unlimited
			//error - set unlimited
			pCpu->setNumber(PRL_CPU_UNLIMITED);
		// CPUMASK
		if (vzctl2_env_get_cpumask(env_param, buf, sizeof(buf)) == 0)
			pCpu->setCpuMask(buf);

		if (vzctl2_env_get_nodemask(env_param, buf, sizeof(buf)) == 0)
			pCpu->setNodeMask(buf);

		// DISK
		conf_get_disk(pConfig, env_param);
		// IP_ADDRESS
		QList<QString> ips;
		vzctl_ip_iterator it = NULL;
		while ((it = vzctl2_env_get_ipaddress(env_param, it)) != NULL) {
			if (vzctl2_env_get_ipstr(it, buf, sizeof(buf)) == 0)
				ips += QString(buf);
		}

		// SEARCHDOMAIN
		QList<QString> searchdomains;
		vzctl_str_iterator itsearchdomain = NULL;
		while ((itsearchdomain = vzctl2_env_get_searchdomain(env_param, itsearchdomain)) != NULL)
			searchdomains += vzctl2_env_get_str_param(itsearchdomain);
		pConfig->getVmSettings()->getGlobalNetwork()->setSearchDomains(searchdomains);

		// NAMESERVER
		QList<QString> nameservers;
		vzctl_str_iterator itnamererver = NULL;
		while ((itnamererver = vzctl2_env_get_nameserver(env_param, itnamererver)) != NULL)
			nameservers += vzctl2_env_get_str_param(itnamererver);
		pConfig->getVmSettings()->getGlobalNetwork()->setDnsIPAddresses(nameservers);

		// APPLY_IPONLY
		if (vzctl2_env_get_apply_iponly(env_param, &val) == 0)
			pConfig->getVmSettings()->getGlobalNetwork()->setAutoApplyIpOnly(!!val);

		CVmGenericNetworkAdapter *pVenet = new CVmGenericNetworkAdapter;

		/* venet device always exists, add it with -1 idx */
		pVenet->setIndex((unsigned int) -1);
		pVenet->setSystemName(QString("venet0"));
		pVenet->setEnabled(true);
		pVenet->setNetAddresses(ips);
		pVenet->setEmulatedType(PNA_ROUTED);
		pVenet->setDnsIPAddresses(nameservers);
		pVenet->setSearchDomains(searchdomains);

		pConfig->getVmHardwareList()->addNetworkAdapter(pVenet);

		// NETIF
		vzctl_veth_dev_iterator itdev = NULL;
		struct vzctl_veth_dev_param dev;

		while ((itdev = vzctl2_env_get_veth(env_param, itdev)) != NULL) {
			int idx;

			bzero(&dev, sizeof(dev));
			vzctl2_env_get_veth_param(itdev, &dev, sizeof(dev));

			if (dev.dev_name_ve == NULL) {
				WRITE_TRACE(DBG_FATAL, "vzctl2_env_get_veth_param: Unable to get veth device name");
				continue;
			}

			QList<QString> ips;
			vzctl_ip_iterator it = NULL;
			while ((it = vzctl2_env_get_veth_ipaddress(itdev, it)) != NULL) {
				if (vzctl2_env_get_ipstr(it, buf, sizeof(buf)) == 0)
					ips += QString(buf);
			}

			if (sscanf(dev.dev_name_ve , "%*[^0-9]%d", &idx) != 1) {
				WRITE_TRACE(DBG_FATAL, "Unable to get veth index from '%s'",
						dev.dev_name_ve );
				continue;
			}

			CVmGenericNetworkAdapter *pNet = new CVmGenericNetworkAdapter;

			pNet->setIndex(idx);
			pNet->setSystemName(QString(dev.dev_name_ve));
			if (dev.dev_name)
				pNet->setHostInterfaceName(QString(dev.dev_name));
			pNet->setEnabled(true);
			pNet->setAutoApply(dev.configure_mode != VZCTL_VETH_CONFIGURE_NONE);
			if (dev.network != NULL)
				pNet->setVirtualNetworkID(QString(dev.network));
			QString mac;
			if (dev.mac_ve != NULL) {
				mac = QString(dev.mac_ve).remove(QChar(':'));
				pNet->setMacAddress(mac);
			}
			if (dev.mac != NULL) {
				mac = QString(dev.mac).remove(QChar(':'));
				pNet->setHostMacAddress(mac);
			}
			pNet->getPktFilter()->setPreventMacSpoof(!dev.allow_mac_spoof);
			pNet->getPktFilter()->setPreventIpSpoof(!dev.allow_ip_spoof);
			if (!ips.isEmpty())
				pNet->setNetAddresses(ips);
			if (dev.dhcp)
				pNet->setConfigureWithDhcpEx(true);
			if (dev.dhcp6)
				pNet->setConfigureWithDhcpIPv6Ex(true);
			if (dev.gw != NULL)
				pNet->setDefaultGateway(QString(dev.gw));
			if (dev.gw6 != NULL)
				pNet->setDefaultGatewayIPv6(QString(dev.gw6));

			/* Linux doesn't allow to specify per-adapter
			 * nameservers - global one is effective for all */
			pNet->setDnsIPAddresses(nameservers);
			pNet->setSearchDomains(searchdomains);

			pConfig->getVmHardwareList()->addNetworkAdapter(pNet);
		}
		// RATE
		CVmNetworkRates *pRates = pConfig->getVmSettings()->getGlobalNetwork()->getNetworkRates();

		vzctl_rate_iterator itrate = NULL;
		while ((itrate = vzctl2_env_get_rate(env_param, itrate)) != NULL) {
			struct vzctl_rate_param param;

			bzero(&param, sizeof(param));
			ret = vzctl2_env_get_rate_param(itrate, &param);
			if (ret) {
				WRITE_TRACE(DBG_FATAL, "vzctl2_env_get_rate_param: %s [%d]",
						vzctl2_get_last_error(), ret);
				continue;
			}
			CVmNetworkRate *pRate = new CVmNetworkRate;
			pRate->setClassId(param.net_class);
			pRate->setRate(param.rate);

			pRates->m_lstNetworkRates += pRate;
		}
		// RATEBOUND
		int isRatebound = 0;
		vzctl2_env_get_ratebound(env_param, &isRatebound);
		pRates->setRateBound(isRatebound ? true: false);

		//HOSTNAME
		const char *hostname;
		if (vzctl2_env_get_hostname(env_param, &hostname) == 0)
			pConfig->getVmSettings()->getGlobalNetwork()->setHostName(QString(hostname));

		// AUTOCOMPACT
		int enabled;
		if (vzctl2_env_get_autocompact(env_param, &enabled) == 0)
			pConfig->getVmSettings()->getVmAutoCompress()->setEnabled(enabled);

		// High Availability Cluster
		CVmHighAvailability *pHA = pConfig->getVmSettings()->getHighAvailability();

		int ha_enabled;
		if (vzctl2_env_get_ha_enable(env_param, &ha_enabled) == 0)
			pHA->setEnabled(ha_enabled ? true : false);

		unsigned long ha_prio;
		if (vzctl2_env_get_ha_prio(env_param, &ha_prio) == 0)
			pHA->setPriority((unsigned int)ha_prio);
	}
	// append additian parametes
	merge_params(pConfig);

//	PUT_RAW_MESSAGE(QSTR2UTF8(pConfig->toString()));
	return 0;
}

QString CVzHelper::getVzPrivateDir()
{
	int ret;
	QString sVzPrivate;
	SmartPtr<CVmConfiguration> pConfig;

	pConfig = get_env_config_from_file(VZ_GLOBAL_CFG, ret);
	if (pConfig) {
		sVzPrivate = pConfig->getVmIdentification()->getHomePath();
		// remove $VEID
		if (sVzPrivate.endsWith(QString("/0")))
			sVzPrivate.chop(2);
	}

	if (sVzPrivate.isEmpty())
		sVzPrivate = "/vz/private";

	return sVzPrivate;
}

int CVzHelper::get_vz_config_param(const char *param, QString &out)
{
	int ret;

	VzctlHandleWrap h(vzctl2_env_open_conf(0, VZ_GLOBAL_CFG, VZCTL_CONF_SKIP_GLOBAL, &ret));
	if (h == NULL)
	{
		WRITE_TRACE(DBG_FATAL, "failed vzctl2_env_open_conf %s [%d]",
			vzctl2_get_last_error(), ret);
		return -1;
	}
	const char *res = NULL;
	vzctl2_env_get_param(h, param, &res);

	if (res == NULL)
		return 1;

	out = res;
	return 0;
}

SmartPtr<CVmConfiguration> CVzHelper::get_env_config_by_ctid(const QString &ctid)
{
	int ret;
	SmartPtr<CVmConfiguration> pConfig;

	VzctlHandleWrap h(vzctl2_env_open(QSTR2UTF8(ctid), 0, &ret));
	if (h == NULL)
	{
		WRITE_TRACE(DBG_FATAL, "failed vzctl2_env_open ctid=%s %s [%d]",
			QSTR2UTF8(ctid), vzctl2_get_last_error(), ret);
		return pConfig;
	}

	pConfig = SmartPtr<CVmConfiguration> (new CVmConfiguration);

	ret = get_vm_config(h, pConfig);

	return pConfig;
}

SmartPtr<CVmConfiguration> CVzHelper::get_env_config(const QString &uuid)
{
	SmartPtr<CVmConfiguration> pConfig;

	QString ctid = CVzHelper::get_ctid_by_uuid(uuid);
	if (ctid.isEmpty()) {
		WRITE_TRACE(DBG_FATAL, "get_env_config: invalid ctid '%s'",
				QSTR2UTF8(uuid));
		return pConfig;
	}

	return get_env_config_by_ctid(ctid);
}

SmartPtr<CVmConfiguration> CVzHelper::get_env_config_from_file(
		const QString &sFile, int &err,
		int layout, bool use_relative_path)
{
	int ret;
	SmartPtr<CVmConfiguration> pConfig;

	if (!QFile::exists(sFile)) {
		WRITE_TRACE(DBG_FATAL, "Sample Configuration file '%s' does not exist",
				QSTR2UTF8(sFile));
		err = PRL_ERR_SAMPLE_CONFIG_NOT_FOUND;
		return pConfig;
	}

	int flags = VZCTL_CONF_SKIP_GLOBAL |
			use_relative_path ? VZCTL_CONF_USE_RELATIVE_PATH : 0;
	VzctlHandleWrap h(vzctl2_env_open_conf(NULL, QSTR2UTF8(sFile), flags, &ret));
	if (h == NULL) {
		WRITE_TRACE(DBG_FATAL, "failed vzctl2_env_open_conf %s [%d]",
			vzctl2_get_last_error(), ret);
		err = PRL_ERR_PARSE_VM_CONFIG;
		return pConfig;
	}

	if (layout) {
		if (vzctl2_env_set_layout(vzctl2_get_env_param(h), layout, flags)) {
			WRITE_TRACE(DBG_FATAL, "failed vzctl2_env_set_layout: %s",
					vzctl2_get_last_error());
			err = PRL_ERR_PARSE_VM_CONFIG;
			return pConfig;
		}
	}

	pConfig = SmartPtr<CVmConfiguration> (new CVmConfiguration);

	err = get_vm_config(h, pConfig);

	return pConfig;
}

static QString get_configsample_file_name(const QString &name)
{
	char fname[1024];

	snprintf(fname, sizeof(fname), VZ_ENV_CONF_SAMPLE, QSTR2UTF8(name));

	return QString(fname);
}

SmartPtr<CVmConfiguration> CVzHelper::get_env_config_sample(const QString &name, int &err)
{
	return get_env_config_from_file(
			get_configsample_file_name(name.isEmpty() ? QString("vswap.512MB") : name), err);
}

static CVmGenericNetworkAdapter *get_venet_device(SmartPtr<CVmConfiguration> &pConfig)
{
	QList<CVmGenericNetworkAdapter* > lstNet = pConfig->getVmHardwareList()->m_lstNetworkAdapters;
	foreach(CVmGenericNetworkAdapter* pNet, lstNet) {
		if (pNet->isVenetDevice())
			return pNet;
	}
	return NULL;
}

int CVzOperationHelper::set_env_name(const QString &uuid, const QString &name)
{
	int ret;

	QString ctid = CVzHelper::get_ctid_by_uuid(uuid);
	if (ctid.isEmpty())
		return PRL_ERR_CT_NOT_FOUND;

	VzctlHandleWrap h(vzctl2_env_open(QSTR2UTF8(ctid), 0, &ret));
	if (h == NULL)
	{
		WRITE_TRACE(DBG_FATAL, "failed vzctl2_env_open ctid=%s %s",
			QSTR2UTF8(uuid), vzctl2_get_last_error());
		return PRL_ERR_OPERATION_FAILED;
	}

	WRITE_TRACE(DBG_INFO, "Change Container %s name %s",
			QSTR2UTF8(uuid), QSTR2UTF8(name));
	if (vzctl2_set_name(h, QSTR2UTF8(name))) {
		WRITE_TRACE(DBG_FATAL, "vzctl2_set_name failed: name='%s': %s",
				QSTR2UTF8(name), vzctl2_get_last_error());
		return PRL_ERR_OPERATION_FAILED;
	}

	return PRL_ERR_SUCCESS;
}

static const char *get_veth_ifname(CVmGenericNetworkAdapter *pAdapter, char *buf, int len)
{
	if (pAdapter->getSystemName().isEmpty())
		snprintf(buf, len, "eth%d",
				pAdapter->getIndex());
	else
		snprintf(buf, len, "%s",
				QSTR2UTF8(pAdapter->getSystemName()));
	return buf;
}

static CVmGenericNetworkAdapter* findNetAdapter(QList<CVmGenericNetworkAdapter* > &lstAdapters,
		CVmGenericNetworkAdapter *pAdapter)
{
	char buf[128];
	QString ifname = get_veth_ifname(pAdapter, buf, sizeof(buf));

	foreach(CVmGenericNetworkAdapter* p, lstAdapters) {
		if (!p->isVenetDevice() && ifname == p->getSystemName())
			return p;
	}
	return NULL;
}

/* we need to analyze all DNS servers and searchdomains changes, because
 * per-adapter settings for Linux containers are not supported.
 *
 * Thus we have to:
 *	1) report user global settings if it were per-adapter (copy them to each
 *	   adapter section).
 *	2) analyze per-adapter settings change and apply difference to the global
 *	   parameter.
 * This function populates two lists - one for addition and another for removal
 * of settings. Removal list will be subtracted from the addition one after we
 * pass through all adapters and global settings.
 */
static void populateDnsListsChanges(QList<QString> lstAdapterNew,
	QList<QString> lstAdapterOld, QList<QString> &lstAdd, QList<QString> &lstDel)
{
	// go through new servers, check that absent previously
	foreach(QString str, lstAdapterNew) {
		if (lstAdapterOld.indexOf(str) == -1)
			// new nameserver, add to the list
			lstAdd.append(str);
	}
	// go through old servers, check that absent now
	foreach(QString str, lstAdapterOld) {
		if (lstAdapterNew.indexOf(str) == -1)
			// removed nameserver, add to the removal list
			lstDel.append(str);
	}
}

static bool is_ub_resource_changed(const CCtResource *pResOld, CCtResource *pRes)
{
	if ((pRes != NULL && pResOld == NULL) ||
			(pResOld != NULL && pRes != NULL &&
			 (pRes->getBarrier() != pResOld->getBarrier() ||
			  pRes->getLimit() != pResOld->getLimit())))
		return true;
	return false;
}

static int fill_env_param(vzctl_env_handle_ptr h, vzctl_env_param_ptr new_param,
		SmartPtr<CVmConfiguration> &pConfig, SmartPtr<CVmConfiguration> &pOldConfig)
{
	int ret;

	QString olddesc = pOldConfig->getVmSettings()->getVmCommonOptions()->getVmDescription();
	QString desc = pConfig->getVmSettings()->getVmCommonOptions()->getVmDescription();
	if (olddesc != desc)
		 vzctl2_env_set_description(new_param, QSTR2UTF8(desc));

	const CVmMemory *old_mem = pOldConfig->getVmHardwareList()->getMemory();
	const CVmMemory *mem = pConfig->getVmHardwareList()->getMemory();
	if (old_mem->getRamSize() != mem->getRamSize() &&
			mem->getRamSize() != 0)
		vzctl2_env_set_ramsize(new_param, mem->getRamSize());

	if (old_mem->getMemGuaranteeType() != mem->getMemGuaranteeType() ||
			old_mem->getMemGuarantee() != mem->getMemGuarantee())
	{
		struct vzctl_mem_guarantee g = vzctl_mem_guarantee();

		g.type = mem->getMemGuaranteeType() == PRL_MEMGUARANTEE_AUTO ?
				VZCTL_MEM_GUARANTEE_AUTO : VZCTL_MEM_GUARANTEE_PCT,
		g.value = mem->getMemGuarantee();

		vzctl2_env_set_memguarantee(new_param, &g);
	}

	// UB resoureces
	CCtResource *pResOld = pOldConfig->getCtSettings()->getResource(PCR_SWAPPAGES);
	CCtResource *pRes = pConfig->getCtSettings()->getResource(PCR_SWAPPAGES);
	struct vzctl_2UL_res res;
	if (is_ub_resource_changed(pResOld, pRes)) {
		res.b = pRes->getBarrier();
		res.l = pRes->getLimit();
		vzctl2_env_set_ub_resource(new_param, VZCTL_PARAM_SWAPPAGES, &res);
	}
	// QUOTAUGIDLIMIT
	pResOld = pOldConfig->getCtSettings()->getResource(PCR_QUOTAUGIDLIMIT);
	pRes = pConfig->getCtSettings()->getResource(PCR_QUOTAUGIDLIMIT);
	if (is_ub_resource_changed(pResOld, pRes))
		vzctl2_env_set_quotaugidlimit(new_param, pRes->getLimit());

	PRL_CPULIMIT_DATA oldcpulimit, cpulimit;
	pOldConfig->getVmHardwareList()->getCpu()->getCpuLimitData(&oldcpulimit);
	pConfig->getVmHardwareList()->getCpu()->getCpuLimitData(&cpulimit);

	if (oldcpulimit.value != cpulimit.value ||
	    oldcpulimit.type != cpulimit.type)

	{
		struct vzctl_cpulimit_param res;

		if (cpulimit.type == PRL_CPULIMIT_PERCENTS)
			res.type = VZCTL_CPULIMIT_PCT;
		else if (cpulimit.type == PRL_CPULIMIT_PERCENTS_TO_MHZ)
			res.type = VZCTL_CPULIMIT_PCT_TO_MHZ;
		else
			res.type = VZCTL_CPULIMIT_MHZ;
		res.limit = cpulimit.value;

		vzctl2_env_set_cpulimit(new_param, &res);
	}

	// Capabilities.
	unsigned long old_capmask = pOldConfig->getCtSettings()->getCapabilitiesMask();
	unsigned long capmask = pConfig->getCtSettings()->getCapabilitiesMask();
	if (old_capmask != capmask && capmask != 0)
		vzctl2_env_set_cap(h, new_param, capmask);

	// Netfilter mode.
	PRL_NETFILTER_MODE old_netfilter_mode = pOldConfig->getCtSettings()->getNetfilterMode();
	PRL_NETFILTER_MODE netfilter_mode = pConfig->getCtSettings()->getNetfilterMode();
	if (old_netfilter_mode != netfilter_mode && netfilter_mode != PCNM_NOT_SET)
		vzctl2_env_set_netfilter(new_param, netfilter_mode);

	struct vzctl_feature_param f;
	f.on = pConfig->getCtSettings()->getFeaturesOnMask();
	f.off = pConfig->getCtSettings()->getFeaturesOffMask();
	if (pOldConfig->getCtSettings()->getFeaturesOnMask() != f.on ||
			pOldConfig->getCtSettings()->getFeaturesOffMask() != f.off)
		vzctl2_env_set_features(new_param, &f);

	unsigned long oldunits = pOldConfig->getVmHardwareList()->getCpu()->getCpuUnits();
	unsigned long units = pConfig->getVmHardwareList()->getCpu()->getCpuUnits();
	if (oldunits != units)
		vzctl2_env_set_cpuunits(new_param, units);

	unsigned long oldcount = pOldConfig->getVmHardwareList()->getCpu()->getNumber();
	unsigned long count = pConfig->getVmHardwareList()->getCpu()->getNumber();
	if (oldcount != count)
		vzctl2_env_set_cpu_count(new_param, count);

	if (pOldConfig->getVmHardwareList()->getCpu()->getCpuMask() !=
			pConfig->getVmHardwareList()->getCpu()->getCpuMask())
		vzctl2_env_set_cpumask(new_param, QSTR2UTF8(pConfig->getVmHardwareList()->getCpu()->getCpuMask()));

	if (pOldConfig->getVmHardwareList()->getCpu()->getNodeMask() !=
			pConfig->getVmHardwareList()->getCpu()->getNodeMask())
		vzctl2_env_set_nodemask(new_param, QSTR2UTF8(pConfig->getVmHardwareList()->getCpu()->getNodeMask()));

	PRL_VM_AUTOSTART_OPTION oldmode = pOldConfig->getVmSettings()->getVmStartupOptions()->getAutoStart();
	PRL_VM_AUTOSTART_OPTION mode = pConfig->getVmSettings()->getVmStartupOptions()->getAutoStart();
	if (oldmode != mode)
		vzctl2_env_set_autostart(new_param, (mode == PAO_VM_START_MANUAL ? 0 : 1));

	bool oldtmpl = pOldConfig->getVmSettings()->getVmCommonOptions()->isTemplate();
	bool tmpl = pConfig->getVmSettings()->getVmCommonOptions()->isTemplate();
	if (oldtmpl != tmpl)
		vzctl2_env_set_type(new_param, tmpl ? VZCTL_ENV_TYPE_TEMPLATE : VZCTL_ENV_TYPE_REGULAR);

	unsigned int oldioprio = pOldConfig->getVmSettings()->getVmRuntimeOptions()->getIoPriority();
	unsigned int ioprio = pConfig->getVmSettings()->getVmRuntimeOptions()->getIoPriority();
	if (oldioprio != ioprio)
		vzctl2_env_set_ioprio(new_param, (int) ioprio);

	CVmIoLimit *pNewIoLimit = pConfig->getVmSettings()->getVmRuntimeOptions()->getIoLimit();
	CVmIoLimit *pOldIoLimit = pOldConfig->getVmSettings()->getVmRuntimeOptions()->getIoLimit();
	if (pNewIoLimit && (!pOldIoLimit || ! pNewIoLimit->operator==(*pOldIoLimit)))
		vzctl2_env_set_iolimit(new_param, pNewIoLimit->getIoLimitValue());

	unsigned int oldlimit = pOldConfig->getVmSettings()->getVmRuntimeOptions()->getIopsLimit();
	unsigned int limit = pConfig->getVmSettings()->getVmRuntimeOptions()->getIopsLimit();
	if (oldlimit != limit)
		vzctl2_env_set_iopslimit(new_param, limit);

	QList<QString> lstSearchdomain, lstDelSearchdomain, lstNameserver, lstDelNameserver;
	populateDnsListsChanges(pConfig->getVmSettings()->getGlobalNetwork()->getDnsIPAddresses(),
		pOldConfig->getVmSettings()->getGlobalNetwork()->getDnsIPAddresses(),
		lstNameserver, lstDelNameserver);
	populateDnsListsChanges(pConfig->getVmSettings()->getGlobalNetwork()->getSearchDomains(),
		pOldConfig->getVmSettings()->getGlobalNetwork()->getSearchDomains(),
		lstSearchdomain, lstDelSearchdomain);

	// NETIF
	bool bChanged = false;
	char ifname[256];
	QList<CVmGenericNetworkAdapter* > lstAdapters = pConfig->getVmHardwareList()->m_lstNetworkAdapters;
	QList<CVmGenericNetworkAdapter* > lstOldAdapters = pOldConfig->getVmHardwareList()->m_lstNetworkAdapters;

	// Add/Update veth device
	foreach(CVmGenericNetworkAdapter* pAdapter, lstAdapters)
	{
		get_veth_ifname(pAdapter, ifname, sizeof(ifname));

		if (pAdapter->getSystemName().isEmpty())
			pAdapter->setSystemName(ifname);

		CVmGenericNetworkAdapter *pOldAdapter = findNetAdapter(lstOldAdapters, pAdapter);

		if (pOldAdapter != NULL) {
			populateDnsListsChanges(pAdapter->getDnsIPAddresses(),
				pOldAdapter->getDnsIPAddresses(), lstNameserver,
				lstDelNameserver);
			populateDnsListsChanges(pAdapter->getSearchDomains(),
				pOldAdapter->getSearchDomains(), lstSearchdomain,
				lstDelSearchdomain);
		} else {
			lstNameserver += pAdapter->getDnsIPAddresses();
			lstSearchdomain += pAdapter->getSearchDomains();
		}
		if (pAdapter->isVenetDevice())
			continue;

		if (pOldAdapter != NULL &&
				pAdapter->getVirtualNetworkID() == pOldAdapter->getVirtualNetworkID() &&
				pAdapter->getMacAddress() == pOldAdapter->getMacAddress() &&
				pAdapter->getNetAddresses() == pOldAdapter->getNetAddresses() &&
				pAdapter->getDefaultGateway() == pOldAdapter->getDefaultGateway() &&
				pAdapter->getDefaultGatewayIPv6() == pOldAdapter->getDefaultGatewayIPv6() &&
				pAdapter->isConfigureWithDhcp() == pOldAdapter->isConfigureWithDhcp() &&
				pAdapter->isConfigureWithDhcpIPv6() == pOldAdapter->isConfigureWithDhcpIPv6() &&
				pAdapter->isAutoApply() == pOldAdapter->isAutoApply() &&
				pAdapter->getHostInterfaceName() == pOldAdapter->getHostInterfaceName() &&
				pAdapter->getPktFilter()->isPreventMacSpoof() == pOldAdapter->getPktFilter()->isPreventMacSpoof() &&
				pAdapter->getPktFilter()->isPreventIpSpoof() == pOldAdapter->getPktFilter()->isPreventIpSpoof())
			continue;

		vzctl_veth_dev_iterator itdev;
		struct vzctl_veth_dev_param dev = vzctl_veth_dev_param();

		dev.dev_name_ve = ifname;

		QByteArray dev_name;
		if (!pAdapter->getHostInterfaceName().isEmpty()) {
			dev_name = pAdapter->getHostInterfaceName().toUtf8();
			dev.dev_name = dev_name.data();
		}

		dev.ip_apply_mode = 1; // set mode
		dev.configure_mode = pAdapter->isAutoApply() ?
					VZCTL_VETH_CONFIGURE_ALL : VZCTL_VETH_CONFIGURE_NONE;

		QByteArray mac_ve;
		if (!pAdapter->getMacAddress().isEmpty()) {
			mac_ve = pAdapter->getMacAddress().toUtf8();
			dev.mac_ve = mac_ve.data();
		}

		QByteArray mac;
		if (!pAdapter->getHostMacAddress().isEmpty()) {
			mac = pAdapter->getHostMacAddress().toUtf8();
			dev.mac = mac.data();
		}

		dev.allow_mac_spoof = !pAdapter->getPktFilter()->isPreventMacSpoof();
		dev.allow_ip_spoof = !pAdapter->getPktFilter()->isPreventIpSpoof();

		QByteArray network;
		if (!pAdapter->getVirtualNetworkID().isEmpty()) {
			network = pAdapter->getVirtualNetworkID().toUtf8();
			dev.network = network.data();
		}

		if (pAdapter->isConfigureWithDhcp())
			dev.dhcp = 1;

		if (pAdapter->isConfigureWithDhcpIPv6())
			dev.dhcp6 = 1;

		QByteArray gw(pAdapter->getDefaultGateway().toUtf8());
		dev.gw = gw.data();

		QByteArray gw6(pAdapter->getDefaultGatewayIPv6().toUtf8());
		dev.gw6 = gw6.data();

		itdev = vzctl2_create_veth_dev(&dev, sizeof(dev));
		if (itdev == NULL) {
			WRITE_TRACE(DBG_FATAL, "vzctl_create_veth_dev failed");
			continue;
		}
		foreach(QString ip, pAdapter->getNetAddresses())
		{
			ret = vzctl2_env_add_veth_ipaddress(itdev, QSTR2UTF8(ip));
			if (ret) {
				WRITE_TRACE(DBG_FATAL, "vzctl2_env_add_veth_ipaddress failed: %s [%d]",
						vzctl2_get_last_error(), ret);
				continue;
			}
		}
		vzctl2_env_add_veth(new_param, itdev);
	}

	// Del veth device
	foreach(CVmGenericNetworkAdapter* pOldAdapter, lstOldAdapters)
	{
		if (pOldAdapter->isVenetDevice())
			continue;

		get_veth_ifname(pOldAdapter, ifname, sizeof(ifname));
		if (pOldAdapter->getSystemName().isEmpty())
			pOldAdapter->setSystemName(ifname);

		if (findNetAdapter(lstAdapters, pOldAdapter))
			continue;

		vzctl2_env_del_veth(new_param, ifname);
	}

	// Update DISK
	QList<CVmHardDisk *> &lstNewHardDisks = pConfig->getVmHardwareList()->m_lstHardDisks;
	QList<CVmHardDisk *> &lstOldHardDisks = pOldConfig->getVmHardwareList()->m_lstHardDisks;
	foreach (CVmHardDisk *pOldHdd, lstOldHardDisks) {
		CVmHardDisk *pHdd = findDiskInList(pOldHdd, lstNewHardDisks);
		if (pHdd != NULL &&
			(pHdd->getMountPoint() != pOldHdd->getMountPoint() ||
				pHdd->isAutoCompressEnabled() != pOldHdd->isAutoCompressEnabled() ||
				pHdd->getUserFriendlyName() != pOldHdd->getUserFriendlyName()))
		{
			struct vzctl_disk_param param = vzctl_disk_param();

			strncpy(param.uuid,  QSTR2UTF8(pHdd->getUuid()), sizeof(param.uuid) -1);

			QByteArray mnt;
			if (pHdd->getMountPoint() != pOldHdd->getMountPoint()) {
				mnt = pHdd->getMountPoint().toUtf8();
				param.mnt = mnt.data();
			}

			if (pHdd->isAutoCompressEnabled() != pOldHdd->isAutoCompressEnabled())
				param.autocompact = pHdd->isAutoCompressEnabled() ? VZCTL_PARAM_ON : VZCTL_PARAM_OFF;

			QByteArray storage_url;
			if (pHdd->getStorageURL() != pOldHdd->getStorageURL()) {
				storage_url = pHdd->getStorageURL().toString().toUtf8();
				param.storage_url = storage_url.data();
			}

			QByteArray path;
			if (pHdd->getUserFriendlyName() != pOldHdd->getUserFriendlyName() &&
					!pHdd->getUserFriendlyName().isEmpty())
			{
				path = pHdd->getUserFriendlyName().toUtf8();
				param.path = path.data();
			}

			if (vzctl2_env_set_disk(h, &param)) {
				WRITE_TRACE(DBG_FATAL, "vzctl2_env_set_disk: %s",
						vzctl2_get_last_error());
				return PRL_ERR_OPERATION_FAILED;
			}
		}
	}

	// APPLY_IPONLY
	if (pConfig->getVmSettings()->getGlobalNetwork()->isAutoApplyIpOnly() !=
			pOldConfig->getVmSettings()->getGlobalNetwork()->isAutoApplyIpOnly())
		vzctl2_env_set_apply_iponly(new_param,
				(int)pConfig->getVmSettings()->getGlobalNetwork()->isAutoApplyIpOnly());

	// RATE
	CVmNetworkRates *pRates = pConfig->getVmSettings()->getGlobalNetwork()->getNetworkRates();
	CVmNetworkRates *pOldRates = pOldConfig->getVmSettings()->getGlobalNetwork()->getNetworkRates();
	bChanged = !(pRates->toString() == pOldRates->toString());
	if (bChanged) {
		foreach( CVmNetworkRate *pRate, pRates->m_lstNetworkRates) {
			struct vzctl_rate_param param;
			vzctl_rate_iterator it;
			param.dev = "*";
			param.net_class = pRate->getClassId();
			param.rate = pRate->getRate();

			it = vzctl2_create_rate(&param);
			if (it == NULL) {
				WRITE_TRACE(DBG_FATAL, "vzctl2_create_rate failed");
				continue;
			}

			ret = vzctl2_env_add_rate(new_param, it);
			if (ret) {
				WRITE_TRACE(DBG_FATAL, "vzctl2_env_add_rate failed [%d]", ret);
				continue;
			}
		}
		// Use new ratebound
		vzctl2_env_set_ratebound(new_param, (pRates->isRateBound() ? 1 : 0));
	} else {
		// RATEBOUND
		bool isRatebound = pRates->isRateBound();
		bool isOldratebound = pOldRates->isRateBound();
		if (isRatebound != isOldratebound)
			vzctl2_env_set_ratebound(new_param, (isRatebound ? 1 : 0));
	}

	CVmGenericNetworkAdapter *pVenet = get_venet_device(pConfig);
	CVmGenericNetworkAdapter *pOldVenet = get_venet_device(pOldConfig);
	QList<QString> lstIps;
	QList<QString> lstOldIps;

	if (pVenet != NULL)
		lstIps = pVenet->getNetAddresses();
	if (pOldVenet != NULL)
		lstOldIps = pOldVenet->getNetAddresses();

	// IP_ADDRESS
	if (!(lstIps == lstOldIps)) {
		vzctl2_env_del_ipaddress(new_param, "all");
		foreach(QString ip, lstIps) {
			ret = vzctl2_env_add_ipaddress(new_param, QSTR2UTF8(ip));
			if (ret)
				WRITE_TRACE(DBG_FATAL, "vzctl2_env_add_ipaddress(%s): %s",
					QSTR2UTF8(ip), vzctl2_get_last_error());
		}
	}

	// SEARCHDOMAIN
	if (!lstSearchdomain.isEmpty() || !lstDelSearchdomain.isEmpty()) {
		// searchdomain changed, apply changes
		// filter out deleted values
		foreach(QString str, lstDelSearchdomain)
			if (lstSearchdomain.indexOf(str) != -1)
				lstSearchdomain.removeAll(str);
		if (lstSearchdomain.isEmpty())
			vzctl2_env_add_searchdomain(new_param, "");
		foreach(QString str, lstSearchdomain) {
			ret = vzctl2_env_add_searchdomain(new_param, QSTR2UTF8(str));
			if (ret)
				WRITE_TRACE(DBG_FATAL, "failed vzctl2_env_add_nameserver(%s) [%d]",
							QSTR2UTF8(str), ret);
		}
	}

	// NAMESERVER
	if (!lstNameserver.isEmpty() || !lstDelNameserver.isEmpty()) {
		// searchdomain changed, apply changes
		// filter out deleted values
		foreach(QString str, lstDelNameserver)
			if (lstNameserver.indexOf(str) != -1)
				lstNameserver.removeAll(str);
		if (lstNameserver.isEmpty())
			vzctl2_env_add_nameserver(new_param, "");
		foreach(QString str, lstNameserver) {
			ret = vzctl2_env_add_nameserver(new_param, QSTR2UTF8(str));
			if (ret)
				WRITE_TRACE(DBG_FATAL, "failed vzctl2_env_add_nameserver(%s) [%d]",
						QSTR2UTF8(str), ret);
		}
	}
	// HOSTNAME
	QString hostnm = pConfig->getVmSettings()->getGlobalNetwork()->getHostName();
	if (hostnm != pOldConfig->getVmSettings()->getGlobalNetwork()->getHostName()) {
		/* Strip end '.' #PSBM-18122 */
		vzctl2_env_set_hostname(new_param, QSTR2UTF8(
						hostnm.remove(QRegExp("\\.+$"))));
	}

	bool isEnabled = pConfig->getVmSettings()->getVmAutoCompress()->isEnabled();
	if (pOldConfig->getVmSettings()->getVmAutoCompress()->isEnabled() != isEnabled)
		vzctl2_env_set_autocompact(new_param, !!isEnabled);

	// High Availability Cluster
	isEnabled = pConfig->getVmSettings()->getHighAvailability()->isEnabled();
	if (isEnabled != pOldConfig->getVmSettings()->getHighAvailability()->isEnabled()) {
		ret = vzctl2_env_set_ha_enable(new_param, (isEnabled ? 1 : 0));
		if (ret)
			WRITE_TRACE(DBG_FATAL, "vzctl2_env_set_ha_enable() failed [%d]", ret);
	}
	unsigned long prio = pConfig->getVmSettings()->getHighAvailability()->getPriority();
	if (prio != pOldConfig->getVmSettings()->getHighAvailability()->getPriority()) {
		ret = vzctl2_env_set_ha_prio(new_param, prio);
		if (ret)
			WRITE_TRACE(DBG_FATAL, "vzctl2_env_set_ha_prio() failed [%d]", ret);
	}

	return 0;
}

static int create_env_config(const QString &uuid, SmartPtr<CVmConfiguration> &pConfig,
		PRL_UINT32 flags)
{
	int ret;

	VzctlHandleWrap h(vzctl2_env_open(QSTR2UTF8(uuid), VZCTL_CONF_SKIP_NON_EXISTS | VZCTL_CONF_SKIP_GLOBAL, &ret));
	if (h == NULL) {
		WRITE_TRACE(DBG_FATAL, "failed vzctl2_env_open ctid=%s %s",
			QSTR2UTF8(uuid), vzctl2_get_last_error());
		return PRL_ERR_OPERATION_FAILED;
	}

	VzctlParamWrap new_param(vzctl2_alloc_env_param());
	if (new_param == NULL) {
		WRITE_TRACE(DBG_FATAL, "vzctl2_alloc_env_param");
		return PRL_ERR_OPERATION_FAILED;
	}

	SmartPtr<CVmConfiguration> pOldConfig(new CVmConfiguration);
	pOldConfig->getVmHardwareList()->getMemory()->setRamSize(0);
	pOldConfig->getVmHardwareList()->getCpu()->setNumber(PRL_CPU_UNLIMITED);

	if (!(flags & PRNVM_PRESERVE_DISK)) {
		foreach(CVmHardDisk* d, pConfig->getVmHardwareList()->m_lstHardDisks)
			pOldConfig->getVmHardwareList()->m_lstHardDisks.push_back(new CVmHardDisk(d));
	}

	ret = fill_env_param(h, new_param, pConfig, pOldConfig);
	if (ret)
		return ret;

	struct vzctl_2UL_res res;
	unsigned long long diskSize = 10 * 1024; /* default size is 10G */

	foreach(CVmHardDisk* d, pConfig->getVmHardwareList()->m_lstHardDisks) {
		diskSize = d->getSize();
		if (d->getMountPoint() == "/")
			break;
	}

	diskSize = diskSize << 10; /* Mbytes -> Kbytes */

	res.b = res.l = diskSize;
	vzctl2_env_set_diskspace(new_param, &res);
	res.b = res.l = diskSize / 4;
	vzctl2_env_set_diskinodes(new_param, &res);

	foreach(QString s, pConfig->getCtSettings()->getRawParam()) {
		int n = s.indexOf('=');

		if (n < 2)
			continue;

		std::string l = s.left(n - 1).toStdString();
		std::string r = s.mid(n + 1).toStdString();

		if (l.empty() || r.empty())
			continue;

		vzctl2_env_set_param(h, l.c_str(), r.c_str());
	}

	ret = vzctl2_apply_param(h, new_param, VZCTL_SKIP_SETUP|VZCTL_SAVE);
	if (ret) {
		WRITE_TRACE(DBG_FATAL, "vzctl2_apply_param failed: %s [%d]",
				vzctl2_get_last_error(), ret);
		return PRL_ERR_OPERATION_FAILED;
	}

	/* add disk(s) to CT configuration */
	if (flags & PRNVM_PRESERVE_DISK) {
		foreach(CVmHardDisk* pHdd, pConfig->getVmHardwareList()->m_lstHardDisks) {
			struct vzctl_disk_param d = vzctl_disk_param();

			unsigned int len = pHdd->getUuid().length();
			if (len >= sizeof(d.uuid))
				len = sizeof(d.uuid) -1;
			strncpy(d.uuid, pHdd->getUuid().toUtf8(), len);

			QByteArray mnt;
			if (!pHdd->getMountPoint().isEmpty()) {
				mnt = pHdd->getMountPoint().toUtf8();
				d.mnt = mnt.data();
			}

			QByteArray path;
			if (!pHdd->getUserFriendlyName().isEmpty()) {
				path = pHdd->getUserFriendlyName().toUtf8();
				d.path = path.data();
			}

			if (pHdd->getEmulatedType() == PVE::RealHardDisk)
				d.use_device = 1;

			QByteArray storage_url;
			if (!pHdd->getStorageURL().isEmpty()) {
				storage_url = pHdd->getStorageURL().toString().toUtf8();
				d.storage_url = storage_url.data();
			}

			if (vzctl2_env_attach_disk(h, &d)) {
				WRITE_TRACE(DBG_FATAL, "vzctl2_env_attach_disk, failed: %s [%d]",
					vzctl2_get_last_error(), ret);
				return PRL_ERR_OPERATION_FAILED;
			}
		}
	}

	return PRL_ERR_SUCCESS;
}

int CVzOperationHelper::update_env_uuid(const SmartPtr<CVmConfiguration> &pConfig,
	const SmartPtr<CVmConfiguration> &pOldConfig)
{
	QString uuid = pConfig->getVmIdentification()->getVmUuid();
	QString olduuid = pOldConfig->getVmIdentification()->getVmUuid();
	if (uuid == olduuid)
		return PRL_ERR_SUCCESS;

	QString ctid = CVzHelper::get_ctid_by_uuid(uuid);
	if (ctid.isEmpty())
		return PRL_ERR_CT_NOT_FOUND;

	int ret;
	VzctlHandleWrap h(vzctl2_env_open(QSTR2UTF8(ctid), 0, &ret));
	if (h == NULL) {
		WRITE_TRACE(DBG_FATAL, "failed vzctl2_env_open ctid=%s: %s",
			QSTR2UTF8(ctid), vzctl2_get_last_error());
		return PRL_ERR_OPERATION_FAILED;
	}

	VzctlParamWrap param(vzctl2_alloc_env_param());
	if (param == NULL) {
		WRITE_TRACE(DBG_FATAL, "vzctl2_alloc_env_param");
		return PRL_ERR_OPERATION_FAILED;
	}

	WRITE_TRACE(DBG_FATAL, "Update uuid for Container %s, new uuid %s",
		QSTR2UTF8(ctid), QSTR2UTF8(uuid));

	remove_brackets_from_uuid(uuid);
	if (vzctl2_env_set_uuid(param, QSTR2UTF8(uuid))) {
		WRITE_TRACE(DBG_FATAL, "vzctl2_env_set_uuid");
		return PRL_ERR_OPERATION_FAILED;
	}

	ret = vzctl2_apply_param(h, param, VZCTL_SAVE);
	if (ret) {
		WRITE_TRACE(DBG_FATAL, "vzctl2_apply_param failed: %s [%d]",
			vzctl2_get_last_error(), ret);
		return PRL_ERR_OPERATION_FAILED;
	}

	return PRL_ERR_SUCCESS;
}

int CVzOperationHelper::apply_env_config(SmartPtr<CVmConfiguration> &pConfig,
		SmartPtr<CVmConfiguration> &pOldConfig, unsigned int nFlags)
{
	int ret;

	QString uuid = pConfig->getVmIdentification()->getVmUuid();

	QString ctid = CVzHelper::get_ctid_by_uuid(uuid);
	if (ctid.isEmpty())
		return PRL_ERR_CT_NOT_FOUND;


	if (!pConfig->getVmSettings()->getVmCommonOptions()->getConfigSampleName().isEmpty()) {
                QStringList args;

		args += "set";
		args += ctid;
		args += "--applyconfig";
		args += pConfig->getVmSettings()->getVmCommonOptions()->getConfigSampleName();
		args += "--save";

		ret = run_prg(BIN_VZCTL, args);
		if (PRL_FAILED(ret))
			return ret;
	}

	if ((PVCF_DESTROY_HDD_BUNDLE | PVCF_DETACH_HDD_BUNDLE) & nFlags) {
		QList<CVmHardDisk *> &lstNewHardDisks = pConfig->getVmHardwareList()->m_lstHardDisks;
		QList<CVmHardDisk *> &lstOldHardDisks = pOldConfig->getVmHardwareList()->m_lstHardDisks;

		foreach(CVmHardDisk *pHdd, lstOldHardDisks) {
			if (!findDiskInList(pHdd, lstNewHardDisks)) {
				ret = del_env_disk(uuid, *pHdd, nFlags);
				if (ret)
					return ret;
			}
		}
	}

	QList<CVmHardDisk *> &lstNewHardDisks = pConfig->getVmHardwareList()->m_lstHardDisks;
	QList<CVmHardDisk *> &lstOldHardDisks = pOldConfig->getVmHardwareList()->m_lstHardDisks;
	/* add DISK */
	foreach (CVmHardDisk *pHdd, lstNewHardDisks) {
		if (findDiskInList(pHdd, lstOldHardDisks) == NULL) {
			if (create_env_disk(uuid, pHdd))
				return PRL_ERR_OPERATION_FAILED;
		}
	}

	VzctlHandleWrap h(vzctl2_env_open(QSTR2UTF8(ctid), 0, &ret));
	if (h == NULL) {
		WRITE_TRACE(DBG_FATAL, "failed vzctl2_env_open ctid=%s: %s",
				QSTR2UTF8(uuid), vzctl2_get_last_error());
		return PRL_ERR_OPERATION_FAILED;
	}

	VzctlParamWrap new_param(vzctl2_alloc_env_param());
	if (new_param == NULL) {
		WRITE_TRACE(DBG_FATAL, "vzctl2_alloc_env_param");
		return PRL_ERR_OPERATION_FAILED;
	}

	ret = fill_env_param(h, new_param, pConfig, pOldConfig);
	if (ret == 0) {

		// Store xml copy
		QString cfg = get_env_xml_config_path(pConfig);
		ret = pConfig->saveToFile(cfg, true, true);

		ret = vzctl2_apply_param(h, new_param, VZCTL_SAVE);
		if (ret) {
			WRITE_TRACE(DBG_FATAL, "vzctl2_apply_param failed: %s [%d]",
					vzctl2_get_last_error(), ret);
			ret = PRL_ERR_OPERATION_FAILED;
		}
	}

	return ret;
}

int CVzHelper::is_vz_running()
{
	struct stat st;
	if (stat("/proc/vz/veinfo", &st)) {
		if (errno == ENOENT)
			return 0;
		return -1;
	}
	return 1;
}

int CVzHelper::lock_env(const QString &uuid, const char *status)
{
	QString ctid = CVzHelper::get_ctid_by_uuid(uuid);
	if (ctid.isEmpty())
		return -1;

	return vzctl2_env_lock_prvt(QSTR2UTF8(ctid), NULL, status);
}

void CVzHelper::unlock_env(const QString &uuid, int lockfd)
{
	QString ctid = CVzHelper::get_ctid_by_uuid(uuid);

	vzctl2_env_unlock_prvt(QSTR2UTF8(ctid), lockfd, NULL);
}

QString CVzHelper::parse_ctid(const QString& src)
{
	ctid_t ctid;

	if (vzctl2_parse_ctid(QSTR2UTF8(src), ctid) != 0)
		return QString();

	return QString(ctid);
}

/*
 * Create ctid copying its value from uuid with removed brackets.
 */
QString CVzHelper::build_ctid_from_uuid(const QString& uuid)
{
	QString ctid = uuid;
	remove_brackets_from_uuid(ctid);
	return ctid;
}

int CVzOperationHelper::register_env(const QString &sPath,
		const QString &sCtId, const QString &sUuid,
		PRL_UINT32 nFlags, SmartPtr<CVmConfiguration> &pConfig)
{
	int ret;
	QString ctid(sCtId);
	QStringList args;
	QFileInfo fi(sPath);
	ctid_t buf;

	if (!sCtId.isEmpty()) {
		ctid = sCtId;
		remove_brackets_from_uuid(ctid);
	} else if (sUuid.isEmpty() &&
			vzctl2_parse_ctid(QSTR2UTF8(fi.fileName()), buf) == 0)
	{
		/* get ctid from VE_PRIVATE/$CTID */
		ctid = buf;
	} else {
		/* use uuid as CTID */
		ctid = sUuid;
		remove_brackets_from_uuid(ctid);
	}

	WRITE_TRACE(DBG_FATAL, "Register Container ctid=%s path=%s, uuid=%s",
			QSTR2UTF8(ctid),
			QSTR2UTF8(sPath),
			QSTR2UTF8(sUuid));

	if (nFlags & PRVF_IGNORE_HA_CLUSTER)
		args += "--ignore-ha-cluster";

	args += "register";
	args += sPath;
	args += ctid;

	if (!sUuid.isEmpty()) {
		args += "--uuid";
		args += sUuid;
	}

	if (nFlags & PRCF_FORCE)
		args += "--force";

	ret = run_prg(BIN_VZCTL, args);
	if (ret)
		return ret;

	pConfig = CVzHelper::get_env_config_by_ctid(ctid);

	return PRL_ERR_SUCCESS;
}

int CVzOperationHelper::unregister_env(const QString &uuid, int flags)
{
	Q_UNUSED(flags);
	QStringList args;

	QString ctid = CVzHelper::get_ctid_by_uuid(uuid);
	if (ctid.isEmpty())
		return PRL_ERR_CT_NOT_FOUND;

	WRITE_TRACE(DBG_FATAL, "Unregister Container %s",
			QSTR2UTF8(ctid));

	args += "unregister";
	args += ctid;

	return run_prg(BIN_VZCTL, args);
}

int CVzOperationHelper::create_env(const QString &dst, SmartPtr<CVmConfiguration> &pConfig,
		PRL_UINT32 flags)
{
	int ret;

	QStringList args;
	QString uuid = pConfig->getVmIdentification()->getVmUuid();
	QString name = pConfig->getVmIdentification()->getVmName();
	QString ostemplate = pConfig->getCtSettings()->getOsTemplate();

	char conf[512];

	vzctl2_get_env_conf_path(QSTR2UTF8(uuid), conf, sizeof(conf));
	ret = create_env_config(uuid, pConfig, flags);
	if (ret) {
		unlink(conf);
		return ret;
	}

	args += "create";
	args += remove_brackets_from_uuid(uuid);

	if (!dst.isEmpty()) {
		args += "--private";
		// Use the same schema for CT and VM
		args += QString("%1/$VEID").arg(dst);
	}
	if (!ostemplate.isEmpty()) {
		args += "--ostemplate";
		args += ostemplate;
	}
	if (!pConfig->getVmSettings()->getVmCommonOptions()->getConfigSampleName().isEmpty()) {
		args += "--config";
		args += pConfig->getVmSettings()->getVmCommonOptions()->getConfigSampleName();
	}
	if (!name.isEmpty()) {
		args += "--name";
		args += name;
	}

	if (flags & PRNVM_PRESERVE_DISK)
		args += "--no-hdd";


	args += "--uuid";
	args += uuid;

	m_process_progress_evt = true;
	m_sUuid = uuid;

	PRL_RESULT res = run_prg(BIN_VZCTL, args);
	if (PRL_FAILED(res)) {
		unlink(conf);
		return vz2prl_err(get_rc());;
	}

	SmartPtr<CVmConfiguration> pNewConfig = CVzHelper::get_env_config_by_ctid(uuid);
	if (!pNewConfig)
		return PRL_ERR_VZ_OPERATION_FAILED;
 
	QString cfg = get_env_xml_config_path(pNewConfig);
	// Store xml copy (#PSBM-8440)
	pConfig->saveToFile(cfg, true, true);
	merge_params(pNewConfig);

	pConfig = pNewConfig;

	return 0;
}

int CVzOperationHelper::clone_env(const SmartPtr<CVmConfiguration> &pConfig,
		const QString &sNewHome, const QString &sNewName, PRL_UINT32 nFlags,
		SmartPtr<CVmConfiguration> &pNewConfig)
{
	Q_UNUSED(nFlags);
	QStringList args;

	QString uuid = pConfig->getVmIdentification()->getVmUuid();

	QString ctid = CVzHelper::get_ctid_by_uuid(uuid);
	if (ctid.isEmpty())
		return PRL_ERR_CT_NOT_FOUND;

	// vzmlocal -C 101 --new-id 201 [--new-private /vz/private/test] --new-uuid <uuid>
	args += "-C";
	args += ctid;

	if (!sNewHome.isEmpty()) {
		// Convert sNewHome to VE_PRIVATE
		args += "--new-private";
		args += QString("%1/%2").arg(sNewHome).arg(uuid);
	}

	if (!sNewName.isEmpty()) {
		args += "--new-name";
		args += sNewName;
	}

	QString dst_uuid = pNewConfig->getVmIdentification()->getVmUuid();
	args += "--new-uuid";
	args += remove_brackets_from_uuid(dst_uuid);

	// Avoid CT template locking because
	// several Clone tasks can be started simultaneously from the same template.
	if (pConfig->getVmSettings()->getVmCommonOptions()->isTemplate())
		args += "--skiplock";

	PRL_RESULT res = run_prg("/usr/sbin/vzmlocal", args);
	if (PRL_FAILED(res))
		return res;

	// Get new Config
	pNewConfig = CVzHelper::get_env_config_by_ctid(dst_uuid);

	return res;
}

int CVzOperationHelper::get_resize_env_info(const QString &uuid, CDiskImageInfo &di)
{
	PRL_RESULT ret;

	SmartPtr<CVmConfiguration> pConfig = CVzHelper::get_env_config(uuid);

	if (!pConfig)
		return PRL_ERR_VM_GET_CONFIG_FAILED;

	quint64 nAvailableSpace = 0;
	quint64 nTotalSpace = 0;
	QString ve_private = pConfig->getVmIdentification()->getHomePath();

	ret = CFileHelper::GetDiskAvailableSpace(ve_private, &nAvailableSpace, &nTotalSpace);
	if (PRL_FAILED(ret))
		return ret;

	QList<CVmHardDisk*> lstDisk = pConfig->getVmHardwareList()->m_lstHardDisks;
	unsigned long nDiskSize = 0;

	foreach(CVmHardDisk* pDisk, lstDisk) {
		if (pDisk->getEmulatedType() == PVE::ContainerHardDisk) {
			nDiskSize = pDisk->getSize();
			break;
		}
	}

	di.setSuspended(false);
	di.setSnapshotCount(0);
	di.setResizeSupported(true);
	di.setCurrentSize(nDiskSize);
	di.setMinSize(10);
	di.setMaxSize(nAvailableSpace >> 10);

	return PRL_ERR_SUCCESS;
}

int CVzOperationHelper::create_env_disk(const QString &uuid, const CVmHardDisk &disk)
{
	QString sPath = disk.getUserFriendlyName();
	unsigned int nNewSize = disk.getSize();
	QStringList args;

	QString ctid = CVzHelper::get_ctid_by_uuid(uuid);
	if (ctid.isEmpty())
		return PRL_ERR_CT_NOT_FOUND;

	args += "set";
	args += ctid;
	args += "--save";

	args += "--device-add";
	args += "hdd";

	if (!disk.getUuid().isEmpty()) {
		args += "--device-uuid";
		args += disk.getUuid();
	}

	if (!sPath.isEmpty()) {
		args += disk.getEmulatedType() == PVE::RealHardDisk ?
						"--device" : "--image";
		args += sPath;
	}

	if (!disk.getMountPoint().isEmpty()) {
		args += "--mnt";
		args += disk.getMountPoint();
	}

	if (!disk.getStorageURL().isEmpty()) {
		args += "--storage-url";
		args += disk.getStorageURL().toString();
	}

	args += "--size";
	args += QString("%1").arg((unsigned long long)nNewSize << 10);

	return run_prg(BIN_VZCTL, args);
}

int CVzOperationHelper::del_env_disk(const QString &uuid, const CVmHardDisk &disk,
		unsigned int flags)
{
	QStringList args;

	QString ctid = CVzHelper::get_ctid_by_uuid(uuid);
	if (ctid.isEmpty())
		return PRL_ERR_CT_NOT_FOUND;

	args += "set";
	args += ctid;
	args += "--save";
	args += "--device-del";
	args += disk.getUuid();

	if (PVCF_DETACH_HDD_BUNDLE & flags)
		args += "--detach";

	return run_prg(BIN_VZCTL, args);
}

/* FIXME: it needed to use UUID for disk ID */
static int find_disk_by_path(vzctl_env_param_ptr env_param, const QString &sPath, char *uuid, int size)
{
	struct vzctl_disk_param disk;
	QString sFullPath = sPath;

	if (sPath.at(0) != '/') {
		const char *home = NULL;

		vzctl2_env_get_ve_private_path(env_param, &home);
		if (home != NULL)
			sFullPath = QString("%1/%2").arg(home).arg(sPath);
	}

	vzctl_disk_iterator it = NULL;
	while ((it = vzctl2_env_get_disk(env_param, it)) != NULL) {
		vzctl2_env_get_disk_param(it, &disk, sizeof(disk));
		if (disk.path != NULL && sFullPath == disk.path) {
			memcpy(uuid, disk.uuid, size);
			return 0;
		}
	}
	return -1;
}

int CVzOperationHelper::resize_env_disk(const QString &uuid, const QString &sPath,
		unsigned int nNewSize, unsigned int flags)
{
	int ret;
	unsigned long long nDiskSize;

	QString ctid = CVzHelper::get_ctid_by_uuid(uuid);
	if (ctid.isEmpty())
		return PRL_ERR_CT_NOT_FOUND;

	VzctlHandleWrap h(vzctl2_env_open(QSTR2UTF8(ctid), 0, &ret));
	if (h == NULL) {
		WRITE_TRACE(DBG_FATAL, "failed vzctl2_env_open ctid=%s %s",
				QSTR2UTF8(uuid), vzctl2_get_last_error());
		return -1;
	}

	QStringList args;
	args += "set";
	args += ctid;
	args += "--save";

	// Mb -> 1Kb
	nDiskSize = (unsigned long long)nNewSize * 1024;

	vzctl_env_param_ptr env_param = vzctl2_get_env_param(h);
	int layout;
	vzctl2_env_get_layout(env_param, &layout);
	if (layout == VZCTL_LAYOUT_5) {
		struct vzctl_disk_param param = vzctl_disk_param();

		if (find_disk_by_path(env_param, sPath, param.uuid, sizeof(param.uuid))) {
			WRITE_TRACE(DBG_FATAL, "Disk image resize failed: unable to find disk by '%s'",
					QSTR2UTF8(sPath));
			return PRL_ERR_OPERATION_FAILED;
		}

		args += "--device-set";
		args += param.uuid;

		args += "--size";
		args += QString("%1").arg(nDiskSize);
		if (flags & PRIF_RESIZE_OFFLINE)
			args += "--offline";
	} else {
		args += "--diskspace";
		args += QString("%1").arg(nDiskSize);

		args += "--diskinodes";
		args += QString("%1").arg(nDiskSize / 4);
	}

	return run_prg(BIN_VZCTL, args);
}

int CVzOperationHelper::start_env(const QString &uuid, PRL_UINT32 nFlags)
{
	QStringList args;

	QString ctid = CVzHelper::get_ctid_by_uuid(uuid);
	if (ctid.isEmpty())
		return PRL_ERR_CT_NOT_FOUND;

	args += "start";
	args += ctid;
	if (nFlags & PNSF_VM_START_WAIT)
		args += "--wait";

	return run_prg(BIN_VZCTL, args);
}

int CVzOperationHelper::restart_env(const QString &uuid)
{
	QStringList args;

	QString ctid = CVzHelper::get_ctid_by_uuid(uuid);
	if (ctid.isEmpty())
		return PRL_ERR_CT_NOT_FOUND;

	args += "restart";
	args += ctid;

	return run_prg(BIN_VZCTL, args);
}

int CVzOperationHelper::mount_env(const QString &uuid)
{
	QStringList args;

	QString ctid = CVzHelper::get_ctid_by_uuid(uuid);
	if (ctid.isEmpty())
		return PRL_ERR_CT_NOT_FOUND;

	args += "mount";
	args += ctid;

	return run_prg(BIN_VZCTL, args);
}

int CVzOperationHelper::umount_env(const QString &uuid)
{
	QStringList args;

	QString ctid = CVzHelper::get_ctid_by_uuid(uuid);
	if (ctid.isEmpty())
		return PRL_ERR_CT_NOT_FOUND;

	args += "umount";
	args += ctid;

	if (PRL_FAILED(run_prg(BIN_VZCTL, args)))
		return vz2prl_err(get_rc());

	return 0;
}

namespace
{

void get_device_mount_info(const QString &device, QString &fstype, QString &mountpoint)
{
	fstype = mountpoint = "unknown";
	if (device.isEmpty())
		return;

	QString out;
	QStringList args;
	args << FINDMNT << "-n" << "-o" << "FSTYPE,TARGET" << "-S" << device;
	if (!HostUtils::RunCmdLineUtility(args.join(" "), out))
		return;

	// In case of multiple mountpoints, we take first.
	out = out.split("\n")[0];

	QRegExp re("^(\\S+)\\s+(.+)$");
	if (re.indexIn(out) == -1)
		return;
	fstype = re.cap(1);
	mountpoint = re.cap(2).trimmed();
}

} // namespace

Prl::Expected<QString, PRL_RESULT> CVzOperationHelper::get_env_mount_info(
		const SmartPtr<CVmConfiguration> &pConfig)
{
	QString uuid = pConfig->getVmIdentification()->getVmUuid();
	QList<Ct::Statistics::Filesystem> filesystems;
	PRL_RESULT res;
	if (PRL_FAILED(res = CVzHelper::get_env_fstat(uuid, filesystems)))
		return res;

	QStringList lstMounts;
	QList<CVmHardDisk*> lstDisk = pConfig->getVmHardwareList()->m_lstHardDisks;
	foreach(const Ct::Statistics::Filesystem &fs, filesystems) {
		if (fs.device.isEmpty())
			continue;

		QString image;
		foreach(CVmHardDisk* pDisk, lstDisk) {
			if (pDisk->getIndex() == fs.index) {
				image = pDisk->getUserFriendlyName();
				break;
			}
		}
		if (image.isEmpty())
			continue;

		QString fstype, mountpoint;
		get_device_mount_info(fs.device + "p1", fstype, mountpoint);
		// Kb to bytes.
		lstMounts << Parallels::formatMountInfo(
				fs.device, image, mountpoint, fstype, fs.total << 10, fs.free << 10);
	}
	return lstMounts.join("\n");
}

int CVzOperationHelper::stop_env(const QString &uuid, PRL_UINT32 nMode)
{
	QStringList args;

	QString ctid = CVzHelper::get_ctid_by_uuid(uuid);
	if (ctid.isEmpty())
		return PRL_ERR_CT_NOT_FOUND;

	args += "stop";
	args += ctid;
	if ((nMode & PRL_VM_STOP_MODE_MASK) == PSM_KILL)
		args += "--fast";

	CVzHelper::sync_env_uptime(uuid);

	return run_prg(BIN_VZCTL, args);
}

int CVzOperationHelper::suspend_env(const QString &uuid)
{
	QStringList args;

	QString ctid = CVzHelper::get_ctid_by_uuid(uuid);
	if (ctid.isEmpty())
		return PRL_ERR_CT_NOT_FOUND;

	args += "suspend";
	args += ctid;

	CVzHelper::sync_env_uptime(uuid);

	return run_prg(BIN_VZCTL, args);
}

int CVzOperationHelper::resume_env(const QString &uuid, PRL_UINT32 flags)
{
	QStringList args;

	QString ctid = CVzHelper::get_ctid_by_uuid(uuid);
	if (ctid.isEmpty())
		return PRL_ERR_CT_NOT_FOUND;

	args += "resume";
	args += ctid;
	if (flags & PNSF_CT_SKIP_ARPDETECT)
		 args += "--skip_arpdetect";

	return run_prg(BIN_VZCTL, args);
}

int CVzOperationHelper::delete_env(const QString &uuid)
{
	QStringList args;

	QString ctid = CVzHelper::get_ctid_by_uuid(uuid);
	if (ctid.isEmpty())
		return PRL_ERR_CT_NOT_FOUND;

	args += "destroy";
	args += ctid;

	return run_prg(BIN_VZCTL, args);
}

int CVzOperationHelper::set_env_userpasswd(const QString &uuid, const QString &user,
		const QString &pw, PRL_UINT32 nFlags)
{
	QStringList args;

	QString ctid = CVzHelper::get_ctid_by_uuid(uuid);
	if (ctid.isEmpty())
		return PRL_ERR_CT_NOT_FOUND;

	args += "set";
	args += ctid;
	args += "--userpasswd";
	args += QString("%1:%2").arg(user).arg(pw);
	if (nFlags & PSPF_PASSWD_CRYPTED)
		args += "--crypted";

	CVzHelper::sync_env_uptime(uuid);

	return run_prg(BIN_VZCTL, args, true);
}

int CVzOperationHelper::auth_env_user(const QString &uuid, const QString &user, const QString &pw)
{
	int ret;

	QString ctid = CVzHelper::get_ctid_by_uuid(uuid);
	if (ctid.isEmpty())
		return PRL_ERR_CT_NOT_FOUND;

	VzctlHandleWrap h(vzctl2_env_open(QSTR2UTF8(ctid), 0, &ret));
	if (h == NULL) {
		WRITE_TRACE(DBG_FATAL, "failed vzctl2_env_open ctid=%s: %s",
				QSTR2UTF8(uuid), vzctl2_get_last_error());
		return PRL_ERR_OPERATION_FAILED;
	}

	if (vzctl2_env_auth(h, QSTR2UTF8(user), QSTR2UTF8(pw), -1, 0)) {
		WRITE_TRACE(DBG_FATAL, "failed vzctl2_env_auth %s: %s",
				QSTR2UTF8(uuid), vzctl2_get_last_error());
		return PRL_ERR_OPERATION_FAILED;
	}

	return PRL_ERR_SUCCESS;
}

int CVzOperationHelper::create_env_snapshot(const QString &uuid,
		const QString &snap_uuid, const QString &name, const QString &desc)
{
	Q_UNUSED(name);
	Q_UNUSED(desc);

	QStringList args;

	QString ctid = CVzHelper::get_ctid_by_uuid(uuid);
	if (ctid.isEmpty())
		return PRL_ERR_CT_NOT_FOUND;

	if (!Uuid::isUuid(snap_uuid))
		return PRL_ERR_INVALID_ARG;

	args += "snapshot";
	args += ctid;
	args += "--uuid"; args += snap_uuid;
	args += "--env";

	m_Envs.insert(VZCTL_ENV_SNAPSHOT_NAME_PARAM, name);
	m_Envs.insert(VZCTL_ENV_SNAPSHOT_DESC_PARAM, desc);

	return run_prg(BIN_VZCTL, args);
}

int CVzOperationHelper::delete_env_snapshot(const QString &uuid, const QString &snap_uuid,
		bool bDelChild)
{
	Q_UNUSED(bDelChild);
	QStringList args;

	QString ctid = CVzHelper::get_ctid_by_uuid(uuid);
	if (ctid.isEmpty())
		return PRL_ERR_CT_NOT_FOUND;

	if (!Uuid::isUuid(snap_uuid))
		return PRL_ERR_INVALID_ARG;

	args += "snapshot-delete";
	args += ctid;
	args += "--uuid"; args += snap_uuid;

	return run_prg(BIN_VZCTL, args);
}

int CVzOperationHelper::switch_env_snapshot(const QString &uuid, const QString &snap_uuid,
		PRL_UINT32 flags)
{
	QStringList args;

	if (!Uuid::isUuid(snap_uuid))
		return PRL_ERR_INVALID_ARG;

	QString ctid = CVzHelper::get_ctid_by_uuid(uuid);
	if (ctid.isEmpty())
		return PRL_ERR_CT_NOT_FOUND;

	args += "snapshot-switch";
	args += ctid;
	args += "--uuid"; args += snap_uuid;

	if (flags & PSSF_SKIP_RESUME)
		args += "--skip-resume";

	return run_prg(BIN_VZCTL, args);
}

int CVzOperationHelper::create_disk_image(const QString &path, quint64 sizeBytes)
{
	int ret;
	struct vzctl_create_image_param param;

	param.mode = 0;		/* expanded */
	param.size = sizeBytes >> 10;	/* 1K blocks*/

	ret = vzctl2_create_disk_image(QSTR2UTF8(path), &param);
	if (ret) {
		WRITE_TRACE(DBG_FATAL, "failed vzctl2_create_disk_image %s: %s [%d]",
				QSTR2UTF8(path), vzctl2_get_last_error(), ret);

		return PRL_ERR_OPERATION_FAILED;
	}
	return PRL_ERR_SUCCESS;
}

int CVzOperationHelper::mount_disk_snapshot(const QString &path,
		const QString &snap_guid, const char *component_name,
		const QString &mnt_dir, QString &dev)
{
	int ret;
	struct vzctl_mount_param param;
	memset(&param, 0, sizeof(param));
	param.ro = 1;

	QByteArray aGuid(snap_guid.toUtf8());
	if (!snap_guid.isEmpty())
		param.guid = aGuid.data();

	QByteArray aMntDir(mnt_dir.toUtf8());
	if (!mnt_dir.isEmpty())
		param.target = aMntDir.data();

	param.component_name = (char *)component_name;

	/* umount snapshot if mounted by component_name, #PSBM-19079 */
	if (component_name != NULL)
		vzctl2_umount_disk_snapshot(QSTR2UTF8(path), param.guid, component_name);

	ret = vzctl2_mount_disk_snapshot(QSTR2UTF8(path), &param);
	if (ret) {
		WRITE_TRACE(DBG_FATAL, "failed vzctl2_mount_disk_snapshot %s: %s [%d]",
				QSTR2UTF8(path), vzctl2_get_last_error(), ret);
		return PRL_ERR_OPERATION_FAILED;
	}

	// return ploop device to the caller
	dev = param.device;

	return PRL_ERR_SUCCESS;
}

int CVzOperationHelper::mount_disk_image(const QString &path,
	const QString &target, QString &dev)
{
	struct vzctl_mount_param param;
	QByteArray target_buf;

	memset(&param, 0, sizeof(struct vzctl_mount_param));

	// process optional mount point
	if (!target.isEmpty()) {
		target_buf = target.toUtf8();
		param.target = target_buf.data();
	}

	if (vzctl2_mount_disk_image(QSTR2UTF8(path), &param)) {
		WRITE_TRACE(DBG_FATAL, "failed vzctl2_mount_disk_image %s %s",
				QSTR2UTF8(path), vzctl2_get_last_error());
		return PRL_ERR_OPERATION_FAILED;
	}
	// return ploop device to the caller
	dev = param.device;

	return PRL_ERR_SUCCESS;
}

int CVzOperationHelper::create_env_private(const QString &ve_private, int layout)
{
	int ret;

	ret = vzctl2_create_env_private(QSTR2UTF8(ve_private), layout);
	if (ret) {
		WRITE_TRACE(DBG_FATAL, "failed vzctl2_create_env_private %s %s [%d]",
				QSTR2UTF8(ve_private), vzctl2_get_last_error(), ret);

		return -1;
	}
	return 0;
}

void CVzOperationHelper::release_snapshot_holder()
{
	if (m_snap_holder != NULL) {
		vzctl2_release_snap_holder(m_snap_holder);
		delete m_snap_holder;
		m_snap_holder = NULL;
	}
}

int CVzOperationHelper::alloc_snapshot_holder()
{
	release_snapshot_holder();

	m_snap_holder = new(std::nothrow)vzctl_snap_holder();
	if (m_snap_holder == NULL) {
		WRITE_TRACE(DBG_FATAL, "alloc_snapshot_holder: ENOMEM");
		return PRL_ERR_OPERATION_FAILED;
	}

	return PRL_ERR_SUCCESS;
}

int CVzOperationHelper::create_tsnapshot(const QString &uuid,
		const QString &snap_guid, const QString &cbt_uuid,
		const char *component_name, const char *snap_dir)
{
	int ret;

	if (!cbt_uuid.isEmpty())
		return PRL_ERR_UNIMPLEMENTED;

	QString ctid = CVzHelper::get_ctid_by_uuid(uuid);
	if (ctid.isEmpty())
		return PRL_ERR_CT_NOT_FOUND;

	VzctlHandleWrap h(vzctl2_env_open(QSTR2UTF8(ctid), 0, &ret));
	if (h == NULL) {
		WRITE_TRACE(DBG_FATAL, "failed vzctl2_env_open ctid=%s: %s",
				QSTR2UTF8(uuid), vzctl2_get_last_error());
		return PRL_ERR_OPERATION_FAILED;
	}

	ret = alloc_snapshot_holder();
	if (ret)
		return ret;

	struct vzctl_tsnapshot_param tsnap;
	memset(&tsnap, 0, sizeof(tsnap));
	tsnap.component_name = (char *)component_name;
	tsnap.snap_dir = (char *)snap_dir;

	ret = vzctl2_env_create_temporary_snapshot(h, QSTR2UTF8(snap_guid),
			&tsnap, m_snap_holder);
	if (ret) {
		WRITE_TRACE(DBG_FATAL, "failed vzctl2_env_create_temporary_snapshot ctid=%s: %s",
				QSTR2UTF8(uuid), vzctl2_get_last_error());

		return PRL_ERR_OPERATION_FAILED;
	}

	return 0;
}

int CVzOperationHelper::umount_snapshot(const QString &dev)
{
	int ret;

	ret = vzctl2_umount_image_by_dev(QSTR2UTF8(dev));
	if (ret) {
		WRITE_TRACE(DBG_FATAL, "failed vzctl2_umount_image_by_dev %s [%s], %d",
				QSTR2UTF8(dev), vzctl2_get_last_error(), ret);
		return -1;
	}
	return 0;
}

int CVzOperationHelper::merge_snapshot(const QString &uuid, const QString &snap_guid)
{
	int ret;

	QString ctid = CVzHelper::get_ctid_by_uuid(uuid);
	if (ctid.isEmpty())
		return PRL_ERR_CT_NOT_FOUND;

	VzctlHandleWrap h(vzctl2_env_open(QSTR2UTF8(ctid), 0, &ret));
	if (h == NULL) {
		WRITE_TRACE(DBG_FATAL, "failed vzctl2_env_open ctid=%s %s [%d]",
				QSTR2UTF8(uuid), vzctl2_get_last_error(), ret);
		return -1;
	}
	if (vzctl2_merge_snapshot(h, QSTR2UTF8(snap_guid))) {
		WRITE_TRACE(DBG_FATAL, "failed vzctl2_merge_snapshot ctid=%s %s",
				QSTR2UTF8(uuid), vzctl2_get_last_error());
		return -1;
	}
	return 0;
}

int CVzOperationHelper::delete_tsnapshot(const QString &uuid, const QString &snap_guid)
{
	int ret;
	QString ctid = CVzHelper::get_ctid_by_uuid(uuid);

	if (ctid.isEmpty())
		return PRL_ERR_CT_NOT_FOUND;

	VzctlHandleWrap h(vzctl2_env_open(QSTR2UTF8(ctid), 0, &ret));
	if (h == NULL) {
		WRITE_TRACE(DBG_FATAL, "failed vzctl2_env_open ctid=%s: %s",
				QSTR2UTF8(uuid), vzctl2_get_last_error());
		return -1;
	}

	if (vzctl2_env_delete_tsnapshot(h, QSTR2UTF8(snap_guid), m_snap_holder)) {
		WRITE_TRACE(DBG_FATAL, "failed vzctl2_delete_tsnapshot ctid=%s %s",
				QSTR2UTF8(uuid), vzctl2_get_last_error());
		return -1;
	}

	return 0;
}

int CVzOperationHelper::remove_disks_from_env_config(SmartPtr<CVmConfiguration> &pConfig,
	SmartPtr<CVmConfiguration> &pOldConfig, const QString &sNewConfName)
{
	int ret;
	QString uuid = pConfig->getVmIdentification()->getVmUuid();

	QString ctid = CVzHelper::get_ctid_by_uuid(uuid);
	if (ctid.isEmpty())
		return PRL_ERR_CT_NOT_FOUND;

	VzctlHandleWrap h(vzctl2_env_open_conf(QSTR2UTF8(ctid), QSTR2UTF8(sNewConfName), 0, &ret));
	if (h == NULL) {
		WRITE_TRACE(DBG_FATAL, "failed vzctl2_env_open ctid=%s: %s",
			QSTR2UTF8(uuid), vzctl2_get_last_error());
		return PRL_ERR_OPERATION_FAILED;
	}

	QList<CVmHardDisk *> &lstNewHardDisks = pConfig->getVmHardwareList()->m_lstHardDisks;
	QList<CVmHardDisk *> &lstOldHardDisks = pOldConfig->getVmHardwareList()->m_lstHardDisks;
	foreach (CVmHardDisk *pOldHdd, lstOldHardDisks) {
		if (!findDiskInList(pOldHdd, lstNewHardDisks)) {
			if (vzctl2_env_del_disk(h, QSTR2UTF8(pOldHdd->getUuid()),
						VZCTL_DISK_DETACH | VZCTL_DISK_SKIP_CONFIGURE))
			{
				WRITE_TRACE(DBG_FATAL, "failed to save CT config '%s': %s [%d]",
					QSTR2UTF8(sNewConfName), vzctl2_get_last_error(), ret);
				return PRL_ERR_OPERATION_FAILED;
			}
		}
	}

	return 0;
}

static Ct::Statistics::Disk get_env_iostat(const QString &uuid)
{
	Ct::Statistics::Disk d;
	QString ctid = CVzHelper::get_ctid_by_uuid(uuid);
	if (ctid.isEmpty())
		return d;

	struct vzctl_iostat s = vzctl_iostat();
	if (vzctl2_get_env_iostat(QSTR2UTF8(ctid), &s, sizeof(s)))
		return d;

	d.read = s.read;
	d.write = s.write;

	return d;
}

static Ct::Statistics::Cpu get_env_cpustat(const QString &uuid)
{
	int ret;
	Ct::Statistics::Cpu c;
	QString ctid = CVzHelper::get_ctid_by_uuid(uuid);
	if (ctid.isEmpty())
		return c;

	VzctlHandleWrap h(vzctl2_env_open(QSTR2UTF8(ctid), 0, &ret));
	if (h == NULL) {
		WRITE_TRACE(DBG_FATAL, "failed vzctl2_env_open ctid=%s: %s",
			QSTR2UTF8(uuid), vzctl2_get_last_error());
		return c;
	}

	struct vzctl_cpustat s = vzctl_cpustat();
	if (vzctl2_env_cpustat(h, &s, sizeof(s)))
		return c;

	const quint64 s_to_ms = 1000000;
	c.uptime = s.uptime * s_to_ms;
	c.system = s.system * s_to_ms;
	c.user = s.user * s_to_ms;
	c.nice = s.nice * s_to_ms;

	return c;
}

static Ct::Statistics::Memory *get_env_meminfo(const QString &uuid)
{
	QString ctid = CVzHelper::get_ctid_by_uuid(uuid);
	if (ctid.isEmpty())
		return NULL;

	struct vzctl_meminfo s = vzctl_meminfo();
	if (vzctl2_get_env_meminfo(QSTR2UTF8(ctid), &s, sizeof(s)))
		return NULL;

	using Ct::Statistics::Memory;

	QScopedPointer<Memory> m(new Memory());
	m->total = s.total;
	m->free = s.free;
	m->cached = s.cached;
	m->swap_in = s.swap_in;
	m->swap_out = s.swap_out;

	return m.take();
}

int CVzHelper::get_env_fstat(const QString &uuid, QList<Ct::Statistics::Filesystem>& fs)
{
	SmartPtr<CVmConfiguration> config = CVzHelper::get_env_config(uuid);
	if (!config.isValid())
		return PRL_ERR_FAILURE;
	QString ctid = CVzHelper::get_ctid_by_uuid(uuid);
	if (ctid.isEmpty())
		return PRL_ERR_CT_NOT_FOUND;
	int ret;
	VzctlHandleWrap h(vzctl2_env_open(QSTR2UTF8(ctid), 0, &ret));
	if (h == NULL) {
		WRITE_TRACE(DBG_FATAL, "failed vzctl2_env_open ctid=%s: %s",
			QSTR2UTF8(uuid), vzctl2_get_last_error());
		return -1;
	}

	QList<Ct::Statistics::Filesystem> tmp;
	foreach (const CVmHardDisk& disk, config->getVmHardwareList()->m_lstHardDisks) {
		struct vzctl_disk_stats stats;
		ret = vzctl2_env_get_disk_stats(h, QSTR2UTF8(disk.getUuid()),
			&stats, sizeof(stats));
		if (ret)
			return PRL_ERR_FAILURE;
		// take only filesystems that are mounted inside CT
		if (stats.device[0] == '\0')
			continue;
		Ct::Statistics::Filesystem f;
		f.total = stats.total;
		f.free = stats.free;
		f.index = disk.getIndex();
		f.device = QString(stats.device);
		tmp.append(f);
	}
	fs.swap(tmp);
	return PRL_ERR_SUCCESS;
}

int CVzHelper::set_env_uptime(const QString &uuid, const quint64 uptime, const QDateTime & date)
{
	int ret;

	QString ctid = CVzHelper::get_ctid_by_uuid(uuid);
	if (ctid.isEmpty())
		return PRL_ERR_CT_NOT_FOUND;

	VzctlHandleWrap h(vzctl2_env_open(QSTR2UTF8(ctid), 0, &ret));
	if (h == NULL) {
		WRITE_TRACE(DBG_FATAL, "failed vzctl2_env_open ctid=%s: %s",
			QSTR2UTF8(uuid), vzctl2_get_last_error());
		return -1;
	}

	if (!uptime)
		ret = vzctl2_env_reset_uptime(h);
	else
		ret = vzctl2_env_set_uptime(h, uptime, date.toTime_t());

	return ret;
}

int CVzHelper::reset_env_uptime(const QString &uuid)
{
	return set_env_uptime(uuid, 0, QDateTime());
}

int CVzHelper::sync_env_uptime(const QString &uuid)
{
	int ret;

	QString ctid = CVzHelper::get_ctid_by_uuid(uuid);
	if (ctid.isEmpty())
		return PRL_ERR_CT_NOT_FOUND;

	VzctlHandleWrap h(vzctl2_env_open(QSTR2UTF8(ctid), 0, &ret));
	if (h == NULL) {
		WRITE_TRACE(DBG_FATAL, "failed vzctl2_env_open ctid=%s: %s",
			QSTR2UTF8(uuid), vzctl2_get_last_error());
		return -1;
	}

	if (vzctl2_env_sync_uptime(h))
		return PRL_ERR_FAILURE;

	return PRL_ERR_SUCCESS;
}

int CVzHelper::set_vziolimit(const char *name)
{
	if (vzctl2_set_vzlimits(name)) {
		WRITE_TRACE(DBG_FATAL, "failed vzctl2_set_vziolimit %s: %s",
				name, vzctl2_get_last_error());
		return -1;
	}
	return 0;
}

/************************************************************************/
void CVzOperationHelper::cancel_operation()
{
	get_cleaner().process();
}

/***********************************************************************/
void CVzOperationCleaner::add(cleanup_callback_FN fn, const QString &sData)
{
	callback_entry entry(fn);

	entry.data.sData = sData;
	m_stack.push(entry);
}

void CVzOperationCleaner::add(cleanup_callback_FN fn, int nInt)
{
	callback_entry entry(fn);

	entry.data.nInt = nInt;
	m_stack.push(entry);
}

void CVzOperationCleaner::add(cleanup_callback_FN fn, void *ptr)
{
	callback_entry entry(fn);

	entry.data.ptr = ptr;
	m_stack.push(entry);
}

void CVzOperationCleaner::pop()
{
	if (!m_stack.isEmpty())
		m_stack.pop();
}

void CVzOperationCleaner::process()
{
	while (!m_stack.isEmpty()) {
		callback_entry &entry = m_stack.top();
		entry.fn(&entry.data);
		m_stack.pop();
	}
}

void CVzOperationCleaner::kill_process(callback_data *data)
{
	int pid = data->nInt;

	if (pid == 0)
		return;

	WRITE_TRACE(DBG_FATAL, "kill_process: %d", pid);
	::kill(pid, SIGTERM);
}

/********************************************************************/
char **CVzExecHelper::make_argv(const QStringList &lst)
{
	char **argv;
	int nelem = lst.size();
	int i = 0;

	argv = (char **)malloc((nelem + 1) * sizeof(char *));
	if (argv == NULL)
		return NULL;
	for (; i < nelem; i++)
		argv[i] = strdup(QSTR2UTF8(lst[i]));
	argv[nelem] = NULL;

	return argv;
}

void CVzExecHelper::free_argv(char **argv)
{
	if (argv != NULL) {
		for (char **p = argv; *p != NULL; p++)
			free(*p);
		free(argv);
	}
}

int CVzExecHelper::run_cmd(const QString &uuid,
		const QString &sPrg,
		const QStringList &Args,
		const QStringList &Envs,
		PRL_UINT32 nFlags, int stdfd[3])
{
	(void) Envs;
	(void) nFlags;
	QStringList args;
	int fdnull;
	QString ctid = CVzHelper::get_ctid_by_uuid(uuid);
	if (ctid.isEmpty())
		return PRL_ERR_CT_NOT_FOUND;


	args += BIN_VZCTL;
	args += "--quiet";
	args += "exec2";
	args += ctid;
	args += sPrg;
	args += Args;

	char **argv = make_argv(args);
	if (argv == NULL)
		return PRL_ERR_OPERATION_FAILED;

	WRITE_TRACE(DBG_DEBUG, "EXEC %s", QSTR2UTF8(args.join(" ")));

	m_pid = vfork();
	if (m_pid < 0) {
		WRITE_TRACE(DBG_FATAL, "fork(): %m");
		free_argv(argv);
		return PRL_ERR_OPERATION_FAILED;
	} else if (m_pid == 0) {
		fdnull = open("/dev/null", O_RDWR);
		dup2(stdfd[0] != -1 ? stdfd[0] : fdnull, 0);
		dup2(stdfd[1] != -1 ? stdfd[1] : fdnull, 1);
		dup2(stdfd[2] != -1 ? stdfd[2] : fdnull, 2);
		for(int fd = getdtablesize(); fd > 2; --fd)
		{
			int i = fcntl(fd, F_GETFD);
			if (i >= 0)
				fcntl(fd, F_SETFD, i | FD_CLOEXEC);
		}

		execv(argv[0], argv);
		_exit(1);
	}
	free_argv(argv);

	return 0;
}

int CVzExecHelper::wait()
{
	int ret;

	ret = vzctl2_env_exec_wait(m_pid, &m_retcode);
	if (ret == 0)
		m_pid = -1;

	return ret;
}

void CVzExecHelper::cancel()
{
	if (m_pid > 0) {
		WRITE_TRACE(DBG_FATAL, "Trying to cancel the process %d", m_pid);
		kill(m_pid, SIGTERM);
	}
}

/************************************************************************/
void CVzStateMonitor::stop()
{
	m_bStopStatusMonitor = true;
	if (m_kevt_fd != -1)
	{
		close(m_kevt_fd);
		m_kevt_fd = -1;
	}
}

CVzStateMonitor::~CVzStateMonitor()
{
	stop();
}

void CVzStateMonitor::start(state_event_handler_fn cb, void *obj)
{
	int ret;
	struct sockaddr_nl nladdr;
	struct iovec iov;
	char buf[512];
	struct msghdr msg;
	vzevt_handle_t *evt_handle;
	int evt_fd = -1;
	bool bVzKernel = QFile::exists("/proc/vz");

	if (m_kevt_fd != -1)
	{
		WRITE_TRACE(DBG_FATAL, "Virtuozzo status monitor already running");
		return;
	}
	m_bStopStatusMonitor = false;
	memset(&msg, 0, sizeof(msg));
	msg.msg_name = (void*)&nladdr;
	msg.msg_namelen = sizeof(nladdr);
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;

	m_kevt_fd = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_VZEVENT);
	if (m_kevt_fd < 0)
	{
		WRITE_TRACE(DBG_FATAL, "Failed to start Vz monitor: %m");
		return;
	}
	if (vzctl2_register_evt(&evt_handle) == 0)
		evt_fd = vzctl2_get_evt_fd(evt_handle);
	else
		WRITE_TRACE(DBG_FATAL, "failed vzctl2_register_evt()");

retry:

	memset(&nladdr, 0, sizeof(nladdr));
	nladdr.nl_family = AF_NETLINK;
	nladdr.nl_groups = 0x01; /* multicast */

	if (bind(m_kevt_fd, (struct sockaddr *)&nladdr, sizeof(nladdr)) == 0)
	{
		nladdr.nl_pid = 0;
		nladdr.nl_groups = 0;
		fd_set read_fd_set;
		struct timeval tv;
		int n, state;
		int max_fd = 0;

		WRITE_TRACE(DBG_FATAL, "Starting Vz event monitor");
		while (!m_bStopStatusMonitor) {
			FD_ZERO(&read_fd_set);
			if (m_kevt_fd != -1) {
				FD_SET(m_kevt_fd, &read_fd_set);
				max_fd = m_kevt_fd;
			}
			if (evt_fd != -1) {
				FD_SET(evt_fd, &read_fd_set);
				if (evt_fd > max_fd)
					max_fd = evt_fd;
			}
			if (m_kevt_fd == -1 && evt_fd == -1)
				break;

			iov.iov_base = buf;
			iov.iov_len = sizeof(buf) - 1;
			// Handle 7 sec task shutdown timeout
			tv.tv_sec = 5;
			tv.tv_usec = 0;

			n = select(max_fd + 1, &read_fd_set, NULL, NULL, &tv);
			if (n == 0)
				continue;
			else if (n < 0)
				break;

			if (evt_fd != -1 && FD_ISSET(evt_fd, &read_fd_set)) {
				struct vzctl_state_evt state_evt = vzctl_state_evt();

				if (vzctl2_get_state_evt(evt_handle, &state_evt,
						sizeof(struct vzctl_state_evt)) == 0)
				{
					if (!m_bStopStatusMonitor){
						cb(obj, QString(state_evt.ctid),
								state_evt.state);
					}
				}
			}
			if (m_kevt_fd != -1 && FD_ISSET(m_kevt_fd, &read_fd_set)) {

				ret = recvmsg(m_kevt_fd, &msg, MSG_DONTWAIT);
				if (ret <= 0) {
					WRITE_TRACE(DBG_FATAL, "recvmsg: %m");
					continue;

				}

				buf[ret] = 0;

				QStringList lst;

				lst = QString(buf).split("@");
				if (lst.count() > 1)
				{
					if (lst[0] == "ve-start") {
						state = VZCTL_ENV_STARTED;
					} else if (lst[0] == "ve-stop" || lst[0] == "ve-reboot") {
						vzctl_env_status_t status;
						ret = vzctl2_get_env_status(QSTR2UTF8(lst[1]), &status, ENV_STATUS_SUSPENDED);
						if (ret == 0 && status.mask & ENV_STATUS_SUSPENDED)
							state = VZCTL_ENV_SUSPENDED;
						else
							state = VZCTL_ENV_STOPPED;
					} else
						continue;

					if (!m_bStopStatusMonitor)
						cb(obj, lst[1], state);
				}
			}
		}
	} else {
		if (bVzKernel) {
			sleep(5);
			goto retry;
		}
		WRITE_TRACE(DBG_FATAL, "Could not bind the socket: %m");
	}

	vzctl2_unregister_evt(evt_handle);
	close(m_kevt_fd);
	m_kevt_fd = -1;
}

int CVzOperationHelper::move_env(const QString &sUuid, const QString &sNewHome,
	const QString &sName)
{
	QString sSrcCtid;
	QString sDstCtid;
	QStringList args;

	sSrcCtid = CVzHelper::get_ctid_by_uuid(sUuid);
	if (sSrcCtid.isEmpty()) {
		WRITE_TRACE(DBG_FATAL, "Can not get container ID for UUID %s", QSTR2UTF8(sUuid));
		return PRL_ERR_CT_NOT_FOUND;
	}

	QString sNewPrivate = QString("%1/%2").arg(sNewHome).arg(sSrcCtid);
	if (QFileInfo(sNewPrivate).exists()) {

		// target private already exists, fail immediately if source CTID
		// present in UUID format
		bool bIsNum;
		sSrcCtid.toUInt(&bIsNum);
		if (!bIsNum) {
			WRITE_TRACE(DBG_FATAL,
				"Private %s already exists, but it is not possible to change "
				"ID for container with UUID name",
				QSTR2UTF8(sNewPrivate));
			return PRL_ERR_OPERATION_FAILED;
		}

		// for unnamed containers dispatcher copy name from VEID, so suppose
		// we move unnamed container if name matches VEID or name empty
		if (sName.isEmpty() || (sName == sSrcCtid)) {
			WRITE_TRACE(DBG_FATAL,
				"Private %s already exists, "
				"but it is not possible to chande ID for unnamed container",
				QSTR2UTF8(sNewPrivate));
			return PRL_ERR_UNNAMED_CT_MOVE;
		}

		// initialize destination CTID using containers UUID if source CTID
		// present in old-style numeric format (VEID)
		sDstCtid = CVzHelper::get_uuid_by_ctid(sSrcCtid);
		remove_brackets_from_uuid(sDstCtid);
		if (sDstCtid.isEmpty()) {
			WRITE_TRACE(DBG_FATAL, "Can't get UUID for CT %s",
				QSTR2UTF8(sSrcCtid));
			return PRL_ERR_OPERATION_FAILED;
		}

	} else {
		sDstCtid = sSrcCtid;
 	}
	sNewPrivate = QString("%1/%2").arg(sNewHome).arg(sDstCtid);

	// vzmlocal 101 --noevent --new-private /vz/private/test
	args += sSrcCtid;
	args += "--noevent";
	args += "--new-private";
	args += sNewPrivate;

	if (sSrcCtid != sDstCtid) {
		args += "--new-id";
		args += sDstCtid;
	}

	PRL_RESULT res = run_prg("/usr/sbin/vzmlocal", args);
	if (PRL_FAILED(res))
		return res;

	if (sSrcCtid != sDstCtid)
		CVzHelper::update_ctid_map(sUuid, sDstCtid);

	return res;
}

int CVzOperationHelper::convert2_env(const QString &srcPath, const QString &dstPath,
	unsigned int layout)
{
	QStringList args;
	args += "convert2";
	args += "--src";
	args += srcPath;
	args += "--dst";
	args += dstPath;
	args += "--velayout";
	args += QString::number(layout);

	return run_prg(BIN_VZCTL, args);
}

Ct::Statistics::Aggregate *CVzHelper::get_env_stat(const QString& uuid)
{
	QString ctid = CVzHelper::get_ctid_by_uuid(uuid);
	if (ctid.isEmpty())
		return NULL;

	vzctl_env_status_t st;
	if (0 != vzctl2_get_env_status(QSTR2UTF8(ctid), &st, ENV_STATUS_RUNNING))
		return NULL;

	using Ct::Statistics::Aggregate;

	QScopedPointer<Aggregate> a(new Aggregate());

	QScopedPointer<PRL_STAT_NET_TRAFFIC> n(get_net_stat(uuid));
	a->net = n.isNull() ? PRL_STAT_NET_TRAFFIC() : *n;
	if (st.mask & ENV_STATUS_RUNNING) {
		a->disk = get_env_iostat(uuid);
		a->memory = SmartPtr<Ct::Statistics::Memory>(get_env_meminfo(uuid));
		a->cpu = get_env_cpustat(uuid);
	}

	return a.take();
}
