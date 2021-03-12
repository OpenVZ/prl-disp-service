LIBTARGET = prl_common_utils
PROJ_FILE = $$PWD/PrlCommonUtils.pro
QTCONFIG = core network xml
!include(../../Build/qmake/staticlib.pri): error(include error)

INCLUDEPATH *= $$PWD

include($$LIBS_LEVEL/NonQtUtils/PrlNonQtUtils.pri)

win32 {
	include($$LIBS_LEVEL/WmiWrap/WmiWrap.pri)
	# TODO Check if these to libs are needed really
	LIBS *= -lNetapi32 -lUserenv -lAdvapi32
}

