///////////////////////////////////////////////////////////////////////////////
///
/// @file Task_RestoreVmBackup.cpp
///
/// Source task for Vm backup restoring
///
/// @author krasnov@
///
/// Copyright (c) 2005-2017, Parallels International GmbH
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
/// Our contact details: Parallels International GmbH, Vordergasse 59, 8200
/// Schaffhausen, Switzerland.
///
///////////////////////////////////////////////////////////////////////////////

//#define LOGGING_ON
//#define FORCE_LOGGING_LEVEL DBG_DEBUG

#include <QProcess>

#include "Interfaces/Debug.h"
#include "prlcommon/Interfaces/ParallelsQt.h"
#include "prlcommon/Interfaces/ParallelsNamespace.h"
#include "prlcommon/HostUtils/HostUtils.h"
#include "Task_RestoreVmBackup_p.h"
#include "prlcommon/Logging/Logging.h"
#include "prlcommon/PrlUuid/Uuid.h"

#include "Task_RestoreVmBackup.h"
#include "Task_BackupHelper_p.h"
#include "Libraries/DispToDispProtocols/CVmBackupProto.h"
#include "CDspService.h"
#include "prlcommon/Std/PrlAssert.h"
#include "Libraries/PrlCommonUtils/CFileHelper.h"
#include "prlxmlmodel/BackupTree/VmItem.h"
#include "Libraries/Virtuozzo/CVzHelper.h"
#include "CDspBackupDevice.h"
#ifdef _CT_
#include "vzctl/libvzctl.h"
#endif

namespace Restore
{
namespace Source
{
///////////////////////////////////////////////////////////////////////////////
// struct Workerv4

PRL_RESULT Workerv4::operator()(Task_RestoreVmBackupSource* task_)
{
	if (NULL == task_)
		return PRL_ERR_INVALID_ARG;

	// send escort files
	PRL_RESULT e = m_escort();
	if (PRL_FAILED(e))
		return e;

	return m_exec(task_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Workerv3

Workerv3::~Workerv3()
{
	if (m_process) {
		m_process->kill();
		m_process->waitForFinished();
	}
}

PRL_RESULT Workerv3::operator()()
{
	QStringList args = QStringList() << QString(PRL_ABACKUP_SERVER);
	PRL_RESULT e = m_process->start(args, BACKUP_PROTO_V3);
	if (PRL_FAILED(e))
		return e;

	// send escort files
	if (PRL_FAILED(e = m_escort()))
		return e;

	e = m_process->waitForFinished();
	m_process = NULL;
	return e;
}

} // namespace Source
} // namespace Restore

/*******************************************************************************

 Backup restore task for server

********************************************************************************/
Task_RestoreVmBackupSource::Task_RestoreVmBackupSource(
		SmartPtr<CDspDispConnection> &pDispConnection,
		const CDispToDispCommandPtr cmd,
		const SmartPtr<IOPackage>& p)
:Task_BackupHelper(pDispConnection->getUserSession(), p),
m_pDispConnection(pDispConnection),
m_ioServer(CDspService::instance()->getIOServer()),
m_bBackupLocked(false)
{
	CVmBackupRestoreCommand *pCmd =
		CDispToDispProtoSerializer::CastToDispToDispCommand<CVmBackupRestoreCommand>(cmd);
	m_sVmUuid = pCmd->GetVmUuid();
	m_sBackupId = pCmd->GetBackupUuid();
	m_nBackupNumber = PRL_BASE_BACKUP_NUMBER;
	m_nFlags = pCmd->GetFlags();
	m_nTotalSize = 0;
	m_hHandle = m_pDispConnection->GetConnectionHandle();
	m_nInternalFlags = 0;
	m_nRemoteVersion = pCmd->GetVersion();
	bool bConnected = QObject::connect(
		&CDspService::instance()->getIOServer(),
		SIGNAL(onClientDisconnected(IOSender::Handle)),
		SLOT(clientDisconnected(IOSender::Handle)),
		Qt::DirectConnection);
	PRL_ASSERT(bConnected);
}

Task_RestoreVmBackupSource::~Task_RestoreVmBackupSource()
{
	QObject::disconnect(
		&CDspService::instance()->getIOServer(),
		SIGNAL(onClientDisconnected(IOSender::Handle)),
		this,
		SLOT(clientDisconnected(IOSender::Handle))
		);

	// #439777 to protect call handler for destroying object
	m_waiter.waitUnlockAndFinalize();
}

PRL_RESULT Task_RestoreVmBackupSource::prepareTask()
{
	PRL_RESULT nRetCode = PRL_ERR_SUCCESS;
	Prl::Expected<VmItem, PRL_RESULT> x;

	if (m_sBackupId.isEmpty()) {
		if (m_sVmUuid.isEmpty()) {
			WRITE_TRACE(DBG_FATAL, "[%s] Vm uuid and backup id are empty", __FUNCTION__);
			nRetCode = PRL_ERR_BACKUP_RESTORE_INTERNAL_PROTO_ERROR;
			goto exit;
		}
		/* restore from last incremental of last backup if backup id does not specified (#420596) */
		BackupItem* b = getLastBaseBackup(m_sVmUuid, &getClient()->getAuthHelper(), PRL_BACKUP_CHECK_MODE_READ);
		if (NULL == b) {
			nRetCode = PRL_ERR_BACKUP_BACKUP_NOT_FOUND;
			CVmEvent *pEvent = getLastError();
			pEvent->setEventCode(nRetCode);
			pEvent->addEventParameter(new CVmEventParameter(
				PVE::String, m_sVmUuid, EVT_PARAM_MESSAGE_PARAM_0));
			WRITE_TRACE(DBG_FATAL, "Could not find any backup of the Vm %s", QSTR2UTF8(m_sVmUuid));
			goto exit;
		}
		m_sBackupUuid = b->getUuid();
		delete b;
		m_nBackupNumber = getCatalog(m_sVmUuid).getSequence(m_sBackupUuid).getIndex().last();
	} else {
		if (PRL_FAILED(parseBackupId(m_sBackupId, m_sBackupUuid, m_nBackupNumber))) {
			nRetCode = PRL_ERR_BACKUP_BACKUP_UUID_NOT_FOUND;
			CVmEvent *pEvent = getLastError();
			pEvent->setEventCode(nRetCode);
			pEvent->addEventParameter(new CVmEventParameter(
				PVE::String, m_sBackupUuid, EVT_PARAM_MESSAGE_PARAM_0));
			WRITE_TRACE(DBG_FATAL, "Invalid backup id \"%s\"", QSTR2UTF8(m_sBackupId));
			goto exit;
		}
	}

	if (m_sVmUuid.isEmpty()) {
		if (PRL_FAILED(findVmUuidForBackupUuid(m_sBackupUuid, m_sVmUuid))) {
			nRetCode = PRL_ERR_BACKUP_BACKUP_UUID_NOT_FOUND;
			CVmEvent *pEvent = getLastError();
			pEvent->setEventCode(nRetCode);
			pEvent->addEventParameter(new CVmEventParameter(
					PVE::String, m_sBackupUuid, EVT_PARAM_MESSAGE_PARAM_0));
			WRITE_TRACE(DBG_FATAL, "Backup \"%s\" does not exist", QSTR2UTF8(m_sBackupUuid));
			goto exit;
		}
	}
	m_sBackupPath = getCatalog(m_sVmUuid).getSequence(m_sBackupUuid)
		.showItemLair(m_nBackupNumber).absolutePath();
	m_sBackupRootPath = getCatalog(m_sVmUuid).getSequence(m_sBackupUuid)
		.showLair().absolutePath();

	/* to check access before */
	if (!CFileHelper::FileCanRead(m_sBackupRootPath, &getClient()->getAuthHelper())) {
		nRetCode = PRL_ERR_BACKUP_BACKUP_UUID_NOT_FOUND;
		CVmEvent *pEvent = getLastError();
		pEvent->setEventCode(nRetCode);
		pEvent->addEventParameter(new CVmEventParameter(
				PVE::String, m_sBackupUuid, EVT_PARAM_MESSAGE_PARAM_0));
		WRITE_TRACE(DBG_FATAL, "User %s have not permissions for restore backup %s",
			QSTR2UTF8(getClient()->getAuthHelper().getUserName()), QSTR2UTF8(m_sBackupUuid));
		goto exit;
	}

	/* to get vmtype */
	x = getCatalog(m_sVmUuid).loadItem();
	if (x.isFailed())
	{
		nRetCode = x.error();
		goto exit;
	}
	m_sVmName = x.value().getName();
	if (x.value().getVmType() == PVBT_CT_VZFS)
		m_nInternalFlags |= PVM_CT_VZFS_BACKUP;
	else if (x.value().getVmType() == PVBT_CT_PLOOP)
		m_nInternalFlags |= PVM_CT_PLOOP_BACKUP;
	else if (x.value().getVmType() == PVBT_CT_VZWIN)
		m_nInternalFlags |= PVM_CT_VZWIN_BACKUP;
	m_nRemoteVersion = x.value().getVersion();
	if (m_nRemoteVersion == 0)
		m_nRemoteVersion = BACKUP_PROTO_V3;

	/* to lock backup */
	if (PRL_FAILED(nRetCode = getMetadataLock().grabShared(m_sBackupUuid)))
		goto exit;
	m_bBackupLocked = true;
#ifdef _LIN_
	/* check that the backup is not attached to some VM - otherwise we'll get
	 * acronis 'Access Denied' error */
	if (Backup::Device::isAttached(m_sBackupUuid)) {
		WRITE_TRACE(DBG_FATAL, "backup '%s' is attached to VM, restore is not possible",
			QSTR2UTF8(m_sBackupUuid));
		nRetCode = PRL_ERR_BACKUP_RESTORE_PROHIBIT_WHEN_ATTACHED;
		getLastError()->setEventCode(nRetCode);
		getLastError()->addEventParameter(new CVmEventParameter(
				PVE::String, m_sBackupUuid, EVT_PARAM_MESSAGE_PARAM_0));
	}
#endif // _LIN_
exit:
	setLastErrorCode(nRetCode);
	return nRetCode;
}

static bool excludeFunc(const QString &sRelPath)
{
	if (sRelPath == PRL_BACKUP_METADATA)
		return true; /* skip metadata */
	return false;
}

PRL_RESULT Task_RestoreVmBackupSource::run_body()
{
	//https://bugzilla.sw.ru/show_bug.cgi?id=267152
	CAuthHelperImpersonateWrapper _impersonate( &getClient()->getAuthHelper() );

	PRL_RESULT nRetCode = PRL_ERR_SUCCESS;

	if (m_nInternalFlags & PVM_CT_BACKUP)
		nRetCode = restore(::Backup::Work::Ct(*this));
	else
		nRetCode = restore(::Backup::Work::Vm(*this));

	setLastErrorCode(nRetCode);
	return nRetCode;
}

void Task_RestoreVmBackupSource::finalizeTask()
{
	m_cABackupServer.kill();

	foreach(const archive_type& f, m_nbds)
		f.second->stop();

	if (m_bBackupLocked)
		getMetadataLock().releaseShared(m_sBackupUuid);
	m_bBackupLocked = false;

	if (PRL_FAILED(getLastErrorCode()))
		m_pDispConnection->sendResponseError(getLastError(), getRequestPackage());
}

PRL_RESULT Task_RestoreVmBackupSource::sendFiles(IOSendJob::Handle& job_)
{
	/* wait client's reply for syncronization */
	if (m_ioServer.waitForResponse(job_, m_nTimeout) != IOSendJob::Success) {
		WRITE_TRACE(DBG_FATAL, "[%s] Package reading failure", __FUNCTION__);
		return PRL_ERR_BACKUP_RESTORE_INTERNAL_PROTO_ERROR;
	}
/* TODO : process response retcode */
	if (operationIsCancelled())
		return PRL_ERR_OPERATION_WAS_CANCELED;

	PRL_RESULT nRetCode = getEntryLists(m_sBackupPath, excludeFunc);
	if (PRL_FAILED(nRetCode))
		return nRetCode;

	m_pSender = SmartPtr<CVmFileListCopySender>(new CVmFileListCopySenderServer(m_ioServer, m_hHandle));
	m_pVmCopySource = SmartPtr<CVmFileListCopySource>(
			new CVmFileListCopySource(
				m_pSender.getImpl(),
				m_sVmUuid,
				m_sBackupPath,
				m_nTotalSize,
				getLastError(),
				m_nTimeout));

	nRetCode = m_pVmCopySource->Copy(m_DirList, m_FileList);
	if (PRL_FAILED(nRetCode)) {
		WRITE_TRACE(DBG_FATAL, "Error occurred while backup with code [%#x][%s]",
			nRetCode, PRL_RESULT_TO_STRING(nRetCode));
		return nRetCode;
	}
	return PRL_ERR_SUCCESS;
}

PRL_RESULT Task_RestoreVmBackupSource::sendStartReply(const SmartPtr<CVmConfiguration>& ve_, IOSendJob::Handle& job_)
{
	qulonglong nOriginalSize = 0;
	quint32 nBundlePermissions = 0;
	PRL_RESULT code = Task_BackupHelper::getBackupParams(
		m_sVmUuid, m_sBackupUuid, m_nBackupNumber, nOriginalSize, nBundlePermissions);
	if (PRL_FAILED(code))
	{
		WRITE_TRACE(DBG_FATAL, "Unable to get backup OriginalSize %x", code);
		nOriginalSize = 0;
	}
	CDispToDispCommandPtr pReply = CDispToDispProtoSerializer::CreateVmBackupRestoreFirstReply(
			m_sVmUuid,
			m_sVmName,
			ve_->toString(),
			m_sBackupUuid,
			m_nBackupNumber,
			m_sBackupRootPath,
			nOriginalSize,
			nBundlePermissions,
			m_nInternalFlags,
			m_nRemoteVersion);
	SmartPtr<IOPackage> pPackage = DispatcherPackage::createInstance(
			pReply->GetCommandId(),
			pReply->GetCommand()->toString(),
			getRequestPackage());

	if (operationIsCancelled())
		return PRL_ERR_OPERATION_WAS_CANCELED;

	// XXX: this job is required because the target generates the ready
	// to copy files packet using the start reply package as a source.
	// that new packet is to sync transfering sides.
	job_ = m_ioServer.sendPackage(m_hHandle, pPackage);
	if (m_ioServer.waitForSend(job_, m_nTimeout) != IOSendJob::Success) {
		WRITE_TRACE(DBG_FATAL, "[%s] Package sending failure", __FUNCTION__);
		return PRL_ERR_BACKUP_RESTORE_INTERNAL_PROTO_ERROR;
	}
	return PRL_ERR_SUCCESS;
}

PRL_RESULT Task_RestoreVmBackupSource::restore(const ::Backup::Work::object_type& variant_)
{
	PRL_RESULT nRetCode = PRL_ERR_SUCCESS;
	Prl::Expected<SmartPtr<CVmConfiguration>, PRL_RESULT> e;
	IOSendJob::Handle job;

	if (operationIsCancelled()) {
		nRetCode = PRL_ERR_OPERATION_WAS_CANCELED;
		goto exit_0;
	}
	if (m_nInternalFlags & PVM_CT_PLOOP_BACKUP && m_nRemoteVersion < BACKUP_PROTO_V3)
	{
		WRITE_TRACE(DBG_FATAL, "Unsupported protocol version %d. >= %d is expected",
			m_nRemoteVersion, BACKUP_PROTO_V3);
		nRetCode = PRL_ERR_BACKUP_RESTORE_INTERNAL_PROTO_ERROR;
		goto exit_0;
	}
	if (!CFileHelper::DirectoryExists(m_sBackupPath, &m_pDispConnection->getUserSession()->getAuthHelper())) {
		nRetCode = PRL_ERR_BACKUP_BACKUP_UUID_NOT_FOUND;
		CVmEvent *pEvent = getLastError();
		pEvent->setEventCode(nRetCode);
		pEvent->addEventParameter(new CVmEventParameter(
				PVE::String, m_sBackupUuid, EVT_PARAM_MESSAGE_PARAM_0));
		WRITE_TRACE(DBG_FATAL, "Backup directory \"%s\" does not exist", QSTR2UTF8(m_sBackupPath));
		goto exit_0;
	}
	e = boost::apply_visitor(::Backup::Work::Loader(m_sBackupPath, getClient()), variant_);
	if (e.isFailed()) {
		nRetCode = PRL_ERR_BACKUP_RESTORE_INTERNAL_ERROR;
		WRITE_TRACE(DBG_FATAL, "Error occurred while VE config at \"%s\" loading with code [%#x][%s]",
			__FUNCTION__, QSTR2UTF8(m_sBackupPath), e.error(), PRL_RESULT_TO_STRING(e.error()));
		goto exit_0;
	}

	if (BACKUP_PROTO_V4 > m_nRemoteVersion)
	{
		bool bConnected = QObject::connect(m_pDispConnection.getImpl(),
			SIGNAL(onPackageReceived(IOSender::Handle, const SmartPtr<IOPackage>)),
			SLOT(handleABackupPackage(IOSender::Handle, const SmartPtr<IOPackage>)),
			Qt::DirectConnection);
		PRL_ASSERT(bConnected);

		nRetCode = sendStartReply(e.value(), job);
		if (PRL_FAILED(nRetCode))
			goto exit_1;

		nRetCode = Restore::Source::Workerv3(
			boost::bind(&Task_RestoreVmBackupSource::sendFiles, this, job),
			&m_cABackupServer)();
	}
	else
	{
		bool bConnected = QObject::connect(m_pDispConnection.getImpl(),
			SIGNAL(onPackageReceived(IOSender::Handle, const SmartPtr<IOPackage>)),
			SLOT(handleVBackupPackage(IOSender::Handle, const SmartPtr<IOPackage>)),
			Qt::DirectConnection);
		PRL_ASSERT(bConnected);

		Migrate::Vm::Target::Tunnel::IO x(*m_pDispConnection);
		bConnected = QObject::connect(&x,
			SIGNAL(onReceived(const SmartPtr<IOPackage>&)),
			SLOT(mountImage(const SmartPtr<IOPackage>&)),
			Qt::DirectConnection);
		PRL_ASSERT(bConnected);

		nRetCode = sendStartReply(e.value(), job);
		if (PRL_FAILED(nRetCode))
			goto exit_1;

		nRetCode = Backup::Tunnel::Target::backend_type::decorate
			(m_pDispConnection, *this, Restore::Source::Workerv4(
					boost::bind(&Task_RestoreVmBackupSource::sendFiles, this, boost::ref(job)),
					boost::bind(&Task_RestoreVmBackupSource::exec, _1)
				));
	}
exit_1:
	m_pDispConnection->disconnect(SIGNAL(onPackageReceived
		(IOSender::Handle, const SmartPtr<IOPackage>)), this);
exit_0:
	setLastErrorCode(nRetCode);
	return nRetCode;
}

void Task_RestoreVmBackupSource::handleABackupPackage(IOSender::Handle h, const SmartPtr<IOPackage> p)
{
	// #439777 to protect call handler for destroying object
	WaiterTillHandlerUsingObject::AutoUnlock lock( m_waiter );
	if( !lock.isLocked() )
		return;

	if (h != m_pDispConnection->GetConnectionHandle())
		return;

	if (PRL_FAILED(Task_BackupHelper::handleABackupPackage(m_pDispConnection, p, m_nBackupTimeout)))
		m_cABackupServer.kill();
}

void Task_RestoreVmBackupSource::mountImage(const SmartPtr<IOPackage>& package_)
{
	if (package_->header.type != VmBackupMountImage)
		return;

	WaiterTillHandlerUsingObject::AutoUnlock g(m_waiter);
	if (!g.isLocked())
		return;

	QString a = UTF8_2QSTR(package_->buffers[0].getImpl());
	::Backup::Storage::Image i(a);
	QSharedPointer< ::Backup::Storage::Nbd> n(new ::Backup::Storage::Nbd());
	if (BACKUP_PROTO_V5 <= m_nRemoteVersion)
		n->setExportName(QFileInfo(a).fileName());

	PRL_RESULT e = n->start(i, m_nFlags);
	IOSendJob::Handle j;
	if (PRL_FAILED(e))
		j = m_pDispConnection->sendSimpleResponse(package_, e);
	else {
		m_nbds << qMakePair(a, n);

		QByteArray url(n->getUrl().toUtf8());
		SmartPtr<IOPackage> r = IOPackage::createInstance(VmBackupRestoreImage, 1, package_);
		r->fillBuffer(0, IOPackage::RawEncoding, url.constData(), url.size()+1);
		j = m_pDispConnection->sendPackage(r);
	}

	if (m_ioServer.waitForSend(j, m_nTimeout) != IOSendJob::Success)
		WRITE_TRACE(DBG_FATAL, "[%s] Package sending failure", __FUNCTION__);
}

void Task_RestoreVmBackupSource::handleVBackupPackage(IOSender::Handle h, const SmartPtr<IOPackage> p)
{
	// #439777 to protect call handler for destroying object
	WaiterTillHandlerUsingObject::AutoUnlock lock( m_waiter );
	if( !lock.isLocked() )
		return;

	if (h != m_pDispConnection->GetConnectionHandle())
		return;

	if (p->header.type == ABackupProxyFinishCmd) {
		exit(PRL_ERR_SUCCESS);
		return;
	}

	if (p->header.type == ABackupProxyCancelCmd) {
		exit(PRL_ERR_OPERATION_WAS_CANCELED);
		return;
	}
}

void Task_RestoreVmBackupSource::clientDisconnected(IOSender::Handle h)
{
	// #439777 to protect call handler for destroying object
	if (!m_waiter.lockHandler())
		return;

	if (h == m_hHandle)
	{
		WRITE_TRACE(DBG_FATAL, "%s", __FUNCTION__);
		SmartPtr<CDspClient> nullClient;
		CancelOperationSupport::cancelOperation(nullClient, getRequestPackage());
		if (m_pVmCopySource.isValid())
			m_pVmCopySource->cancelOperation();

		m_waiter.unlockHandler();
	}
	else
	{
		m_waiter.unlockHandler();
		return;
	}
	SmartPtr<IOPackage> x = IOPackage::createInstance(ABackupProxyCancelCmd, 0);
	if (BACKUP_PROTO_V4 > m_nRemoteVersion)
		handleABackupPackage(h, x);
	else
		handleVBackupPackage(h, x);
}
