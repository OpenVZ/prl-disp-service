///////////////////////////////////////////////////////////////////////////////
///
/// @file Task_RestoreVmBackup.h
///
/// Dispatcher source-side task for Vm backup creation
///
/// @author krasnov@
///
/// Copyright (c) 2005-2017, Parallels International GmbH
/// Copyright (c) 2017-2019 Virtuozzo International GmbH, All rights reserved.
///
/// This file is part of Virtuozzo Core. Virtuozzo Core is free
/// software; you can redistribute it and/or modify it under the terms
/// of the GNU General Public License as published by the Free Software
/// Foundation; either version 2 of the License, or (at your option) any
/// later version.
///
/// This program is distributed in the hope that it will be useful,
/// but WITHOUT ANY WARRANTY; without even the implied warranty of
/// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
/// GNU General Public License for more details.
///
/// You should have received a copy of the GNU General Public License
/// along with this program; if not, write to the Free Software
/// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
/// 02110-1301, USA.
///
/// Our contact details: Virtuozzo International GmbH, Vordergasse 59, 8200
/// Schaffhausen, Switzerland.
///
///////////////////////////////////////////////////////////////////////////////

#ifndef __Task_RestoreVmBackup_H_
#define __Task_RestoreVmBackup_H_

#include <QString>
#include <QDateTime>
#include <memory>
#include "CDspTaskHelper.h"
#include "CDspRegistry.h"
#include "CDspClient.h"
#include "CDspVmConfigManager.h"
#include "prlxmlmodel/VmConfig/CVmConfiguration.h"
#include <prlcommon/ProtoSerializer/CProtoCommands.h>
#include "prlcommon/IOService/IOCommunication/IOClient.h"
#include "Libraries/DispToDispProtocols/CDispToDispCommonProto.h"
#include "CDspDispConnection.h"
#include "Task_BackupHelper.h"
#include "CDspVzHelper.h"
#include "Legacy/VmConverter.h"

class Task_RestoreVmBackupTarget;

namespace Backup
{
namespace Tunnel
{
namespace Source
{
struct Factory;

} // namespace Source
} // namespace Tunnel
} // namespace Backup

namespace Restore
{
struct Move;
struct Converter;
///////////////////////////////////////////////////////////////////////////////
// struct Assembly

struct Assembly
{
	typedef boost::function<void (const QString& )> trashPolicy_type;

	explicit Assembly(CAuthHelper& auth_);
	~Assembly();

	PRL_RESULT do_();
	void revert();
	void addExternal(const QFileInfo& src_, const QString& dst_);
	void addEssential(const QString& src_, const QString& dst_);
	void adopt(const trashPolicy_type& trashPolicy_)
	{
		m_trashPolicy = trashPolicy_;
	}

private:
	CAuthHelper* m_auth;
	QList<Move* > m_ready;
	QList<Move* > m_pending;
	QStringList m_trash;
	trashPolicy_type m_trashPolicy;
};

namespace Source
{
struct Archive;

} // namespace Source

namespace Target
{
namespace Activity
{
typedef SmartPtr<CVmConfiguration> config_type;

///////////////////////////////////////////////////////////////////////////////
// struct Product

struct Product: private boost::tuple<QString, QString, QString, config_type>
{
	typedef boost::function<bool ()> new_type;
	typedef boost::function<const QString& ()> uuid_type;

	Product(const uuid_type& uuid_, const new_type& new_): m_new(new_), m_uuid(uuid_)
	{
	}

	bool isNew() const
	{
		return !(m_new.empty() || m_new());
	}
	const QString& getHome() const
	{
		return get<0>();
	}
	void setHome(const QString& value_)
	{
		get<0>() = value_;
	}
	const QString& getName() const
	{
		return get<1>();
	}
	void setName(const QString& value_)
	{
		get<1>() = value_;
	}
	const QString& getUuid() const
	{
		return m_uuid();
	}
	const config_type& getConfig() const
	{
		return get<3>();
	}
	void setConfig(const config_type& value_)
	{
		get<3>() = value_;
	}
	void cloneConfig(const QString& name_);

private:
	new_type m_new;
	uuid_type m_uuid;
};

///////////////////////////////////////////////////////////////////////////////
// struct Unit

struct Unit: private Product
{
	typedef Task_RestoreVmBackupTarget context_type;
	
	Unit(const Product& product_, context_type& context_):
		Product(product_), m_context(&context_)
	{
	}

	context_type& getContext() const
	{
		return *m_context;
	}
	const Product& getProduct() const
	{
		return *this;
	}
	const QString& getObjectUuid() const;
	Activity::config_type getObjectConfig() const;
	PRL_RESULT saveProductConfig(const QString& folder_);

private:
	context_type* m_context;
};

} // namespace Activity

///////////////////////////////////////////////////////////////////////////////
// struct Driver

struct Driver: private Activity::Unit
{
	explicit Driver(const Activity::Unit& activity_): Activity::Unit(activity_)
	{
	}

	PRL_RESULT define(const Activity::config_type& object_);
	void undefine();
	PRL_RESULT start();
	void stop();
	PRL_RESULT rebase();
};

///////////////////////////////////////////////////////////////////////////////
// struct Enrollment

struct Enrollment: private Activity::Unit
{
	Enrollment(const Activity::Unit& activity_, Registry::Public& registry_):
		Activity::Unit(activity_), m_registry(&registry_)
	{
	}

	PRL_RESULT execute();
	PRL_RESULT rollback();

private:
	Registry::Public* m_registry;	
};

///////////////////////////////////////////////////////////////////////////////
// struct Factory

struct Factory
{
	typedef boost::function<PRL_RESULT ()> escort_type;
	typedef Target::Driver driver_type;
	typedef Activity::Unit activity_type;
	typedef Enrollment enrollment_type;

	virtual ~Factory();

	virtual escort_type craftEscort() = 0;
	virtual driver_type craftDriver() = 0;
	virtual activity_type craftActivity() = 0;
	virtual enrollment_type craftEnrollment() = 0;
};

namespace Escort
{
///////////////////////////////////////////////////////////////////////////////
// struct Gear

struct Gear: QObject
{
	typedef SmartPtr<CVmFileListCopyTarget> transport_type;

	Gear(const transport_type& transport_, IOClient& io_);

	PRL_RESULT operator()();

private slots:
	void react(const SmartPtr<IOPackage> package_);

private:
	Q_OBJECT

	IOClient* m_io;
	QEventLoop m_loop;
	SmartPtr<CVmFileListCopyTarget> m_transport;
};

} // namespace Escort
} // namespace Target
} // namespace Restore

class Task_RestoreVmBackupSource : public Task_BackupHelper
{
	Q_OBJECT

	typedef QPair< QString,
		QSharedPointer< ::Restore::Source::Archive> > archive_type;

public:
	Task_RestoreVmBackupSource(
		SmartPtr<CDspDispConnection> &,
		const CDispToDispCommandPtr,
		const SmartPtr<IOPackage> &);

protected:
	virtual PRL_RESULT prepareTask();
	virtual PRL_RESULT run_body();
	virtual void finalizeTask();

private:
	SmartPtr<CDspDispConnection> m_pDispConnection;
	QString m_sVmName;
	QString m_sBackupId;
	QString m_sBackupUuid;
	QString m_sBackupPath;
	quint64 m_nTotalSize;
	quint32 m_nBackupNumber;
	IOServerInterface_Client &m_ioServer;
	IOSender::Handle m_hHandle;
	SmartPtr<CVmFileListCopySource> m_pVmCopySource;
	SmartPtr<CVmFileListCopySender> m_pSender;
	bool m_bBackupLocked;
	QString m_sBackupRootPath;
 
	WaiterTillHandlerUsingObject m_waiter;

	QList<archive_type> m_nbds;

private:
	PRL_RESULT restore(const ::Backup::Work::object_type& variant_);
	PRL_RESULT restoreVmABackup(SmartPtr<CVmConfiguration> ve_);
	PRL_RESULT restoreVmVBackup(SmartPtr<CVmConfiguration> ve_);
	PRL_RESULT sendFiles(IOSendJob::Handle& job_);
	PRL_RESULT sendStartReply(const SmartPtr<CVmConfiguration>& ve_, IOSendJob::Handle& job_);
	PRL_RESULT getBackupParams(quint64 &nSize, quint32 &nBundlePermissions);
	/* To get directory and file lists for start path with excludes.
	   This function does not clean directory and file lists before.
	   excludeFunc will get relative path and should return 'true' to
           exclude obj from dirList or fileList */
	PRL_RESULT getEntryLists();

private slots:
	void mountImage(const SmartPtr<IOPackage>& package_);
	void clientDisconnected(IOSender::Handle h);
	void handleABackupPackage(IOSender::Handle h, const SmartPtr<IOPackage> p);
	void handleVBackupPackage(IOSender::Handle h, const SmartPtr<IOPackage> p);
};

class Task_RestoreVmBackupTarget : public Task_BackupHelper, Restore::Target::Factory
{
	Q_OBJECT

public:
	Task_RestoreVmBackupTarget(
		Registry::Public&,
		SmartPtr<CDspClient> &,
		CProtoCommandPtr,
		const SmartPtr<IOPackage> &);
	virtual ~Task_RestoreVmBackupTarget();

	::Backup::Tunnel::Source::Factory craftTunnel();
	Prl::Expected<QString, PRL_RESULT> sendMountImageRequest(const QString&);
	const QString& getOriginVmUuid() const
	{
		return m_sOriginVmUuid;
	}

protected:
	virtual PRL_RESULT prepareTask();
	virtual PRL_RESULT run_body();
	virtual void finalizeTask();

private:
	PRL_RESULT getFiles(bool bVmExist_);
	PRL_RESULT sendStartRequest();
	PRL_RESULT saveVmConfig();
	virtual void cancelOperation(SmartPtr<CDspClient> pUser, const SmartPtr<IOPackage>& pkg);
	PRL_RESULT fixHardWareList();
	PRL_RESULT doV2V();

	PRL_RESULT restoreVm();
	PRL_RESULT restoreCt();

	PRL_RESULT restoreVmOverExisting();
	PRL_RESULT restoreNewVm();
	PRL_RESULT restoreVmToTargetPath(std::auto_ptr<Restore::Assembly>& dst_);
#ifdef _CT_
	PRL_RESULT restoreCtOverExisting(const SmartPtr<CVmConfiguration> &pConfig);
	PRL_RESULT restoreNewCt(const QString &sDefaultCtFolder);
	PRL_RESULT restoreCtToTargetPath(
			bool bIsRealMountPoint,
			std::auto_ptr<Restore::Assembly>& dst_);
#endif
	PRL_RESULT lockExclusiveVmParameters(SmartPtr<CVmDirectory::TemporaryCatalogueItem> pInfo);

private:
	// Restore::Target::Factory part
	escort_type craftEscort();
	driver_type craftDriver();
	activity_type craftActivity();
	enrollment_type craftEnrollment();
	
private:
	Registry::Public& m_registry;
	QString m_sOriginVmUuid;
	QString m_sBackupId;
	SmartPtr<CVmFileListCopyTarget> m_pVmCopyTarget;
	SmartPtr<CVmFileListCopySender> m_pSender;
	bool m_bVmExist;
	QString m_sTargetPath;
	Restore::Target::Activity::Product m_product;
	QString m_sTargetStorageId;
	quint32 m_nBundlePermissions;
	//https://bugzilla.sw.ru/show_bug.cgi?id=464218
	//VM uptime values before restore
	quint64 m_nCurrentVmUptime;
	QDateTime m_current_vm_uptime_start_date;
	SmartPtr<IOPackage> m_pReply;

#ifdef _CT_
	CVzOperationHelper m_VzOpHelper;
#endif
 	QString m_sVzCacheDir;
 	QString m_sVzCacheTmpDir;
 
	WaiterTillHandlerUsingObject m_waiter;
	std::auto_ptr<Legacy::Vm::Converter> m_converter;

private slots:
	void runV2V();
};

#endif //__Task_RestoreVmBackup_H_
