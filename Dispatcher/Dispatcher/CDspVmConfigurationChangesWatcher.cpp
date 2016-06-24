///////////////////////////////////////////////////////////////////////////////
///
/// @file CDspVmConfigurationChangesWatcher
///
/// this object monitored vm configuration files to changes it on disk and posted events to clients
/// implementation
/// @author Artemr
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

// #define FORCE_LOGGING_ON
// #define FORCE_LOGGING_LEVEL		DBG_DEBUG



#include "CDspVmConfigurationChangesWatcher.h"
#include "CDspVmDirManager.h"
#include "CDspService.h"
#include "CDspBugPatcherLogic.h"
#include "Libraries/ProtoSerializer/CProtoSerializer.h"
#include <QFile>
#include <QMutexLocker>
#include <prlcommon/Std/noncopyable.h>

#include <prlcommon/Std/PrlAssert.h>

#ifdef _WIN_
	#include <process.h>
	#define getpid _getpid
#endif

using namespace Parallels;

namespace
{
	const int TIMEOUT_TO_WATCH_INVALID_VM_MSEC = 1000;
};

namespace Watcher
{
	typedef CDspVmConfigurationChangesWatcher::VmInfo VmInfo;
	typedef CDspVmConfigurationChangesWatcher::ComplexMap ComplexMap;
	typedef CDspVmConfigurationChangesWatcher::HashOfInvalidVm HashOfInvalidVm;
	typedef CDspVmConfigurationChangesWatcher::HashOfParentDirs HashOfParentDirs;

	typedef QPair< QString/* vmConfigPath */, VmInfo > ValidMapEntry;

	struct Data
	{
		QStringList lstBecomeInvalid;
		QStringList lstBecomeValid;
		QList< ValidMapEntry > lstStillValid;
		HashOfInvalidVm lstStillInvalid;

		mutable QSet<CVmIdent> lstEvents;
	};

class Processor: noncopyable
{
public:
	Processor( CDspVmConfigurationChangesWatcher& w)
		: m_d(&w){}

	template< class T>
	void process( T& strategy );

private:

	void processValidToInvalid( const Data& d );

	// returns Ids of becoming valid vms
	QSet<CVmIdent> processInvalidToValid( const Data& d );

	void applyBugPatcher( const QSet<CVmIdent>& lstToAppyPatches );

	void sendConfigChangeEvents( const QSet<CVmIdent>& lstEvents );

	void stopInvalidVms( const Data& d);

private:
	CDspVmConfigurationChangesWatcher& d() { return *m_d; }
private:

	CDspVmConfigurationChangesWatcher* m_d;
};

namespace Strategy
{
		struct TimerBased
		{
			void collect( CDspVmConfigurationChangesWatcher& w, Data& out );
			void processPermissions( const Data& d, CDspVmConfigurationChangesWatcher& w);
		};

		struct FsEventBased
		{
			void collect( CDspVmConfigurationChangesWatcher& w, Data& out );
			void processPermissions( const Data& d, CDspVmConfigurationChangesWatcher& w);

			FsEventBased( const QString& dir /* Any dir in VMs path. It is sent in file-system event */ )
				:m_dir(dir){}
		private:
			QString m_dir;
		};
} // namespace Strategy


} //  namespace Watcher


void CDspVmConfigurationChangesWatcher::init()
{
	// connect signal which emits when file changed
	bool bConnected = connect(this, SIGNAL(fileChanged ( QString )),
		this, SLOT(vmConfigChanged( QString )), Qt::QueuedConnection);
	PRL_ASSERT(bConnected);

	bConnected = connect(this, SIGNAL(directoryChanged ( QString )),
		this, SLOT(vmParentDirChanged( QString )), Qt::QueuedConnection);
	PRL_ASSERT(bConnected);

	bConnected = connect(this, SIGNAL(needRegisterVmToWatch(QString, QString, QString)),
		this, SLOT(onRegisterVmToWatch(QString, QString, QString)), Qt::QueuedConnection);
	PRL_ASSERT(bConnected);

	bConnected = connect(this,
		SIGNAL( needToUpdateWatcher() ),
		this,
		SLOT( updateVmConfigWatcher() ),
		Qt::QueuedConnection);
	PRL_ASSERT(bConnected);

	bConnected = connect(this,
		SIGNAL( needToUnregisterVm(QString, bool) ),
		this,
		SLOT( unregisterVmToWatch(QString, bool) ),
		Qt::QueuedConnection);
	PRL_ASSERT(bConnected);

	// #PDFM-42119
	// Timer-based logic is disabled for PDfM due performance issues.
	// Of course, we will not monitor vm availability state on shared storages
	// ( SMB / NFS / PSTORAGE / .../ )
	// But it is a lesser evil than constanly week-ups ( it blocks to go host to sleep ).
	// More details are here: #PDFM-42119

	m_iInvalidVmTimerId = startTimer( TIMEOUT_TO_WATCH_INVALID_VM_MSEC );
	PRL_ASSERT( -1 != m_iInvalidVmTimerId );
	Q_UNUSED(bConnected);
}

CDspVmConfigurationChangesWatcher::CDspVmConfigurationChangesWatcher()
:	m_AccessMapMutex( QMutex::Recursive ),
	m_iInvalidVmTimerId(-1),
	m_bEnabled(false)
{
}

void CDspVmConfigurationChangesWatcher::deinit()
{
	killTimer( m_iInvalidVmTimerId );
	disconnect(this);
}

CDspVmConfigurationChangesWatcher::~CDspVmConfigurationChangesWatcher(void)
{
	deinit();
}

void CDspVmConfigurationChangesWatcher::setEnabled(bool bEnable)
{
	if (bEnable != m_bEnabled)
	{
		QMutexLocker locker( &m_AccessMapMutex );
		if (bEnable)
		{
			WRITE_TRACE(DBG_FATAL, "Enable Vm config watcher");
			init();
		}
		else
		{
			WRITE_TRACE(DBG_FATAL, "Disable Vm config watcher");
			deinit();
		}
		m_bEnabled = bEnable;
	}
}

void CDspVmConfigurationChangesWatcher::onRegisterVmToWatch(QString strConfigPath,
						 QString strVmDirUuid,
						 QString strVmUuid)
{
	// NOTE:
	// Workaround for #456470
	// All operation with QFileSystemWatcher should be called under lock till QT patched
	//	as defined in https://bugzilla.sw.ru/show_bug.cgi?id=456470#c4
	// Need to prevent deadlock in QFileSystemWatcher::removePath() .

	WRITE_TRACE(DBG_TRACE, " %s", __FUNCTION__ );
	PRL_RESULT err = PRL_ERR_FIXME;
	CVmIdent vmIdent = MakeVmIdent( strVmUuid, strVmDirUuid );
	QString strConfigAsString = getConfigAsString( vmIdent, err);

	if( PRL_ERR_VM_UUID_NOT_FOUND == err )
	{
		WRITE_TRACE(DBG_FATAL, "Ignore register to watch vm, because this vm doesn't not found."
			"uuid = %s, path = %s"
			, QSTR2UTF8( strVmUuid )
			, QSTR2UTF8( strConfigPath ) );
		return;
	}

	bool bVmWasMoved = strConfigAsString.isEmpty() && !QFileInfo( strConfigPath ).exists();

	QMutexLocker locker( &m_AccessMapMutex );

	if( !bVmWasMoved )
	{
		QDateTime dt = QFileInfo( strConfigPath ).lastModified();

		m_mapVmPathToVmInfo.insert(
			strConfigPath,
			VmInfo( vmIdent, dt, strConfigAsString )
			);

		locker.unlock();
		updateUserPermissionsForMonitoringVm( vmIdent );
		locker.relock();
	}
	else
	{
		// Note: we allow to register empty config:
		//		reason - config file was corrupted, but exists on drive
		//		and we watch to change this config to valid state.

		m_hashInvalidVm.insert( strConfigPath, vmIdent );
		WRITE_TRACE(DBG_FATAL, "Vm by path '%s' was added to Watcher of invalid configs."
			, QSTR2UTF8( strConfigPath)
			);
	}

	if( !bVmWasMoved )
	{
		if( allowToKeepOpenedFileHandle( strConfigPath )/* #444190 */
			&& !files().contains( strConfigPath ) )
		{
			addPath( strConfigPath );
		}

		// add parent directories to monitoring
		addParentDirsToMonitoring( strConfigPath );
	}
}

void CDspVmConfigurationChangesWatcher::addParentDirsToMonitoring( const QString & strConfigPath )
{
	// "/x/y/z/a.pvm/config.pvs" ==> /, /x, /x/y, /x/y/z, /x/y/z/a.pvm

	QSet<QString> addedDirs = directories().toSet();

	bool first = true;
	QDir dir = QFileInfo( strConfigPath ).dir();
	do
	{
		if( !first )
		{
			// Do cd up:
			// We can't use QDir::cdUp() because it works for existing dirs only

			// FIXME: get rid this
			dir.setPath( QFileInfo( dir.absolutePath() ).absolutePath() );
		}
		else
			first = false;

		QString path = dir.absolutePath();

		HashOfParentDirs::iterator iter = m_hashParentDirs.find( path );
		if ( iter == m_hashParentDirs.end() )
			iter = m_hashParentDirs.insert( path , QSet<QString>() << strConfigPath );
		else if( !iter.value().contains( strConfigPath ) )
			iter.value().insert( strConfigPath );

		if ( allowToKeepOpenedFileHandle( path )
			&& !addedDirs.contains( path ) )
		{
			addPath( path );
			addedDirs << path;
		}
	}while ( !dir.isRoot() );

	// TODO: need to check for links in the path too.
}

/**
* @brief add configuration to watch changes.
* @param const QString &strVmUuid - configuration uuid
* @param const QString &strVmDirUuid - vm directory uuid
* @return none
*/
void CDspVmConfigurationChangesWatcher
	::registerVmToWatch(const QString &strConfigPath, const CVmIdent& vmIdent )
{
	registerVmToWatch( strConfigPath, vmIdent.second, vmIdent.first );
}


void CDspVmConfigurationChangesWatcher::registerVmToWatch(const QString &strConfigPath,
														  const QString &strVmDirUuid,
														  const QString &strVmUuid)
{
	if (!m_bEnabled)
		return;
	emit needRegisterVmToWatch(strConfigPath, strVmDirUuid, strVmUuid);
}

/**
* @brief remove configuration from watch changes.
* @param const QString &strVmUuid - configuration uuid
* @return none
*/
QList<CVmIdent> CDspVmConfigurationChangesWatcher::unregisterVmToWatch( QString strConfigPath, bool bStillInvalid )
{
	// NOTE:
	// Workaround for #456470
	// All operation with QFileSystemWatcher should be called under lock till QT patched
	//	as defined in https://bugzilla.sw.ru/show_bug.cgi?id=456470#c4
	// Need to prevent deadlock in QFileSystemWatcher::removePath() .

	WRITE_TRACE(DBG_TRACE, " %s, bStillInvalid=%d", __FUNCTION__ ,bStillInvalid  );

	QList<CVmIdent> lstUnregisteredIds;

	if (!m_bEnabled)
		return lstUnregisteredIds;

	QMutexLocker locker( &m_AccessMapMutex );


	VmInfo tmpInfo = m_mapVmPathToVmInfo.take( strConfigPath );
	lstUnregisteredIds << tmpInfo.vmIdent;
	if( !bStillInvalid )
	{
		m_hashInvalidVm.remove( strConfigPath );
		removeParentDirsfromMonitoring( strConfigPath );
	}

	if( files().contains( strConfigPath ) )
		removePath( strConfigPath );

	return lstUnregisteredIds;
}

void CDspVmConfigurationChangesWatcher::removeParentDirsfromMonitoring( const QString & strConfigPath )
{
	QSet<QString> addedDirs = directories().toSet();

	bool first = true;
	QDir dir = QFileInfo( strConfigPath ).dir();
	do
	{
		if( !first )
		{
			// Do cd up:
			// We can't use QDir::cdUp() because it works for existing dirs only
			dir.setPath( QFileInfo( dir.absolutePath() ).absolutePath() );
		}
		else
			first = false;

		QString path = dir.absolutePath();

		HashOfParentDirs::iterator iter = m_hashParentDirs.find( path );
		if ( iter == m_hashParentDirs.end() )
			continue;

		iter.value().remove( strConfigPath );
		if ( ! iter.value().empty() )
			continue;

		m_hashParentDirs.remove( path );
		if( addedDirs.contains( path ) )
		{
			removePath( path );
			addedDirs.remove( path );
		}

	}while ( !dir.isRoot() );
}

/**
* @brief slot - it called when config on disk changed .
* @param const QString & strVmPath - path to config on disk
* @return none
*/
void CDspVmConfigurationChangesWatcher::vmConfigChanged(QString strVmPath)
{
	WRITE_TRACE(DBG_INFO, "ConfigWatcher: vmConfigChanged: %s", QSTR2UTF8(strVmPath));

	CVmIdent vmIdent;
	QString sOldConfig;
	bool bNeedToUnregisterPath = false;
	bool bFileWasRemoved = ! files().contains(strVmPath);
	VmInfo tmpVmInfo;
	// search map for uuid of vm
	{
		QMutexLocker locker( &m_AccessMapMutex );

		bool flgFound = false;
		ComplexMap::Iterator it = m_mapVmPathToVmInfo.find( strVmPath );
		if( it != m_mapVmPathToVmInfo.end() )
		{
			flgFound = true;

			vmIdent		= it.value().vmIdent;
			sOldConfig	= it.value().sVmConfig;
			tmpVmInfo = it.value();
		}

		// #119128  VM was moved/deleted
		if( flgFound && bFileWasRemoved )
		{
			WRITE_TRACE(DBG_FATAL, "Config for vm=%s at path %s was moved or deleted from drive."
				"It was registered to watcher of invalid config"
				, QSTR2UTF8( vmIdent.first )
				, QSTR2UTF8( strVmPath )
				);

			m_mapVmPathToVmInfo.erase( it );
			m_hashInvalidVm.insert( strVmPath, vmIdent );

			bNeedToUnregisterPath = true;
		}

		if( !flgFound )
		{
			WRITE_TRACE(DBG_FATAL, "CDspVmConfigurationChangesWatcher:: path %s not found !",
				QSTR2UTF8( strVmPath ));
			return;
		}
	}

	if( ! bNeedToUnregisterPath && checkPermissionChanged( tmpVmInfo ) )
		updateUserPermissionsForMonitoringVm( vmIdent );

	PRL_RESULT err = PRL_ERR_FIXME;
	QString strActualCfg =	getConfigAsString( vmIdent, err );

	if( !bFileWasRemoved && strActualCfg == sOldConfig )
	{
		WRITE_TRACE( DBG_INFO, "ConfigWatcher: Config for vm %s at path %s was not changed."
				, QSTR2UTF8( vmIdent.first )
				, QSTR2UTF8( strVmPath )
				);
		return;
	}

	sendConfigChangesEvent( vmIdent );

	if( bFileWasRemoved )
		WRITE_TRACE( DBG_WARNING, "File by path %s was removed/replaced.", QSTR2UTF8(strVmPath));

	if (bNeedToUnregisterPath)
		emit needToUnregisterVm(strVmPath, true);
}

void CDspVmConfigurationChangesWatcher::vmParentDirChanged( QString strDirPath )
{
	WRITE_TRACE(DBG_INFO, "ConfigWatcher: vmHomeDirChanged: %s",
			QSTR2UTF8(strDirPath));

	Watcher::Processor proc(*this);
	Watcher::Strategy::FsEventBased s( strDirPath );
	proc.process( s );
}

/**
* @brief unregister all files from watcher!
* @return none
*/
void CDspVmConfigurationChangesWatcher::unregisterAll()
{
	WRITE_TRACE(DBG_TRACE, " %s", __FUNCTION__ );
	QMutexLocker locker( &m_AccessMapMutex );
	m_mapVmPathToVmInfo.clear();
	m_hashInvalidVm.clear();
	m_hashParentDirs.clear();
}

/**
* @brief method emits signal about self update!
*/
void CDspVmConfigurationChangesWatcher::update()
{
	if (!m_bEnabled)
		return;

	emit needToUpdateWatcher();
}

void CDspVmConfigurationChangesWatcher::updateVmConfigWatcher()
{
	WRITE_TRACE(DBG_TRACE, " %s", __FUNCTION__ );
	// clear first!
	unregisterAll();
	// it possible register to watch vm which not presents in vm directory.
	// but event about notification not will be posted later
	Vm::Directory::Dao::Free d;
	foreach(const Vm::Directory::Item::List::value_type& i, d.getItemList())
	{
		if (i.second->getVmType() == PVT_VM)
			registerVmToWatch(i.second->getVmHome(),
				i.first, i.second->getVmUuid());
	}
}


QString CDspVmConfigurationChangesWatcher::getConfigAsString(
	const CVmIdent& vmIdent,
	PRL_RESULT& err
	)
{
	bool bLoadCondigDirectlyFromDisk = true;
	// FIXME: Need load directly from disk without hash !!!
	SmartPtr<CVmConfiguration> pConfig =
		CDspService::instance()->getVmDirHelper().getVmConfigByUuid( vmIdent, err, true, bLoadCondigDirectlyFromDisk );

	if (!pConfig)
		return "";

	return pConfig->toString();
}

void CDspVmConfigurationChangesWatcher::timerEvent(QTimerEvent* te)
{
	WRITE_TRACE(DBG_TRACE, " %s", __FUNCTION__ );
	PRL_ASSERT( te );
	if( te->timerId() != m_iInvalidVmTimerId )
		return;

	Watcher::Processor proc(*this);
	Watcher::Strategy::TimerBased s;
	proc.process( s );
}

namespace Watcher
{

template<class Strategy>
void Processor::process( Strategy& strategy )
{
	QMutexLocker locker( & d().m_AccessMapMutex );

	Data data;
	strategy.collect( d(), data );

	processValidToInvalid( data );

	QSet<CVmIdent>
		lstNew = processInvalidToValid( data );

	locker.unlock();

	applyBugPatcher( lstNew );

	strategy.processPermissions( data, d() );

	sendConfigChangeEvents( data.lstEvents );

	stopInvalidVms( data );
}

void Processor::processValidToInvalid( const Data& data )
{
		// 1. process valid --> invalid
		foreach( const QString& path, data.lstBecomeInvalid )
		{
			VmInfo vi = d().m_mapVmPathToVmInfo.take( path );
			data.lstEvents << vi.vmIdent ;
			d().m_hashInvalidVm.insert( path, vi.vmIdent );

			WRITE_TRACE(DBG_FATAL
				, "Vm was not found on the disk. It will moved to invalid list. vm %s  path='%s'"
			, QSTR2UTF8( vi.vmIdent.first )
			, QSTR2UTF8( path )
			);
		}
}

QSet<CVmIdent> Processor::processInvalidToValid( const Data& data )
{
		QSet<CVmIdent> lstNew;
		// ==============================
		// 2. process invalid --> valid
		foreach( const QString& path, data.lstBecomeValid )
		{
			CVmIdent id = d().m_hashInvalidVm.take( path );

			data.lstEvents << id;
			lstNew << id;

			d().registerVmToWatch( path, id );

			WRITE_TRACE(DBG_FATAL, "Vm was already found on the disk. vm=%s, path='%s'"
				, QSTR2UTF8( id.first )
				, QSTR2UTF8( path )
			);
		}

		return lstNew;
}

void Processor::applyBugPatcher( const QSet<CVmIdent>& lstToAppyPatches )
{
	if( lstToAppyPatches.size() )
	{
		// #PDFM-23383 patch ex-invalid VM configs
		CDspBugPatcherLogic logic( *CDspService::instance()->getHostInfo()->data() );
		foreach(const CVmIdent& id, lstToAppyPatches )
			logic.patchVmConfig(id);
	}
}

void Processor::sendConfigChangeEvents( const QSet<CVmIdent>& events )
{
	foreach( const CVmIdent& id, events )
		d().sendConfigChangesEvent( id );
}

void Processor::stopInvalidVms( const Data& data )
{
	for( HashOfInvalidVm::ConstIterator itTmp = data.lstStillInvalid.begin();
		itTmp != data.lstStillInvalid.end(); ++itTmp )
	{
		//At first check case that whether VM was just renamed
		//see https://bugzilla.sw.ru/show_bug.cgi?id=440382 for more details
		QString sVmHome;
		{
			CDspLockedPointer<CVmDirectoryItem>
				pVmDirItem = CDspService::instance()->getVmDirManager()
											.getVmDirItemByUuid( itTmp.value() );
			if ( pVmDirItem )
			{
				sVmHome = pVmDirItem->getVmHome();
				if ( pVmDirItem && QFile::exists( sVmHome ) )
					// VM was just renamed - skip VM process stop
					continue;
			}
		}
		SmartPtr<CDspVm> pVm = CDspVm::GetVmInstanceByUuid( itTmp.value() );
		if (pVm)
		{
			VIRTUAL_MACHINE_STATE nState = pVm->getVmState();
			if ( !(nState == VMS_STOPPING || nState == VMS_STOPPED
				|| nState == VMS_SUSPENDING || nState == VMS_SUSPENDED || nState == VMS_SUSPENDING_SYNC))
			{
#ifdef _WIN_
				//Perhaps VM on network storage
				if ( pVm->getVmRunner().isValid() )
				{
					//https://bugzilla.sw.ru/show_bug.cgi?id=267152
					CAuthHelperImpersonateWrapper _impersonate( &pVm->getVmRunner()->getAuthHelper() );

					// VM stored on network share which not seen for dispatcher session - skip it
					if ( QFile::exists( sVmHome ) )
						continue;
				}
#endif

				SmartPtr<CDspClient> pStopUser = pVm->getVmRunner();
				if( !pStopUser )
				{
					pStopUser = SmartPtr<CDspClient>(new CDspClient(IOSender::Handle()));
					pStopUser->getAuthHelper().AuthUserBySelfProcessOwner();
					pStopUser->setVmDirectoryUuid( pVm->getVmDirUuid() );
				}

				CProtoCommandPtr pCmd =
					CProtoSerializer::CreateProtoVmCommandStop(pVm->getVmUuid(), PSM_KILL, 0);
				const SmartPtr<IOPackage> p = DispatcherPackage::createInstance( PVE::DspCmdVmStop, pCmd );
				WRITE_TRACE( DBG_FATAL, "Stopping VM '%s' due path '%s' not exists"
					, QSTR2UTF8(itTmp.value().first)
					, QSTR2UTF8(itTmp.key()) );
		//		pVm->stop(pStopUser, p, PSM_KILL, true);
			}
		}
	}

}

////////////////   Watcher::TimerBased /////////////////////

void Strategy::TimerBased::collect( CDspVmConfigurationChangesWatcher& w, Data& out )
{
	// FIXME Change to iterators
	foreach( const QString& path, w.m_mapVmPathToVmInfo.keys() )
	{
		if ( !QFile::exists( path ) )
			out.lstBecomeInvalid += path;
		else
			out.lstStillValid += qMakePair( path, w.m_mapVmPathToVmInfo.value( path ) );
	}

	// FIXME Change to iterators
	foreach( const QString& path, w.m_hashInvalidVm.keys() )
	{
		if ( QFile::exists( path ) )
			out.lstBecomeValid += path;
		else
			out.lstStillInvalid[ path ] = w.m_hashInvalidVm.value( path );
	}
}

void Strategy::TimerBased::processPermissions( const Data& data, CDspVmConfigurationChangesWatcher& w)
{
	//////////////////////////////////////////////////////////////////////////
	// #444190 allowToKeepOpenedFileHandle() logic
	//	monitoring for vms for which we can not use QFileSystemWatcher mech
	foreach( const ValidMapEntry& e, data.lstStillValid )
	{
		QString sPath = e.first;

		// #444190 skip logic which will processed through QFileSystemWatcher mech by slot vmConfigChanged()
		if( w.allowToKeepOpenedFileHandle( sPath ) )
			continue;

		// only for server mode to optimize perfomance for Desktop because check permission pooling mode
		// may be hard for CPU
		{
			// send event to client if vm permissions changed for curent user
			// this code pooling 1 time to second user permissions for windows server mode
			if( w.checkPermissionChanged( e.second ) )
				w.updateUserPermissionsForMonitoringVm( e.second.vmIdent );
		}

		// check file timestamp to work like vmConfigChanged()
		// skip if file doesn't changed
		if( QFileInfo(sPath).lastModified() == e.second.dtPathChanged )
			continue;

		// skip race. will be fixed on next iteration of this method
		if ( !QFile::exists( sPath ) )
			continue;

		PRL_RESULT err = PRL_ERR_FIXME;
		QString strActualCfg =
				w.getConfigAsString( e.second.vmIdent, err );

		// lock again to update information
		{
			QMutexLocker locker( & w.m_AccessMapMutex );
			ComplexMap::Iterator itValid = w.m_mapVmPathToVmInfo.find( sPath );

			if( itValid == w.m_mapVmPathToVmInfo.end() )
				continue;

			if( strActualCfg != itValid.value().sVmConfig )
				itValid.value().sVmConfig = strActualCfg;
			else
				WRITE_TRACE( DBG_INFO, "Config at path %s was not changed. vm %s"
					, QSTR2UTF8( sPath )
					, QSTR2UTF8( itValid.value().vmIdent.first )
				);

			itValid.value().dtPathChanged = QFileInfo( sPath ).lastModified();
		}

		data.lstEvents += e.second.vmIdent;
	}
}

////////////////   Watcher::FsEventBased /////////////////////

void Strategy::FsEventBased::collect( CDspVmConfigurationChangesWatcher& w, Data& out )
{
	// check for vms from changed parent directory
	HashOfParentDirs::iterator iter = w.m_hashParentDirs.find( m_dir );
	if ( iter == w.m_hashParentDirs.end() )
		return;

	foreach ( const QString& path/* path to file convig.pvs */, iter.value() )
	{
		if( w.m_mapVmPathToVmInfo.contains( path ) )
		{
			if( ! QFile::exists( path ) )
				out.lstBecomeInvalid += path;
			else
				out.lstStillValid += qMakePair( path, w.m_mapVmPathToVmInfo.value( path ) );
		}

		if( w.m_hashInvalidVm.contains( path ) )
		{
			if( QFile::exists( path ) )
				out.lstBecomeValid += path;
			else
				out.lstStillInvalid[ path ] = w.m_hashInvalidVm.value( path );
		}
	}
}

void Strategy::FsEventBased::processPermissions( const Data& data, CDspVmConfigurationChangesWatcher& w)
{
	foreach( const ValidMapEntry& e, data.lstStillValid )
	{
		// check valid --> valid ( check perms, color, etc )
		if( w.checkPermissionChanged( e.second ) )
			w.updateUserPermissionsForMonitoringVm( e.second.vmIdent );

		// Checks for event with vm-bundle's directory ONLY:
		QString sVmDir = QFileInfo( e.first ).absolutePath();
		if( QFileInfo( sVmDir ) == m_dir )
		{
		}
	}
}

} // namespace Watcher

void CDspVmConfigurationChangesWatcher::sendConfigChangesEvent( const CVmIdent& vmIdent )
{
	WRITE_TRACE(DBG_TRACE, " %s", __FUNCTION__ );

	// To prevent race on load config from cache and from disk we do not update cache here!
	// We remove config from cache, so user load last version from disk!
	QString sVmHome = CDspVmDirManager::getVmHomeByUuid(vmIdent);
	CDspService::instance()->getVmConfigManager().removeFromCache( sVmHome );

	// post vm config change event!
	CVmEvent event( PET_DSP_EVT_VM_CONFIG_CHANGED,
		vmIdent.first,
		PIE_DISPATCHER );

	SmartPtr<IOPackage> p = DispatcherPackage::createInstance( PVE::DspVmEvent, event );
	CDspService::instance()->getClientManager()
		.sendPackageToVmClients( p, vmIdent );

	WRITE_TRACE(DBG_INFO, "Event EVT_VM_CONFIG_CHANGED was sent for vm=%s", QSTR2UTF8( vmIdent.first ) );
}

bool CDspVmConfigurationChangesWatcher::allowToKeepOpenedFileHandle( const QString &strFilePath  )
{
	// #444190	We can't use QFileSystemWatcher  mech for some cases
	//			because it open file handle constantly and user can't to safe unmount disk with VM by that reason.
	//			Affects: [Sentillion player [win] ], and all removable devices on WIN platform.
	//
	//			TODO/FIXME:
	//			NOW it enabled only for SENTILLION_VTHERE_PLAYER and all storages on WIN platform
	//			Need implement it only for removable devices on WIN platform
	//
	//			NOTE: On MAC and Linux QFileSystemWatcher doesn't block umount procedure.

	Q_UNUSED(strFilePath);

#ifdef SENTILLION_VTHERE_PLAYER
	return false;
#endif

#ifdef _WIN_
	return false;
#endif

	return true;
}

void CDspVmConfigurationChangesWatcher::addUserToMonitoringPermChanges( const IOSender::Handle& h,
																	   const QString & strVmUuid )
{
	SmartPtr<CDspClient> pClient = CDspService::instance()->getClientManager().getUserSession( h );
	if( !pClient )
		return;

	CVmDirectory tmpVmDirCatalogue;
	if( !strVmUuid.isEmpty() )
	{
		// get only one dir item
		CVmDirectoryItem * pItem = new CVmDirectoryItem;

		CDspLockedPointer<CVmDirectoryItem> pLockedItem =
			CDspService::instance()->getVmDirManager().getVmDirItemByUuid( pClient->getVmDirectoryUuid(),
												strVmUuid );
		if (!pLockedItem)
		{
			delete pItem;
			return;
		}
		pItem->fromString( pLockedItem->toString() );
		tmpVmDirCatalogue.m_lstVmDirectoryItems.append( pItem );
	}
	else
	{// for CDspLockedPointer
		CDspLockedPointer<CVmDirectory> pVmDirCat =
			CDspService::instance()->getVmDirManager().getVmDirectory( pClient->getVmDirectoryUuid() );
		if (!pVmDirCat)
			return;

		tmpVmDirCatalogue.fromString( pVmDirCat->toString() );
	}

	QMutexLocker locker( &m_AccessMapMutex );

	// search diritem
	foreach( CVmDirectoryItem * pItem, tmpVmDirCatalogue.m_lstVmDirectoryItems )
	{
		ComplexMap::Iterator it = m_mapVmPathToVmInfo.find( pItem->getVmHome() );

		if( ( it != m_mapVmPathToVmInfo.end() ) &&
		( pItem->getVmUuid() == it->vmIdent.first ) &&
			( pClient->getVmDirectoryUuid() == it->vmIdent.second ) )
		{
			CDspAccessManager::VmAccessRights rights = CDspService::instance()->getAccessManager().getAccessRightsToVm(
														pClient,
														pItem );
			it.value().qmVmUserRights.insert( h, rights.getVmAccessRights() );
		}
	}
}

void CDspVmConfigurationChangesWatcher::removeUserFromMonitoringPermChanges( const IOSender::Handle& h)
{
	QMutexLocker locker( &m_AccessMapMutex );
	ComplexMap::Iterator it = m_mapVmPathToVmInfo.begin();

	while( it != m_mapVmPathToVmInfo.end() )
	{
		it.value().qmVmUserRights.remove( h );
		it++;
	}
}

bool CDspVmConfigurationChangesWatcher::checkPermissionChanged( const VmInfo & vmsItem )
{

	// check that permissions for vm changes for all registered users
	// search in vminfo struct for all vm users
	bool bPermChanged = false;
	QMap<IOSender::Handle ,PRL_SEC_AM>::const_iterator clientsIt = vmsItem.qmVmUserRights.begin();
	while( clientsIt != vmsItem.qmVmUserRights.end() )
	{
		SmartPtr<CDspClient> pClient =
			CDspService::instance()->getClientManager().getUserSession( clientsIt.key() );

		if( pClient.isValid() )
		{
			CDspAccessManager::VmAccessRights rights =
				CDspService::instance()->getAccessManager().getAccessRightsToVm(
																pClient,
																vmsItem.vmIdent.first );

			if( rights.getVmAccessRights() != clientsIt.value() )
			{
				CDspService::instance()->getVmDirHelper().sendEventVmSecurityChangedToUser(
																pClient,
																vmsItem.vmIdent.first );

				bPermChanged = true;
			}
		}
		clientsIt++;
	}
	return bPermChanged;
}

void CDspVmConfigurationChangesWatcher::updateUserPermissionsForMonitoringVm( const CVmIdent & VmIdent )
{
	// this function adds all logged for current vm directory users to monitoring map
	// only dir uuid uses because we use getSessionsListSnapshot function instead getSessionListByVm which
	// have not internal locks
	// we uses getSessionsListSnapshot because it return list all sessions without permission on vm checks
	QHash< IOSender::Handle, SmartPtr<CDspClient> > qhClients =
		CDspService::instance()->getClientManager().getSessionsListSnapshot( VmIdent.second );
	QList< IOSender::Handle > qlKeys = qhClients.keys();

	foreach( IOSender::Handle h, qlKeys )
		addUserToMonitoringPermChanges( h, VmIdent.first );
}

