LIBTARGET = CAuth
PROJ_FILE = $$PWD/CAuth.pro
QTCONFIG = core
!include(../../Build/qmake/staticlib.pri): error(include error)

linux-*: LIBS += -lpam
win32 {
	# not sure that we need DEFINES here
	DEFINES *= __WINDOWS__ _WINDOWS
	LIBS += -lAdvapi32 -lShlwapi -lNetapi32 -lSecur32 -lauthz
	INCLUDEPATH *= $$PWD
}

