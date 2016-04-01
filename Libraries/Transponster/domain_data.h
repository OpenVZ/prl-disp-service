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

#ifndef __DOMAIN_DATA_H__
#define __DOMAIN_DATA_H__
#include <QString>
#include <QDateTime>
#include "base.h"
#include "patterns.h"
#include "domain_enum.h"

namespace Libvirt
{
///////////////////////////////////////////////////////////////////////////////
// struct PUnsignedInt

namespace Domain
{
namespace Xml
{
struct PUnsignedInt
{
	typedef quint32 value_type;
};

} // namespace Xml
} // namespace Domain

template<>
struct Traits<Domain::Xml::PUnsignedInt>
{
	static bool parse(const QString& src_, Domain::Xml::PUnsignedInt::value_type& dst_);

	static QString generate(Domain::Xml::PUnsignedInt::value_type src_);

};


///////////////////////////////////////////////////////////////////////////////
// struct PUnsignedLong

namespace Domain
{
namespace Xml
{
struct PUnsignedLong
{
	typedef ulong value_type;
};

} // namespace Xml
} // namespace Domain

template<>
struct Traits<Domain::Xml::PUnsignedLong>
{
	static bool parse(const QString& src_, Domain::Xml::PUnsignedLong::value_type& dst_);

	static QString generate(Domain::Xml::PUnsignedLong::value_type src_);

};


///////////////////////////////////////////////////////////////////////////////
// struct PPositiveInteger

namespace Domain
{
namespace Xml
{
struct PPositiveInteger
{
	typedef quint32 value_type;
};

} // namespace Xml
} // namespace Domain

template<>
struct Traits<Domain::Xml::PPositiveInteger>
{
	static bool parse(const QString& src_, Domain::Xml::PPositiveInteger::value_type& dst_);

	static QString generate(Domain::Xml::PPositiveInteger::value_type src_);

};


///////////////////////////////////////////////////////////////////////////////
// struct POctalMode

namespace Domain
{
namespace Xml
{
struct POctalMode
{
	typedef quint32 value_type;
};

} // namespace Xml
} // namespace Domain

template<>
struct Traits<Domain::Xml::POctalMode>
{
	static bool parse(const QString& src_, Domain::Xml::POctalMode::value_type& dst_);

	static QString generate(Domain::Xml::POctalMode::value_type src_);

};


///////////////////////////////////////////////////////////////////////////////
// struct PData6

namespace Domain
{
namespace Xml
{
struct PData6
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Domain

template<>
struct Validatable<Domain::Xml::PData6>: mpl::true_
{
	static bool validate(const Domain::Xml::PData6::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData8

namespace Domain
{
namespace Xml
{
struct PData8
{
	typedef qint32 value_type;
};

} // namespace Xml
} // namespace Domain

template<>
struct Traits<Domain::Xml::PData8>
{
	static bool parse(const QString& src_, Domain::Xml::PData8::value_type& dst_);

	static QString generate(Domain::Xml::PData8::value_type src_);

};

template<>
struct Validatable<Domain::Xml::PData8>: mpl::true_
{
	static bool validate(Domain::Xml::PData8::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData11

namespace Domain
{
namespace Xml
{
struct PData11
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Domain

template<>
struct Validatable<Domain::Xml::PData11>: mpl::true_
{
	static bool validate(const Domain::Xml::PData11::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData13

namespace Domain
{
namespace Xml
{
struct PData13
{
	typedef qint32 value_type;
};

} // namespace Xml
} // namespace Domain

template<>
struct Traits<Domain::Xml::PData13>
{
	static bool parse(const QString& src_, Domain::Xml::PData13::value_type& dst_);

	static QString generate(Domain::Xml::PData13::value_type src_);

};

template<>
struct Validatable<Domain::Xml::PData13>: mpl::true_
{
	static bool validate(Domain::Xml::PData13::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData16

namespace Domain
{
namespace Xml
{
struct PData16
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Domain

template<>
struct Validatable<Domain::Xml::PData16>: mpl::true_
{
	static bool validate(const Domain::Xml::PData16::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData18

namespace Domain
{
namespace Xml
{
struct PData18
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Domain

template<>
struct Validatable<Domain::Xml::PData18>: mpl::true_
{
	static bool validate(const Domain::Xml::PData18::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PUniMacAddr

namespace Domain
{
namespace Xml
{
struct PUniMacAddr
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Domain

template<>
struct Validatable<Domain::Xml::PUniMacAddr>: mpl::true_
{
	static bool validate(const Domain::Xml::PUniMacAddr::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PMultiMacAddr

namespace Domain
{
namespace Xml
{
struct PMultiMacAddr
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Domain

template<>
struct Validatable<Domain::Xml::PMultiMacAddr>: mpl::true_
{
	static bool validate(const Domain::Xml::PMultiMacAddr::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PMacAddr

namespace Domain
{
namespace Xml
{
struct PMacAddr
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Domain

template<>
struct Validatable<Domain::Xml::PMacAddr>: mpl::true_
{
	static bool validate(const Domain::Xml::PMacAddr::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PDuidLLT

namespace Domain
{
namespace Xml
{
struct PDuidLLT
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Domain

template<>
struct Validatable<Domain::Xml::PDuidLLT>: mpl::true_
{
	static bool validate(const Domain::Xml::PDuidLLT::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PDuidEN

namespace Domain
{
namespace Xml
{
struct PDuidEN
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Domain

template<>
struct Validatable<Domain::Xml::PDuidEN>: mpl::true_
{
	static bool validate(const Domain::Xml::PDuidEN::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PDuidLL

namespace Domain
{
namespace Xml
{
struct PDuidLL
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Domain

template<>
struct Validatable<Domain::Xml::PDuidLL>: mpl::true_
{
	static bool validate(const Domain::Xml::PDuidLL::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PDuidUUID

namespace Domain
{
namespace Xml
{
struct PDuidUUID
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Domain

template<>
struct Validatable<Domain::Xml::PDuidUUID>: mpl::true_
{
	static bool validate(const Domain::Xml::PDuidUUID::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PIpv4Addr

namespace Domain
{
namespace Xml
{
struct PIpv4Addr
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Domain

template<>
struct Validatable<Domain::Xml::PIpv4Addr>: mpl::true_
{
	static bool validate(const Domain::Xml::PIpv4Addr::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PIpv6Addr

namespace Domain
{
namespace Xml
{
struct PIpv6Addr
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Domain

template<>
struct Validatable<Domain::Xml::PIpv6Addr>: mpl::true_
{
	static bool validate(const Domain::Xml::PIpv6Addr::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PIpv4Prefix

namespace Domain
{
namespace Xml
{
struct PIpv4Prefix
{
	typedef quint32 value_type;
};

} // namespace Xml
} // namespace Domain

template<>
struct Traits<Domain::Xml::PIpv4Prefix>
{
	static bool parse(const QString& src_, Domain::Xml::PIpv4Prefix::value_type& dst_);

	static QString generate(Domain::Xml::PIpv4Prefix::value_type src_);

};

template<>
struct Validatable<Domain::Xml::PIpv4Prefix>: mpl::true_
{
	static bool validate(Domain::Xml::PIpv4Prefix::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PIpv6Prefix

namespace Domain
{
namespace Xml
{
struct PIpv6Prefix
{
	typedef quint32 value_type;
};

} // namespace Xml
} // namespace Domain

template<>
struct Traits<Domain::Xml::PIpv6Prefix>
{
	static bool parse(const QString& src_, Domain::Xml::PIpv6Prefix::value_type& dst_);

	static QString generate(Domain::Xml::PIpv6Prefix::value_type src_);

};

template<>
struct Validatable<Domain::Xml::PIpv6Prefix>: mpl::true_
{
	static bool validate(Domain::Xml::PIpv6Prefix::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PGenericName

namespace Domain
{
namespace Xml
{
struct PGenericName
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Domain

template<>
struct Validatable<Domain::Xml::PGenericName>: mpl::true_
{
	static bool validate(const Domain::Xml::PGenericName::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PDnsName

namespace Domain
{
namespace Xml
{
struct PDnsName
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Domain

template<>
struct Validatable<Domain::Xml::PDnsName>: mpl::true_
{
	static bool validate(const Domain::Xml::PDnsName::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PDeviceName

namespace Domain
{
namespace Xml
{
struct PDeviceName
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Domain

template<>
struct Validatable<Domain::Xml::PDeviceName>: mpl::true_
{
	static bool validate(const Domain::Xml::PDeviceName::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PFilePath

namespace Domain
{
namespace Xml
{
struct PFilePath
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Domain

template<>
struct Validatable<Domain::Xml::PFilePath>: mpl::true_
{
	static bool validate(const Domain::Xml::PFilePath::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PDirPath

namespace Domain
{
namespace Xml
{
struct PDirPath
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Domain

template<>
struct Validatable<Domain::Xml::PDirPath>: mpl::true_
{
	static bool validate(const Domain::Xml::PDirPath::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PAbsFilePath

namespace Domain
{
namespace Xml
{
struct PAbsFilePath
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Domain

template<>
struct Validatable<Domain::Xml::PAbsFilePath>: mpl::true_
{
	static bool validate(const Domain::Xml::PAbsFilePath::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PAbsDirPath

namespace Domain
{
namespace Xml
{
struct PAbsDirPath
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Domain

template<>
struct Validatable<Domain::Xml::PAbsDirPath>: mpl::true_
{
	static bool validate(const Domain::Xml::PAbsDirPath::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PUnit

namespace Domain
{
namespace Xml
{
struct PUnit
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Domain

template<>
struct Validatable<Domain::Xml::PUnit>: mpl::true_
{
	static bool validate(const Domain::Xml::PUnit::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PPciDomain

namespace Domain
{
namespace Xml
{
struct PPciDomain
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Domain

template<>
struct Validatable<Domain::Xml::PPciDomain>: mpl::true_
{
	static bool validate(const Domain::Xml::PPciDomain::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PPciBus

namespace Domain
{
namespace Xml
{
struct PPciBus
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Domain

template<>
struct Validatable<Domain::Xml::PPciBus>: mpl::true_
{
	static bool validate(const Domain::Xml::PPciBus::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PPciSlot

namespace Domain
{
namespace Xml
{
struct PPciSlot
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Domain

template<>
struct Validatable<Domain::Xml::PPciSlot>: mpl::true_
{
	static bool validate(const Domain::Xml::PPciSlot::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PPciFunc

namespace Domain
{
namespace Xml
{
struct PPciFunc
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Domain

template<>
struct Validatable<Domain::Xml::PPciFunc>: mpl::true_
{
	static bool validate(const Domain::Xml::PPciFunc::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PWwn

namespace Domain
{
namespace Xml
{
struct PWwn
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Domain

template<>
struct Validatable<Domain::Xml::PWwn>: mpl::true_
{
	static bool validate(const Domain::Xml::PWwn::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PCpuset

namespace Domain
{
namespace Xml
{
struct PCpuset
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Domain

template<>
struct Validatable<Domain::Xml::PCpuset>: mpl::true_
{
	static bool validate(const Domain::Xml::PCpuset::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PVolName

namespace Domain
{
namespace Xml
{
struct PVolName
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Domain

template<>
struct Validatable<Domain::Xml::PVolName>: mpl::true_
{
	static bool validate(const Domain::Xml::PVolName::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PPortNumber

namespace Domain
{
namespace Xml
{
struct PPortNumber
{
	typedef qint32 value_type;
};

} // namespace Xml
} // namespace Domain

template<>
struct Traits<Domain::Xml::PPortNumber>
{
	static bool parse(const QString& src_, Domain::Xml::PPortNumber::value_type& dst_);

	static QString generate(Domain::Xml::PPortNumber::value_type src_);

};

template<>
struct Validatable<Domain::Xml::PPortNumber>: mpl::true_
{
	static bool validate(Domain::Xml::PPortNumber::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PIobase

namespace Domain
{
namespace Xml
{
struct PIobase
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Domain

template<>
struct Validatable<Domain::Xml::PIobase>: mpl::true_
{
	static bool validate(const Domain::Xml::PIobase::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PIrq

namespace Domain
{
namespace Xml
{
struct PIrq
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Domain

template<>
struct Validatable<Domain::Xml::PIrq>: mpl::true_
{
	static bool validate(const Domain::Xml::PIrq::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PCompat

namespace Domain
{
namespace Xml
{
struct PCompat
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Domain

template<>
struct Validatable<Domain::Xml::PCompat>: mpl::true_
{
	static bool validate(const Domain::Xml::PCompat::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PVirtualPortProfileID

namespace Domain
{
namespace Xml
{
struct PVirtualPortProfileID
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Domain

template<>
struct Validatable<Domain::Xml::PVirtualPortProfileID>: mpl::true_
{
	static bool validate(const Domain::Xml::PVirtualPortProfileID::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PSpeed

namespace Domain
{
namespace Xml
{
struct PSpeed
{
	typedef quint32 value_type;
};

} // namespace Xml
} // namespace Domain

template<>
struct Traits<Domain::Xml::PSpeed>
{
	static bool parse(const QString& src_, Domain::Xml::PSpeed::value_type& dst_);

	static QString generate(Domain::Xml::PSpeed::value_type src_);

};

template<>
struct Validatable<Domain::Xml::PSpeed>: mpl::true_
{
	static bool validate(Domain::Xml::PSpeed::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PBurstSize

namespace Domain
{
namespace Xml
{
struct PBurstSize
{
	typedef quint32 value_type;
};

} // namespace Xml
} // namespace Domain

template<>
struct Traits<Domain::Xml::PBurstSize>
{
	static bool parse(const QString& src_, Domain::Xml::PBurstSize::value_type& dst_);

	static QString generate(Domain::Xml::PBurstSize::value_type src_);

};

template<>
struct Validatable<Domain::Xml::PBurstSize>: mpl::true_
{
	static bool validate(Domain::Xml::PBurstSize::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PUnsignedShort

namespace Domain
{
namespace Xml
{
struct PUnsignedShort
{
	typedef qint32 value_type;
};

} // namespace Xml
} // namespace Domain

template<>
struct Traits<Domain::Xml::PUnsignedShort>
{
	static bool parse(const QString& src_, Domain::Xml::PUnsignedShort::value_type& dst_);

	static QString generate(Domain::Xml::PUnsignedShort::value_type src_);

};

template<>
struct Validatable<Domain::Xml::PUnsignedShort>: mpl::true_
{
	static bool validate(Domain::Xml::PUnsignedShort::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PProtocol

namespace Domain
{
namespace Xml
{
struct PProtocol
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Domain

template<>
struct Validatable<Domain::Xml::PProtocol>: mpl::true_
{
	static bool validate(const Domain::Xml::PProtocol::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PAddrFamily

namespace Domain
{
namespace Xml
{
struct PAddrFamily
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Domain

template<>
struct Validatable<Domain::Xml::PAddrFamily>: mpl::true_
{
	static bool validate(const Domain::Xml::PAddrFamily::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PId

namespace Domain
{
namespace Xml
{
struct PId
{
	typedef quint32 value_type;
};

} // namespace Xml
} // namespace Domain

template<>
struct Traits<Domain::Xml::PId>
{
	static bool parse(const QString& src_, Domain::Xml::PId::value_type& dst_);

	static QString generate(Domain::Xml::PId::value_type src_);

};

template<>
struct Validatable<Domain::Xml::PId>: mpl::true_
{
	static bool validate(Domain::Xml::PId::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PPort

namespace Domain
{
namespace Xml
{
struct PPort
{
	typedef qint32 value_type;
};

} // namespace Xml
} // namespace Domain

template<>
struct Traits<Domain::Xml::PPort>
{
	static bool parse(const QString& src_, Domain::Xml::PPort::value_type& dst_);

	static QString generate(Domain::Xml::PPort::value_type src_);

};

template<>
struct Validatable<Domain::Xml::PPort>: mpl::true_
{
	static bool validate(Domain::Xml::PPort::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PTitle

namespace Domain
{
namespace Xml
{
struct PTitle
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Domain

template<>
struct Validatable<Domain::Xml::PTitle>: mpl::true_
{
	static bool validate(const Domain::Xml::PTitle::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PMachine

namespace Domain
{
namespace Xml
{
struct PMachine
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Domain

template<>
struct Validatable<Domain::Xml::PMachine>: mpl::true_
{
	static bool validate(const Domain::Xml::PMachine::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PReadIopsSec

namespace Domain
{
namespace Xml
{
struct PReadIopsSec
{
	typedef quint32 value_type;
};

} // namespace Xml
} // namespace Domain

template<>
struct Traits<Domain::Xml::PReadIopsSec>
{
	static bool parse(const QString& src_, Domain::Xml::PReadIopsSec::value_type& dst_);

	static QString generate(Domain::Xml::PReadIopsSec::value_type src_);

};


///////////////////////////////////////////////////////////////////////////////
// struct PWriteIopsSec

namespace Domain
{
namespace Xml
{
struct PWriteIopsSec
{
	typedef quint32 value_type;
};

} // namespace Xml
} // namespace Domain

template<>
struct Traits<Domain::Xml::PWriteIopsSec>
{
	static bool parse(const QString& src_, Domain::Xml::PWriteIopsSec::value_type& dst_);

	static QString generate(Domain::Xml::PWriteIopsSec::value_type src_);

};


///////////////////////////////////////////////////////////////////////////////
// struct PReadBytesSec

namespace Domain
{
namespace Xml
{
struct PReadBytesSec
{
	typedef ulong value_type;
};

} // namespace Xml
} // namespace Domain

template<>
struct Traits<Domain::Xml::PReadBytesSec>
{
	static bool parse(const QString& src_, Domain::Xml::PReadBytesSec::value_type& dst_);

	static QString generate(Domain::Xml::PReadBytesSec::value_type src_);

};


///////////////////////////////////////////////////////////////////////////////
// struct PWriteBytesSec

namespace Domain
{
namespace Xml
{
struct PWriteBytesSec
{
	typedef ulong value_type;
};

} // namespace Xml
} // namespace Domain

template<>
struct Traits<Domain::Xml::PWriteBytesSec>
{
	static bool parse(const QString& src_, Domain::Xml::PWriteBytesSec::value_type& dst_);

	static QString generate(Domain::Xml::PWriteBytesSec::value_type src_);

};


///////////////////////////////////////////////////////////////////////////////
// struct PVendor

namespace Domain
{
namespace Xml
{
struct PVendor
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Domain

template<>
struct Validatable<Domain::Xml::PVendor>: mpl::true_
{
	static bool validate(const Domain::Xml::PVendor::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PProduct

namespace Domain
{
namespace Xml
{
struct PProduct
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Domain

template<>
struct Validatable<Domain::Xml::PProduct>: mpl::true_
{
	static bool validate(const Domain::Xml::PProduct::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PDiskTarget

namespace Domain
{
namespace Xml
{
struct PDiskTarget
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Domain

template<>
struct Validatable<Domain::Xml::PDiskTarget>: mpl::true_
{
	static bool validate(const Domain::Xml::PDiskTarget::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PCyls

namespace Domain
{
namespace Xml
{
struct PCyls
{
	typedef qint32 value_type;
};

} // namespace Xml
} // namespace Domain

template<>
struct Traits<Domain::Xml::PCyls>
{
	static bool parse(const QString& src_, Domain::Xml::PCyls::value_type& dst_);

	static QString generate(Domain::Xml::PCyls::value_type src_);

};


///////////////////////////////////////////////////////////////////////////////
// struct PHeads

namespace Domain
{
namespace Xml
{
struct PHeads
{
	typedef qint32 value_type;
};

} // namespace Xml
} // namespace Domain

template<>
struct Traits<Domain::Xml::PHeads>
{
	static bool parse(const QString& src_, Domain::Xml::PHeads::value_type& dst_);

	static QString generate(Domain::Xml::PHeads::value_type src_);

};


///////////////////////////////////////////////////////////////////////////////
// struct PSecs

namespace Domain
{
namespace Xml
{
struct PSecs
{
	typedef qint32 value_type;
};

} // namespace Xml
} // namespace Domain

template<>
struct Traits<Domain::Xml::PSecs>
{
	static bool parse(const QString& src_, Domain::Xml::PSecs::value_type& dst_);

	static QString generate(Domain::Xml::PSecs::value_type src_);

};


///////////////////////////////////////////////////////////////////////////////
// struct PLogicalBlockSize

namespace Domain
{
namespace Xml
{
struct PLogicalBlockSize
{
	typedef qint32 value_type;
};

} // namespace Xml
} // namespace Domain

template<>
struct Traits<Domain::Xml::PLogicalBlockSize>
{
	static bool parse(const QString& src_, Domain::Xml::PLogicalBlockSize::value_type& dst_);

	static QString generate(Domain::Xml::PLogicalBlockSize::value_type src_);

};


///////////////////////////////////////////////////////////////////////////////
// struct PPhysicalBlockSize

namespace Domain
{
namespace Xml
{
struct PPhysicalBlockSize
{
	typedef qint32 value_type;
};

} // namespace Xml
} // namespace Domain

template<>
struct Traits<Domain::Xml::PPhysicalBlockSize>
{
	static bool parse(const QString& src_, Domain::Xml::PPhysicalBlockSize::value_type& dst_);

	static QString generate(Domain::Xml::PPhysicalBlockSize::value_type src_);

};


///////////////////////////////////////////////////////////////////////////////
// struct PType

namespace Domain
{
namespace Xml
{
struct PType
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Domain

template<>
struct Validatable<Domain::Xml::PType>: mpl::true_
{
	static bool validate(const Domain::Xml::PType::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PPasswdValidTo

namespace Domain
{
namespace Xml
{
struct PPasswdValidTo
{
	typedef QDateTime value_type;
};

} // namespace Xml
} // namespace Domain

template<>
struct Traits<Domain::Xml::PPasswdValidTo>
{
	static bool parse(const QString& src_, Domain::Xml::PPasswdValidTo::value_type& dst_);

	static QString generate(const Domain::Xml::PPasswdValidTo::value_type& src_);

};


///////////////////////////////////////////////////////////////////////////////
// struct PVendorId

namespace Domain
{
namespace Xml
{
struct PVendorId
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Domain

template<>
struct Validatable<Domain::Xml::PVendorId>: mpl::true_
{
	static bool validate(const Domain::Xml::PVendorId::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PSysinfoValue

namespace Domain
{
namespace Xml
{
struct PSysinfoValue
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Domain

template<>
struct Validatable<Domain::Xml::PSysinfoValue>: mpl::true_
{
	static bool validate(const Domain::Xml::PSysinfoValue::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PFilter

namespace Domain
{
namespace Xml
{
struct PFilter
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Domain


///////////////////////////////////////////////////////////////////////////////
// struct PTotalBytesSec

namespace Domain
{
namespace Xml
{
struct PTotalBytesSec
{
	typedef ulong value_type;
};

} // namespace Xml
} // namespace Domain

template<>
struct Traits<Domain::Xml::PTotalBytesSec>
{
	static bool parse(const QString& src_, Domain::Xml::PTotalBytesSec::value_type& dst_);

	static QString generate(Domain::Xml::PTotalBytesSec::value_type src_);

};


///////////////////////////////////////////////////////////////////////////////
// struct PTotalIopsSec

namespace Domain
{
namespace Xml
{
struct PTotalIopsSec
{
	typedef ulong value_type;
};

} // namespace Xml
} // namespace Domain

template<>
struct Traits<Domain::Xml::PTotalIopsSec>
{
	static bool parse(const QString& src_, Domain::Xml::PTotalIopsSec::value_type& dst_);

	static QString generate(Domain::Xml::PTotalIopsSec::value_type src_);

};


///////////////////////////////////////////////////////////////////////////////
// struct PReadIopsSec1

namespace Domain
{
namespace Xml
{
struct PReadIopsSec1
{
	typedef ulong value_type;
};

} // namespace Xml
} // namespace Domain

template<>
struct Traits<Domain::Xml::PReadIopsSec1>
{
	static bool parse(const QString& src_, Domain::Xml::PReadIopsSec1::value_type& dst_);

	static QString generate(Domain::Xml::PReadIopsSec1::value_type src_);

};


///////////////////////////////////////////////////////////////////////////////
// struct PWriteIopsSec1

namespace Domain
{
namespace Xml
{
struct PWriteIopsSec1
{
	typedef ulong value_type;
};

} // namespace Xml
} // namespace Domain

template<>
struct Traits<Domain::Xml::PWriteIopsSec1>
{
	static bool parse(const QString& src_, Domain::Xml::PWriteIopsSec1::value_type& dst_);

	static QString generate(Domain::Xml::PWriteIopsSec1::value_type src_);

};


///////////////////////////////////////////////////////////////////////////////
// struct PRetries

namespace Domain
{
namespace Xml
{
struct PRetries
{
	typedef quint32 value_type;
};

} // namespace Xml
} // namespace Domain

template<>
struct Traits<Domain::Xml::PRetries>
{
	static bool parse(const QString& src_, Domain::Xml::PRetries::value_type& dst_);

	static QString generate(Domain::Xml::PRetries::value_type src_);

};


///////////////////////////////////////////////////////////////////////////////
// struct PValue

namespace Domain
{
namespace Xml
{
struct PValue
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Domain

template<>
struct Validatable<Domain::Xml::PValue>: mpl::true_
{
	static bool validate(const Domain::Xml::PValue::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PCountCPU

namespace Domain
{
namespace Xml
{
struct PCountCPU
{
	typedef quint16 value_type;
};

} // namespace Xml
} // namespace Domain

template<>
struct Traits<Domain::Xml::PCountCPU>
{
	static bool parse(const QString& src_, Domain::Xml::PCountCPU::value_type& dst_);

	static QString generate(Domain::Xml::PCountCPU::value_type src_);

};

template<>
struct Validatable<Domain::Xml::PCountCPU>: mpl::true_
{
	static bool validate(Domain::Xml::PCountCPU::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PVcpuid

namespace Domain
{
namespace Xml
{
struct PVcpuid
{
	typedef quint16 value_type;
};

} // namespace Xml
} // namespace Domain

template<>
struct Traits<Domain::Xml::PVcpuid>
{
	static bool parse(const QString& src_, Domain::Xml::PVcpuid::value_type& dst_);

	static QString generate(Domain::Xml::PVcpuid::value_type src_);

};


///////////////////////////////////////////////////////////////////////////////
// struct PCpushares

namespace Domain
{
namespace Xml
{
struct PCpushares
{
	typedef quint32 value_type;
};

} // namespace Xml
} // namespace Domain

template<>
struct Traits<Domain::Xml::PCpushares>
{
	static bool parse(const QString& src_, Domain::Xml::PCpushares::value_type& dst_);

	static QString generate(Domain::Xml::PCpushares::value_type src_);

};


///////////////////////////////////////////////////////////////////////////////
// struct PCpuperiod

namespace Domain
{
namespace Xml
{
struct PCpuperiod
{
	typedef ulong value_type;
};

} // namespace Xml
} // namespace Domain

template<>
struct Traits<Domain::Xml::PCpuperiod>
{
	static bool parse(const QString& src_, Domain::Xml::PCpuperiod::value_type& dst_);

	static QString generate(Domain::Xml::PCpuperiod::value_type src_);

};

template<>
struct Validatable<Domain::Xml::PCpuperiod>: mpl::true_
{
	static bool validate(Domain::Xml::PCpuperiod::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PCpuquota

namespace Domain
{
namespace Xml
{
struct PCpuquota
{
	typedef long value_type;
};

} // namespace Xml
} // namespace Domain

template<>
struct Traits<Domain::Xml::PCpuquota>
{
	static bool parse(const QString& src_, Domain::Xml::PCpuquota::value_type& dst_);

	static QString generate(Domain::Xml::PCpuquota::value_type src_);

};

template<>
struct Validatable<Domain::Xml::PCpuquota>: mpl::true_
{
	static bool validate(Domain::Xml::PCpuquota::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PRebootTimeoutDelay

namespace Domain
{
namespace Xml
{
struct PRebootTimeoutDelay
{
	typedef qint16 value_type;
};

} // namespace Xml
} // namespace Domain

template<>
struct Traits<Domain::Xml::PRebootTimeoutDelay>
{
	static bool parse(const QString& src_, Domain::Xml::PRebootTimeoutDelay::value_type& dst_);

	static QString generate(Domain::Xml::PRebootTimeoutDelay::value_type src_);

};

template<>
struct Validatable<Domain::Xml::PRebootTimeoutDelay>: mpl::true_
{
	static bool validate(Domain::Xml::PRebootTimeoutDelay::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PWeight

namespace Domain
{
namespace Xml
{
struct PWeight
{
	typedef quint32 value_type;
};

} // namespace Xml
} // namespace Domain

template<>
struct Traits<Domain::Xml::PWeight>
{
	static bool parse(const QString& src_, Domain::Xml::PWeight::value_type& dst_);

	static QString generate(Domain::Xml::PWeight::value_type src_);

};

template<>
struct Validatable<Domain::Xml::PWeight>: mpl::true_
{
	static bool validate(Domain::Xml::PWeight::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PMemoryKB

namespace Domain
{
namespace Xml
{
struct PMemoryKB
{
	typedef quint32 value_type;
};

} // namespace Xml
} // namespace Domain

template<>
struct Traits<Domain::Xml::PMemoryKB>
{
	static bool parse(const QString& src_, Domain::Xml::PMemoryKB::value_type& dst_);

	static QString generate(Domain::Xml::PMemoryKB::value_type src_);

};


///////////////////////////////////////////////////////////////////////////////
// struct PDomainName

namespace Domain
{
namespace Xml
{
struct PDomainName
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Domain

template<>
struct Validatable<Domain::Xml::PDomainName>: mpl::true_
{
	static bool validate(const Domain::Xml::PDomainName::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PDiskSerial

namespace Domain
{
namespace Xml
{
struct PDiskSerial
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Domain

template<>
struct Validatable<Domain::Xml::PDiskSerial>: mpl::true_
{
	static bool validate(const Domain::Xml::PDiskSerial::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PBridgeMode

namespace Domain
{
namespace Xml
{
struct PBridgeMode
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Domain

template<>
struct Validatable<Domain::Xml::PBridgeMode>: mpl::true_
{
	static bool validate(const Domain::Xml::PBridgeMode::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PAddrIPorName

namespace Domain
{
namespace Xml
{
struct PAddrIPorName
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Domain

template<>
struct Validatable<Domain::Xml::PAddrIPorName>: mpl::true_
{
	static bool validate(const Domain::Xml::PAddrIPorName::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PUsbIdDefault

namespace Domain
{
namespace Xml
{
struct PUsbIdDefault
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Domain

template<>
struct Validatable<Domain::Xml::PUsbIdDefault>: mpl::true_
{
	static bool validate(const Domain::Xml::PUsbIdDefault::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PUsbId

namespace Domain
{
namespace Xml
{
struct PUsbId
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Domain

template<>
struct Validatable<Domain::Xml::PUsbId>: mpl::true_
{
	static bool validate(const Domain::Xml::PUsbId::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PUsbVersion

namespace Domain
{
namespace Xml
{
struct PUsbVersion
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Domain

template<>
struct Validatable<Domain::Xml::PUsbVersion>: mpl::true_
{
	static bool validate(const Domain::Xml::PUsbVersion::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PUsbAddr

namespace Domain
{
namespace Xml
{
struct PUsbAddr
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Domain

template<>
struct Validatable<Domain::Xml::PUsbAddr>: mpl::true_
{
	static bool validate(const Domain::Xml::PUsbAddr::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PUsbClass

namespace Domain
{
namespace Xml
{
struct PUsbClass
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Domain

template<>
struct Validatable<Domain::Xml::PUsbClass>: mpl::true_
{
	static bool validate(const Domain::Xml::PUsbClass::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PUsbPort

namespace Domain
{
namespace Xml
{
struct PUsbPort
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Domain

template<>
struct Validatable<Domain::Xml::PUsbPort>: mpl::true_
{
	static bool validate(const Domain::Xml::PUsbPort::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PDriveController

namespace Domain
{
namespace Xml
{
struct PDriveController
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Domain

template<>
struct Validatable<Domain::Xml::PDriveController>: mpl::true_
{
	static bool validate(const Domain::Xml::PDriveController::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PDriveBus

namespace Domain
{
namespace Xml
{
struct PDriveBus
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Domain

template<>
struct Validatable<Domain::Xml::PDriveBus>: mpl::true_
{
	static bool validate(const Domain::Xml::PDriveBus::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PDriveTarget

namespace Domain
{
namespace Xml
{
struct PDriveTarget
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Domain

template<>
struct Validatable<Domain::Xml::PDriveTarget>: mpl::true_
{
	static bool validate(const Domain::Xml::PDriveTarget::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PDriveUnit

namespace Domain
{
namespace Xml
{
struct PDriveUnit
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Domain

template<>
struct Validatable<Domain::Xml::PDriveUnit>: mpl::true_
{
	static bool validate(const Domain::Xml::PDriveUnit::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PFeatureName

namespace Domain
{
namespace Xml
{
struct PFeatureName
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Domain

template<>
struct Validatable<Domain::Xml::PFeatureName>: mpl::true_
{
	static bool validate(const Domain::Xml::PFeatureName::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PTimeDelta

namespace Domain
{
namespace Xml
{
struct PTimeDelta
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Domain

template<>
struct Validatable<Domain::Xml::PTimeDelta>: mpl::true_
{
	static bool validate(const Domain::Xml::PTimeDelta::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PTimeZone

namespace Domain
{
namespace Xml
{
struct PTimeZone
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Domain

template<>
struct Validatable<Domain::Xml::PTimeZone>: mpl::true_
{
	static bool validate(const Domain::Xml::PTimeZone::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PFilterParamName

namespace Domain
{
namespace Xml
{
struct PFilterParamName
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Domain

template<>
struct Validatable<Domain::Xml::PFilterParamName>: mpl::true_
{
	static bool validate(const Domain::Xml::PFilterParamName::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PFilterParamValue

namespace Domain
{
namespace Xml
{
struct PFilterParamValue
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Domain

template<>
struct Validatable<Domain::Xml::PFilterParamValue>: mpl::true_
{
	static bool validate(const Domain::Xml::PFilterParamValue::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PSpaprvioReg

namespace Domain
{
namespace Xml
{
struct PSpaprvioReg
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Domain

template<>
struct Validatable<Domain::Xml::PSpaprvioReg>: mpl::true_
{
	static bool validate(const Domain::Xml::PSpaprvioReg::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PAliasName

namespace Domain
{
namespace Xml
{
struct PAliasName
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Domain

template<>
struct Validatable<Domain::Xml::PAliasName>: mpl::true_
{
	static bool validate(const Domain::Xml::PAliasName::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData1116

namespace Domain
{
namespace Xml
{
struct PData1116
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Domain

template<>
struct Validatable<Domain::Xml::PData1116>: mpl::true_
{
	static bool validate(const Domain::Xml::PData1116::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData1118

namespace Domain
{
namespace Xml
{
struct PData1118
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Domain

template<>
struct Validatable<Domain::Xml::PData1118>: mpl::true_
{
	static bool validate(const Domain::Xml::PData1118::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData1120

namespace Domain
{
namespace Xml
{
struct PData1120
{
	typedef qint32 value_type;
};

} // namespace Xml
} // namespace Domain

template<>
struct Traits<Domain::Xml::PData1120>
{
	static bool parse(const QString& src_, Domain::Xml::PData1120::value_type& dst_);

	static QString generate(Domain::Xml::PData1120::value_type src_);

};

template<>
struct Validatable<Domain::Xml::PData1120>: mpl::true_
{
	static bool validate(Domain::Xml::PData1120::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PCcwSsidRange

namespace Domain
{
namespace Xml
{
struct PCcwSsidRange
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Domain

template<>
struct Validatable<Domain::Xml::PCcwSsidRange>: mpl::true_
{
	static bool validate(const Domain::Xml::PCcwSsidRange::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData1122

namespace Domain
{
namespace Xml
{
struct PData1122
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Domain

template<>
struct Validatable<Domain::Xml::PData1122>: mpl::true_
{
	static bool validate(const Domain::Xml::PData1122::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData1124

namespace Domain
{
namespace Xml
{
struct PData1124
{
	typedef qint32 value_type;
};

} // namespace Xml
} // namespace Domain

template<>
struct Traits<Domain::Xml::PData1124>
{
	static bool parse(const QString& src_, Domain::Xml::PData1124::value_type& dst_);

	static QString generate(Domain::Xml::PData1124::value_type src_);

};

template<>
struct Validatable<Domain::Xml::PData1124>: mpl::true_
{
	static bool validate(Domain::Xml::PData1124::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct VUUID

namespace Domain
{
namespace Xml
{
typedef Choice<mpl::vector<PData16, PData18 > > VUUIDImpl;
typedef VUUIDImpl::value_type VUUID;

} // namespace Xml
} // namespace Domain

template<>
struct Traits<Domain::Xml::VUUID>
{
	static bool parse(const QString& src_, Domain::Xml::VUUID& dst_);

	static QString generate(const Domain::Xml::VUUID& src_);

};

///////////////////////////////////////////////////////////////////////////////
// struct VAdjustment

namespace Domain
{
namespace Xml
{
typedef Choice<mpl::vector<EAdjustment, PTimeDelta > > VAdjustmentImpl;
typedef VAdjustmentImpl::value_type VAdjustment;

} // namespace Xml
} // namespace Domain

template<>
struct Traits<Domain::Xml::VAdjustment>
{
	static bool parse(const QString& src_, Domain::Xml::VAdjustment& dst_);

	static QString generate(const Domain::Xml::VAdjustment& src_);

};

///////////////////////////////////////////////////////////////////////////////
// struct VIpAddr

namespace Domain
{
namespace Xml
{
typedef Choice<mpl::vector<PIpv4Addr, PIpv6Addr > > VIpAddrImpl;
typedef VIpAddrImpl::value_type VIpAddr;

} // namespace Xml
} // namespace Domain

template<>
struct Traits<Domain::Xml::VIpAddr>
{
	static bool parse(const QString& src_, Domain::Xml::VIpAddr& dst_);

	static QString generate(const Domain::Xml::VIpAddr& src_);

};

///////////////////////////////////////////////////////////////////////////////
// struct VName

namespace Domain
{
namespace Xml
{
typedef Choice<mpl::vector<PDnsName, VIpAddr > > VNameImpl;
typedef VNameImpl::value_type VName;

} // namespace Xml
} // namespace Domain

template<>
struct Traits<Domain::Xml::VName>
{
	static bool parse(const QString& src_, Domain::Xml::VName& dst_);

	static QString generate(const Domain::Xml::VName& src_);

};

///////////////////////////////////////////////////////////////////////////////
// struct VStorageFormat

namespace Domain
{
namespace Xml
{
typedef Choice<mpl::vector<EStorageFormat, EStorageFormatBacking > > VStorageFormatImpl;
typedef VStorageFormatImpl::value_type VStorageFormat;

} // namespace Xml
} // namespace Domain

template<>
struct Traits<Domain::Xml::VStorageFormat>
{
	static bool parse(const QString& src_, Domain::Xml::VStorageFormat& dst_);

	static QString generate(const Domain::Xml::VStorageFormat& src_);

};

///////////////////////////////////////////////////////////////////////////////
// struct VType

namespace Domain
{
namespace Xml
{
typedef Choice<mpl::vector<EType3, VStorageFormat > > VTypeImpl;
typedef VTypeImpl::value_type VType;

} // namespace Xml
} // namespace Domain

template<>
struct Traits<Domain::Xml::VType>
{
	static bool parse(const QString& src_, Domain::Xml::VType& dst_);

	static QString generate(const Domain::Xml::VType& src_);

};

///////////////////////////////////////////////////////////////////////////////
// struct VCcwCssidRange

namespace Domain
{
namespace Xml
{
typedef Choice<mpl::vector<PData1116, PData1118, PData1120 > > VCcwCssidRangeImpl;
typedef VCcwCssidRangeImpl::value_type VCcwCssidRange;

} // namespace Xml
} // namespace Domain

template<>
struct Traits<Domain::Xml::VCcwCssidRange>
{
	static bool parse(const QString& src_, Domain::Xml::VCcwCssidRange& dst_);

	static QString generate(const Domain::Xml::VCcwCssidRange& src_);

};

///////////////////////////////////////////////////////////////////////////////
// struct VCcwDevnoRange

namespace Domain
{
namespace Xml
{
typedef Choice<mpl::vector<PData1122, PData1124 > > VCcwDevnoRangeImpl;
typedef VCcwDevnoRangeImpl::value_type VCcwDevnoRange;

} // namespace Xml
} // namespace Domain

template<>
struct Traits<Domain::Xml::VCcwDevnoRange>
{
	static bool parse(const QString& src_, Domain::Xml::VCcwDevnoRange& dst_);

	static QString generate(const Domain::Xml::VCcwDevnoRange& src_);

};

///////////////////////////////////////////////////////////////////////////////
// struct VUint8range

namespace Domain
{
namespace Xml
{
typedef Choice<mpl::vector<PData6, PData8 > > VUint8rangeImpl;
typedef VUint8rangeImpl::value_type VUint8range;

} // namespace Xml
} // namespace Domain

template<>
struct Traits<Domain::Xml::VUint8range>
{
	static bool parse(const QString& src_, Domain::Xml::VUint8range& dst_);

	static QString generate(const Domain::Xml::VUint8range& src_);

};

///////////////////////////////////////////////////////////////////////////////
// struct VUint24range

namespace Domain
{
namespace Xml
{
typedef Choice<mpl::vector<PData11, PData13 > > VUint24rangeImpl;
typedef VUint24rangeImpl::value_type VUint24range;

} // namespace Xml
} // namespace Domain

template<>
struct Traits<Domain::Xml::VUint24range>
{
	static bool parse(const QString& src_, Domain::Xml::VUint24range& dst_);

	static QString generate(const Domain::Xml::VUint24range& src_);

};

///////////////////////////////////////////////////////////////////////////////
// struct VIpPrefix

namespace Domain
{
namespace Xml
{
typedef Choice<mpl::vector<PIpv4Prefix, PIpv6Prefix > > VIpPrefixImpl;
typedef VIpPrefixImpl::value_type VIpPrefix;

} // namespace Xml
} // namespace Domain

template<>
struct Traits<Domain::Xml::VIpPrefix>
{
	static bool parse(const QString& src_, Domain::Xml::VIpPrefix& dst_);

	static QString generate(const Domain::Xml::VIpPrefix& src_);

};

///////////////////////////////////////////////////////////////////////////////
// struct VClass

namespace Domain
{
namespace Xml
{
typedef Choice<mpl::vector<PUsbClass, PUsbIdDefault > > VClassImpl;
typedef VClassImpl::value_type VClass;

} // namespace Xml
} // namespace Domain

template<>
struct Traits<Domain::Xml::VClass>
{
	static bool parse(const QString& src_, Domain::Xml::VClass& dst_);

	static QString generate(const Domain::Xml::VClass& src_);

};

///////////////////////////////////////////////////////////////////////////////
// struct VVendor

namespace Domain
{
namespace Xml
{
typedef Choice<mpl::vector<PUsbId, PUsbIdDefault > > VVendorImpl;
typedef VVendorImpl::value_type VVendor;

} // namespace Xml
} // namespace Domain

template<>
struct Traits<Domain::Xml::VVendor>
{
	static bool parse(const QString& src_, Domain::Xml::VVendor& dst_);

	static QString generate(const Domain::Xml::VVendor& src_);

};

///////////////////////////////////////////////////////////////////////////////
// struct VProduct

namespace Domain
{
namespace Xml
{
typedef VVendorImpl VProductImpl;
typedef VVendor VProduct;

} // namespace Xml
} // namespace Domain

///////////////////////////////////////////////////////////////////////////////
// struct VVersion

namespace Domain
{
namespace Xml
{
typedef Choice<mpl::vector<PUsbVersion, PUsbIdDefault > > VVersionImpl;
typedef VVersionImpl::value_type VVersion;

} // namespace Xml
} // namespace Domain

template<>
struct Traits<Domain::Xml::VVersion>
{
	static bool parse(const QString& src_, Domain::Xml::VVersion& dst_);

	static QString generate(const Domain::Xml::VVersion& src_);

};

} // namespace Libvirt

#endif // __DOMAIN_DATA_H__
