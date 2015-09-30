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
// struct PData1267

namespace Snapshot
{
namespace Xml
{
struct PData1267
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PData1267>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PData1267::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData1269

namespace Snapshot
{
namespace Xml
{
struct PData1269
{
	typedef qint32 value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Traits<Snapshot::Xml::PData1269>
{
	static bool parse(const QString& src_, Snapshot::Xml::PData1269::value_type& dst_);

	static QString generate(Snapshot::Xml::PData1269::value_type src_);

};

template<>
struct Validatable<Snapshot::Xml::PData1269>: mpl::true_
{
	static bool validate(Snapshot::Xml::PData1269::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData1271

namespace Snapshot
{
namespace Xml
{
struct PData1271
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PData1271>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PData1271::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData1273

namespace Snapshot
{
namespace Xml
{
struct PData1273
{
	typedef qint32 value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Traits<Snapshot::Xml::PData1273>
{
	static bool parse(const QString& src_, Snapshot::Xml::PData1273::value_type& dst_);

	static QString generate(Snapshot::Xml::PData1273::value_type src_);

};

template<>
struct Validatable<Snapshot::Xml::PData1273>: mpl::true_
{
	static bool validate(Snapshot::Xml::PData1273::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData1275

namespace Snapshot
{
namespace Xml
{
struct PData1275
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PData1275>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PData1275::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData1277

namespace Snapshot
{
namespace Xml
{
struct PData1277
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PData1277>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PData1277::value_type& value_);

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
	typedef qint16 value_type;
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
// struct PMachine1

namespace Snapshot
{
namespace Xml
{
struct PMachine1
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PMachine1>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PMachine1::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PMachine2

namespace Snapshot
{
namespace Xml
{
struct PMachine2
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PMachine2>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PMachine2::value_type& value_);

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
// struct PPasswdValidTo1

namespace Snapshot
{
namespace Xml
{
struct PPasswdValidTo1
{
	typedef QDateTime value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Traits<Snapshot::Xml::PPasswdValidTo1>
{
	static bool parse(const QString& src_, Snapshot::Xml::PPasswdValidTo1::value_type& dst_);

	static QString generate(const Snapshot::Xml::PPasswdValidTo1::value_type& src_);

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
// struct PReadBytesSec1

namespace Snapshot
{
namespace Xml
{
struct PReadBytesSec1
{
	typedef ulong value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Traits<Snapshot::Xml::PReadBytesSec1>
{
	static bool parse(const QString& src_, Snapshot::Xml::PReadBytesSec1::value_type& dst_);

	static QString generate(Snapshot::Xml::PReadBytesSec1::value_type src_);

};


///////////////////////////////////////////////////////////////////////////////
// struct PWriteBytesSec1

namespace Snapshot
{
namespace Xml
{
struct PWriteBytesSec1
{
	typedef ulong value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Traits<Snapshot::Xml::PWriteBytesSec1>
{
	static bool parse(const QString& src_, Snapshot::Xml::PWriteBytesSec1::value_type& dst_);

	static QString generate(Snapshot::Xml::PWriteBytesSec1::value_type src_);

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
// struct PData1472

namespace Snapshot
{
namespace Xml
{
struct PData1472
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PData1472>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PData1472::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData1474

namespace Snapshot
{
namespace Xml
{
struct PData1474
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PData1474>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PData1474::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData1476

namespace Snapshot
{
namespace Xml
{
struct PData1476
{
	typedef qint32 value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Traits<Snapshot::Xml::PData1476>
{
	static bool parse(const QString& src_, Snapshot::Xml::PData1476::value_type& dst_);

	static QString generate(Snapshot::Xml::PData1476::value_type src_);

};

template<>
struct Validatable<Snapshot::Xml::PData1476>: mpl::true_
{
	static bool validate(Snapshot::Xml::PData1476::value_type value_);

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
// struct PData1478

namespace Snapshot
{
namespace Xml
{
struct PData1478
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PData1478>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PData1478::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData1480

namespace Snapshot
{
namespace Xml
{
struct PData1480
{
	typedef qint32 value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Traits<Snapshot::Xml::PData1480>
{
	static bool parse(const QString& src_, Snapshot::Xml::PData1480::value_type& dst_);

	static QString generate(Snapshot::Xml::PData1480::value_type src_);

};

template<>
struct Validatable<Snapshot::Xml::PData1480>: mpl::true_
{
	static bool validate(Snapshot::Xml::PData1480::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PUnsignedInt1

namespace Snapshot
{
namespace Xml
{
struct PUnsignedInt1
{
	typedef quint32 value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Traits<Snapshot::Xml::PUnsignedInt1>
{
	static bool parse(const QString& src_, Snapshot::Xml::PUnsignedInt1::value_type& dst_);

	static QString generate(Snapshot::Xml::PUnsignedInt1::value_type src_);

};


///////////////////////////////////////////////////////////////////////////////
// struct PUnsignedLong1

namespace Snapshot
{
namespace Xml
{
struct PUnsignedLong1
{
	typedef ulong value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Traits<Snapshot::Xml::PUnsignedLong1>
{
	static bool parse(const QString& src_, Snapshot::Xml::PUnsignedLong1::value_type& dst_);

	static QString generate(Snapshot::Xml::PUnsignedLong1::value_type src_);

};


///////////////////////////////////////////////////////////////////////////////
// struct PPositiveInteger1

namespace Snapshot
{
namespace Xml
{
struct PPositiveInteger1
{
	typedef quint32 value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Traits<Snapshot::Xml::PPositiveInteger1>
{
	static bool parse(const QString& src_, Snapshot::Xml::PPositiveInteger1::value_type& dst_);

	static QString generate(Snapshot::Xml::PPositiveInteger1::value_type src_);

};


///////////////////////////////////////////////////////////////////////////////
// struct POctalMode1

namespace Snapshot
{
namespace Xml
{
struct POctalMode1
{
	typedef quint32 value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Traits<Snapshot::Xml::POctalMode1>
{
	static bool parse(const QString& src_, Snapshot::Xml::POctalMode1::value_type& dst_);

	static QString generate(Snapshot::Xml::POctalMode1::value_type src_);

};


///////////////////////////////////////////////////////////////////////////////
// struct PData1490

namespace Snapshot
{
namespace Xml
{
struct PData1490
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PData1490>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PData1490::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData1492

namespace Snapshot
{
namespace Xml
{
struct PData1492
{
	typedef qint32 value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Traits<Snapshot::Xml::PData1492>
{
	static bool parse(const QString& src_, Snapshot::Xml::PData1492::value_type& dst_);

	static QString generate(Snapshot::Xml::PData1492::value_type src_);

};

template<>
struct Validatable<Snapshot::Xml::PData1492>: mpl::true_
{
	static bool validate(Snapshot::Xml::PData1492::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData1494

namespace Snapshot
{
namespace Xml
{
struct PData1494
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PData1494>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PData1494::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData1496

namespace Snapshot
{
namespace Xml
{
struct PData1496
{
	typedef qint32 value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Traits<Snapshot::Xml::PData1496>
{
	static bool parse(const QString& src_, Snapshot::Xml::PData1496::value_type& dst_);

	static QString generate(Snapshot::Xml::PData1496::value_type src_);

};

template<>
struct Validatable<Snapshot::Xml::PData1496>: mpl::true_
{
	static bool validate(Snapshot::Xml::PData1496::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData1498

namespace Snapshot
{
namespace Xml
{
struct PData1498
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PData1498>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PData1498::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData1500

namespace Snapshot
{
namespace Xml
{
struct PData1500
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PData1500>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PData1500::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PUniMacAddr1

namespace Snapshot
{
namespace Xml
{
struct PUniMacAddr1
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PUniMacAddr1>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PUniMacAddr1::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PMultiMacAddr1

namespace Snapshot
{
namespace Xml
{
struct PMultiMacAddr1
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PMultiMacAddr1>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PMultiMacAddr1::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PMacAddr1

namespace Snapshot
{
namespace Xml
{
struct PMacAddr1
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PMacAddr1>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PMacAddr1::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PDuidLLT1

namespace Snapshot
{
namespace Xml
{
struct PDuidLLT1
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PDuidLLT1>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PDuidLLT1::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PDuidEN1

namespace Snapshot
{
namespace Xml
{
struct PDuidEN1
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PDuidEN1>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PDuidEN1::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PDuidLL1

namespace Snapshot
{
namespace Xml
{
struct PDuidLL1
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PDuidLL1>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PDuidLL1::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PDuidUUID1

namespace Snapshot
{
namespace Xml
{
struct PDuidUUID1
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PDuidUUID1>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PDuidUUID1::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PIpv4Addr1

namespace Snapshot
{
namespace Xml
{
struct PIpv4Addr1
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PIpv4Addr1>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PIpv4Addr1::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PIpv6Addr1

namespace Snapshot
{
namespace Xml
{
struct PIpv6Addr1
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PIpv6Addr1>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PIpv6Addr1::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PIpv4Prefix1

namespace Snapshot
{
namespace Xml
{
struct PIpv4Prefix1
{
	typedef quint32 value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Traits<Snapshot::Xml::PIpv4Prefix1>
{
	static bool parse(const QString& src_, Snapshot::Xml::PIpv4Prefix1::value_type& dst_);

	static QString generate(Snapshot::Xml::PIpv4Prefix1::value_type src_);

};

template<>
struct Validatable<Snapshot::Xml::PIpv4Prefix1>: mpl::true_
{
	static bool validate(Snapshot::Xml::PIpv4Prefix1::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PIpv6Prefix1

namespace Snapshot
{
namespace Xml
{
struct PIpv6Prefix1
{
	typedef quint32 value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Traits<Snapshot::Xml::PIpv6Prefix1>
{
	static bool parse(const QString& src_, Snapshot::Xml::PIpv6Prefix1::value_type& dst_);

	static QString generate(Snapshot::Xml::PIpv6Prefix1::value_type src_);

};

template<>
struct Validatable<Snapshot::Xml::PIpv6Prefix1>: mpl::true_
{
	static bool validate(Snapshot::Xml::PIpv6Prefix1::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PGenericName1

namespace Snapshot
{
namespace Xml
{
struct PGenericName1
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PGenericName1>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PGenericName1::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PDnsName1

namespace Snapshot
{
namespace Xml
{
struct PDnsName1
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PDnsName1>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PDnsName1::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PDeviceName1

namespace Snapshot
{
namespace Xml
{
struct PDeviceName1
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PDeviceName1>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PDeviceName1::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PFilePath1

namespace Snapshot
{
namespace Xml
{
struct PFilePath1
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PFilePath1>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PFilePath1::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PDirPath1

namespace Snapshot
{
namespace Xml
{
struct PDirPath1
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PDirPath1>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PDirPath1::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PAbsFilePath1

namespace Snapshot
{
namespace Xml
{
struct PAbsFilePath1
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PAbsFilePath1>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PAbsFilePath1::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PAbsDirPath1

namespace Snapshot
{
namespace Xml
{
struct PAbsDirPath1
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PAbsDirPath1>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PAbsDirPath1::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PUnit1

namespace Snapshot
{
namespace Xml
{
struct PUnit1
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PUnit1>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PUnit1::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PPciDomain1

namespace Snapshot
{
namespace Xml
{
struct PPciDomain1
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PPciDomain1>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PPciDomain1::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PPciBus1

namespace Snapshot
{
namespace Xml
{
struct PPciBus1
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PPciBus1>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PPciBus1::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PPciSlot1

namespace Snapshot
{
namespace Xml
{
struct PPciSlot1
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PPciSlot1>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PPciSlot1::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PPciFunc1

namespace Snapshot
{
namespace Xml
{
struct PPciFunc1
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PPciFunc1>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PPciFunc1::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PWwn1

namespace Snapshot
{
namespace Xml
{
struct PWwn1
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PWwn1>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PWwn1::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PCpuset1

namespace Snapshot
{
namespace Xml
{
struct PCpuset1
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PCpuset1>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PCpuset1::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PVolName1

namespace Snapshot
{
namespace Xml
{
struct PVolName1
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PVolName1>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PVolName1::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PPortNumber1

namespace Snapshot
{
namespace Xml
{
struct PPortNumber1
{
	typedef qint16 value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Traits<Snapshot::Xml::PPortNumber1>
{
	static bool parse(const QString& src_, Snapshot::Xml::PPortNumber1::value_type& dst_);

	static QString generate(Snapshot::Xml::PPortNumber1::value_type src_);

};

template<>
struct Validatable<Snapshot::Xml::PPortNumber1>: mpl::true_
{
	static bool validate(Snapshot::Xml::PPortNumber1::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PIobase1

namespace Snapshot
{
namespace Xml
{
struct PIobase1
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PIobase1>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PIobase1::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PIrq1

namespace Snapshot
{
namespace Xml
{
struct PIrq1
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PIrq1>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PIrq1::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PCompat1

namespace Snapshot
{
namespace Xml
{
struct PCompat1
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PCompat1>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PCompat1::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PVirtualPortProfileID1

namespace Snapshot
{
namespace Xml
{
struct PVirtualPortProfileID1
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PVirtualPortProfileID1>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PVirtualPortProfileID1::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PSpeed1

namespace Snapshot
{
namespace Xml
{
struct PSpeed1
{
	typedef quint32 value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Traits<Snapshot::Xml::PSpeed1>
{
	static bool parse(const QString& src_, Snapshot::Xml::PSpeed1::value_type& dst_);

	static QString generate(Snapshot::Xml::PSpeed1::value_type src_);

};

template<>
struct Validatable<Snapshot::Xml::PSpeed1>: mpl::true_
{
	static bool validate(Snapshot::Xml::PSpeed1::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PBurstSize1

namespace Snapshot
{
namespace Xml
{
struct PBurstSize1
{
	typedef quint32 value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Traits<Snapshot::Xml::PBurstSize1>
{
	static bool parse(const QString& src_, Snapshot::Xml::PBurstSize1::value_type& dst_);

	static QString generate(Snapshot::Xml::PBurstSize1::value_type src_);

};

template<>
struct Validatable<Snapshot::Xml::PBurstSize1>: mpl::true_
{
	static bool validate(Snapshot::Xml::PBurstSize1::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PUnsignedShort1

namespace Snapshot
{
namespace Xml
{
struct PUnsignedShort1
{
	typedef qint32 value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Traits<Snapshot::Xml::PUnsignedShort1>
{
	static bool parse(const QString& src_, Snapshot::Xml::PUnsignedShort1::value_type& dst_);

	static QString generate(Snapshot::Xml::PUnsignedShort1::value_type src_);

};

template<>
struct Validatable<Snapshot::Xml::PUnsignedShort1>: mpl::true_
{
	static bool validate(Snapshot::Xml::PUnsignedShort1::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PProtocol1

namespace Snapshot
{
namespace Xml
{
struct PProtocol1
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PProtocol1>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PProtocol1::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PAddrFamily1

namespace Snapshot
{
namespace Xml
{
struct PAddrFamily1
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PAddrFamily1>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PAddrFamily1::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PId1

namespace Snapshot
{
namespace Xml
{
struct PId1
{
	typedef quint32 value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Traits<Snapshot::Xml::PId1>
{
	static bool parse(const QString& src_, Snapshot::Xml::PId1::value_type& dst_);

	static QString generate(Snapshot::Xml::PId1::value_type src_);

};

template<>
struct Validatable<Snapshot::Xml::PId1>: mpl::true_
{
	static bool validate(Snapshot::Xml::PId1::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PPort1

namespace Snapshot
{
namespace Xml
{
struct PPort1
{
	typedef qint32 value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Traits<Snapshot::Xml::PPort1>
{
	static bool parse(const QString& src_, Snapshot::Xml::PPort1::value_type& dst_);

	static QString generate(Snapshot::Xml::PPort1::value_type src_);

};

template<>
struct Validatable<Snapshot::Xml::PPort1>: mpl::true_
{
	static bool validate(Snapshot::Xml::PPort1::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PTitle1

namespace Snapshot
{
namespace Xml
{
struct PTitle1
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PTitle1>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PTitle1::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PMachine3

namespace Snapshot
{
namespace Xml
{
struct PMachine3
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PMachine3>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PMachine3::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PMachine4

namespace Snapshot
{
namespace Xml
{
struct PMachine4
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PMachine4>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PMachine4::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PMachine5

namespace Snapshot
{
namespace Xml
{
struct PMachine5
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PMachine5>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PMachine5::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PReadIopsSec2

namespace Snapshot
{
namespace Xml
{
struct PReadIopsSec2
{
	typedef quint32 value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Traits<Snapshot::Xml::PReadIopsSec2>
{
	static bool parse(const QString& src_, Snapshot::Xml::PReadIopsSec2::value_type& dst_);

	static QString generate(Snapshot::Xml::PReadIopsSec2::value_type src_);

};


///////////////////////////////////////////////////////////////////////////////
// struct PWriteIopsSec2

namespace Snapshot
{
namespace Xml
{
struct PWriteIopsSec2
{
	typedef quint32 value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Traits<Snapshot::Xml::PWriteIopsSec2>
{
	static bool parse(const QString& src_, Snapshot::Xml::PWriteIopsSec2::value_type& dst_);

	static QString generate(Snapshot::Xml::PWriteIopsSec2::value_type src_);

};


///////////////////////////////////////////////////////////////////////////////
// struct PReadBytesSec2

namespace Snapshot
{
namespace Xml
{
struct PReadBytesSec2
{
	typedef ulong value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Traits<Snapshot::Xml::PReadBytesSec2>
{
	static bool parse(const QString& src_, Snapshot::Xml::PReadBytesSec2::value_type& dst_);

	static QString generate(Snapshot::Xml::PReadBytesSec2::value_type src_);

};


///////////////////////////////////////////////////////////////////////////////
// struct PWriteBytesSec2

namespace Snapshot
{
namespace Xml
{
struct PWriteBytesSec2
{
	typedef ulong value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Traits<Snapshot::Xml::PWriteBytesSec2>
{
	static bool parse(const QString& src_, Snapshot::Xml::PWriteBytesSec2::value_type& dst_);

	static QString generate(Snapshot::Xml::PWriteBytesSec2::value_type src_);

};


///////////////////////////////////////////////////////////////////////////////
// struct PVendor1

namespace Snapshot
{
namespace Xml
{
struct PVendor1
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PVendor1>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PVendor1::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PProduct1

namespace Snapshot
{
namespace Xml
{
struct PProduct1
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PProduct1>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PProduct1::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PDiskTarget1

namespace Snapshot
{
namespace Xml
{
struct PDiskTarget1
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PDiskTarget1>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PDiskTarget1::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PCyls1

namespace Snapshot
{
namespace Xml
{
struct PCyls1
{
	typedef qint32 value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Traits<Snapshot::Xml::PCyls1>
{
	static bool parse(const QString& src_, Snapshot::Xml::PCyls1::value_type& dst_);

	static QString generate(Snapshot::Xml::PCyls1::value_type src_);

};


///////////////////////////////////////////////////////////////////////////////
// struct PHeads1

namespace Snapshot
{
namespace Xml
{
struct PHeads1
{
	typedef qint32 value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Traits<Snapshot::Xml::PHeads1>
{
	static bool parse(const QString& src_, Snapshot::Xml::PHeads1::value_type& dst_);

	static QString generate(Snapshot::Xml::PHeads1::value_type src_);

};


///////////////////////////////////////////////////////////////////////////////
// struct PSecs1

namespace Snapshot
{
namespace Xml
{
struct PSecs1
{
	typedef qint32 value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Traits<Snapshot::Xml::PSecs1>
{
	static bool parse(const QString& src_, Snapshot::Xml::PSecs1::value_type& dst_);

	static QString generate(Snapshot::Xml::PSecs1::value_type src_);

};


///////////////////////////////////////////////////////////////////////////////
// struct PLogicalBlockSize1

namespace Snapshot
{
namespace Xml
{
struct PLogicalBlockSize1
{
	typedef qint32 value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Traits<Snapshot::Xml::PLogicalBlockSize1>
{
	static bool parse(const QString& src_, Snapshot::Xml::PLogicalBlockSize1::value_type& dst_);

	static QString generate(Snapshot::Xml::PLogicalBlockSize1::value_type src_);

};


///////////////////////////////////////////////////////////////////////////////
// struct PPhysicalBlockSize1

namespace Snapshot
{
namespace Xml
{
struct PPhysicalBlockSize1
{
	typedef qint32 value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Traits<Snapshot::Xml::PPhysicalBlockSize1>
{
	static bool parse(const QString& src_, Snapshot::Xml::PPhysicalBlockSize1::value_type& dst_);

	static QString generate(Snapshot::Xml::PPhysicalBlockSize1::value_type src_);

};


///////////////////////////////////////////////////////////////////////////////
// struct PType1

namespace Snapshot
{
namespace Xml
{
struct PType1
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PType1>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PType1::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PPasswdValidTo2

namespace Snapshot
{
namespace Xml
{
struct PPasswdValidTo2
{
	typedef QDateTime value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Traits<Snapshot::Xml::PPasswdValidTo2>
{
	static bool parse(const QString& src_, Snapshot::Xml::PPasswdValidTo2::value_type& dst_);

	static QString generate(const Snapshot::Xml::PPasswdValidTo2::value_type& src_);

};


///////////////////////////////////////////////////////////////////////////////
// struct PPasswdValidTo3

namespace Snapshot
{
namespace Xml
{
struct PPasswdValidTo3
{
	typedef QDateTime value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Traits<Snapshot::Xml::PPasswdValidTo3>
{
	static bool parse(const QString& src_, Snapshot::Xml::PPasswdValidTo3::value_type& dst_);

	static QString generate(const Snapshot::Xml::PPasswdValidTo3::value_type& src_);

};


///////////////////////////////////////////////////////////////////////////////
// struct PVendorId1

namespace Snapshot
{
namespace Xml
{
struct PVendorId1
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PVendorId1>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PVendorId1::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PSysinfoValue1

namespace Snapshot
{
namespace Xml
{
struct PSysinfoValue1
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PSysinfoValue1>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PSysinfoValue1::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PFilter1

namespace Snapshot
{
namespace Xml
{
struct PFilter1
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot


///////////////////////////////////////////////////////////////////////////////
// struct PTotalBytesSec1

namespace Snapshot
{
namespace Xml
{
struct PTotalBytesSec1
{
	typedef ulong value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Traits<Snapshot::Xml::PTotalBytesSec1>
{
	static bool parse(const QString& src_, Snapshot::Xml::PTotalBytesSec1::value_type& dst_);

	static QString generate(Snapshot::Xml::PTotalBytesSec1::value_type src_);

};


///////////////////////////////////////////////////////////////////////////////
// struct PReadBytesSec3

namespace Snapshot
{
namespace Xml
{
struct PReadBytesSec3
{
	typedef ulong value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Traits<Snapshot::Xml::PReadBytesSec3>
{
	static bool parse(const QString& src_, Snapshot::Xml::PReadBytesSec3::value_type& dst_);

	static QString generate(Snapshot::Xml::PReadBytesSec3::value_type src_);

};


///////////////////////////////////////////////////////////////////////////////
// struct PWriteBytesSec3

namespace Snapshot
{
namespace Xml
{
struct PWriteBytesSec3
{
	typedef ulong value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Traits<Snapshot::Xml::PWriteBytesSec3>
{
	static bool parse(const QString& src_, Snapshot::Xml::PWriteBytesSec3::value_type& dst_);

	static QString generate(Snapshot::Xml::PWriteBytesSec3::value_type src_);

};


///////////////////////////////////////////////////////////////////////////////
// struct PTotalIopsSec1

namespace Snapshot
{
namespace Xml
{
struct PTotalIopsSec1
{
	typedef ulong value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Traits<Snapshot::Xml::PTotalIopsSec1>
{
	static bool parse(const QString& src_, Snapshot::Xml::PTotalIopsSec1::value_type& dst_);

	static QString generate(Snapshot::Xml::PTotalIopsSec1::value_type src_);

};


///////////////////////////////////////////////////////////////////////////////
// struct PReadIopsSec3

namespace Snapshot
{
namespace Xml
{
struct PReadIopsSec3
{
	typedef ulong value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Traits<Snapshot::Xml::PReadIopsSec3>
{
	static bool parse(const QString& src_, Snapshot::Xml::PReadIopsSec3::value_type& dst_);

	static QString generate(Snapshot::Xml::PReadIopsSec3::value_type src_);

};


///////////////////////////////////////////////////////////////////////////////
// struct PWriteIopsSec3

namespace Snapshot
{
namespace Xml
{
struct PWriteIopsSec3
{
	typedef ulong value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Traits<Snapshot::Xml::PWriteIopsSec3>
{
	static bool parse(const QString& src_, Snapshot::Xml::PWriteIopsSec3::value_type& dst_);

	static QString generate(Snapshot::Xml::PWriteIopsSec3::value_type src_);

};


///////////////////////////////////////////////////////////////////////////////
// struct PRetries1

namespace Snapshot
{
namespace Xml
{
struct PRetries1
{
	typedef quint32 value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Traits<Snapshot::Xml::PRetries1>
{
	static bool parse(const QString& src_, Snapshot::Xml::PRetries1::value_type& dst_);

	static QString generate(Snapshot::Xml::PRetries1::value_type src_);

};


///////////////////////////////////////////////////////////////////////////////
// struct PCountCPU1

namespace Snapshot
{
namespace Xml
{
struct PCountCPU1
{
	typedef quint16 value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Traits<Snapshot::Xml::PCountCPU1>
{
	static bool parse(const QString& src_, Snapshot::Xml::PCountCPU1::value_type& dst_);

	static QString generate(Snapshot::Xml::PCountCPU1::value_type src_);

};

template<>
struct Validatable<Snapshot::Xml::PCountCPU1>: mpl::true_
{
	static bool validate(Snapshot::Xml::PCountCPU1::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PVcpuid1

namespace Snapshot
{
namespace Xml
{
struct PVcpuid1
{
	typedef quint16 value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Traits<Snapshot::Xml::PVcpuid1>
{
	static bool parse(const QString& src_, Snapshot::Xml::PVcpuid1::value_type& dst_);

	static QString generate(Snapshot::Xml::PVcpuid1::value_type src_);

};


///////////////////////////////////////////////////////////////////////////////
// struct PCpushares1

namespace Snapshot
{
namespace Xml
{
struct PCpushares1
{
	typedef quint32 value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Traits<Snapshot::Xml::PCpushares1>
{
	static bool parse(const QString& src_, Snapshot::Xml::PCpushares1::value_type& dst_);

	static QString generate(Snapshot::Xml::PCpushares1::value_type src_);

};


///////////////////////////////////////////////////////////////////////////////
// struct PCpuperiod1

namespace Snapshot
{
namespace Xml
{
struct PCpuperiod1
{
	typedef ulong value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Traits<Snapshot::Xml::PCpuperiod1>
{
	static bool parse(const QString& src_, Snapshot::Xml::PCpuperiod1::value_type& dst_);

	static QString generate(Snapshot::Xml::PCpuperiod1::value_type src_);

};

template<>
struct Validatable<Snapshot::Xml::PCpuperiod1>: mpl::true_
{
	static bool validate(Snapshot::Xml::PCpuperiod1::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PCpuquota1

namespace Snapshot
{
namespace Xml
{
struct PCpuquota1
{
	typedef long value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Traits<Snapshot::Xml::PCpuquota1>
{
	static bool parse(const QString& src_, Snapshot::Xml::PCpuquota1::value_type& dst_);

	static QString generate(Snapshot::Xml::PCpuquota1::value_type src_);

};

template<>
struct Validatable<Snapshot::Xml::PCpuquota1>: mpl::true_
{
	static bool validate(Snapshot::Xml::PCpuquota1::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PRebootTimeoutDelay1

namespace Snapshot
{
namespace Xml
{
struct PRebootTimeoutDelay1
{
	typedef qint16 value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Traits<Snapshot::Xml::PRebootTimeoutDelay1>
{
	static bool parse(const QString& src_, Snapshot::Xml::PRebootTimeoutDelay1::value_type& dst_);

	static QString generate(Snapshot::Xml::PRebootTimeoutDelay1::value_type src_);

};

template<>
struct Validatable<Snapshot::Xml::PRebootTimeoutDelay1>: mpl::true_
{
	static bool validate(Snapshot::Xml::PRebootTimeoutDelay1::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PWeight1

namespace Snapshot
{
namespace Xml
{
struct PWeight1
{
	typedef quint32 value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Traits<Snapshot::Xml::PWeight1>
{
	static bool parse(const QString& src_, Snapshot::Xml::PWeight1::value_type& dst_);

	static QString generate(Snapshot::Xml::PWeight1::value_type src_);

};

template<>
struct Validatable<Snapshot::Xml::PWeight1>: mpl::true_
{
	static bool validate(Snapshot::Xml::PWeight1::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PMemoryKB1

namespace Snapshot
{
namespace Xml
{
struct PMemoryKB1
{
	typedef quint32 value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Traits<Snapshot::Xml::PMemoryKB1>
{
	static bool parse(const QString& src_, Snapshot::Xml::PMemoryKB1::value_type& dst_);

	static QString generate(Snapshot::Xml::PMemoryKB1::value_type src_);

};


///////////////////////////////////////////////////////////////////////////////
// struct PDomainName1

namespace Snapshot
{
namespace Xml
{
struct PDomainName1
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PDomainName1>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PDomainName1::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PDiskSerial1

namespace Snapshot
{
namespace Xml
{
struct PDiskSerial1
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PDiskSerial1>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PDiskSerial1::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PBridgeMode1

namespace Snapshot
{
namespace Xml
{
struct PBridgeMode1
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PBridgeMode1>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PBridgeMode1::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PAddrIPorName1

namespace Snapshot
{
namespace Xml
{
struct PAddrIPorName1
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PAddrIPorName1>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PAddrIPorName1::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PUsbIdDefault1

namespace Snapshot
{
namespace Xml
{
struct PUsbIdDefault1
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PUsbIdDefault1>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PUsbIdDefault1::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PUsbId1

namespace Snapshot
{
namespace Xml
{
struct PUsbId1
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PUsbId1>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PUsbId1::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PUsbVersion1

namespace Snapshot
{
namespace Xml
{
struct PUsbVersion1
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PUsbVersion1>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PUsbVersion1::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PUsbAddr1

namespace Snapshot
{
namespace Xml
{
struct PUsbAddr1
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PUsbAddr1>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PUsbAddr1::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PUsbClass1

namespace Snapshot
{
namespace Xml
{
struct PUsbClass1
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PUsbClass1>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PUsbClass1::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PUsbPort1

namespace Snapshot
{
namespace Xml
{
struct PUsbPort1
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PUsbPort1>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PUsbPort1::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PDriveController1

namespace Snapshot
{
namespace Xml
{
struct PDriveController1
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PDriveController1>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PDriveController1::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PDriveBus1

namespace Snapshot
{
namespace Xml
{
struct PDriveBus1
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PDriveBus1>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PDriveBus1::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PDriveTarget1

namespace Snapshot
{
namespace Xml
{
struct PDriveTarget1
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PDriveTarget1>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PDriveTarget1::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PDriveUnit1

namespace Snapshot
{
namespace Xml
{
struct PDriveUnit1
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PDriveUnit1>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PDriveUnit1::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PFeatureName1

namespace Snapshot
{
namespace Xml
{
struct PFeatureName1
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PFeatureName1>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PFeatureName1::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PTimeDelta1

namespace Snapshot
{
namespace Xml
{
struct PTimeDelta1
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PTimeDelta1>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PTimeDelta1::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PTimeZone1

namespace Snapshot
{
namespace Xml
{
struct PTimeZone1
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PTimeZone1>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PTimeZone1::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PFilterParamName1

namespace Snapshot
{
namespace Xml
{
struct PFilterParamName1
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PFilterParamName1>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PFilterParamName1::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PFilterParamValue1

namespace Snapshot
{
namespace Xml
{
struct PFilterParamValue1
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PFilterParamValue1>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PFilterParamValue1::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PSpaprvioReg1

namespace Snapshot
{
namespace Xml
{
struct PSpaprvioReg1
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PSpaprvioReg1>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PSpaprvioReg1::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PAliasName1

namespace Snapshot
{
namespace Xml
{
struct PAliasName1
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PAliasName1>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PAliasName1::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData1695

namespace Snapshot
{
namespace Xml
{
struct PData1695
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PData1695>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PData1695::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData1697

namespace Snapshot
{
namespace Xml
{
struct PData1697
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PData1697>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PData1697::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData1699

namespace Snapshot
{
namespace Xml
{
struct PData1699
{
	typedef qint32 value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Traits<Snapshot::Xml::PData1699>
{
	static bool parse(const QString& src_, Snapshot::Xml::PData1699::value_type& dst_);

	static QString generate(Snapshot::Xml::PData1699::value_type src_);

};

template<>
struct Validatable<Snapshot::Xml::PData1699>: mpl::true_
{
	static bool validate(Snapshot::Xml::PData1699::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PCcwSsidRange1

namespace Snapshot
{
namespace Xml
{
struct PCcwSsidRange1
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PCcwSsidRange1>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PCcwSsidRange1::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData1701

namespace Snapshot
{
namespace Xml
{
struct PData1701
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Validatable<Snapshot::Xml::PData1701>: mpl::true_
{
	static bool validate(const Snapshot::Xml::PData1701::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData1703

namespace Snapshot
{
namespace Xml
{
struct PData1703
{
	typedef qint32 value_type;
};

} // namespace Xml
} // namespace Snapshot

template<>
struct Traits<Snapshot::Xml::PData1703>
{
	static bool parse(const QString& src_, Snapshot::Xml::PData1703::value_type& dst_);

	static QString generate(Snapshot::Xml::PData1703::value_type src_);

};

template<>
struct Validatable<Snapshot::Xml::PData1703>: mpl::true_
{
	static bool validate(Snapshot::Xml::PData1703::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct VName

namespace Snapshot
{
namespace Xml
{
typedef Choice<mpl::vector<PDiskTarget1, PAbsFilePath1 > > VNameImpl;
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
typedef Choice<mpl::vector<PIpv4Addr1, PIpv6Addr1 > > VIpAddrImpl;
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
typedef Choice<mpl::vector<PDnsName1, VIpAddr > > VName1Impl;
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
typedef Choice<mpl::vector<PData1498, PData1500 > > VUUIDImpl;
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
