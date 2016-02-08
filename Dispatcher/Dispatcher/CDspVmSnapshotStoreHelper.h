////////////////////////////////////////////////////////////////////////////////
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
///	CDspVmSnapshotStoreHelper.h
///
/// @brief
///	Definition of the class CDspVmSnapshotStoreHelper
///
/// @brief
///	This class implements snapshot three managing logic
///
/// @author sergeyt
///	ilya@
///
////////////////////////////////////////////////////////////////////////////////

#ifndef __CDspVmSnapshotStoreHelper_H_
#define __CDspVmSnapshotStoreHelper_H_

#include <QMutex>
#include <prlcommon/Interfaces/ParallelsDomModel.h>
#include <prlcommon/Interfaces/ParallelsNamespace.h>

#include "CDspClient.h"
#include <prlxmlmodel/VmConfig/CVmConfiguration.h>
#include <prlcommon/IOService/IOCommunication/IOServer.h>
#include <prlcommon/Std/SmartPtr.h>
#include <prlxmlmodel/VmDirectory/CVmDirectory.h>
#include "Libraries/StatesStore/SavedStateStore.h"

// VirtualDisk commented out by request from CP team
//class CDSManager;

/* Create Snapshot flags */
enum {
	SNAP_NOTIFY	= 0x1,
	SNAP_BACKUP_MODE= 0x2,
};

namespace Libvirt
{
namespace Snapshot
{
///////////////////////////////////////////////////////////////////////////////
// struct Stash

struct Stash {
	Stash(const SmartPtr<CVmConfiguration>& cfg_, const QString& snapshot_);
	~Stash();

	bool add(const QStringList& file_);
	bool restore(const QStringList& file_);
	SmartPtr<CVmConfiguration> restoreConfig(const QString& file_);
	bool hasFile(const QString file_)
	{
		return m_dir.exists(file_);
	}
	void commit();

private:
	const QString m_vmUuid;
	QDir m_dir;
	QStringList m_files;
};
	
} // namespace Snapshot
} // namespace Libvirt

/**
* @brief This class implements snapshot three managing logic
* @author Ilya@
*/
class CDspVmSnapshotStoreHelper
{

public:
	// constructor
	CDspVmSnapshotStoreHelper();

	// destructor
	~CDspVmSnapshotStoreHelper();

public:

	/////////////////////////////////////
	//
	//  clients requests
	//
	/////////////////////////////////////

	// Create new snapshot record in snapshots tree
	void createSnapshot(SmartPtr<CDspClient> user, const SmartPtr<IOPackage>& pkg,
						VIRTUAL_MACHINE_STATE vmState, PRL_UINT32 m_nFlags);

	// Switch to snapshot
	void switchToSnapshot(SmartPtr<CDspClient> user, const SmartPtr<IOPackage>& pkg);

	// Delete snapshot
	void deleteSnapshot(SmartPtr<CVmConfiguration> pVmConfig, const QString& sSnapshotUuid, bool bChild = false);

	// Returns snapshot tree
	PRL_RESULT sendSnapshotsTree(SmartPtr<CDspClient> user, const SmartPtr<IOPackage>& pkg);
	// atomically lock given snapshots list
	PRL_RESULT lockSnapshotList(const CVmIdent &vmId, const QStringList& snapList, PRL_RESULT rcode);
	// atomically unlock given snapshots list
	void unlockSnapshotList(const CVmIdent &vmIdent, const QStringList& snapList);
	// Check write access to snapshot
	PRL_RESULT IsSnapshotLocked(const QString &sSnapUuid);
	// Sets new snapshot name and description
	void updateSnapshotData(SmartPtr<CDspClient> user, const SmartPtr<IOPackage>& pkg);

	// Set default access rights to snapshot and hdd files
	static PRL_RESULT SetDefaultAccessRights(const QString& strFullPath
		, SmartPtr<CDspClient> pSession
		, const CVmDirectoryItem* pVmDirItem
		, bool bRecursive = true);

	static PRL_RESULT setDefaultAccessRightsToAllHdd(
		const SmartPtr<CVmConfiguration> pVmConfig
		, SmartPtr<CDspClient> pSession
		, const CVmDirectoryItem* pVmDirItem
		, bool bBreakByError );

// VirtualDisk commented out by request from CP team
//	static PRL_RESULT PrepareDiskStateManager(
//		const SmartPtr<CVmConfiguration> pVmConfig,
//		CDSManager* pStatesManager);

	/////////////////////////////////////
	//
	//  internal dispatcher requests
	//
	/////////////////////////////////////

	// Returns state of snapshoted VM
	PVE::SnapshotedVmState getSnapshotedVmState(SmartPtr<CDspClient> user, const SmartPtr<IOPackage>& pkg);

	/* Load snapshots tree under internal lock */
	bool getSnapshotsTree(const CVmIdent &vmIdent, CSavedStateStore *pSavedStateStore);
	bool getSnapshotsTree(const QString& sVmConfigPath, CSavedStateStore *pSavedStateStore);

	static bool doesSnapshotExist(const CVmIdent &vmIdent, const QString& snapshotId);
	static bool hasSnapshotChildren(const CVmIdent &vmIdent, const QString& snapshotId);

	// returns valid path or "" if vm doesn't exists
	static QString getPathToSnapshotsXml( const CVmIdent& vmIdent );

	void removeStateFiles(QString strVmFolder, QString strSnapshotUuid);

	PRL_RESULT removeHddFromSnapshotTree(SmartPtr<CDspClient> pUser,
			const QString sVmUUid,
			const CVmHardDisk *pHdd);

	// Notify VM clients: snapshot tree changed
	static void notifyVmClientsTreeChanged(const SmartPtr<IOPackage>& pkg,
		const CVmIdent& id );

private:

	// Notify VM clients: snapshot tree changed
	static void notifyVmClientsTreeChanged(const SmartPtr<IOPackage>& pkg,
		const QString &strVmDirUuid,
		const QString &strVmUuid);

	// decrypt encrypted snapshots and replace its as base64
	QString getSnapshotTreeWithBase64Images( SmartPtr<CDspClient> pUser
		, const SmartPtr<CVmConfiguration> pVmConfig
		, quint32 nFlags
		, CSavedStateStore& snapTree/*[ IN/OUT]*/ );

	PRL_RESULT removeHddFromVmConfig(SmartPtr<CDspClient> pUser,
			const QString &sVmHome, const QString &sSnapUuid,
			const QString &sHddSystemName);

	void fillSnapshotRuntimeFields(const CVmIdent& id, CSavedStateTree* pSavedState);

private:
	typedef QHash<QString, PRL_RESULT> SnapHashType;
	typedef QHash<CVmIdent, SmartPtr<SnapHashType> > CVmIdentSnapHashType;
	CVmIdentSnapHashType m_lockedSnap;
	mutable QMutex m_mutex;

}; // class CDspVmSnapshotStoreHelper


#endif // __CDspVmSnapshotStoreHelper_H_
