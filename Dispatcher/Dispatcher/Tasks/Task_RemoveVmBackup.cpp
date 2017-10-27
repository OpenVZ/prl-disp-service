///////////////////////////////////////////////////////////////////////////////
///
/// @file Task_RemoveVmBackup.cpp
///
/// Source and target tasks for Vm backup restoring
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

#include "Interfaces/Debug.h"
#include "Task_RemoveVmBackup_p.h"
#include "prlcommon/Interfaces/ParallelsQt.h"
#include "prlcommon/Interfaces/ParallelsNamespace.h"

#include "prlcommon/Logging/Logging.h"
#include "prlcommon/HostUtils/HostUtils.h"
#include "Libraries/StatesStore/SavedStateTree.h"

#include "Task_RemoveVmBackup.h"
#include "Tasks/Task_RegisterVm.h"
#include "Libraries/DispToDispProtocols/CVmBackupProto.h"
#include "CDspService.h"
#include <boost/foreach.hpp>
#include <boost/range/combine.hpp>
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
			nRetCode = CDspTaskFailure(*this)
				(PRL_ERR_BACKUP_BACKUP_UUID_NOT_FOUND, m_sBackupId);
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
			nRetCode = CDspTaskFailure(*this)
				(PRL_ERR_BACKUP_BACKUP_UUID_NOT_FOUND, m_sBackupId);
			WRITE_TRACE(DBG_FATAL, "Failed to find vm for backupId \"%s\".",
				QSTR2UTF8(sBackupUuid));
			goto exit;
		}
	}

	pCommand = CDispToDispProtoSerializer::CreateVmBackupRemoveCommand(m_sVmUuid, m_sBackupId, m_nFlags);
	pPackage = DispatcherPackage::createInstance(pCommand->GetCommandId(), pCommand->GetCommand()->toString());

	if (PRL_FAILED(nRetCode = SendReqAndWaitReplyLong(pPackage, pReply, ~0U)))
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
	return m_sequence.save(b, m_item.getNumber());
}

PRL_RESULT Meta::operator()(const PartialBackupItem& from_, const PartialBackupItem& to_) const
{
	Q_UNUSED(from_);
	PartialBackupItem p(to_);
	p.setSize(getSize());
	return m_sequence.update(p, m_item.getNumber());
}

qulonglong Meta::getSize() const
{
	qulonglong output = 0;
	foreach (const QString& t, m_item.getFiles())
		output += QFileInfo(t).size();

	return output;
}

///////////////////////////////////////////////////////////////////////////////
// struct Flavor

Flavor::Flavor(Task_BackupHelper& context_): m_failure(context_)
{
}

batch_type Flavor::operator()(const Item& item_)
{
	batch_type output;
	output.addItem(boost::bind<PRL_RESULT>(*this, item_.getLair()));
	foreach(QString f, item_.getFiles())
		output.addItem(boost::bind(Flavor::unlink, f));

	return output;
}

PRL_RESULT Flavor::operator()(const QString& folder_)
{
	if (!CFileHelper::ClearAndDeleteDir(folder_))
	{
		m_failure(folder_);
		WRITE_TRACE(DBG_FATAL, "Can't remove \"%s\" directory",
			QSTR2UTF8(folder_));
		return PRL_ERR_BACKUP_CANNOT_REMOVE_DIRECTORY;
	}

	return PRL_ERR_SUCCESS;
}

PRL_RESULT Flavor::unlink(const QString& file_)
{
	QFile f(file_);
	if (f.remove())
		return PRL_ERR_SUCCESS;
	WRITE_TRACE(DBG_FATAL, "QFile(%s)::remove error: %s", QSTR2UTF8(f.fileName()),
			QSTR2UTF8(f.errorString()));
	return PRL_ERR_OPERATION_FAILED;
}

///////////////////////////////////////////////////////////////////////////////
// struct Confectioner

Confectioner::result_type Confectioner::operator()
	(Coalesce::Confectioner producer_, Prl::Expected<VmItem, PRL_RESULT> catalog_) const
{
	if (catalog_.isFailed())
		return PRL_ERR_BACKUP_BACKUP_NOT_FOUND;

	if (catalog_.value().getVersion() < BACKUP_PROTO_V4)
		return PRL_ERR_BACKUP_UNSUPPORTED_KEEP_CHAIN;

	return producer_(m_number);
}

Confectioner::result_type Confectioner::operator()
	(Flavor flavor_, const Metadata::Sequence& sequence_) const
{
	int z = 0;
	QList<Item> o;
	foreach(quint32 n, m_index)
	{
		Item x(n);
		x.load(sequence_);
		if (n == m_number)
			z = o.size();
		o << x;
	}
	batch_type output;
	if (0 == z)
	{
		if (!o.isEmpty())
		{
			output.addItem(boost::bind<PRL_RESULT>(flavor_,
				QFileInfo(o.first().getLair()).absolutePath()));
		}
	}
	else
	{
		for (int i = z; i < o.size(); ++i)
		{
			output.addItem(flavor_(o.at(i)));
		}
	}
	return output;
}

} // namespace Remove

namespace Coalesce
{
///////////////////////////////////////////////////////////////////////////////
// struct Setup

Prl::Expected<Setup::item_type, PRL_RESULT> Setup::getLeader()
{
	QList<quint32>::const_iterator e = m_index.constEnd();
	QList<quint32>::const_iterator p = std::find(m_index.constBegin(), e, m_point);
	if (e == p || (++p) == e)
		return PRL_ERR_BACKUP_BACKUP_NOT_FOUND;

	return find(*p);
}

Prl::Expected<Setup::item_type, PRL_RESULT> Setup::getVictim()
{
	if (!m_index.contains(m_point))
		return PRL_ERR_BACKUP_BACKUP_NOT_FOUND;

	return find(m_point);
}

Prl::Expected<QList<Setup::item_type>, PRL_RESULT> Setup::getTail()
{
	QList<quint32>::const_iterator b = m_index.begin();
	QList<quint32>::const_iterator e = m_index.end();
	QList<quint32>::const_iterator p = std::find(b, e, m_point);
	if (e == p)
		return PRL_ERR_BACKUP_BACKUP_NOT_FOUND;

	QList<item_type> output;
	if (b != p)
	{
		Prl::Expected<item_type, PRL_RESULT> x = find(*(--p));
		if (x.isFailed())
			return x.error();

		output << x.value();
	}

	return output;
}

Prl::Expected<Setup::item_type, PRL_RESULT> Setup::find(quint32 point_)
{
	if (m_cache.contains(point_))
		return m_cache[point_];

	item_type i(point_);
	PRL_RESULT e = i.load(*m_sequence);
	if (PRL_FAILED(e))
		return e;

	return m_cache[point_] = i;
}

///////////////////////////////////////////////////////////////////////////////
// struct Instrument

Instrument::Instrument(const QString& home_, Setup& setup_): m_setup(&setup_)
{
	m_result.setVmType(PVT_VM);
	CVmHardware* h = m_result.getVmHardwareList();
	h->getCpu()->setNumber(1);
	h->getCpu()->setSockets(1);

	Uuid u(Uuid::createUuid());
	CVmIdentification* i = m_result.getVmIdentification();
	i->setHomePath(home_);
	i->setVmName(u.toStringWithoutBrackets());
	i->setVmUuid(u.toString());
}

PRL_RESULT Instrument::addImages()
{
	Prl::Expected<Setup::item_type, PRL_RESULT> a = m_setup->getLeader();
	if (a.isFailed())
		return a.error();

	CVmHardware* h = new CVmHardware();
	m_result.setVmHardwareList(h);
	foreach (const QString& f, a.value().getFiles())
	{
		CVmHardDisk* d = new CVmHardDisk();
		d->setSystemName(f);
		d->setUserFriendlyName(f);
		d->setEnabled(PVE::DeviceEnabled);
		d->setConnected(PVE::DeviceConnected);
		d->setItemId(h->m_lstHardDisks.size());
		d->setIndex(h->m_lstHardDisks.size());
		d->setInterfaceType(PMS_SCSI_DEVICE);
		d->setEmulatedType(PVE::HardDiskImage);

		h->m_lstHardDisks << d;
	}

	return PRL_ERR_SUCCESS;
}

PRL_RESULT Instrument::addBackingStores()
{
	Prl::Expected<Setup::item_type, PRL_RESULT> a =
		m_setup->getVictim();
	if (a.isFailed())
		return a.error();

	Prl::Expected<QList<Setup::item_type>, PRL_RESULT> b =
		m_setup->getTail();
	if (b.isFailed())
		return a.error();

	if (m_result.getVmHardwareList()->m_lstHardDisks.isEmpty())
		return PRL_ERR_INVALID_HANDLE;

	foreach (CVmHardDisk* d, m_result.getVmHardwareList()->m_lstHardDisks)
	{
		QList<CVmHddPartition* > b;
		d->m_lstPartition.swap(b);
		CVmHardDisk y = *d;
		y.m_lstPartition.swap(b);
	}

	QList<Setup::item_type> x = b.value() << a.value();
	foreach (const Setup::item_type& y, x)
	{
		QString f;
		CVmHardDisk* d;
		BOOST_FOREACH(boost::tie(f, d), boost::combine
			(y.getFiles(), m_result.getVmHardwareList()->m_lstHardDisks))
		{
			CVmHddPartition* p = new CVmHddPartition();
			p->setSystemName(f);
			d->m_lstPartition << p;
		}
	}

	return PRL_ERR_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
// struct Confectioner

Prl::Expected<batch_type, PRL_RESULT> Confectioner::operator()(quint32 item_)
{
	Setup s(m_sequence, item_);
	Prl::Expected<Setup::item_type, PRL_RESULT> v = s.getVictim();
	if (v.isFailed())
		return v.error();

	Prl::Expected<Setup::item_type, PRL_RESULT> l = s.getLeader();
	if (l.isFailed())
		return Remove::Flavor(*m_context)(v.value());

	Instrument i(m_sequence.showLair().absolutePath(), s);
	PRL_RESULT e = i.addImages();
	if (PRL_FAILED(e))
		return e;

	e = i.addBackingStores();
	if (PRL_FAILED(e))
		return e;

	::Libvirt::Instrument::Agent::Vm::Grub::result_type u =
		Libvirt::Kit.vms().getGrub(i.getResult()).spawnPaused();
	if (u.isFailed())
		return u.error().code();

	PRL_RESULT (*f)(const Remove::Meta&, Remove::item_type&, Remove::item_type&) =
		&boost::apply_visitor<Remove::Meta, Remove::item_type, Remove::item_type>;

	batch_type output;
	output.addItem(Unit(u.value()));
	output.addItem(boost::bind(f, Remove::Meta(l.value(), m_sequence),
		v.value().getData(), l.value().getData()));
	output.addItem(Remove::Flavor(*m_context)(v.value()));

	return output;
}

///////////////////////////////////////////////////////////////////////////////
// struct Unit

PRL_RESULT Unit::operator()()
{
	CVmConfiguration x;
	::Libvirt::Result e = m_agent.getConfig(x, true);
	if (e.isFailed())
		return e.error().code();

	namespace mb = ::Libvirt::Instrument::Agent::Vm::Block;

	mb::Launcher::imageList_type y;
	foreach (CVmHardDisk* d, x.getVmHardwareList()->m_lstHardDisks)
	{
		y << *d;
	}
	mb::Completion c;
	// rebase can fail. add a handler.
	mb::Activity a = m_agent.getVolume().rebase(y, c);
	c.wait();
	a.stop();
	m_agent.getState().kill();

	return PRL_ERR_SUCCESS;
}

} // namespace Coalesce
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

Prl::Expected<Task_RemoveVmBackupTarget::batch_type, PRL_RESULT>
	Task_RemoveVmBackupTarget::prepare()
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
	Prl::Expected<batch_type, PRL_RESULT> output;
	if (1 == i.size() && u.size() == 1)
		output = removeAllBackupsForVm();
	else
	{
		Backup::Remove::Confectioner x(i, m_nBackupNumber);
		if (m_nFlags & PBT_KEEP_CHAIN)
			output = x(Backup::Coalesce::Confectioner(*this, s), c.loadItem());
		else
			output = x(Backup::Remove::Flavor(*this), s);

		if (output.isSucceed())
			m_lstBackupUuid.append(m_sBackupUuid);
		else
			CDspTaskFailure(*this)(m_sVmUuid);
	}
	getMetadataLock().releaseShared(m_sBackupUuid);
	return output;
}

PRL_RESULT Task_RemoveVmBackupTarget::do_(batch_type batch_)
{
	/* to lock all backups */
	PRL_RESULT e;
	for (m_nLocked = 0; m_nLocked < m_lstBackupUuid.size(); ++m_nLocked) {
		if (PRL_FAILED(e = getMetadataLock().grabExclusive(m_lstBackupUuid.at(m_nLocked))))
			return e;
	}

	return batch_.execute();
}

PRL_RESULT Task_RemoveVmBackupTarget::run_body()
{
	//https://bugzilla.sw.ru/show_bug.cgi?id=267152
	CAuthHelperImpersonateWrapper _impersonate( &getClient()->getAuthHelper() );

	PRL_RESULT nRetCode = PRL_ERR_SUCCESS;
	Prl::Expected<batch_type, PRL_RESULT> e = prepare();
	nRetCode = e.isFailed() ? e.error() : do_(e.value());
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
Prl::Expected<Task_RemoveVmBackupTarget::batch_type, PRL_RESULT> Task_RemoveVmBackupTarget::removeAllBackupsForVm()
{
	/* get base backup list for this user */
	m_lstBackupUuid = getCatalog(m_sVmUuid).getIndexForWrite(getClient()->getAuthHelper());
	if (m_lstBackupUuid.isEmpty()) {
		CDspTaskFailure(*this)(m_sVmUuid);
		WRITE_TRACE(DBG_FATAL, "Could not find any backup of the Vm \"%s\"", QSTR2UTF8(m_sVmUuid));
		return PRL_ERR_BACKUP_BACKUP_NOT_FOUND;
	}

	batch_type output;
	QString x = Backup::Metadata::Carcass(getBackupDirectory(), m_sVmUuid).getCatalog().absolutePath();
	output.addItem(boost::bind<PRL_RESULT>(Backup::Remove::Flavor(*this), x));

	return output;
}
