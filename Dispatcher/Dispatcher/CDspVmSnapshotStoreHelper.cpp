////////////////////////////////////////////////////////////////////////////////(
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
/// @file
///  CDspVmSnapshotStoreHelper.cpp
///
/// @brief
///  Implementation of the class CDspVmSnapshotStoreHelper
///
/// @brief
///  This class implements snapshot tree managing logic
///
/// @author sergeyt
///  ilya@
///
////////////////////////////////////////////////////////////////////////////////

#include <prlcommon/ProtoSerializer/CProtoSerializer.h>


#include <prlcommon/Interfaces/ParallelsQt.h>
#include "CDspVmSnapshotStoreHelper.h"

#include "CDspService.h"
#include "CDspClientManager.h"
#include "Libraries/PrlCommonUtils/CFileHelper.h"
#include "CDspVmInfoDatabase.h"
#include <boost/foreach.hpp>

#include <prlcommon/Std/PrlAssert.h>

#include <prlcommon/Logging/Logging.h>
#include "Libraries/StatesUtils/StatesHelper.h"
#include "Libraries/StatesStore/SavedStateStore.h"
//#include "Libraries/VirtualDisk/DiskStatesManager.h"  // VirtualDisk commented out by request from CP team

#include <prlcommon/HostUtils/HostUtils.h>
#ifdef _WIN_
#	include <io.h> /* for _commit() call */
#endif

// By adding this interface we enable allocations tracing in the module
#include "Interfaces/Debug.h"

#include <prlsdk/PrlEnums.h>

using namespace Parallels;

namespace
{
///////////////////////////////////////////////////////////////////////////////
// struct View

struct View
{
	typedef QList<Libvirt::Instrument::Agent::Vm::Snapshot::Unit> model_type;

	explicit View(SmartPtr<CVmConfiguration> config_): m_config(config_)
	{
	}

	bool operator()();
	void setModel(const model_type& value_);
	const CSavedStateStore& getResult() const
	{
		return m_result;
	}

private:
	typedef std::map<QString, model_type> tree_type;

	SmartPtr<CVmConfiguration> m_config;
	tree_type m_input;
	CSavedStateStore m_result;
};

bool View::operator()()
{
	m_result.ClearSavedStateTree();

	if (m_input.empty())
		return true;

	CSavedState f;
	f.SetGuid(Uuid::createUuid().toString());
	m_result.CreateSnapshot(f);
	m_result.FindCurrentSnapshot()->SetCurrent(false);
	QStack<CSavedStateTree* > s;
	for (s.push(m_result.GetSavedStateTree()); !s.isEmpty();)
	{
		tree_type::mapped_type& c = m_input[s.top()->GetGuid()];
		if (c.isEmpty())
		{
			CSavedStateTree* x = s.pop();
			if (!s.isEmpty())
				s.top()->AddChild(x);
		}
		else
		{
			model_type::value_type u = c.takeFirst();
			CSavedStateTree* x = new CSavedStateTree();
			u.getState(*x);
			s.push(x);

			Prl::Expected<CSavedStateTree, PRL_RESULT> snapshot =
				Libvirt::Snapshot::Stash(m_config, x->GetGuid()).getMetadata();

			if (snapshot.isSucceed())
				x->SetName(snapshot.value().GetName());
		}
	}
	m_result.DeleteNode(f.GetGuid());
	return true;
}

void View::setModel(const model_type& value_)
{
	m_input.clear();
	BOOST_FOREACH(model_type::const_reference u, value_)
	{
		QString i;
		u.getParent().getUuid(i);
		m_input[i] << u;
	}
}

} // namespace

namespace Libvirt
{
namespace Snapshot
{
///////////////////////////////////////////////////////////////////////////////
// struct Stash

Stash::Stash(const SmartPtr<CVmConfiguration>& cfg_, const QString& snapshot_)
	: m_vmUuid(cfg_->getVmIdentification()->getVmUuid()),
		m_dir((QStringList()
			<< QFileInfo(cfg_->getVmIdentification()->getHomePath()).absolutePath()
			<< "snapshots"
			<< snapshot_).join(QDir::separator()))
{
}

Stash::~Stash()
{
	CFileHelper::DeleteFilesFromList(m_files);
}

bool Stash::add(const QStringList& files_)
{
	if (!m_dir.exists() && !m_dir.mkpath("."))
	{
		WRITE_TRACE(DBG_FATAL, "Unable to create directory %s", QSTR2UTF8(m_dir.absolutePath()));
		return false;
	}
	foreach (const QString& f, files_)
	{
		QString path = m_dir.absoluteFilePath(QFileInfo(f).fileName());
		if (!QFile::copy(f, path))
			return false;
		m_files.append(path);
	}
	m_files.push_back(m_dir.absolutePath());
	return true;
}

bool Stash::restore(const QStringList& files_)
{
	if (!m_dir.exists())
	{
		WRITE_TRACE(DBG_FATAL, "Directory %s is absent", QSTR2UTF8(m_dir.absolutePath()));
		return false;
	}
	foreach (const QString& f, files_)
	{
		QString name = QFileInfo(f).fileName();
		if (!m_dir.exists(name))
			continue;
		if (!QFile::copy(m_dir.absoluteFilePath(name), f))
			return false;
		m_files.push_back(f);
	}
	return true;
}

bool Stash::setMetadata(const CSavedState& state_)
{
	if (!m_dir.exists())
	{
		WRITE_TRACE(DBG_FATAL, "Directory %s is absent", QSTR2UTF8(m_dir.absolutePath()));
		return false;
	}

	CSavedStateStore s;
	s.CreateSnapshot(state_);
	QString file = m_dir.filePath(metadataFile);
	s.Save(file);
	m_files.push_front(file);
	return true;
}

const char Stash::metadataFile[] = "snapshot.xml";

Prl::Expected<CSavedStateTree, PRL_RESULT> Stash::getMetadata() const
{
	if (!m_dir.exists())
	{
		WRITE_TRACE(DBG_FATAL, "Directory %s is absent", QSTR2UTF8(m_dir.absolutePath()));
		return PRL_ERR_FAILURE;
	}

	CSavedStateStore s(m_dir.filePath(metadataFile));
	CSavedStateTree* t = s.FindCurrentSnapshot();
	return NULL == t ? CSavedStateTree() : *t;
}

SmartPtr<CVmConfiguration> Stash::restoreConfig(const QString& file_)
{
	QString name = QFileInfo(file_).fileName();
	QString path = m_dir.absoluteFilePath(name);
	if (!m_dir.exists(name)) {
		WRITE_TRACE(DBG_FATAL, "Config file is absent: %s!", QSTR2UTF8(path));
		return SmartPtr<CVmConfiguration>();
	}
	SmartPtr<CVmConfiguration> pVmConfig(new CVmConfiguration());

	QFile f(path);
	if (PRL_FAILED(pVmConfig->loadFromFile(&f, false))) {
		WRITE_TRACE(DBG_FATAL, "Unable to load config from %s", QSTR2UTF8(path));
		return SmartPtr<CVmConfiguration>();
	}
	m_files.push_back(file_);
	return pVmConfig;
}

void Stash::commit()
{
	m_files.clear();
}

} // namespace Snapshot
} // namespace Libvirt

// constructor
CDspVmSnapshotStoreHelper::CDspVmSnapshotStoreHelper( )
{

}

// destructor
CDspVmSnapshotStoreHelper::~CDspVmSnapshotStoreHelper()
{
	/* Mutex is not mandatory here because destructor is the very last method
	   of the object. But it may helps up to catch use-after-free bugs.
	 */
	//////////////////////////////////////////////////////////////////////////
	// Work under internal lock
	//////////////////////////////////////////////////////////////////////////
	QMutexLocker locker( &m_mutex );
	CVmIdentSnapHashType::iterator it;

	for (it = m_lockedSnap.begin(); it != m_lockedSnap.end(); ) {
		if (!(*it)->empty()) {
			/* This should't happen. Looks like resource leakage */
			SnapHashType::const_iterator snIt;
			WRITE_TRACE(DBG_FATAL, "Lock hash stil contain objects :"
				"vmIdent:{vmUuid:%s, vmDir:%s} \n",
				QSTR2UTF8(it.key().first), QSTR2UTF8(it.key().second));
			WRITE_TRACE(DBG_FATAL, "Locked snapshot list:");

			for (snIt = (*it)->begin(); snIt != (*it)->end(); ++snIt) {
				WRITE_TRACE(DBG_FATAL,"{ snapUuid:%s  rcode:%x (%s) }",
					QSTR2UTF8(snIt.key()), snIt.value(),
					PRL_RESULT_TO_STRING(snIt.value()));
			}
		}
		it = m_lockedSnap.erase(it);
		/* now iterator points to next item in the hash */
	}
}

/////////////////////////////////////
//
//  dispatcher internal requests
//
/////////////////////////////////////


/**
* @brief Create new snapshot record in snapshots tree.
* @param pRequestParams
* @return
*/
void CDspVmSnapshotStoreHelper::createSnapshot(SmartPtr<CDspClient> user, const SmartPtr<IOPackage>& pkg,
		VIRTUAL_MACHINE_STATE vmState, PRL_UINT32 nFlags)
{
	// Unpack command
	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(PVE::DspCmdVmCreateSnapshot, UTF8_2QSTR(pkg->buffers[0].getImpl()));
	CProtoCreateSnapshotCommand *pCreateSnapshotCmd = CProtoSerializer::CastToProtoCommand<CProtoCreateSnapshotCommand>(pCmd);
	// Get command parameters
	QString sVmUuid = pCmd->GetVmUuid();
	QString sSnapshotUuid = pCreateSnapshotCmd->GetSnapshotUuid();
	QString sSnapshotCreator = pCreateSnapshotCmd->GetSnapshotCreator();

	QString sVmConfig = CDspVmDirManager::getVmHomeByUuid( user->getVmIdent( sVmUuid ) );
	if( sVmConfig.isEmpty() )
	{
		WRITE_TRACE( DBG_WARNING, "Unable to found vm with id %s", QSTR2UTF8(sVmUuid) );
		return;
	}

	// Generate name Snapshots.xml, based on full path to .pvs file
	QFileInfo fileInfo(sVmConfig);
	QString sVmFolder = fileInfo.dir().absolutePath();
	QString sSnapshotsTreePath = sVmFolder + "/" + VM_GENERATED_SNAPSHOTS_CONFIG_FILE;

	// Get current VM state
	PVE::SnapshotedVmState state = PVE::SnapshotedVmPoweredOff;

	switch(vmState)
	{
	case VMS_RUNNING:
		state = PVE::SnapshotedVmRunning;
		break;
	case VMS_PAUSED:
		state = PVE::SnapshotedVmPaused;
		break;
	case VMS_SUSPENDED:
		state = PVE::SnapshotedVmSuspended;
		break;
	default:
		WRITE_TRACE(DBG_FATAL, "createSnapshot state: %d", vmState );
		break;
	}

	//////////////////////////////////////////////////////////////////////////
	// Work under internal lock
	//////////////////////////////////////////////////////////////////////////
	QMutexLocker locker( &m_mutex );

	// Save current configuration file into .pvc file
	QString sNewVmConfig = CStatesHelper::MakeCfgFileName(sVmFolder, sSnapshotUuid);
	CStatesHelper::SaveVmConfig(sVmConfig, sNewVmConfig);
	CDspVmInfoDatabase::saveVmInfoToSnapshot(sVmFolder, sSnapshotUuid);

	/* Store only config.pvs in BACKUP mode */
	if (nFlags & SNAP_BACKUP_MODE)
		return;

	// Load snapshots tree
	CSavedStateStore cSavedStateStore("");
	cSavedStateStore.Load(sSnapshotsTreePath);

	// Add new snapshot into XML configuration
	QDateTime dtCurrent = QDateTime::currentDateTime();
	CSavedState cState;
	cState.SetCreateTime(dtCurrent.toString(XML_DATETIME_FORMAT));
	cState.SetDescription( pCreateSnapshotCmd->GetDescription() );
	cState.SetGuid( sSnapshotUuid );
	cState.SetCreator( sSnapshotCreator );
	cState.SetName( pCreateSnapshotCmd->GetName() );
	cState.SetVmState(state);

	if (vmState == VMS_RUNNING || vmState == VMS_PAUSED || vmState == VMS_SUSPENDED)
		cState.SetScreenShot( QString(sSnapshotUuid + ".png") );

	// Save snapshots tree
	if (cSavedStateStore.CreateSnapshot(cState) == SnapshotParser::RcSuccess)
	{
		cSavedStateStore.Save();
	}


	if (nFlags & SNAP_NOTIFY)
	{
		// Notify all users: snapshot tree changed
		notifyVmClientsTreeChanged(pkg, user->getVmDirectoryUuid(), sVmUuid);
	}
}

/**
* @brief Switch to snapshot.
* @param pRequestParams
* @return
*/
void CDspVmSnapshotStoreHelper::switchToSnapshot(SmartPtr<CDspClient> user, const SmartPtr<IOPackage>& pkg)
{
	// Unpack command
	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(PVE::DspCmdVmSwitchToSnapshot, UTF8_2QSTR(pkg->buffers[0].getImpl()));
	CProtoSwitchToSnapshotCommand *pCmdSwitch = CProtoSerializer::CastToProtoCommand<CProtoSwitchToSnapshotCommand>(pCmd);
	// Get VM uuid
	QString sVmUuid = pCmd->GetVmUuid();
	QString sSnapshotUuid = pCmdSwitch->GetSnapshotUuid();

	QString sSnapshotsTreePath = getPathToSnapshotsXml( user->getVmIdent( sVmUuid ) );

	//////////////////////////////////////////////////////////////////////////
	// Work under internal lock
	//////////////////////////////////////////////////////////////////////////
	QMutexLocker locker( &m_mutex );

	// Load snapshots tree
	CSavedStateStore cSavedStateStore("");
	cSavedStateStore.Load(sSnapshotsTreePath);

	// Switch to snapshot
	CSavedStateTree *pTree = cSavedStateStore.GetSavedStateTree();
	if (pTree)
	{
		CSavedStateTree *lpcNode = pTree->FindByUuid(sSnapshotUuid);
		if (lpcNode)
			lpcNode->SetCurrent(true);
	}

	cSavedStateStore.Save();
}

/**
* @brief Delete snapshot.
* @param pRequestParams
* @return
*/
void CDspVmSnapshotStoreHelper::deleteSnapshot(SmartPtr<CVmConfiguration> pVmConfig, const QString& sSnapshotUuid, bool bChild)
{
	//////////////////////////////////////////////////////////////////////////
	// Work under internal lock
	//////////////////////////////////////////////////////////////////////////
	QMutexLocker locker( &m_mutex );

	// Generate name Snapshots.xml
	QString sVmFolder = pVmConfig->getConfigDirectory();
	QString sSnapshotsTreePath = sVmFolder + "/" + VM_GENERATED_SNAPSHOTS_CONFIG_FILE;

	// Load snapshots tree
	CSavedStateStore cSavedStateStore("");
	if (cSavedStateStore.Load(sSnapshotsTreePath) != SnapshotParser::RcSuccess)
		return;
	if ( bChild )
		cSavedStateStore.DeleteBranch(sSnapshotUuid);
	else
		cSavedStateStore.DeleteNode(sSnapshotUuid);

	cSavedStateStore.Save();
}

/* atomically lock given snapshots list */
PRL_RESULT CDspVmSnapshotStoreHelper::lockSnapshotList(const CVmIdent &vmIdent,
						const QStringList& snapList, PRL_RESULT err_code)
{
	PRL_RESULT ret = PRL_ERR_CANT_PARSE_VM_CONFIG;
	SnapHashType::const_iterator it;
	CVmIdentSnapHashType::iterator vmIdentIt;
	QStringList::const_iterator lstIt;
	QString sVmHome;
	QString SnapFile;

	{
		// LOCK inside brackets
		CDspLockedPointer<CVmDirectoryItem>
			p = CDspService::instance()->getVmDirManager().getVmDirItemByUuid(
				vmIdent.second, vmIdent.first);
		if (!p)
			return PRL_ERR_VM_UUID_NOT_FOUND;
		sVmHome = p->getVmHome();
	}

	QFileInfo fileInfo(sVmHome);
	QString sSnapshotsTreePath = fileInfo.dir().absolutePath();
	SnapFile = QString("%1/%2").arg(sSnapshotsTreePath).arg(VM_GENERATED_SNAPSHOTS_CONFIG_FILE);

	//////////////////////////////////////////////////////////////////////////
	// Work under internal lock
	//////////////////////////////////////////////////////////////////////////
	QMutexLocker locker(&m_mutex);
	CSavedStateStore cSavedStateStore("");
	cSavedStateStore.Load(SnapFile);
	CSavedStateTree *pTree = cSavedStateStore.GetSavedStateTree();
	if ( ! pTree && ! snapList.contains(UNDO_DISKS_UUID))
	{
		WRITE_TRACE(DBG_FATAL, "Can't parse snapshot file: %s", QSTR2UTF8(sSnapshotsTreePath));
		return PRL_ERR_VM_SNAPSHOTS_CONFIG_NOT_FOUND;
	}

	vmIdentIt = m_lockedSnap.find(vmIdent);
	if (vmIdentIt == m_lockedSnap.end())
		vmIdentIt = m_lockedSnap.insert(vmIdent, SmartPtr<SnapHashType>( new SnapHashType));


	PRL_ASSERT(vmIdentIt->getImpl() != NULL);
	for (lstIt = snapList.constBegin(); lstIt != snapList.constEnd(); ++lstIt) {
		/* We have to check what each snapshot from snapList exist in config file.
		   It is necessery because some one may delete it after prepare snapList,
		   but before this->m_mutex was locked by the function.
		 */
		if (*lstIt != UNDO_DISKS_UUID && pTree && ! pTree->FindByUuid(*lstIt)) {
			WRITE_TRACE(DBG_WARNING, "Can't find snapshot:%s in file: %s",
				QSTR2UTF8(*lstIt), QSTR2UTF8(sSnapshotsTreePath));
			ret = PRL_ERR_VM_SNAPSHOT_NOT_FOUND;
			goto out;
		}
		it = (*vmIdentIt)->constFind(*lstIt);
		if (it != (*vmIdentIt)->constEnd()) {
			ret = it.value();
			WRITE_TRACE(DBG_WARNING, "Snapshot:%s Vm:%s VmDir:%s is already locked, code: %s (%#x)",
				QSTR2UTF8(*lstIt), QSTR2UTF8(vmIdent.first),
				QSTR2UTF8(vmIdent.second),
				PRL_RESULT_TO_STRING(ret), ret);
			goto out;
		}
	}
	for (lstIt = snapList.constBegin(); lstIt != snapList.constEnd(); ++lstIt)
		(*vmIdentIt)->insert(*lstIt, err_code);

	ret = PRL_ERR_SUCCESS;
out:
	if ((*vmIdentIt)->empty())
		m_lockedSnap.erase(vmIdentIt);
	return ret;
}

/* atomically unlock given snapshots list */
void CDspVmSnapshotStoreHelper::unlockSnapshotList(const CVmIdent &vmIdent, const QStringList& snapList)
{
	CVmIdentSnapHashType::iterator vmIdentIt;
	SnapHashType::const_iterator it;
	QStringList::const_iterator lstIt;

	//////////////////////////////////////////////////////////////////////////
	// Work under internal lock
	//////////////////////////////////////////////////////////////////////////
	QMutexLocker locker(&m_mutex);

	vmIdentIt = m_lockedSnap.find(vmIdent);
	if (vmIdentIt == m_lockedSnap.end()) {
		/* We are about to unlock entry what wasn't locked => fatal code bug*/
		PRL_ASSERT(0);
		return;
	}

	for (lstIt = snapList.constBegin(); lstIt != snapList.constEnd(); ++lstIt) {
		it = (*vmIdentIt)->constFind(*lstIt);
		if (it == (*vmIdentIt)->constEnd()) {
			/* We are about to unlock entry what wasn't locked => fatal code bug*/
			PRL_ASSERT(0);
			return;
		}
	}

	for (lstIt = snapList.constBegin(); lstIt != snapList.constEnd(); ++lstIt)
		(*vmIdentIt)->remove(*lstIt);

	if ((*vmIdentIt)->empty())
		m_lockedSnap.erase(vmIdentIt);
}

/**
* @brief Returns snapshot tree.
* @param pRequestParams
* @return
*/
PRL_RESULT CDspVmSnapshotStoreHelper::sendSnapshotsTree(SmartPtr<CDspClient> pUser, const SmartPtr<IOPackage>& pkg)
{
	CAuthHelperImpersonateWrapper authWrap( &pUser->getAuthHelper() );

	CProtoCommandPtr cmd = CProtoSerializer::ParseCommand(pkg);
	if (!cmd->IsValid())
	{
		// Send error
		pUser->sendSimpleResponse(pkg, PRL_ERR_FAILURE);
		return PRL_ERR_FAILURE;
	}

	PRL_RESULT ret;
	SmartPtr<CVmConfiguration> config = CDspService::instance()->getVmDirHelper()
		.getVmConfigByUuid(pUser->getVmIdent(cmd->GetVmUuid()), ret);

	if (!config)
		return ret;

	View::model_type x;
	Libvirt::Result e = Libvirt::Kit.vms().at(cmd->GetVmUuid()).getSnapshot().all(x);
	if (e.isFailed())
	{
		WRITE_TRACE(DBG_FATAL, "Unable to load snapshot tree for vm %s",
			QSTR2UTF8(cmd->GetVmUuid()));
		pUser->sendResponseError(e.error().convertToEvent(), pkg);
		return e.error().code();
	}

	View v(config);

	v.setModel(x);
	v();
	QBuffer buffer;
	if (!buffer.open( QIODevice::WriteOnly))
		WRITE_TRACE(DBG_FATAL, "Can't open internal buffer");
	else
		v.getResult().Save(buffer, true);

	// Prepare response
	CProtoCommandPtr pCmd = CProtoSerializer::CreateDspWsResponseCommand( pkg, PRL_ERR_SUCCESS );
	CProtoCommandDspWsResponse *pResponseCmd = CProtoSerializer::CastToProtoCommand<CProtoCommandDspWsResponse>( pCmd );
	pResponseCmd->SetSnapshotsTree(UTF8_2QSTR(buffer.data()));
	SmartPtr<IOPackage> responsePkg = DispatcherPackage::createInstance( PVE::DspWsResponse, pCmd, pkg );
	pUser->sendPackage(responsePkg);

	return PRL_ERR_SUCCESS;
}

namespace {
struct FillRuntime
{
	FillRuntime( const CVmIdent& id, CSavedStateTree& tree);
	void _do();
private:
// VirtualDisk commented out by request from CP team
//	typedef QHash< QString/*snapId*/, PROCESS_TYPE/* unfinished ops type */ > UnfinishedOpsMap;
	typedef QMap<QString/*snapId*/, PRL_UINT64/* snapshot size */ > SnapshotsSizeMap;

	// helpers:
	void iterate( CSavedStateTree* pState);
	QString getVmSnapshotsPath(const CVmIdent& vmIdent);
	void fillDisksInfo();
	// VirtualDisk commented out by request from CP team
	//void fillSnapshotsSize(const SNAPTREE_ELEMENT& se);
private:
	static void addOsVersion( const QString& snapshotDirPath, CSavedStateTree* pState );
// VirtualDisk commented out by request from CP team
//	static void addUnfinishedState( const UnfinishedOpsMap& ops, CSavedStateTree* pState );
	static void addSnapshotSize(const QString& snapshotDirPath,
								const SnapshotsSizeMap& snapshotsSize,
								CSavedStateTree* pState);
private:
	CVmIdent m_vmId;
	CSavedStateTree* m_pTree;

	QString m_sSnapshotsDir;
// VirtualDisk commented out by request from CP team
//	UnfinishedOpsMap m_unfinishedOps;
	SnapshotsSizeMap m_snapshotsSize;
};

FillRuntime::FillRuntime( const CVmIdent& id, CSavedStateTree& tree )
: m_vmId( id )
, m_pTree(&tree)
{
	m_sSnapshotsDir = getVmSnapshotsPath(m_vmId);
	fillDisksInfo();
}

void FillRuntime::_do()
{
	iterate( m_pTree );
}

void FillRuntime::iterate( CSavedStateTree* pState)
{
	for( ; pState; pState = pState->GetNextSibling() )
	{
		if ( ! pState->GetGuid().isEmpty() )
		{
			addOsVersion( m_sSnapshotsDir, pState );
// VirtualDisk commented out by request from CP team
//			addUnfinishedState( m_unfinishedOps, pState );
			addSnapshotSize( m_sSnapshotsDir, m_snapshotsSize, pState );
		}

		foreach(CSavedStateTree* pChildSavedState, *pState->GetChilds())
			iterate( pChildSavedState );
	}
}

QString FillRuntime::getVmSnapshotsPath(const CVmIdent& vmIdent)
{
	QString qsPath = CDspService::instance()->getVmDirManager().getVmHomeByUuid(vmIdent);
	qsPath = QFileInfo(qsPath).path();
	qsPath = QFileInfo(QDir(qsPath), VM_GENERATED_WINDOWS_SNAPSHOTS_DIR).filePath();
	return qsPath;
}

void FillRuntime::fillDisksInfo()
{
	PRL_RESULT nRetCode;

	SmartPtr<CVmConfiguration> pVmConfig =  CDspService::instance()->getVmDirHelper()
		.getVmConfigByUuid( m_vmId, nRetCode );
	if( !pVmConfig )
	{
		WRITE_TRACE( DBG_FATAL, "Unable to load config for %s by error %s"
			, QSTR2UTF8( m_vmId.first )
			, PRL_RESULT_TO_STRING(nRetCode));
		return;
	}

// VirtualDisk commented out by request from CP team
//	// TODO: Need to reuse Shapshot::getUnfinishedOps() with the same logic
//	foreach ( CVmHardDisk *pHdd,  pVmConfig->getVmHardwareList()->m_lstHardDisks)
//	{
//		IDisk* pDisk = IDisk::OpenDisk( pHdd->getSystemName(),
//			PRL_DISK_NO_ERROR_CHECKING | PRL_DISK_READ | PRL_DISK_FAKE_OPEN, &nRetCode);
//
//		if (PRL_FAILED(nRetCode))
//		{
//			WRITE_TRACE(DBG_DEBUG, "IDisk::OpenDisk(%s) error : %#x( '%s' )",
//				QSTR2UTF8(pHdd->getSystemName()), nRetCode, PRL_RESULT_TO_STRING(nRetCode));
//			continue;
//		}
//		if (!pDisk)
//		{
//			WRITE_TRACE(DBG_DEBUG, "IDisk::OpenDisk(%s) return invalid pointer",
//					QSTR2UTF8(pHdd->getSystemName()));
//			continue;
//		}
//
//		LOG_MESSAGE( DBG_FATAL, "pDisk->IsUncommited()=%d, getSystemName()='%s'"
//			, pDisk->IsUncommited(), QSTR2UTF8(pHdd->getSystemName()) );
//
//		if( pDisk->IsUncommited() )
//		{
//			m_unfinishedOps[ pDisk->GetUnfinishedOpUid().toString() ] = pDisk->GetUnfinishedOpType();
//
//			LOG_MESSAGE( DBG_FATAL, "pDisk->GetUnfinishedOpUid='%s', GetUnfinishedOpType()=%d"
//				, QSTR2UTF8( pDisk->GetUnfinishedOpUid().toString() )
//				, pDisk->GetUnfinishedOpType() );
//		}
//
//		SNAPTREE_ELEMENT se;
//		nRetCode = pDisk->GetSnapshotsTree(&se);
//		if (PRL_SUCCEEDED(nRetCode))
//			fillSnapshotsSize(se);
//		else
//			WRITE_TRACE(DBG_DEBUG, "IDisk::GetSnapshotsTree(%s) error : %#x( '%s' )",
//				QSTR2UTF8(pHdd->getSystemName()), nRetCode, PRL_RESULT_TO_STRING(nRetCode));
//
//		pDisk->Release();
//		pDisk = NULL;
//	}
}

// VirtualDisk commented out by request from CP team
//void FillRuntime::fillSnapshotsSize(const SNAPTREE_ELEMENT& se)
//{
//	QString qsId = se.m_Uid.toString();
//	if (m_snapshotsSize.contains(qsId))
//		m_snapshotsSize[qsId] += se.m_Size;
//	else
//		m_snapshotsSize.insert(qsId, se.m_Size);
//
//	std::list<struct __SNAPTREE_ELEMENT>::const_iterator it;
//	for(it = se.m_Children.begin(); it != se.m_Children.end(); ++it)
//	{
//		fillSnapshotsSize(*it);
//	}
//}

void FillRuntime::addOsVersion( const QString& sSnapshotsDir, CSavedStateTree* pState )
{
		QString path = QDir(sSnapshotsDir).filePath( pState->GetGuid() + ".pvc");
		QFile cf( path );

		CVmConfiguration cfg;
		int nRes = cfg.loadFromFile(&cf);
		if (PRL_SUCCEEDED(nRes))
		{
			CSavedState::Runtime rt = pState->GetRuntime();

			rt.nOsVersion = cfg.getVmSettings()->getVmCommonOptions()->getOsVersion();

			pState->SetRuntime(rt);
		}
		else
		{
			WRITE_TRACE(DBG_DEBUG, "Cannot load VM snapshot config '%s' !",
							QSTR2UTF8( path ) );
		}
}

// VirtualDisk commented out by request from CP team
//void FillRuntime::addUnfinishedState( const UnfinishedOpsMap& map, CSavedStateTree* pState )
//{
//	if( ! map.contains( pState->GetGuid() ) )
//		return;
//
//		CSavedState::Runtime rt = pState->GetRuntime();
//		rt.nUnfinishedOpType = map[pState->GetGuid()];
//		pState->SetRuntime(rt);
//}

void FillRuntime::addSnapshotSize(const QString& snapshotDirPath,
								  const SnapshotsSizeMap& snapshotsSize,
								  CSavedStateTree* pState)
{
	PRL_UINT64 nSize = 0;
	if ( snapshotsSize.contains(pState->GetGuid()) )
		nSize += snapshotsSize.value(pState->GetGuid());

	QDir sd(snapshotDirPath);

	QFileInfoList fil = sd.entryInfoList(QStringList() << (pState->GetGuid() + ".*"), QDir::Files);
	foreach(QFileInfo fi, fil)
	{
		QFile f(fi.filePath());
		nSize += (PRL_UINT64 )f.size();
	}

	CSavedState::Runtime rt = pState->GetRuntime();
	rt.nSize = nSize;
	pState->SetRuntime(rt);
}

} // namespace

void CDspVmSnapshotStoreHelper::fillSnapshotRuntimeFields(const CVmIdent& id,
														   CSavedStateTree* pSavedState)
{
	FillRuntime rt(id, *pSavedState);
	rt._do();
}

/**
* @brief Returns state of snapshoted VM
* @param pRequestParams
* @return
*/
PVE::SnapshotedVmState CDspVmSnapshotStoreHelper::getSnapshotedVmState(SmartPtr<CDspClient> user, const SmartPtr<IOPackage>& pkg)
{
	// Unpack command
	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(PVE::DspCmdVmSwitchToSnapshot, UTF8_2QSTR(pkg->buffers[0].getImpl()));
	CProtoSwitchToSnapshotCommand *pCmdSwitch = CProtoSerializer::CastToProtoCommand<CProtoSwitchToSnapshotCommand>(pCmd);
	QString sVmUuid = pCmd->GetVmUuid();
	QString sSnapshotUuid = pCmdSwitch->GetSnapshotUuid();

	QString sSnapshotsTreePath = getPathToSnapshotsXml( user->getVmIdent( sVmUuid ) );

	//////////////////////////////////////////////////////////////////////////
	// Work under internal lock
	//////////////////////////////////////////////////////////////////////////
	QMutexLocker locker( &m_mutex );

	// Load snapshots tree
	CSavedStateStore cSavedStateStore("");
	cSavedStateStore.Load(sSnapshotsTreePath);

	// Switch to snapshot
	CSavedStateTree *pTree = cSavedStateStore.GetSavedStateTree();
	if (pTree)
	{
		CSavedStateTree *lpcNode = pTree->FindByUuid(sSnapshotUuid);
		if (lpcNode)
			return lpcNode->GetVmState();
	}

	return PVE::SnapshotedVmPoweredOff;
}

PRL_RESULT CDspVmSnapshotStoreHelper::SetDefaultAccessRights(const QString& strFullPath,
													   SmartPtr<CDspClient> pSession,
													   const CVmDirectoryItem* pVmDirItem,
													   bool bRecursive)
{

	{
		//https://bugzilla.sw.ru/show_bug.cgi?id=267152
		CAuthHelperImpersonateWrapper _impersonate( &pSession->getAuthHelper() );

		//https://bugzilla.sw.ru/show_bug.cgi?id=427025
		// https://jira.sw.ru/browse/PSBM-16849
		if( !CFileHelper::isFsSupportPermsAndOwner( strFullPath )
			|| ( pVmDirItem ? !CFileHelper::isFsSupportPermsAndOwner( pVmDirItem->getVmHome() ) : false ) )
		{
			WRITE_TRACE(DBG_FATAL, "File system does not support permissions. Setting default permissions will be ignored. (path=%s)"
				, QSTR2UTF8( strFullPath ) );
			return PRL_ERR_SUCCESS;
		}
	}

	return CDspService::instance()->getAccessManager().setOwnerAndAccessRightsToPathAccordingVmRights(strFullPath
		, pSession
		, pVmDirItem
		, bRecursive);
}

PRL_RESULT CDspVmSnapshotStoreHelper::setDefaultAccessRightsToAllHdd(
	const SmartPtr<CVmConfiguration> pVmConfig,
	SmartPtr<CDspClient> pSession,
	const CVmDirectoryItem* pVmDirItem,
	bool bBreakByError)
{
	PRL_ASSERT(pVmConfig);
	PRL_ASSERT(pSession);
	PRL_ASSERT(pVmDirItem);

	/**
	* Set access rights to hard disks folders
	*/
	CVmHardware *lpcHardware = pVmConfig->getVmHardwareList();
	for (int i = 0; i < lpcHardware->m_lstHardDisks.size() ; i++)
	{
		if( lpcHardware->m_lstHardDisks[i]->getEmulatedType() != PVE::HardDiskImage)
			continue;

		QString strDiskFolder = lpcHardware->m_lstHardDisks[i]->getSystemName();

		{
			//https://bugzilla.sw.ru/show_bug.cgi?id=267152
			CAuthHelperImpersonateWrapper _impersonate( &pSession->getAuthHelper() );
			if( ! QFileInfo( strDiskFolder  ).exists() )
			{
				WRITE_TRACE(DBG_FATAL, "%s: hdd does not exists. Skip it. Path= '%s'"
					, __FUNCTION__
					, QSTR2UTF8( strDiskFolder ) );
				continue;
			}
		}

		PRL_RESULT ret = SetDefaultAccessRights( strDiskFolder
				, pSession
				, pVmDirItem );

		if( PRL_FAILED(ret) )
		{
			WRITE_TRACE(DBG_FATAL, "Failed to set access rights to folder [%s] with error [%s]"
				, QSTR2UTF8(strDiskFolder), PRL_RESULT_TO_STRING(ret));

			if( bBreakByError )
				return ret;
		}
	}

	return PRL_ERR_SUCCESS;
}

// VirtualDisk commented out by request from CP team
//PRL_RESULT CDspVmSnapshotStoreHelper::PrepareDiskStateManager(
//	const SmartPtr<CVmConfiguration> pVmConfig,
//	CDSManager *pStatesManager)
//{
//	PRL_ASSERT(pVmConfig.getImpl());
//	PRL_ASSERT(pStatesManager);
//
//	PRL_RESULT ret = PRL_ERR_SUCCESS;
//
//	pStatesManager->Clear();
//
//	CVmHardware *lpcHardware = pVmConfig->getVmHardwareList();
//	for (int i = 0; i < lpcHardware->m_lstHardDisks.size() ; i++)
//	{
//		if ( ! lpcHardware->m_lstHardDisks[i]->getEnabled()
//			|| lpcHardware->m_lstHardDisks[i]->getEmulatedType() != PVE::HardDiskImage )
//			continue;
//
//		ret = pStatesManager->AddDisk( lpcHardware->m_lstHardDisks[i]->getSystemName() );
//		if (PRL_SUCCEEDED(ret))
//			continue;
//
//		WRITE_TRACE( DBG_FATAL, "AddDisk() failed by error %#x (%s) for disk \"%s\"",
//			ret, PRL_RESULT_TO_STRING(ret),
//			QSTR2UTF8(lpcHardware->m_lstHardDisks[i]->getSystemName()) );
//		break;
//	}
//
//	return ret;
//}

/**
* @brief Sets new snapshot name and description
* @param client session object
* @param request parameters
* @return
*/
void CDspVmSnapshotStoreHelper::updateSnapshotData(SmartPtr<CDspClient> user, const SmartPtr<IOPackage>& pkg)
{
	// Unpack command
	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(PVE::DspCmdVmUpdateSnapshotData,
		UTF8_2QSTR(pkg->buffers[0].getImpl()));

	CProtoUpdateSnapshotDataCommand *pCmdUpdate = \
		CProtoSerializer::CastToProtoCommand<CProtoUpdateSnapshotDataCommand>(pCmd);

	// Get VM uuid
	QString sVmUuid = pCmd->GetVmUuid();
	QString sSnapshotUuid = pCmdUpdate->GetSnapshotUuid();

	QString sSnapshotsTreePath = getPathToSnapshotsXml( user->getVmIdent( sVmUuid ) );
	if( sSnapshotsTreePath.isEmpty() )
	{
		user->sendSimpleResponse(pkg, PRL_ERR_VM_UPDATE_SNAPSHOT_DATA_FAILED);
		return;
	}

	//////////////////////////////////////////////////////////////////////////
	// Work under internal lock
	//////////////////////////////////////////////////////////////////////////
	QMutexLocker locker( &m_mutex );

	// Load snapshots tree
	CSavedStateStore cSavedStateStore("");
	cSavedStateStore.Load(sSnapshotsTreePath);

	// Find snapshot and update data
	PRL_RESULT res = PRL_ERR_SUCCESS;
	CSavedStateTree *pTree = cSavedStateStore.GetSavedStateTree();
	if (pTree)
	{
		CSavedStateTree *lpcNode = pTree->FindByUuid(sSnapshotUuid);
		if (lpcNode)
		{
			lpcNode->SetDescription( pCmdUpdate->GetDescription() );
			lpcNode->SetName( pCmdUpdate->GetName() );

			cSavedStateStore.Save();

			// Notify all users: snapshot tree changed
			notifyVmClientsTreeChanged(pkg, user->getVmDirectoryUuid(), sVmUuid);
		}
		else
			res = PRL_ERR_VM_UPDATE_SNAPSHOT_DATA_FAILED;
	}
	// Send response
	user->sendSimpleResponse(pkg, res);
}

/**
* @brief Notify VM clients: snapshot tree changed
* @param initial client request
* @param vm directory uuid
* @param vm uuid
* @return
*/
void CDspVmSnapshotStoreHelper::notifyVmClientsTreeChanged(const SmartPtr<IOPackage>& pkg,
														   const CVmIdent& id )
{
		notifyVmClientsTreeChanged( pkg, id.second, id.first );
}

void CDspVmSnapshotStoreHelper::notifyVmClientsTreeChanged(const SmartPtr<IOPackage>& pkg,
														   const QString &strVmDirUuid,
														   const QString &strVmUuid)
{
	CVmEvent event( PET_DSP_EVT_VM_SNAPSHOTS_TREE_CHANGED, strVmUuid, PIE_DISPATCHER );
	SmartPtr<IOPackage> p = DispatcherPackage::createInstance( PVE::DspVmEvent, event, pkg );
	CDspService::instance()->getClientManager().sendPackageToVmClients( p, strVmDirUuid, strVmUuid );
}

/**
* @brief Load snapshots tree under internal lock
* @param Vm ident
* @param pointer to snapshot tree
* @return
*/
bool CDspVmSnapshotStoreHelper::getSnapshotsTree(const CVmIdent &vmIdent, CSavedStateStore *pSavedStateStore)
{
	QString sVmHome = CDspVmDirManager::getVmHomeByUuid( vmIdent );
	if( sVmHome.isEmpty() )
	{
			WRITE_TRACE(DBG_FATAL, "Can't find snapshot.xml path for vmIdent:{vmUuid:%s, vmDir:%s}",
				QSTR2UTF8(vmIdent.first), QSTR2UTF8(vmIdent.second));
			return false;
	}

	return getSnapshotsTree( sVmHome, pSavedStateStore );
}

bool CDspVmSnapshotStoreHelper::getSnapshotsTree(const QString& sVmConfigPath, CSavedStateStore *pSavedStateStore)
{
	PRL_ASSERT(pSavedStateStore != NULL);

	if( sVmConfigPath.isEmpty() )
			return false;

	QString path = QString( "%1/%2" )
		.arg( QFileInfo(sVmConfigPath).absolutePath() )
		.arg( VM_GENERATED_SNAPSHOTS_CONFIG_FILE );

	QMutexLocker locker( &m_mutex );
	return (SnapshotParser::RcSuccess == pSavedStateStore->Load(path));
}


bool CDspVmSnapshotStoreHelper::doesSnapshotExist(const CVmIdent &vmIdent, const QString& snapshotId)
{
	CSavedStateStore ss;
	if (!CDspService::instance()->getVmSnapshotStoreHelper().getSnapshotsTree( vmIdent, &ss ))
		return false;

	CSavedStateTree *pTree = ss.GetSavedStateTree();
	if (!pTree)
		return false;

	return NULL != pTree->FindByUuid(snapshotId);
}

bool CDspVmSnapshotStoreHelper::hasSnapshotChildren( const CVmIdent& id, const QString& snapshotId )
{
	CSavedStateStore ssTree;
	CDspService::instance()->getVmSnapshotStoreHelper().getSnapshotsTree( id, &ssTree);

	CSavedStateTree *pTree = ssTree.GetSavedStateTree();
	if (!pTree)
		return false;;

	CSavedStateTree * pState = pTree->FindByUuid(snapshotId);
	if (!pState)
		return false;

	return pState->GetChildCount() > 0;
}


QString CDspVmSnapshotStoreHelper::getPathToSnapshotsXml( const CVmIdent& vmIdent )
{
	QString sVmHome = CDspVmDirManager::getVmHomeByUuid( vmIdent );
	if( sVmHome.isEmpty() )
		return "";
	return QString( "%1/%2" )
		.arg( QFileInfo(sVmHome).absolutePath() )
		.arg( VM_GENERATED_SNAPSHOTS_CONFIG_FILE );
}

void CDspVmSnapshotStoreHelper::removeStateFiles(QString strVmFolder, QString strSnapshotUuid)
{
	CStatesHelper::RemoveStateFile(strVmFolder, strSnapshotUuid, ".mem");
	CStatesHelper::RemoveStateFile(strVmFolder, strSnapshotUuid, ".sav");
	CStatesHelper::RemoveStateFile(strVmFolder, strSnapshotUuid, ".pvc");
	CStatesHelper::RemoveStateFile(strVmFolder, strSnapshotUuid, ".png");
	CStatesHelper::RemoveStateFile(strVmFolder, strSnapshotUuid, ".mem.trc");
	CStatesHelper::RemoveStateFile(strVmFolder, strSnapshotUuid, VM_INFO_FILE_SUFFIX);
	CStatesHelper::RemoveStateFile(strVmFolder, strSnapshotUuid, ".dat", true);
}

PRL_RESULT CDspVmSnapshotStoreHelper::removeHddFromVmConfig(SmartPtr<CDspClient> pUser,
		const QString &sVmHome, const QString &sSnapUuid, const QString &sHddSystemName)
{
	SmartPtr<CVmConfiguration> pSnapVmConfig(new CVmConfiguration());
	QString sVmConfigPath = CStatesHelper::MakeCfgFileName(sVmHome, sSnapUuid);
	PRL_RESULT res = CDspService::instance()->getVmConfigManager().loadConfig(pSnapVmConfig,
			sVmConfigPath, pUser, false, true);
	if (PRL_FAILED(res))
	{
		WRITE_TRACE(DBG_FATAL, "Can't parse VM snapshoy config file %s: %s"
				, QSTR2UTF8(sVmConfigPath)
				, PRL_RESULT_TO_STRING(res));
		return res;
	}

	QList<CVmHardDisk* > &lstHardDisks = pSnapVmConfig->getVmHardwareList()->m_lstHardDisks;
	for (int i = 0; i < lstHardDisks.size(); ++i)
	{
		CVmHardDisk *pSnapHdd = lstHardDisks.at(i);

		if (pSnapHdd->getEmulatedType() != PVE::HardDiskImage ||
				pSnapHdd->getSystemName() != sHddSystemName)
			continue;

		WRITE_TRACE(DBG_FATAL, "Delete HDD %s from %s",
				QSTR2UTF8(pSnapHdd->getSystemName()),
				QSTR2UTF8(sVmConfigPath));
		lstHardDisks.removeAt(i);

		res = CDspService::instance()->getVmConfigManager().saveConfig(pSnapVmConfig,
				sVmConfigPath, pUser, true, true);
		if (PRL_FAILED(res))
		{
			WRITE_TRACE(DBG_FATAL, "Can't save snapshot vm config %s",
					QSTR2UTF8(sVmConfigPath));
			return res;
		}
		break;
	}

	return PRL_ERR_SUCCESS;
}

int CDspVmSnapshotStoreHelper::removeHddFromSnapshotTree(SmartPtr<CDspClient> pUser,
		const QString sVmUuid,
		const CVmHardDisk *pHdd)
{
	QString sVmHome = CFileHelper::GetFileRoot(
				CDspVmDirManager::getVmHomeByUuid( pUser->getVmIdent( sVmUuid ) )
				);
	if (sVmHome.isEmpty())
		return PRL_ERR_SUCCESS;

	CSavedStateStore snapTree;
	if (!getSnapshotsTree(pUser->getVmIdent(sVmUuid), &snapTree))
		return PRL_ERR_SUCCESS;

	CSavedStateTree *pTree = snapTree.GetSavedStateTree();
	if (pTree == NULL)
	{
                WRITE_TRACE(DBG_FATAL, "Can't parse snapshot file");
                return PRL_ERR_VM_SNAPSHOTS_CONFIG_NOT_FOUND;
	}

	QList<CSavedStateTree *> snapList;
	snapList.append(pTree);

	for (int i = 0; i < snapList.size(); ++i)
	{
		for (int j = 0; j < snapList.at(i)->GetChildCount(); ++j)
			snapList.append(snapList.at(i)->GetChild(j) );
	}

	/* to compare by SytemName in same way */
	CVmHardDisk hdd(pHdd);
	hdd.setRelativeSystemName(sVmHome);

	foreach(CSavedStateTree* pState, snapList)
	{
		PRL_RESULT res;

		if (pState->GetGuid().isEmpty())
			continue;

		res = removeHddFromVmConfig(pUser, sVmHome, pState->GetGuid(),
					hdd.getSystemName());
		if (PRL_FAILED(res))
			return res;
	}

	return PRL_ERR_SUCCESS;
}
