LIBTARGET = virtuozzo
PROJ_FILE = $$PWD/Virtuozzo.pro
QTCONFIG = core network xml
!include(../../Build/qmake/staticlib.pri): error(include error)

DEFINES *= _CT_

include($$SRC_LEVEL/XmlModel/XmlModel.pri)
# include($$LIBS_LEVEL/PrlNetworking/PrlNetworking.pri)
include($$LIBS_LEVEL/PrlCommonUtils/PrlCommonUtils.pri)

linux-*: {
	LIBS += -ldl
	contains(DYN_VZLIB, TRUE) {
		LIBS += -lvzctl2 -lploop
	}
}

