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

#ifndef __BLOCKSNAPSHOT_DATA_H__
#define __BLOCKSNAPSHOT_DATA_H__
#include <QString>
#include <QDateTime>
#include "base.h"
#include "patterns.h"
#include "blocksnapshot_enum.h"

namespace Libvirt
{
///////////////////////////////////////////////////////////////////////////////
// struct PUnsignedInt

namespace Blocksnapshot
{
namespace Xml
{
struct PUnsignedInt
{
	typedef quint32 value_type;
};

} // namespace Xml
} // namespace Blocksnapshot

template<>
struct Traits<Blocksnapshot::Xml::PUnsignedInt>
{
	static bool parse(const QString& src_, Blocksnapshot::Xml::PUnsignedInt::value_type& dst_);

	static QString generate(Blocksnapshot::Xml::PUnsignedInt::value_type src_);

};


///////////////////////////////////////////////////////////////////////////////
// struct PUnsignedLong

namespace Blocksnapshot
{
namespace Xml
{
struct PUnsignedLong
{
	typedef ulong value_type;
};

} // namespace Xml
} // namespace Blocksnapshot

template<>
struct Traits<Blocksnapshot::Xml::PUnsignedLong>
{
	static bool parse(const QString& src_, Blocksnapshot::Xml::PUnsignedLong::value_type& dst_);

	static QString generate(Blocksnapshot::Xml::PUnsignedLong::value_type src_);

};


///////////////////////////////////////////////////////////////////////////////
// struct PHexuint

namespace Blocksnapshot
{
namespace Xml
{
struct PHexuint
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Blocksnapshot

template<>
struct Validatable<Blocksnapshot::Xml::PHexuint>: mpl::true_
{
	static bool validate(const Blocksnapshot::Xml::PHexuint::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PPositiveInteger

namespace Blocksnapshot
{
namespace Xml
{
struct PPositiveInteger
{
	typedef quint32 value_type;
};

} // namespace Xml
} // namespace Blocksnapshot

template<>
struct Traits<Blocksnapshot::Xml::PPositiveInteger>
{
	static bool parse(const QString& src_, Blocksnapshot::Xml::PPositiveInteger::value_type& dst_);

	static QString generate(Blocksnapshot::Xml::PPositiveInteger::value_type src_);

};


///////////////////////////////////////////////////////////////////////////////
// struct POctalMode

namespace Blocksnapshot
{
namespace Xml
{
struct POctalMode
{
	typedef quint32 value_type;
};

} // namespace Xml
} // namespace Blocksnapshot

template<>
struct Traits<Blocksnapshot::Xml::POctalMode>
{
	static bool parse(const QString& src_, Blocksnapshot::Xml::POctalMode::value_type& dst_);

	static QString generate(Blocksnapshot::Xml::POctalMode::value_type src_);

};


///////////////////////////////////////////////////////////////////////////////
// struct PData3196

namespace Blocksnapshot
{
namespace Xml
{
struct PData3196
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Blocksnapshot

template<>
struct Validatable<Blocksnapshot::Xml::PData3196>: mpl::true_
{
	static bool validate(const Blocksnapshot::Xml::PData3196::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData3198

namespace Blocksnapshot
{
namespace Xml
{
struct PData3198
{
	typedef qint32 value_type;
};

} // namespace Xml
} // namespace Blocksnapshot

template<>
struct Traits<Blocksnapshot::Xml::PData3198>
{
	static bool parse(const QString& src_, Blocksnapshot::Xml::PData3198::value_type& dst_);

	static QString generate(Blocksnapshot::Xml::PData3198::value_type src_);

};

template<>
struct Validatable<Blocksnapshot::Xml::PData3198>: mpl::true_
{
	static bool validate(Blocksnapshot::Xml::PData3198::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData3200

namespace Blocksnapshot
{
namespace Xml
{
struct PData3200
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Blocksnapshot

template<>
struct Validatable<Blocksnapshot::Xml::PData3200>: mpl::true_
{
	static bool validate(const Blocksnapshot::Xml::PData3200::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData3202

namespace Blocksnapshot
{
namespace Xml
{
struct PData3202
{
	typedef qint32 value_type;
};

} // namespace Xml
} // namespace Blocksnapshot

template<>
struct Traits<Blocksnapshot::Xml::PData3202>
{
	static bool parse(const QString& src_, Blocksnapshot::Xml::PData3202::value_type& dst_);

	static QString generate(Blocksnapshot::Xml::PData3202::value_type src_);

};

template<>
struct Validatable<Blocksnapshot::Xml::PData3202>: mpl::true_
{
	static bool validate(Blocksnapshot::Xml::PData3202::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData3204

namespace Blocksnapshot
{
namespace Xml
{
struct PData3204
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Blocksnapshot

template<>
struct Validatable<Blocksnapshot::Xml::PData3204>: mpl::true_
{
	static bool validate(const Blocksnapshot::Xml::PData3204::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData3206

namespace Blocksnapshot
{
namespace Xml
{
struct PData3206
{
	typedef qint32 value_type;
};

} // namespace Xml
} // namespace Blocksnapshot

template<>
struct Traits<Blocksnapshot::Xml::PData3206>
{
	static bool parse(const QString& src_, Blocksnapshot::Xml::PData3206::value_type& dst_);

	static QString generate(Blocksnapshot::Xml::PData3206::value_type src_);

};

template<>
struct Validatable<Blocksnapshot::Xml::PData3206>: mpl::true_
{
	static bool validate(Blocksnapshot::Xml::PData3206::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData3208

namespace Blocksnapshot
{
namespace Xml
{
struct PData3208
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Blocksnapshot

template<>
struct Validatable<Blocksnapshot::Xml::PData3208>: mpl::true_
{
	static bool validate(const Blocksnapshot::Xml::PData3208::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData3210

namespace Blocksnapshot
{
namespace Xml
{
struct PData3210
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Blocksnapshot

template<>
struct Validatable<Blocksnapshot::Xml::PData3210>: mpl::true_
{
	static bool validate(const Blocksnapshot::Xml::PData3210::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PUniMacAddr

namespace Blocksnapshot
{
namespace Xml
{
struct PUniMacAddr
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Blocksnapshot

template<>
struct Validatable<Blocksnapshot::Xml::PUniMacAddr>: mpl::true_
{
	static bool validate(const Blocksnapshot::Xml::PUniMacAddr::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PMultiMacAddr

namespace Blocksnapshot
{
namespace Xml
{
struct PMultiMacAddr
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Blocksnapshot

template<>
struct Validatable<Blocksnapshot::Xml::PMultiMacAddr>: mpl::true_
{
	static bool validate(const Blocksnapshot::Xml::PMultiMacAddr::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PMacAddr

namespace Blocksnapshot
{
namespace Xml
{
struct PMacAddr
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Blocksnapshot

template<>
struct Validatable<Blocksnapshot::Xml::PMacAddr>: mpl::true_
{
	static bool validate(const Blocksnapshot::Xml::PMacAddr::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PDuidLLT

namespace Blocksnapshot
{
namespace Xml
{
struct PDuidLLT
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Blocksnapshot

template<>
struct Validatable<Blocksnapshot::Xml::PDuidLLT>: mpl::true_
{
	static bool validate(const Blocksnapshot::Xml::PDuidLLT::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PDuidEN

namespace Blocksnapshot
{
namespace Xml
{
struct PDuidEN
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Blocksnapshot

template<>
struct Validatable<Blocksnapshot::Xml::PDuidEN>: mpl::true_
{
	static bool validate(const Blocksnapshot::Xml::PDuidEN::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PDuidLL

namespace Blocksnapshot
{
namespace Xml
{
struct PDuidLL
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Blocksnapshot

template<>
struct Validatable<Blocksnapshot::Xml::PDuidLL>: mpl::true_
{
	static bool validate(const Blocksnapshot::Xml::PDuidLL::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PDuidUUID

namespace Blocksnapshot
{
namespace Xml
{
struct PDuidUUID
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Blocksnapshot

template<>
struct Validatable<Blocksnapshot::Xml::PDuidUUID>: mpl::true_
{
	static bool validate(const Blocksnapshot::Xml::PDuidUUID::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PIpv4Addr

namespace Blocksnapshot
{
namespace Xml
{
struct PIpv4Addr
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Blocksnapshot

template<>
struct Validatable<Blocksnapshot::Xml::PIpv4Addr>: mpl::true_
{
	static bool validate(const Blocksnapshot::Xml::PIpv4Addr::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PIpv6Addr

namespace Blocksnapshot
{
namespace Xml
{
struct PIpv6Addr
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Blocksnapshot

template<>
struct Validatable<Blocksnapshot::Xml::PIpv6Addr>: mpl::true_
{
	static bool validate(const Blocksnapshot::Xml::PIpv6Addr::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PIpv4Prefix

namespace Blocksnapshot
{
namespace Xml
{
struct PIpv4Prefix
{
	typedef quint32 value_type;
};

} // namespace Xml
} // namespace Blocksnapshot

template<>
struct Traits<Blocksnapshot::Xml::PIpv4Prefix>
{
	static bool parse(const QString& src_, Blocksnapshot::Xml::PIpv4Prefix::value_type& dst_);

	static QString generate(Blocksnapshot::Xml::PIpv4Prefix::value_type src_);

};

template<>
struct Validatable<Blocksnapshot::Xml::PIpv4Prefix>: mpl::true_
{
	static bool validate(Blocksnapshot::Xml::PIpv4Prefix::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PIpv6Prefix

namespace Blocksnapshot
{
namespace Xml
{
struct PIpv6Prefix
{
	typedef quint32 value_type;
};

} // namespace Xml
} // namespace Blocksnapshot

template<>
struct Traits<Blocksnapshot::Xml::PIpv6Prefix>
{
	static bool parse(const QString& src_, Blocksnapshot::Xml::PIpv6Prefix::value_type& dst_);

	static QString generate(Blocksnapshot::Xml::PIpv6Prefix::value_type src_);

};

template<>
struct Validatable<Blocksnapshot::Xml::PIpv6Prefix>: mpl::true_
{
	static bool validate(Blocksnapshot::Xml::PIpv6Prefix::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PGenericName

namespace Blocksnapshot
{
namespace Xml
{
struct PGenericName
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Blocksnapshot

template<>
struct Validatable<Blocksnapshot::Xml::PGenericName>: mpl::true_
{
	static bool validate(const Blocksnapshot::Xml::PGenericName::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PDnsName

namespace Blocksnapshot
{
namespace Xml
{
struct PDnsName
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Blocksnapshot

template<>
struct Validatable<Blocksnapshot::Xml::PDnsName>: mpl::true_
{
	static bool validate(const Blocksnapshot::Xml::PDnsName::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PDeviceName

namespace Blocksnapshot
{
namespace Xml
{
struct PDeviceName
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Blocksnapshot

template<>
struct Validatable<Blocksnapshot::Xml::PDeviceName>: mpl::true_
{
	static bool validate(const Blocksnapshot::Xml::PDeviceName::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PFilePath

namespace Blocksnapshot
{
namespace Xml
{
struct PFilePath
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Blocksnapshot

template<>
struct Validatable<Blocksnapshot::Xml::PFilePath>: mpl::true_
{
	static bool validate(const Blocksnapshot::Xml::PFilePath::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PDirPath

namespace Blocksnapshot
{
namespace Xml
{
struct PDirPath
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Blocksnapshot

template<>
struct Validatable<Blocksnapshot::Xml::PDirPath>: mpl::true_
{
	static bool validate(const Blocksnapshot::Xml::PDirPath::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PAbsFilePath

namespace Blocksnapshot
{
namespace Xml
{
struct PAbsFilePath
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Blocksnapshot

template<>
struct Validatable<Blocksnapshot::Xml::PAbsFilePath>: mpl::true_
{
	static bool validate(const Blocksnapshot::Xml::PAbsFilePath::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PAbsDirPath

namespace Blocksnapshot
{
namespace Xml
{
struct PAbsDirPath
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Blocksnapshot

template<>
struct Validatable<Blocksnapshot::Xml::PAbsDirPath>: mpl::true_
{
	static bool validate(const Blocksnapshot::Xml::PAbsDirPath::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PUnit

namespace Blocksnapshot
{
namespace Xml
{
struct PUnit
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Blocksnapshot

template<>
struct Validatable<Blocksnapshot::Xml::PUnit>: mpl::true_
{
	static bool validate(const Blocksnapshot::Xml::PUnit::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PPciDomain

namespace Blocksnapshot
{
namespace Xml
{
struct PPciDomain
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Blocksnapshot

template<>
struct Validatable<Blocksnapshot::Xml::PPciDomain>: mpl::true_
{
	static bool validate(const Blocksnapshot::Xml::PPciDomain::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PPciBus

namespace Blocksnapshot
{
namespace Xml
{
struct PPciBus
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Blocksnapshot

template<>
struct Validatable<Blocksnapshot::Xml::PPciBus>: mpl::true_
{
	static bool validate(const Blocksnapshot::Xml::PPciBus::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PPciSlot

namespace Blocksnapshot
{
namespace Xml
{
struct PPciSlot
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Blocksnapshot

template<>
struct Validatable<Blocksnapshot::Xml::PPciSlot>: mpl::true_
{
	static bool validate(const Blocksnapshot::Xml::PPciSlot::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PPciFunc

namespace Blocksnapshot
{
namespace Xml
{
struct PPciFunc
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Blocksnapshot

template<>
struct Validatable<Blocksnapshot::Xml::PPciFunc>: mpl::true_
{
	static bool validate(const Blocksnapshot::Xml::PPciFunc::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PWwn

namespace Blocksnapshot
{
namespace Xml
{
struct PWwn
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Blocksnapshot

template<>
struct Validatable<Blocksnapshot::Xml::PWwn>: mpl::true_
{
	static bool validate(const Blocksnapshot::Xml::PWwn::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData3220

namespace Blocksnapshot
{
namespace Xml
{
struct PData3220
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Blocksnapshot

template<>
struct Validatable<Blocksnapshot::Xml::PData3220>: mpl::true_
{
	static bool validate(const Blocksnapshot::Xml::PData3220::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData3222

namespace Blocksnapshot
{
namespace Xml
{
struct PData3222
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Blocksnapshot

template<>
struct Validatable<Blocksnapshot::Xml::PData3222>: mpl::true_
{
	static bool validate(const Blocksnapshot::Xml::PData3222::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData3224

namespace Blocksnapshot
{
namespace Xml
{
struct PData3224
{
	typedef qint32 value_type;
};

} // namespace Xml
} // namespace Blocksnapshot

template<>
struct Traits<Blocksnapshot::Xml::PData3224>
{
	static bool parse(const QString& src_, Blocksnapshot::Xml::PData3224::value_type& dst_);

	static QString generate(Blocksnapshot::Xml::PData3224::value_type src_);

};

template<>
struct Validatable<Blocksnapshot::Xml::PData3224>: mpl::true_
{
	static bool validate(Blocksnapshot::Xml::PData3224::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PCcwSsidRange

namespace Blocksnapshot
{
namespace Xml
{
struct PCcwSsidRange
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Blocksnapshot

template<>
struct Validatable<Blocksnapshot::Xml::PCcwSsidRange>: mpl::true_
{
	static bool validate(const Blocksnapshot::Xml::PCcwSsidRange::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData3226

namespace Blocksnapshot
{
namespace Xml
{
struct PData3226
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Blocksnapshot

template<>
struct Validatable<Blocksnapshot::Xml::PData3226>: mpl::true_
{
	static bool validate(const Blocksnapshot::Xml::PData3226::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData3228

namespace Blocksnapshot
{
namespace Xml
{
struct PData3228
{
	typedef qint32 value_type;
};

} // namespace Xml
} // namespace Blocksnapshot

template<>
struct Traits<Blocksnapshot::Xml::PData3228>
{
	static bool parse(const QString& src_, Blocksnapshot::Xml::PData3228::value_type& dst_);

	static QString generate(Blocksnapshot::Xml::PData3228::value_type src_);

};

template<>
struct Validatable<Blocksnapshot::Xml::PData3228>: mpl::true_
{
	static bool validate(Blocksnapshot::Xml::PData3228::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PCpuset

namespace Blocksnapshot
{
namespace Xml
{
struct PCpuset
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Blocksnapshot

template<>
struct Validatable<Blocksnapshot::Xml::PCpuset>: mpl::true_
{
	static bool validate(const Blocksnapshot::Xml::PCpuset::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PVolName

namespace Blocksnapshot
{
namespace Xml
{
struct PVolName
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Blocksnapshot

template<>
struct Validatable<Blocksnapshot::Xml::PVolName>: mpl::true_
{
	static bool validate(const Blocksnapshot::Xml::PVolName::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PPortNumber

namespace Blocksnapshot
{
namespace Xml
{
struct PPortNumber
{
	typedef qint32 value_type;
};

} // namespace Xml
} // namespace Blocksnapshot

template<>
struct Traits<Blocksnapshot::Xml::PPortNumber>
{
	static bool parse(const QString& src_, Blocksnapshot::Xml::PPortNumber::value_type& dst_);

	static QString generate(Blocksnapshot::Xml::PPortNumber::value_type src_);

};

template<>
struct Validatable<Blocksnapshot::Xml::PPortNumber>: mpl::true_
{
	static bool validate(Blocksnapshot::Xml::PPortNumber::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PIobase

namespace Blocksnapshot
{
namespace Xml
{
struct PIobase
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Blocksnapshot

template<>
struct Validatable<Blocksnapshot::Xml::PIobase>: mpl::true_
{
	static bool validate(const Blocksnapshot::Xml::PIobase::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PIrq

namespace Blocksnapshot
{
namespace Xml
{
struct PIrq
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Blocksnapshot

template<>
struct Validatable<Blocksnapshot::Xml::PIrq>: mpl::true_
{
	static bool validate(const Blocksnapshot::Xml::PIrq::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PCompat

namespace Blocksnapshot
{
namespace Xml
{
struct PCompat
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Blocksnapshot

template<>
struct Validatable<Blocksnapshot::Xml::PCompat>: mpl::true_
{
	static bool validate(const Blocksnapshot::Xml::PCompat::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PVirtualPortProfileID

namespace Blocksnapshot
{
namespace Xml
{
struct PVirtualPortProfileID
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Blocksnapshot

template<>
struct Validatable<Blocksnapshot::Xml::PVirtualPortProfileID>: mpl::true_
{
	static bool validate(const Blocksnapshot::Xml::PVirtualPortProfileID::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PSpeed

namespace Blocksnapshot
{
namespace Xml
{
struct PSpeed
{
	typedef quint32 value_type;
};

} // namespace Xml
} // namespace Blocksnapshot

template<>
struct Traits<Blocksnapshot::Xml::PSpeed>
{
	static bool parse(const QString& src_, Blocksnapshot::Xml::PSpeed::value_type& dst_);

	static QString generate(Blocksnapshot::Xml::PSpeed::value_type src_);

};

template<>
struct Validatable<Blocksnapshot::Xml::PSpeed>: mpl::true_
{
	static bool validate(Blocksnapshot::Xml::PSpeed::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PBurstSize

namespace Blocksnapshot
{
namespace Xml
{
struct PBurstSize
{
	typedef quint32 value_type;
};

} // namespace Xml
} // namespace Blocksnapshot

template<>
struct Traits<Blocksnapshot::Xml::PBurstSize>
{
	static bool parse(const QString& src_, Blocksnapshot::Xml::PBurstSize::value_type& dst_);

	static QString generate(Blocksnapshot::Xml::PBurstSize::value_type src_);

};

template<>
struct Validatable<Blocksnapshot::Xml::PBurstSize>: mpl::true_
{
	static bool validate(Blocksnapshot::Xml::PBurstSize::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PUnsignedShort

namespace Blocksnapshot
{
namespace Xml
{
struct PUnsignedShort
{
	typedef qint32 value_type;
};

} // namespace Xml
} // namespace Blocksnapshot

template<>
struct Traits<Blocksnapshot::Xml::PUnsignedShort>
{
	static bool parse(const QString& src_, Blocksnapshot::Xml::PUnsignedShort::value_type& dst_);

	static QString generate(Blocksnapshot::Xml::PUnsignedShort::value_type src_);

};

template<>
struct Validatable<Blocksnapshot::Xml::PUnsignedShort>: mpl::true_
{
	static bool validate(Blocksnapshot::Xml::PUnsignedShort::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PProtocol

namespace Blocksnapshot
{
namespace Xml
{
struct PProtocol
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Blocksnapshot

template<>
struct Validatable<Blocksnapshot::Xml::PProtocol>: mpl::true_
{
	static bool validate(const Blocksnapshot::Xml::PProtocol::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PAddrFamily

namespace Blocksnapshot
{
namespace Xml
{
struct PAddrFamily
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Blocksnapshot

template<>
struct Validatable<Blocksnapshot::Xml::PAddrFamily>: mpl::true_
{
	static bool validate(const Blocksnapshot::Xml::PAddrFamily::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PId

namespace Blocksnapshot
{
namespace Xml
{
struct PId
{
	typedef quint32 value_type;
};

} // namespace Xml
} // namespace Blocksnapshot

template<>
struct Traits<Blocksnapshot::Xml::PId>
{
	static bool parse(const QString& src_, Blocksnapshot::Xml::PId::value_type& dst_);

	static QString generate(Blocksnapshot::Xml::PId::value_type src_);

};

template<>
struct Validatable<Blocksnapshot::Xml::PId>: mpl::true_
{
	static bool validate(Blocksnapshot::Xml::PId::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PPort

namespace Blocksnapshot
{
namespace Xml
{
struct PPort
{
	typedef qint32 value_type;
};

} // namespace Xml
} // namespace Blocksnapshot

template<>
struct Traits<Blocksnapshot::Xml::PPort>
{
	static bool parse(const QString& src_, Blocksnapshot::Xml::PPort::value_type& dst_);

	static QString generate(Blocksnapshot::Xml::PPort::value_type src_);

};

template<>
struct Validatable<Blocksnapshot::Xml::PPort>: mpl::true_
{
	static bool validate(Blocksnapshot::Xml::PPort::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PTitle

namespace Blocksnapshot
{
namespace Xml
{
struct PTitle
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Blocksnapshot

template<>
struct Validatable<Blocksnapshot::Xml::PTitle>: mpl::true_
{
	static bool validate(const Blocksnapshot::Xml::PTitle::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PMachine

namespace Blocksnapshot
{
namespace Xml
{
struct PMachine
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Blocksnapshot

template<>
struct Validatable<Blocksnapshot::Xml::PMachine>: mpl::true_
{
	static bool validate(const Blocksnapshot::Xml::PMachine::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PReadIopsSec

namespace Blocksnapshot
{
namespace Xml
{
struct PReadIopsSec
{
	typedef quint32 value_type;
};

} // namespace Xml
} // namespace Blocksnapshot

template<>
struct Traits<Blocksnapshot::Xml::PReadIopsSec>
{
	static bool parse(const QString& src_, Blocksnapshot::Xml::PReadIopsSec::value_type& dst_);

	static QString generate(Blocksnapshot::Xml::PReadIopsSec::value_type src_);

};


///////////////////////////////////////////////////////////////////////////////
// struct PWriteIopsSec

namespace Blocksnapshot
{
namespace Xml
{
struct PWriteIopsSec
{
	typedef quint32 value_type;
};

} // namespace Xml
} // namespace Blocksnapshot

template<>
struct Traits<Blocksnapshot::Xml::PWriteIopsSec>
{
	static bool parse(const QString& src_, Blocksnapshot::Xml::PWriteIopsSec::value_type& dst_);

	static QString generate(Blocksnapshot::Xml::PWriteIopsSec::value_type src_);

};


///////////////////////////////////////////////////////////////////////////////
// struct PReadBytesSec

namespace Blocksnapshot
{
namespace Xml
{
struct PReadBytesSec
{
	typedef ulong value_type;
};

} // namespace Xml
} // namespace Blocksnapshot

template<>
struct Traits<Blocksnapshot::Xml::PReadBytesSec>
{
	static bool parse(const QString& src_, Blocksnapshot::Xml::PReadBytesSec::value_type& dst_);

	static QString generate(Blocksnapshot::Xml::PReadBytesSec::value_type src_);

};


///////////////////////////////////////////////////////////////////////////////
// struct PWriteBytesSec

namespace Blocksnapshot
{
namespace Xml
{
struct PWriteBytesSec
{
	typedef ulong value_type;
};

} // namespace Xml
} // namespace Blocksnapshot

template<>
struct Traits<Blocksnapshot::Xml::PWriteBytesSec>
{
	static bool parse(const QString& src_, Blocksnapshot::Xml::PWriteBytesSec::value_type& dst_);

	static QString generate(Blocksnapshot::Xml::PWriteBytesSec::value_type src_);

};


///////////////////////////////////////////////////////////////////////////////
// struct PVendor

namespace Blocksnapshot
{
namespace Xml
{
struct PVendor
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Blocksnapshot

template<>
struct Validatable<Blocksnapshot::Xml::PVendor>: mpl::true_
{
	static bool validate(const Blocksnapshot::Xml::PVendor::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PProduct

namespace Blocksnapshot
{
namespace Xml
{
struct PProduct
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Blocksnapshot

template<>
struct Validatable<Blocksnapshot::Xml::PProduct>: mpl::true_
{
	static bool validate(const Blocksnapshot::Xml::PProduct::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PDiskTarget

namespace Blocksnapshot
{
namespace Xml
{
struct PDiskTarget
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Blocksnapshot

template<>
struct Validatable<Blocksnapshot::Xml::PDiskTarget>: mpl::true_
{
	static bool validate(const Blocksnapshot::Xml::PDiskTarget::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PCyls

namespace Blocksnapshot
{
namespace Xml
{
struct PCyls
{
	typedef qint32 value_type;
};

} // namespace Xml
} // namespace Blocksnapshot

template<>
struct Traits<Blocksnapshot::Xml::PCyls>
{
	static bool parse(const QString& src_, Blocksnapshot::Xml::PCyls::value_type& dst_);

	static QString generate(Blocksnapshot::Xml::PCyls::value_type src_);

};


///////////////////////////////////////////////////////////////////////////////
// struct PHeads

namespace Blocksnapshot
{
namespace Xml
{
struct PHeads
{
	typedef qint32 value_type;
};

} // namespace Xml
} // namespace Blocksnapshot

template<>
struct Traits<Blocksnapshot::Xml::PHeads>
{
	static bool parse(const QString& src_, Blocksnapshot::Xml::PHeads::value_type& dst_);

	static QString generate(Blocksnapshot::Xml::PHeads::value_type src_);

};


///////////////////////////////////////////////////////////////////////////////
// struct PSecs

namespace Blocksnapshot
{
namespace Xml
{
struct PSecs
{
	typedef qint32 value_type;
};

} // namespace Xml
} // namespace Blocksnapshot

template<>
struct Traits<Blocksnapshot::Xml::PSecs>
{
	static bool parse(const QString& src_, Blocksnapshot::Xml::PSecs::value_type& dst_);

	static QString generate(Blocksnapshot::Xml::PSecs::value_type src_);

};


///////////////////////////////////////////////////////////////////////////////
// struct PLogicalBlockSize

namespace Blocksnapshot
{
namespace Xml
{
struct PLogicalBlockSize
{
	typedef qint32 value_type;
};

} // namespace Xml
} // namespace Blocksnapshot

template<>
struct Traits<Blocksnapshot::Xml::PLogicalBlockSize>
{
	static bool parse(const QString& src_, Blocksnapshot::Xml::PLogicalBlockSize::value_type& dst_);

	static QString generate(Blocksnapshot::Xml::PLogicalBlockSize::value_type src_);

};


///////////////////////////////////////////////////////////////////////////////
// struct PPhysicalBlockSize

namespace Blocksnapshot
{
namespace Xml
{
struct PPhysicalBlockSize
{
	typedef qint32 value_type;
};

} // namespace Xml
} // namespace Blocksnapshot

template<>
struct Traits<Blocksnapshot::Xml::PPhysicalBlockSize>
{
	static bool parse(const QString& src_, Blocksnapshot::Xml::PPhysicalBlockSize::value_type& dst_);

	static QString generate(Blocksnapshot::Xml::PPhysicalBlockSize::value_type src_);

};


///////////////////////////////////////////////////////////////////////////////
// struct PType

namespace Blocksnapshot
{
namespace Xml
{
struct PType
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Blocksnapshot

template<>
struct Validatable<Blocksnapshot::Xml::PType>: mpl::true_
{
	static bool validate(const Blocksnapshot::Xml::PType::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PPasswdValidTo

namespace Blocksnapshot
{
namespace Xml
{
struct PPasswdValidTo
{
	typedef QDateTime value_type;
};

} // namespace Xml
} // namespace Blocksnapshot

template<>
struct Traits<Blocksnapshot::Xml::PPasswdValidTo>
{
	static bool parse(const QString& src_, Blocksnapshot::Xml::PPasswdValidTo::value_type& dst_);

	static QString generate(const Blocksnapshot::Xml::PPasswdValidTo::value_type& src_);

};


///////////////////////////////////////////////////////////////////////////////
// struct PVendorId

namespace Blocksnapshot
{
namespace Xml
{
struct PVendorId
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Blocksnapshot

template<>
struct Validatable<Blocksnapshot::Xml::PVendorId>: mpl::true_
{
	static bool validate(const Blocksnapshot::Xml::PVendorId::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PSysinfoValue

namespace Blocksnapshot
{
namespace Xml
{
struct PSysinfoValue
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Blocksnapshot

template<>
struct Validatable<Blocksnapshot::Xml::PSysinfoValue>: mpl::true_
{
	static bool validate(const Blocksnapshot::Xml::PSysinfoValue::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PFilter

namespace Blocksnapshot
{
namespace Xml
{
struct PFilter
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Blocksnapshot


///////////////////////////////////////////////////////////////////////////////
// struct PTotalBytesSec

namespace Blocksnapshot
{
namespace Xml
{
struct PTotalBytesSec
{
	typedef ulong value_type;
};

} // namespace Xml
} // namespace Blocksnapshot

template<>
struct Traits<Blocksnapshot::Xml::PTotalBytesSec>
{
	static bool parse(const QString& src_, Blocksnapshot::Xml::PTotalBytesSec::value_type& dst_);

	static QString generate(Blocksnapshot::Xml::PTotalBytesSec::value_type src_);

};


///////////////////////////////////////////////////////////////////////////////
// struct PTotalIopsSec

namespace Blocksnapshot
{
namespace Xml
{
struct PTotalIopsSec
{
	typedef ulong value_type;
};

} // namespace Xml
} // namespace Blocksnapshot

template<>
struct Traits<Blocksnapshot::Xml::PTotalIopsSec>
{
	static bool parse(const QString& src_, Blocksnapshot::Xml::PTotalIopsSec::value_type& dst_);

	static QString generate(Blocksnapshot::Xml::PTotalIopsSec::value_type src_);

};


///////////////////////////////////////////////////////////////////////////////
// struct PReadIopsSec1

namespace Blocksnapshot
{
namespace Xml
{
struct PReadIopsSec1
{
	typedef ulong value_type;
};

} // namespace Xml
} // namespace Blocksnapshot

template<>
struct Traits<Blocksnapshot::Xml::PReadIopsSec1>
{
	static bool parse(const QString& src_, Blocksnapshot::Xml::PReadIopsSec1::value_type& dst_);

	static QString generate(Blocksnapshot::Xml::PReadIopsSec1::value_type src_);

};


///////////////////////////////////////////////////////////////////////////////
// struct PWriteIopsSec1

namespace Blocksnapshot
{
namespace Xml
{
struct PWriteIopsSec1
{
	typedef ulong value_type;
};

} // namespace Xml
} // namespace Blocksnapshot

template<>
struct Traits<Blocksnapshot::Xml::PWriteIopsSec1>
{
	static bool parse(const QString& src_, Blocksnapshot::Xml::PWriteIopsSec1::value_type& dst_);

	static QString generate(Blocksnapshot::Xml::PWriteIopsSec1::value_type src_);

};


///////////////////////////////////////////////////////////////////////////////
// struct PRetries

namespace Blocksnapshot
{
namespace Xml
{
struct PRetries
{
	typedef quint32 value_type;
};

} // namespace Xml
} // namespace Blocksnapshot

template<>
struct Traits<Blocksnapshot::Xml::PRetries>
{
	static bool parse(const QString& src_, Blocksnapshot::Xml::PRetries::value_type& dst_);

	static QString generate(Blocksnapshot::Xml::PRetries::value_type src_);

};


///////////////////////////////////////////////////////////////////////////////
// struct PValue

namespace Blocksnapshot
{
namespace Xml
{
struct PValue
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Blocksnapshot

template<>
struct Validatable<Blocksnapshot::Xml::PValue>: mpl::true_
{
	static bool validate(const Blocksnapshot::Xml::PValue::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PCountCPU

namespace Blocksnapshot
{
namespace Xml
{
struct PCountCPU
{
	typedef quint16 value_type;
};

} // namespace Xml
} // namespace Blocksnapshot

template<>
struct Traits<Blocksnapshot::Xml::PCountCPU>
{
	static bool parse(const QString& src_, Blocksnapshot::Xml::PCountCPU::value_type& dst_);

	static QString generate(Blocksnapshot::Xml::PCountCPU::value_type src_);

};

template<>
struct Validatable<Blocksnapshot::Xml::PCountCPU>: mpl::true_
{
	static bool validate(Blocksnapshot::Xml::PCountCPU::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PVcpuid

namespace Blocksnapshot
{
namespace Xml
{
struct PVcpuid
{
	typedef quint16 value_type;
};

} // namespace Xml
} // namespace Blocksnapshot

template<>
struct Traits<Blocksnapshot::Xml::PVcpuid>
{
	static bool parse(const QString& src_, Blocksnapshot::Xml::PVcpuid::value_type& dst_);

	static QString generate(Blocksnapshot::Xml::PVcpuid::value_type src_);

};


///////////////////////////////////////////////////////////////////////////////
// struct PCpushares

namespace Blocksnapshot
{
namespace Xml
{
struct PCpushares
{
	typedef quint32 value_type;
};

} // namespace Xml
} // namespace Blocksnapshot

template<>
struct Traits<Blocksnapshot::Xml::PCpushares>
{
	static bool parse(const QString& src_, Blocksnapshot::Xml::PCpushares::value_type& dst_);

	static QString generate(Blocksnapshot::Xml::PCpushares::value_type src_);

};


///////////////////////////////////////////////////////////////////////////////
// struct PCpuperiod

namespace Blocksnapshot
{
namespace Xml
{
struct PCpuperiod
{
	typedef ulong value_type;
};

} // namespace Xml
} // namespace Blocksnapshot

template<>
struct Traits<Blocksnapshot::Xml::PCpuperiod>
{
	static bool parse(const QString& src_, Blocksnapshot::Xml::PCpuperiod::value_type& dst_);

	static QString generate(Blocksnapshot::Xml::PCpuperiod::value_type src_);

};

template<>
struct Validatable<Blocksnapshot::Xml::PCpuperiod>: mpl::true_
{
	static bool validate(Blocksnapshot::Xml::PCpuperiod::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PCpuquota

namespace Blocksnapshot
{
namespace Xml
{
struct PCpuquota
{
	typedef long value_type;
};

} // namespace Xml
} // namespace Blocksnapshot

template<>
struct Traits<Blocksnapshot::Xml::PCpuquota>
{
	static bool parse(const QString& src_, Blocksnapshot::Xml::PCpuquota::value_type& dst_);

	static QString generate(Blocksnapshot::Xml::PCpuquota::value_type src_);

};

template<>
struct Validatable<Blocksnapshot::Xml::PCpuquota>: mpl::true_
{
	static bool validate(Blocksnapshot::Xml::PCpuquota::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PRebootTimeoutDelay

namespace Blocksnapshot
{
namespace Xml
{
struct PRebootTimeoutDelay
{
	typedef qint16 value_type;
};

} // namespace Xml
} // namespace Blocksnapshot

template<>
struct Traits<Blocksnapshot::Xml::PRebootTimeoutDelay>
{
	static bool parse(const QString& src_, Blocksnapshot::Xml::PRebootTimeoutDelay::value_type& dst_);

	static QString generate(Blocksnapshot::Xml::PRebootTimeoutDelay::value_type src_);

};

template<>
struct Validatable<Blocksnapshot::Xml::PRebootTimeoutDelay>: mpl::true_
{
	static bool validate(Blocksnapshot::Xml::PRebootTimeoutDelay::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PWeight

namespace Blocksnapshot
{
namespace Xml
{
struct PWeight
{
	typedef quint32 value_type;
};

} // namespace Xml
} // namespace Blocksnapshot

template<>
struct Traits<Blocksnapshot::Xml::PWeight>
{
	static bool parse(const QString& src_, Blocksnapshot::Xml::PWeight::value_type& dst_);

	static QString generate(Blocksnapshot::Xml::PWeight::value_type src_);

};

template<>
struct Validatable<Blocksnapshot::Xml::PWeight>: mpl::true_
{
	static bool validate(Blocksnapshot::Xml::PWeight::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PMemoryKB

namespace Blocksnapshot
{
namespace Xml
{
struct PMemoryKB
{
	typedef quint32 value_type;
};

} // namespace Xml
} // namespace Blocksnapshot

template<>
struct Traits<Blocksnapshot::Xml::PMemoryKB>
{
	static bool parse(const QString& src_, Blocksnapshot::Xml::PMemoryKB::value_type& dst_);

	static QString generate(Blocksnapshot::Xml::PMemoryKB::value_type src_);

};


///////////////////////////////////////////////////////////////////////////////
// struct PDomainName

namespace Blocksnapshot
{
namespace Xml
{
struct PDomainName
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Blocksnapshot

template<>
struct Validatable<Blocksnapshot::Xml::PDomainName>: mpl::true_
{
	static bool validate(const Blocksnapshot::Xml::PDomainName::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PDiskSerial

namespace Blocksnapshot
{
namespace Xml
{
struct PDiskSerial
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Blocksnapshot

template<>
struct Validatable<Blocksnapshot::Xml::PDiskSerial>: mpl::true_
{
	static bool validate(const Blocksnapshot::Xml::PDiskSerial::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PBridgeMode

namespace Blocksnapshot
{
namespace Xml
{
struct PBridgeMode
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Blocksnapshot

template<>
struct Validatable<Blocksnapshot::Xml::PBridgeMode>: mpl::true_
{
	static bool validate(const Blocksnapshot::Xml::PBridgeMode::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PAddrIPorName

namespace Blocksnapshot
{
namespace Xml
{
struct PAddrIPorName
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Blocksnapshot

template<>
struct Validatable<Blocksnapshot::Xml::PAddrIPorName>: mpl::true_
{
	static bool validate(const Blocksnapshot::Xml::PAddrIPorName::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PUsbIdDefault

namespace Blocksnapshot
{
namespace Xml
{
struct PUsbIdDefault
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Blocksnapshot

template<>
struct Validatable<Blocksnapshot::Xml::PUsbIdDefault>: mpl::true_
{
	static bool validate(const Blocksnapshot::Xml::PUsbIdDefault::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PUsbId

namespace Blocksnapshot
{
namespace Xml
{
struct PUsbId
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Blocksnapshot

template<>
struct Validatable<Blocksnapshot::Xml::PUsbId>: mpl::true_
{
	static bool validate(const Blocksnapshot::Xml::PUsbId::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PUsbVersion

namespace Blocksnapshot
{
namespace Xml
{
struct PUsbVersion
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Blocksnapshot

template<>
struct Validatable<Blocksnapshot::Xml::PUsbVersion>: mpl::true_
{
	static bool validate(const Blocksnapshot::Xml::PUsbVersion::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PUsbAddr

namespace Blocksnapshot
{
namespace Xml
{
struct PUsbAddr
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Blocksnapshot

template<>
struct Validatable<Blocksnapshot::Xml::PUsbAddr>: mpl::true_
{
	static bool validate(const Blocksnapshot::Xml::PUsbAddr::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PUsbClass

namespace Blocksnapshot
{
namespace Xml
{
struct PUsbClass
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Blocksnapshot

template<>
struct Validatable<Blocksnapshot::Xml::PUsbClass>: mpl::true_
{
	static bool validate(const Blocksnapshot::Xml::PUsbClass::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PUsbPort

namespace Blocksnapshot
{
namespace Xml
{
struct PUsbPort
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Blocksnapshot

template<>
struct Validatable<Blocksnapshot::Xml::PUsbPort>: mpl::true_
{
	static bool validate(const Blocksnapshot::Xml::PUsbPort::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PDriveController

namespace Blocksnapshot
{
namespace Xml
{
struct PDriveController
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Blocksnapshot

template<>
struct Validatable<Blocksnapshot::Xml::PDriveController>: mpl::true_
{
	static bool validate(const Blocksnapshot::Xml::PDriveController::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PDriveBus

namespace Blocksnapshot
{
namespace Xml
{
struct PDriveBus
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Blocksnapshot

template<>
struct Validatable<Blocksnapshot::Xml::PDriveBus>: mpl::true_
{
	static bool validate(const Blocksnapshot::Xml::PDriveBus::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PDriveTarget

namespace Blocksnapshot
{
namespace Xml
{
struct PDriveTarget
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Blocksnapshot

template<>
struct Validatable<Blocksnapshot::Xml::PDriveTarget>: mpl::true_
{
	static bool validate(const Blocksnapshot::Xml::PDriveTarget::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PDriveUnit

namespace Blocksnapshot
{
namespace Xml
{
struct PDriveUnit
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Blocksnapshot

template<>
struct Validatable<Blocksnapshot::Xml::PDriveUnit>: mpl::true_
{
	static bool validate(const Blocksnapshot::Xml::PDriveUnit::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PFeatureName

namespace Blocksnapshot
{
namespace Xml
{
struct PFeatureName
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Blocksnapshot

template<>
struct Validatable<Blocksnapshot::Xml::PFeatureName>: mpl::true_
{
	static bool validate(const Blocksnapshot::Xml::PFeatureName::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PTimeDelta

namespace Blocksnapshot
{
namespace Xml
{
struct PTimeDelta
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Blocksnapshot

template<>
struct Validatable<Blocksnapshot::Xml::PTimeDelta>: mpl::true_
{
	static bool validate(const Blocksnapshot::Xml::PTimeDelta::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PTimeZone

namespace Blocksnapshot
{
namespace Xml
{
struct PTimeZone
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Blocksnapshot

template<>
struct Validatable<Blocksnapshot::Xml::PTimeZone>: mpl::true_
{
	static bool validate(const Blocksnapshot::Xml::PTimeZone::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PFilterParamName

namespace Blocksnapshot
{
namespace Xml
{
struct PFilterParamName
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Blocksnapshot

template<>
struct Validatable<Blocksnapshot::Xml::PFilterParamName>: mpl::true_
{
	static bool validate(const Blocksnapshot::Xml::PFilterParamName::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PFilterParamValue

namespace Blocksnapshot
{
namespace Xml
{
struct PFilterParamValue
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Blocksnapshot

template<>
struct Validatable<Blocksnapshot::Xml::PFilterParamValue>: mpl::true_
{
	static bool validate(const Blocksnapshot::Xml::PFilterParamValue::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PSpaprvioReg

namespace Blocksnapshot
{
namespace Xml
{
struct PSpaprvioReg
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Blocksnapshot

template<>
struct Validatable<Blocksnapshot::Xml::PSpaprvioReg>: mpl::true_
{
	static bool validate(const Blocksnapshot::Xml::PSpaprvioReg::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PAliasName

namespace Blocksnapshot
{
namespace Xml
{
struct PAliasName
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Blocksnapshot

template<>
struct Validatable<Blocksnapshot::Xml::PAliasName>: mpl::true_
{
	static bool validate(const Blocksnapshot::Xml::PAliasName::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct VName

namespace Blocksnapshot
{
namespace Xml
{
typedef Choice<mpl::vector<PDiskTarget, PAbsFilePath > > VNameImpl;
typedef VNameImpl::value_type VName;

} // namespace Xml
} // namespace Blocksnapshot

template<>
struct Traits<Blocksnapshot::Xml::VName>
{
	static bool parse(const QString& src_, Blocksnapshot::Xml::VName& dst_);

	static QString generate(const Blocksnapshot::Xml::VName& src_);

};

} // namespace Libvirt

#endif // __BLOCKSNAPSHOT_DATA_H__
