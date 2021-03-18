CONFIG += qtestlib
QT = xml network core

INCLUDEPATH += /usr/share /usr/include/prlsdk
DEFINES += BOOST_MPL_CFG_NO_PREPROCESSED_HEADERS BOOST_MPL_LIMIT_VECTOR_SIZE=40 BOOST_SPIRIT_THREADSAFE
DEFINES += BOOST_THREAD_PROVIDES_FUTURE BOOST_THREAD_PROVIDES_FUTURE_CONTINUATION
include(DispatcherInternalTest.deps)

copydata.commands = $(COPY_DIR) $$PWD/TransponsterNwfilterTestFixtures $$PWD/../../z-Build/Debug
first.depends = $(first) copydata
export(first.depends)
export(copydata.commands)
QMAKE_EXTRA_TARGETS += first copydata

HEADERS += \
	$$SRC_LEVEL/Dispatcher/Dispatcher/Stat/CDspStatisticsGuard.h\
	$$SRC_LEVEL/Dispatcher/Dispatcher/Stat/CDspSystemInfo.h\
	$$SRC_LEVEL/Tests/DispatcherTestsUtils.h\
	$$SRC_LEVEL/Tests/AclTestsUtils.h\
	CDspStatisticsGuardTest.h\
	PrlCommonUtilsTest.h \
	CGuestOsesHelperTest.h \
	CProblemReportUtilsTest.h \
	CXmlModelHelperTest.h \
	CFeaturesMatrixTest.h \
	CTransponsterNwfilterTest.h

SOURCES += \
	Main.cpp\
	$$SRC_LEVEL/Dispatcher/Dispatcher/Stat/CDspStatisticsGuard.cpp\
	CDspStatisticsGuardTest.cpp\
	PrlCommonUtilsTest.cpp \
	CGuestOsesHelperTest.cpp \
	CProblemReportUtilsTest.cpp \
	CXmlModelHelperTest.cpp \
	CFeaturesMatrixTest.cpp \
	CTransponsterNwfilterTest.cpp


win32: SOURCES	+= $$SRC_LEVEL/Dispatcher/Dispatcher/Stat/CDspSystemInfo_win.cpp
linux-*: SOURCES+= $$SRC_LEVEL/Dispatcher/Dispatcher/Stat/CDspSystemInfo_lin.cpp
macx:	SOURCES	+= $$SRC_LEVEL/Dispatcher/Dispatcher/Stat/CDspSystemInfo_mac.cpp


linux-g++* {
	system(ld -lboost_thread-mt) {
		message( boost-with-mt )
		CONFIG += boost-with-mt
	} else {
		system(ld -lboost_thread) {
			message( boost-without-mt )
			CONFIG += boost-without-mt
		}
	}
}

LIBS += -L$$SRC_LEVEL/z-Build/Release -lprlcommon -lTransponster \ 
		-lPrlNetworking -lCpuFeatures -lStatesStore -llibprlTestsUtils

boost-with-mt {
	LIBS += -lboost_filesystem-mt -lboost_system-mt
}
boost-without-mt {
	LIBS += -lboost_filesystem -lboost_system
}

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
