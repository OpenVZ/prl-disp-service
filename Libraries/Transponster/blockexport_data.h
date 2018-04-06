/*
 * Copyright (c) 2015-2017, Parallels International GmbH
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
 * Our contact details: Parallels International GmbH, Vordergasse 59, 8200
 * Schaffhausen, Switzerland.
 */

#ifndef __BLOCKEXPORT_DATA_H__
#define __BLOCKEXPORT_DATA_H__
#include <QString>
#include <QDateTime>
#include "base.h"
#include "patterns.h"
#include "blockexport_enum.h"

namespace Libvirt
{
///////////////////////////////////////////////////////////////////////////////
// struct PUnsignedInt

namespace Blockexport
{
namespace Xml
{
struct PUnsignedInt
{
	typedef quint32 value_type;
};

} // namespace Xml
} // namespace Blockexport

template<>
struct Traits<Blockexport::Xml::PUnsignedInt>
{
	static bool parse(const QString& src_, Blockexport::Xml::PUnsignedInt::value_type& dst_);

	static QString generate(Blockexport::Xml::PUnsignedInt::value_type src_);

};


///////////////////////////////////////////////////////////////////////////////
// struct PUnsignedLong

namespace Blockexport
{
namespace Xml
{
struct PUnsignedLong
{
	typedef ulong value_type;
};

} // namespace Xml
} // namespace Blockexport

template<>
struct Traits<Blockexport::Xml::PUnsignedLong>
{
	static bool parse(const QString& src_, Blockexport::Xml::PUnsignedLong::value_type& dst_);

	static QString generate(Blockexport::Xml::PUnsignedLong::value_type src_);

};


///////////////////////////////////////////////////////////////////////////////
// struct PHexuint

namespace Blockexport
{
namespace Xml
{
struct PHexuint
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Blockexport

template<>
struct Validatable<Blockexport::Xml::PHexuint>: mpl::true_
{
	static bool validate(const Blockexport::Xml::PHexuint::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PPositiveInteger

namespace Blockexport
{
namespace Xml
{
struct PPositiveInteger
{
	typedef quint32 value_type;
};

} // namespace Xml
} // namespace Blockexport

template<>
struct Traits<Blockexport::Xml::PPositiveInteger>
{
	static bool parse(const QString& src_, Blockexport::Xml::PPositiveInteger::value_type& dst_);

	static QString generate(Blockexport::Xml::PPositiveInteger::value_type src_);

};


///////////////////////////////////////////////////////////////////////////////
// struct POctalMode

namespace Blockexport
{
namespace Xml
{
struct POctalMode
{
	typedef quint32 value_type;
};

} // namespace Xml
} // namespace Blockexport

template<>
struct Traits<Blockexport::Xml::POctalMode>
{
	static bool parse(const QString& src_, Blockexport::Xml::POctalMode::value_type& dst_);

	static QString generate(Blockexport::Xml::POctalMode::value_type src_);

};


///////////////////////////////////////////////////////////////////////////////
// struct PData1868

namespace Blockexport
{
namespace Xml
{
struct PData1868
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Blockexport

template<>
struct Validatable<Blockexport::Xml::PData1868>: mpl::true_
{
	static bool validate(const Blockexport::Xml::PData1868::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData1870

namespace Blockexport
{
namespace Xml
{
struct PData1870
{
	typedef qint32 value_type;
};

} // namespace Xml
} // namespace Blockexport

template<>
struct Traits<Blockexport::Xml::PData1870>
{
	static bool parse(const QString& src_, Blockexport::Xml::PData1870::value_type& dst_);

	static QString generate(Blockexport::Xml::PData1870::value_type src_);

};

template<>
struct Validatable<Blockexport::Xml::PData1870>: mpl::true_
{
	static bool validate(Blockexport::Xml::PData1870::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData1872

namespace Blockexport
{
namespace Xml
{
struct PData1872
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Blockexport

template<>
struct Validatable<Blockexport::Xml::PData1872>: mpl::true_
{
	static bool validate(const Blockexport::Xml::PData1872::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData1874

namespace Blockexport
{
namespace Xml
{
struct PData1874
{
	typedef qint32 value_type;
};

} // namespace Xml
} // namespace Blockexport

template<>
struct Traits<Blockexport::Xml::PData1874>
{
	static bool parse(const QString& src_, Blockexport::Xml::PData1874::value_type& dst_);

	static QString generate(Blockexport::Xml::PData1874::value_type src_);

};

template<>
struct Validatable<Blockexport::Xml::PData1874>: mpl::true_
{
	static bool validate(Blockexport::Xml::PData1874::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData1876

namespace Blockexport
{
namespace Xml
{
struct PData1876
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Blockexport

template<>
struct Validatable<Blockexport::Xml::PData1876>: mpl::true_
{
	static bool validate(const Blockexport::Xml::PData1876::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData1878

namespace Blockexport
{
namespace Xml
{
struct PData1878
{
	typedef qint32 value_type;
};

} // namespace Xml
} // namespace Blockexport

template<>
struct Traits<Blockexport::Xml::PData1878>
{
	static bool parse(const QString& src_, Blockexport::Xml::PData1878::value_type& dst_);

	static QString generate(Blockexport::Xml::PData1878::value_type src_);

};

template<>
struct Validatable<Blockexport::Xml::PData1878>: mpl::true_
{
	static bool validate(Blockexport::Xml::PData1878::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData1880

namespace Blockexport
{
namespace Xml
{
struct PData1880
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Blockexport

template<>
struct Validatable<Blockexport::Xml::PData1880>: mpl::true_
{
	static bool validate(const Blockexport::Xml::PData1880::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData1882

namespace Blockexport
{
namespace Xml
{
struct PData1882
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Blockexport

template<>
struct Validatable<Blockexport::Xml::PData1882>: mpl::true_
{
	static bool validate(const Blockexport::Xml::PData1882::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PUniMacAddr

namespace Blockexport
{
namespace Xml
{
struct PUniMacAddr
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Blockexport

template<>
struct Validatable<Blockexport::Xml::PUniMacAddr>: mpl::true_
{
	static bool validate(const Blockexport::Xml::PUniMacAddr::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PMultiMacAddr

namespace Blockexport
{
namespace Xml
{
struct PMultiMacAddr
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Blockexport

template<>
struct Validatable<Blockexport::Xml::PMultiMacAddr>: mpl::true_
{
	static bool validate(const Blockexport::Xml::PMultiMacAddr::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PMacAddr

namespace Blockexport
{
namespace Xml
{
struct PMacAddr
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Blockexport

template<>
struct Validatable<Blockexport::Xml::PMacAddr>: mpl::true_
{
	static bool validate(const Blockexport::Xml::PMacAddr::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PDuidLLT

namespace Blockexport
{
namespace Xml
{
struct PDuidLLT
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Blockexport

template<>
struct Validatable<Blockexport::Xml::PDuidLLT>: mpl::true_
{
	static bool validate(const Blockexport::Xml::PDuidLLT::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PDuidEN

namespace Blockexport
{
namespace Xml
{
struct PDuidEN
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Blockexport

template<>
struct Validatable<Blockexport::Xml::PDuidEN>: mpl::true_
{
	static bool validate(const Blockexport::Xml::PDuidEN::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PDuidLL

namespace Blockexport
{
namespace Xml
{
struct PDuidLL
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Blockexport

template<>
struct Validatable<Blockexport::Xml::PDuidLL>: mpl::true_
{
	static bool validate(const Blockexport::Xml::PDuidLL::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PDuidUUID

namespace Blockexport
{
namespace Xml
{
struct PDuidUUID
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Blockexport

template<>
struct Validatable<Blockexport::Xml::PDuidUUID>: mpl::true_
{
	static bool validate(const Blockexport::Xml::PDuidUUID::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PIpv4Addr

namespace Blockexport
{
namespace Xml
{
struct PIpv4Addr
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Blockexport

template<>
struct Validatable<Blockexport::Xml::PIpv4Addr>: mpl::true_
{
	static bool validate(const Blockexport::Xml::PIpv4Addr::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PIpv6Addr

namespace Blockexport
{
namespace Xml
{
struct PIpv6Addr
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Blockexport

template<>
struct Validatable<Blockexport::Xml::PIpv6Addr>: mpl::true_
{
	static bool validate(const Blockexport::Xml::PIpv6Addr::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PIpv4Prefix

namespace Blockexport
{
namespace Xml
{
struct PIpv4Prefix
{
	typedef quint32 value_type;
};

} // namespace Xml
} // namespace Blockexport

template<>
struct Traits<Blockexport::Xml::PIpv4Prefix>
{
	static bool parse(const QString& src_, Blockexport::Xml::PIpv4Prefix::value_type& dst_);

	static QString generate(Blockexport::Xml::PIpv4Prefix::value_type src_);

};

template<>
struct Validatable<Blockexport::Xml::PIpv4Prefix>: mpl::true_
{
	static bool validate(Blockexport::Xml::PIpv4Prefix::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PIpv6Prefix

namespace Blockexport
{
namespace Xml
{
struct PIpv6Prefix
{
	typedef quint32 value_type;
};

} // namespace Xml
} // namespace Blockexport

template<>
struct Traits<Blockexport::Xml::PIpv6Prefix>
{
	static bool parse(const QString& src_, Blockexport::Xml::PIpv6Prefix::value_type& dst_);

	static QString generate(Blockexport::Xml::PIpv6Prefix::value_type src_);

};

template<>
struct Validatable<Blockexport::Xml::PIpv6Prefix>: mpl::true_
{
	static bool validate(Blockexport::Xml::PIpv6Prefix::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PGenericName

namespace Blockexport
{
namespace Xml
{
struct PGenericName
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Blockexport

template<>
struct Validatable<Blockexport::Xml::PGenericName>: mpl::true_
{
	static bool validate(const Blockexport::Xml::PGenericName::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PDnsName

namespace Blockexport
{
namespace Xml
{
struct PDnsName
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Blockexport

template<>
struct Validatable<Blockexport::Xml::PDnsName>: mpl::true_
{
	static bool validate(const Blockexport::Xml::PDnsName::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PDeviceName

namespace Blockexport
{
namespace Xml
{
struct PDeviceName
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Blockexport

template<>
struct Validatable<Blockexport::Xml::PDeviceName>: mpl::true_
{
	static bool validate(const Blockexport::Xml::PDeviceName::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PFilePath

namespace Blockexport
{
namespace Xml
{
struct PFilePath
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Blockexport

template<>
struct Validatable<Blockexport::Xml::PFilePath>: mpl::true_
{
	static bool validate(const Blockexport::Xml::PFilePath::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PDirPath

namespace Blockexport
{
namespace Xml
{
struct PDirPath
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Blockexport

template<>
struct Validatable<Blockexport::Xml::PDirPath>: mpl::true_
{
	static bool validate(const Blockexport::Xml::PDirPath::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PAbsFilePath

namespace Blockexport
{
namespace Xml
{
struct PAbsFilePath
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Blockexport

template<>
struct Validatable<Blockexport::Xml::PAbsFilePath>: mpl::true_
{
	static bool validate(const Blockexport::Xml::PAbsFilePath::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PAbsDirPath

namespace Blockexport
{
namespace Xml
{
struct PAbsDirPath
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Blockexport

template<>
struct Validatable<Blockexport::Xml::PAbsDirPath>: mpl::true_
{
	static bool validate(const Blockexport::Xml::PAbsDirPath::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PUnit

namespace Blockexport
{
namespace Xml
{
struct PUnit
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Blockexport

template<>
struct Validatable<Blockexport::Xml::PUnit>: mpl::true_
{
	static bool validate(const Blockexport::Xml::PUnit::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PPciDomain

namespace Blockexport
{
namespace Xml
{
struct PPciDomain
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Blockexport

template<>
struct Validatable<Blockexport::Xml::PPciDomain>: mpl::true_
{
	static bool validate(const Blockexport::Xml::PPciDomain::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PPciBus

namespace Blockexport
{
namespace Xml
{
struct PPciBus
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Blockexport

template<>
struct Validatable<Blockexport::Xml::PPciBus>: mpl::true_
{
	static bool validate(const Blockexport::Xml::PPciBus::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PPciSlot

namespace Blockexport
{
namespace Xml
{
struct PPciSlot
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Blockexport

template<>
struct Validatable<Blockexport::Xml::PPciSlot>: mpl::true_
{
	static bool validate(const Blockexport::Xml::PPciSlot::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PPciFunc

namespace Blockexport
{
namespace Xml
{
struct PPciFunc
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Blockexport

template<>
struct Validatable<Blockexport::Xml::PPciFunc>: mpl::true_
{
	static bool validate(const Blockexport::Xml::PPciFunc::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PWwn

namespace Blockexport
{
namespace Xml
{
struct PWwn
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Blockexport

template<>
struct Validatable<Blockexport::Xml::PWwn>: mpl::true_
{
	static bool validate(const Blockexport::Xml::PWwn::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PCpuset

namespace Blockexport
{
namespace Xml
{
struct PCpuset
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Blockexport

template<>
struct Validatable<Blockexport::Xml::PCpuset>: mpl::true_
{
	static bool validate(const Blockexport::Xml::PCpuset::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PVolName

namespace Blockexport
{
namespace Xml
{
struct PVolName
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Blockexport

template<>
struct Validatable<Blockexport::Xml::PVolName>: mpl::true_
{
	static bool validate(const Blockexport::Xml::PVolName::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PPortNumber

namespace Blockexport
{
namespace Xml
{
struct PPortNumber
{
	typedef qint32 value_type;
};

} // namespace Xml
} // namespace Blockexport

template<>
struct Traits<Blockexport::Xml::PPortNumber>
{
	static bool parse(const QString& src_, Blockexport::Xml::PPortNumber::value_type& dst_);

	static QString generate(Blockexport::Xml::PPortNumber::value_type src_);

};

template<>
struct Validatable<Blockexport::Xml::PPortNumber>: mpl::true_
{
	static bool validate(Blockexport::Xml::PPortNumber::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PIobase

namespace Blockexport
{
namespace Xml
{
struct PIobase
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Blockexport

template<>
struct Validatable<Blockexport::Xml::PIobase>: mpl::true_
{
	static bool validate(const Blockexport::Xml::PIobase::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PIrq

namespace Blockexport
{
namespace Xml
{
struct PIrq
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Blockexport

template<>
struct Validatable<Blockexport::Xml::PIrq>: mpl::true_
{
	static bool validate(const Blockexport::Xml::PIrq::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PCompat

namespace Blockexport
{
namespace Xml
{
struct PCompat
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Blockexport

template<>
struct Validatable<Blockexport::Xml::PCompat>: mpl::true_
{
	static bool validate(const Blockexport::Xml::PCompat::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PVirtualPortProfileID

namespace Blockexport
{
namespace Xml
{
struct PVirtualPortProfileID
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Blockexport

template<>
struct Validatable<Blockexport::Xml::PVirtualPortProfileID>: mpl::true_
{
	static bool validate(const Blockexport::Xml::PVirtualPortProfileID::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PSpeed

namespace Blockexport
{
namespace Xml
{
struct PSpeed
{
	typedef quint32 value_type;
};

} // namespace Xml
} // namespace Blockexport

template<>
struct Traits<Blockexport::Xml::PSpeed>
{
	static bool parse(const QString& src_, Blockexport::Xml::PSpeed::value_type& dst_);

	static QString generate(Blockexport::Xml::PSpeed::value_type src_);

};

template<>
struct Validatable<Blockexport::Xml::PSpeed>: mpl::true_
{
	static bool validate(Blockexport::Xml::PSpeed::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PBurstSize

namespace Blockexport
{
namespace Xml
{
struct PBurstSize
{
	typedef quint32 value_type;
};

} // namespace Xml
} // namespace Blockexport

template<>
struct Traits<Blockexport::Xml::PBurstSize>
{
	static bool parse(const QString& src_, Blockexport::Xml::PBurstSize::value_type& dst_);

	static QString generate(Blockexport::Xml::PBurstSize::value_type src_);

};

template<>
struct Validatable<Blockexport::Xml::PBurstSize>: mpl::true_
{
	static bool validate(Blockexport::Xml::PBurstSize::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PUnsignedShort

namespace Blockexport
{
namespace Xml
{
struct PUnsignedShort
{
	typedef qint32 value_type;
};

} // namespace Xml
} // namespace Blockexport

template<>
struct Traits<Blockexport::Xml::PUnsignedShort>
{
	static bool parse(const QString& src_, Blockexport::Xml::PUnsignedShort::value_type& dst_);

	static QString generate(Blockexport::Xml::PUnsignedShort::value_type src_);

};

template<>
struct Validatable<Blockexport::Xml::PUnsignedShort>: mpl::true_
{
	static bool validate(Blockexport::Xml::PUnsignedShort::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PProtocol

namespace Blockexport
{
namespace Xml
{
struct PProtocol
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Blockexport

template<>
struct Validatable<Blockexport::Xml::PProtocol>: mpl::true_
{
	static bool validate(const Blockexport::Xml::PProtocol::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PAddrFamily

namespace Blockexport
{
namespace Xml
{
struct PAddrFamily
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Blockexport

template<>
struct Validatable<Blockexport::Xml::PAddrFamily>: mpl::true_
{
	static bool validate(const Blockexport::Xml::PAddrFamily::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PId

namespace Blockexport
{
namespace Xml
{
struct PId
{
	typedef quint32 value_type;
};

} // namespace Xml
} // namespace Blockexport

template<>
struct Traits<Blockexport::Xml::PId>
{
	static bool parse(const QString& src_, Blockexport::Xml::PId::value_type& dst_);

	static QString generate(Blockexport::Xml::PId::value_type src_);

};

template<>
struct Validatable<Blockexport::Xml::PId>: mpl::true_
{
	static bool validate(Blockexport::Xml::PId::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PPort

namespace Blockexport
{
namespace Xml
{
struct PPort
{
	typedef qint32 value_type;
};

} // namespace Xml
} // namespace Blockexport

template<>
struct Traits<Blockexport::Xml::PPort>
{
	static bool parse(const QString& src_, Blockexport::Xml::PPort::value_type& dst_);

	static QString generate(Blockexport::Xml::PPort::value_type src_);

};

template<>
struct Validatable<Blockexport::Xml::PPort>: mpl::true_
{
	static bool validate(Blockexport::Xml::PPort::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PTitle

namespace Blockexport
{
namespace Xml
{
struct PTitle
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Blockexport

template<>
struct Validatable<Blockexport::Xml::PTitle>: mpl::true_
{
	static bool validate(const Blockexport::Xml::PTitle::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PMachine

namespace Blockexport
{
namespace Xml
{
struct PMachine
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Blockexport

template<>
struct Validatable<Blockexport::Xml::PMachine>: mpl::true_
{
	static bool validate(const Blockexport::Xml::PMachine::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PReadIopsSec

namespace Blockexport
{
namespace Xml
{
struct PReadIopsSec
{
	typedef quint32 value_type;
};

} // namespace Xml
} // namespace Blockexport

template<>
struct Traits<Blockexport::Xml::PReadIopsSec>
{
	static bool parse(const QString& src_, Blockexport::Xml::PReadIopsSec::value_type& dst_);

	static QString generate(Blockexport::Xml::PReadIopsSec::value_type src_);

};


///////////////////////////////////////////////////////////////////////////////
// struct PWriteIopsSec

namespace Blockexport
{
namespace Xml
{
struct PWriteIopsSec
{
	typedef quint32 value_type;
};

} // namespace Xml
} // namespace Blockexport

template<>
struct Traits<Blockexport::Xml::PWriteIopsSec>
{
	static bool parse(const QString& src_, Blockexport::Xml::PWriteIopsSec::value_type& dst_);

	static QString generate(Blockexport::Xml::PWriteIopsSec::value_type src_);

};


///////////////////////////////////////////////////////////////////////////////
// struct PReadBytesSec

namespace Blockexport
{
namespace Xml
{
struct PReadBytesSec
{
	typedef ulong value_type;
};

} // namespace Xml
} // namespace Blockexport

template<>
struct Traits<Blockexport::Xml::PReadBytesSec>
{
	static bool parse(const QString& src_, Blockexport::Xml::PReadBytesSec::value_type& dst_);

	static QString generate(Blockexport::Xml::PReadBytesSec::value_type src_);

};


///////////////////////////////////////////////////////////////////////////////
// struct PWriteBytesSec

namespace Blockexport
{
namespace Xml
{
struct PWriteBytesSec
{
	typedef ulong value_type;
};

} // namespace Xml
} // namespace Blockexport

template<>
struct Traits<Blockexport::Xml::PWriteBytesSec>
{
	static bool parse(const QString& src_, Blockexport::Xml::PWriteBytesSec::value_type& dst_);

	static QString generate(Blockexport::Xml::PWriteBytesSec::value_type src_);

};


///////////////////////////////////////////////////////////////////////////////
// struct PVendor

namespace Blockexport
{
namespace Xml
{
struct PVendor
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Blockexport

template<>
struct Validatable<Blockexport::Xml::PVendor>: mpl::true_
{
	static bool validate(const Blockexport::Xml::PVendor::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PProduct

namespace Blockexport
{
namespace Xml
{
struct PProduct
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Blockexport

template<>
struct Validatable<Blockexport::Xml::PProduct>: mpl::true_
{
	static bool validate(const Blockexport::Xml::PProduct::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PDiskTarget

namespace Blockexport
{
namespace Xml
{
struct PDiskTarget
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Blockexport

template<>
struct Validatable<Blockexport::Xml::PDiskTarget>: mpl::true_
{
	static bool validate(const Blockexport::Xml::PDiskTarget::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PCyls

namespace Blockexport
{
namespace Xml
{
struct PCyls
{
	typedef qint32 value_type;
};

} // namespace Xml
} // namespace Blockexport

template<>
struct Traits<Blockexport::Xml::PCyls>
{
	static bool parse(const QString& src_, Blockexport::Xml::PCyls::value_type& dst_);

	static QString generate(Blockexport::Xml::PCyls::value_type src_);

};


///////////////////////////////////////////////////////////////////////////////
// struct PHeads

namespace Blockexport
{
namespace Xml
{
struct PHeads
{
	typedef qint32 value_type;
};

} // namespace Xml
} // namespace Blockexport

template<>
struct Traits<Blockexport::Xml::PHeads>
{
	static bool parse(const QString& src_, Blockexport::Xml::PHeads::value_type& dst_);

	static QString generate(Blockexport::Xml::PHeads::value_type src_);

};


///////////////////////////////////////////////////////////////////////////////
// struct PSecs

namespace Blockexport
{
namespace Xml
{
struct PSecs
{
	typedef qint32 value_type;
};

} // namespace Xml
} // namespace Blockexport

template<>
struct Traits<Blockexport::Xml::PSecs>
{
	static bool parse(const QString& src_, Blockexport::Xml::PSecs::value_type& dst_);

	static QString generate(Blockexport::Xml::PSecs::value_type src_);

};


///////////////////////////////////////////////////////////////////////////////
// struct PLogicalBlockSize

namespace Blockexport
{
namespace Xml
{
struct PLogicalBlockSize
{
	typedef qint32 value_type;
};

} // namespace Xml
} // namespace Blockexport

template<>
struct Traits<Blockexport::Xml::PLogicalBlockSize>
{
	static bool parse(const QString& src_, Blockexport::Xml::PLogicalBlockSize::value_type& dst_);

	static QString generate(Blockexport::Xml::PLogicalBlockSize::value_type src_);

};


///////////////////////////////////////////////////////////////////////////////
// struct PPhysicalBlockSize

namespace Blockexport
{
namespace Xml
{
struct PPhysicalBlockSize
{
	typedef qint32 value_type;
};

} // namespace Xml
} // namespace Blockexport

template<>
struct Traits<Blockexport::Xml::PPhysicalBlockSize>
{
	static bool parse(const QString& src_, Blockexport::Xml::PPhysicalBlockSize::value_type& dst_);

	static QString generate(Blockexport::Xml::PPhysicalBlockSize::value_type src_);

};


///////////////////////////////////////////////////////////////////////////////
// struct PType

namespace Blockexport
{
namespace Xml
{
struct PType
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Blockexport

template<>
struct Validatable<Blockexport::Xml::PType>: mpl::true_
{
	static bool validate(const Blockexport::Xml::PType::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PPasswdValidTo

namespace Blockexport
{
namespace Xml
{
struct PPasswdValidTo
{
	typedef QDateTime value_type;
};

} // namespace Xml
} // namespace Blockexport

template<>
struct Traits<Blockexport::Xml::PPasswdValidTo>
{
	static bool parse(const QString& src_, Blockexport::Xml::PPasswdValidTo::value_type& dst_);

	static QString generate(const Blockexport::Xml::PPasswdValidTo::value_type& src_);

};


///////////////////////////////////////////////////////////////////////////////
// struct PVendorId

namespace Blockexport
{
namespace Xml
{
struct PVendorId
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Blockexport

template<>
struct Validatable<Blockexport::Xml::PVendorId>: mpl::true_
{
	static bool validate(const Blockexport::Xml::PVendorId::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PSysinfoValue

namespace Blockexport
{
namespace Xml
{
struct PSysinfoValue
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Blockexport

template<>
struct Validatable<Blockexport::Xml::PSysinfoValue>: mpl::true_
{
	static bool validate(const Blockexport::Xml::PSysinfoValue::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PFilter

namespace Blockexport
{
namespace Xml
{
struct PFilter
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Blockexport


///////////////////////////////////////////////////////////////////////////////
// struct PTotalBytesSec

namespace Blockexport
{
namespace Xml
{
struct PTotalBytesSec
{
	typedef ulong value_type;
};

} // namespace Xml
} // namespace Blockexport

template<>
struct Traits<Blockexport::Xml::PTotalBytesSec>
{
	static bool parse(const QString& src_, Blockexport::Xml::PTotalBytesSec::value_type& dst_);

	static QString generate(Blockexport::Xml::PTotalBytesSec::value_type src_);

};


///////////////////////////////////////////////////////////////////////////////
// struct PTotalIopsSec

namespace Blockexport
{
namespace Xml
{
struct PTotalIopsSec
{
	typedef ulong value_type;
};

} // namespace Xml
} // namespace Blockexport

template<>
struct Traits<Blockexport::Xml::PTotalIopsSec>
{
	static bool parse(const QString& src_, Blockexport::Xml::PTotalIopsSec::value_type& dst_);

	static QString generate(Blockexport::Xml::PTotalIopsSec::value_type src_);

};


///////////////////////////////////////////////////////////////////////////////
// struct PReadIopsSec1

namespace Blockexport
{
namespace Xml
{
struct PReadIopsSec1
{
	typedef ulong value_type;
};

} // namespace Xml
} // namespace Blockexport

template<>
struct Traits<Blockexport::Xml::PReadIopsSec1>
{
	static bool parse(const QString& src_, Blockexport::Xml::PReadIopsSec1::value_type& dst_);

	static QString generate(Blockexport::Xml::PReadIopsSec1::value_type src_);

};


///////////////////////////////////////////////////////////////////////////////
// struct PWriteIopsSec1

namespace Blockexport
{
namespace Xml
{
struct PWriteIopsSec1
{
	typedef ulong value_type;
};

} // namespace Xml
} // namespace Blockexport

template<>
struct Traits<Blockexport::Xml::PWriteIopsSec1>
{
	static bool parse(const QString& src_, Blockexport::Xml::PWriteIopsSec1::value_type& dst_);

	static QString generate(Blockexport::Xml::PWriteIopsSec1::value_type src_);

};


///////////////////////////////////////////////////////////////////////////////
// struct PRetries

namespace Blockexport
{
namespace Xml
{
struct PRetries
{
	typedef quint32 value_type;
};

} // namespace Xml
} // namespace Blockexport

template<>
struct Traits<Blockexport::Xml::PRetries>
{
	static bool parse(const QString& src_, Blockexport::Xml::PRetries::value_type& dst_);

	static QString generate(Blockexport::Xml::PRetries::value_type src_);

};


///////////////////////////////////////////////////////////////////////////////
// struct PValue

namespace Blockexport
{
namespace Xml
{
struct PValue
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Blockexport

template<>
struct Validatable<Blockexport::Xml::PValue>: mpl::true_
{
	static bool validate(const Blockexport::Xml::PValue::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PCountCPU

namespace Blockexport
{
namespace Xml
{
struct PCountCPU
{
	typedef quint16 value_type;
};

} // namespace Xml
} // namespace Blockexport

template<>
struct Traits<Blockexport::Xml::PCountCPU>
{
	static bool parse(const QString& src_, Blockexport::Xml::PCountCPU::value_type& dst_);

	static QString generate(Blockexport::Xml::PCountCPU::value_type src_);

};

template<>
struct Validatable<Blockexport::Xml::PCountCPU>: mpl::true_
{
	static bool validate(Blockexport::Xml::PCountCPU::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PVcpuid

namespace Blockexport
{
namespace Xml
{
struct PVcpuid
{
	typedef quint16 value_type;
};

} // namespace Xml
} // namespace Blockexport

template<>
struct Traits<Blockexport::Xml::PVcpuid>
{
	static bool parse(const QString& src_, Blockexport::Xml::PVcpuid::value_type& dst_);

	static QString generate(Blockexport::Xml::PVcpuid::value_type src_);

};


///////////////////////////////////////////////////////////////////////////////
// struct PCpushares

namespace Blockexport
{
namespace Xml
{
struct PCpushares
{
	typedef quint32 value_type;
};

} // namespace Xml
} // namespace Blockexport

template<>
struct Traits<Blockexport::Xml::PCpushares>
{
	static bool parse(const QString& src_, Blockexport::Xml::PCpushares::value_type& dst_);

	static QString generate(Blockexport::Xml::PCpushares::value_type src_);

};


///////////////////////////////////////////////////////////////////////////////
// struct PCpuperiod

namespace Blockexport
{
namespace Xml
{
struct PCpuperiod
{
	typedef ulong value_type;
};

} // namespace Xml
} // namespace Blockexport

template<>
struct Traits<Blockexport::Xml::PCpuperiod>
{
	static bool parse(const QString& src_, Blockexport::Xml::PCpuperiod::value_type& dst_);

	static QString generate(Blockexport::Xml::PCpuperiod::value_type src_);

};

template<>
struct Validatable<Blockexport::Xml::PCpuperiod>: mpl::true_
{
	static bool validate(Blockexport::Xml::PCpuperiod::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PCpuquota

namespace Blockexport
{
namespace Xml
{
struct PCpuquota
{
	typedef long value_type;
};

} // namespace Xml
} // namespace Blockexport

template<>
struct Traits<Blockexport::Xml::PCpuquota>
{
	static bool parse(const QString& src_, Blockexport::Xml::PCpuquota::value_type& dst_);

	static QString generate(Blockexport::Xml::PCpuquota::value_type src_);

};

template<>
struct Validatable<Blockexport::Xml::PCpuquota>: mpl::true_
{
	static bool validate(Blockexport::Xml::PCpuquota::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PRebootTimeoutDelay

namespace Blockexport
{
namespace Xml
{
struct PRebootTimeoutDelay
{
	typedef qint16 value_type;
};

} // namespace Xml
} // namespace Blockexport

template<>
struct Traits<Blockexport::Xml::PRebootTimeoutDelay>
{
	static bool parse(const QString& src_, Blockexport::Xml::PRebootTimeoutDelay::value_type& dst_);

	static QString generate(Blockexport::Xml::PRebootTimeoutDelay::value_type src_);

};

template<>
struct Validatable<Blockexport::Xml::PRebootTimeoutDelay>: mpl::true_
{
	static bool validate(Blockexport::Xml::PRebootTimeoutDelay::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PWeight

namespace Blockexport
{
namespace Xml
{
struct PWeight
{
	typedef quint32 value_type;
};

} // namespace Xml
} // namespace Blockexport

template<>
struct Traits<Blockexport::Xml::PWeight>
{
	static bool parse(const QString& src_, Blockexport::Xml::PWeight::value_type& dst_);

	static QString generate(Blockexport::Xml::PWeight::value_type src_);

};

template<>
struct Validatable<Blockexport::Xml::PWeight>: mpl::true_
{
	static bool validate(Blockexport::Xml::PWeight::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PMemoryKB

namespace Blockexport
{
namespace Xml
{
struct PMemoryKB
{
	typedef quint32 value_type;
};

} // namespace Xml
} // namespace Blockexport

template<>
struct Traits<Blockexport::Xml::PMemoryKB>
{
	static bool parse(const QString& src_, Blockexport::Xml::PMemoryKB::value_type& dst_);

	static QString generate(Blockexport::Xml::PMemoryKB::value_type src_);

};


///////////////////////////////////////////////////////////////////////////////
// struct PDomainName

namespace Blockexport
{
namespace Xml
{
struct PDomainName
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Blockexport

template<>
struct Validatable<Blockexport::Xml::PDomainName>: mpl::true_
{
	static bool validate(const Blockexport::Xml::PDomainName::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PDiskSerial

namespace Blockexport
{
namespace Xml
{
struct PDiskSerial
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Blockexport

template<>
struct Validatable<Blockexport::Xml::PDiskSerial>: mpl::true_
{
	static bool validate(const Blockexport::Xml::PDiskSerial::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PBridgeMode

namespace Blockexport
{
namespace Xml
{
struct PBridgeMode
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Blockexport

template<>
struct Validatable<Blockexport::Xml::PBridgeMode>: mpl::true_
{
	static bool validate(const Blockexport::Xml::PBridgeMode::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PAddrIPorName

namespace Blockexport
{
namespace Xml
{
struct PAddrIPorName
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Blockexport

template<>
struct Validatable<Blockexport::Xml::PAddrIPorName>: mpl::true_
{
	static bool validate(const Blockexport::Xml::PAddrIPorName::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PUsbIdDefault

namespace Blockexport
{
namespace Xml
{
struct PUsbIdDefault
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Blockexport

template<>
struct Validatable<Blockexport::Xml::PUsbIdDefault>: mpl::true_
{
	static bool validate(const Blockexport::Xml::PUsbIdDefault::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PUsbId

namespace Blockexport
{
namespace Xml
{
struct PUsbId
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Blockexport

template<>
struct Validatable<Blockexport::Xml::PUsbId>: mpl::true_
{
	static bool validate(const Blockexport::Xml::PUsbId::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PUsbVersion

namespace Blockexport
{
namespace Xml
{
struct PUsbVersion
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Blockexport

template<>
struct Validatable<Blockexport::Xml::PUsbVersion>: mpl::true_
{
	static bool validate(const Blockexport::Xml::PUsbVersion::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PUsbAddr

namespace Blockexport
{
namespace Xml
{
struct PUsbAddr
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Blockexport

template<>
struct Validatable<Blockexport::Xml::PUsbAddr>: mpl::true_
{
	static bool validate(const Blockexport::Xml::PUsbAddr::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PUsbClass

namespace Blockexport
{
namespace Xml
{
struct PUsbClass
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Blockexport

template<>
struct Validatable<Blockexport::Xml::PUsbClass>: mpl::true_
{
	static bool validate(const Blockexport::Xml::PUsbClass::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PUsbPort

namespace Blockexport
{
namespace Xml
{
struct PUsbPort
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Blockexport

template<>
struct Validatable<Blockexport::Xml::PUsbPort>: mpl::true_
{
	static bool validate(const Blockexport::Xml::PUsbPort::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PDriveController

namespace Blockexport
{
namespace Xml
{
struct PDriveController
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Blockexport

template<>
struct Validatable<Blockexport::Xml::PDriveController>: mpl::true_
{
	static bool validate(const Blockexport::Xml::PDriveController::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PDriveBus

namespace Blockexport
{
namespace Xml
{
struct PDriveBus
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Blockexport

template<>
struct Validatable<Blockexport::Xml::PDriveBus>: mpl::true_
{
	static bool validate(const Blockexport::Xml::PDriveBus::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PDriveTarget

namespace Blockexport
{
namespace Xml
{
struct PDriveTarget
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Blockexport

template<>
struct Validatable<Blockexport::Xml::PDriveTarget>: mpl::true_
{
	static bool validate(const Blockexport::Xml::PDriveTarget::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PDriveUnit

namespace Blockexport
{
namespace Xml
{
struct PDriveUnit
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Blockexport

template<>
struct Validatable<Blockexport::Xml::PDriveUnit>: mpl::true_
{
	static bool validate(const Blockexport::Xml::PDriveUnit::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PFeatureName

namespace Blockexport
{
namespace Xml
{
struct PFeatureName
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Blockexport

template<>
struct Validatable<Blockexport::Xml::PFeatureName>: mpl::true_
{
	static bool validate(const Blockexport::Xml::PFeatureName::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PTimeDelta

namespace Blockexport
{
namespace Xml
{
struct PTimeDelta
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Blockexport

template<>
struct Validatable<Blockexport::Xml::PTimeDelta>: mpl::true_
{
	static bool validate(const Blockexport::Xml::PTimeDelta::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PTimeZone

namespace Blockexport
{
namespace Xml
{
struct PTimeZone
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Blockexport

template<>
struct Validatable<Blockexport::Xml::PTimeZone>: mpl::true_
{
	static bool validate(const Blockexport::Xml::PTimeZone::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PFilterParamName

namespace Blockexport
{
namespace Xml
{
struct PFilterParamName
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Blockexport

template<>
struct Validatable<Blockexport::Xml::PFilterParamName>: mpl::true_
{
	static bool validate(const Blockexport::Xml::PFilterParamName::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PFilterParamValue

namespace Blockexport
{
namespace Xml
{
struct PFilterParamValue
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Blockexport

template<>
struct Validatable<Blockexport::Xml::PFilterParamValue>: mpl::true_
{
	static bool validate(const Blockexport::Xml::PFilterParamValue::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PSpaprvioReg

namespace Blockexport
{
namespace Xml
{
struct PSpaprvioReg
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Blockexport

template<>
struct Validatable<Blockexport::Xml::PSpaprvioReg>: mpl::true_
{
	static bool validate(const Blockexport::Xml::PSpaprvioReg::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PAliasName

namespace Blockexport
{
namespace Xml
{
struct PAliasName
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Blockexport

template<>
struct Validatable<Blockexport::Xml::PAliasName>: mpl::true_
{
	static bool validate(const Blockexport::Xml::PAliasName::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData2077

namespace Blockexport
{
namespace Xml
{
struct PData2077
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Blockexport

template<>
struct Validatable<Blockexport::Xml::PData2077>: mpl::true_
{
	static bool validate(const Blockexport::Xml::PData2077::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData2079

namespace Blockexport
{
namespace Xml
{
struct PData2079
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Blockexport

template<>
struct Validatable<Blockexport::Xml::PData2079>: mpl::true_
{
	static bool validate(const Blockexport::Xml::PData2079::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData2081

namespace Blockexport
{
namespace Xml
{
struct PData2081
{
	typedef qint32 value_type;
};

} // namespace Xml
} // namespace Blockexport

template<>
struct Traits<Blockexport::Xml::PData2081>
{
	static bool parse(const QString& src_, Blockexport::Xml::PData2081::value_type& dst_);

	static QString generate(Blockexport::Xml::PData2081::value_type src_);

};

template<>
struct Validatable<Blockexport::Xml::PData2081>: mpl::true_
{
	static bool validate(Blockexport::Xml::PData2081::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PCcwSsidRange

namespace Blockexport
{
namespace Xml
{
struct PCcwSsidRange
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Blockexport

template<>
struct Validatable<Blockexport::Xml::PCcwSsidRange>: mpl::true_
{
	static bool validate(const Blockexport::Xml::PCcwSsidRange::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData2083

namespace Blockexport
{
namespace Xml
{
struct PData2083
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Blockexport

template<>
struct Validatable<Blockexport::Xml::PData2083>: mpl::true_
{
	static bool validate(const Blockexport::Xml::PData2083::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData2085

namespace Blockexport
{
namespace Xml
{
struct PData2085
{
	typedef qint32 value_type;
};

} // namespace Xml
} // namespace Blockexport

template<>
struct Traits<Blockexport::Xml::PData2085>
{
	static bool parse(const QString& src_, Blockexport::Xml::PData2085::value_type& dst_);

	static QString generate(Blockexport::Xml::PData2085::value_type src_);

};

template<>
struct Validatable<Blockexport::Xml::PData2085>: mpl::true_
{
	static bool validate(Blockexport::Xml::PData2085::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct VName

namespace Blockexport
{
namespace Xml
{
typedef Choice<mpl::vector<PDiskTarget, PAbsFilePath > > VNameImpl;
typedef VNameImpl::value_type VName;

} // namespace Xml
} // namespace Blockexport

template<>
struct Traits<Blockexport::Xml::VName>
{
	static bool parse(const QString& src_, Blockexport::Xml::VName& dst_);

	static QString generate(const Blockexport::Xml::VName& src_);

};

} // namespace Libvirt

#endif // __BLOCKEXPORT_DATA_H__
