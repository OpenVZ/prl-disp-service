LIBTARGET = StatesUtils
PROJ_FILE = $$PWD/StatesUtils.pro
QTCONFIG = core xml
!include(../../Build/qmake/staticlib.pri): error(include error)

INCLUDEPATH += $$PWD
INCLUDEPATH += $$EXT_LEVEL
INCLUDEPATH += $$SRC_LEVEL/VI/Interfaces

include($$SRC_LEVEL/XmlModel/XmlModel.pri)
include($$LIBS_LEVEL/PrlCommonUtils/PrlCommonUtils.pri)
