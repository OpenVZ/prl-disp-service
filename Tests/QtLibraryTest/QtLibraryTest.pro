CONFIG += qtestlib
QT = xml network core

include(QtLibraryTest.deps)

HEADERS += QtCoreTest.h \
	$$SRC_LEVEL/Tests/CommonTestsUtils.h

SOURCES += Main.cpp \
		QtCoreTest.cpp \
	       $$SRC_LEVEL/Tests/CommonTestsUtils.cpp



# It is important to have "File Info" embedded in the
# windows binaries - which means we need windows resource file
win32:RC_FILE = $$SRC_LEVEL/Tests/UnitTests.rc
