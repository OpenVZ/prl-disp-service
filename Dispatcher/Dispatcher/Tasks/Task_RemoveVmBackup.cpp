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
				if (backup->getUuid() != sBackupUuid)
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
		QString sVmDirUuid = CDspVmDirHelper::getVmDirUuidByVmUuid(m_sVmUuid, getClient());
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

PRL_RESULT Item::load(const Backup::Metadata::Sequence& sequence_)
{
	QStringList l;
	QList<quint32> i = sequence_.getIndex();
	if (!i.contains(m_number))
		return PRL_ERR_BACKUP_BACKUP_NOT_FOUND;

	if (m_number == i.front())
	{
		Prl::Expected<BackupItem, PRL_RESULT> b =
			sequence_.getHeadItem(m_number);
		if (b.isFailed())
			return b.error();
		l = b.value().getTibFileList();
		m_data = b.value();
	}
	else
	{
		Prl::Expected<PartialBackupItem, PRL_RESULT> p =
			sequence_.getTailItem(m_number);
		if (p.isFailed())
			return p.error();
		l = p.value().getTibFileList();
		m_data = p.value();
	}
	m_files.clear();
	m_lair = sequence_.showItemLair(m_number).absolutePath();
	foreach(const QString& f, l)
		m_files << QFileInfo(sequence_.showLair(), f).absoluteFilePath();

	return PRL_ERR_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
// struct Meta

PRL_RESULT Meta::operator()(const BackupItem& from_, const PartialBackupItem& to_) const
{
	BackupItem b;
	b.setId(to_.getId());
	b.setUuid(from_.getUuid());
	b.setHost(to_.getHost());
	b.setServerUuid(to_.getServerUuid());
	b.setDateTime(to_.getDateTime());
	b.setCreator(to_.getCreator());
	b.setSize(getSize());
	b.setDescription(to_.getDescription());
	b.setOriginalSize(to_.getOriginalSize() + from_.getOriginalSize());
	b.setBundlePermissions(to_.getBundlePermissions());
	b.setTibFileList(to_.getTibFileList());
	b.setFlags(from_.getFlags());
	b.setLastNumber(from_.getLastNumber());
	return m_sequence.save(b, m_item->getNumber());
}

PRL_RESULT Meta::operator()(const PartialBackupItem& from_, const PartialBackupItem& to_) const
{
	Q_UNUSED(from_);
	PartialBackupItem p(to_);
	p.setSize(getSize());
	return m_sequence.update(p, m_item->getNumber());
}

qulonglong Meta::getSize() const
{
	qulonglong output = 0;
	foreach (const QString& t, m_item->getFiles())
		output += QFileInfo(t).size();

	return output;
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
	a << boost::bind(Remover::rmdir, item_.getLair(), fail_);
	foreach(QString f, item_.getFiles())
		a << boost::bind(Remover::unlink, f);
	return a;
}

QList<action_type> Remover::operator()(const list_type& objects_, quint32 index_)
{
	if (!index_) {
		return QList<action_type>() << boost::bind(Remover::rmdir,
			QFileInfo(objects_.first()->getLair()).absolutePath(),
			CDspTaskFailure(*m_context));
	}

	QList<action_type> a;
	for (int i = index_; i < objects_.size(); i++)
		a << unlinkItem(*objects_.at(i), CDspTaskFailure(*m_context));
	return a;
}

///////////////////////////////////////////////////////////////////////////////
// struct Shifter

PRL_RESULT Shifter::rebase(const QString& file_, const QString& base_)
{
	QStringList a = QStringList() << QEMU_IMG << "rebase" << "-t" << "none";
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

QList<action_type> Shifter::rebaseItem(const Item& item_, const Item& base_)
{
	QList<action_type> a;
	for (int i = 0; i < item_.getFiles().size(); i++) {
		QString base = (i < base_.getFiles().size()) ? base_.getFiles().at(i) : ""; 
		a << boost::bind(Shifter::rebase, item_.getFiles().at(i), base);
	}
	return a;
}

QList<action_type> Shifter::operator()(quint32 item_)
{
	element_type c(new Item(item_));
	c->load(m_sequence);
	if (m_next.isNull())
		return Remover::unlinkItem(*c, CDspTaskFailure(*m_context));

	element_type n = m_next, p = m_preceding;
	n->load(m_sequence);
	if (p.isNull())
		p = element_type(new Item());
	else
		p->load(m_sequence);

	QList<action_type> output = rebaseItem(*n, *p);
	PRL_RESULT (*f)(const Meta&, item_type&, item_type&) =
		&boost::apply_visitor<Meta, item_type, item_type>;
	output << boost::bind(f, Meta(n, m_sequence), c->getData(), n->getData());
	output << Remover::unlinkItem(*c, CDspTaskFailure(*m_context));
	return output;
}

void Shifter::setNext(quint32 value_)
{
	m_next = element_type(new Item(value_));
}

void Shifter::setPreceding(quint32 value_)
{
	m_preceding = element_type(new Item(value_));
}

///////////////////////////////////////////////////////////////////////////////
// struct Confectioner

Confectioner::result_type Confectioner::operator()
	(Shifter producer_,Prl::Expected<VmItem, PRL_RESULT> catalog_) const
{
	if (catalog_.isFailed())
		return PRL_ERR_BACKUP_BACKUP_NOT_FOUND;

	if (catalog_.value().getVersion() < BACKUP_PROTO_V4)
		return PRL_ERR_BACKUP_UNSUPPORTED_KEEP_CHAIN;

	QList<quint32>::const_iterator p = std::find
		(m_index.constBegin(), m_index.constEnd(), m_number);
	if (m_index.constEnd() == p)
		return PRL_ERR_BACKUP_BACKUP_NOT_FOUND;

	if (m_index.constBegin() != p)
		producer_.setPreceding(*(p - 1));
	if (m_number != m_index.last())
		producer_.setNext(*(p + 1));

	return producer_(m_number);
}

Confectioner::result_type Confectioner::operator()
	(Remover producer_, const Metadata::Sequence& sequence_) const
{
	int z = 0;
	list_type o;
	foreach(quint32 n, m_index)
	{
		element_type x(new Item(n));
		x->load(sequence_);
		if (n == m_number)
			z = o.size();
		o << x;
	}
	return producer_(o, z);
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

	Backup::Metadata::Catalog c(getCatalog(m_sVmUuid));
	parseBackupId(m_sBackupId, m_sBackupUuid, m_nBackupNumber);
	QStringList u = c.getIndexForRead();
	if (!u.contains(m_sBackupUuid))
		return PRL_ERR_BACKUP_BACKUP_UUID_NOT_FOUND;

	PRL_RESULT e = getMetadataLock().grabShared(m_sBackupUuid);
	if (PRL_FAILED(e))
		return e;

	Backup::Metadata::Sequence s = c.getSequence(m_sBackupUuid);
	QList<quint32> i = s.getIndex();
	Prl::Expected<QList<Backup::Remove::action_type>, PRL_RESULT> output;
	if (1 == i.size() && u.size() == 1)
		output = removeAllBackupsForVm();
	else
	{
		Backup::Remove::Confectioner x(i, m_nBackupNumber);
		if (m_nFlags & PBT_KEEP_CHAIN)
			output = x(Backup::Remove::Shifter(*this, s), c.loadItem());
		else
			output = x(Backup::Remove::Remover(*this), s);

		if (output.isSucceed())
			m_lstBackupUuid.append(m_sBackupUuid);
		else
			CDspTaskFailure(*this)(m_sVmUuid);
	}
	getMetadataLock().releaseShared(m_sBackupUuid);
	return output;
}

PRL_RESULT Task_RemoveVmBackupTarget::do_(const QList<Backup::Remove::action_type>& actions_)
{
	/* to lock all backups */
	PRL_RESULT e;
	for (m_nLocked = 0; m_nLocked < m_lstBackupUuid.size(); ++m_nLocked) {
		if (PRL_FAILED(e = getMetadataLock().grabExclusive(m_lstBackupUuid.at(m_nLocked))))
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
			getMetadataLock().releaseExclusive(m_lstBackupUuid.at(i));
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
	m_lstBackupUuid = getCatalog(m_sVmUuid).getIndexForWrite(getClient()->getAuthHelper());
	if (m_lstBackupUuid.isEmpty()) {
		CDspTaskFailure(*this)(m_sVmUuid);
		WRITE_TRACE(DBG_FATAL, "Could not find any backup of the Vm \"%s\"", QSTR2UTF8(m_sVmUuid));
		return PRL_ERR_BACKUP_BACKUP_NOT_FOUND;
	}

	QString x = Backup::Metadata::Carcass(getBackupDirectory(), m_sVmUuid).getCatalog().absolutePath();
	return QList<Backup::Remove::action_type>() <<
		boost::bind(Backup::Remove::Remover::rmdir, x, CDspTaskFailure(*this));
}
