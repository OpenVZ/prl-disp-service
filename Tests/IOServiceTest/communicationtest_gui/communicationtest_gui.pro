
TEMPLATE = subdirs

LEVEL = ../../../..
include($$LEVEL/Sources/Build/Options.pri)

include($$PWD/Client/Client.deps)
include($$PWD/Server/Server.deps)
