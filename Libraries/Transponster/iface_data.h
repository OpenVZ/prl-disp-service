/*
 * Copyright (c) 2015-2017, Parallels International GmbH
 * Copyright (c) 2017-2021 Virtuozzo International GmbH. All rights reserved.
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
 * Our contact details: Virtuozzo International GmbH, Vordergasse 59, 8200
 * Schaffhausen, Switzerland.
 */

#ifndef __IFACE_DATA_H__
#define __IFACE_DATA_H__
#include <QString>
#include <QDateTime>
#include "base.h"
#include "patterns.h"
#include "iface_enum.h"

namespace Libvirt
{
///////////////////////////////////////////////////////////////////////////////
// struct PUnsignedInt

namespace Iface
{
namespace Xml
{
struct PUnsignedInt
{
	typedef quint32 value_type;
};

} // namespace Xml
} // namespace Iface

template<>
struct Traits<Iface::Xml::PUnsignedInt>
{
	static bool parse(const QString& src_, Iface::Xml::PUnsignedInt::value_type& dst_);

	static QString generate(Iface::Xml::PUnsignedInt::value_type src_);

};


///////////////////////////////////////////////////////////////////////////////
// struct PUnsignedLong

namespace Iface
{
namespace Xml
{
struct PUnsignedLong
{
	typedef ulong value_type;
};

} // namespace Xml
} // namespace Iface

template<>
struct Traits<Iface::Xml::PUnsignedLong>
{
	static bool parse(const QString& src_, Iface::Xml::PUnsignedLong::value_type& dst_);

	static QString generate(Iface::Xml::PUnsignedLong::value_type src_);

};


///////////////////////////////////////////////////////////////////////////////
// struct PHexuint

namespace Iface
{
namespace Xml
{
struct PHexuint
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Iface

template<>
struct Validatable<Iface::Xml::PHexuint>: mpl::true_
{
	static bool validate(const Iface::Xml::PHexuint::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PPositiveInteger

namespace Iface
{
namespace Xml
{
struct PPositiveInteger
{
	typedef quint32 value_type;
};

} // namespace Xml
} // namespace Iface

template<>
struct Traits<Iface::Xml::PPositiveInteger>
{
	static bool parse(const QString& src_, Iface::Xml::PPositiveInteger::value_type& dst_);

	static QString generate(Iface::Xml::PPositiveInteger::value_type src_);

};


///////////////////////////////////////////////////////////////////////////////
// struct POctalMode

namespace Iface
{
namespace Xml
{
struct POctalMode
{
	typedef quint32 value_type;
};

} // namespace Xml
} // namespace Iface

template<>
struct Traits<Iface::Xml::POctalMode>
{
	static bool parse(const QString& src_, Iface::Xml::POctalMode::value_type& dst_);

	static QString generate(Iface::Xml::POctalMode::value_type src_);

};


///////////////////////////////////////////////////////////////////////////////
// struct PData7137

namespace Iface
{
namespace Xml
{
struct PData7137
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Iface

template<>
struct Validatable<Iface::Xml::PData7137>: mpl::true_
{
	static bool validate(const Iface::Xml::PData7137::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData3976

namespace Iface
{
namespace Xml
{
struct PData3976
{
	typedef qint32 value_type;
};

} // namespace Xml
} // namespace Iface

template<>
struct Traits<Iface::Xml::PData3976>
{
	static bool parse(const QString& src_, Iface::Xml::PData3976::value_type& dst_);

	static QString generate(Iface::Xml::PData3976::value_type src_);

};

template<>
struct Validatable<Iface::Xml::PData3976>: mpl::true_
{
	static bool validate(Iface::Xml::PData3976::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData3978

namespace Iface
{
namespace Xml
{
struct PData3978
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Iface

template<>
struct Validatable<Iface::Xml::PData3978>: mpl::true_
{
	static bool validate(const Iface::Xml::PData3978::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData3980

namespace Iface
{
namespace Xml
{
struct PData3980
{
	typedef qint32 value_type;
};

} // namespace Xml
} // namespace Iface

template<>
struct Traits<Iface::Xml::PData3980>
{
	static bool parse(const QString& src_, Iface::Xml::PData3980::value_type& dst_);

	static QString generate(Iface::Xml::PData3980::value_type src_);

};

template<>
struct Validatable<Iface::Xml::PData3980>: mpl::true_
{
	static bool validate(Iface::Xml::PData3980::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData3982

namespace Iface
{
namespace Xml
{
struct PData3982
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Iface

template<>
struct Validatable<Iface::Xml::PData3982>: mpl::true_
{
	static bool validate(const Iface::Xml::PData3982::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData7139

namespace Iface
{
namespace Xml
{
struct PData7139
{
	typedef qint32 value_type;
};

} // namespace Xml
} // namespace Iface

template<>
struct Traits<Iface::Xml::PData7139>
{
	static bool parse(const QString& src_, Iface::Xml::PData7139::value_type& dst_);

	static QString generate(Iface::Xml::PData7139::value_type src_);

};

template<>
struct Validatable<Iface::Xml::PData7139>: mpl::true_
{
	static bool validate(Iface::Xml::PData7139::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData7141

namespace Iface
{
namespace Xml
{
struct PData7141
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Iface

template<>
struct Validatable<Iface::Xml::PData7141>: mpl::true_
{
	static bool validate(const Iface::Xml::PData7141::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData3984

namespace Iface
{
namespace Xml
{
struct PData3984
{
	typedef qint32 value_type;
};

} // namespace Xml
} // namespace Iface

template<>
struct Traits<Iface::Xml::PData3984>
{
	static bool parse(const QString& src_, Iface::Xml::PData3984::value_type& dst_);

	static QString generate(Iface::Xml::PData3984::value_type src_);

};

template<>
struct Validatable<Iface::Xml::PData3984>: mpl::true_
{
	static bool validate(Iface::Xml::PData3984::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData3986

namespace Iface
{
namespace Xml
{
struct PData3986
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Iface

template<>
struct Validatable<Iface::Xml::PData3986>: mpl::true_
{
	static bool validate(const Iface::Xml::PData3986::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData7143

namespace Iface
{
namespace Xml
{
struct PData7143
{
	typedef qint32 value_type;
};

} // namespace Xml
} // namespace Iface

template<>
struct Traits<Iface::Xml::PData7143>
{
	static bool parse(const QString& src_, Iface::Xml::PData7143::value_type& dst_);

	static QString generate(Iface::Xml::PData7143::value_type src_);

};

template<>
struct Validatable<Iface::Xml::PData7143>: mpl::true_
{
	static bool validate(Iface::Xml::PData7143::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData7145

namespace Iface
{
namespace Xml
{
struct PData7145
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Iface

template<>
struct Validatable<Iface::Xml::PData7145>: mpl::true_
{
	static bool validate(const Iface::Xml::PData7145::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData7147

namespace Iface
{
namespace Xml
{
struct PData7147
{
	typedef quint32 value_type;
};

} // namespace Xml
} // namespace Iface

template<>
struct Traits<Iface::Xml::PData7147>
{
	static bool parse(const QString& src_, Iface::Xml::PData7147::value_type& dst_);

	static QString generate(Iface::Xml::PData7147::value_type src_);

};


///////////////////////////////////////////////////////////////////////////////
// struct PData7149

namespace Iface
{
namespace Xml
{
struct PData7149
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Iface

template<>
struct Validatable<Iface::Xml::PData7149>: mpl::true_
{
	static bool validate(const Iface::Xml::PData7149::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData3988

namespace Iface
{
namespace Xml
{
struct PData3988
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Iface

template<>
struct Validatable<Iface::Xml::PData3988>: mpl::true_
{
	static bool validate(const Iface::Xml::PData3988::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PNumaDistanceValue

namespace Iface
{
namespace Xml
{
struct PNumaDistanceValue
{
	typedef quint32 value_type;
};

} // namespace Xml
} // namespace Iface

template<>
struct Traits<Iface::Xml::PNumaDistanceValue>
{
	static bool parse(const QString& src_, Iface::Xml::PNumaDistanceValue::value_type& dst_);

	static QString generate(Iface::Xml::PNumaDistanceValue::value_type src_);

};

template<>
struct Validatable<Iface::Xml::PNumaDistanceValue>: mpl::true_
{
	static bool validate(Iface::Xml::PNumaDistanceValue::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PUniMacAddr

namespace Iface
{
namespace Xml
{
struct PUniMacAddr
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Iface

template<>
struct Validatable<Iface::Xml::PUniMacAddr>: mpl::true_
{
	static bool validate(const Iface::Xml::PUniMacAddr::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PMultiMacAddr

namespace Iface
{
namespace Xml
{
struct PMultiMacAddr
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Iface

template<>
struct Validatable<Iface::Xml::PMultiMacAddr>: mpl::true_
{
	static bool validate(const Iface::Xml::PMultiMacAddr::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PMacAddr

namespace Iface
{
namespace Xml
{
struct PMacAddr
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Iface

template<>
struct Validatable<Iface::Xml::PMacAddr>: mpl::true_
{
	static bool validate(const Iface::Xml::PMacAddr::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PDuidLLT

namespace Iface
{
namespace Xml
{
struct PDuidLLT
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Iface

template<>
struct Validatable<Iface::Xml::PDuidLLT>: mpl::true_
{
	static bool validate(const Iface::Xml::PDuidLLT::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PDuidEN

namespace Iface
{
namespace Xml
{
struct PDuidEN
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Iface

template<>
struct Validatable<Iface::Xml::PDuidEN>: mpl::true_
{
	static bool validate(const Iface::Xml::PDuidEN::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PDuidLL

namespace Iface
{
namespace Xml
{
struct PDuidLL
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Iface

template<>
struct Validatable<Iface::Xml::PDuidLL>: mpl::true_
{
	static bool validate(const Iface::Xml::PDuidLL::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PDuidUUID

namespace Iface
{
namespace Xml
{
struct PDuidUUID
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Iface

template<>
struct Validatable<Iface::Xml::PDuidUUID>: mpl::true_
{
	static bool validate(const Iface::Xml::PDuidUUID::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PIpv4Addr

namespace Iface
{
namespace Xml
{
struct PIpv4Addr
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Iface

template<>
struct Validatable<Iface::Xml::PIpv4Addr>: mpl::true_
{
	static bool validate(const Iface::Xml::PIpv4Addr::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PIpv6Addr

namespace Iface
{
namespace Xml
{
struct PIpv6Addr
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Iface

template<>
struct Validatable<Iface::Xml::PIpv6Addr>: mpl::true_
{
	static bool validate(const Iface::Xml::PIpv6Addr::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PIpv4Prefix

namespace Iface
{
namespace Xml
{
struct PIpv4Prefix
{
	typedef quint32 value_type;
};

} // namespace Xml
} // namespace Iface

template<>
struct Traits<Iface::Xml::PIpv4Prefix>
{
	static bool parse(const QString& src_, Iface::Xml::PIpv4Prefix::value_type& dst_);

	static QString generate(Iface::Xml::PIpv4Prefix::value_type src_);

};

template<>
struct Validatable<Iface::Xml::PIpv4Prefix>: mpl::true_
{
	static bool validate(Iface::Xml::PIpv4Prefix::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PIpv6Prefix

namespace Iface
{
namespace Xml
{
struct PIpv6Prefix
{
	typedef quint32 value_type;
};

} // namespace Xml
} // namespace Iface

template<>
struct Traits<Iface::Xml::PIpv6Prefix>
{
	static bool parse(const QString& src_, Iface::Xml::PIpv6Prefix::value_type& dst_);

	static QString generate(Iface::Xml::PIpv6Prefix::value_type src_);

};

template<>
struct Validatable<Iface::Xml::PIpv6Prefix>: mpl::true_
{
	static bool validate(Iface::Xml::PIpv6Prefix::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PObjectNameWithSlash

namespace Iface
{
namespace Xml
{
struct PObjectNameWithSlash
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Iface

template<>
struct Validatable<Iface::Xml::PObjectNameWithSlash>: mpl::true_
{
	static bool validate(const Iface::Xml::PObjectNameWithSlash::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PObjectName

namespace Iface
{
namespace Xml
{
struct PObjectName
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Iface

template<>
struct Validatable<Iface::Xml::PObjectName>: mpl::true_
{
	static bool validate(const Iface::Xml::PObjectName::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PGenericName

namespace Iface
{
namespace Xml
{
struct PGenericName
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Iface

template<>
struct Validatable<Iface::Xml::PGenericName>: mpl::true_
{
	static bool validate(const Iface::Xml::PGenericName::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PDnsName

namespace Iface
{
namespace Xml
{
struct PDnsName
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Iface

template<>
struct Validatable<Iface::Xml::PDnsName>: mpl::true_
{
	static bool validate(const Iface::Xml::PDnsName::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PDeviceName

namespace Iface
{
namespace Xml
{
struct PDeviceName
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Iface

template<>
struct Validatable<Iface::Xml::PDeviceName>: mpl::true_
{
	static bool validate(const Iface::Xml::PDeviceName::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PZoneName

namespace Iface
{
namespace Xml
{
struct PZoneName
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Iface

template<>
struct Validatable<Iface::Xml::PZoneName>: mpl::true_
{
	static bool validate(const Iface::Xml::PZoneName::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PFilePath

namespace Iface
{
namespace Xml
{
struct PFilePath
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Iface

template<>
struct Validatable<Iface::Xml::PFilePath>: mpl::true_
{
	static bool validate(const Iface::Xml::PFilePath::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PDirPath

namespace Iface
{
namespace Xml
{
struct PDirPath
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Iface

template<>
struct Validatable<Iface::Xml::PDirPath>: mpl::true_
{
	static bool validate(const Iface::Xml::PDirPath::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PAbsFilePath

namespace Iface
{
namespace Xml
{
struct PAbsFilePath
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Iface

template<>
struct Validatable<Iface::Xml::PAbsFilePath>: mpl::true_
{
	static bool validate(const Iface::Xml::PAbsFilePath::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PVmwarePath

namespace Iface
{
namespace Xml
{
struct PVmwarePath
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Iface

template<>
struct Validatable<Iface::Xml::PVmwarePath>: mpl::true_
{
	static bool validate(const Iface::Xml::PVmwarePath::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PAbsDirPath

namespace Iface
{
namespace Xml
{
struct PAbsDirPath
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Iface

template<>
struct Validatable<Iface::Xml::PAbsDirPath>: mpl::true_
{
	static bool validate(const Iface::Xml::PAbsDirPath::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PUnit

namespace Iface
{
namespace Xml
{
struct PUnit
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Iface

template<>
struct Validatable<Iface::Xml::PUnit>: mpl::true_
{
	static bool validate(const Iface::Xml::PUnit::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PPciDomain

namespace Iface
{
namespace Xml
{
struct PPciDomain
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Iface

template<>
struct Validatable<Iface::Xml::PPciDomain>: mpl::true_
{
	static bool validate(const Iface::Xml::PPciDomain::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PPciBus

namespace Iface
{
namespace Xml
{
struct PPciBus
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Iface

template<>
struct Validatable<Iface::Xml::PPciBus>: mpl::true_
{
	static bool validate(const Iface::Xml::PPciBus::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PPciSlot

namespace Iface
{
namespace Xml
{
struct PPciSlot
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Iface

template<>
struct Validatable<Iface::Xml::PPciSlot>: mpl::true_
{
	static bool validate(const Iface::Xml::PPciSlot::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PPciFunc

namespace Iface
{
namespace Xml
{
struct PPciFunc
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Iface

template<>
struct Validatable<Iface::Xml::PPciFunc>: mpl::true_
{
	static bool validate(const Iface::Xml::PPciFunc::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PWwn

namespace Iface
{
namespace Xml
{
struct PWwn
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Iface

template<>
struct Validatable<Iface::Xml::PWwn>: mpl::true_
{
	static bool validate(const Iface::Xml::PWwn::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData3998

namespace Iface
{
namespace Xml
{
struct PData3998
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Iface

template<>
struct Validatable<Iface::Xml::PData3998>: mpl::true_
{
	static bool validate(const Iface::Xml::PData3998::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData4777

namespace Iface
{
namespace Xml
{
struct PData4777
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Iface

template<>
struct Validatable<Iface::Xml::PData4777>: mpl::true_
{
	static bool validate(const Iface::Xml::PData4777::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData4779

namespace Iface
{
namespace Xml
{
struct PData4779
{
	typedef qint32 value_type;
};

} // namespace Xml
} // namespace Iface

template<>
struct Traits<Iface::Xml::PData4779>
{
	static bool parse(const QString& src_, Iface::Xml::PData4779::value_type& dst_);

	static QString generate(Iface::Xml::PData4779::value_type src_);

};

template<>
struct Validatable<Iface::Xml::PData4779>: mpl::true_
{
	static bool validate(Iface::Xml::PData4779::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PCcwSsidRange

namespace Iface
{
namespace Xml
{
struct PCcwSsidRange
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Iface

template<>
struct Validatable<Iface::Xml::PCcwSsidRange>: mpl::true_
{
	static bool validate(const Iface::Xml::PCcwSsidRange::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData4781

namespace Iface
{
namespace Xml
{
struct PData4781
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Iface

template<>
struct Validatable<Iface::Xml::PData4781>: mpl::true_
{
	static bool validate(const Iface::Xml::PData4781::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData4783

namespace Iface
{
namespace Xml
{
struct PData4783
{
	typedef qint32 value_type;
};

} // namespace Xml
} // namespace Iface

template<>
struct Traits<Iface::Xml::PData4783>
{
	static bool parse(const QString& src_, Iface::Xml::PData4783::value_type& dst_);

	static QString generate(Iface::Xml::PData4783::value_type src_);

};

template<>
struct Validatable<Iface::Xml::PData4783>: mpl::true_
{
	static bool validate(Iface::Xml::PData4783::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PCpuset

namespace Iface
{
namespace Xml
{
struct PCpuset
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Iface

template<>
struct Validatable<Iface::Xml::PCpuset>: mpl::true_
{
	static bool validate(const Iface::Xml::PCpuset::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PVolName

namespace Iface
{
namespace Xml
{
struct PVolName
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Iface

template<>
struct Validatable<Iface::Xml::PVolName>: mpl::true_
{
	static bool validate(const Iface::Xml::PVolName::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PPortNumber

namespace Iface
{
namespace Xml
{
struct PPortNumber
{
	typedef qint32 value_type;
};

} // namespace Xml
} // namespace Iface

template<>
struct Traits<Iface::Xml::PPortNumber>
{
	static bool parse(const QString& src_, Iface::Xml::PPortNumber::value_type& dst_);

	static QString generate(Iface::Xml::PPortNumber::value_type src_);

};

template<>
struct Validatable<Iface::Xml::PPortNumber>: mpl::true_
{
	static bool validate(Iface::Xml::PPortNumber::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PIobase

namespace Iface
{
namespace Xml
{
struct PIobase
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Iface

template<>
struct Validatable<Iface::Xml::PIobase>: mpl::true_
{
	static bool validate(const Iface::Xml::PIobase::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PIrq

namespace Iface
{
namespace Xml
{
struct PIrq
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Iface

template<>
struct Validatable<Iface::Xml::PIrq>: mpl::true_
{
	static bool validate(const Iface::Xml::PIrq::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PTimeval

namespace Iface
{
namespace Xml
{
struct PTimeval
{
	typedef double value_type;
};

} // namespace Xml
} // namespace Iface

template<>
struct Traits<Iface::Xml::PTimeval>
{
	static bool parse(const QString& src_, Iface::Xml::PTimeval::value_type& dst_);

	static QString generate(Iface::Xml::PTimeval::value_type src_);

};

template<>
struct Validatable<Iface::Xml::PTimeval>: mpl::true_
{
	static bool validate(Iface::Xml::PTimeval::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PVlanId

namespace Iface
{
namespace Xml
{
struct PVlanId
{
	typedef quint32 value_type;
};

} // namespace Xml
} // namespace Iface

template<>
struct Traits<Iface::Xml::PVlanId>
{
	static bool parse(const QString& src_, Iface::Xml::PVlanId::value_type& dst_);

	static QString generate(Iface::Xml::PVlanId::value_type src_);

};

template<>
struct Validatable<Iface::Xml::PVlanId>: mpl::true_
{
	static bool validate(Iface::Xml::PVlanId::value_type value_);

};

} // namespace Libvirt

#endif // __IFACE_DATA_H__
