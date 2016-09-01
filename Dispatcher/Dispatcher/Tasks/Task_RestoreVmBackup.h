///////////////////////////////////////////////////////////////////////////////
///
/// @file Task_RestoreVmBackup.h
///
/// Dispatcher source-side task for Vm backup creation
///
/// @author krasnov@
///
/// Copyright (c) 2005-2015 Parallels IP Holdings GmbH
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
/// Our contact details: Parallels IP Holdings GmbH, Vordergasse 59, 8200
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
#include "Libraries/ProtoSerializer/CProtoCommands.h"
#include "prlcommon/IOService/IOCommunication/IOClient.h"
#include "Libraries/DispToDispProtocols/CDispToDispCommonProto.h"
#include "CDspDispConnection.h"
#include "Task_BackupHelper.h"
#include "CDspVzHelper.h"
#include "Legacy/VmConverter.h"

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
	explicit Assembly(CAuthHelper& auth_): m_auth(&auth_)
	{
	}
	~Assembly();

	PRL_RESULT do_();
	void revert();
	void addExternal(const QFileInfo& src_, const QString& dst_);
	void addEssential(const QString& src_, const QString& dst_);
private:
	CAuthHelper* m_auth;
	QList<Move* > m_ready;
	QList<Move* > m_pending;
	QStringList m_trash;
};

} // namespace Restore

class Task_RestoreVmBackupSource : public Task_BackupHelper
{
	Q_OBJECT

	typedef QPair< QString,
		QSharedPointer< ::Backup::Storage::Nbd> > archive_type;

public:
	Task_RestoreVmBackupSource(
		SmartPtr<CDspDispConnection> &,
		const CDispToDispCommandPtr,
		const SmartPtr<IOPackage> &);
	virtual ~Task_RestoreVmBackupSource();

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
	quint32 m_nInternalFlags;
 
	WaiterTillHandlerUsingObject m_waiter;

	QList<archive_type> m_nbds;

private:
	PRL_RESULT restore(const ::Backup::Work::object_type& variant_);
	PRL_RESULT restoreVmABackup(SmartPtr<CVmConfiguration> ve_);
	PRL_RESULT restoreVmVBackup(SmartPtr<CVmConfiguration> ve_);
	PRL_RESULT sendFiles(IOSendJob::Handle& job_);
	PRL_RESULT sendStartReply(const SmartPtr<CVmConfiguration>& ve_, IOSendJob::Handle& job_);
	void mountImage(const SmartPtr<IOPackage> p_);

private slots:
	void clientDisconnected(IOSender::Handle h);
	void handleABackupPackage(IOSender::Handle h, const SmartPtr<IOPackage> p);
	void handleVBackupPackage(IOSender::Handle h, const SmartPtr<IOPackage> p);
};

class Task_RestoreVmBackupTarget : public Task_BackupHelper
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

protected:
	virtual PRL_RESULT prepareTask();
	virtual PRL_RESULT run_body();
	virtual void finalizeTask();

private:
	PRL_RESULT getFiles(bool bVmExist_);
	PRL_RESULT sendStartRequest();
	PRL_RESULT saveVmConfig();
	PRL_RESULT registerVm();
	PRL_RESULT unregisterVm();
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
			const QString &sCtName,
			bool bIsRealMountPoint,
			std::auto_ptr<Restore::Assembly>& dst_);
#endif
	PRL_RESULT lockExclusiveVmParameters(SmartPtr<CVmDirectory::TemporaryCatalogueItem> pInfo);

private:
	Registry::Public& m_registry;
	SmartPtr<CVmConfiguration> m_pVmConfig;
	QString m_sOriginVmUuid;
	QString m_sBackupId;
	QString m_sBackupUuid;
	quint32 m_nBackupNumber;
	SmartPtr<CVmFileListCopyTarget> m_pVmCopyTarget;
	SmartPtr<CVmFileListCopySender> m_pSender;
	bool m_bVmExist;
	QString m_sTargetPath;
	QString m_sTargetVmHomePath;
	QString m_sTargetVmName;
	QString m_sTargetStorageId;
	QString m_sBackupRootPath;
	quint64 m_nOriginalSize;
	quint32 m_nBundlePermissions;
	//https://bugzilla.sw.ru/show_bug.cgi?id=464218
	//VM uptime values before restore
	quint64 m_nCurrentVmUptime;
	QDateTime m_current_vm_uptime_start_date;
	quint32 m_nInternalFlags;
	SmartPtr<IOPackage> m_pReply;

#ifdef _CT_
	CVzOperationHelper m_VzOpHelper;
#endif
 	QString m_sVzCacheDir;
 	QString m_sVzCacheTmpDir;
 
	WaiterTillHandlerUsingObject m_waiter;
	std::auto_ptr<Legacy::Vm::Converter> m_converter;

private slots:
	void handlePackage(const SmartPtr<IOPackage> p);
	void runV2V();
};

#endif //__Task_RestoreVmBackup_H_
