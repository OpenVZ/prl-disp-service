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
// struct PData4066

namespace Nodedev
{
namespace Xml
{
struct PData4066
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Nodedev

template<>
struct Validatable<Nodedev::Xml::PData4066>: mpl::true_
{
	static bool validate(const Nodedev::Xml::PData4066::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData5141

namespace Nodedev
{
namespace Xml
{
struct PData5141
{
	typedef qint32 value_type;
};

} // namespace Xml
} // namespace Nodedev

template<>
struct Traits<Nodedev::Xml::PData5141>
{
	static bool parse(const QString& src_, Nodedev::Xml::PData5141::value_type& dst_);

	static QString generate(Nodedev::Xml::PData5141::value_type src_);

};

template<>
struct Validatable<Nodedev::Xml::PData5141>: mpl::true_
{
	static bool validate(Nodedev::Xml::PData5141::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData5143

namespace Nodedev
{
namespace Xml
{
struct PData5143
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Nodedev

template<>
struct Validatable<Nodedev::Xml::PData5143>: mpl::true_
{
	static bool validate(const Nodedev::Xml::PData5143::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData5145

namespace Nodedev
{
namespace Xml
{
struct PData5145
{
	typedef qint32 value_type;
};

} // namespace Xml
} // namespace Nodedev

template<>
struct Traits<Nodedev::Xml::PData5145>
{
	static bool parse(const QString& src_, Nodedev::Xml::PData5145::value_type& dst_);

	static QString generate(Nodedev::Xml::PData5145::value_type src_);

};

template<>
struct Validatable<Nodedev::Xml::PData5145>: mpl::true_
{
	static bool validate(Nodedev::Xml::PData5145::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData5147

namespace Nodedev
{
namespace Xml
{
struct PData5147
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Nodedev

template<>
struct Validatable<Nodedev::Xml::PData5147>: mpl::true_
{
	static bool validate(const Nodedev::Xml::PData5147::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData9806

namespace Nodedev
{
namespace Xml
{
struct PData9806
{
	typedef qint32 value_type;
};

} // namespace Xml
} // namespace Nodedev

template<>
struct Traits<Nodedev::Xml::PData9806>
{
	static bool parse(const QString& src_, Nodedev::Xml::PData9806::value_type& dst_);

	static QString generate(Nodedev::Xml::PData9806::value_type src_);

};

template<>
struct Validatable<Nodedev::Xml::PData9806>: mpl::true_
{
	static bool validate(Nodedev::Xml::PData9806::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData9875

namespace Nodedev
{
namespace Xml
{
struct PData9875
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Nodedev

template<>
struct Validatable<Nodedev::Xml::PData9875>: mpl::true_
{
	static bool validate(const Nodedev::Xml::PData9875::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData5149

namespace Nodedev
{
namespace Xml
{
struct PData5149
{
	typedef qint32 value_type;
};

} // namespace Xml
} // namespace Nodedev

template<>
struct Traits<Nodedev::Xml::PData5149>
{
	static bool parse(const QString& src_, Nodedev::Xml::PData5149::value_type& dst_);

	static QString generate(Nodedev::Xml::PData5149::value_type src_);

};

template<>
struct Validatable<Nodedev::Xml::PData5149>: mpl::true_
{
	static bool validate(Nodedev::Xml::PData5149::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData5151

namespace Nodedev
{
namespace Xml
{
struct PData5151
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Nodedev

template<>
struct Validatable<Nodedev::Xml::PData5151>: mpl::true_
{
	static bool validate(const Nodedev::Xml::PData5151::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData9877

namespace Nodedev
{
namespace Xml
{
struct PData9877
{
	typedef qint32 value_type;
};

} // namespace Xml
} // namespace Nodedev

template<>
struct Traits<Nodedev::Xml::PData9877>
{
	static bool parse(const QString& src_, Nodedev::Xml::PData9877::value_type& dst_);

	static QString generate(Nodedev::Xml::PData9877::value_type src_);

};

template<>
struct Validatable<Nodedev::Xml::PData9877>: mpl::true_
{
	static bool validate(Nodedev::Xml::PData9877::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData9879

namespace Nodedev
{
namespace Xml
{
struct PData9879
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Nodedev

template<>
struct Validatable<Nodedev::Xml::PData9879>: mpl::true_
{
	static bool validate(const Nodedev::Xml::PData9879::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData9881

namespace Nodedev
{
namespace Xml
{
struct PData9881
{
	typedef quint32 value_type;
};

} // namespace Xml
} // namespace Nodedev

template<>
struct Traits<Nodedev::Xml::PData9881>
{
	static bool parse(const QString& src_, Nodedev::Xml::PData9881::value_type& dst_);

	static QString generate(Nodedev::Xml::PData9881::value_type src_);

};


///////////////////////////////////////////////////////////////////////////////
// struct PData9883

namespace Nodedev
{
namespace Xml
{
struct PData9883
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Nodedev

template<>
struct Validatable<Nodedev::Xml::PData9883>: mpl::true_
{
	static bool validate(const Nodedev::Xml::PData9883::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData5153

namespace Nodedev
{
namespace Xml
{
struct PData5153
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Nodedev

template<>
struct Validatable<Nodedev::Xml::PData5153>: mpl::true_
{
	static bool validate(const Nodedev::Xml::PData5153::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PNumaDistanceValue

namespace Nodedev
{
namespace Xml
{
struct PNumaDistanceValue
{
	typedef quint32 value_type;
};

} // namespace Xml
} // namespace Nodedev

template<>
struct Traits<Nodedev::Xml::PNumaDistanceValue>
{
	static bool parse(const QString& src_, Nodedev::Xml::PNumaDistanceValue::value_type& dst_);

	static QString generate(Nodedev::Xml::PNumaDistanceValue::value_type src_);

};

template<>
struct Validatable<Nodedev::Xml::PNumaDistanceValue>: mpl::true_
{
	static bool validate(Nodedev::Xml::PNumaDistanceValue::value_type value_);

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
// struct PObjectNameWithSlash

namespace Nodedev
{
namespace Xml
{
struct PObjectNameWithSlash
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Nodedev

template<>
struct Validatable<Nodedev::Xml::PObjectNameWithSlash>: mpl::true_
{
	static bool validate(const Nodedev::Xml::PObjectNameWithSlash::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PObjectName

namespace Nodedev
{
namespace Xml
{
struct PObjectName
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Nodedev

template<>
struct Validatable<Nodedev::Xml::PObjectName>: mpl::true_
{
	static bool validate(const Nodedev::Xml::PObjectName::value_type& value_);

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
// struct PZoneName

namespace Nodedev
{
namespace Xml
{
struct PZoneName
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Nodedev

template<>
struct Validatable<Nodedev::Xml::PZoneName>: mpl::true_
{
	static bool validate(const Nodedev::Xml::PZoneName::value_type& value_);

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
// struct PVmwarePath

namespace Nodedev
{
namespace Xml
{
struct PVmwarePath
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Nodedev

template<>
struct Validatable<Nodedev::Xml::PVmwarePath>: mpl::true_
{
	static bool validate(const Nodedev::Xml::PVmwarePath::value_type& value_);

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
// struct PData5163

namespace Nodedev
{
namespace Xml
{
struct PData5163
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Nodedev

template<>
struct Validatable<Nodedev::Xml::PData5163>: mpl::true_
{
	static bool validate(const Nodedev::Xml::PData5163::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData4068

namespace Nodedev
{
namespace Xml
{
struct PData4068
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Nodedev

template<>
struct Validatable<Nodedev::Xml::PData4068>: mpl::true_
{
	static bool validate(const Nodedev::Xml::PData4068::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData4070

namespace Nodedev
{
namespace Xml
{
struct PData4070
{
	typedef qint32 value_type;
};

} // namespace Xml
} // namespace Nodedev

template<>
struct Traits<Nodedev::Xml::PData4070>
{
	static bool parse(const QString& src_, Nodedev::Xml::PData4070::value_type& dst_);

	static QString generate(Nodedev::Xml::PData4070::value_type src_);

};

template<>
struct Validatable<Nodedev::Xml::PData4070>: mpl::true_
{
	static bool validate(Nodedev::Xml::PData4070::value_type value_);

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
// struct PData4072

namespace Nodedev
{
namespace Xml
{
struct PData4072
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Nodedev

template<>
struct Validatable<Nodedev::Xml::PData4072>: mpl::true_
{
	static bool validate(const Nodedev::Xml::PData4072::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData4074

namespace Nodedev
{
namespace Xml
{
struct PData4074
{
	typedef qint32 value_type;
};

} // namespace Xml
} // namespace Nodedev

template<>
struct Traits<Nodedev::Xml::PData4074>
{
	static bool parse(const QString& src_, Nodedev::Xml::PData4074::value_type& dst_);

	static QString generate(Nodedev::Xml::PData4074::value_type src_);

};

template<>
struct Validatable<Nodedev::Xml::PData4074>: mpl::true_
{
	static bool validate(Nodedev::Xml::PData4074::value_type value_);

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
// struct PData9837

namespace Nodedev
{
namespace Xml
{
struct PData9837
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Nodedev

template<>
struct Validatable<Nodedev::Xml::PData9837>: mpl::true_
{
	static bool validate(const Nodedev::Xml::PData9837::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData9839

namespace Nodedev
{
namespace Xml
{
struct PData9839
{
	typedef qint32 value_type;
};

} // namespace Xml
} // namespace Nodedev

template<>
struct Traits<Nodedev::Xml::PData9839>
{
	static bool parse(const QString& src_, Nodedev::Xml::PData9839::value_type& dst_);

	static QString generate(Nodedev::Xml::PData9839::value_type src_);

};

template<>
struct Validatable<Nodedev::Xml::PData9839>: mpl::true_
{
	static bool validate(Nodedev::Xml::PData9839::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PVpdFieldValueFormat

namespace Nodedev
{
namespace Xml
{
struct PVpdFieldValueFormat
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Nodedev

template<>
struct Validatable<Nodedev::Xml::PVpdFieldValueFormat>: mpl::true_
{
	static bool validate(const Nodedev::Xml::PVpdFieldValueFormat::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PVendorVPDFieldIndex

namespace Nodedev
{
namespace Xml
{
struct PVendorVPDFieldIndex
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Nodedev

template<>
struct Validatable<Nodedev::Xml::PVendorVPDFieldIndex>: mpl::true_
{
	static bool validate(const Nodedev::Xml::PVendorVPDFieldIndex::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PSystemVPDFieldIndex

namespace Nodedev
{
namespace Xml
{
struct PSystemVPDFieldIndex
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Nodedev

template<>
struct Validatable<Nodedev::Xml::PSystemVPDFieldIndex>: mpl::true_
{
	static bool validate(const Nodedev::Xml::PSystemVPDFieldIndex::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct VUUID

namespace Nodedev
{
namespace Xml
{
typedef Choice<mpl::vector<PData9883, PData5153 > > VUUIDImpl;
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
typedef Choice<mpl::vector<PData5163, PData4068, PData4070 > > VCcwCssidRangeImpl;
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
typedef Choice<mpl::vector<PData4072, PData4074 > > VCcwDevnoRangeImpl;
typedef VCcwDevnoRangeImpl::value_type VCcwDevnoRange;

} // namespace Xml
} // namespace Nodedev

template<>
struct Traits<Nodedev::Xml::VCcwDevnoRange>
{
	static bool parse(const QString& src_, Nodedev::Xml::VCcwDevnoRange& dst_);

	static QString generate(const Nodedev::Xml::VCcwDevnoRange& src_);

};

///////////////////////////////////////////////////////////////////////////////
// struct VUint8

namespace Nodedev
{
namespace Xml
{
typedef Choice<mpl::vector<PData4066, PData5141 > > VUint8Impl;
typedef VUint8Impl::value_type VUint8;

} // namespace Xml
} // namespace Nodedev

template<>
struct Traits<Nodedev::Xml::VUint8>
{
	static bool parse(const QString& src_, Nodedev::Xml::VUint8& dst_);

	static QString generate(const Nodedev::Xml::VUint8& src_);

};

///////////////////////////////////////////////////////////////////////////////
// struct VApDomainRange

namespace Nodedev
{
namespace Xml
{
typedef Choice<mpl::vector<PData9837, PData9839 > > VApDomainRangeImpl;
typedef VApDomainRangeImpl::value_type VApDomainRange;

} // namespace Xml
} // namespace Nodedev

template<>
struct Traits<Nodedev::Xml::VApDomainRange>
{
	static bool parse(const QString& src_, Nodedev::Xml::VApDomainRange& dst_);

	static QString generate(const Nodedev::Xml::VApDomainRange& src_);

};

} // namespace Libvirt

#endif // __NODEDEV_DATA_H__
