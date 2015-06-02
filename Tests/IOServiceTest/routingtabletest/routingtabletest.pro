TEMPLATE = app
CONFIG += qtestlib console warn_on
QT = core network

include(routingtabletest.deps)

SOURCES += RoutingTableTest.cpp

# It is important to have "File Info" embedded in the
# windows binaries - which means we need windows resource file
win32:RC_FILE = $$SRC_LEVEL/Tests/UnitTests.rc
