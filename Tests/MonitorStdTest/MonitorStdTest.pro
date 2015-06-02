CONFIG += qtestlib
QT += core
QT -= gui

TARGET = test_monitor_std

LEVEL = ./../../../
include($$LEVEL/Sources/Parallels.pri)

INCLUDEPATH += ./Monitor/Source/Core/ ./../../

HEADERS += $$LEVEL/Sources/Tests/MonitorStdTest/MonitorStdTest.h\
		   $$LEVEL/Sources/Tests/MonitorStdTest/Monitor.h\
		   $$LEVEL/Sources/Tests/MonitorStdTest/Monitor/Source/Core/MonitorBuffers.h

SOURCES += $$LEVEL/Sources/Tests/MonitorStdTest/MonitorStdTest.cpp\
		   $$LEVEL/Sources/Tests/MonitorStdTest/Monitor.cpp\
		   $$LEVEL/Sources/Monitor/Source/Std/Dmm.cpp\
		   $$LEVEL/Sources/Monitor/Source/Std/HashTree.cpp\
		   $$LEVEL/Sources/Monitor/Source/Std/Lookaside.cpp\
		   $$LEVEL/Sources/Monitor/Source/Std/MemManager.cpp\
		   $$LEVEL/Sources/Monitor/Source/Std/CacheLru.cpp\
		   $$LEVEL/Sources/Monitor/Source/Std/StdLibrary.c

DEFINES += _MONITOR_
