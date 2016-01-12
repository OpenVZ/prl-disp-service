TEMPLATE = lib
CONFIG += console staticlib
QT = xml

include(PerfCounter.pri)

macx:LIBS	+= -framework IOKit 

HEADERS += \
		PerfCounter.h \
		platform_spec.h \
		trace.h \
		PerfCountersOut.h \
		PerfCounterHost.h


SOURCES += \
		PerfCounter.cpp \
		PerfCountersOut.cpp

unix:SOURCES += PerfCounter_unix.cpp
win32:SOURCES += PerfCounter_win.cpp

macx:SOURCES	+= PerfCounterHost_mac.cpp
win32:SOURCES	+= PerfCounterHost_win.cpp
linux-*:SOURCES	+= PerfCounterHost_lin.cpp
