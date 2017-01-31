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

#ifndef __CVZHELPER_H__
#define __CVZHELPER_H__
#include <QString>
#include <QMap>
#include <QHash>
#include <QStack>
#include <QMutex>
#include <QProcessEnvironment>
#include <QThread>
#include <prlsdk/PrlIOStructs.h>

#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

#include <prlcommon/Std/SmartPtr.h>
#include <prlcommon/PrlCommonUtilsBase/SysError.h>
#include <prlsdk/PrlEnums.h>
#include <prlxmlmodel/NetworkConfig/CNetworkClassesConfig.h>
#include <prlxmlmodel/NetworkConfig/CNetworkShapingConfig.h>
#include <prlxmlmodel/VmConfig/CVmNetworkRates.h>
#include <prlxmlmodel/VmConfig/CVmConfiguration.h>
#include <prlxmlmodel/HostHardwareInfo/CSystemStatistics.h>
#include <prlxmlmodel/DiskImageInfo/CDiskImageInfo.h>
#include <prlxmlmodel/CtTemplate/CtTemplate.h>
#include <boost/function.hpp>

#include <boost/logic/tribool.hpp>

typedef boost::logic::tribool tribool_type;

#ifndef NETLINK_VZEVENT
#define NETLINK_VZEVENT         31
#endif

#define VZ_CT_CONFIG_FILE			"ve.conf"
#define VZ_CT_XML_CONFIG_FILE			".ve.xml"

typedef void (*state_event_handler_fn)(void *obj, const QString &, int);
typedef void (*notify_event_handler_fn)(void *obj, PRL_EVENT_TYPE type,
		const QString &sUuid, const QString &stage, int data);


#define MAX_CPUMASK	4096
#define CPUMASK_SIZE	(MAX_CPUMASK/sizeof(unsigned long))
struct CNodeMask {
	unsigned long avail_cpumask[CPUMASK_SIZE];
	unsigned long used_cpumask[CPUMASK_SIZE];

	int get_free_cpu();
};
// nodeid : struct CNodeMask
typedef QMap<unsigned, struct CNodeMask> NodemaskMap;

struct CEnvCpumask {
	unsigned nid;
	unsigned long reserved_ram;
	unsigned long cpumask[CPUMASK_SIZE];

	CEnvCpumask(unsigned _nid) : nid(_nid), reserved_ram(0)
	{
#ifdef _LIN_
		bzero(cpumask, sizeof(cpumask));
#endif
	}
	void set_reserved_ram(unsigned long ram) { reserved_ram = ram; }
};

namespace Statistics
{

///////////////////////////////////////////////////////////////////////////////
// struct Filesystem

struct Filesystem
{
	Filesystem() : total(0), free(0), index(0)
	{
	}

	quint64 total;
	quint64 free;
	quint64 index;
	QString device;
};

///////////////////////////////////////////////////////////////////////////////
// struct Disk

struct Disk {
	Disk() : read(0), write(0), index(0)
	{
	}

	// bytes
	quint64 read;
	quint64 write;
	int index;
};


} // namespace Statistics

namespace Ct
{
namespace Statistics
{

///////////////////////////////////////////////////////////////////////////////
// struct Cpu

struct Cpu {
	Cpu() : uptime(0), nice(0), user(0), system(0)
	{
	}

	// in microseconds
	quint64 uptime;
	quint64 nice;
	quint64 user;
	quint64 system;
};

///////////////////////////////////////////////////////////////////////////////
// struct Memory

struct Memory {
	Memory() : total(0), free(0), cached(0),
		swap_in(0), swap_out(0)
	{
	}
	// bytes
	quint64 total;
	quint64 free;
	quint64 cached;
	// count
	quint64 swap_in;
	quint64 swap_out;
};

namespace Network
{

///////////////////////////////////////////////////////////////////////////////
// struct Classfull

struct Classfull
{
	PRL_STAT_NET_TRAFFIC ipv4;
	PRL_STAT_NET_TRAFFIC ipv6;
	PRL_STAT_NET_TRAFFIC total;
};

///////////////////////////////////////////////////////////////////////////////
// struct Net

struct General
{
	General() : bytes_in(0), bytes_out(0), pkts_in(0), pkts_out(0), index(0)
	{
	}

	quint64 bytes_in;
	quint64 bytes_out;
	quint64 pkts_in;
	quint64 pkts_out;
	unsigned index;
};

} //namespace Network

///////////////////////////////////////////////////////////////////////////////
// struct Aggregate

struct Aggregate
{
	Cpu cpu;
	SmartPtr<Memory> memory;
	QList< ::Statistics::Disk> disk;
	Network::Classfull net;
	QList< ::Statistics::Filesystem> filesystem;
};

} // namespace Statistics
} // namespace Ct

struct CHwNetAdapter;
class CVzHelper {

public:
	CVzHelper();
	~CVzHelper();

	/***
	 * Check Vz status: 1 - running
	 *		    0 - not running
	 *		   -1 - error
	 */
	static int is_vz_running();
	static PRL_RESULT restart_shaper();

	static QString getVzPrivateDir(void);

	static QString get_cpu_mask(const SmartPtr<CVmConfiguration> &pVmConfig, bool bOvercommit);
	// public
	static void release_cpu_mask(const QString &uuid);
	// private
	static int autocalculate_cpumask(unsigned envid, unsigned ncpu,
			unsigned long ram, unsigned long *mask, int size);
	static Ct::Statistics::Network::Classfull *get_net_classfull_stat(const QString &id_);
	static int get_net_stat_by_dev(const QString &ctid, CVmGenericNetworkAdapter *dev, Ct::Statistics::Network::General& stat);
	static int get_net_stat(const SmartPtr<CVmConfiguration>& config, QList<Ct::Statistics::Network::General>& stat);
	static int update_network_classes_config(const CNetworkClassesConfig &conf);
	static int get_network_classes_config(CNetworkClassesConfig &conf);
	static int update_network_shaping_config(const CNetworkShapingConfig &conf);
	static int get_network_shaping_config(CNetworkShapingConfig &conf);
	static int set_rate(const CVmConfiguration &config, const CVmNetworkRates &lstRate);
	static SmartPtr<CVmConfiguration> get_env_config_by_ctid(const QString &ctid);
	static SmartPtr<CVmConfiguration> get_env_config(const QString &uuid);
	static SmartPtr<CVmConfiguration> get_env_config_from_file(const QString &sFile,
			int &err, int layout = 0,
			bool use_relative_path = false);
	static SmartPtr<CVmConfiguration> get_env_config_sample(const QString &name, int &err);
	/**
	 * Get paramater by name 'PARAM=VALUE'from global configuration file
	 * return:	-1 - open file error
			 0 - parameter found
			 1 = parameer not found
	 */
	static int get_vz_config_param(const char *param, QString &out);
	int get_envid_list(QStringList &lst);
	static int get_env_status_by_ctid(const QString &ctid, VIRTUAL_MACHINE_STATE &nState);
	static int get_env_status(const QString &uuid, VIRTUAL_MACHINE_STATE &nState);
	static tribool_type is_env_running(const QString &uuid);
	static Ct::Statistics::Aggregate* get_env_stat(const QString& uuid);
	static int get_env_disk_stat(const SmartPtr<CVmConfiguration>& config,
			QList<Statistics::Filesystem>& fs,
			QList<Statistics::Disk>& disk);
	static int set_env_uptime(const QString &uuid, const quint64 uptime, const QDateTime & date);
	static int reset_env_uptime(const QString &uuid);
	static int sync_env_uptime(const QString& uuid_);
	int set_vziolimit(const char *name);

	static int init_lib();

	/* Lock Container
	 * @return:     > 0 lock file descriptor
	 *              -1 locking error
	 *              -2 CT already locked
	 */
	static int lock_env(const QString &uuid, const char *status);
	/** Unlock Container.
	 *
	 * @param veid          id.
	 * @param lckfd         lock file descriptor
	 */
	static void unlock_env(const QString &uuid, int lockfd);
	static void update_ctid_map(const QString &uuid, const QString &ctid)
	{
		QMutexLocker lock(&s_mtxEnvUuidMap);
		if (ctid.isEmpty())
			s_envUuidMap.remove(uuid);
		else
			s_envUuidMap[uuid] = ctid;
	}

	static const QString get_ctid_by_uuid(const QString &uuid) 
	{
		QMutexLocker lock(&s_mtxEnvUuidMap);
		QHash<QString, QString>::const_iterator it = s_envUuidMap.find(uuid);
		if (it != s_envUuidMap.end())
			return it.value();

		return QString();
	}

	static const QString get_uuid_by_ctid(const QString &ctid)
	{
		QMutexLocker lock(&s_mtxEnvUuidMap);

		QHashIterator<QString, QString> it(s_envUuidMap);
		while (it.hasNext()) {
			it.next();
			if (it.value() == ctid)
				return it.key();
		}

		return QString();
	}

	static QString parse_ctid(const QString& src);
	static QString build_ctid_from_uuid(const QString& uuid);

private:

	static QMutex s_mtxEnvUuidMap;
	static QHash<QString, QString > s_envUuidMap;
};

struct callback_data
{
	int nInt;
	void *ptr;
	QString sData;

	callback_data() : nInt(0), ptr(0)
	{}
};
typedef void (* cleanup_callback_FN)(callback_data *);

struct callback_entry
{
        cleanup_callback_FN fn;
	callback_data data;

	callback_entry() : fn(NULL)
	{}
	callback_entry(cleanup_callback_FN _fn) :
		fn(_fn)
	{}
};

class CVzOperationCleaner {
private:
	QStack<callback_entry> m_stack;

public:
	void add(cleanup_callback_FN fn, const QString &data);
	void add(cleanup_callback_FN fn, int nInt);
	void add(cleanup_callback_FN fn, void *ptr);
	void pop();
	void process();
	static void kill_process(callback_data *data);
};

struct CProgressHepler : public QThread
{
	typedef boost::function2<void, const QString&, int> callback_type;

	CProgressHepler(const callback_type& callback_, int fd_)
		: m_callback(callback_), m_fd(fd_)
	{
	}

	virtual ~CProgressHepler()
	{
		if (m_fd != -1)
			close(m_fd);
	}

	virtual void run()
	{
		process_progress_evt();
	}

private:
	void process_progress_evt();

	callback_type m_callback;
	int m_fd;
};

typedef struct vzctl_snap_holder vzctl_snap_holder_t;

class CVzOperationHelper
{
public:
	CVzOperationHelper() :
		m_Rc(0),
		m_process_progress_evt(false),
		m_snap_holder(NULL)
	{}
	CVzOperationHelper(notify_event_handler_fn fn_, void *obj_) :
		m_Rc(0),
		m_notify(fn_),
		m_notifyObject(obj_),
		m_process_progress_evt(false),
		m_snap_holder(NULL)
	{}
	~CVzOperationHelper();
	void cancel_operation();
	int create_env(const QString &dst, SmartPtr<CVmConfiguration> &pConfig,
			PRL_UINT32 flags);
	int start_env(const QString &uuid, PRL_UINT32 nFlags);
	int pause_env(const QString &uuid);
	int restart_env(const QString &uuid);
	int stop_env(const QString &uuid, PRL_UINT32 nMode);
	int mount_env(const QString &uuid);
	int umount_env(const QString &uuid);
	Prl::Expected<QString, PRL_RESULT> get_env_mount_info(
			const SmartPtr<CVmConfiguration> &pConfig);
	int suspend_env(const QString &uuid);
	int resume_env(const QString &uuid, PRL_UINT32 flags);
	int delete_env(const QString &uuid);
	int delete_env(unsigned int id);
	int register_env(const QString &sPath, const QString &ctId,
			const QString &sUuid, const QString &sName,
			PRL_UINT32 nFlags, SmartPtr<CVmConfiguration> &pConfig);
	int register_env(const SmartPtr<CVmConfiguration> &pConfig, PRL_UINT32 nFlags);
	int unregister_env(const QString &sUuid, int flags);
	int set_env_userpasswd(const QString &uuid, const QString &user,
			const QString &pw, PRL_UINT32 nFlags);
	int auth_env_user(const QString &uuid, const QString &user, const QString &pw);
	int clone_env(const SmartPtr<CVmConfiguration> &pConfig, const QString &sNewHome,
			const QString &sNewName, PRL_UINT32 nFlags, SmartPtr<CVmConfiguration> &pNewConfig);
	int move_env(const QString &sUuid, const QString &sNewHome, const QString &sName);
	int set_env_name(const QString &uuid, const QString &name);
	int set_env_name(unsigned int id, const QString &name);
	int create_env_disk(const QString &uuid, const CVmHardDisk &disk);
	int create_disk_image(const QString &path, quint64 sizeBytes);
	int resize_env_disk(const QString &uuid, const QString &sPath,
			unsigned int nNewSize, unsigned int flags);
	int get_resize_env_info(const QString &uuid, CDiskImageInfo &di);
	const QString &get_error_msg() const { return m_sErrorMsg; }
	unsigned int get_rc() const { return m_Rc; }
	const QString &get_uuid() const { return m_sUuid; }
	void set_error_msg(const QString& qs) { m_sErrorMsg = qs; }

	CVzOperationHelper &get_op_helper() { return *this; }

	int update_env_uuid(const SmartPtr<CVmConfiguration> &pConfig,
			const SmartPtr<CVmConfiguration> &pOldConfig);
	int apply_env_config(SmartPtr<CVmConfiguration> &pConfig,
			SmartPtr<CVmConfiguration> &pOldConfig, unsigned int nFlags = 0);
	int del_env_disk(const QString &uuid, const CVmHardDisk &disk, unsigned int flags);
	int create_env_snapshot(const QString &uuid, const QString &snap_uuid,
			const QString &name, const QString &desc,
			PRL_UINT32 nFlags);
	int delete_env_snapshot(const QString &uuid, const QString &snap_uuid, bool bDelChild);
	int switch_env_snapshot(const QString &uuid, const QString &snap_uuid,
			PRL_UINT32 flags);
	int mount_disk_image(const QString &path, const QString &target, QString &dev);
	int create_env_private(const QString &ve_private, int layout = 5);
	void release_snapshot_holder();
	int alloc_snapshot_holder();
	int create_tsnapshot(const QString &guid, const QString &snap_guid,
			const QString &cbt_uuid, const char *component_name,
			const char *snap_dir);
	int delete_tsnapshot(const QString &uuid, const QString &snapsot_guid);
	int umount_snapshot(const QString &dev);
	int mount_disk_snapshot(const QString &path, const QString &snap_guid,
			const char *component_name, const QString &mnt_dir, QString &dev);
	int merge_snapshot(const QString &uuid, const QString &snapsot_guid);
	int remove_disks_from_env_config(SmartPtr<CVmConfiguration> &pConfig,
		SmartPtr<CVmConfiguration> &pOldConfig, const QString &sNewConfName);
	int get_env_netinfo(const QString &uuid, QList<CHwNetAdapter*> &adapters);
	int set_disk_encryption(const QString& uuid_, const CVmHardDisk& disk_,
		unsigned int flags_);
	int reinstall_env(const QString &uuid, const QString &os,
                PRL_UINT32 flags);

private:
	CVzOperationCleaner &get_cleaner() { return m_cleaner; }
	PRL_RESULT run_prg(const char *name, const QStringList &lstArgs, bool quiet = false);

	bool process_progress_evt() { return m_process_progress_evt; }

private:
	CVzOperationCleaner m_cleaner;
	QString m_sErrorMsg;
	unsigned int m_Rc;
	notify_event_handler_fn m_notify;
	void *m_notifyObject;
	QProcessEnvironment m_Envs;
	QString m_sUuid;
	bool m_process_progress_evt;
	SmartPtr<CProgressHepler> m_pProgressTask;

	vzctl_snap_holder_t *m_snap_holder;
};

class CVzStateMonitor
{
public:
	CVzStateMonitor() :
		m_kevt_fd(-1), m_bStopStatusMonitor(false)
	{}
	~CVzStateMonitor();
	void start(state_event_handler_fn, void *obj);
	void stop();

private:
	int m_kevt_fd;
	bool m_bStopStatusMonitor;
public:
	static state_event_handler_fn m_cb;
	static void * m_obj;
};

class CVzExecHelper
{
public:
	CVzExecHelper() :
		m_pid(-1), m_retcode(0)
	{}
private:
	char **make_argv(const QStringList &lst);
	void free_argv(char **argv);

public:
	int run_cmd(const QString &uuid,
			const QString &sPrgname,
			const QStringList &Args,
			const QStringList &Envss,
			PRL_UINT32 nFlags, int fds[3]);
	int wait();
	void cancel();
	int get_retcode() const { return m_retcode; }

private:
	int m_pid;
	int m_retcode;

};
#endif /*__CVZHELPER_H__ */
