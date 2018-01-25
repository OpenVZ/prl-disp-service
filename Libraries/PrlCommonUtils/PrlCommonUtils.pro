TARGET = prl_common_utils
TEMPLATE = lib
CONFIG += staticlib

DEFINES += PRINTABLE_TARGET=cmn_utils

include(PrlCommonUtils.pri)

HEADERS = MetaObjectUtils.h


SOURCES = MetaObjectUtils.cpp


	HEADERS += \
			CVmQuestionHelper.h \
			CAuthHelper.h \
			CFileHelper.h \
			PrlQSettings.h \
			CFirewallHelper.h \
			UserFolderDefs.h \
			UserFolder.h \
			CVmMigrateHelper.h

	SOURCES += \
			CVmQuestionHelper.cpp \
			CAuthHelper.cpp \
			CFileHelper.cpp \
			PrlQSettings.cpp \
			CFirewallHelper.cpp \
			CVmMigrateHelper.cpp \
			UserFolder.cpp


	win32 {
	HEADERS +=			\
		UacActions.h		\
		EfsHelper.h		\
			ServiceManager.h \
		OsInfo_Wmi.h

	SOURCES +=			\
		UacActions.cpp		\
		EfsHelper.cpp		\
			ServiceManager.cpp \
		OsInfo_Wmi.cpp

	}

	unix {
		HEADERS += CUnixSignalHandler.h
		SOURCES += CUnixSignalHandler.cpp
	}

