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
///	CDspVmDirHelper.h
///
/// @brief
///	Definition of the class CDspVmDirHelper
///
/// @brief
///	This class implements VM Directory managing logic
///
/// @author sergeyt
///	SergeyM
///
/// @date
///	2006-04-04
///
/// @history
///
////////////////////////////////////////////////////////////////////////////////

#ifndef __CDspVmDirHelper_H_
#define __CDspVmDirHelper_H_

#include <QMutex>

#include <prlcommon/Interfaces/ParallelsDomModel.h>
#include <prlcommon/Interfaces/ParallelsNamespace.h>

#include "CDspClient.h"
#include "CDspVm.h"
#include "CDspVmMounter.h"

#include <prlcommon/IOService/IOCommunication/IOServer.h>

#include <prlxmlmodel/VmConfig/CVmConfiguration.h>

#include <prlxmlmodel/VmDirectory/CVmDirectory.h>
#include <prlcommon/Std/SmartPtr.h>

#include "CDspSync.h"
#include <prlxmlmodel/ProblemReport/CProblemReport.h>

#include "Tasks/Task_DeleteVm.h"

class CMultiEditMergeVmConfig;
class CVmHardDisk;
class CPackedProblemReport;

/**
* @brief This class implements VM Directory managing logic
* @author SergeyM
*/
class CDspVmDirHelper
{
	friend class CDspVmDirManager; // to access to CDspVmDirHelper::getVmConfigForDirectoryItem();
	friend class CDspVm;
public:
	// constructor
	CDspVmDirHelper ();

	// destructor
	~CDspVmDirHelper();

public:
	/////////////////////////////////////
	//
	//  clients requests
	//
	/////////////////////////////////////


	/**
	 * @brief Gets VM list for specified user
	 * @param pUserSession
	 */
	QList<QString> getVmList ( SmartPtr<CDspClient>& pUserSession ) const;

	// Sends VM list
	bool sendVmList(const IOSender::Handle& sender,
		SmartPtr<CDspClient> pUserSession,
		const SmartPtr<IOPackage>& pkg );

	// Sends VM configuration
	bool sendVmConfig ( const IOSender::Handle& sender,
		SmartPtr<CDspClient> pUserSession,
		const SmartPtr<IOPackage>& );

	// Sends VM configuration by given name or UUID
	bool findVm ( const IOSender::Handle& sender,
		SmartPtr<CDspClient> pUserSession,
		const SmartPtr<IOPackage>& );

	// Sends VM info
	bool sendVmInfo( const IOSender::Handle& sender,
		SmartPtr<CDspClient> pUserSession,
		const SmartPtr<IOPackage>& );

	// Sends VM Tools info
	bool sendVmToolsInfo( const IOSender::Handle& sender,
		SmartPtr<CDspClient> pUserSession,
		const SmartPtr<IOPackage>& );

	// Register new VM in catalogue
	void createNewVm ( const IOSender::Handle& sender,
		SmartPtr<CDspClient> pUserSession,
		const SmartPtr<IOPackage>& );

	// Register VM in catalogue
	void registerVm ( SmartPtr<CDspClient> pUserSession,
		const SmartPtr<IOPackage>& );

	// Restore VM config
	void restoreVm ( SmartPtr<CDspClient> pUserSession,
		const SmartPtr<IOPackage>& );

	// Create new image file
	void createNewImage ( const IOSender::Handle& sender,
		SmartPtr<CDspClient> pUserSession,
		const SmartPtr<IOPackage>& );

	// Copy image file
	void copyImage ( SmartPtr<CDspClient> pUserSession,
		const SmartPtr<IOPackage>& p);

	// VM in catalogue
	void cloneVm ( const IOSender::Handle& sender,
		SmartPtr<CDspClient> pUserSession,
		const SmartPtr<IOPackage>& );

	// Initiates VM migration task
	void migrateVm ( const IOSender::Handle& sender,
		SmartPtr<CDspClient> pUserSession,
		const SmartPtr<IOPackage>& );

	// Check VM configuration
	void validateSectionVmConfig(SmartPtr<CDspClient> pUserSession,
		const SmartPtr<IOPackage>& );

	// Delete VM
	void deleteVm ( const IOSender::Handle& sender,
		SmartPtr<CDspClient> pUserSession,
		const SmartPtr<IOPackage>& );

	// Unregister VM
	void unregVm ( const IOSender::Handle& sender,
		SmartPtr<CDspClient> pUserSession,
		const SmartPtr<IOPackage>& );

	// Edit VM
	void beginEditVm ( const IOSender::Handle& sender,
		SmartPtr<CDspClient> pUserSession,
		const SmartPtr<IOPackage>& );

	void editVm ( const IOSender::Handle& sender,
		SmartPtr<CDspClient> pUserSession,
		const SmartPtr<IOPackage>& );

	// return true when config was updated otherwise return false
	bool atomicEditVmConfigByVm(const QString &sClientVmDirUuid,
		const QString& vmUuid,
		const CVmEvent& evtFromVm,
		SmartPtr<CDspClient> pUserSession
		);
	bool atomicEditVmConfigByVm(const CVmIdent &vmIdent,
		const CVmEvent& evtFromVm,
		SmartPtr<CDspClient> pUserSession
		)
	{
		return atomicEditVmConfigByVm( vmIdent.second, vmIdent.first, evtFromVm, pUserSession );
	}

	void dropSuspendedState( const IOSender::Handle& sender,
		SmartPtr<CDspClient> pUserSession,
		const SmartPtr<IOPackage>& );

	// search lost configs
	void searchLostConfigs (SmartPtr<CDspClient> pUserSession, const SmartPtr<IOPackage>& );

	// Create floppy image file for unattended Windows installation
	void createUnattendedFloppy ( SmartPtr<CDspClient> pUserSession, const SmartPtr<IOPackage>&p );

	// Create bootable ISO-image file for unattended Linux installation
	void createUnattendedCd ( SmartPtr<CDspClient> pUserSession, const SmartPtr<IOPackage>&p );

	// get problem report for stopped vm
	bool UpdateDeviceInfo (const IOSender::Handle& sender,SmartPtr<CDspClient> pUser, const SmartPtr<IOPackage>&p );

	// Updates VM security info
	void updateVmSecurityInfo ( SmartPtr<CDspClient> pUserSession, const SmartPtr<IOPackage>&p );

	/** Get suspended VM screen */
	void getSuspendedVmScreen( SmartPtr<CDspClient> pUserSession, const SmartPtr<IOPackage>& p );

	// Start convert VM virtual disks
	void startConvertDisks( SmartPtr<CDspClient> pUserSession, const SmartPtr<IOPackage>& p );
	void mountVm( SmartPtr<CDspClient> pUserSession, const SmartPtr<IOPackage>& p );
	void umountVm( SmartPtr<CDspClient> pUserSession, const SmartPtr<IOPackage>& p );

	void moveVm( SmartPtr<CDspClient> pUserSession, const SmartPtr<IOPackage>& pkg );

	/**
	 * Processes request on VM exclusive lock
	 * @param pointer to the user session requesting operation
	 * @param pointer to the package object
	 */
	void lockVm( SmartPtr<CDspClient> pUserSession, const SmartPtr<IOPackage> &p );

	/**
	 * To lock VM and send event to clients
	 * @param Vm UUID
	 * @param Directory UUID
	 * @param pointer to the user session requesting operation
	 * @param incoming package
	 */
	PRL_RESULT lockVm(
			const QString& vmUuid,
			const SmartPtr<CDspClient> &pUserSession,
			const SmartPtr<IOPackage> &p);

	/**
	 * Processes request on VM unlock
	 * @param pointer to the user session requesting operation
	 * @param pointer to the package object
	 */
	void unlockVm( SmartPtr<CDspClient> pUserSession, const SmartPtr<IOPackage> &p );

	/**
	 * To unlock VM and send event to clients
	 * @param Vm UUID
	 * @param Directory UUID
	 * @param pointer to the user session requesting operation
	 * @param incoming package
	 */
	PRL_RESULT unlockVm(
			const QString& vmUuid,
			const SmartPtr<CDspClient> &pUserSession,
			const SmartPtr<IOPackage> &p);

	// Unregister and delete VM from disk
	CDspTaskFuture<Task_DeleteVm> unregOrDeleteVm( SmartPtr<CDspClient> pUserSession,
		const SmartPtr<IOPackage>& pkg,
		const QString& vm_config,
		PRL_UINT32 flags,
		const QStringList & strFilesToDelete = QStringList());

public:
	/////////////////////////////////////
	//
	//  dispatcher internal requests
	//
	/////////////////////////////////////

	/**
	* Update vm state on all client.
	*/
	static void sendNotValidState(SmartPtr<CDspClient> pUserSession, PRL_RESULT rc
		, const QString& vmUuid, bool bSetNotValid);

	/**
	* Update vm state on all client.
	*/
	static void sendNotValidState(const QString& strVmDirUuid, PRL_RESULT rc
		, const QString& vmUuid, bool bSetNotValid);

	// Set VM Directory accessor for specified user
	bool createUsersVmDirectory( SmartPtr<CDspClient> pUserSession );

	/**
	 * Get VM configuration by WS request
	 * @param pointer to user session object
	 * @param VM UUID
	 * @param error code (out parameter - error code will be stored in it if operation'll fail)
	 * @param pointer to additional error info (necessary error message parameters such as VM name
	 *			and etc. will be stored here if operation'll fail)
	 */
	static SmartPtr<CVmConfiguration> getVmConfigByUuid ( SmartPtr<CDspClient>,
		const QString& vm_uuid,
		PRL_RESULT&	outError,
		CVmEvent *pErrorInfo = NULL);

	/**
	 * Get VM configuration by VM UUID and VM dir UUID
	 * @param VM directory UUID
	 * @param VM UUID
	 */
	SmartPtr<CVmConfiguration> getVmConfigByUuid ( const QString &sVmDirUuid,
		const QString& vm_uuid,
		PRL_RESULT&	outError,
		bool bAbsolute = true,
		bool bLoadConfigDirectlyFromDisk = false);

	/**
	* Get VM configuration by VmIdent
	*/
	SmartPtr<CVmConfiguration> getVmConfigByUuid (
		const CVmIdent& vmIdent,
		PRL_RESULT&	outError,
		bool bAbsolute = true,
		bool bLoadConfigDirectlyFromDisk = false )
	{
		return getVmConfigByUuid( vmIdent.second, vmIdent.first, outError, bAbsolute, bLoadConfigDirectlyFromDisk );
	}

	// get VM directory item by uuid
	CDspLockedPointer<CVmDirectoryItem> getVmDirectoryItemByUuid ( SmartPtr<CDspClient>,
		const QString& vmUuid );


	// get VM directory item by uuid
	CDspLockedPointer<CVmDirectoryItem> getVmDirectoryItemByUuid ( const QString &sVmDirUuid,
		const QString& vmUuid );


	// Insert VM directory item
	PRL_RESULT insertVmDirectoryItem ( const QString &sDirUuid,
		CVmDirectoryItem* vm_item );

	// Delete vm directory item
	PRL_RESULT deleteVmDirectoryItem ( const QString &sDirUuid,
		const QString& vm_uuid );

	/**
	* Add new Vm Directory to Vm Directory Catalogue
	* @return return UUID of new entry or empty string if operation failed.
	**/
	QString CreateVmDirCatalogueEntry ( const QString& vmDirPath,
		const QString& vmDirName );

	/**
	* Return path of Vm Home of user.
	* Note: pUser should be fully initialized.
	**/
	static QString getVmRootPathForUser ( SmartPtr<CDspClient> pUser );

	/**
	 * @brief Gets VM list: keys is a vm dir uuid, value is a vm uuid
	 * @param vmDirUuid
	 */
	QMultiHash<QString, SmartPtr<CVmConfiguration> > getAllVmList(const QString& vmDirUuid = QString()) const;

	/** @brief Register lock to exclusive operations with vm.
	* @param VM UUID
	* @param parent VM dir UUID
	* @param registering operation identifier
	* @param pointer to the owner session object
	* @return PRL_ERR_SUCCESS if operation was registered
	* @return PRL_ERR_*  if wasn't registered.
	**/
	PRL_RESULT	registerExclusiveVmOperation( const QString& vmUuid,
		const QString& vmDirUuid,
		PVE::IDispatcherCommands cmd,
		const SmartPtr<CDspClient> &pUserSession,
		const QString &sTaskId = QString());

	/** @brief unRegister exclusive operations with vm, registered with 'registerExclusiveOperation()' call;
	* @param VM UUID
	* @param parent VM dir UUID
	* @param registering operation identifier
	* @param pointer to the owner session object
	**/
	PRL_RESULT  unregisterExclusiveVmOperation( const QString& vmUuid,
		const QString& vmDirUuid,
		PVE::IDispatcherCommands cmd,
		const SmartPtr<CDspClient> &pUserSession );

	/** @brief replaces exclusive operation with another one.
	* @param VM UUID
	* @param parent VM dir UUID
	* @param replacing operation identifier
	* @param new operation identifier
	* @param pointer to the owner session object
	**/
	PRL_RESULT  replaceExclusiveVmOperation( const QString& vmUuid,
		const QString& vmDirUuid,
		PVE::IDispatcherCommands fromCmd,
		PVE::IDispatcherCommands toCmd,
		const SmartPtr<CDspClient> &pUserSession );

	/** Methods provided for convenience. Let to pass directly session connection
	 * handle instead of pointer to the session object
	**/
	PRL_RESULT	registerExclusiveVmOperation( const QString& vmUuid,
		const QString& vmDirUuid,
		PVE::IDispatcherCommands cmd,
		const IOSender::Handle &hUserSession,
		const QString &sTaskId = QString());
	PRL_RESULT  unregisterExclusiveVmOperation( const QString& vmUuid,
		const QString& vmDirUuid,
		PVE::IDispatcherCommands cmd,
		const IOSender::Handle &hUserSession );
	PRL_RESULT  replaceExclusiveVmOperation( const QString& vmUuid,
		const QString& vmDirUuid,
		PVE::IDispatcherCommands fromCmd,
		PVE::IDispatcherCommands toCmd,
		const IOSender::Handle &hUserSession );

	/** Returns session handle which locked VM */
	IOSender::Handle getVmLockerHandle( const QString& vmUuid, const QString& vmDirUuid ) const;

	/** Returns Task uuid's which locks the Vm */
	QList<QString> getTasksUnderExclusiveOperation( const CVmIdent &vmIdent ) const;

	/**
	 * Cleanups all exclusive VM locks that belongs to specified session
	 * handle
	 * @param session handle
	 */
	void cleanupSessionVmLocks( const IOSender::Handle &hSession );

	static QStringList getListOfLastCrashedLogs(const QStringList& listOfCrashDirs );

	/** #9246 */
	bool loadSecureData( SmartPtr<CVmConfiguration> vmConfig );

	/** #9246 */
	PRL_RESULT getVmStartUser(SmartPtr<CVmConfiguration> vmConfig,
								SmartPtr<CDspClient> &pRunUser,
								bool bAutoStart = false);

	/** #9246 */
	void resetSecureParamsFromVmConfig( SmartPtr<CVmConfiguration> vmConfig );

	/** bug #9058 **/
	void recoverMixedVmPermission( SmartPtr<CDspClient> pOwnerSession );

        /**
        * Processes command of start Vm disk resizing
        * @param pointer to the user session object that initialized request
        * @param pointer to request package object
        */
        void resizeDiskImage(SmartPtr<CDspClient> pUser, const SmartPtr<IOPackage> &p);

	/**
	* append additional parameters from VM directory
	* (VM home, last change date, last modification date, ....)
	*/
	void appendAdvancedParamsToVmConfig ( SmartPtr<CDspClient> pUserSession,
		SmartPtr<CVmConfiguration> pOutVmConfig
		//https://bugzilla.sw.ru/show_bug.cgi?id=439944
		, bool bSynchronizeFileColor = false,
		bool bFillAutogenerated = false
		);

	/**
	* reset additional parameters in VM configuration
	* (VM home, last change date, last modification date - never store in VM configuration itself!)
	*/
	void resetAdvancedParamsFromVmConfig( SmartPtr<CVmConfiguration> pOutVmConfig );

	/**
	* fill full user specified information
	*/
	void fillOuterConfigParams( SmartPtr<CDspClient> pUserSession,
			SmartPtr<CVmConfiguration> pOutVmConfig, bool bFillAutogenerated = false );

	void setNetworkRate(
			const SmartPtr<CVmConfiguration> &pVmConfig,
			const SmartPtr<CDspClient> pUser,
			const SmartPtr<IOPackage> p =
				DispatcherPackage::createInstance( PVE::DspCmdCtlDispatherFakeCommand )
			);
	void restartNetworkShaping(
			bool initConfig,
			SmartPtr<CDspClient> pUser,
			const SmartPtr<IOPackage> pPackage);

	/**
	* Generate "VM security changed" event for usr session
	* @param vm dir uuid
	* @param vm uuid
	* @param pointer to the request package initiating an event
	*/
	static void sendEventVmSecurityChangedToUser(	SmartPtr<CDspClient> pUserSession,
											const QString& vmUuid,
											const SmartPtr<IOPackage> &pRequest = SmartPtr<IOPackage>(0) );

	// Get multi edit dispatcher object
	CMultiEditMergeVmConfig* getMultiEditDispatcher();

	PRL_VM_COLOR getUniqueVmColor( const QString& vmDirUuid );

	static SmartPtr<CVmConfiguration> CreateVmConfigFromDirItem(
				const QString& sServerUuid, CVmDirectoryItem* pDirItem);

private:
	/** #9246 */
	PRL_RESULT saveSecureData( SmartPtr<CDspClient> pUserSession
		, SmartPtr<CVmConfiguration> vmConfig, CVmEvent& errorEvent );

	/** #9246 */
	void removeSecureData( const QString& vm_uuid );

	bool sendVmConfigByUuid ( const IOSender::Handle& sender,
		SmartPtr<CDspClient> pUserSession,
		const SmartPtr<IOPackage>&,
		QString vm_uuid );
protected:
	// Get VM configuration for specific Directory item
	static SmartPtr<CVmConfiguration> getVmConfigForDirectoryItem(CVmDirectoryItem* pDirectoryItem
		,PRL_RESULT& error, bool bAbsolute = true, bool bLoadConfigDirectlyFromDisk= false );

private:
	enum ItemParamType {paramIsVmUuid, paramIsVmHome, paramIsVmName};

	/**
	 * Get VM configuration for specific Directory item
	 * @param pointer to user session object
	 * @param pointer to VM directory item object
	 * @param error code (out parameter - error code will be stored in it if operation'll fail)
	 * @param pointer to additional error info (necessary error message parameters such as VM name
	 *			and etc. will be stored here if operation'll fail)
	 */
	static SmartPtr<CVmConfiguration> getVmConfigForDirectoryItem (
		SmartPtr<CDspClient>,
		CVmDirectoryItem* ,
		PRL_RESULT&	outError,
		CVmEvent *pErrorInfo = NULL );

	// Internal implementation of VM Directory accessor
	CVmDirectory* getVmDirectory ( CDispUser*, CAuthHelper* p_authHelper);

/*
	// Get VM directory item by param
	CVmDirectoryItem* getVmDirectoryItemByParam ( SmartPtr<CDspClient>,
		ItemParamType paramType,
		const QString& paramValue);
*/



	/**
	* Return path to Vm Home of user.
	**/
	static QString getVmRootPathForUser( const QString& dirUuid, const QString& userSettingsUuid );

	// update hard disk information
public:
	static PRL_RESULT UpdateHardDiskInformation(QList<CVmHardDisk*>& lstHdd, SmartPtr<CDspClient> pUserSession);

	static void sendVmConfigChangedEvent(const CVmIdent& vmIdent
		, const SmartPtr<IOPackage> &pRequest = SmartPtr<IOPackage>(0));

	static void sendVmConfigChangedEvent(const QString& vmDirUuid, const QString& vmUuid
		, const SmartPtr<IOPackage> &pRequest = SmartPtr<IOPackage>(0));

private:
	/**
	* Fill Vm Security
	* @return SmartPtr<CVmSecurity>
	* @param  pUserSession  user session object
	* @param  pVmDirItem
	*/
	static SmartPtr<CVmSecurity> FillVmSecurity(
		SmartPtr<CDspClient> pUserSession,
		CVmDirectoryItem* pVmDirItem );

	/**
	* @brief fill VM info event.
	* @param pUserSession
	* @param pVmSecurity
	* @param outVmEvent
	* @return
	*/
	static PRL_RESULT fillVmInfo(
		SmartPtr<CDspClient> pUserSession
		, const QString& vm_uuid
		, CVmSecurity* pVmSecurity
		, CVmEvent& outVmEvent );


	/**
	* @brief fill VM Sate event.
	* @param pUserSession
	* @param sVmUuid
	* @param outVmEvent
	* @return
	*/
	static void fillVmState(
		SmartPtr<CDspClient> pUserSession
		, const QString& sVmUuid
		, CVmEvent& outVmEvent );

	/**
	* Update Vm Security
	* @return PRL_RESULT
	* @param  pUserSession  user session object
	* @param  pVmDirItem
	* @param  pVmSecurity
	*/
	PRL_RESULT updateVmSecurityInfo ( SmartPtr<CDspClient> pUserSession
		, const CVmDirectoryItem* pVmDirItem
		, SmartPtr<CVmSecurity> pVmSecurity );

	/**
	* Generate "VM security changed" event for all active sessions
	* @param vm dir uuid
	* @param vm uuid
	* @param pointer to the request package initiating an event
	*/
	static void sendEventVmSecurityChanged(
		const QString& VmDirUuid,
		const QString& vmUuid,
		const SmartPtr<IOPackage> &pRequest = SmartPtr<IOPackage>(0) );

	static PRL_RESULT saveFastRebootData( const QString &vmUuid, const QString &user );

	static PRL_RESULT loadFastRebootData( const QString &vmUuid, QString &user );

private:
	class ExclusiveVmOperations
	{
	public:
		ExclusiveVmOperations();

		//TODO: add lockId to safe programming
		PRL_RESULT	registerOp( const QString& vmUuid, const QString& vmDirUuid, PVE::IDispatcherCommands cmd,
				const IOSender::Handle &hSession, const QString &sTaskId );
		PRL_RESULT	unregisterOp( const QString& vmUuid, const QString& vmDirUuid, PVE::IDispatcherCommands cmd, const IOSender::Handle &hSession );
		PRL_RESULT	replaceOp( const QString& vmUuid, const QString& vmDirUuid, PVE::IDispatcherCommands fromCmd, PVE::IDispatcherCommands toCmd, const IOSender::Handle &hSession );
		IOSender::Handle getVmLockerHandle( const QString& vmUuid, const QString& vmDirUuid ) const;
		QList<QString> getTasksUnderExclusiveOperation( const CVmIdent &vmIden ) const;

		/**
		 * Cleanups all exclusive VM locks that belongs to specified session
		 * handle
		 * @param session handle
		 */
		void cleanupSessionVmLocks( const IOSender::Handle &hSession );


	private:
		struct OperationData
		{
			IOSender::Handle hSession;
			QString taskId;

			OperationData(IOSender::Handle hSession_, const QString &taskId_) :
				hSession(hSession_), taskId(taskId_)
			{}
		};

		typedef QMultiHash< PVE::IDispatcherCommands, OperationData> ExecutedHash;
		struct Op
		{
			ExecutedHash hashExecuted;
			QSharedPointer<QWaitCondition> event;

			Op( PVE::IDispatcherCommands cmd_, const IOSender::Handle &hSession, const QString &taskId_):
				event(new QWaitCondition())
			{
				hashExecuted.insert( cmd_, OperationData(hSession, taskId_));
			}
		};

	private:
		/* @brief: return key makes from vmUuid and vmDirUuid*/
		QString makeKey( const QString& vmUuid, const QString& vmDirUuid ) const;

		/* @brief: get error code by executed cmd number */
		PRL_RESULT getFailureCode( PVE::IDispatcherCommands executingCmd );

	private:
		mutable QMutex m_mutex;
		QHash< QString, Op > m_opHash;
	};

	SmartPtr<CVmConfiguration> CreateDefaultVmConfigByRcValid(
		SmartPtr<CDspClient> pUserSession, PRL_RESULT rc, const QString& vmUuid);

	/**
	* @brief Gets vmDirUuid by vm_uuid
	* @param vm_uuid
	*/
	static QList<QString> getVmDirUuidByVmUuid( const QString& vm_uuid );

private:

	CMultiEditMergeVmConfig* m_pVmConfigEdit;
	ExclusiveVmOperations	m_exclusiveVmOperations;
	SmartPtr<CDspVmMountRegistry> m_vmMountRegistry;

	friend class Task_EditVm;
	friend class Task_RegisterVm;

	static QSet<int> g_remoteDisplayPorts;

}; // class CDspVmDirHelper


#endif // __CDspVmDirHelper_H_
