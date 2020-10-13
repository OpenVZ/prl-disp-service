TEMPLATE = lib
CONFIG += staticlib

include(Transponster.pri)

DEFINES += BOOST_MPL_CFG_NO_PREPROCESSED_HEADERS BOOST_MPL_LIMIT_VECTOR_SIZE=40
QMAKE_CXXFLAGS += -Wno-maybe-uninitialized

HEADERS += \
	base.h \
	enum.h \
	capability_data.h \
	capability_enum.h \
	capability_type.h \
	domain_enum.h \
	domain_data.h \
	domain_type.h \
	filter_enum.h \
	filter_data.h \
	filter_type.h \
	iface_data.h \
	iface_enum.h \
	iface_type.h \
	network_enum.h \
	network_data.h \
	network_type.h \
	snapshot_enum.h \
	snapshot_data.h \
	snapshot_type.h \
	blockexport_data.h \
	blockexport_enum.h \
	blockexport_type.h \
	blocksnapshot_data.h \
	blocksnapshot_enum.h \
	blocksnapshot_type.h \
	nodedev_data.h \
	nodedev_enum.h \
	nodedev_type.h \
	marshal.h \
	patterns.h \
	Direct.h \
	Direct_p.h \
	Reverse.h \
	Reverse_p.h \
	NetFilter.h

SOURCES	+= \
	text.cpp \
	capability_data.cpp \
	capability_enum.cpp \
	capability_type.cpp \
	domain_data.cpp \
	domain_enum.cpp \
	domain_type.cpp \
	filter_enum.cpp \
	filter_data.cpp \
	filter_type.cpp \
	iface_data.cpp \
	iface_enum.cpp \
	iface_type.cpp \
	network_data.cpp \
	network_enum.cpp \
	network_type.cpp \
	snapshot_data.cpp \
	snapshot_enum.cpp \
	snapshot_type.cpp \
	blockexport_data.cpp \
	blockexport_enum.cpp \
	blockexport_type.cpp \
	blocksnapshot_data.cpp \
	blocksnapshot_enum.cpp \
	blocksnapshot_type.cpp \
	nodedev_data.cpp \
	nodedev_enum.cpp \
	nodedev_type.cpp \
	Direct.cpp \
	Reverse.cpp \
	NetFilter.cpp
