LIBTARGET = StatesStore
PROJ_FILE = $$PWD/StatesStore.pro
QTCONFIG = core xml
!include(../../Build/qmake/staticlib.pri): error(include error)

include($$LIBS_LEVEL/PrlCommonUtils/PrlCommonUtils.pri)
