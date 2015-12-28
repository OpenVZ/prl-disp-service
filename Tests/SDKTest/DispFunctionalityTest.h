/////////////////////////////////////////////////////////////////////////////
///
/// Copyright (C) 2006-2015 Parallels IP Holdings GmbH
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
///		DispFunctionalityTest.h
///
/// @author
///		sergeyt
///
/// @brief
///		Test dispatcher common functionality.
///
/// @brief
///		None.
///
/////////////////////////////////////////////////////////////////////////////
#ifndef H_DispFunctionalityTest_H
#define H_DispFunctionalityTest_H

#include <QObject>
#include <prlcommon/Std/SmartPtr.h>
#include "SDK/Wrappers/SdkWrap/SdkHandleWrap.h"
#include "Tests/PrlPluginExample/PluginUuid.h"
#include "Tests/PrlPluginExample2/PluginUuid.h"


class ICheckPatchBase;

class DispFunctionalityTest: public QObject
{

   Q_OBJECT
public:
   DispFunctionalityTest();
   ~DispFunctionalityTest();

private slots:
	void init();
	void cleanup();
// private:
	void test_login();

	void test_bug117830_SaveAutoStartParams();

	void test_RegisterVmWhenVmWasUnregisteredOnSameServer();

	// https://bugzilla.sw.ru/show_bug.cgi?id=126119
	void test_bug126119_RegisterAlreadyRegistredVm();
	void test_bug126119_CreateAlreadyRegistredVm();

	// https://bugzilla.sw.ru/show_bug.cgi?id=127473
	void test_bug127473_DenyToCreateUserDefinedVmDir_OnLogin();
	void test_bug127473_AllowToCreateDefaultVmDir_OnLogin();

	void test_bug127473_DenyToCreateVmInUnexistingUserDefinedVmDir();
	void test_bug127473_DenyToCloneVmToUnexistingUserDefinedVmDir();

	void test_bug127473_DenyToCreateVmInUnexistingDir();
	void test_bug127473_DenyToCloneVmToUnexistingDir();
	void test_VmUptime();

	// https://bugzilla.sw.ru/show_bug.cgi?id=424340
	// https://svn.parallels.com/viewvc?view=rev&revision=352888
	void testCommonPrefsSetReadOnlyValues();

	// https://bugzilla.sw.ru/show_bug.cgi?id=430586
	void testRegenerateVmUuidOnRegisterVm();

	void testRegenerateVmSrcUuidOnRegisterVm();

	void testRegisterVmWithCustomUuid();

	// https://bugzilla.sw.ru/show_bug.cgi?id=433740
	void testUnableToRegisterSecondCopyOfCopiedAndRemovedVm();

	//https://bugzilla.sw.ru/show_bug.cgi?id=435473
	void testUnableToCreateVmIfUnexistingInvalidVmIsPresent();

	//https://bugzilla.sw.ru/show_bug.cgi?id=436109
	void testConfirm();

	// NOTE: Need test that confirmations havn't effect if confirmation is disabled by default.

	// set confirm mode
	void testSetConfirmMode_UnableToDisableWithWrongAdminCredentials();
	void testSetConfirmMode_UnableToDisableByNotAdmin();

	void testSetConfirmMode_AllowToEnableByNotAdmin();
	void testSetConfirmMode_UnableToAlreadyEnableOrDisable();

	// test confirm operations
	void testConfirm_UnableExecuteOperationWhich_BlockedInCommonList();
	void testConfirm_UnableExecuteOperationWhich_BlockedInPerVmList();
	void testConfirm_UnableExecuteOperationWhich_BlockedInCommonList_ButPermittedInPerVmList();

	// -- BEGIN of test atomic edit members suite
	//		PrlSrv_StoreValueByKey / PrlVm_StoreValueByKey
	void testStoreValueByKey_InstalledSoftwareId();
	void testStoreValueByKey_InstalledSoftwareIdOnWrongParams();

	// PrlVm_UpdateToolsSection testsuite
	void testAtomicEdit_EVT_PARAM_VMCFG_TOOLS_NATIVE_LOOK();


// FIXME	// PrlVm_UpdateToolsSection testsuite

	void testAtomicEdit_EVT_PARAM_VMCFG_DEVICE_CONFIG_WITH_NEW_STATE();

	void testAtomicEdit_EVT_PARAM_VMCFG_TOOLS_COHERENCE();
	void testAtomicEdit_EVT_PARAM_VMCFG_TOOLS_SHARED_FOLDERS();
	void testAtomicEdit_EVT_PARAM_VMCFG_TOOLS_SHARED_PROFILES();
	void testAtomicEdit_EVT_PARAM_VMCFG_TOOLS_SHARED_APPLICATIONS();

	void testRemoteDisplayPasswordLength();

	// -- END of test atomic edit members suite


	//////////////////////////////////////////////////////////////////////////
	// VM CONFIG MERGE TESTSUITE
	void testMergeVmConfig_MergeDifferentFields();
	void testMergeVmConfig_MergeWholeSection();

	//////////////////////////////////////////////////////////////////////////

	void testMergeVmConfig_byAtomicChangeBetween();
	void testMergeVmConfig_byAtomicChangeBetween_Conflict();

	// we have next test matrix with atomic commits:
	// (A) - commits in another sessions
	// (B) - commit in this session ( with merging attempt )
	//  In this case matrix values should not be intersected.
	//  __________________________________________________________
	// |                 | (B) atomic only -- |    (B) !atomic    |
	// |________________ |  _____________________________________ |
	// |(A) atomic only: |       MERGE        |     MERGE         |
	// |_______________  |  _____________(11)_|______________(12)_|
	// |(A) not atomic:  |       MERGE        |     MERGE      |
	// |_________________|_______________(21)_|______________(22)_|
	//

	void testMergeVmConfig_case11_AtomicAndAtomic_MERGE();
	void testMergeVmConfig_case12_AtomicAndNotAtomic_MERGE();
	void testMergeVmConfig_case21_NotAtomicAndAtomic_MERGE();
	void testMergeVmConfig_case22_NotAtomicAndNotAtomic();

	void testMergeVmConfig_case11_AtomicAndAtomic_TheSame_CONFLICT();

	// TODO: special test for EVT_PARAM_VM_UPTIME_DELTA

	// https://bugzilla.sw.ru/show_bug.cgi?id=438525
	// https://bugzilla.sw.ru/show_bug.cgi?id=436867
	void testUnableToRegisterSameVm_WithRegenerateVmUuidFlag();

	// https://bugzilla.sw.ru/show_bug.cgi?id=440983
	void testRegisterVm_WithSameNameAndSameUuid_ShouldRenameBundleToo();

	// https://bugzilla.sw.ru/show_bug.cgi?id=445239
	// 	[[config watcher] On GUI side do not refresh VM config
	// 	when user change config.pvs by externaly tools ( TextEdit / Vim / etc.)]
	// https://bugzilla.sw.ru/show_bug.cgi?id=444190
	// 	[[Sentillion] Unable to rename folder that contains VM folder ]
	void testConfigWatcher_FileWasChanged();
	void testConfigWatcher_FileWasChangedThroughReplace();
	void testConfigWatcher_FileWasMoved();
	void testConfigWatcher_FileWasRemoved();
	void testConfigWatcher_VmDirWasMoved();
	void testConfigWatcher_VmDirWasDeleted();
	void testConfigWatcher_VmDirWasChanged();
	void testConfigWatcher_AfterEditVm();
	void testConfigWatcher_AfterAddNewVm();
	void testConfigWatcher_AfterDeleteVm();

	// https://bugzilla.sw.ru/show_bug.cgi?id=456470
	void testLostVmPermissionAfterStartByAdmin();

// private slots:
	void testVmEncryption_RegisterEncryptedVm(); // FIXME: TODO

	void testVmEncryption_testVmEncrypt();
	void testVmEncryption_testVmDecrypt();

	void testVmEncryption_testAlreadyEncryptVm();
	void testVmEncryption_testAlreadyDecryptVm();

	void testVmEncryption_testVmDecryptFromAnotherSession();
	void testVmEncryption_testChangeVmPassword();
	void testVmEncryption_testStartVm();
	void testVmEncryption_testStartVm_WithMemFiles();

	// # https://jira.sw.ru:9443/browse/PDFM-1404
	void testVmEncryption_testCancelOnDisconnect();
	void testVmEncryption_testNoCancelOnDisconnect_WithoutFlag();

	// # https://jira.sw.ru/browse/PDFM-1515
	void testVmEncryption_testFlag_CheckFreesizeOnly();

	// TESTS checkPreconditions
	//		https://jira.sw.ru:9443/browse/PDFM-24686
	void testEncryption_DecryptVmWithHdd_AttachedToAnotherVm_AndChangedPwd();
	void testEncryption_DecryptVmWithHdd_AttachedToAnotherVm_AndDecrypted();
	void testEncryption_DecryptVmWithHdd_AttachedToAnotherVm_AndEncryptedWithPlugin2();
	void testEncryption_EncryptVmWithHdd_AttachedToAnotherVm_AndEncrypted();
	// END of TESTS checkPreconditions

// private slots:
	void testEncryptionPluginsEnable();
	void testEncryptionPluginsEnable_withNotAdmin();
	void testEncryptionPluginsEnable_authVm();
	void testEncryptionPluginsEnable_LoadTwoPlugins();
	void testEncryptionPluginsEnable_LoadTwoPluginsWithSameId();
	void testEncryptionPluginsDisable();
	void testEncryptionPluginsDisable_withNotAdmin();
	void testEncryptionPluginsDisable_withRunningVm();
	void testEncryptionPluginsDisable_withEncryptOperation();
	void testEncryptionPluginsDisable_tryAuthToVm();
	void testEncryptionPluginsDisable_testThatDefaultPluginIsEmpty();
	void testEncryptionPluginsDisable_EncryptVmWithHdds_WithDifferentPluginsId();

	void testSetDefaultEncryptPlugin_testEncryptWithTestPluginByDefault();
	void testSetDefaultEncryptPlugin_whenPluginsAreDisabled();
	void testSetDefaultEncryptPlugin_whenPluginNotFound();
	void testSetDefaultEncryptPlugin_whenPluginIdHasWrongFormat();

	void testEncryptionPluginsRescan();
	void testEncryptionPluginsRescan_AddPlugin2AndRescan();
	void testEncryptionPluginsRescan_DeletePluginAndRescan();
	void testEncryptionPluginsRescan_withDisabled();
	void testEncryptionPluginsRescan_withNotAdmin();
	void testEncryptionPluginsRescan_withRunningVm();
	void testEncryptionPluginsRescan_withEncryptOperation();
	void testEncryptionPluginsRescan_LoadTwoPluginsWithSameId();
	void testEncryptionPluginsRescan_EncryptVmWithHdds_WithDifferentPluginsId(); // FIXME: TODO

	void testEncryptionGetPluginsList();
	void testEncryptionGetPluginsList_SpecialGuiCase();

// private:
	// https://bugzilla.sw.ru/show_bug.cgi?id=463792
	void testGetVmInfo();

	void testGetVmConfig_FromVmEditBeginResponse();

	void testGetVmConfig_FromVmEditCommitResponse();
	void testGetVmConfig_FromOldVmEditCommitResponse();

	void testLicenseInfo_FromUpdateLicenseResponse();
	void testLicenseInfo_FromUpdateLicenseResponseOld();

	// https://jira.sw.ru/browse/PSBM-6193
	void testCloneVm_withExternalHdd();

	void testGetServerAppMode_fromLoginResponse();
	void testGetServerAppMode_fromLoginResponseOnWrongParameters();

	void testCreateWrongHddImage_SizeIsZero();

	// task #PDFM-22954
	void testToSplitHdd_AfterResizeSplittedHddTo1Gb();

// private slots:
	// https://jira.sw.ru/browse/PDFM-26527
	//	[Shared WebCam doesn't install automatically on Windows 2000]
	void testPatchingSharedCameraEnabled_onRegistration();
	void testPatchingSharedCameraDisabled_onRegistration();

	// https://jira.sw.ru/browse/PDFM-26226
	void testPatchingCornerActions_onRegistration();

// private slots:
	//https://jira.sw.ru:9443/browse/PDFM-26697
	void testPatchingOptimizePowerConsumptionMode_onRegistration();

	void testSCSIBusLogicIncompatibleWithEfi_addBusLogicDiskWithEfiEnabled();
	void testSCSIBusLogicIncompatibleWithEfi_addBusLogicDiskWithEfiDisabled();
	void testSCSIBusLogicIncompatibleWithEfi_EnableEfiWithBusLogicDisk();
	void testSCSIBusLogicIncompatibleWithEfi_addLsiSpiDiskWithEfiEnabled();
	void testSCSIBusLogicIncompatibleWithEfi_addLsiSasDiskWithEfiEnabled();

// private slots:
    void testCreateVmAndSendProblemReport_OnBehalfOfVm_ToServer();

// private slots:

	void testRenameExternalDisksBundlesOnVmRename();
	void testKeepExternalDisksWithoutBundlesOnVmRename();
	void testDontRenameExternalDisksBundlesOnVmRenameIfFlagIsNotSet();
	void testFailOnVmRenameIfTargetExternalDiskBundleExitst();

public:
	enum CreateEncryptedFlags
	{ cemWithoutMemFiles = 0, cemMemFromSnapshots = 1, cemMemFromSuspend = 2, cemWithPlainHdd = 4 };

	// NOTE: SHOULD BE FLAGS!
	enum EncryptionPluginType
	{ eptDefault = 0, eptTestPlugin1 =2 , eptTestPlugin2 = 4 };
private:

	void createVm( const QString& sVmName
		, bool& bRes /*out*/, SdkHandleWrap& hVm/*out*/, QString& sVmUuid /*out*/ );
	static void createVm( const SdkHandleWrap& hServerHandle, const QString& sVmName
		, bool& bRes /*out*/, SdkHandleWrap& hVm/*out*/, QString& sVmUuid /*out*/ );


	void AddPlainHdd( const SdkHandleWrap& hVm, quint32 nHddSizeMb, bool& bRes /*out*/ );
	void AddExpandingHdd( const SdkHandleWrap& hVm, quint32 nHddSizeMb, bool& bRes /*out*/ );
	static void AddHdd( bool& bRes, const SdkHandleWrap& hVmHandle
		, quint32 nHddSizeMb, PRL_HARD_DISK_INTERNAL_FORMAT type
		, SdkHandleWrap& hSrvConfig, SdkHandleWrap& hOutHddHandle );
	void createExtDisk(const SdkHandleWrap& hVmHandle, SdkHandleWrap& hHddHandle, const QString& sHddPath, bool& bRes);

	// create encrypted vm with default parameters
	static void createEncryptedVm( bool& bRes /*out*/,  SdkHandleWrap& hVm/*out*/
		, const SdkHandleWrap& hServerHandle, const QString& sVmName
		, EncryptionPluginType pluginType = eptDefault
		, quint32 nFlags = (cemMemFromSnapshots | cemMemFromSuspend)
		);
	// FIXME: this function should be static
	void createEncryptedVm( const QString& sVmName, const QString& sPassword
		, quint32 createEncriptedFlags // from CreateEncryptedFlags
		, bool& bRes /*out*/, SdkHandleWrap& hVm/*out*/, QString& sVmUuid /*out*/ );
	static void createEncryptedVm( bool& bRes /*out*/
		, const SdkHandleWrap& hServerHandle
		, const QString& sVmName, const QString& sPassword
		, const QString& sPluginId
		, quint32 createEncriptedFlags // from CreateEncryptedFlags
		, SdkHandleWrap& hVm/*out*/, QString& sVmUuid /*out*/ );

	enum PluginsSwitchAction { psaEnable, psaDisable };
	static void switchPluginsOnOrOff( bool& bRes
		, const SdkHandleWrap& hServer, PluginsSwitchAction action, bool bRevertAlreadyState
		, PRL_RESULT nExpectedResult = PRL_ERR_SUCCESS
		, bool bSendTestFlags = true );

	static void loadTestPlugin( bool& bRes
		, const SdkHandleWrap& hServer
		, quint32 pluginsMask/* from EncryptionPluginType*/
		, bool bLoadCopyWithSameId = false );

	void checkEncryptFailure( bool& bRes
		, const SdkHandleWrap& hServer, const QString& sVmName, EncryptionPluginType pluginType
		, PRL_RESULT nExpectedError );

	// create vm, encrypt, decrypt, delete
	static void doEncryptDecryptPair( bool& bRes
		, const SdkHandleWrap& hServer, const QString& sVmName, EncryptionPluginType pluginType );

	enum SlowAction{ saDoEncrypt, saDoDecrypt };
	static void startSlowEncryptedOperation_andWaitFirstProgressEvent(bool& bRes
		, const SdkHandleWrap& hServer
		, const QString& sVmName
		, SdkHandleWrap& hOutVmHandle
		, SdkHandleWrap& hOutJobToCancel
		);
	static void startSlowEncryptedOperation_andWaitFirstProgressEvent(bool& bRes
		, SdkHandleWrap& hVmHandle // exist vm with plain hdd
		, SdkHandleWrap& hOutJobToCancel
		, bool bDoEncrypt );

	static void startSlowDecryptOperation_andWaitFirstProgressEvent(bool& bRes
		, const SdkHandleWrap& hServer
		, const QString& sVmName
		, SdkHandleWrap& hOutVmHandle
		, SdkHandleWrap& hOutJobToCancel
		);

	static void canUserChangeServerPrefs( bool& bRes
		, const SdkHandleWrap& hServer, PRL_BOOL& bUserCanChangeServerPrefs );

	static void setDefaultEncryptionPlugin(bool & bRes
		, const SdkHandleWrap& hServer, const QString& sPluginId
		, PRL_RESULT nExpectedResult = PRL_ERR_SUCCESS);

	static QString GetVmUuidByHandle( const SdkHandleWrap& hVmHandle );

	static void GetHddPathsByVmHandle( bool& bRes
		, const SdkHandleWrap& hVmHandle, PRL_HARD_DISK_INTERNAL_FORMAT type
		, QStringList& outLstPaths );

	static void GetHddListByVmHandle( bool& bRes
		, const SdkHandleWrap& hVmHandle
		, QList<SdkHandleWrap>& outHddList
		, bool bPatchToAbsPath = false
		, PRL_HARD_DISK_INTERNAL_FORMAT type = (PRL_HARD_DISK_INTERNAL_FORMAT)-1 /* -1 to return all*/
		, const QString& sHddPath = "" /* "" for all */ );

	static void addExistingHddToVm( bool& bRes
		, const SdkHandleWrap& hVmHandle
		, const SdkHandleWrap& hHddHandleToCopy
		, const SdkHandleWrap& hSrvConfig
		, SdkHandleWrap& hOutHddHandle
		, const QString& sHddPassword
		);

	static void createVmAndPrepareForEncrypt( bool& bRes /*out*/
		, const SdkHandleWrap& hServerHandle
		, const QString& sVmName
		, quint32 createEncriptedFlags
		, SdkHandleWrap& hVmHandle/*out*/
		, QString& sVmUuid /*out*/ );


	// try to determine adminstators credentials
	bool tryToGetAdministratorCredentials( QString& outAdminLogin, QString& outAdminPassword );

	static QString getPluginUuidByType( EncryptionPluginType pluginType );

	static void testXMLPatchingMech_onRegistration(
		bool& bRes, const QString& sVmName,  PRL_UINT32 osVersion, SmartPtr<ICheckPatchBase> pCheckObj );

private:
	SdkHandleWrap m_ServerHandle;
	SdkHandleWrap m_hSrvConfig;
	QString m_sServerUuid;
	SdkHandleWrap m_JobHandle;
	SdkHandleWrap m_ResultHandle;
	SdkHandleWrap m_VmHandle;
	QList<SdkHandleWrap> m_lstVmHandles;

	bool bParam;
};
typedef DispFunctionalityTest::EncryptionPluginType EncryptionPluginType;


class DefaultParams
{
public:
	static QString getTestPluginUuid(){ return g_sPluginExampleUuid; }
	static QString getTestPlugin2Uuid(){ return g_sPlugin2ExampleUuid; }

	static const char* getVmPassword() { return "1q2w3e"; }
	static QString getVmPasswordStr() { return getVmPassword(); }
	static PRL_UINT32 getEncryptionFlags()
		{ return DispFunctionalityTest::cemMemFromSnapshots
			| DispFunctionalityTest::cemMemFromSuspend;
		}
};

#endif //H_DispFunctionalityTest_H
