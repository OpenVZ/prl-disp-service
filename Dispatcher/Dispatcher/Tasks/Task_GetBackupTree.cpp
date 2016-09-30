///////////////////////////////////////////////////////////////////////////////
///
/// @file Task_GetBackupTree.cpp
///
/// Source & target tasks for backup tree listing
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

#include "Interfaces/Debug.h"
#include "prlcommon/Interfaces/ParallelsQt.h"
#include "prlcommon/Interfaces/ParallelsNamespace.h"

#include "prlcommon/Logging/Logging.h"
#include "Libraries/StatesStore/SavedStateTree.h"

#include "Task_GetBackupTree.h"
#include "CDspService.h"
#include "Libraries/DispToDispProtocols/CVmBackupProto.h"
#include "prlcommon/Std/PrlAssert.h"
#include "prlxmlmodel/BackupTree/BackupTree.h"
#include "Libraries/PrlCommonUtils/CFileHelper.h"

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

	PRL_RESULT nRetCode = PRL_ERR_SUCCESS;
	QString msg;
	CDispToDispCommandPtr pReply;
	SmartPtr<IOPackage> pPackage;

	getBackupTree(msg);
	pReply = CDispToDispProtoSerializer::CreateGetBackupTreeReply(msg);
	pPackage = DispatcherPackage::createInstance(
			pReply->GetCommandId(),
			pReply->GetCommand()->toString(),
			getRequestPackage());

	IOSendJob::Handle hJob = m_pDispConnection->sendPackage(pPackage);
	if (CDspService::instance()->getIOServer().waitForSend(hJob, m_nTimeout) != IOSendJob::Success) {
		WRITE_TRACE(DBG_FATAL, "Package sending failure");
		setLastErrorCode(PRL_ERR_BACKUP_INTERNAL_PROTO_ERROR);
		return PRL_ERR_BACKUP_INTERNAL_PROTO_ERROR;
	}

	return nRetCode;
}

template <class T>
PRL_RESULT Task_GetBackupTreeTarget::addDisks(T& entry,
	const VmItem& vm, const QString& uuid, unsigned number)
{
	QString path = QString("%1/%2/%3").arg(getBackupDirectory()).arg(vm.getUuid()).arg(uuid);
	if (number == PRL_BASE_BACKUP_NUMBER)
		path = QString("%1/" PRL_BASE_BACKUP_DIRECTORY).arg(path);
	else
		path = QString("%1/%2").arg(path).arg(number);

	QScopedPointer<CBackupDisks> list(new (std::nothrow) CBackupDisks);
	if (!list)
		return PRL_ERR_OUT_OF_MEMORY;
	SmartPtr<CVmConfiguration> conf;
	PRL_RESULT res = loadVeConfig(uuid, path, vm.getVmType(), conf);
	if (PRL_FAILED(res))
		return res;
	conf->setRelativePath();
	/* XXX: empty home doesn't work here, so generate some random cookie */
	QString home("/" + Uuid::createUuid().toString());
	Backup::Product::Model p(Backup::Object::Model(conf), home);
	Backup::Product::componentList_type archives;
	if (BACKUP_PROTO_V4 <= vm.getVersion()) {
		p.setSuffix(::Backup::Suffix(number)());
		archives = p.getVmTibs();
	} else if (vm.getVmType() == PVBT_VM)
		archives = p.getVmTibs();
	else
		archives = p.getCtTibs();

	foreach(const Backup::Product::component_type& a, archives) {
		CBackupDisk *b = new (std::nothrow) CBackupDisk;
		if (!b)
			return PRL_ERR_OUT_OF_MEMORY;
		b->setName(a.second.fileName());
		QFileInfo fi(a.first.getImage());
		/* use relative paths for disks that reside in the VM home directory */
		b->setOriginalPath(fi.dir() == home ? fi.fileName() : fi.filePath());
		b->setSize(a.first.getDevice().getSize() << 20);
		CVmHddEncryption *e = a.first.getDevice().getEncryption();
		if (e)
			b->setEncryption(new CVmHddEncryption(e));
		list->m_lstBackupDisks << b;
	}
	entry.setBackupDisks(list.take());
	return PRL_ERR_SUCCESS;
}

void Task_GetBackupTreeTarget::getBackupTree(QString &msg)
{
	QDir dir;
	QFileInfoList entryList;
	int i, j, k;
	BackupTree bTree;
	QString sVmUuid;
	QString sBackupUuid;
	unsigned nBackupNumber;
	QStringList lstBaseBackupUuid;
	QList<unsigned> lstPartialBackupNumber;
	VmItem cVmItem;
	bool chain = false;

	/* set empty xml string at the first (https://jira.sw.ru/browse/PSBM-9137) */
	msg = bTree.toString();
	dir.setPath(getBackupDirectory());
	if (!dir.exists())
		return;

	entryList = dir.entryInfoList(QDir::NoDotAndDotDot | QDir::Dirs);
	for (i = 0; i < entryList.size(); ++i) {
		sVmUuid = entryList.at(i).fileName();
		if (!Uuid::isUuid(sVmUuid))
			continue;
		if (!backupFilterEnabled() && !m_sUuid.isEmpty() && (m_sUuid != sVmUuid))
			continue;

		loadVmMetadata(sVmUuid, &cVmItem);

		/* skip inappropriate vmtype */
		if (!(m_nFlags & PBT_VM) && (cVmItem.getVmType() == PVBT_VM))
			continue;
		if (!(m_nFlags & PBT_CT) && (cVmItem.getVmType() != PVBT_VM))
			continue;

		QScopedPointer<VmItem> pVmItem(new (std::nothrow) VmItem(cVmItem));
		if (!pVmItem)
			return;
		getBaseBackupList(sVmUuid, lstBaseBackupUuid, &getClient()->getAuthHelper(), PRL_BACKUP_CHECK_MODE_READ);
		for (j = 0; j < lstBaseBackupUuid.size(); ++j) {
			sBackupUuid = lstBaseBackupUuid.at(j);
			QScopedPointer<BackupItem> pBackupItem(new (std::nothrow) BackupItem);
			if (!pBackupItem)
				return;
			if (PRL_FAILED(loadBaseBackupMetadata(sVmUuid, sBackupUuid, pBackupItem.data())))
				continue;
			if (PRL_FAILED(addDisks(*pBackupItem, cVmItem, sBackupUuid, PRL_BASE_BACKUP_NUMBER)))
				continue;
			if (backupFilterEnabled() && (m_sUuid == pBackupItem->getId())) {
				if (filterSingleBackup()) {
					/* found needed full backup - nothing to do anymore */
					addBackup(pVmItem->m_lstBackupItem, pBackupItem.take());
					break;
				} else if (filterBackupChain()) {
					/* enable chain search mode - add all incremental
					 * backups of this full backup to the output */
					chain = true;
				}
			}
			getPartialBackupList(sVmUuid, sBackupUuid, lstPartialBackupNumber);
			for (k = 0; k < lstPartialBackupNumber.size(); ++k) {
				nBackupNumber = lstPartialBackupNumber.at(k);
				QScopedPointer<PartialBackupItem> pPartialBackupItem(new (std::nothrow) PartialBackupItem);
				if (!pPartialBackupItem)
					return;
				if (PRL_FAILED(loadPartialBackupMetadata(sVmUuid, sBackupUuid, nBackupNumber, pPartialBackupItem.data())))
					continue;
				if (PRL_FAILED(addDisks(*pPartialBackupItem, cVmItem, sBackupUuid, nBackupNumber)))
					continue;
				if (backupFilterEnabled()) {
					if (m_sUuid == pPartialBackupItem->getId()) {
						if (filterSingleBackup()) {
							/* found needed incremental backup - nothing to do anymore */
							addBackup(pBackupItem->m_lstPartialBackupItem, pPartialBackupItem.take());
							break;
						} else if (filterBackupChain()) {
							/* add all incremental backups after this backup to the output */
							chain = true;
						}
					} else if (!chain) {
						continue;
					}
				}
				addBackup(pBackupItem->m_lstPartialBackupItem, pPartialBackupItem.take());
			}
			if (backupFilterEnabled() && pBackupItem->m_lstPartialBackupItem.isEmpty()
				&& (m_sUuid != pBackupItem->getId())) {
				/* no partial backups are found for current full backup */
				continue;
			}
			addBackup(pVmItem->m_lstBackupItem, pBackupItem.take());
		}
		if (pVmItem->m_lstBackupItem.isEmpty()) {
			/* to skip Vm directories without backups */
			continue;
		}
		bTree.m_lstVmItem.append(pVmItem.take());
		/* if we searched for a backup or a chain - it is already found here */
		if (backupFilterEnabled())
			break;
	}
	msg = bTree.toString();
}

template <class T>
void Task_GetBackupTreeTarget::addBackup(QList<T*>& list, T *backup)
{
	int i;
	/* order list by backup data&time creation */
	for (i = 0; i < list.size(); ++i)
		if (list.at(i)->getDateTime() > backup->getDateTime())
			break;
	list.insert(i, backup);
}
