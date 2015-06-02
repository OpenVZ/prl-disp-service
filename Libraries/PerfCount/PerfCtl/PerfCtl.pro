TEMPLATE = app

CONFIG += console
CONFIG -= qt app_bundle

include(PerfCtl.deps)

# win32:LIBS += -lPsapi -lAdvapi32
macx:LIBS	+= -framework IOKit -framework Carbon

INCLUDEPATH += ..

HEADERS += \
    ../PerfLib/PerfCounter.h

SOURCES += \
    PerfCtl.cpp

# It is important to have "File Info" embedded in the
# windows binaries - which means we need windows resource file
win32:RC_FILE = PerfCtl.rc
