LIBTARGET = pvsHostInfo
PROJ_FILE = $$PWD/CHostInfo.pro
QTCONFIG = network core xml
!include(../../Build/qmake/staticlib.pri): error(include error)

win32: LIBS *= -lwinspool

include($$SRC_LEVEL/XmlModel/XmlModel.pri)
unix: {
	LIBS *= -ldl
}
win32: include($$LIBS_LEVEL/HostUtils/HostUtils.pri)
include($$LIBS_LEVEL/PrlNetworking/PrlNetworking.pri)
