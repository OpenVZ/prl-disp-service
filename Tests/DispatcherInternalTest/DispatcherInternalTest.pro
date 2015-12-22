CONFIG += qtestlib
QT = xml network core

INCLUDEPATH += /usr/share /usr/include/prlsdk
include(DispatcherInternalTest.deps)

HEADERS += \
	CFileHelperTest.h\
	CAuthHelperTest.h\
	$$SRC_LEVEL/Dispatcher/Dispatcher/Stat/CDspStatisticsGuard.h\
	$$SRC_LEVEL/Dispatcher/Dispatcher/Stat/CDspSystemInfo.h\
	$$SRC_LEVEL/Tests/CommonTestsUtils.h\
	$$SRC_LEVEL/Tests/AclTestsUtils.h\
	CDspStatisticsGuardTest.h\
	PrlCommonUtilsTest.h \
	CAclHelperTest.h \
	CGuestOsesHelperTest.h \
	CProblemReportUtilsTest.h \
	CXmlModelHelperTest.h \
	CFeaturesMatrixTest.h \

SOURCES += \
	CFileHelperTest.cpp\
	CAuthHelperTest.cpp\
	Main.cpp\
	$$SRC_LEVEL/Dispatcher/Dispatcher/Stat/CDspStatisticsGuard.cpp\
	$$SRC_LEVEL/Tests/CommonTestsUtils.cpp\
	CDspStatisticsGuardTest.cpp\
	PrlCommonUtilsTest.cpp \
	CAclHelperTest.cpp \
	CGuestOsesHelperTest.cpp \
	CProblemReportUtilsTest.cpp \
	CXmlModelHelperTest.cpp \
	CFeaturesMatrixTest.cpp \


win32: SOURCES	+= $$SRC_LEVEL/Dispatcher/Dispatcher/Stat/CDspSystemInfo_win.cpp
linux-*: SOURCES+= $$SRC_LEVEL/Dispatcher/Dispatcher/Stat/CDspSystemInfo_lin.cpp
macx:	SOURCES	+= $$SRC_LEVEL/Dispatcher/Dispatcher/Stat/CDspSystemInfo_mac.cpp


LIBS += -lprlcommon -lboost_filesystem-mt -lboost_system-mt

win32 {
	HEADERS += CWifiHelperTest.h
	SOURCES += CWifiHelperTest.cpp

	qt_shared {
		QMAKE_LFLAGS += /NODEFAULTLIB:libcmt /NODEFAULTLIB:libcmtd
	}

}

macx {
	# include($$LEVEL/Build/MacUtil/Frameworks.pri)
	LIBS += \
		-framework Security
}

# It is important to have "File Info" embedded in the
# windows binaries - which means we need windows resource file
win32: RC_FILE = $$SRC_LEVEL/Tests/UnitTests.rc
