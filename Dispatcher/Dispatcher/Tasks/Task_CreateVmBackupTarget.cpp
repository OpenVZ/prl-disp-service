///////////////////////////////////////////////////////////////////////////////
///
/// @file Task_CreateVmBackupTarget.cpp
///
/// Target task for Vm backup creation
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

#include "prlcommon/Interfaces/ParallelsQt.h"
#include "prlcommon/Interfaces/ParallelsNamespace.h"
#include "prlcommon/Logging/Logging.h"

#include "CDspService.h"
#include "Libraries/DispToDispProtocols/CVmBackupProto.h"
#ifdef _LIN_
#include "CDspBackupDevice.h"
#include "vzctl/libvzctl.h"
#endif
#include "Task_CreateVmBackup.h"
#include "Task_BackupHelper_p.h"

namespace Backup
{
namespace Target
{
///////////////////////////////////////////////////////////////////////////////
// struct Image

PRL_RESULT Image::build(quint64 size_, const QString& base_)
{
	VirtualDisk::qcow2PolicyList_type p(1, VirtualDisk::Policy::Qcow2::size_type(size_));
	if (!base_.isEmpty())
		p.push_back(VirtualDisk::Policy::Qcow2::base_type(base_));
	return VirtualDisk::Qcow2::create(m_path, p);
}

void Image::remove() const
{
	QFile(m_path).remove();
}

///////////////////////////////////////////////////////////////////////////////
// struct Nbd

PRL_RESULT Nbd::start(const Image& image_)
{
	QTemporaryFile f;
	f.setFileTemplate(QString("%1/%2.sock.XXXXXX")
		.arg(QDir::tempPath())
		.arg(QFileInfo(image_.getPath()).fileName()));
	if (!f.open())
		return PRL_ERR_BACKUP_INTERNAL_ERROR;
	QString n(f.fileName());
	f.setAutoRemove(false);
	f.close();

	VirtualDisk::qcow2PolicyList_type p(1, VirtualDisk::Policy::Qcow2::unix_type(n));
	PRL_RESULT e = m_nbd.open(image_.getPath(), PRL_DISK_WRITE, p);
	if (PRL_SUCCEEDED(e)) 
		m_url = n;
	return e;
}

void Nbd::stop()
{
	if (m_url.isEmpty())
		return;
	m_nbd.close();
	QFile::remove(m_url);
}

QString Nbd::getUrl() const
{
	return QString("nbd+unix://%1").arg(m_url);
}

} // namespace Target
} // namespace Backup

/*******************************************************************************

 Vm & Ct Backup creation task for server

********************************************************************************/
Task_CreateVmBackupTarget::Task_CreateVmBackupTarget(
		SmartPtr<CDspDispConnection> &pDispConnection,
		CDispToDispCommandPtr pCmd,
		const SmartPtr<IOPackage> &p,
		::Backup::Activity::Service& service_)
:Task_BackupHelper(pDispConnection->getUserSession(), p),
m_pDispConnection(pDispConnection),
m_bBackupLocked(false)
{
	CVmBackupCreateCommand *pStartCommand;

	pStartCommand = CDispToDispProtoSerializer::CastToDispToDispCommand<CVmBackupCreateCommand>(pCmd);
	m_sVmUuid = pStartCommand->GetVmUuid();
	m_sVmName = pStartCommand->GetVmName();
	m_sSourceHost = pStartCommand->GetHost();
	m_sServerUuid = pStartCommand->GetServerUuid();
	m_sDescription = pStartCommand->GetDescription();
	m_nFlags = pStartCommand->GetFlags();
	m_nOriginalSize = pStartCommand->GetOriginalSize();
	setInternalFlags(pStartCommand->GetInternalFlags());
	m_hConnHandle = pDispConnection->GetConnectionHandle();
	m_nFreeDiskSpace = ~0;
	m_bABackupFirstPacket = true;
	m_bStorageRegistered = false;
	m_nRemoteVersion = pStartCommand->GetVersion();
	m_service = &service_;

	m_nBundlePermissions = 0;
	if (m_nRemoteVersion >= BACKUP_PROTO_V2)
		m_nBundlePermissions = pStartCommand->GetBundlePermissions();
	if (m_nRemoteVersion >= BACKUP_PROTO_V3)
		m_pVmConfig->fromString(pStartCommand->GetVmConfig());
	if (m_nRemoteVersion >= BACKUP_PROTO_V4)
		m_bitmaps = pStartCommand->GetBitmaps();

	bool bConnected = QObject::connect(
		&CDspService::instance()->getIOServer(),
		SIGNAL(onClientDisconnected(IOSender::Handle)),
		SLOT(clientDisconnected(IOSender::Handle)),
		Qt::DirectConnection);
	PRL_ASSERT(bConnected);
}

Task_CreateVmBackupTarget::~Task_CreateVmBackupTarget()
{
	// #439777 protect handler from destroying object
	m_waiter.waitUnlockAndFinalize();
}

PRL_RESULT Task_CreateVmBackupTarget::validateBackupDir(const QString &sPath)
{
	PRL_RESULT nRetCode = PRL_ERR_SUCCESS;
	QDir backupsdir(sPath);

	/* if directory does not exist - to create */
	if (!backupsdir.exists()) {
		if (!backupsdir.mkdir(sPath)) {
			nRetCode = PRL_ERR_BACKUP_CANNOT_CREATE_DIRECTORY;
			CVmEvent *pEvent = getLastError();
			pEvent->setEventCode(nRetCode);
			pEvent->addEventParameter(new CVmEventParameter(
					PVE::String, sPath, EVT_PARAM_MESSAGE_PARAM_0));
			WRITE_TRACE(DBG_FATAL, "Cannot create \"%s\" directory", QSTR2UTF8(sPath));
			return nRetCode;
		}
	}

	/* and fix permissions */
	QFile fbackupsdir(QString("%1/.").arg(sPath));
	QFile::Permissions nPermissions =
			QFile::ReadUser | QFile::WriteUser | QFile::ExeUser |
			QFile::ReadGroup | QFile::WriteGroup | QFile::ExeGroup |
			QFile::ReadOther | QFile::WriteOther | QFile::ExeOther;

	if (fbackupsdir.permissions() != nPermissions) {
		if (!fbackupsdir.setPermissions(nPermissions)) {
			nRetCode = PRL_ERR_BACKUP_CANNOT_SET_PERMISSIONS;
			CVmEvent *pEvent = getLastError();
			pEvent->setEventCode(nRetCode);
			pEvent->addEventParameter(new CVmEventParameter(
				PVE::String, sPath, EVT_PARAM_MESSAGE_PARAM_0));
			WRITE_TRACE(DBG_FATAL, "Cannot set permissions for directory \"%s\"", QSTR2UTF8(sPath));
			return nRetCode;
		}
	}
#ifndef _WIN_
	/* set sticky bit for directory with perms 777.
	   from stat man page:
	   The `sticky' bit (S_ISVTX) on a directory means that a file in that directory can be renamed
	   or deleted only by the owner of the file, by the owner of the directory, and by a privileged process.
	*/
	struct stat st;
	if (stat(QSTR2UTF8(sPath), &st) || chmod(QSTR2UTF8(sPath), st.st_mode | S_ISVTX))
		WRITE_TRACE(DBG_FATAL, "Cannot set 'sticky' bit for directory \"%s\"", QSTR2UTF8(sPath));
#endif
	return nRetCode;
}

PRL_RESULT Task_CreateVmBackupTarget::guessBackupType()
{
	bool f = 0 != (m_nFlags & PBT_FULL);
	do {
		if (f) {
			break;
		}
		if ((f = !m_lastBase.isValid())) {
			WRITE_TRACE(DBG_FATAL,
				"This Vm has no full backup, will create the "
				"first full backup instead of incremental");
			break;
		}
		m_sBackupUuid = m_lastBase->getUuid();
		m_nBackupNumber = getNextPartialBackup(m_sVmUuid, m_sBackupUuid);
		if ((f = (m_lastBase->getLastNumber() &&
			m_nBackupNumber != m_lastBase->getLastNumber() + 1))) {
			WRITE_TRACE(DBG_FATAL, "The last incremental backup of this Vm "
				"is not found, will create a full backup instead of incremental");
			break;
		}
		setBackupRoot(QString("%1/%2/%3").arg(getBackupDirectory())
				.arg(m_sVmUuid).arg(m_sBackupUuid));
		if (0 != (getInternalFlags() & PVM_CT_VZFS_BACKUP)) {
			break;
		}
		PRL_RESULT e = wasHddListChanged(&f);
		if (PRL_FAILED(e))
			return e;
		if (f) {
			WRITE_TRACE(DBG_FATAL, "HDD list was changed since last base backup,"
						" will to create full backup instead of incremental");
		}
		if (BACKUP_PROTO_V4 <= m_nRemoteVersion)
			f = !m_bitmaps.contains(m_sBackupUuid);
	} while(false);
	if (f) {
		m_nFlags &= ~PBT_INCREMENTAL;
		m_nFlags |= PBT_FULL;
		/* create new base backup */
		m_sBackupUuid = Uuid::createUuid().toString();
		m_nBackupNumber = 0;
		setBackupRoot(QString("%1/%2/%3").arg(getBackupDirectory()).arg(m_sVmUuid).arg(m_sBackupUuid));
		m_sTargetPath = QString("%1/" PRL_BASE_BACKUP_DIRECTORY).arg(getBackupRoot());
	} else
		m_sTargetPath = QString("%1/%2").arg(getBackupRoot()).arg(m_nBackupNumber);
	return PRL_ERR_SUCCESS;
}

PRL_RESULT Task_CreateVmBackupTarget::prepareImages()
{
	if (m_nRemoteVersion < BACKUP_PROTO_V4)
		return PRL_ERR_SUCCESS;

	::Backup::Product::Model p(::Backup::Object::Model(m_pVmConfig), m_sVmHomePath);
	p.setStore(getBackupRoot());
	unsigned n = getBackupNumber();
	p.setSuffix(::Backup::Suffix(n, getFlags())());
	::Backup::Work::object_type o(::Backup::Work::Vm(*this));
	if (getInternalFlags() & PVM_CT_PLOOP_BACKUP)
		o = ::Backup::Work::Ct(*this);
	::Backup::Product::componentList_type x, l = p.getVmTibs();

	// we need to set previous backup archive as qcow backing store for
	// successful restore.
	if (n >= PRL_PARTIAL_BACKUP_START_NUMBER) {
		::Backup::Product::Model y(p);
		y.setSuffix(::Backup::Suffix(n - 1, getFlags())());
		x = y.getVmTibs();
		PRL_ASSERT(x.size() == l.size());
	}

	for (int i = 0; i < l.size(); ++i) {
		::Backup::Target::Image a(l.at(i).second.absoluteFilePath());
		QString base((x.size() ? x.at(i).second.absoluteFilePath() : ""));
		PRL_RESULT e = a.build(l.at(i).first.getDevice().getSize() << 20, base);
		if (PRL_FAILED(e))
			return e;
		m_lstTibFileList << l.at(i).second.fileName();

		if (!(getInternalFlags() & PVM_CT_PLOOP_BACKUP))
			continue;

		QSharedPointer< ::Backup::Target::Nbd> n(new ::Backup::Target::Nbd());
		m_createdTibs << qMakePair(a, n);
		if (PRL_FAILED((e = n->start(a))))
			return e;
	}
	return PRL_ERR_SUCCESS;
}

PRL_RESULT Task_CreateVmBackupTarget::prepareTask()
{
	m_lastBase = SmartPtr<BackupItem>(getLastBaseBackup(m_sVmUuid,
						&getClient()->getAuthHelper(),
						PRL_BACKUP_CHECK_MODE_WRITE));
	PRL_RESULT nRetCode;
	if (PRL_FAILED(nRetCode = guessBackupType()))
		goto exit;

	m_pSender = SmartPtr<CVmFileListCopySender>(new CVmFileListCopySenderServer(
		CDspService::instance()->getIOServer(),
		m_pDispConnection->GetConnectionHandle()));
	m_pVmCopyTarget = SmartPtr<CVmFileListCopyTarget>(
		new CVmFileListCopyTarget(m_pSender.getImpl(), m_sVmUuid, m_sTargetPath, getLastError(), m_nTimeout));

	if (CFileHelper::DirectoryExists(m_sTargetPath, &getClient()->getAuthHelper())) {
		nRetCode = PRL_ERR_BACKUP_DIRECTORY_ALREADY_EXIST;
		CVmEvent *pEvent = getLastError();
		pEvent->setEventCode(nRetCode);
		pEvent->addEventParameter(new CVmEventParameter(PVE::String, m_sTargetPath, EVT_PARAM_MESSAGE_PARAM_0));
		WRITE_TRACE(DBG_FATAL, "Target directory \"%s\" already exist", QSTR2UTF8(m_sTargetPath));
		goto exit;
	}

	//Check whether backup directory exist and create it otherwise
	{
		// Helper for root users.
		CAuthHelper rootAuth;

		if ( ! CDspService::instance()->checkExistAndCreateDirectory( getBackupDirectory(), rootAuth, CDspService::permBackupDir ) )
		{
			WRITE_TRACE( DBG_FATAL, "Can't create parallels backup directory '%s'", QSTR2UTF8( getBackupDirectory() ) );
			nRetCode = PRL_ERR_BACKUP_CANNOT_CREATE_DIRECTORY;
			CVmEvent *pEvent = getLastError();
			pEvent->setEventCode(nRetCode);
			pEvent->addEventParameter(new CVmEventParameter(PVE::String, getBackupDirectory(), EVT_PARAM_MESSAGE_PARAM_0));

			goto exit;
		}
	}

	CFileHelper::GetDiskAvailableSpace(getBackupDirectory(), &m_nFreeDiskSpace);

	if (m_nFlags & PBT_FULL) {
		/* Create /var/parallels/backups/VM_UUID */
		nRetCode = validateBackupDir(QString("%1/%2").arg(getBackupDirectory()).arg(m_sVmUuid));
		if (nRetCode != PRL_ERR_SUCCESS)
			goto exit;

		/* Create /var/parallels/backups/VM_UUID/BACKUP_UUID with owner = client */
		if (!CFileHelper::CreateDirectoryPath(getBackupRoot(), &getClient()->getAuthHelper())) {
			nRetCode = PRL_ERR_BACKUP_CANNOT_CREATE_DIRECTORY;
			CVmEvent *pEvent = getLastError();
			pEvent->setEventCode(nRetCode);
			pEvent->addEventParameter(new CVmEventParameter(
					PVE::String, getBackupRoot(), EVT_PARAM_MESSAGE_PARAM_0));
			WRITE_TRACE(DBG_FATAL, "Cannot create \"%s\" directory", QSTR2UTF8(getBackupRoot()));
			goto exit;
		}
		/* and set next perms : all operations - for owner only, list and restore - for group */
		QFile fdir(QString("%1/.").arg(getBackupRoot()));
		if (!fdir.setPermissions(QFile::ReadUser|QFile::WriteUser|QFile::ExeUser|QFile::ReadGroup))
		{
			nRetCode = PRL_ERR_BACKUP_CANNOT_SET_PERMISSIONS;
			CVmEvent *pEvent = getLastError();
			pEvent->setEventCode(nRetCode);
			pEvent->addEventParameter(new CVmEventParameter(
					PVE::String, getBackupRoot(), EVT_PARAM_MESSAGE_PARAM_0));
			WRITE_TRACE(DBG_FATAL, "Cannot set permissions for directory \"%s\"", QSTR2UTF8(getBackupRoot()));
			goto exit;
		}
	} else {
		/* to check access before */
		if (!CFileHelper::FileCanWrite(getBackupRoot(), &getClient()->getAuthHelper())) {
			nRetCode = PRL_ERR_BACKUP_BACKUP_UUID_NOT_FOUND;
			CVmEvent *pEvent = getLastError();
			pEvent->setEventCode(nRetCode);
			pEvent->addEventParameter(new CVmEventParameter(
					PVE::String, m_sBackupUuid, EVT_PARAM_MESSAGE_PARAM_0));
			WRITE_TRACE(DBG_FATAL, "User %s have not permissions for create incremental backup for %s",
				QSTR2UTF8(getClient()->getAuthHelper().getUserName()), QSTR2UTF8(m_sBackupUuid));
			goto exit;
		}
	}
	if (!CFileHelper::WriteDirectory(m_sTargetPath, &getClient()->getAuthHelper())) {
		nRetCode = PRL_ERR_BACKUP_CANNOT_CREATE_DIRECTORY;
		CVmEvent *pEvent = getLastError();
		pEvent->setEventCode(nRetCode);
		pEvent->addEventParameter(new CVmEventParameter(PVE::String, m_sTargetPath, EVT_PARAM_MESSAGE_PARAM_0));
		WRITE_TRACE(DBG_FATAL, "Cannot create \"%s\" directory", QSTR2UTF8(m_sTargetPath));
		goto exit;
	}

	if (PRL_FAILED(nRetCode = prepareImages()))
		goto exit;

	/*
	   To lock _full_ backup, but exclusive for full backup and shared for incremental:
	   in last case we must ban base backup removing, but allow list of base backup and
	   restore from base and other incremental backups. To avoid list of backup on creation
	   state, will skip backup without .metadata file (backup create this file on last step)
	   https://jira.sw.ru/browse/PSBM-8198
	 */
	if (m_nFlags & PBT_FULL)
		nRetCode = lockExclusive(m_sBackupUuid);
	else
		nRetCode = lockShared(m_sBackupUuid);
	if (PRL_FAILED(nRetCode))
		goto exit;
	m_bBackupLocked = true;

exit:
	setLastErrorCode(nRetCode);
	return nRetCode;
}

PRL_RESULT Task_CreateVmBackupTarget::loadTibFiles()
{
	QFile file(m_sABackupOutFile);
	/* try to load tib files list */
	if (file.open(QIODevice::ReadOnly)) {
		QByteArray array;
		while (1) {
			array = file.readLine();
			if (array.isEmpty())
				break;
			array.replace('\n', '\0');
			m_lstTibFileList.append(QString(array.data()));
		}
		file.close();
	}
	return PRL_ERR_SUCCESS;
}

PRL_RESULT Task_CreateVmBackupTarget::run_body()
{
	PRL_RESULT nRetCode = PRL_ERR_SUCCESS;
	CDispToDispCommandPtr pReply;
	SmartPtr<IOPackage> pPackage;
	IOSendJob::Handle hJob;
	bool bConnected;
	QTemporaryFile tmpFile;
	QStringList args;
	int i = 1;

	/* to lock mutex to avoid ABackup packages processing before backup server start */
	QMutexLocker locker(&m_cABackupMutex);

	if (operationIsCancelled()) {
		nRetCode = PRL_ERR_OPERATION_WAS_CANCELED;
		goto exit;
	}

	pReply = CDispToDispProtoSerializer::CreateVmBackupCreateFirstReply(
				m_sBackupUuid, m_nBackupNumber, getBackupRoot(), m_nFreeDiskSpace, m_nFlags);
	pPackage = DispatcherPackage::createInstance(
			pReply->GetCommandId(),
			pReply->GetCommand()->toString(),
			getRequestPackage(), false, m_createdTibs.size() * 2 + 1);

	foreach(const archive_type& a, m_createdTibs) {
		QString path(a.first.getPath());
		QString url(a.second->getUrl());
		pPackage->fillBuffer(i++, IOPackage::RawEncoding, QSTR2UTF8(path), path.size()+1);
		pPackage->fillBuffer(i++, IOPackage::RawEncoding, QSTR2UTF8(url), url.size()+1);
	}

	/* set signal handler before reply - to avoid race (#467221) */
	bConnected = QObject::connect(m_pDispConnection.getImpl(),
		SIGNAL(onPackageReceived(IOSender::Handle, const SmartPtr<IOPackage>)),
		SLOT(handlePackage(IOSender::Handle, const SmartPtr<IOPackage>)),
		Qt::DirectConnection);

	PRL_ASSERT(bConnected);
	// send reply to client
	hJob = m_pDispConnection->sendPackage(pPackage);
	if (CDspService::instance()->getIOServer().waitForSend(hJob, m_nTimeout) != IOSendJob::Success) {
		WRITE_TRACE(DBG_FATAL, "Package sending failure");
		nRetCode = PRL_ERR_BACKUP_INTERNAL_PROTO_ERROR;
		goto exit;
	}

	if (operationIsCancelled()) {
		nRetCode = PRL_ERR_OPERATION_WAS_CANCELED;
		goto exit;
	}

	/* part one : plain copy of config files */
	nRetCode = exec();
	if (PRL_FAILED(nRetCode)) {
		WRITE_TRACE(DBG_FATAL, "Backup Plain copy stage failed: %s",
				PRL_RESULT_TO_STRING(nRetCode));
		goto exit;
	}

	/* create temporary file for abackup server */
	tmpFile.setFileTemplate(QString("%1/ABackupOutFile.XXXXXX").arg(m_sTargetPath));
	/* open() & closed() to get real file name */
	if (!tmpFile.open()) {
		WRITE_TRACE(DBG_FATAL, "QTemporaryFile::open() error: %s",
				QSTR2UTF8(tmpFile.errorString()));
		nRetCode = PRL_ERR_BACKUP_INTERNAL_ERROR;
		goto exit;
	}
	if (BACKUP_PROTO_V4 <= m_nRemoteVersion) {
		nRetCode = exec();
	} else {
		/* for QTemporaryFile file name exist after open() and before close() only */
		m_sABackupOutFile = tmpFile.fileName();
		tmpFile.close();

		/* for QTemporaryFile file name exist after open() and before close() only */
		args.append(m_sABackupOutFile);
		args.prepend(QString(PRL_ABACKUP_SERVER));

		/* Target side - preserve old arguments processing */
		if (PRL_FAILED(nRetCode = m_cABackupServer.start(args, BACKUP_PROTO_V3)))
			goto exit;
		locker.unlock();

		nRetCode = m_cABackupServer.waitForFinished();
		loadTibFiles();
	}

	QObject::disconnect(m_pDispConnection.getImpl(),
		SIGNAL(onPackageReceived(IOSender::Handle, const SmartPtr<IOPackage>)),
		this,
		SLOT(handlePackage(IOSender::Handle, const SmartPtr<IOPackage>)));
	if (PRL_FAILED(nRetCode))
		goto exit;

	if (PRL_FAILED(nRetCode = saveMetadata()))
		goto exit;

exit:
	setLastErrorCode(nRetCode);
	return nRetCode;
}

void Task_CreateVmBackupTarget::finalizeTask()
{
	QObject::disconnect(
		&CDspService::instance()->getIOServer(),
		SIGNAL(onClientDisconnected(IOSender::Handle)),
		this,
		SLOT(clientDisconnected(IOSender::Handle)));

	IOSendJob::Handle hJob;

	m_cABackupServer.kill();

	if (m_bBackupLocked) {
		if (m_nFlags & PBT_FULL)
			unlockExclusive(m_sBackupUuid);
		else
			unlockShared(m_sBackupUuid);
	}
	m_bBackupLocked = false;

	foreach(const archive_type& f, m_createdTibs)
		f.second->stop();

	if (PRL_SUCCEEDED(getLastErrorCode())) {
		hJob = m_pDispConnection->sendSimpleResponse(getRequestPackage(), PRL_ERR_SUCCESS);
	} else {
		/* remove all tib files of this backup */
		for (int i = 0; i < m_lstTibFileList.size(); ++i)
			QFile::remove(QString("%1/%2").arg(getBackupRoot()).arg(m_lstTibFileList.at(i)));

		/* remove current backup directory */
		if (m_sTargetPath.size()) {
			QString path;
			CFileHelper::ClearAndDeleteDir(m_sTargetPath);

			/* remove BackupUuid directory if it is empty */
			if (isBackupDirEmpty(m_sVmUuid, m_sBackupUuid)) {
				CFileHelper::ClearAndDeleteDir(getBackupRoot());
			}
			/* and remove VmUuid directory if it is empty - check it as root */
			QStringList lstBackupUuid;
			getBaseBackupList(m_sVmUuid, lstBackupUuid);
			if (lstBackupUuid.isEmpty()) {
				path = QString("%1/%2").arg(getBackupDirectory()).arg(m_sVmUuid);
				CFileHelper::ClearAndDeleteDir(path);
			}
		}
		hJob = m_pDispConnection->sendResponseError(getLastError(), getRequestPackage());
	}
	CDspService::instance()->getIOServer().waitForSend(hJob, m_nTimeout);

	m_lastBase.reset();
}

void Task_CreateVmBackupTarget::handlePackage(IOSender::Handle h, const SmartPtr<IOPackage> p)
{
	PRL_RESULT nRetCode;
	bool bExit;

	// #439777 to protect call handler for destroying object
	WaiterTillHandlerUsingObject::AutoUnlock lock( m_waiter );
	if( !lock.isLocked() )
		return;

	if (h != m_pDispConnection->GetConnectionHandle())
		return;

	if (IS_FILE_COPY_PACKAGE(p->header.type)) {
		PRL_ASSERT(m_pVmCopyTarget);
		nRetCode = m_pVmCopyTarget->handlePackage(p, &bExit);
		if (bExit)
			exit(nRetCode);
	} else if (BACKUP_PROTO_V4 <= m_nRemoteVersion) {
		if (p->header.type == ABackupProxyCancelCmd)
			exit(PRL_ERR_OPERATION_WAS_CANCELED);
		if (p->header.type == ABackupProxyFinishCmd)
			exit(PRL_ERR_SUCCESS);
	} else if (IS_ABACKUP_PROXY_PACKAGE(p->header.type)) {
		/* Do not process _first_ incoming ABackup packages before backup server start
		   Wait m_nTimeout only because client will disconnect after this timeout */
		if (m_bABackupFirstPacket) {
			if (!m_cABackupMutex.tryLock(m_nTimeout)) {
				WRITE_TRACE(DBG_FATAL, "QMutex::tryLock(%d) error, package with type %d ignored",
						m_nTimeout, p->header.type);
				return;
			}
			m_bABackupFirstPacket = false;
			m_cABackupMutex.unlock();
			if (PRL_FAILED(getLastErrorCode()))
				/* but we got error before backup server start */
				return;
		}

		if (PRL_FAILED(Task_BackupHelper::handleABackupPackage(m_pDispConnection, p, m_nBackupTimeout)))
			m_cABackupServer.kill();
	}
}

quint64 Task_CreateVmBackupTarget::getBackupSize()
{
	quint64 nSize = 0;

	if (m_nFlags & PBT_FULL)
	{
		// Total directory size for full backup.
		CFileHelper::GetDirSize(getBackupRoot(), &nSize);
	}
	else
	{
		// Config size + tlb's for incremental
		CFileHelper::GetDirSize(m_sTargetPath, &nSize);
		for (int i = 0; i < m_lstTibFileList.size(); ++i)
		{
			QFileInfo fi(QString("%1/%2").arg(getBackupRoot()).arg(m_lstTibFileList.at(i)));
			nSize += fi.size();
		}
	}
	return nSize;
}

PRL_RESULT Task_CreateVmBackupTarget::saveMetadata()
{
	PRL_RESULT nRetCode = PRL_ERR_SUCCESS;
	VmItem iVm;

	/* rewrote metadata in Vm directory */
	if (getInternalFlags() & PVM_CT_VZFS_BACKUP)
		iVm.setVmType(PVBT_CT_VZFS);
	else if (getInternalFlags() & PVM_CT_PLOOP_BACKUP)
		iVm.setVmType(PVBT_CT_PLOOP);
	else if (getInternalFlags() & PVM_CT_VZWIN_BACKUP)
		iVm.setVmType(PVBT_CT_VZWIN);
	else
		iVm.setVmType(PVBT_VM);

	iVm.setUuid(m_sVmUuid);
	iVm.setName(m_sVmName);
	iVm.setVersion(m_nRemoteVersion);
	nRetCode = iVm.saveToFile(QString("%1/%2/" PRL_BACKUP_METADATA).arg(getBackupDirectory()).arg(m_sVmUuid));

	/* create metadata in backup directory */
	if (m_nFlags & PBT_FULL) {
		BackupItem cBackup;
		cBackup.setUuid(m_sBackupUuid);
		cBackup.setId(m_sBackupUuid);
		cBackup.setHost(m_sSourceHost);
		cBackup.setServerUuid(m_sServerUuid);
		QDateTime x = QDateTime::currentDateTime();
		if (m_lastBase.isValid() && x.secsTo(m_lastBase->getDateTime()) >= 0) {
			x = m_lastBase->getDateTime().addSecs(1);
		}
		cBackup.setDateTime(x);
		cBackup.setCreator(getClient()->getAuthHelper().getUserName());
		cBackup.setSize(getBackupSize());
		cBackup.setType(PRL_BACKUP_FULL_TYPE);
		cBackup.setDescription(m_sDescription);
		cBackup.setTibFileList(m_lstTibFileList);
		cBackup.setOriginalSize(m_nOriginalSize);
		cBackup.setBundlePermissions(m_nBundlePermissions);
		nRetCode = cBackup.saveToFile(QString("%1/" PRL_BACKUP_METADATA).arg(m_sTargetPath));
	} else {
		PartialBackupItem cBackup;
		cBackup.setNumber(m_nBackupNumber);
		cBackup.setId(QString("%1.%2").arg(m_sBackupUuid).arg(m_nBackupNumber));
		cBackup.setHost(m_sSourceHost);
		cBackup.setServerUuid(m_sServerUuid);
		cBackup.setDateTime(QDateTime::currentDateTime());
		cBackup.setCreator(getClient()->getAuthHelper().getUserName());
		cBackup.setSize(getBackupSize());
		if (m_nFlags & PBT_INCREMENTAL)
			cBackup.setType(PRL_BACKUP_INCREMENTAL_TYPE);
		else
			cBackup.setType(PRL_BACKUP_DIFFERENTIAL_TYPE);
		cBackup.setDescription(m_sDescription);
		cBackup.setTibFileList(m_lstTibFileList);
		cBackup.setOriginalSize(m_nOriginalSize);
		cBackup.setBundlePermissions(m_nBundlePermissions);
		nRetCode = cBackup.saveToFile(QString("%1/" PRL_BACKUP_METADATA).arg(m_sTargetPath));
		if (PRL_SUCCEEDED(nRetCode))
			nRetCode = updateLastPartialNumber(m_sVmUuid, m_sBackupUuid, m_nBackupNumber);
	}
	return nRetCode;
}

void Task_CreateVmBackupTarget::clientDisconnected(IOSender::Handle h)
{
	// #439777 to protect call handler for destroying object
	WaiterTillHandlerUsingObject::AutoUnlock lock( m_waiter );
	if( !lock.isLocked() )
		return;

	if (h != m_hConnHandle)
		return;

	WRITE_TRACE(DBG_FATAL, "%s : backup client was disconnected", __FUNCTION__);
	SmartPtr<CDspClient> nullClient;
	CancelOperationSupport::cancelOperation(nullClient, getRequestPackage());

	if (m_pVmCopyTarget.isValid())
		m_pVmCopyTarget->cancelOperation();

	m_cABackupServer.kill();
	// quit event loop
	QThread::exit(PRL_ERR_OPERATION_WAS_CANCELED);
}

// compare hdd lists from current VM config and from VM config of last base backup
PRL_RESULT Task_CreateVmBackupTarget::wasHddListChanged(bool *pbWasChanged)
{
	std::map<QString, unsigned long> ls0, ls1;
	QFileInfo fi;

	// load hdd list from current config
	foreach(CVmHardDisk *pHdd, ::Backup::Object::Model(m_pVmConfig).getImages()) {
		QString d = pHdd->getSystemName();
		if (QFileInfo(d).isRelative()) {
			d = QFileInfo(QDir(m_pVmConfig->getVmIdentification()->getHomePath()),
				d).absoluteFilePath();
		}
		ls0[d] = pHdd->getSize();
	}

	// load hdd list from last full backup config
	QString x;
	CVmConfiguration VmConfig;
	SmartPtr<CVmConfiguration> p;
	QFileInfo b(getBackupRoot(), PRL_BASE_BACKUP_DIRECTORY);
	if (0 == (getInternalFlags() & PVM_CT_PLOOP_BACKUP))
	{
		if (0 == (getInternalFlags() & PVM_CT_VZWIN_BACKUP))
			x = QDir(b.absoluteFilePath()).absoluteFilePath(VMDIR_DEFAULT_VM_CONFIG_FILE);
		else
			x = QDir(b.absoluteFilePath()).absoluteFilePath(VZ_CT_XML_CONFIG_FILE);

		QFile file(x);
		// load config from base backup with relative path
		if (PRL_SUCCEEDED(VmConfig.loadFromFile(&file, false)))
			p = SmartPtr<CVmConfiguration>(&VmConfig, SmartPtrPolicy::DoNotReleasePointee);
	}
	else
	{
		int y = 0;
		x = QDir(b.absoluteFilePath()).absoluteFilePath(VZ_CT_CONFIG_FILE);
		p = CVzHelper::get_env_config_from_file(x, y, VZCTL_LAYOUT_5, true);
	}
	if (!p.isValid())
	{
		WRITE_TRACE(DBG_FATAL, "Can not load config file %s", QSTR2UTF8(x));
		return PRL_ERR_BACKUP_INTERNAL_ERROR;
	}
	foreach(CVmHardDisk *pHdd, ::Backup::Object::Model(p).getImages()) {
		QString d = pHdd->getSystemName();
		if (QFileInfo(d).isRelative()) {
			d = QFileInfo(QDir(m_pVmConfig->getVmIdentification()->getHomePath()),
				d).absoluteFilePath();
		}
		ls1[d] = pHdd->getSize();
	}
	*pbWasChanged = !(ls1.size() == ls0.size() && std::equal(ls1.begin(), ls1.end(), ls0.begin()));
	return PRL_ERR_SUCCESS;
}

