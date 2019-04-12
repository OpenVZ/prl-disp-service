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

#ifndef __NODEDEV_DATA_H__
#define __NODEDEV_DATA_H__
#include <QString>
#include <QDateTime>
#include "base.h"
#include "patterns.h"
#include "nodedev_enum.h"

namespace Libvirt
{
///////////////////////////////////////////////////////////////////////////////
// struct PUnsignedInt

namespace Nodedev
{
namespace Xml
{
struct PUnsignedInt
{
	typedef quint32 value_type;
};

} // namespace Xml
} // namespace Nodedev

template<>
struct Traits<Nodedev::Xml::PUnsignedInt>
{
	static bool parse(const QString& src_, Nodedev::Xml::PUnsignedInt::value_type& dst_);

	static QString generate(Nodedev::Xml::PUnsignedInt::value_type src_);

};


///////////////////////////////////////////////////////////////////////////////
// struct PUnsignedLong

namespace Nodedev
{
namespace Xml
{
struct PUnsignedLong
{
	typedef ulong value_type;
};

} // namespace Xml
} // namespace Nodedev

template<>
struct Traits<Nodedev::Xml::PUnsignedLong>
{
	static bool parse(const QString& src_, Nodedev::Xml::PUnsignedLong::value_type& dst_);

	static QString generate(Nodedev::Xml::PUnsignedLong::value_type src_);

};


///////////////////////////////////////////////////////////////////////////////
// struct PHexuint

namespace Nodedev
{
namespace Xml
{
struct PHexuint
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Nodedev

template<>
struct Validatable<Nodedev::Xml::PHexuint>: mpl::true_
{
	static bool validate(const Nodedev::Xml::PHexuint::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PPositiveInteger

namespace Nodedev
{
namespace Xml
{
struct PPositiveInteger
{
	typedef quint32 value_type;
};

} // namespace Xml
} // namespace Nodedev

template<>
struct Traits<Nodedev::Xml::PPositiveInteger>
{
	static bool parse(const QString& src_, Nodedev::Xml::PPositiveInteger::value_type& dst_);

	static QString generate(Nodedev::Xml::PPositiveInteger::value_type src_);

};


///////////////////////////////////////////////////////////////////////////////
// struct POctalMode

namespace Nodedev
{
namespace Xml
{
struct POctalMode
{
	typedef quint32 value_type;
};

} // namespace Xml
} // namespace Nodedev

template<>
struct Traits<Nodedev::Xml::POctalMode>
{
	static bool parse(const QString& src_, Nodedev::Xml::POctalMode::value_type& dst_);

	static QString generate(Nodedev::Xml::POctalMode::value_type src_);

};


///////////////////////////////////////////////////////////////////////////////
// struct PData3196

namespace Nodedev
{
namespace Xml
{
struct PData3196
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Nodedev

template<>
struct Validatable<Nodedev::Xml::PData3196>: mpl::true_
{
	static bool validate(const Nodedev::Xml::PData3196::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData3198

namespace Nodedev
{
namespace Xml
{
struct PData3198
{
	typedef qint32 value_type;
};

} // namespace Xml
} // namespace Nodedev

template<>
struct Traits<Nodedev::Xml::PData3198>
{
	static bool parse(const QString& src_, Nodedev::Xml::PData3198::value_type& dst_);

	static QString generate(Nodedev::Xml::PData3198::value_type src_);

};

template<>
struct Validatable<Nodedev::Xml::PData3198>: mpl::true_
{
	static bool validate(Nodedev::Xml::PData3198::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData3200

namespace Nodedev
{
namespace Xml
{
struct PData3200
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Nodedev

template<>
struct Validatable<Nodedev::Xml::PData3200>: mpl::true_
{
	static bool validate(const Nodedev::Xml::PData3200::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData3202

namespace Nodedev
{
namespace Xml
{
struct PData3202
{
	typedef qint32 value_type;
};

} // namespace Xml
} // namespace Nodedev

template<>
struct Traits<Nodedev::Xml::PData3202>
{
	static bool parse(const QString& src_, Nodedev::Xml::PData3202::value_type& dst_);

	static QString generate(Nodedev::Xml::PData3202::value_type src_);

};

template<>
struct Validatable<Nodedev::Xml::PData3202>: mpl::true_
{
	static bool validate(Nodedev::Xml::PData3202::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData3204

namespace Nodedev
{
namespace Xml
{
struct PData3204
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Nodedev

template<>
struct Validatable<Nodedev::Xml::PData3204>: mpl::true_
{
	static bool validate(const Nodedev::Xml::PData3204::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData3206

namespace Nodedev
{
namespace Xml
{
struct PData3206
{
	typedef qint32 value_type;
};

} // namespace Xml
} // namespace Nodedev

template<>
struct Traits<Nodedev::Xml::PData3206>
{
	static bool parse(const QString& src_, Nodedev::Xml::PData3206::value_type& dst_);

	static QString generate(Nodedev::Xml::PData3206::value_type src_);

};

template<>
struct Validatable<Nodedev::Xml::PData3206>: mpl::true_
{
	static bool validate(Nodedev::Xml::PData3206::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData3208

namespace Nodedev
{
namespace Xml
{
struct PData3208
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Nodedev

template<>
struct Validatable<Nodedev::Xml::PData3208>: mpl::true_
{
	static bool validate(const Nodedev::Xml::PData3208::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData3210

namespace Nodedev
{
namespace Xml
{
struct PData3210
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Nodedev

template<>
struct Validatable<Nodedev::Xml::PData3210>: mpl::true_
{
	static bool validate(const Nodedev::Xml::PData3210::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PUniMacAddr

namespace Nodedev
{
namespace Xml
{
struct PUniMacAddr
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Nodedev

template<>
struct Validatable<Nodedev::Xml::PUniMacAddr>: mpl::true_
{
	static bool validate(const Nodedev::Xml::PUniMacAddr::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PMultiMacAddr

namespace Nodedev
{
namespace Xml
{
struct PMultiMacAddr
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Nodedev

template<>
struct Validatable<Nodedev::Xml::PMultiMacAddr>: mpl::true_
{
	static bool validate(const Nodedev::Xml::PMultiMacAddr::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PMacAddr

namespace Nodedev
{
namespace Xml
{
struct PMacAddr
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Nodedev

template<>
struct Validatable<Nodedev::Xml::PMacAddr>: mpl::true_
{
	static bool validate(const Nodedev::Xml::PMacAddr::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PDuidLLT

namespace Nodedev
{
namespace Xml
{
struct PDuidLLT
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Nodedev

template<>
struct Validatable<Nodedev::Xml::PDuidLLT>: mpl::true_
{
	static bool validate(const Nodedev::Xml::PDuidLLT::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PDuidEN

namespace Nodedev
{
namespace Xml
{
struct PDuidEN
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Nodedev

template<>
struct Validatable<Nodedev::Xml::PDuidEN>: mpl::true_
{
	static bool validate(const Nodedev::Xml::PDuidEN::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PDuidLL

namespace Nodedev
{
namespace Xml
{
struct PDuidLL
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Nodedev

template<>
struct Validatable<Nodedev::Xml::PDuidLL>: mpl::true_
{
	static bool validate(const Nodedev::Xml::PDuidLL::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PDuidUUID

namespace Nodedev
{
namespace Xml
{
struct PDuidUUID
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Nodedev

template<>
struct Validatable<Nodedev::Xml::PDuidUUID>: mpl::true_
{
	static bool validate(const Nodedev::Xml::PDuidUUID::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PIpv4Addr

namespace Nodedev
{
namespace Xml
{
struct PIpv4Addr
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Nodedev

template<>
struct Validatable<Nodedev::Xml::PIpv4Addr>: mpl::true_
{
	static bool validate(const Nodedev::Xml::PIpv4Addr::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PIpv6Addr

namespace Nodedev
{
namespace Xml
{
struct PIpv6Addr
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Nodedev

template<>
struct Validatable<Nodedev::Xml::PIpv6Addr>: mpl::true_
{
	static bool validate(const Nodedev::Xml::PIpv6Addr::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PIpv4Prefix

namespace Nodedev
{
namespace Xml
{
struct PIpv4Prefix
{
	typedef quint32 value_type;
};

} // namespace Xml
} // namespace Nodedev

template<>
struct Traits<Nodedev::Xml::PIpv4Prefix>
{
	static bool parse(const QString& src_, Nodedev::Xml::PIpv4Prefix::value_type& dst_);

	static QString generate(Nodedev::Xml::PIpv4Prefix::value_type src_);

};

template<>
struct Validatable<Nodedev::Xml::PIpv4Prefix>: mpl::true_
{
	static bool validate(Nodedev::Xml::PIpv4Prefix::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PIpv6Prefix

namespace Nodedev
{
namespace Xml
{
struct PIpv6Prefix
{
	typedef quint32 value_type;
};

} // namespace Xml
} // namespace Nodedev

template<>
struct Traits<Nodedev::Xml::PIpv6Prefix>
{
	static bool parse(const QString& src_, Nodedev::Xml::PIpv6Prefix::value_type& dst_);

	static QString generate(Nodedev::Xml::PIpv6Prefix::value_type src_);

};

template<>
struct Validatable<Nodedev::Xml::PIpv6Prefix>: mpl::true_
{
	static bool validate(Nodedev::Xml::PIpv6Prefix::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PGenericName

namespace Nodedev
{
namespace Xml
{
struct PGenericName
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Nodedev

template<>
struct Validatable<Nodedev::Xml::PGenericName>: mpl::true_
{
	static bool validate(const Nodedev::Xml::PGenericName::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PDnsName

namespace Nodedev
{
namespace Xml
{
struct PDnsName
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Nodedev

template<>
struct Validatable<Nodedev::Xml::PDnsName>: mpl::true_
{
	static bool validate(const Nodedev::Xml::PDnsName::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PDeviceName

namespace Nodedev
{
namespace Xml
{
struct PDeviceName
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Nodedev

template<>
struct Validatable<Nodedev::Xml::PDeviceName>: mpl::true_
{
	static bool validate(const Nodedev::Xml::PDeviceName::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PFilePath

namespace Nodedev
{
namespace Xml
{
struct PFilePath
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Nodedev

template<>
struct Validatable<Nodedev::Xml::PFilePath>: mpl::true_
{
	static bool validate(const Nodedev::Xml::PFilePath::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PDirPath

namespace Nodedev
{
namespace Xml
{
struct PDirPath
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Nodedev

template<>
struct Validatable<Nodedev::Xml::PDirPath>: mpl::true_
{
	static bool validate(const Nodedev::Xml::PDirPath::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PAbsFilePath

namespace Nodedev
{
namespace Xml
{
struct PAbsFilePath
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Nodedev

template<>
struct Validatable<Nodedev::Xml::PAbsFilePath>: mpl::true_
{
	static bool validate(const Nodedev::Xml::PAbsFilePath::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PAbsDirPath

namespace Nodedev
{
namespace Xml
{
struct PAbsDirPath
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Nodedev

template<>
struct Validatable<Nodedev::Xml::PAbsDirPath>: mpl::true_
{
	static bool validate(const Nodedev::Xml::PAbsDirPath::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PUnit

namespace Nodedev
{
namespace Xml
{
struct PUnit
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Nodedev

template<>
struct Validatable<Nodedev::Xml::PUnit>: mpl::true_
{
	static bool validate(const Nodedev::Xml::PUnit::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PPciDomain

namespace Nodedev
{
namespace Xml
{
struct PPciDomain
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Nodedev

template<>
struct Validatable<Nodedev::Xml::PPciDomain>: mpl::true_
{
	static bool validate(const Nodedev::Xml::PPciDomain::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PPciBus

namespace Nodedev
{
namespace Xml
{
struct PPciBus
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Nodedev

template<>
struct Validatable<Nodedev::Xml::PPciBus>: mpl::true_
{
	static bool validate(const Nodedev::Xml::PPciBus::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PPciSlot

namespace Nodedev
{
namespace Xml
{
struct PPciSlot
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Nodedev

template<>
struct Validatable<Nodedev::Xml::PPciSlot>: mpl::true_
{
	static bool validate(const Nodedev::Xml::PPciSlot::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PPciFunc

namespace Nodedev
{
namespace Xml
{
struct PPciFunc
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Nodedev

template<>
struct Validatable<Nodedev::Xml::PPciFunc>: mpl::true_
{
	static bool validate(const Nodedev::Xml::PPciFunc::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PWwn

namespace Nodedev
{
namespace Xml
{
struct PWwn
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Nodedev

template<>
struct Validatable<Nodedev::Xml::PWwn>: mpl::true_
{
	static bool validate(const Nodedev::Xml::PWwn::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData3220

namespace Nodedev
{
namespace Xml
{
struct PData3220
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Nodedev

template<>
struct Validatable<Nodedev::Xml::PData3220>: mpl::true_
{
	static bool validate(const Nodedev::Xml::PData3220::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData3222

namespace Nodedev
{
namespace Xml
{
struct PData3222
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Nodedev

template<>
struct Validatable<Nodedev::Xml::PData3222>: mpl::true_
{
	static bool validate(const Nodedev::Xml::PData3222::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData3224

namespace Nodedev
{
namespace Xml
{
struct PData3224
{
	typedef qint32 value_type;
};

} // namespace Xml
} // namespace Nodedev

template<>
struct Traits<Nodedev::Xml::PData3224>
{
	static bool parse(const QString& src_, Nodedev::Xml::PData3224::value_type& dst_);

	static QString generate(Nodedev::Xml::PData3224::value_type src_);

};

template<>
struct Validatable<Nodedev::Xml::PData3224>: mpl::true_
{
	static bool validate(Nodedev::Xml::PData3224::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PCcwSsidRange

namespace Nodedev
{
namespace Xml
{
struct PCcwSsidRange
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Nodedev

template<>
struct Validatable<Nodedev::Xml::PCcwSsidRange>: mpl::true_
{
	static bool validate(const Nodedev::Xml::PCcwSsidRange::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData3226

namespace Nodedev
{
namespace Xml
{
struct PData3226
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Nodedev

template<>
struct Validatable<Nodedev::Xml::PData3226>: mpl::true_
{
	static bool validate(const Nodedev::Xml::PData3226::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData3228

namespace Nodedev
{
namespace Xml
{
struct PData3228
{
	typedef qint32 value_type;
};

} // namespace Xml
} // namespace Nodedev

template<>
struct Traits<Nodedev::Xml::PData3228>
{
	static bool parse(const QString& src_, Nodedev::Xml::PData3228::value_type& dst_);

	static QString generate(Nodedev::Xml::PData3228::value_type src_);

};

template<>
struct Validatable<Nodedev::Xml::PData3228>: mpl::true_
{
	static bool validate(Nodedev::Xml::PData3228::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PCpuset

namespace Nodedev
{
namespace Xml
{
struct PCpuset
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Nodedev

template<>
struct Validatable<Nodedev::Xml::PCpuset>: mpl::true_
{
	static bool validate(const Nodedev::Xml::PCpuset::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PVolName

namespace Nodedev
{
namespace Xml
{
struct PVolName
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Nodedev

template<>
struct Validatable<Nodedev::Xml::PVolName>: mpl::true_
{
	static bool validate(const Nodedev::Xml::PVolName::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PPortNumber

namespace Nodedev
{
namespace Xml
{
struct PPortNumber
{
	typedef qint32 value_type;
};

} // namespace Xml
} // namespace Nodedev

template<>
struct Traits<Nodedev::Xml::PPortNumber>
{
	static bool parse(const QString& src_, Nodedev::Xml::PPortNumber::value_type& dst_);

	static QString generate(Nodedev::Xml::PPortNumber::value_type src_);

};

template<>
struct Validatable<Nodedev::Xml::PPortNumber>: mpl::true_
{
	static bool validate(Nodedev::Xml::PPortNumber::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PIobase

namespace Nodedev
{
namespace Xml
{
struct PIobase
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Nodedev

template<>
struct Validatable<Nodedev::Xml::PIobase>: mpl::true_
{
	static bool validate(const Nodedev::Xml::PIobase::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PIrq

namespace Nodedev
{
namespace Xml
{
struct PIrq
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Nodedev

template<>
struct Validatable<Nodedev::Xml::PIrq>: mpl::true_
{
	static bool validate(const Nodedev::Xml::PIrq::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PClass

namespace Nodedev
{
namespace Xml
{
struct PClass
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Nodedev

template<>
struct Validatable<Nodedev::Xml::PClass>: mpl::true_
{
	static bool validate(const Nodedev::Xml::PClass::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PId

namespace Nodedev
{
namespace Xml
{
struct PId
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Nodedev


///////////////////////////////////////////////////////////////////////////////
// struct PNode

namespace Nodedev
{
namespace Xml
{
struct PNode
{
	typedef qint32 value_type;
};

} // namespace Xml
} // namespace Nodedev

template<>
struct Traits<Nodedev::Xml::PNode>
{
	static bool parse(const QString& src_, Nodedev::Xml::PNode::value_type& dst_);

	static QString generate(Nodedev::Xml::PNode::value_type src_);

};


///////////////////////////////////////////////////////////////////////////////
// struct PSpeed

namespace Nodedev
{
namespace Xml
{
struct PSpeed
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Nodedev

template<>
struct Validatable<Nodedev::Xml::PSpeed>: mpl::true_
{
	static bool validate(const Nodedev::Xml::PSpeed::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PNetfeaturename

namespace Nodedev
{
namespace Xml
{
struct PNetfeaturename
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Nodedev

template<>
struct Validatable<Nodedev::Xml::PNetfeaturename>: mpl::true_
{
	static bool validate(const Nodedev::Xml::PNetfeaturename::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PMac

namespace Nodedev
{
namespace Xml
{
struct PMac
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Nodedev

template<>
struct Validatable<Nodedev::Xml::PMac>: mpl::true_
{
	static bool validate(const Nodedev::Xml::PMac::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PPath

namespace Nodedev
{
namespace Xml
{
struct PPath
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Nodedev

template<>
struct Validatable<Nodedev::Xml::PPath>: mpl::true_
{
	static bool validate(const Nodedev::Xml::PPath::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct VUUID

namespace Nodedev
{
namespace Xml
{
typedef Choice<mpl::vector<PData3208, PData3210 > > VUUIDImpl;
typedef VUUIDImpl::value_type VUUID;

} // namespace Xml
} // namespace Nodedev

template<>
struct Traits<Nodedev::Xml::VUUID>
{
	static bool parse(const QString& src_, Nodedev::Xml::VUUID& dst_);

	static QString generate(const Nodedev::Xml::VUUID& src_);

};

///////////////////////////////////////////////////////////////////////////////
// struct VCcwCssidRange

namespace Nodedev
{
namespace Xml
{
typedef Choice<mpl::vector<PData3220, PData3222, PData3224 > > VCcwCssidRangeImpl;
typedef VCcwCssidRangeImpl::value_type VCcwCssidRange;

} // namespace Xml
} // namespace Nodedev

template<>
struct Traits<Nodedev::Xml::VCcwCssidRange>
{
	static bool parse(const QString& src_, Nodedev::Xml::VCcwCssidRange& dst_);

	static QString generate(const Nodedev::Xml::VCcwCssidRange& src_);

};

///////////////////////////////////////////////////////////////////////////////
// struct VCcwDevnoRange

namespace Nodedev
{
namespace Xml
{
typedef Choice<mpl::vector<PData3226, PData3228 > > VCcwDevnoRangeImpl;
typedef VCcwDevnoRangeImpl::value_type VCcwDevnoRange;

} // namespace Xml
} // namespace Nodedev

template<>
struct Traits<Nodedev::Xml::VCcwDevnoRange>
{
	static bool parse(const QString& src_, Nodedev::Xml::VCcwDevnoRange& dst_);

	static QString generate(const Nodedev::Xml::VCcwDevnoRange& src_);

};

} // namespace Libvirt

#endif // __NODEDEV_DATA_H__
