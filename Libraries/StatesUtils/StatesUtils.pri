LIBTARGET = StatesUtils
PROJ_FILE = $$PWD/StatesUtils.pro
QTCONFIG = core xml
!include(../../Build/qmake/staticlib.pri): error(include error)

INCLUDEPATH += $$PWD
INCLUDEPATH += $$EXT_LEVEL
INCLUDEPATH += $$SRC_LEVEL/VI/Interfaces

include($$LIBS_LEVEL/PrlCommonUtils/PrlCommonUtils.pri)
