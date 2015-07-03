TEMPLATE = lib

CONFIG += qtestlib thread
QT -= gui

Debug:VER = Debug
CONFIG(debug) {
	VER = Debug
} else {
	VER = Release
}

QMAKE_POST_LINK += $$PWD/ArchiveTests.py $$VER
