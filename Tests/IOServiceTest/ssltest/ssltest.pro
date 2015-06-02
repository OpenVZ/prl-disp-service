TEMPLATE = app
CONFIG += qtestlib warn_on
QT = core network

include(ssltest.deps)

HEADERS += SslTest.h
SOURCES += SslTest.cpp

# It is important to have "File Info" embedded in the
# windows binaries - which means we need windows resource file
win32:RC_FILE = $$SRC_LEVEL/Tests/UnitTests.rc
