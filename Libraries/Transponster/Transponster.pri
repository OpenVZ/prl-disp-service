LIBTARGET = Transponster
PROJ_FILE = $$PWD/Transponster.pro
QTCONFIG = core xml network
!include(../../Build/qmake/staticlib.pri): error(include error)

INCLUDEPATH *= $$PWD

