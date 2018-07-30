///////////////////////////////////////////////////////////////////////////////
///
/// @file Task_GetBackupTree.cpp
///
/// Source & target tasks for backup tree listing
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

#include "Interfaces/Debug.h"
#include "prlcommon/Interfaces/ParallelsQt.h"
#include "prlcommon/Interfaces/ParallelsNamespace.h"
#include <boost/foreach.hpp>
#include "prlcommon/Logging/Logging.h"
#include "Libraries/StatesStore/SavedStateTree.h"

#include "Task_GetBackupTree.h"
#include "CDspService.h"
#include "Libraries/DispToDispProtocols/CVmBackupProto.h"
#include "prlcommon/Std/PrlAssert.h"
#include "prlxmlmodel/BackupTree/BackupTree.h"
#include "Libraries/PrlCommonUtils/CFileHelper.h"

namespace Backup
{
namespace Tree
{
///////////////////////////////////////////////////////////////////////////////
// struct Branch

BackupItem* Branch::show() const
{
	QList<quint32> x = m_metadata.getIndex();
	if (x.isEmpty())
		return NULL;

	return filterList(x.first(), x);
}

BackupItem* Branch::filterOne(const QString& sequence_, quint32 number_) const
{
	if (sequence_ != m_uuid)
		return NULL;
	QList<quint32> x = m_metadata.getIndex();
	if (!x.contains(number_))
		return NULL;

	return filterList(x.first(), QList<quint32>() << number_);
}

BackupItem* Branch::filterChain(const QString& sequence_, quint32 number_) const
{
	if (sequence_ != m_uuid)
		return NULL;

	QList<quint32> x = m_metadata.getIndex();
	QList<quint32>::iterator p = std::find(x.begin(), x.end(), number_);
	if (x.constEnd() == p)
		return NULL;

	quint32 h = x.first();
	x.erase(x.begin(), p);
	return filterList(h, x);
}

BackupItem* Branch::filterList(quint32 head_, QList<quint32> filter_) const
{
	if (filter_.isEmpty())
		return NULL;

	Prl::Expected<BackupItem, PRL_RESULT> b = m_metadata.getHeadItem(head_);
	if (b.isFailed())
		return NULL;

	Prl::Expected<CBackupDisks, PRL_RESULT> d =
		Backup::Metadata::Sequence(m_metadata).getDisks(head_);
	if (d.isFailed())
		return NULL;

	filter_.removeOne(head_);
	BackupItem* output = new BackupItem(b.value());
	output->setBackupDisks(new CBackupDisks(d.value()));
	foreach (quint32 n, filter_)
	{
		Prl::Expected<PartialBackupItem, PRL_RESULT> p = m_metadata.getTailItem(n);
		if (p.isFailed())
			continue;
		d = Backup::Metadata::Sequence(m_metadata).getDisks(n);
		if (d.isFailed())
			continue;

		output->m_lstPartialBackupItem << new PartialBackupItem(p.value());
		output->m_lstPartialBackupItem.last()->setBackupDisks
			(new CBackupDisks(d.value()));
	}
	return output;
}

} // namespace Tree
} // namespace Backup

/*******************************************************************************

 Backup creation task for client

********************************************************************************/
Task_GetBackupTreeSource::Task_GetBackupTreeSource(
		const SmartPtr<CDspClient>& client,
		const CProtoCommandPtr cmd,
		const SmartPtr<IOPackage>& p)
:Task_BackupHelper(client, p),
 m_sUuid(m_sVmUuid)
{
	CProtoGetBackupTreeCommand *pCmd = CProtoSerializer::CastToProtoCommand<CProtoGetBackupTreeCommand>(cmd);
	m_sUuid = pCmd->GetVmUuid();
	m_sServerHostname = pCmd->GetServerHostname();
	m_nServerPort = pCmd->GetServerPort();
	m_sServerSessionUuid = pCmd->GetServerSessionUuid();
	m_nFlags = pCmd->GetFlags();
	if (!(m_nFlags & PBT_VM) && !(m_nFlags & PBT_CT))
		/* to show VM by default */
		m_nFlags |= PBT_VM;
}

PRL_RESULT Task_GetBackupTreeSource::prepareTask()
{
	PRL_RESULT ret = PRL_ERR_SUCCESS;
	try {
		if ((m_nFlags & (PBT_BACKUP_ID | PBT_CHAIN)) && m_sUuid.isEmpty()) {
			WRITE_TRACE(DBG_FATAL, "A backup UUID must be specified to get information "
								   "on a single backup or backup chain.");
			throw PRL_ERR_INVALID_PARAM;
		}
		if ((m_nFlags & PBT_BACKUP_ID) && (m_nFlags & PBT_CHAIN)) {
			WRITE_TRACE(DBG_FATAL, "The PBT_BACKUP_ID and PBT_CHAIN flags are mutually exclusive.");
			throw PRL_ERR_INVALID_PARAM;
		}
	} catch (PRL_RESULT code) {
		getLastError()->setEventCode(code);
		ret = code;
		WRITE_TRACE(DBG_FATAL, "An error occurred while preparing to get the backup tree [%#x][%s]",
			code, PRL_RESULT_TO_STRING(code));
	}

	return ret;
}

PRL_RESULT Task_GetBackupTreeSource::run_body()
{
	//https://bugzilla.sw.ru/show_bug.cgi?id=267152
	CAuthHelperImpersonateWrapper _impersonate( &getClient()->getAuthHelper() );

	PRL_RESULT nRetCode = PRL_ERR_SUCCESS;
	QFileInfoList excludeList;
	QFileInfoList dirList;
	QFileInfoList fileList;

	if (PRL_FAILED(nRetCode = connect()))
		goto exit;

	if (PRL_FAILED(nRetCode = GetBackupTreeRequest(m_sUuid, m_sBackupTree)))
		goto exit;


exit:
	setLastErrorCode(nRetCode);
	return nRetCode;
}

/* Finalize task */
void Task_GetBackupTreeSource::finalizeTask()
{
	Disconnect();

	if (PRL_SUCCEEDED(getLastErrorCode())) {
		CProtoCommandPtr pResponse = CProtoSerializer::CreateDspWsResponseCommand(getRequestPackage(), PRL_ERR_SUCCESS);
		CProtoCommandDspWsResponse *pDspWsResponseCmd =
			CProtoSerializer::CastToProtoCommand<CProtoCommandDspWsResponse>(pResponse);
		pDspWsResponseCmd->SetBackupsTree(m_sBackupTree);

		getClient()->sendResponse( pResponse, getRequestPackage() );
	} else {
		getClient()->sendResponseError( getLastError(), getRequestPackage() );
	}
}



/*******************************************************************************

 Backup creation task for server

********************************************************************************/
Task_GetBackupTreeTarget::Task_GetBackupTreeTarget(
		SmartPtr<CDspDispConnection> &pDispConnection,
		CDispToDispCommandPtr pCmd,
		const SmartPtr<IOPackage> &p)
:Task_BackupHelper(pDispConnection->getUserSession(), p),
m_pDispConnection(pDispConnection),
m_sUuid(m_sVmUuid)
{
	CGetBackupTreeCommand *pStartCommand =
		CDispToDispProtoSerializer::CastToDispToDispCommand<CGetBackupTreeCommand>(pCmd);
	m_sUuid = pStartCommand->GetVmUuid();
	m_nFlags = pStartCommand->GetFlags();
	if (!(m_nFlags & PBT_VM) && !(m_nFlags & PBT_CT))
		/* to show VM by default */
		m_nFlags |= PBT_VM;
}

PRL_RESULT Task_GetBackupTreeTarget::run_body()
{
	//https://bugzilla.sw.ru/show_bug.cgi?id=267152
	CAuthHelperImpersonateWrapper _impersonate( &getClient()->getAuthHelper() );

	QString msg;
	getBackupTree(msg);

	PRL_RESULT nRetCode = PRL_ERR_SUCCESS;
	if (isConnected())
	{
		CDispToDispCommandPtr pReply = CDispToDispProtoSerializer::CreateGetBackupTreeReply(msg);
		SmartPtr<IOPackage> pPackage = DispatcherPackage::createInstance(
				pReply->GetCommandId(),
				pReply->GetCommand()->toString(),
				getRequestPackage());

		IOSendJob::Handle hJob = m_pDispConnection->sendPackage(pPackage);
		if (CDspService::instance()->getIOServer().waitForSend(hJob, m_nTimeout) != IOSendJob::Success) {
			WRITE_TRACE(DBG_FATAL, "Package sending failure");
			setLastErrorCode(nRetCode = PRL_ERR_BACKUP_INTERNAL_PROTO_ERROR);
		}
	}
	else
		setLastErrorCode(nRetCode = getCancelResult());

	return nRetCode;
}

void Task_GetBackupTreeTarget::getBackupTree(QString &msg)
{
	if (!isConnected())
		return;

	QString s;
	quint32 n = 0;
	if (backupFilterEnabled())
		parseBackupId(m_sUuid, s, n); 
		
	BackupTree bTree;
	/* set empty xml string at the first (https://jira.sw.ru/browse/PSBM-9137) */
	foreach (const QFileInfo& e, QDir(getBackupDirectory()).entryInfoList(QDir::NoDotAndDotDot | QDir::Dirs))
	{
		if (!isConnected())
			return;

		const QString sVmUuid = e.fileName();
		if (!Uuid::isUuid(sVmUuid))
			continue;
		if (!backupFilterEnabled() && !m_sUuid.isEmpty() && (m_sUuid != sVmUuid))
			continue;

		Backup::Metadata::Catalog c = getCatalog(sVmUuid);
		Prl::Expected<VmItem, PRL_RESULT> v = c.loadItem();
		if (v.isFailed())
			continue;

		/* skip inappropriate vmtype */
		bool t = PVBT_VM == v.value().getVmType();
		if ((t && (m_nFlags & PBT_VM) == 0) || (!t && (m_nFlags & PBT_CT) == 0))
			continue;

		BOOST_FOREACH (const QString& sBackupUuid, c.getIndexForRead(&getClient()->getAuthHelper()))
		{
			if (!isConnected())
				return;

			if (PRL_FAILED(getMetadataLock().grabShared(sBackupUuid)))
				continue;

			BackupItem* i;
			Backup::Tree::Branch b(sBackupUuid, c.getSequence(sBackupUuid));
			if (filterSingleBackup())
				i = b.filterOne(s, n);
			else if (filterBackupChain() && v.value().m_lstBackupItem.isEmpty())
				i = b.filterChain(s, n);
			else
				i = b.show();
			getMetadataLock().releaseShared(sBackupUuid);
			if (NULL == i)
				continue;

			v.value().m_lstBackupItem << i;
			if (filterSingleBackup())
				break;
		}
		if (v.value().m_lstBackupItem.isEmpty()) {
			/* to skip Vm directories without backups */
			continue;
		}
		std::sort(v.value().m_lstBackupItem.begin(), v.value().m_lstBackupItem.end(),
			boost::bind(&BackupItem::getDateTime, _1) <
			boost::bind(&BackupItem::getDateTime, _2));
		bTree.m_lstVmItem.append(new VmItem(v.value()));
		/* if we searched for a backup or a chain - it is already found here */
		if (backupFilterEnabled())
			break;
	}
	msg = bTree.toString();
}

bool Task_GetBackupTreeTarget::isConnected() const
{
	return m_pDispConnection.isValid() && CDspService::instance()->getIOServer()
		.clientState(m_pDispConnection->GetConnectionHandle()) == IOSender::Connected;
}
