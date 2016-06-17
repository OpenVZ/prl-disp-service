///////////////////////////////////////////////////////////////////////////////
///
/// @file Task_RemoveVmBackup.cpp
///
/// Source and target tasks for Vm backup restoring
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

//#define LOGGING_ON
//#define FORCE_LOGGING_LEVEL DBG_DEBUG

#include "Interfaces/Debug.h"
#include "prlcommon/Interfaces/ParallelsQt.h"
#include "prlcommon/Interfaces/ParallelsNamespace.h"

#include "prlcommon/Logging/Logging.h"
#include "prlcommon/HostUtils/HostUtils.h"
#include "Libraries/StatesStore/SavedStateTree.h"

#include "Task_RemoveVmBackup.h"
#include "Tasks/Task_RegisterVm.h"
#include "Libraries/DispToDispProtocols/CVmBackupProto.h"
#include "CDspService.h"
#include "prlcommon/Std/PrlAssert.h"
#include "Libraries/PrlCommonUtils/CFileHelper.h"
#include "prlxmlmodel/BackupTree/BackupTree.h"
#include "Tasks/Task_BackupHelper_p.h"

/*******************************************************************************

 Backup removing task for client

********************************************************************************/
Task_RemoveVmBackupSource::Task_RemoveVmBackupSource(
		SmartPtr<CDspClient> &client,
		CProtoCommandPtr cmd,
		const SmartPtr<IOPackage> &p)
:Task_BackupHelper(client, p)
{
	CProtoRemoveVmBackupCommand *pCmd = CProtoSerializer::CastToProtoCommand<CProtoRemoveVmBackupCommand>(cmd);
	m_sVmUuid = pCmd->GetVmUuid();
	m_sBackupId = pCmd->GetBackupUuid();
	m_sServerHostname = pCmd->GetServerHostname();
	m_nServerPort = pCmd->GetServerPort();
	m_sServerSessionUuid = pCmd->GetServerSessionUuid();
	m_nFlags = pCmd->GetFlags() | PBT_VM | PBT_CT; // by default for Vm and Ct
}

PRL_RESULT Task_RemoveVmBackupSource::run_body()
{
	PRL_RESULT nRetCode = PRL_ERR_SUCCESS;
	CDispToDispCommandPtr pCommand;
	SmartPtr<IOPackage> pPackage;
	SmartPtr<IOPackage> pReply;
	CDispToDispCommandPtr pResponce;
	CDispToDispResponseCommand *pResponseCmd;

	if (PRL_FAILED(nRetCode = connect()))
		goto exit;

	if (operationIsCancelled())
	{
		nRetCode = PRL_ERR_OPERATION_WAS_CANCELED;
		goto exit;
	}

	if (m_sVmUuid.isEmpty())
	{
		if (m_sBackupId.isEmpty()) {
			nRetCode = PRL_ERR_INVALID_ARG;
			WRITE_TRACE(DBG_FATAL, "[%s] Invalid parameters : empty backup Uuid and Vm Uuid", __FUNCTION__);
			goto exit;
		}

		QString sBackupTree;
		if (PRL_FAILED(nRetCode = GetBackupTreeRequest(m_sVmUuid, sBackupTree)))
		{
			nRetCode = PRL_ERR_BACKUP_INTERNAL_ERROR;
			WRITE_TRACE( DBG_FATAL, "Failed to get backup tree");
			goto exit;
		}

		QString sBackupUuid;
		unsigned nBackupNumber;

		if (PRL_FAILED(parseBackupId(m_sBackupId, sBackupUuid, nBackupNumber))) {
				nRetCode = PRL_ERR_BACKUP_BACKUP_UUID_NOT_FOUND;
				CVmEvent *pEvent = getLastError();
				pEvent->setEventCode(nRetCode);
				pEvent->addEventParameter(new CVmEventParameter(
						PVE::String, m_sBackupId, EVT_PARAM_MESSAGE_PARAM_0));
				WRITE_TRACE(DBG_FATAL, "Failed to parse backupId \"%s\".",
					    QSTR2UTF8(m_sBackupId));
				goto exit;
		}

		BackupTree bTree;

		bTree.fromString(sBackupTree);
		foreach(VmItem *vm , bTree.m_lstVmItem)
		{
			foreach( BackupItem *backup, vm->m_lstBackupItem )
			{
				if ( backup->getId() != sBackupUuid )
					continue;

				m_sVmUuid = vm->getUuid();
				break;
			}

			if (!m_sVmUuid.isEmpty())
				break;
		}

		if (m_sVmUuid.isEmpty())
		{
			nRetCode = PRL_ERR_BACKUP_BACKUP_UUID_NOT_FOUND;
			CVmEvent *pEvent = getLastError();
			pEvent->setEventCode(nRetCode);
			pEvent->addEventParameter(new CVmEventParameter(
					PVE::String, m_sBackupId, EVT_PARAM_MESSAGE_PARAM_0));
			WRITE_TRACE(DBG_FATAL, "Failed to find vm for backupId \"%s\".",
				QSTR2UTF8(sBackupUuid));
			goto exit;
		}
	}

	pCommand = CDispToDispProtoSerializer::CreateVmBackupRemoveCommand(m_sVmUuid, m_sBackupId, m_nFlags);
	pPackage = DispatcherPackage::createInstance(pCommand->GetCommandId(), pCommand->GetCommand()->toString());

	if (PRL_FAILED(nRetCode = SendReqAndWaitReply(pPackage, pReply)))
		goto exit;

	if (pReply->header.type != DispToDispResponseCmd) {
		WRITE_TRACE(DBG_FATAL, "Invalid package header:%x, expected header:%x",
			pReply->header.type, DispToDispResponseCmd);
		nRetCode = PRL_ERR_BACKUP_INTERNAL_PROTO_ERROR;
		goto exit;
	}

	pResponce = CDispToDispProtoSerializer::ParseCommand(
		DispToDispResponseCmd, UTF8_2QSTR(pReply->buffers[0].getImpl()));
	pResponseCmd = CDispToDispProtoSerializer::CastToDispToDispCommand<CDispToDispResponseCommand>(pResponce);
	if (PRL_FAILED(nRetCode = pResponseCmd->GetRetCode()))
		getLastError()->fromString(pResponseCmd->GetErrorInfo()->toString());

exit:
	setLastErrorCode(nRetCode);
	return nRetCode;
}

/* Finalize task */
void Task_RemoveVmBackupSource::finalizeTask()
{
	if (isConnected()) {
		CDispToDispCommandPtr pCmd =
			CDispToDispProtoSerializer::CreateDispToDispCommandWithoutParams(DispToDispLogoffCmd);
		SmartPtr<IOPackage> pPackage =
			DispatcherPackage::createInstance(pCmd->GetCommandId(), pCmd->GetCommand()->toString());
		SendPkg(pPackage);
	}

	if (PRL_SUCCEEDED(getLastErrorCode()))
	{
		QString sVmDirUuid = getClient()->getVmDirectoryUuid();
		bool isVmExists;

		SmartPtr<IOPackage> pPackage;
		CVmEvent event(PET_DSP_EVT_REMOVE_BACKUP_FINISHED, m_sVmUuid, PIE_DISPATCHER);
		event.addEventParameter( new CVmEventParameter( PVE::String,
														m_sBackupId,
														EVT_PARAM_BACKUP_CMD_BACKUP_UUID ) );
		pPackage = DispatcherPackage::createInstance(PVE::DspVmEvent, event, getRequestPackage());
		{
			CDspLockedPointer<CVmDirectoryItem> pLockedVmDirItem =
				CDspService::instance()->getVmDirManager().getVmDirItemByUuid(sVmDirUuid, m_sVmUuid);
			isVmExists = ( pLockedVmDirItem );
		}
		if ( isVmExists )
		{
			CDspService::instance()->getClientManager().sendPackageToVmClients(pPackage, sVmDirUuid, m_sVmUuid);
		}
		else
		{
			// VM is not exists on node - send event to task client only
			getClient()->sendPackage(pPackage);
		}

		getClient()->sendSimpleResponse(getRequestPackage(), PRL_ERR_SUCCESS);
	}
	else
		getClient()->sendResponseError(getLastError(), getRequestPackage());
}

namespace Backup
{
namespace Remove
{
///////////////////////////////////////////////////////////////////////////////
// struct Item

PRL_RESULT Item::load(Task_BackupHelper *context_)
{
	PRL_RESULT e;
	QStringList l;
	if (m_number < PRL_PARTIAL_BACKUP_START_NUMBER) {
		BackupItem b;
		if (PRL_FAILED((e = context_->loadBaseBackupMetadata(m_ve, m_uuid, &b))))
			return e;
		l = b.getTibFileList();
		m_data = b;
	} else {
		PartialBackupItem p;
		if (PRL_FAILED((e = context_->loadPartialBackupMetadata(m_ve, m_uuid, m_number, &p))))
			return e;
		l = p.getTibFileList();
		m_data = p;
	}
	foreach(QString f, l)
		m_files << QFileInfo(m_dir.absoluteDir(), f).absoluteFilePath();
	return e;
}

///////////////////////////////////////////////////////////////////////////////
// struct Meta

PRL_RESULT Meta::operator()(const PartialBackupItem& from_, const BackupItem& to_) const
{
	BackupItem b(to_);
	b.setHost(from_.getHost());
	b.setServerUuid(from_.getServerUuid());
	b.setDateTime(from_.getDateTime());
	b.setCreator(from_.getCreator());
	b.setSize(b.getSize() + from_.getSize());
	b.setDescription(from_.getDescription());
	b.setOriginalSize(b.getOriginalSize() + from_.getOriginalSize());
	b.setBundlePermissions(from_.getBundlePermissions());
	return b.saveToFile(m_file);
}

PRL_RESULT Meta::operator()(const PartialBackupItem& from_, const PartialBackupItem& to_) const
{
	PartialBackupItem p(from_);
	p.setNumber(to_.getNumber());
	p.setId(to_.getId());
	p.setTibFileList(to_.getTibFileList());
	return p.saveToFile(m_file);
}

///////////////////////////////////////////////////////////////////////////////
// struct Remover

PRL_RESULT Remover::rmdir(const QString& dir_, CDspTaskFailure fail_)
{
	if (!CFileHelper::ClearAndDeleteDir(dir_)) {
		fail_(dir_);
		WRITE_TRACE(DBG_FATAL, "Can't remove \"%s\" directory", QSTR2UTF8(dir_));
		return PRL_ERR_BACKUP_CANNOT_REMOVE_DIRECTORY;
	}
	return PRL_ERR_SUCCESS;
}

PRL_RESULT Remover::unlink(const QString& file_)
{
	QFile f(file_);
	if (f.remove())
		return PRL_ERR_SUCCESS;
	WRITE_TRACE(DBG_FATAL, "QFile(%s)::remove error: %s", QSTR2UTF8(f.fileName()),
			QSTR2UTF8(f.errorString()));
	return PRL_ERR_OPERATION_FAILED;
}

QList<action_type> Remover::unlinkItem(const Item& item_, const CDspTaskFailure& fail_)
{
	QList<action_type> a;
	a << boost::bind(Remover::rmdir, item_.getDir(), fail_);
	foreach(QString f, item_.getFiles())
		a << boost::bind(Remover::unlink, f);
	return a;
}

QList<action_type> Remover::operator()(const list_type& objects_, quint32 index_)
{
	if (!index_) {
		return QList<action_type>() << boost::bind(Remover::rmdir,
			QFileInfo(objects_.first()->getDir()).absolutePath(),
			CDspTaskFailure(*m_context));
	}

	QList<action_type> a;
	for (int i = index_; i < objects_.size(); i++)
		a << unlinkItem(*objects_.at(i), CDspTaskFailure(*m_context));
	return a;
}

///////////////////////////////////////////////////////////////////////////////
// struct Shifter

PRL_RESULT Shifter::move(const QString& from_, const QString& to_)
{
	QFile f(from_);
	if (f.rename(to_))
		return PRL_ERR_SUCCESS;
	WRITE_TRACE(DBG_FATAL, "QFile::rename from %s to %s failed, error: %s",
			QSTR2UTF8(from_), QSTR2UTF8(to_), QSTR2UTF8(f.errorString()));
	return PRL_ERR_OPERATION_FAILED;
}

PRL_RESULT Shifter::moveDir(const QString& from_, const QString& to_)
{
	QFileInfo f(from_), t(to_);
	QDir b(f.dir());
	if (b.rename(f.fileName(), t.fileName()))
		return PRL_ERR_SUCCESS;
	WRITE_TRACE(DBG_FATAL, "QDir::rename from %s to %s failed!",
					QSTR2UTF8(from_), QSTR2UTF8(to_));
	return PRL_ERR_OPERATION_FAILED;
}

QList<action_type> Shifter::moveItem(const Item& from_, const Item& to_)
{
	QList<action_type> a;
	PRL_RESULT (*f)(const Meta&, item_type&, item_type&) = boost::apply_visitor<Meta, item_type, item_type>;
	a << boost::bind(f, Meta(from_.getDir()), from_.getData(), to_.getData());
	a << boost::bind(Shifter::moveDir, from_.getDir(), to_.getDir());

	if (from_.getFiles().size() != to_.getFiles().size()) {
		WRITE_TRACE(DBG_FATAL, "Different number of archives for backup %s and %s",
				QSTR2UTF8(from_.getDir()), QSTR2UTF8(to_.getDir()));
		return a << action_type();
	}
	for (int i = 0; i < from_.getFiles().size(); i++)
		a << boost::bind(Shifter::move, from_.getFiles().at(i), to_.getFiles().at(i));
	return a;
}

PRL_RESULT Shifter::rebase(const QString& file_, const QString& base_, bool safe_)
{
	QStringList a = QStringList() << QEMU_IMG << "rebase" << "-t" << "none";
	if (safe_) {
		// -u (unsafe) means simply change of base name - a caller
		// guarantee that content of base image is not changed - and it
		// is so indeed, because we just renamed base image and need to
		// reflect this rename in this one.
		a << "-u";
	}
	a << "-b" << base_ << file_;
	WRITE_TRACE(DBG_FATAL, "Run cmd: %s", QSTR2UTF8(a.join(" ")));

	QProcess process;
	DefaultExecHandler h(process, a.join(" "));
	if (!HostUtils::RunCmdLineUtilityEx(a, process,  QEMU_IMG_RUN_TIMEOUT, NULL)(h).isSuccess())
	{
		WRITE_TRACE(DBG_FATAL, "Cannot rebase hdd '%s' to '%s': %s", QSTR2UTF8(file_),
				QSTR2UTF8(base_), h.getStderr().constData());
		return PRL_ERR_BACKUP_INTERNAL_ERROR;
	}
	return PRL_ERR_SUCCESS;
}

QList<action_type> Shifter::rebaseItem(const Item& item_, const Item& base_, bool safe_)
{
	QList<action_type> a;
	for (int i = 0; i < item_.getFiles().size(); i++) {
		QString base = (i < base_.getFiles().size()) ? base_.getFiles().at(i) : ""; 
		a << boost::bind(Shifter::rebase, item_.getFiles().at(i), base, safe_);
	}
	return a;
}

QList<action_type> Shifter::operator()(const list_type& objects_, quint32 index_)
{
	// Overall algorithm:
	// 	* rebase n(next) to c(current)(to be removed) item to the p(previous) one
	// 	* remove c(current)
	// 	* shift n(next) to the place of c(current)
	// 	* repeat shift for all subsequent images, with renames of their bases
	QList<action_type> a;
	element_type c = objects_.at(index_);
	if (objects_.size() == (int)index_ + 1)
		return Remover::unlinkItem(*c, CDspTaskFailure(*m_context));

	element_type n = objects_.at(index_ + 1);
	element_type p = (index_ > 0) ? objects_.at(index_ - 1) : element_type(new Item);
	a << rebaseItem(*n, *p, false);
	a << Remover::unlinkItem(*c, CDspTaskFailure(*m_context));
	a << moveItem(*n, *c);
	for (int i = index_ + 2; i < objects_.size(); i++) {
		n = objects_.at(i);
		c = objects_.at(i - 1);
		element_type p = objects_.at(i - 2);
		a << rebaseItem(*n, *p, true);
		a << moveItem(*n, *c);
	}
	return a;
}

} // namespace Remove
} // namespace Backup

/*******************************************************************************

 Backup removing task for server

********************************************************************************/
Task_RemoveVmBackupTarget::Task_RemoveVmBackupTarget(
		SmartPtr<CDspDispConnection> &pDispConnection,
		CDispToDispCommandPtr cmd,
		const SmartPtr<IOPackage> &p)
:Task_BackupHelper(pDispConnection->getUserSession(), p),
m_pDispConnection(pDispConnection)
{
	CVmBackupRemoveCommand *pCmd =
		CDispToDispProtoSerializer::CastToDispToDispCommand<CVmBackupRemoveCommand>(cmd);
	m_sVmUuid = pCmd->GetVmUuid();
	m_sBackupId = pCmd->GetBackupUuid();
	m_nFlags = pCmd->GetFlags();
	m_nBackupNumber = 0;
	m_nLocked = 0;
}

Prl::Expected<QList<Backup::Remove::action_type>, PRL_RESULT> Task_RemoveVmBackupTarget::prepare()
{
	if (m_sBackupId.isEmpty() && !m_sVmUuid.isEmpty())
		return removeAllBackupsForVm();

	parseBackupId(m_sBackupId, m_sBackupUuid, m_nBackupNumber);

	QStringList b;
	getBaseBackupList(m_sVmUuid, b);
	if (!b.contains(m_sBackupUuid))
		return PRL_ERR_BACKUP_BACKUP_UUID_NOT_FOUND;

	QString path = QString("%1/%2/%3").arg(getBackupDirectory()).arg(m_sVmUuid).arg(m_sBackupUuid);
	Backup::Remove::list_type o;
	o << Backup::Remove::element_type(new Backup::Remove::Item(path, m_sVmUuid, m_sBackupUuid));
	int pos = 0;

	QList<unsigned> p;
	bool shift = m_nFlags & PBT_KEEP_CHAIN;
	if (m_nBackupNumber != PRL_BASE_BACKUP_NUMBER || shift) {
		getPartialBackupList(m_sVmUuid, m_sBackupUuid, p);
		if (p.isEmpty())
			shift = false;
	}
	if ((b.size() == 1) && m_nBackupNumber == PRL_BASE_BACKUP_NUMBER && !shift)
		return removeAllBackupsForVm();
	foreach(unsigned i, p) {
		Backup::Remove::element_type x(
			new Backup::Remove::Item(path, m_sVmUuid, m_sBackupUuid));
		x->setNumber(i);
		if (i == m_nBackupNumber)
			pos = o.size();
		o << x;
	}

	m_lstBackupUuid.append(m_sBackupUuid);

	foreach(Backup::Remove::element_type i, o)
		i->load(this);
	if (shift)
		return Backup::Remove::Shifter(*this)(o, pos);

	return Backup::Remove::Remover(*this)(o, pos);
}

PRL_RESULT Task_RemoveVmBackupTarget::do_(const QList<Backup::Remove::action_type>& actions_)
{
	/* to lock all backups */
	PRL_RESULT e;
	for (m_nLocked = 0; m_nLocked < m_lstBackupUuid.size(); ++m_nLocked) {
		if (PRL_FAILED(e = lockExclusive(m_lstBackupUuid.at(m_nLocked))))
			return e;
	}

	foreach(Backup::Remove::action_type a, actions_) {
		if (a.empty())
			return PRL_ERR_BACKUP_INTERNAL_ERROR;
	}
	foreach(Backup::Remove::action_type a, actions_) {
		if (PRL_FAILED((e = a())))
			return e;
	}
	return PRL_ERR_SUCCESS;
}

PRL_RESULT Task_RemoveVmBackupTarget::run_body()
{
	//https://bugzilla.sw.ru/show_bug.cgi?id=267152
	CAuthHelperImpersonateWrapper _impersonate( &getClient()->getAuthHelper() );

	PRL_RESULT nRetCode = PRL_ERR_SUCCESS;
	Prl::Expected<QList<Backup::Remove::action_type>, PRL_RESULT> e = prepare();
	if (e.isFailed()) {
		nRetCode = e.error();
		goto exit;
	}

	nRetCode = do_(e.value());

exit:
	if (nRetCode == PRL_ERR_BACKUP_BACKUP_UUID_NOT_FOUND) {
		if (m_nFlags & PBT_IGNORE_NOT_EXISTS)
			nRetCode = PRL_ERR_SUCCESS;
		else {
			CDspTaskFailure(*this)(m_sBackupUuid);
			WRITE_TRACE(DBG_FATAL, "Backup \"%s\" does not exist", QSTR2UTF8(m_sBackupUuid));
		}
	}

	setLastErrorCode(nRetCode);
	return nRetCode;
}

void Task_RemoveVmBackupTarget::finalizeTask()
{
	if (m_nLocked)
		for (int i = 0; (i < m_nLocked) && (i < m_lstBackupUuid.size()); ++i)
			unlockExclusive(m_lstBackupUuid.at(i));
	m_nLocked = 0;

	if (PRL_FAILED(getLastErrorCode()))
		m_pDispConnection->sendResponseError(getLastError(), getRequestPackage());
	else
		m_pDispConnection->sendSimpleResponse(getRequestPackage(), PRL_ERR_SUCCESS);
	return;
}

/* to remove all backups of this client for this Vm */
Prl::Expected<QList<Backup::Remove::action_type>, PRL_RESULT> Task_RemoveVmBackupTarget::removeAllBackupsForVm()
{
	/* get base backup list for this user */
	getBaseBackupList(m_sVmUuid, m_lstBackupUuid, &getClient()->getAuthHelper(), PRL_BACKUP_CHECK_MODE_WRITE);
	if (m_lstBackupUuid.isEmpty()) {
		CDspTaskFailure(*this)(m_sVmUuid);
		WRITE_TRACE(DBG_FATAL, "Could not find any backup of the Vm \"%s\"", QSTR2UTF8(m_sVmUuid));
		return PRL_ERR_BACKUP_BACKUP_NOT_FOUND;
	}

	QFileInfo x(QString("%1/%2").arg(getBackupDirectory()).arg(m_sVmUuid));
	return QList<Backup::Remove::action_type>() <<
		boost::bind(Backup::Remove::Remover::rmdir, x.absoluteFilePath(), CDspTaskFailure(*this));
}
