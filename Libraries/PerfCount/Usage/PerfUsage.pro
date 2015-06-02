TEMPLATE = lib
CONFIG += console staticlib
QT =

include(PerfUsage.pri)

macx:LIBS += -framework IOKit

HEADERS += \
           SimpleStorage.h

SOURCES += \
           SimpleStorage.cpp
