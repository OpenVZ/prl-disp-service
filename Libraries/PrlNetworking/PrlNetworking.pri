LIBTARGET = PrlNetworking
PROJ_FILE = $$PWD/PrlNetworking.pro
QTCONFIG = xml core network
!include(../../Build/qmake/staticlib.pri): error(include error)

include($$SRC_LEVEL/XmlModel/XmlModel.pri)
linux-*: include($$LIBS_LEVEL/Virtuozzo/Virtuozzo.pri)
include($$LIBS_LEVEL/PrlCommonUtilsBase/PrlCommonUtilsBase.pri)
include($$LIBS_LEVEL/Std/Std.pri)

INCLUDEPATH += $$SRC_LEVEL/System/Network/drivers/common
linux-*:	INCLUDEPATH += $$SRC_LEVEL/System/Network/drivers/lin/pvsnet
win32:		INCLUDEPATH += $$SRC_LEVEL/System/Network/drivers/win/prl_net

win32 {
	LIBS *= iphlpapi.lib ws2_32.lib
	include($$LIBS_LEVEL/PrlNetEnum/PrlNetEnum.pri)
}

# Apple broke binary compatibility between SDK 10.5 and 10.6:
# symbols _SCPreferencesCreate, SCDynamicStoreCreate were "copied"
# from SystemConfiguration to IOKit. Thus in order to support 10.5
# system need to force using those symbols from SystemConfigution
# during build on SDK 10.6. This is achieved by placing SystemConfigution
# framework *before* IOKit (qmake merges frameworks in linker commandline
# into opposite side as for libraries).
macx: LIBS = -framework SystemConfiguration $$LIBS
