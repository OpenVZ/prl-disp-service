CONFIG += qtestlib
QT += core xml gui network

include(SDKTest.deps)

HEADERS +=	HandlesManipulationsTest.h \
						Main.h\
						PrlSrvManipulationsTest.h \
						PrlVirtualNetworkTest.h \
						JobsManipulationsTest.h \
						PrlVmManipulationsTest.h \
						PrlVmValidateConfigTest.h \
						PrlVmDefaultConfigTest.h \
						PrlVmDevManipulationsTest.h \
						UserCallbackTest.h \
						PrivateSituationsTest.h \
						SimpleServerWrapper.h \
						PrlApiBasicsTest.h\
						DispFunctionalityTest.h \
						PrlAutoReportsTest.h \
						PrlVmExecFunctionalityTest.h \
						PrlExclusiveVmLockTest.h \
						$$SRC_LEVEL/Tests/CommonTestsUtils.h \
						$$SRC_LEVEL/Tests/AclTestsUtils.h \
						AutoHelpers.h \
						PrlVmUptimeTest.h \
						PrlNetworkShapingTest.h \
						PrlCtManipulationsTest.h \
						PrlSataDevicesHotPlugTest.h \
						PrlIPPrivateNetworkTest.h \
						PrlUsbDevicesHotPlugTest.h \
						PrlUserProfiletest.h \

SOURCES +=	HandlesManipulationsTest.cpp \
						PrlSrvManipulationsTest.cpp \
						PrlVirtualNetworkTest.cpp \
						Main.cpp \
						JobsManipulationsTest.cpp \
						PrlVmManipulationsTest.cpp \
						PrlVmValidateConfigTest.cpp \
						PrlVmDefaultConfigTest.cpp \
						PrlVmDevManipulationsTest.cpp \
						UserCallbackTest.cpp \
						PrivateSituationsTest.cpp \
						SimpleServerWrapper.cpp \
						PrlApiBasicsTest.cpp \
						DispFunctionalityTest.cpp \
						PrlAutoReportsTest.cpp \
						PrlVmExecFunctionalityTest.cpp \
						PrlExclusiveVmLockTest.cpp \
						$$SRC_LEVEL/Tests/CommonTestsUtils.cpp \
						AutoHelpers.cpp \
						PrlVmUptimeTest.cpp \
						PrlNetworkShapingTest.cpp \
						PrlCtManipulationsTest.cpp \
						PrlSataDevicesHotPlugTest.cpp \
						PrlIPPrivateNetworkTest.cpp \
						PrlUsbDevicesHotPlugTest.cpp \
						PrlUserProfiletest.cpp \

# It is important to have "File Info" embedded in the
# windows binaries - which means we need windows resource file
win32:RC_FILE = $$SRC_LEVEL/Tests/UnitTests.rc

# Workaround to eliminate tests suite compilation freeze
macx {
QMAKE_CXXFLAGS_RELEASE -= -Os
QMAKE_CFLAGS_RELEASE -= -Os

    LIBS += \
	-framework DirectoryService \
	-framework SystemConfiguration
}

linux-* {
QMAKE_CXXFLAGS_RELEASE -= -O2
QMAKE_CFLAGS_RELEASE -= -O2
}

SUBDIRS = HandlesManipulationsTest \
		PrlSrvManipulationsTest \
		PrlVirtualNetworkTest \
		JobsManipulationsTest \
		PrlVmManipulationsTest \
		PrlVmValidateConfigTest \
		PrlVmDefaultConfigTest \
		UserCallbackTest \
		PrlVmDevManipulationsTest \
		PrivateSituationsTest \
		PrlApiBasicsTest \
		PrlUserProfileTest \
		PrlVmExecFunctionalityTest \
		PrlExclusiveVmLockTest \
		PrlVmUptimeTest \
		PrlNetworkShapingTest \
		PrlCtManipulationsTest \
		PrlSataDevicesHotPlugTest \
		PrlUsbDevicesHotPlugTest \
		PrlIPPrivateNetworkTest \
		PrlAutoReportsTest \
		DispFunctionalityTest
