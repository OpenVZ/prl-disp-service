LIBTARGET = pvsHostInfo
PROJ_FILE = $$PWD/CHostInfo.pro
QTCONFIG = network core xml
!include(../../Build/qmake/staticlib.pri): error(include error)

win32: LIBS *= -lwinspool

unix: {
	LIBS *= -ldl
}
include($$LIBS_LEVEL/PrlNetworking/PrlNetworking.pri)
