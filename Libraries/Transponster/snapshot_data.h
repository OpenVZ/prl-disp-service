/*
 * Copyright (c) 2015 Parallels IP Holdings GmbH
 *
 * This file is part of Virtuozzo Core Libraries. Virtuozzo Core
 * Libraries is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <http://www.gnu.org/licenses/> or write to Free Software Foundation,
 * 51 Franklin Street, Fifth Floor Boston, MA 02110, USA.
 *
 * Our contact details: Parallels IP Holdings GmbH, Vordergasse 59, 8200
 * Schaffhausen, Switzerland.
 */

#ifndef __SNAPSHOT_DATA_H__
#define __SNAPSHOT_DATA_H__
#include <QString>
#include <QDateTime>
#include "base.h"
#include "patterns.h"
#include "snapshot_enum.h"

namespace Libvirt
{
///////////////////////////////////////////////////////////////////////////////
// struct PUnsignedInt

namespace Snapshot
{
namespace Xml
{
struct PUnsignedInt
{
	typedef quint32 value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Traits<Snapshot::Xml::PUnsignedInt>
{
	static bool parse(const QString& src_, Snapshot::Xml::PUnsignedInt::value_type& dst_);

	static QString generate(Snapshot::Xml::PUnsignedInt::value_type src_);

};


///////////////////////////////////////////////////////////////////////////////
// struct PUnsignedLong

namespace Snapshot
{
namespace Xml
{
struct PUnsignedLong
{
	typedef ulong value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Traits<Snapshot::Xml::PUnsignedLong>
{
	static bool parse(const QString& src_, Snapshot::Xml::PUnsignedLong::value_type& dst_);

	static QString generate(Snapshot::Xml::PUnsignedLong::value_type src_);

};


///////////////////////////////////////////////////////////////////////////////
// struct PPositiveInteger

namespace Snapshot
{
namespace Xml
{
struct PPositiveInteger
{
	typedef quint32 value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Traits<Snapshot::Xml::PPositiveInteger>
{
	static bool parse(const QString& src_, Snapshot::Xml::PPositiveInteger::value_type& dst_);

	static QString generate(Snapshot::Xml::PPositiveInteger::value_type src_);

};


///////////////////////////////////////////////////////////////////////////////
// struct POctalMode

namespace Snapshot
{
namespace Xml
{
struct POctalMode
{
	typedef quint32 value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Traits<Snapshot::Xml::POctalMode>
{
	static bool parse(const QString& src_, Snapshot::Xml::POctalMode::value_type& dst_);

	static QString generate(Snapshot::Xml::POctalMode::value_type src_);

};


///////////////////////////////////////////////////////////////////////////////
// struct PData1279

namespace Snapshot
{
namespace Xml
{
struct PData1279
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PData1279>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PData1279::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData1281

namespace Snapshot
{
namespace Xml
{
struct PData1281
{
	typedef qint32 value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Traits<Snapshot::Xml::PData1281>
{
	static bool parse(const QString& src_, Snapshot::Xml::PData1281::value_type& dst_);

	static QString generate(Snapshot::Xml::PData1281::value_type src_);

};

template<>
struct Validatable<Snapshot::Xml::PData1281>: mpl::true_
{
	static bool validate(Snapshot::Xml::PData1281::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData1283

namespace Snapshot
{
namespace Xml
{
struct PData1283
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PData1283>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PData1283::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData1285

namespace Snapshot
{
namespace Xml
{
struct PData1285
{
	typedef qint32 value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Traits<Snapshot::Xml::PData1285>
{
	static bool parse(const QString& src_, Snapshot::Xml::PData1285::value_type& dst_);

	static QString generate(Snapshot::Xml::PData1285::value_type src_);

};

template<>
struct Validatable<Snapshot::Xml::PData1285>: mpl::true_
{
	static bool validate(Snapshot::Xml::PData1285::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData1287

namespace Snapshot
{
namespace Xml
{
struct PData1287
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PData1287>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PData1287::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData1289

namespace Snapshot
{
namespace Xml
{
struct PData1289
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PData1289>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PData1289::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PUniMacAddr

namespace Snapshot
{
namespace Xml
{
struct PUniMacAddr
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PUniMacAddr>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PUniMacAddr::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PMultiMacAddr

namespace Snapshot
{
namespace Xml
{
struct PMultiMacAddr
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PMultiMacAddr>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PMultiMacAddr::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PMacAddr

namespace Snapshot
{
namespace Xml
{
struct PMacAddr
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PMacAddr>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PMacAddr::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PDuidLLT

namespace Snapshot
{
namespace Xml
{
struct PDuidLLT
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PDuidLLT>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PDuidLLT::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PDuidEN

namespace Snapshot
{
namespace Xml
{
struct PDuidEN
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PDuidEN>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PDuidEN::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PDuidLL

namespace Snapshot
{
namespace Xml
{
struct PDuidLL
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PDuidLL>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PDuidLL::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PDuidUUID

namespace Snapshot
{
namespace Xml
{
struct PDuidUUID
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PDuidUUID>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PDuidUUID::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PIpv4Addr

namespace Snapshot
{
namespace Xml
{
struct PIpv4Addr
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PIpv4Addr>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PIpv4Addr::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PIpv6Addr

namespace Snapshot
{
namespace Xml
{
struct PIpv6Addr
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PIpv6Addr>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PIpv6Addr::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PIpv4Prefix

namespace Snapshot
{
namespace Xml
{
struct PIpv4Prefix
{
	typedef quint32 value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Traits<Snapshot::Xml::PIpv4Prefix>
{
	static bool parse(const QString& src_, Snapshot::Xml::PIpv4Prefix::value_type& dst_);

	static QString generate(Snapshot::Xml::PIpv4Prefix::value_type src_);

};

template<>
struct Validatable<Snapshot::Xml::PIpv4Prefix>: mpl::true_
{
	static bool validate(Snapshot::Xml::PIpv4Prefix::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PIpv6Prefix

namespace Snapshot
{
namespace Xml
{
struct PIpv6Prefix
{
	typedef quint32 value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Traits<Snapshot::Xml::PIpv6Prefix>
{
	static bool parse(const QString& src_, Snapshot::Xml::PIpv6Prefix::value_type& dst_);

	static QString generate(Snapshot::Xml::PIpv6Prefix::value_type src_);

};

template<>
struct Validatable<Snapshot::Xml::PIpv6Prefix>: mpl::true_
{
	static bool validate(Snapshot::Xml::PIpv6Prefix::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PGenericName

namespace Snapshot
{
namespace Xml
{
struct PGenericName
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PGenericName>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PGenericName::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PDnsName

namespace Snapshot
{
namespace Xml
{
struct PDnsName
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PDnsName>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PDnsName::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PDeviceName

namespace Snapshot
{
namespace Xml
{
struct PDeviceName
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PDeviceName>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PDeviceName::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PFilePath

namespace Snapshot
{
namespace Xml
{
struct PFilePath
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PFilePath>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PFilePath::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PDirPath

namespace Snapshot
{
namespace Xml
{
struct PDirPath
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PDirPath>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PDirPath::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PAbsFilePath

namespace Snapshot
{
namespace Xml
{
struct PAbsFilePath
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PAbsFilePath>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PAbsFilePath::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PAbsDirPath

namespace Snapshot
{
namespace Xml
{
struct PAbsDirPath
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PAbsDirPath>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PAbsDirPath::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PUnit

namespace Snapshot
{
namespace Xml
{
struct PUnit
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PUnit>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PUnit::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PPciDomain

namespace Snapshot
{
namespace Xml
{
struct PPciDomain
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PPciDomain>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PPciDomain::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PPciBus

namespace Snapshot
{
namespace Xml
{
struct PPciBus
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PPciBus>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PPciBus::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PPciSlot

namespace Snapshot
{
namespace Xml
{
struct PPciSlot
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PPciSlot>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PPciSlot::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PPciFunc

namespace Snapshot
{
namespace Xml
{
struct PPciFunc
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PPciFunc>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PPciFunc::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PWwn

namespace Snapshot
{
namespace Xml
{
struct PWwn
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PWwn>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PWwn::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PCpuset

namespace Snapshot
{
namespace Xml
{
struct PCpuset
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PCpuset>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PCpuset::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PVolName

namespace Snapshot
{
namespace Xml
{
struct PVolName
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PVolName>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PVolName::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PPortNumber

namespace Snapshot
{
namespace Xml
{
struct PPortNumber
{
	typedef qint32 value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Traits<Snapshot::Xml::PPortNumber>
{
	static bool parse(const QString& src_, Snapshot::Xml::PPortNumber::value_type& dst_);

	static QString generate(Snapshot::Xml::PPortNumber::value_type src_);

};

template<>
struct Validatable<Snapshot::Xml::PPortNumber>: mpl::true_
{
	static bool validate(Snapshot::Xml::PPortNumber::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PIobase

namespace Snapshot
{
namespace Xml
{
struct PIobase
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PIobase>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PIobase::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PIrq

namespace Snapshot
{
namespace Xml
{
struct PIrq
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PIrq>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PIrq::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PCompat

namespace Snapshot
{
namespace Xml
{
struct PCompat
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PCompat>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PCompat::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PVirtualPortProfileID

namespace Snapshot
{
namespace Xml
{
struct PVirtualPortProfileID
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PVirtualPortProfileID>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PVirtualPortProfileID::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PSpeed

namespace Snapshot
{
namespace Xml
{
struct PSpeed
{
	typedef quint32 value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Traits<Snapshot::Xml::PSpeed>
{
	static bool parse(const QString& src_, Snapshot::Xml::PSpeed::value_type& dst_);

	static QString generate(Snapshot::Xml::PSpeed::value_type src_);

};

template<>
struct Validatable<Snapshot::Xml::PSpeed>: mpl::true_
{
	static bool validate(Snapshot::Xml::PSpeed::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PBurstSize

namespace Snapshot
{
namespace Xml
{
struct PBurstSize
{
	typedef quint32 value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Traits<Snapshot::Xml::PBurstSize>
{
	static bool parse(const QString& src_, Snapshot::Xml::PBurstSize::value_type& dst_);

	static QString generate(Snapshot::Xml::PBurstSize::value_type src_);

};

template<>
struct Validatable<Snapshot::Xml::PBurstSize>: mpl::true_
{
	static bool validate(Snapshot::Xml::PBurstSize::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PUnsignedShort

namespace Snapshot
{
namespace Xml
{
struct PUnsignedShort
{
	typedef qint32 value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Traits<Snapshot::Xml::PUnsignedShort>
{
	static bool parse(const QString& src_, Snapshot::Xml::PUnsignedShort::value_type& dst_);

	static QString generate(Snapshot::Xml::PUnsignedShort::value_type src_);

};

template<>
struct Validatable<Snapshot::Xml::PUnsignedShort>: mpl::true_
{
	static bool validate(Snapshot::Xml::PUnsignedShort::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PProtocol

namespace Snapshot
{
namespace Xml
{
struct PProtocol
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PProtocol>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PProtocol::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PAddrFamily

namespace Snapshot
{
namespace Xml
{
struct PAddrFamily
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PAddrFamily>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PAddrFamily::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PId

namespace Snapshot
{
namespace Xml
{
struct PId
{
	typedef quint32 value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Traits<Snapshot::Xml::PId>
{
	static bool parse(const QString& src_, Snapshot::Xml::PId::value_type& dst_);

	static QString generate(Snapshot::Xml::PId::value_type src_);

};

template<>
struct Validatable<Snapshot::Xml::PId>: mpl::true_
{
	static bool validate(Snapshot::Xml::PId::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PPort

namespace Snapshot
{
namespace Xml
{
struct PPort
{
	typedef qint32 value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Traits<Snapshot::Xml::PPort>
{
	static bool parse(const QString& src_, Snapshot::Xml::PPort::value_type& dst_);

	static QString generate(Snapshot::Xml::PPort::value_type src_);

};

template<>
struct Validatable<Snapshot::Xml::PPort>: mpl::true_
{
	static bool validate(Snapshot::Xml::PPort::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PTitle

namespace Snapshot
{
namespace Xml
{
struct PTitle
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PTitle>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PTitle::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PMachine

namespace Snapshot
{
namespace Xml
{
struct PMachine
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PMachine>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PMachine::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PReadIopsSec

namespace Snapshot
{
namespace Xml
{
struct PReadIopsSec
{
	typedef quint32 value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Traits<Snapshot::Xml::PReadIopsSec>
{
	static bool parse(const QString& src_, Snapshot::Xml::PReadIopsSec::value_type& dst_);

	static QString generate(Snapshot::Xml::PReadIopsSec::value_type src_);

};


///////////////////////////////////////////////////////////////////////////////
// struct PWriteIopsSec

namespace Snapshot
{
namespace Xml
{
struct PWriteIopsSec
{
	typedef quint32 value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Traits<Snapshot::Xml::PWriteIopsSec>
{
	static bool parse(const QString& src_, Snapshot::Xml::PWriteIopsSec::value_type& dst_);

	static QString generate(Snapshot::Xml::PWriteIopsSec::value_type src_);

};


///////////////////////////////////////////////////////////////////////////////
// struct PReadBytesSec

namespace Snapshot
{
namespace Xml
{
struct PReadBytesSec
{
	typedef ulong value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Traits<Snapshot::Xml::PReadBytesSec>
{
	static bool parse(const QString& src_, Snapshot::Xml::PReadBytesSec::value_type& dst_);

	static QString generate(Snapshot::Xml::PReadBytesSec::value_type src_);

};


///////////////////////////////////////////////////////////////////////////////
// struct PWriteBytesSec

namespace Snapshot
{
namespace Xml
{
struct PWriteBytesSec
{
	typedef ulong value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Traits<Snapshot::Xml::PWriteBytesSec>
{
	static bool parse(const QString& src_, Snapshot::Xml::PWriteBytesSec::value_type& dst_);

	static QString generate(Snapshot::Xml::PWriteBytesSec::value_type src_);

};


///////////////////////////////////////////////////////////////////////////////
// struct PVendor

namespace Snapshot
{
namespace Xml
{
struct PVendor
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PVendor>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PVendor::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PProduct

namespace Snapshot
{
namespace Xml
{
struct PProduct
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PProduct>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PProduct::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PDiskTarget

namespace Snapshot
{
namespace Xml
{
struct PDiskTarget
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PDiskTarget>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PDiskTarget::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PCyls

namespace Snapshot
{
namespace Xml
{
struct PCyls
{
	typedef qint32 value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Traits<Snapshot::Xml::PCyls>
{
	static bool parse(const QString& src_, Snapshot::Xml::PCyls::value_type& dst_);

	static QString generate(Snapshot::Xml::PCyls::value_type src_);

};


///////////////////////////////////////////////////////////////////////////////
// struct PHeads

namespace Snapshot
{
namespace Xml
{
struct PHeads
{
	typedef qint32 value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Traits<Snapshot::Xml::PHeads>
{
	static bool parse(const QString& src_, Snapshot::Xml::PHeads::value_type& dst_);

	static QString generate(Snapshot::Xml::PHeads::value_type src_);

};


///////////////////////////////////////////////////////////////////////////////
// struct PSecs

namespace Snapshot
{
namespace Xml
{
struct PSecs
{
	typedef qint32 value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Traits<Snapshot::Xml::PSecs>
{
	static bool parse(const QString& src_, Snapshot::Xml::PSecs::value_type& dst_);

	static QString generate(Snapshot::Xml::PSecs::value_type src_);

};


///////////////////////////////////////////////////////////////////////////////
// struct PLogicalBlockSize

namespace Snapshot
{
namespace Xml
{
struct PLogicalBlockSize
{
	typedef qint32 value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Traits<Snapshot::Xml::PLogicalBlockSize>
{
	static bool parse(const QString& src_, Snapshot::Xml::PLogicalBlockSize::value_type& dst_);

	static QString generate(Snapshot::Xml::PLogicalBlockSize::value_type src_);

};


///////////////////////////////////////////////////////////////////////////////
// struct PPhysicalBlockSize

namespace Snapshot
{
namespace Xml
{
struct PPhysicalBlockSize
{
	typedef qint32 value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Traits<Snapshot::Xml::PPhysicalBlockSize>
{
	static bool parse(const QString& src_, Snapshot::Xml::PPhysicalBlockSize::value_type& dst_);

	static QString generate(Snapshot::Xml::PPhysicalBlockSize::value_type src_);

};


///////////////////////////////////////////////////////////////////////////////
// struct PType

namespace Snapshot
{
namespace Xml
{
struct PType
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PType>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PType::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PPasswdValidTo

namespace Snapshot
{
namespace Xml
{
struct PPasswdValidTo
{
	typedef QDateTime value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Traits<Snapshot::Xml::PPasswdValidTo>
{
	static bool parse(const QString& src_, Snapshot::Xml::PPasswdValidTo::value_type& dst_);

	static QString generate(const Snapshot::Xml::PPasswdValidTo::value_type& src_);

};


///////////////////////////////////////////////////////////////////////////////
// struct PVendorId

namespace Snapshot
{
namespace Xml
{
struct PVendorId
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PVendorId>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PVendorId::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PSysinfoValue

namespace Snapshot
{
namespace Xml
{
struct PSysinfoValue
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PSysinfoValue>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PSysinfoValue::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PFilter

namespace Snapshot
{
namespace Xml
{
struct PFilter
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot


///////////////////////////////////////////////////////////////////////////////
// struct PTotalBytesSec

namespace Snapshot
{
namespace Xml
{
struct PTotalBytesSec
{
	typedef ulong value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Traits<Snapshot::Xml::PTotalBytesSec>
{
	static bool parse(const QString& src_, Snapshot::Xml::PTotalBytesSec::value_type& dst_);

	static QString generate(Snapshot::Xml::PTotalBytesSec::value_type src_);

};


///////////////////////////////////////////////////////////////////////////////
// struct PTotalIopsSec

namespace Snapshot
{
namespace Xml
{
struct PTotalIopsSec
{
	typedef ulong value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Traits<Snapshot::Xml::PTotalIopsSec>
{
	static bool parse(const QString& src_, Snapshot::Xml::PTotalIopsSec::value_type& dst_);

	static QString generate(Snapshot::Xml::PTotalIopsSec::value_type src_);

};


///////////////////////////////////////////////////////////////////////////////
// struct PReadIopsSec1

namespace Snapshot
{
namespace Xml
{
struct PReadIopsSec1
{
	typedef ulong value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Traits<Snapshot::Xml::PReadIopsSec1>
{
	static bool parse(const QString& src_, Snapshot::Xml::PReadIopsSec1::value_type& dst_);

	static QString generate(Snapshot::Xml::PReadIopsSec1::value_type src_);

};


///////////////////////////////////////////////////////////////////////////////
// struct PWriteIopsSec1

namespace Snapshot
{
namespace Xml
{
struct PWriteIopsSec1
{
	typedef ulong value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Traits<Snapshot::Xml::PWriteIopsSec1>
{
	static bool parse(const QString& src_, Snapshot::Xml::PWriteIopsSec1::value_type& dst_);

	static QString generate(Snapshot::Xml::PWriteIopsSec1::value_type src_);

};


///////////////////////////////////////////////////////////////////////////////
// struct PRetries

namespace Snapshot
{
namespace Xml
{
struct PRetries
{
	typedef quint32 value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Traits<Snapshot::Xml::PRetries>
{
	static bool parse(const QString& src_, Snapshot::Xml::PRetries::value_type& dst_);

	static QString generate(Snapshot::Xml::PRetries::value_type src_);

};


///////////////////////////////////////////////////////////////////////////////
// struct PCountCPU

namespace Snapshot
{
namespace Xml
{
struct PCountCPU
{
	typedef quint16 value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Traits<Snapshot::Xml::PCountCPU>
{
	static bool parse(const QString& src_, Snapshot::Xml::PCountCPU::value_type& dst_);

	static QString generate(Snapshot::Xml::PCountCPU::value_type src_);

};

template<>
struct Validatable<Snapshot::Xml::PCountCPU>: mpl::true_
{
	static bool validate(Snapshot::Xml::PCountCPU::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PVcpuid

namespace Snapshot
{
namespace Xml
{
struct PVcpuid
{
	typedef quint16 value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Traits<Snapshot::Xml::PVcpuid>
{
	static bool parse(const QString& src_, Snapshot::Xml::PVcpuid::value_type& dst_);

	static QString generate(Snapshot::Xml::PVcpuid::value_type src_);

};


///////////////////////////////////////////////////////////////////////////////
// struct PCpushares

namespace Snapshot
{
namespace Xml
{
struct PCpushares
{
	typedef quint32 value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Traits<Snapshot::Xml::PCpushares>
{
	static bool parse(const QString& src_, Snapshot::Xml::PCpushares::value_type& dst_);

	static QString generate(Snapshot::Xml::PCpushares::value_type src_);

};


///////////////////////////////////////////////////////////////////////////////
// struct PCpuperiod

namespace Snapshot
{
namespace Xml
{
struct PCpuperiod
{
	typedef ulong value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Traits<Snapshot::Xml::PCpuperiod>
{
	static bool parse(const QString& src_, Snapshot::Xml::PCpuperiod::value_type& dst_);

	static QString generate(Snapshot::Xml::PCpuperiod::value_type src_);

};

template<>
struct Validatable<Snapshot::Xml::PCpuperiod>: mpl::true_
{
	static bool validate(Snapshot::Xml::PCpuperiod::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PCpuquota

namespace Snapshot
{
namespace Xml
{
struct PCpuquota
{
	typedef long value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Traits<Snapshot::Xml::PCpuquota>
{
	static bool parse(const QString& src_, Snapshot::Xml::PCpuquota::value_type& dst_);

	static QString generate(Snapshot::Xml::PCpuquota::value_type src_);

};

template<>
struct Validatable<Snapshot::Xml::PCpuquota>: mpl::true_
{
	static bool validate(Snapshot::Xml::PCpuquota::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PRebootTimeoutDelay

namespace Snapshot
{
namespace Xml
{
struct PRebootTimeoutDelay
{
	typedef qint16 value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Traits<Snapshot::Xml::PRebootTimeoutDelay>
{
	static bool parse(const QString& src_, Snapshot::Xml::PRebootTimeoutDelay::value_type& dst_);

	static QString generate(Snapshot::Xml::PRebootTimeoutDelay::value_type src_);

};

template<>
struct Validatable<Snapshot::Xml::PRebootTimeoutDelay>: mpl::true_
{
	static bool validate(Snapshot::Xml::PRebootTimeoutDelay::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PWeight

namespace Snapshot
{
namespace Xml
{
struct PWeight
{
	typedef quint32 value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Traits<Snapshot::Xml::PWeight>
{
	static bool parse(const QString& src_, Snapshot::Xml::PWeight::value_type& dst_);

	static QString generate(Snapshot::Xml::PWeight::value_type src_);

};

template<>
struct Validatable<Snapshot::Xml::PWeight>: mpl::true_
{
	static bool validate(Snapshot::Xml::PWeight::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PMemoryKB

namespace Snapshot
{
namespace Xml
{
struct PMemoryKB
{
	typedef quint32 value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Traits<Snapshot::Xml::PMemoryKB>
{
	static bool parse(const QString& src_, Snapshot::Xml::PMemoryKB::value_type& dst_);

	static QString generate(Snapshot::Xml::PMemoryKB::value_type src_);

};


///////////////////////////////////////////////////////////////////////////////
// struct PDomainName

namespace Snapshot
{
namespace Xml
{
struct PDomainName
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PDomainName>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PDomainName::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PDiskSerial

namespace Snapshot
{
namespace Xml
{
struct PDiskSerial
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PDiskSerial>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PDiskSerial::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PBridgeMode

namespace Snapshot
{
namespace Xml
{
struct PBridgeMode
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PBridgeMode>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PBridgeMode::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PAddrIPorName

namespace Snapshot
{
namespace Xml
{
struct PAddrIPorName
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PAddrIPorName>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PAddrIPorName::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PUsbIdDefault

namespace Snapshot
{
namespace Xml
{
struct PUsbIdDefault
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PUsbIdDefault>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PUsbIdDefault::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PUsbId

namespace Snapshot
{
namespace Xml
{
struct PUsbId
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PUsbId>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PUsbId::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PUsbVersion

namespace Snapshot
{
namespace Xml
{
struct PUsbVersion
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PUsbVersion>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PUsbVersion::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PUsbAddr

namespace Snapshot
{
namespace Xml
{
struct PUsbAddr
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PUsbAddr>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PUsbAddr::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PUsbClass

namespace Snapshot
{
namespace Xml
{
struct PUsbClass
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PUsbClass>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PUsbClass::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PUsbPort

namespace Snapshot
{
namespace Xml
{
struct PUsbPort
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PUsbPort>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PUsbPort::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PDriveController

namespace Snapshot
{
namespace Xml
{
struct PDriveController
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PDriveController>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PDriveController::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PDriveBus

namespace Snapshot
{
namespace Xml
{
struct PDriveBus
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PDriveBus>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PDriveBus::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PDriveTarget

namespace Snapshot
{
namespace Xml
{
struct PDriveTarget
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PDriveTarget>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PDriveTarget::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PDriveUnit

namespace Snapshot
{
namespace Xml
{
struct PDriveUnit
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PDriveUnit>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PDriveUnit::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PFeatureName

namespace Snapshot
{
namespace Xml
{
struct PFeatureName
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PFeatureName>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PFeatureName::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PTimeDelta

namespace Snapshot
{
namespace Xml
{
struct PTimeDelta
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PTimeDelta>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PTimeDelta::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PTimeZone

namespace Snapshot
{
namespace Xml
{
struct PTimeZone
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PTimeZone>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PTimeZone::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PFilterParamName

namespace Snapshot
{
namespace Xml
{
struct PFilterParamName
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PFilterParamName>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PFilterParamName::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PFilterParamValue

namespace Snapshot
{
namespace Xml
{
struct PFilterParamValue
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PFilterParamValue>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PFilterParamValue::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PSpaprvioReg

namespace Snapshot
{
namespace Xml
{
struct PSpaprvioReg
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PSpaprvioReg>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PSpaprvioReg::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PAliasName

namespace Snapshot
{
namespace Xml
{
struct PAliasName
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PAliasName>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PAliasName::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData1485

namespace Snapshot
{
namespace Xml
{
struct PData1485
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PData1485>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PData1485::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData1487

namespace Snapshot
{
namespace Xml
{
struct PData1487
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PData1487>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PData1487::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData1489

namespace Snapshot
{
namespace Xml
{
struct PData1489
{
	typedef qint32 value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Traits<Snapshot::Xml::PData1489>
{
	static bool parse(const QString& src_, Snapshot::Xml::PData1489::value_type& dst_);

	static QString generate(Snapshot::Xml::PData1489::value_type src_);

};

template<>
struct Validatable<Snapshot::Xml::PData1489>: mpl::true_
{
	static bool validate(Snapshot::Xml::PData1489::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PCcwSsidRange

namespace Snapshot
{
namespace Xml
{
struct PCcwSsidRange
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PCcwSsidRange>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PCcwSsidRange::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData1491

namespace Snapshot
{
namespace Xml
{
struct PData1491
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PData1491>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PData1491::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData1493

namespace Snapshot
{
namespace Xml
{
struct PData1493
{
	typedef qint32 value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Traits<Snapshot::Xml::PData1493>
{
	static bool parse(const QString& src_, Snapshot::Xml::PData1493::value_type& dst_);

	static QString generate(Snapshot::Xml::PData1493::value_type src_);

};

template<>
struct Validatable<Snapshot::Xml::PData1493>: mpl::true_
{
	static bool validate(Snapshot::Xml::PData1493::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData1503

namespace Snapshot
{
namespace Xml
{
struct PData1503
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PData1503>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PData1503::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData1505

namespace Snapshot
{
namespace Xml
{
struct PData1505
{
	typedef qint32 value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Traits<Snapshot::Xml::PData1505>
{
	static bool parse(const QString& src_, Snapshot::Xml::PData1505::value_type& dst_);

	static QString generate(Snapshot::Xml::PData1505::value_type src_);

};

template<>
struct Validatable<Snapshot::Xml::PData1505>: mpl::true_
{
	static bool validate(Snapshot::Xml::PData1505::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData1507

namespace Snapshot
{
namespace Xml
{
struct PData1507
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PData1507>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PData1507::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData1509

namespace Snapshot
{
namespace Xml
{
struct PData1509
{
	typedef qint32 value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Traits<Snapshot::Xml::PData1509>
{
	static bool parse(const QString& src_, Snapshot::Xml::PData1509::value_type& dst_);

	static QString generate(Snapshot::Xml::PData1509::value_type src_);

};

template<>
struct Validatable<Snapshot::Xml::PData1509>: mpl::true_
{
	static bool validate(Snapshot::Xml::PData1509::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData1511

namespace Snapshot
{
namespace Xml
{
struct PData1511
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PData1511>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PData1511::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData1513

namespace Snapshot
{
namespace Xml
{
struct PData1513
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PData1513>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PData1513::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData1709

namespace Snapshot
{
namespace Xml
{
struct PData1709
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PData1709>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PData1709::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData1711

namespace Snapshot
{
namespace Xml
{
struct PData1711
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PData1711>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PData1711::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData1713

namespace Snapshot
{
namespace Xml
{
struct PData1713
{
	typedef qint32 value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Traits<Snapshot::Xml::PData1713>
{
	static bool parse(const QString& src_, Snapshot::Xml::PData1713::value_type& dst_);

	static QString generate(Snapshot::Xml::PData1713::value_type src_);

};

template<>
struct Validatable<Snapshot::Xml::PData1713>: mpl::true_
{
	static bool validate(Snapshot::Xml::PData1713::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData1715

namespace Snapshot
{
namespace Xml
{
struct PData1715
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PData1715>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PData1715::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData1717

namespace Snapshot
{
namespace Xml
{
struct PData1717
{
	typedef qint32 value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Traits<Snapshot::Xml::PData1717>
{
	static bool parse(const QString& src_, Snapshot::Xml::PData1717::value_type& dst_);

	static QString generate(Snapshot::Xml::PData1717::value_type src_);

};

template<>
struct Validatable<Snapshot::Xml::PData1717>: mpl::true_
{
	static bool validate(Snapshot::Xml::PData1717::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct VName

namespace Snapshot
{
namespace Xml
{
typedef Choice<mpl::vector<PDiskTarget, PAbsFilePath > > VNameImpl;
typedef VNameImpl::value_type VName;

} // namespace Xml
} // namespace Snapshot

template<>
struct Traits<Snapshot::Xml::VName>
{
	static bool parse(const QString& src_, Snapshot::Xml::VName& dst_);

	static QString generate(const Snapshot::Xml::VName& src_);

};

///////////////////////////////////////////////////////////////////////////////
// struct VIpAddr

namespace Snapshot
{
namespace Xml
{
typedef Choice<mpl::vector<PIpv4Addr, PIpv6Addr > > VIpAddrImpl;
typedef VIpAddrImpl::value_type VIpAddr;

} // namespace Xml
} // namespace Snapshot

template<>
struct Traits<Snapshot::Xml::VIpAddr>
{
	static bool parse(const QString& src_, Snapshot::Xml::VIpAddr& dst_);

	static QString generate(const Snapshot::Xml::VIpAddr& src_);

};

///////////////////////////////////////////////////////////////////////////////
// struct VName1

namespace Snapshot
{
namespace Xml
{
typedef Choice<mpl::vector<PDnsName, VIpAddr > > VName1Impl;
typedef VName1Impl::value_type VName1;

} // namespace Xml
} // namespace Snapshot

template<>
struct Traits<Snapshot::Xml::VName1>
{
	static bool parse(const QString& src_, Snapshot::Xml::VName1& dst_);

	static QString generate(const Snapshot::Xml::VName1& src_);

};

///////////////////////////////////////////////////////////////////////////////
// struct VUUID

namespace Snapshot
{
namespace Xml
{
typedef Choice<mpl::vector<PData1511, PData1513 > > VUUIDImpl;
typedef VUUIDImpl::value_type VUUID;

} // namespace Xml
} // namespace Snapshot

template<>
struct Traits<Snapshot::Xml::VUUID>
{
	static bool parse(const QString& src_, Snapshot::Xml::VUUID& dst_);

	static QString generate(const Snapshot::Xml::VUUID& src_);

};

} // namespace Libvirt

#endif // __SNAPSHOT_DATA_H__
