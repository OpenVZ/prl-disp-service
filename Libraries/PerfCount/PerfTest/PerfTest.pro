TARGET = PerfTest
TEMPLATE = app

CONFIG += console
QT =

LEVEL = ./../../../..

include($$LEVEL/Sources/Parallels.pri)
include($$LEVEL/Sources/Build/Build.pri)

LIBS += -lperfcounter

win32:LIBS += -lPsapi -lAdvapi32
unix:LIBS += -lpthread
linux-*:LIBS += -lrt

INCLUDEPATH += ..

HEADERS += \
    ../PerfLib/PerfCounter.h  \
    ../PerfLib/trace.h


SOURCES += \
    perfcounter_test.cpp
