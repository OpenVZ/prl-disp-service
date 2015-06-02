CONFIG += qtestlib thread
QT += core
QT -= gui

TARGET = test_monitor_atomic_ops

LEVEL = ./../../../
include($$LEVEL/Sources/Parallels.pri)

HEADERS += $$LEVEL/Sources/Tests/MonitorAtomicOpsTest/MonitorAtomicOpsTest.h

SOURCES += $$LEVEL/Sources/Tests/MonitorAtomicOpsTest/MonitorAtomicOpsTest.cpp

# It is important to have "File Info" embedded in the
# windows binaries - which means we need windows resource file
win32:RC_FILE = $$LEVEL/Sources/Tests/UnitTests.rc
