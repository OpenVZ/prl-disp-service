TEMPLATE = lib
CONFIG += staticlib

include(CHostInfo.pri)

INCLUDEPATH += ../../Monitor/Interfaces

HEADERS += \
	CHostInfo.h \
	UsbFriendlyNames.h

SOURCES += CHostInfo_common.cpp

win32 {
	SOURCES += \
		CHostInfo_win.cpp \
		$$SRC_LEVEL/Devices/Usb/UsbHostDevInfo_Win.cpp
}

linux-* {
	HEADERS += \
		cpufeatures.h
	SOURCES += \
		CHostInfo_lin.cpp \
		cpufeatures.cpp
}

