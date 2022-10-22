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

#ifndef __CAPABILITY_DATA_H__
#define __CAPABILITY_DATA_H__
#include <QString>
#include <QDateTime>
#include "base.h"
#include "patterns.h"
#include "capability_enum.h"

namespace Libvirt
{
///////////////////////////////////////////////////////////////////////////////
// struct PUnsignedInt

namespace Capability
{
namespace Xml
{
struct PUnsignedInt
{
	typedef quint32 value_type;
};

} // namespace Xml
} // namespace Capability

template<>
struct Traits<Capability::Xml::PUnsignedInt>
{
	static bool parse(const QString& src_, Capability::Xml::PUnsignedInt::value_type& dst_);

	static QString generate(Capability::Xml::PUnsignedInt::value_type src_);

};


///////////////////////////////////////////////////////////////////////////////
// struct PUnsignedLong

namespace Capability
{
namespace Xml
{
struct PUnsignedLong
{
	typedef ulong value_type;
};

} // namespace Xml
} // namespace Capability

template<>
struct Traits<Capability::Xml::PUnsignedLong>
{
	static bool parse(const QString& src_, Capability::Xml::PUnsignedLong::value_type& dst_);

	static QString generate(Capability::Xml::PUnsignedLong::value_type src_);

};


///////////////////////////////////////////////////////////////////////////////
// struct PHexuint

namespace Capability
{
namespace Xml
{
struct PHexuint
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Capability

template<>
struct Validatable<Capability::Xml::PHexuint>: mpl::true_
{
	static bool validate(const Capability::Xml::PHexuint::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PPositiveInteger

namespace Capability
{
namespace Xml
{
struct PPositiveInteger
{
	typedef quint32 value_type;
};

} // namespace Xml
} // namespace Capability

template<>
struct Traits<Capability::Xml::PPositiveInteger>
{
	static bool parse(const QString& src_, Capability::Xml::PPositiveInteger::value_type& dst_);

	static QString generate(Capability::Xml::PPositiveInteger::value_type src_);

};


///////////////////////////////////////////////////////////////////////////////
// struct POctalMode

namespace Capability
{
namespace Xml
{
struct POctalMode
{
	typedef quint32 value_type;
};

} // namespace Xml
} // namespace Capability

template<>
struct Traits<Capability::Xml::POctalMode>
{
	static bool parse(const QString& src_, Capability::Xml::POctalMode::value_type& dst_);

	static QString generate(Capability::Xml::POctalMode::value_type src_);

};


///////////////////////////////////////////////////////////////////////////////
// struct PData4806

namespace Capability
{
namespace Xml
{
struct PData4806
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Capability

template<>
struct Validatable<Capability::Xml::PData4806>: mpl::true_
{
	static bool validate(const Capability::Xml::PData4806::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData4024

namespace Capability
{
namespace Xml
{
struct PData4024
{
	typedef qint32 value_type;
};

} // namespace Xml
} // namespace Capability

template<>
struct Traits<Capability::Xml::PData4024>
{
	static bool parse(const QString& src_, Capability::Xml::PData4024::value_type& dst_);

	static QString generate(Capability::Xml::PData4024::value_type src_);

};

template<>
struct Validatable<Capability::Xml::PData4024>: mpl::true_
{
	static bool validate(Capability::Xml::PData4024::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData4026

namespace Capability
{
namespace Xml
{
struct PData4026
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Capability

template<>
struct Validatable<Capability::Xml::PData4026>: mpl::true_
{
	static bool validate(const Capability::Xml::PData4026::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData4028

namespace Capability
{
namespace Xml
{
struct PData4028
{
	typedef qint32 value_type;
};

} // namespace Xml
} // namespace Capability

template<>
struct Traits<Capability::Xml::PData4028>
{
	static bool parse(const QString& src_, Capability::Xml::PData4028::value_type& dst_);

	static QString generate(Capability::Xml::PData4028::value_type src_);

};

template<>
struct Validatable<Capability::Xml::PData4028>: mpl::true_
{
	static bool validate(Capability::Xml::PData4028::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData4030

namespace Capability
{
namespace Xml
{
struct PData4030
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Capability

template<>
struct Validatable<Capability::Xml::PData4030>: mpl::true_
{
	static bool validate(const Capability::Xml::PData4030::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData8005

namespace Capability
{
namespace Xml
{
struct PData8005
{
	typedef qint32 value_type;
};

} // namespace Xml
} // namespace Capability

template<>
struct Traits<Capability::Xml::PData8005>
{
	static bool parse(const QString& src_, Capability::Xml::PData8005::value_type& dst_);

	static QString generate(Capability::Xml::PData8005::value_type src_);

};

template<>
struct Validatable<Capability::Xml::PData8005>: mpl::true_
{
	static bool validate(Capability::Xml::PData8005::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData8734

namespace Capability
{
namespace Xml
{
struct PData8734
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Capability

template<>
struct Validatable<Capability::Xml::PData8734>: mpl::true_
{
	static bool validate(const Capability::Xml::PData8734::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData4032

namespace Capability
{
namespace Xml
{
struct PData4032
{
	typedef qint32 value_type;
};

} // namespace Xml
} // namespace Capability

template<>
struct Traits<Capability::Xml::PData4032>
{
	static bool parse(const QString& src_, Capability::Xml::PData4032::value_type& dst_);

	static QString generate(Capability::Xml::PData4032::value_type src_);

};

template<>
struct Validatable<Capability::Xml::PData4032>: mpl::true_
{
	static bool validate(Capability::Xml::PData4032::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData4034

namespace Capability
{
namespace Xml
{
struct PData4034
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Capability

template<>
struct Validatable<Capability::Xml::PData4034>: mpl::true_
{
	static bool validate(const Capability::Xml::PData4034::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData8736

namespace Capability
{
namespace Xml
{
struct PData8736
{
	typedef qint32 value_type;
};

} // namespace Xml
} // namespace Capability

template<>
struct Traits<Capability::Xml::PData8736>
{
	static bool parse(const QString& src_, Capability::Xml::PData8736::value_type& dst_);

	static QString generate(Capability::Xml::PData8736::value_type src_);

};

template<>
struct Validatable<Capability::Xml::PData8736>: mpl::true_
{
	static bool validate(Capability::Xml::PData8736::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData8738

namespace Capability
{
namespace Xml
{
struct PData8738
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Capability

template<>
struct Validatable<Capability::Xml::PData8738>: mpl::true_
{
	static bool validate(const Capability::Xml::PData8738::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData8740

namespace Capability
{
namespace Xml
{
struct PData8740
{
	typedef quint32 value_type;
};

} // namespace Xml
} // namespace Capability

template<>
struct Traits<Capability::Xml::PData8740>
{
	static bool parse(const QString& src_, Capability::Xml::PData8740::value_type& dst_);

	static QString generate(Capability::Xml::PData8740::value_type src_);

};


///////////////////////////////////////////////////////////////////////////////
// struct PData8742

namespace Capability
{
namespace Xml
{
struct PData8742
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Capability

template<>
struct Validatable<Capability::Xml::PData8742>: mpl::true_
{
	static bool validate(const Capability::Xml::PData8742::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData4036

namespace Capability
{
namespace Xml
{
struct PData4036
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Capability

template<>
struct Validatable<Capability::Xml::PData4036>: mpl::true_
{
	static bool validate(const Capability::Xml::PData4036::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PNumaDistanceValue

namespace Capability
{
namespace Xml
{
struct PNumaDistanceValue
{
	typedef quint32 value_type;
};

} // namespace Xml
} // namespace Capability

template<>
struct Traits<Capability::Xml::PNumaDistanceValue>
{
	static bool parse(const QString& src_, Capability::Xml::PNumaDistanceValue::value_type& dst_);

	static QString generate(Capability::Xml::PNumaDistanceValue::value_type src_);

};

template<>
struct Validatable<Capability::Xml::PNumaDistanceValue>: mpl::true_
{
	static bool validate(Capability::Xml::PNumaDistanceValue::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PUniMacAddr

namespace Capability
{
namespace Xml
{
struct PUniMacAddr
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Capability

template<>
struct Validatable<Capability::Xml::PUniMacAddr>: mpl::true_
{
	static bool validate(const Capability::Xml::PUniMacAddr::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PMultiMacAddr

namespace Capability
{
namespace Xml
{
struct PMultiMacAddr
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Capability

template<>
struct Validatable<Capability::Xml::PMultiMacAddr>: mpl::true_
{
	static bool validate(const Capability::Xml::PMultiMacAddr::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PMacAddr

namespace Capability
{
namespace Xml
{
struct PMacAddr
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Capability

template<>
struct Validatable<Capability::Xml::PMacAddr>: mpl::true_
{
	static bool validate(const Capability::Xml::PMacAddr::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PDuidLLT

namespace Capability
{
namespace Xml
{
struct PDuidLLT
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Capability

template<>
struct Validatable<Capability::Xml::PDuidLLT>: mpl::true_
{
	static bool validate(const Capability::Xml::PDuidLLT::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PDuidEN

namespace Capability
{
namespace Xml
{
struct PDuidEN
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Capability

template<>
struct Validatable<Capability::Xml::PDuidEN>: mpl::true_
{
	static bool validate(const Capability::Xml::PDuidEN::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PDuidLL

namespace Capability
{
namespace Xml
{
struct PDuidLL
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Capability

template<>
struct Validatable<Capability::Xml::PDuidLL>: mpl::true_
{
	static bool validate(const Capability::Xml::PDuidLL::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PDuidUUID

namespace Capability
{
namespace Xml
{
struct PDuidUUID
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Capability

template<>
struct Validatable<Capability::Xml::PDuidUUID>: mpl::true_
{
	static bool validate(const Capability::Xml::PDuidUUID::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PIpv4Addr

namespace Capability
{
namespace Xml
{
struct PIpv4Addr
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Capability

template<>
struct Validatable<Capability::Xml::PIpv4Addr>: mpl::true_
{
	static bool validate(const Capability::Xml::PIpv4Addr::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PIpv6Addr

namespace Capability
{
namespace Xml
{
struct PIpv6Addr
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Capability

template<>
struct Validatable<Capability::Xml::PIpv6Addr>: mpl::true_
{
	static bool validate(const Capability::Xml::PIpv6Addr::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PIpv4Prefix

namespace Capability
{
namespace Xml
{
struct PIpv4Prefix
{
	typedef quint32 value_type;
};

} // namespace Xml
} // namespace Capability

template<>
struct Traits<Capability::Xml::PIpv4Prefix>
{
	static bool parse(const QString& src_, Capability::Xml::PIpv4Prefix::value_type& dst_);

	static QString generate(Capability::Xml::PIpv4Prefix::value_type src_);

};

template<>
struct Validatable<Capability::Xml::PIpv4Prefix>: mpl::true_
{
	static bool validate(Capability::Xml::PIpv4Prefix::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PIpv6Prefix

namespace Capability
{
namespace Xml
{
struct PIpv6Prefix
{
	typedef quint32 value_type;
};

} // namespace Xml
} // namespace Capability

template<>
struct Traits<Capability::Xml::PIpv6Prefix>
{
	static bool parse(const QString& src_, Capability::Xml::PIpv6Prefix::value_type& dst_);

	static QString generate(Capability::Xml::PIpv6Prefix::value_type src_);

};

template<>
struct Validatable<Capability::Xml::PIpv6Prefix>: mpl::true_
{
	static bool validate(Capability::Xml::PIpv6Prefix::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PObjectNameWithSlash

namespace Capability
{
namespace Xml
{
struct PObjectNameWithSlash
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Capability

template<>
struct Validatable<Capability::Xml::PObjectNameWithSlash>: mpl::true_
{
	static bool validate(const Capability::Xml::PObjectNameWithSlash::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PObjectName

namespace Capability
{
namespace Xml
{
struct PObjectName
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Capability

template<>
struct Validatable<Capability::Xml::PObjectName>: mpl::true_
{
	static bool validate(const Capability::Xml::PObjectName::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PGenericName

namespace Capability
{
namespace Xml
{
struct PGenericName
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Capability

template<>
struct Validatable<Capability::Xml::PGenericName>: mpl::true_
{
	static bool validate(const Capability::Xml::PGenericName::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PDnsName

namespace Capability
{
namespace Xml
{
struct PDnsName
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Capability

template<>
struct Validatable<Capability::Xml::PDnsName>: mpl::true_
{
	static bool validate(const Capability::Xml::PDnsName::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PDeviceName

namespace Capability
{
namespace Xml
{
struct PDeviceName
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Capability

template<>
struct Validatable<Capability::Xml::PDeviceName>: mpl::true_
{
	static bool validate(const Capability::Xml::PDeviceName::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PZoneName

namespace Capability
{
namespace Xml
{
struct PZoneName
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Capability

template<>
struct Validatable<Capability::Xml::PZoneName>: mpl::true_
{
	static bool validate(const Capability::Xml::PZoneName::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PFilePath

namespace Capability
{
namespace Xml
{
struct PFilePath
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Capability

template<>
struct Validatable<Capability::Xml::PFilePath>: mpl::true_
{
	static bool validate(const Capability::Xml::PFilePath::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PDirPath

namespace Capability
{
namespace Xml
{
struct PDirPath
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Capability

template<>
struct Validatable<Capability::Xml::PDirPath>: mpl::true_
{
	static bool validate(const Capability::Xml::PDirPath::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PAbsFilePath

namespace Capability
{
namespace Xml
{
struct PAbsFilePath
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Capability

template<>
struct Validatable<Capability::Xml::PAbsFilePath>: mpl::true_
{
	static bool validate(const Capability::Xml::PAbsFilePath::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PVmwarePath

namespace Capability
{
namespace Xml
{
struct PVmwarePath
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Capability

template<>
struct Validatable<Capability::Xml::PVmwarePath>: mpl::true_
{
	static bool validate(const Capability::Xml::PVmwarePath::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PAbsDirPath

namespace Capability
{
namespace Xml
{
struct PAbsDirPath
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Capability

template<>
struct Validatable<Capability::Xml::PAbsDirPath>: mpl::true_
{
	static bool validate(const Capability::Xml::PAbsDirPath::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PUnit

namespace Capability
{
namespace Xml
{
struct PUnit
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Capability

template<>
struct Validatable<Capability::Xml::PUnit>: mpl::true_
{
	static bool validate(const Capability::Xml::PUnit::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PPciDomain

namespace Capability
{
namespace Xml
{
struct PPciDomain
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Capability

template<>
struct Validatable<Capability::Xml::PPciDomain>: mpl::true_
{
	static bool validate(const Capability::Xml::PPciDomain::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PPciBus

namespace Capability
{
namespace Xml
{
struct PPciBus
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Capability

template<>
struct Validatable<Capability::Xml::PPciBus>: mpl::true_
{
	static bool validate(const Capability::Xml::PPciBus::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PPciSlot

namespace Capability
{
namespace Xml
{
struct PPciSlot
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Capability

template<>
struct Validatable<Capability::Xml::PPciSlot>: mpl::true_
{
	static bool validate(const Capability::Xml::PPciSlot::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PPciFunc

namespace Capability
{
namespace Xml
{
struct PPciFunc
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Capability

template<>
struct Validatable<Capability::Xml::PPciFunc>: mpl::true_
{
	static bool validate(const Capability::Xml::PPciFunc::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PWwn

namespace Capability
{
namespace Xml
{
struct PWwn
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Capability

template<>
struct Validatable<Capability::Xml::PWwn>: mpl::true_
{
	static bool validate(const Capability::Xml::PWwn::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData4046

namespace Capability
{
namespace Xml
{
struct PData4046
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Capability

template<>
struct Validatable<Capability::Xml::PData4046>: mpl::true_
{
	static bool validate(const Capability::Xml::PData4046::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData4048

namespace Capability
{
namespace Xml
{
struct PData4048
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Capability

template<>
struct Validatable<Capability::Xml::PData4048>: mpl::true_
{
	static bool validate(const Capability::Xml::PData4048::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData4050

namespace Capability
{
namespace Xml
{
struct PData4050
{
	typedef qint32 value_type;
};

} // namespace Xml
} // namespace Capability

template<>
struct Traits<Capability::Xml::PData4050>
{
	static bool parse(const QString& src_, Capability::Xml::PData4050::value_type& dst_);

	static QString generate(Capability::Xml::PData4050::value_type src_);

};

template<>
struct Validatable<Capability::Xml::PData4050>: mpl::true_
{
	static bool validate(Capability::Xml::PData4050::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PCcwSsidRange

namespace Capability
{
namespace Xml
{
struct PCcwSsidRange
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Capability

template<>
struct Validatable<Capability::Xml::PCcwSsidRange>: mpl::true_
{
	static bool validate(const Capability::Xml::PCcwSsidRange::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData4052

namespace Capability
{
namespace Xml
{
struct PData4052
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Capability

template<>
struct Validatable<Capability::Xml::PData4052>: mpl::true_
{
	static bool validate(const Capability::Xml::PData4052::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData4054

namespace Capability
{
namespace Xml
{
struct PData4054
{
	typedef qint32 value_type;
};

} // namespace Xml
} // namespace Capability

template<>
struct Traits<Capability::Xml::PData4054>
{
	static bool parse(const QString& src_, Capability::Xml::PData4054::value_type& dst_);

	static QString generate(Capability::Xml::PData4054::value_type src_);

};

template<>
struct Validatable<Capability::Xml::PData4054>: mpl::true_
{
	static bool validate(Capability::Xml::PData4054::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PCpuset

namespace Capability
{
namespace Xml
{
struct PCpuset
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Capability

template<>
struct Validatable<Capability::Xml::PCpuset>: mpl::true_
{
	static bool validate(const Capability::Xml::PCpuset::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PVolName

namespace Capability
{
namespace Xml
{
struct PVolName
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Capability

template<>
struct Validatable<Capability::Xml::PVolName>: mpl::true_
{
	static bool validate(const Capability::Xml::PVolName::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PPortNumber

namespace Capability
{
namespace Xml
{
struct PPortNumber
{
	typedef qint32 value_type;
};

} // namespace Xml
} // namespace Capability

template<>
struct Traits<Capability::Xml::PPortNumber>
{
	static bool parse(const QString& src_, Capability::Xml::PPortNumber::value_type& dst_);

	static QString generate(Capability::Xml::PPortNumber::value_type src_);

};

template<>
struct Validatable<Capability::Xml::PPortNumber>: mpl::true_
{
	static bool validate(Capability::Xml::PPortNumber::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PIobase

namespace Capability
{
namespace Xml
{
struct PIobase
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Capability

template<>
struct Validatable<Capability::Xml::PIobase>: mpl::true_
{
	static bool validate(const Capability::Xml::PIobase::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PIrq

namespace Capability
{
namespace Xml
{
struct PIrq
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Capability

template<>
struct Validatable<Capability::Xml::PIrq>: mpl::true_
{
	static bool validate(const Capability::Xml::PIrq::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PVendorId

namespace Capability
{
namespace Xml
{
struct PVendorId
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Capability

template<>
struct Validatable<Capability::Xml::PVendorId>: mpl::true_
{
	static bool validate(const Capability::Xml::PVendorId::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PMemoryKB

namespace Capability
{
namespace Xml
{
struct PMemoryKB
{
	typedef ulong value_type;
};

} // namespace Xml
} // namespace Capability

template<>
struct Traits<Capability::Xml::PMemoryKB>
{
	static bool parse(const QString& src_, Capability::Xml::PMemoryKB::value_type& dst_);

	static QString generate(Capability::Xml::PMemoryKB::value_type src_);

};


///////////////////////////////////////////////////////////////////////////////
// struct PFeatureName

namespace Capability
{
namespace Xml
{
struct PFeatureName
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Capability

template<>
struct Validatable<Capability::Xml::PFeatureName>: mpl::true_
{
	static bool validate(const Capability::Xml::PFeatureName::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PName

namespace Capability
{
namespace Xml
{
struct PName
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Capability

template<>
struct Validatable<Capability::Xml::PName>: mpl::true_
{
	static bool validate(const Capability::Xml::PName::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PCbitpos

namespace Capability
{
namespace Xml
{
struct PCbitpos
{
	typedef quint32 value_type;
};

} // namespace Xml
} // namespace Capability

template<>
struct Traits<Capability::Xml::PCbitpos>
{
	static bool parse(const QString& src_, Capability::Xml::PCbitpos::value_type& dst_);

	static QString generate(Capability::Xml::PCbitpos::value_type src_);

};


///////////////////////////////////////////////////////////////////////////////
// struct PReducedPhysBits

namespace Capability
{
namespace Xml
{
struct PReducedPhysBits
{
	typedef quint32 value_type;
};

} // namespace Xml
} // namespace Capability

template<>
struct Traits<Capability::Xml::PReducedPhysBits>
{
	static bool parse(const QString& src_, Capability::Xml::PReducedPhysBits::value_type& dst_);

	static QString generate(Capability::Xml::PReducedPhysBits::value_type src_);

};


///////////////////////////////////////////////////////////////////////////////
// struct PMaxGuests

namespace Capability
{
namespace Xml
{
struct PMaxGuests
{
	typedef quint32 value_type;
};

} // namespace Xml
} // namespace Capability

template<>
struct Traits<Capability::Xml::PMaxGuests>
{
	static bool parse(const QString& src_, Capability::Xml::PMaxGuests::value_type& dst_);

	static QString generate(Capability::Xml::PMaxGuests::value_type src_);

};


///////////////////////////////////////////////////////////////////////////////
// struct PMaxESGuests

namespace Capability
{
namespace Xml
{
struct PMaxESGuests
{
	typedef quint32 value_type;
};

} // namespace Xml
} // namespace Capability

template<>
struct Traits<Capability::Xml::PMaxESGuests>
{
	static bool parse(const QString& src_, Capability::Xml::PMaxESGuests::value_type& dst_);

	static QString generate(Capability::Xml::PMaxESGuests::value_type src_);

};


} // namespace Libvirt

#endif // __CAPABILITY_DATA_H__
