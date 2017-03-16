///////////////////////////////////////////////////////////////////////////////
///
/// @file CDspVmConfigurationChangesWatcher
///
/// This object monitored vm configuration files to changes its on disk
///		and posted events to clients
///	Also this object watches to paths of invalid VMs when config was moved.
///
/// @author Artemr
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

#ifndef CDSP_VM_CONFIGURATION_CHANGES_WATCHER_H
#define CDSP_VM_CONFIGURATION_CHANGES_WATCHER_H

#include <QFileSystemWatcher>
#include <QMap>
#include <QHash>
#include <QMutex>
#include <QPair>
#include <QSet>
#include <prlsdk/PrlErrors.h>
#include "CDspClientManager.h"
#include "CDspAccessManager.h"

namespace Watcher {
	class Processor;

	namespace Strategy {
		struct TimerBased;
		struct FsEventBased;
	}
}

class CDspVmConfigurationChangesWatcher : public QFileSystemWatcher
{
	Q_OBJECT

	friend class Watcher::Processor;

	friend struct Watcher::Strategy::TimerBased;
	friend struct Watcher::Strategy::FsEventBased;

public:
	CDspVmConfigurationChangesWatcher();
	virtual ~CDspVmConfigurationChangesWatcher(void);

	/**
	* @brief add configuration to watch changes.
	*/
	void registerVmToWatch(const QString &strConfigPath, const CVmIdent& vmIdent );

	void registerVmToWatch(const QString &strConfigPath,
				const QString &strVmDirUuid,
				const QString &strVmUuid);

	/**
	* @brief method emits signal about self recreate!
	*/
	void update();
	/**
	* @brief unregister all files from watcher!
	*/
	void unregisterAll();

	/**
	* @brief this function add to monitoring vm config files user connection handle
	* and current user permitions
	*/
	void addUserToMonitoringPermChanges(const IOSender::Handle& , const QString & strVmUuid = QString() );

	/**
	* @brief this function removed from monitoring vm config files user connection handle
	*/
	void removeUserFromMonitoringPermChanges( const IOSender::Handle& );
	void setEnabled(bool bEnable);

signals:
	void needToUpdateWatcher();

	void needRegisterVmToWatch(QString strConfigPath,
				QString strVmDirUuid,
				QString strVmUuid);
	void needToUnregisterVm(QString sVmConfigPath, bool bStillInvalid);

public slots:
	/**
	* @brief remove configuration from watch changes. Returns Ids of unregisterd Vms
	*/
	QList<CVmIdent> unregisterVmToWatch(QString strConfigPath, bool bStillInvalid = false );

private slots:
	/**
	* @brief slot - it called when config on disk changed .
	*/
	void vmConfigChanged(QString strVmPath);

	/**
	* @brief slot - it called to register vm to watch in thread when watcher called.
	*/
	void onRegisterVmToWatch(QString strConfigPath,
		QString strVmDirUuid,
		QString strVmUuid);

	/**
	* @brief slot - it called to update watcher with list of current vms.
	*/
	void updateVmConfigWatcher();

	//https://bugzilla.sw.ru/show_bug.cgi?id=439944
	/**
	* @brief slot - it called when parent VM home directory on disk changed .
	*/
	void vmParentDirChanged(QString strDirPath);

private:
	QString getConfigAsString( const CVmIdent& vmIdent, PRL_RESULT& err );

	virtual void timerEvent(QTimerEvent* te);

	void sendConfigChangesEvent( const CVmIdent& vmIdent );

	// #444190	We can't use QFileSystemWatcher  mech for some cases
	//			because it open file handle constantly and user can't to safe unmount disk with VM by that reason.
	//			Affects: [Sentillion player [win] ], and all removable devices on WIN platform.
	//
	//			TODO/FIXME:
	//			NOW it enabled only for SENTILLION_VTHERE_PLAYER and all storages on WIN platform
	//			Need implement it only for removable devices on WIN platform
	//
	//			NOTE: On MAC and Linux QFileSystemWatcher doesn't block umount procedure.

	bool allowToKeepOpenedFileHandle( const QString &strFilePath );

	/**
	* @brief this function added parent vm dirs(exclude vm dir) to HashOfParentDirs hash.
	*/
	void addParentDirsToMonitoring( const QString & strConfigPath );

	/**
	* @brief this function removed parent vm dirs(exclude vm dir) to HashOfParentDirs hash.
	*/
	void removeParentDirsfromMonitoring( const QString & strConfigPath );

public:

	struct VmInfo
	{
		CVmIdent	vmIdent;
		QDateTime	dtPathChanged;
		QString		sVmConfig;
		QMap<IOSender::Handle ,PRL_SEC_AM> qmVmUserRights;
		VmInfo(){}
		VmInfo( const CVmIdent& vmIdent, const QDateTime& dtPathChanged, const QString& sVmConfig)
			:vmIdent(vmIdent), dtPathChanged(dtPathChanged), sVmConfig(sVmConfig)
		{}
	};

	// Map <VmPath, VmInfo >
	typedef QMap< QString, VmInfo > ComplexMap;
		// FIXME: ComplexMap HAVE TO BE renamed! To ValidVmInfoMap for example

	// Hash <VmPath, CVmIdent >
	typedef QHash< QString, CVmIdent >  HashOfInvalidVm ;

	//QHash<QString,QSet<QString>>
	// QString-path to dir
	// QSet<QString> - set of configs contains this dir
	typedef QHash< QString, QSet< QString > > HashOfParentDirs;
private:
	QMutex	m_AccessMapMutex;

		// FIXME: Here DOES NOT SUPPORTED SHARED VM CASE (one vm in several vmdirectories.)
	ComplexMap m_mapVmPathToVmInfo;
	HashOfInvalidVm	m_hashInvalidVm;
	HashOfParentDirs m_hashParentDirs;

	int		m_iInvalidVmTimerId;
	bool	m_bEnabled;

private:
	/**
	* @brief this function returned users chnged permissions on vm for all registered users
	* must be declared after complexmap
	*/
	bool checkPermissionChanged( const VmInfo & vmsItem );

	void updateUserPermissionsForMonitoringVm( const CVmIdent & VmIdent);
	void init();
	void deinit();
};

#endif //CDSP_VM_CONFIGURATION_CHANGES_WATCHER_H
