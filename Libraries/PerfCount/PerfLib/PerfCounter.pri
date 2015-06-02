LIBTARGET = perfcounter
PROJ_FILE = $$PWD/PerfCounter.pro
!include(../../../Build/qmake/staticlib.pri): error(include error)

linux-*: LIBS += -lrt
win32: LIBS += -lpsapi -ladvapi32
