CONFIG += qtestlib
QT += core xml network

include(test.deps)

HEADERS +=	../PrlAutoReportsTest.h \
						../SimpleServerWrapper.h \
						$$SRC_LEVEL/Tests/CommonTestsUtils.h \
						$$SRC_LEVEL/Tests/AclTestsUtils.h \
						../AutoHelpers.h

SOURCES +=	../PrlAutoReportsTest.cpp \
						Main.cpp \
						../SimpleServerWrapper.cpp \
						$$SRC_LEVEL/Tests/CommonTestsUtils.cpp \
						../AutoHelpers.cpp

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
