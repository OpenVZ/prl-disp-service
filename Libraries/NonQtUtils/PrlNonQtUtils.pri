LIBTARGET = prl_common_nonqt_utils
PROJ_FILE = $$PWD/PrlNonQtUtils.pro
!include(../../Build/qmake/staticlib.pri): error(include error)

INCLUDEPATH *= $$PWD

include($$LIBS_LEVEL/PrlCommonUtilsBase/PrlCommonUtilsBase.pri)
include($$LIBS_LEVEL/Logging/Logging.pri)

