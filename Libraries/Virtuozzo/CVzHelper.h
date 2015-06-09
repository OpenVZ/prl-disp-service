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
#include <QStack>
#include <QMutex>
#include <QProcessEnvironment>
#include <QThread>
#include <prlsdk/PrlIOStructs.h>

#ifdef _LIN_
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#endif

#include "Libraries/Std/SmartPtr.h"
#include <prlsdk/PrlEnums.h>
#include "XmlModel/NetworkConfig/CNetworkClassesConfig.h"
#include "XmlModel/NetworkConfig/CNetworkShapingConfig.h"
#include "XmlModel/VmConfig/CVmNetworkRates.h"
#include "XmlModel/VmConfig/CVmConfiguration.h"
#include "XmlModel/HostHardwareInfo/CSystemStatistics.h"
#include "XmlModel/DiskImageInfo/CDiskImageInfo.h"
#include "XmlModel/CtTemplate/CtTemplate.h"

#ifndef NETLINK_VZEVENT
#define NETLINK_VZEVENT         31
#endif

#define VZ_CT_CONFIG_FILE			"ve.conf"
#ifdef _WIN_
#define VZ_CT_XML_CONFIG_FILE			"ve.xml"
#else
#define VZ_CT_XML_CONFIG_FILE			".ve.xml"
#endif

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

// envid : struct CEnvNodeMask
typedef QMap<unsigned, struct CEnvCpumask> EnvCpumaskMap;

struct CNumaNode {
	NodemaskMap node;
	EnvCpumaskMap env_cpumask;
};

namespace Ct
{
namespace Statistics
{

///////////////////////////////////////////////////////////////////////////////
// struct Aggregate

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

///////////////////////////////////////////////////////////////////////////////
// struct Disk

struct Disk {
	Disk() : read(0), write(0)
	{
	}
	// bytes
	quint64 read;
	quint64 write;
};

struct Aggregate
{
	Cpu cpu;
	SmartPtr<Memory> memory;
	PRL_STAT_NET_TRAFFIC net;
	Disk disk;
};

} // namespace Statistics
} // namespace Ct

class CVzHelper {

public:
	CVzHelper();
	~CVzHelper();
	/***
	 * Cpuunits is CPU weight for a Vm. Argument is positive non-zero
	 * number, passed to and used in the kernel fair scheduler.
	 * The larger the number is, the more CPU time this container gets.
	 * Maximum value is 500000, minimal is 8.  Number is relative to
	 * weights of all the other running Vm.
	 */
	static int set_cpuunits(const QString &uuid, unsigned int units);

	/***
	 * Limit of CPU usage for the Vm, in per cent.
	 * Note if the computer has 2 CPUs, it has total of 200% CPU time.
	 * To drop cpu limit the 0 value should passed.
	 */
	static int set_cpulimit(const QString &uuid, float limit);

	static int set_cpumask(const QString &uuid, const QString &sCpuMask, unsigned int nVCpu);

	/***
	 * Assigns I/O priority. Priority range is 0-7. The greater priority is,
	 * the more time for I/O  activity Vm has. By default each Vm has priority of 4.
	 */
	static int set_ioprio(const QString &uuid, unsigned int prio);

	/***
	 * Assigns I/O limit in bytes per second.
	 */
	static int set_iolimit(const QString &uuid, unsigned int limit);

	/***
	 * Assigns IOPS limit
	 */
	static int set_iopslimit(const QString &uuid, unsigned int value);

	/***
	 * Check Vz status: 1 - running
	 *		    0 - not running
	 *		   -1 - error
	 */
	static int is_vz_running();

	static QString getVzPrivateDir(void);

	static QString get_cpu_mask(const SmartPtr<CVmConfiguration> &pVmConfig, bool bOvercommit);
#ifdef _WIN_
	static QString getVzRootDir(void);
	static QString getVzTemplateDir(void);
	static QString getVzInstallDir(void);

	static int add_privnet(unsigned int id, QList<QString> &ips);
	static int remove_privnet(unsigned int id);
	static int enable_privnet(bool enabled);

	static int getTemplateInfo(
		const QString &name,
		SmartPtr<CtTemplate> *ctTemplate = NULL,/* XmlModel template info */
		QString *installPath = NULL,		/* path to vzwin installed dir */
		QString *archivePath = NULL,		/* path to vzwin efd archive */
		bool *isInstalled = NULL,		/* installed dir is present */
		bool *isArchived = NULL			/* efd archive is present */
		);

	static QString getCtPrivatePath(unsigned int ctid);
	static QString getCtConfPath(unsigned int ctid);

	static int install_templates_env(QString env_uuid, QStringList &lstVzTmpl);
	static int remove_templates_env(QString env_uuid, QStringList &lstVzTmpl);

	static int get_templates(QList<SmartPtr<CtTemplate> > &lstVzTmpl);
	static int install_template(QString sName, QString sOsTmplName);
	static int remove_template(QString sName, QString sOsTmplName);
	static int is_ostemplate_exists(const QString &sOsTemplate);
	static int convert_os_ver(const QString& osname, const QString& osver);
	static int set_rate(ULONG envId, const CVmNetworkRates &lstRate);
	static int lock_env(unsigned int id, int state, int substate, void ** opaque);
	static int unlock_env(unsigned int id, void ** opaque);

	/* CT migration support */
	static int src_start_migrate_env(unsigned int id, void ** opaque);
	static int src_complete_migrate_env(unsigned int id, void ** opaque, bool fDelete);
	static int dst_start_migrate_env(unsigned int * id, void ** opaque);
	static int dst_complete_migrate_env(const QString& uuid, unsigned int id, unsigned int origin_id,
					    void ** opaque);

	static int store_config_env(unsigned int id); // copy conf to private
	static int restore_config_env(unsigned int id); // copy private to conf

#else
	// public
	static void release_cpu_mask(const QString &uuid);
	// private
	static int autocalculate_cpumask(unsigned envid, unsigned ncpu,
			unsigned long ram, unsigned long *mask, int size);
	static int numa_calculate_cpumask(unsigned envid, unsigned ncpu,
			struct CNodeMask &node, struct CEnvCpumask &env_cpumask,
			unsigned long *out, int size);
	static void numa_release_cpu_mask(unsigned envid);

	static QMutex s_mtxNodemask;
	static CNumaNode s_numanodes;
#endif

	static PRL_STAT_NET_TRAFFIC *get_net_stat(const QString &uuid);
	static int update_network_classes_config(const CNetworkClassesConfig &conf);
	static int get_network_classes_config(CNetworkClassesConfig &conf);
	static int update_network_shaping_config(const CNetworkShapingConfig &conf);
	static int get_network_shaping_config(CNetworkShapingConfig &conf);
	static int set_rate(const QString &uuid, const CVmNetworkRates &lstRate);

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
	static int get_env_status(const QString &uuid, VIRTUAL_MACHINE_STATE &nState);
	static Ct::Statistics::Aggregate* get_env_stat(const QString& uuid_);
	int set_env_uptime(const QString &uuid, const quint64 uptime, const QDateTime & date);
	int reset_env_uptime(const QString &uuid);
	static int sync_env_uptime(const QString& uuid_);
	int set_vziolimit(const char *name);

	static int init_lib();

	/* Lock Container
	 * @return:     > 0 lock file descriptor
	 *              -1 locking error
	 *              -2 CT already locked
	 */
	static int lock_env(unsigned int id, const char *status);
	/** Unlock Container.
	 *
	 * @param veid          id.
	 * @param lckfd         lock file descriptor
	 */
	static void unlock_env(unsigned int id, int lockfd);
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

class CProgressHepler : public QThread {
#ifdef _LIN_
public:
	CProgressHepler(notify_event_handler_fn fn, void *obj) :
		m_fd(-1),
		m_notify(fn),
		m_notify_obj(obj)
	{}
	void set_param(const QString &sUuid, int fd)
	{
		m_sUuid = sUuid;
		m_fd = fd;
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
	void send_progress_evt(const QString &stage, int progress);

private:
	int m_fd;
	QString m_sUuid;
	notify_event_handler_fn m_notify;
	void *m_notify_obj;
#else
public:
	CProgressHepler(notify_event_handler_fn, void *) {}
#endif
};

#if _LIN_
typedef struct vzctl_snap_holder vzctl_snap_holder_t;
#else
typedef void vzctl_snap_holder_t;
#endif

class CVzOperationHelper
{
public:
	CVzOperationHelper() :
		m_Rc(0),
		m_process_progress_evt(false),
		m_snap_holder(NULL)
	{}
	CVzOperationHelper(notify_event_handler_fn fn, void *obj) :
		m_Rc(0),
		m_pProgressTask(new CProgressHepler(fn, obj)),
		m_process_progress_evt(false),
		m_snap_holder(NULL)
	{}
	~CVzOperationHelper();
	void cancel_operation();
	int create_env(const QString &dst, SmartPtr<CVmConfiguration> &pConfig,
			PRL_UINT32 flags);
	int start_env(const QString &uuid, PRL_UINT32 nFlags);
	int restart_env(const QString &uuid);
	int stop_env(const QString &uuid, PRL_UINT32 nMode);
	int mount_env(const QString &uuid);
	int umount_env(const QString &uuid);
	int suspend_env(const QString &uuid);
	int resume_env(const QString &uuid, PRL_UINT32 flags);
	int delete_env(const QString &uuid);
	int delete_env(unsigned int id);
	int register_env(const QString &sPath, const QString &sUuid,
			PRL_UINT32 nFlags, SmartPtr<CVmConfiguration> &pConfig);
	int unregister_env(const QString &sUuid, int flags);
	int set_env_userpasswd(const QString &uuid, const QString &user,
			const QString &pw, PRL_UINT32 nFlags);
	int auth_env_user(const QString &uuid, const QString &user, const QString &pw);
	int clone_env(const QString &uuid, const QString &sNewHome, const QString &sNewName,
			PRL_UINT32 nFlags,  SmartPtr<CVmConfiguration> &pNewConfig);
	int move_env(const QString &sNewHome, const QString &sName, const QString &sSrcCtid);
	int convert2_env(const QString &srcPath, const QString &dstPath, unsigned int layout);
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

	int apply_env_config(SmartPtr<CVmConfiguration> &pConfig,
			SmartPtr<CVmConfiguration> &pOldConfig, unsigned int nFlags = 0);
#ifdef _WIN_
	static PRL_RESULT run_prg(const char *name, const QStringList &lstArgs,
				  CVmEvent *pEvent = NULL);
	// real method: just apply diff between old and new config
	int apply_env_config_internal(SmartPtr<CVmConfiguration> &pConfig,
			SmartPtr<CVmConfiguration> &pOldConfig,
			unsigned int nFlags);
#endif
#ifdef _LIN_
	int del_env_disk(const QString &uuid, const CVmHardDisk &disk, unsigned int flags);
	int create_env_snapshot(const QString &uuid, const QString &snap_uuid,
			const QString &name, const QString &desc);
	int delete_env_snapshot(const QString &uuid, const QString &snap_uuid, bool bDelChild);
	int switch_env_snapshot(const QString &uuid, const QString &snap_uuid,
			PRL_UINT32 flags);
	int mount_disk_image(const QString &path, const QString &target, QString &dev);
	int create_env_private(const QString &ve_private, int layout = 5);
	void release_snapshot_holder();
	int alloc_snapshot_holder();
	int create_tsnapshot(const QString &guid, const QString &snap_guid,
			const char *component_name, const char *snap_dir);
	int delete_tsnapshot(const QString &uuid, const QString &snapsot_guid);
	int umount_snapshot(const QString &dev);
	int mount_disk_snapshot(const QString &path, const QString &snap_guid,
			const char *component_name, const QString &mnt_dir, QString &dev);
	int merge_snapshot(const QString &uuid, const QString &snapsot_guid);
	int remove_disks_from_env_config(SmartPtr<CVmConfiguration> &pConfig,
		SmartPtr<CVmConfiguration> &pOldConfig, const QString &sNewConfName);
#endif
private:
	CVzOperationCleaner &get_cleaner() { return m_cleaner; }
#ifdef _LIN_
	PRL_RESULT run_prg(const char *name, const QStringList &lstArgs, bool quiet = false);
#endif

	bool process_progress_evt() { return m_process_progress_evt; }

private:
	CVzOperationCleaner m_cleaner;
	QString m_sErrorMsg;
	unsigned int m_Rc;
	QProcessEnvironment m_Envs;
	QString m_sUuid;
	SmartPtr<CProgressHepler> m_pProgressTask;
	bool m_process_progress_evt;

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

#ifdef _LIN_
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
#endif

#ifdef _WIN_
enum {
	VZCTL_ENV_NONE,
	VZCTL_ENV_STARTED,
	VZCTL_ENV_STOPPED,
	VZCTL_ENV_DELETED,
	VZCTL_ENV_CREATED,
	VZCTL_ENV_REGISTERED,
	VZCTL_ENV_UNREGISTERED,
	VZCTL_ENV_SUSPENDED,
};

enum {
	VZ_REG_FORCE            = 0x01, /* force reg: skip owner check */
	VZ_REG_SKIP_OWNER       = 0x02, /* Skip .owner update on register */
	VZ_REG_SKIP_CLUSTER     = 0x04, /* Skip .cluster_service_name
					   update on register */
	VZ_REG_RENEW            = 0x08, /* renew registration */
	VZ_UNREG_PRESERVE       = 0x10, /* remove only VEID.conf preserve all
					   data under VE_PRIVATE */
};
#endif

#endif /*__CVZHELPER_H__ */
