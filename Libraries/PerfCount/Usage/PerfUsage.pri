LIBTARGET = perfusage
PROJ_FILE = $$PWD/PerfUsage.pro
!include(../../../../Sources/Build/qmake/staticlib.pri): error(include error)

include($$LIBS_LEVEL/PerfCount/PerfLib/PerfCounter.pri)
