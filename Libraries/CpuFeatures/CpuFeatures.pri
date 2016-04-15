LIBTARGET = CpuFeatures
PROJ_FILE = $$PWD/CpuFeatures.pro
QTCONFIG = core xml
!include(../../Build/qmake/staticlib.pri): error(include error)

INCLUDEPATH *= $$PWD
