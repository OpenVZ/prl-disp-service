TEMPLATE = app
CONFIG += qt

include(Enc_test.deps)

QT += core
QT -= gui

HEADER += Parser.h

SOURCES +=	Enc_test.cpp \
		Parser.cpp

# It is important to have "File Info" embedded in the
# windows binaries - which means we need windows resource file
win32:RC_FILE = $$SRC_LEVEL/Tests/UnitTests.rc

macx {
	CONFIG += x86 x86_64
}
