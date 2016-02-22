TEMPLATE = app
CONFIG += console qt warn_on thread

contains(USE_PRECOMPILED_HEADERS, 1) {
	win32:PRECOMPILED_HEADER = Precomp.h
}

QT += network core xml
QT -= gui

linux-g++* {
	system(g++ -dumpversion | (echo 4.8 && cat) | sort -V | head -n1 | grep -F 4.8) {
		message( libvirt )
		CONFIG += libvirt
	}
}

include(Dispatcher.deps)

DEFINES += PRINTABLE_TARGET=disp
DEFINES += BOOST_MPL_CFG_NO_PREPROCESSED_HEADERS BOOST_MPL_LIMIT_VECTOR_SIZE=40

INCLUDEPATH *= \
	Dispatcher \
	LostConfigSearcher \
	$$SRC_LEVEL/VI/Interfaces \
	$$SRC_LEVEL/Dispatcher/Libraries \
	$$EXT_LEVEL/Virtuozzo/include

VPATH = Dispatcher
HEADERS += \
	CDspCommon.h \
	CVmIdent.h \
	CDspAccessManager.h \
	CDspBroadcastListener.h \
	CDspClient.h \
	CDspClientManager.h \
	CDspDispConfigGuard.h \
	CDspHandlerRegistrator.h \
	CDspIOClientHandler.h \
	CDspIOCtClientHandler.h \
	CDspRouter.h \
	CDspService.h \
	CDspShellHelper.h \
	CDspStarter.h \
	CDspSync.h \
	CDspTaskHelper.h \
	CDspTaskManager.h \
	CDspUserHelper.h \
	CDspVNCStarter.h \
	CDspVm.h \
	CDspDispConnection.h \
	CDspVmDirManager.h \
	CDspVmDirHelper.h \
	CDspVmManager.h \
	CDspVmManager_p.h \
	CDspVmMounter.h \
	CDspVmGuestPersonality.h \
	CDspVmNetworkHelper.h \
	CDspDispConnectionsManager.h \
	CFileHelperDepPart.h \
	CVmValidateConfig.h \
	DspLogicGuard.h \
	DspMonitor.h \
	CDspRequestsToVmHandler.h \
	CDspVmSnapshotStoreHelper.h \
	CDspVmMigrateHelper.h \
	CDspVmConfigurationChangesWatcher.h \
	CDspVmConfigManager.h \
	CDspVmStateMachine.h \
	CDspVmAutoTaskManagerBase.h \
	CDspVmStateSender.h \
	CDspTestConfig.h \
	CDspBackupHelper.h \
	CDspBugPatcherLogic.h \
	CDspProblemReportHelper.h \
	CDspVmInfoDatabase.h \
	CDspVmSuspendHelper.h \
    CDspHostSettingsHelper.h \
	CDspHaClusterHelper.h \
	CDspVm_p.h \
	CDspVmSnapshotInfrastructure.h \
	CDspVmBackupInfrastructure_p.h \
	CDspVmBackupInfrastructure.h \
	CDspSettingsWrap.h \
	CDspAsyncRequest.h \
	CVcmmdInterface.h \
	\
	EditHelpers/CMultiEditDispatcher.h \
	EditHelpers/CMultiEditMergeHelper.h \
	EditHelpers/CMultiEditMergeVmConfig.h \
	EditHelpers/CMultiEditMergeDispConfig.h \
	\
	Stat/CDspStatCollectingThread.h \
	Stat/CDspStatCollector.h \
	Stat/CDspStatisticsGuard.h \
	Stat/CDspSystemInfo.h \
	\
	Tasks/Task_AutoStart.h \
	Tasks/Mixin_CreateHddSupport.h \
	Tasks/Mixin_CreateVmSupport.h \
	Tasks/Task_CloneVm.h \
	Tasks/Task_CloneVm_p.h \
	Tasks/Task_MoveVm.h \
	Tasks/Task_CommonHeaders.h \
	Tasks/Task_CreateImage.h \
	Tasks/Task_CopyImage.h \
	Tasks/Task_DeleteVm.h \
	Tasks/Task_FileSystemEntriesOperations.h \
	Tasks/Task_GetFileSystemEntries.h \
	Tasks/Task_GetInfoFromParallelsUtils.h \
	Tasks/Task_ManagePrlNetService.h \
	Tasks/Task_RegisterVm.h \
	Tasks/Task_SearchLostConfigs.h \
	Tasks/Task_UpdateCommonPrefs.h \
	Tasks/Task_CreateSnapshot.h \
	Tasks/Task_SwitchToSnapshot.h \
	Tasks/Task_DeleteSnapshot.h \
	Tasks/Task_BackgroundJob.h \
	Tasks/Task_ConfigureGenericPci.h \
	Tasks/Task_PrepareForHibernate.h \
	Tasks/Task_CommitUnfinishedDiskOp.h \
	Tasks/Task_CreateVmBackup.h \
	Tasks/Task_RestoreVmBackup.h \
	Tasks/Task_RestoreVmBackup_p.h \
	Tasks/Task_GetBackupTree.h \
	Tasks/Task_RemoveVmBackup.h \
	Tasks/Task_BackupHelper.h \
	Tasks/Task_DiskImageResizer.h \
	Tasks/Task_SetConfirmationMode.h \
	Tasks/Task_ChangeSID.h \
	Tasks/Task_CreateProblemReport.h \
	Tasks/Task_SyncVmsUptime.h \
	Tasks/Task_MigrateVm.h \
	Tasks/Task_MigrateVmTarget.h \
	Tasks/Task_MigrateVmTarget_p.h \
	Tasks/Task_DispToDispConnHelper.h \
	Tasks/Task_ConvertDisks.h \
	Tasks/Task_EditVm.h \
	Tasks/Task_EditVm_p.h \
	Tasks/Task_VmDataStatistic.h \
	Tasks/Task_EventLoopBase.h \
	Tasks/Task_MigrateVm_p.h \
	Tasks/Task_MigrateVmSource_p.h \
	\
	HwMonitor/CDspHwMonitorNotifier.h \
	HwMonitor/CDspHwMonitorHandler.h \
	HwMonitor/CDspHwMonitorThread.h \
	HwMonitor/CDspSystemEventsMonitor.h \
	\
	Guards/Server_Sentry.h \
	\
	Cache/Cache.h \
	Cache/CacheImpl.h \
	Cache/CacheImpl.cpp

SOURCES += \
	RoutesTable.cpp \
	CDspAccessManager.cpp \
	CDspBroadcastListener.cpp \
	CDspClient.cpp \
	CDspClientManager.cpp \
	CDspDispConfigGuard.cpp \
	CDspHandlerRegistrator.cpp \
	CDspIOClientHandler.cpp \
	CDspIOCtClientHandler.cpp \
	CDspRouter.cpp \
	CDspService.cpp \
	CDspShellHelper.cpp \
	CDspStarter.cpp \
	CDspTaskHelper.cpp \
	CDspTaskManager.cpp \
	CDspUserHelper.cpp \
	CDspVNCStarter.cpp \
	CDspVm.cpp \
	CDspDispConnection.cpp \
	CDspVmDirManager.cpp \
	CDspVmDirHelper.cpp \
	CDspVmManager.cpp \
	CDspVmMounter.cpp \
	CDspVmGuestPersonality.cpp \
	CDspVmNetworkHelper.cpp \
	CDspDispConnectionsManager.cpp \
	CFileHelperDepPart.cpp \
	CVmValidateConfig.cpp \
	DspLogicGuard.cpp \
	DspMonitor.cpp \
	CDspRequestsToVmHandler.cpp \
	CDspVmSnapshotStoreHelper.cpp \
	CDspVmMigrateHelper.cpp\
	CDspVmConfigurationChangesWatcher.cpp \
	CDspVmConfigManager.cpp \
	CDspVmStateMachine.cpp \
	CDspVmAutoTaskManagerBase.cpp \
	CDspVmStateSender.cpp \
	CDspTestConfig.cpp \
	CDspBackupHelper.cpp \
	CDspBugPatcherLogic.cpp \
	CDspProblemReportHelper.cpp \
	CDspVmInfoDatabase.cpp \
	CDspVmSuspendHelper.cpp \
    CDspHostSettingsHelper.cpp \
	CDspHaClusterHelper.cpp \
	CDspVm_p.cpp \
	CDspVmSnapshotInfrastructure.cpp \
	CDspVmBackupInfrastructure.cpp \
	CDspSettingsWrap.cpp \
	CDspAsyncRequest.cpp \
	CVcmmdInterface.cpp \
	\
	EditHelpers/CMultiEditDispatcher.cpp \
	EditHelpers/CMultiEditMergeVmConfig.cpp \
	EditHelpers/CMultiEditMergeDispConfig.cpp \
	\
	Stat/CDspStatCollectingThread.cpp \
	Stat/CDspStatCollector_common.cpp \
	Stat/CDspStatisticsGuard.cpp \
	\
	Tasks/Task_AutoStart.cpp \
	Tasks/Mixin_CreateHddSupport.cpp \
	Tasks/Mixin_CreateVmSupport.cpp \
	Tasks/Task_CloneVm.cpp \
	Tasks/Task_MoveVm.cpp \
	Tasks/Task_CreateImage.cpp \
	Tasks/Task_CopyImage.cpp \
	Tasks/Task_DeleteVm.cpp \
	Tasks/Task_FileSystemEntriesOperations.cpp \
	Tasks/Task_GetFileSystemEntries.cpp \
	Tasks/Task_GetInfoFromParallelsUtils.cpp \
	Tasks/Task_ManagePrlNetService.cpp \
	Tasks/Task_RegisterVm.cpp \
	Tasks/Task_SearchLostConfigs.cpp \
	Tasks/Task_UpdateCommonPrefs.cpp \
	Tasks/Task_CreateSnapshot.cpp \
	Tasks/Task_SwitchToSnapshot.cpp \
	Tasks/Task_DeleteSnapshot.cpp \
	Tasks/Task_BackgroundJob.cpp \
	Tasks/Task_ConfigureGenericPci.cpp \
	Tasks/Task_PrepareForHibernate.cpp \
	Tasks/Task_CommitUnfinishedDiskOp.cpp \
	Tasks/Task_CreateVmBackup.cpp \
	Tasks/Task_RestoreVmBackup.cpp \
	Tasks/Task_RemoveVmBackup.cpp \
	Tasks/Task_GetBackupTree.cpp \
	Tasks/Task_BackupHelper.cpp \
	Tasks/Task_DiskImageResizer.cpp \
	Tasks/Task_SetConfirmationMode.cpp \
	Tasks/Task_ChangeSID.cpp \
	Tasks/Task_CreateProblemReport.cpp \
	Tasks/Task_SyncVmsUptime.cpp \
	Tasks/Task_MigrateVm.cpp \
	Tasks/Task_MigrateVmTarget.cpp \
	Tasks/Task_DispToDispConnHelper.cpp \
	Tasks/Task_ConvertDisks.cpp \
	Tasks/Task_EditVm.cpp \
	Tasks/Task_VmDataStatistic.cpp \
	Tasks/Task_EventLoopBase.cpp \
	\
	HwMonitor/CDspHwMonitorNotifier.cpp \
	HwMonitor/CDspHwMonitorThread.cpp  \
	HwMonitor/CDspSystemEventsMonitor.cpp \
	\
	Guards/Server_Sentry.cpp \
	\

linux-* {
	INCLUDEPATH *= \
		$$SRC_LEVEL/VI/Sources/Explorer/Includes

	HEADERS += \
		CDspBackupDevice.h \
		Tasks/Task_AttachVmBackup_p.h \
		Tasks/Task_AttachVmBackup.h \
		Tasks/Task_MountVm.h

	SOURCES += \
		CDspBackupDevice.cpp \
		Stat/CDspStatCollector_lin.cpp\
		Stat/CDspSystemInfo_lin.cpp \
		\
		Tasks/Task_AttachVmBackup.cpp \
		Tasks/Task_MountVm.cpp \
		\
		HwMonitor/CDspHwMonitorHandler_lin.cpp

	LIBS += -lsystemd
}

contains(DEFINES, _CT_) {
	HEADERS += \
		CDspVzHelper.h \
		Tasks/Task_VzManager.h \
		Tasks/Task_VzStateMonitor.h \

	SOURCES += \
		CDspVzHelper.cpp \
		Tasks/Task_VzManager.cpp \
		Tasks/Task_VzStateMonitor.cpp \

	linux-* {
		HEADERS += \
			Tasks/Task_VzMigrate.h \
			Tasks/Task_MigrateCtSource.h \
			Tasks/Task_MigrateCtTarget.h \
			Tasks/Task_CopyCtTemplate.h \
			Tasks/Task_ExecVm.h

		SOURCES += \
			Tasks/Task_VzMigrate.cpp \
			Tasks/Task_MigrateCtSource.cpp \
			Tasks/Task_MigrateCtTarget.cpp \
			Tasks/Task_CopyCtTemplate.cpp \
			Tasks/Task_ExecVm.cpp
	}

	win32 {
		HEADERS += \
			Tasks/Task_CopyCtTemplate_win.h \
			Tasks/Task_MigrateCt_win.h

		SOURCES += \
			Tasks/Task_CopyCtTemplate_win.cpp \
			Tasks/Task_MigrateCt_win.cpp
	}
}

libvirt {
	HEADERS += \
		CDspLibvirt.h \
		CDspLibvirtExec.h \
		CDspLibvirt_p.h

	SOURCES += \
		CDspLibvirt.cpp \
		CDspLibvirtAgent.cpp

	DEFINES += _LIBVIRT_=1
}
