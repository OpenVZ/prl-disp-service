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

#ifndef __NETWORK_DATA_H__
#define __NETWORK_DATA_H__
#include <QString>
#include <QDateTime>
#include "base.h"
#include "patterns.h"
#include "network_enum.h"

namespace Libvirt
{
///////////////////////////////////////////////////////////////////////////////
// struct PUnsignedInt

namespace Network
{
namespace Xml
{
struct PUnsignedInt
{
	typedef quint32 value_type;
};

} // namespace Xml
} // namespace Network

template<>
struct Traits<Network::Xml::PUnsignedInt>
{
	static bool parse(const QString& src_, Network::Xml::PUnsignedInt::value_type& dst_);

	static QString generate(Network::Xml::PUnsignedInt::value_type src_);

};


///////////////////////////////////////////////////////////////////////////////
// struct PUnsignedLong

namespace Network
{
namespace Xml
{
struct PUnsignedLong
{
	typedef ulong value_type;
};

} // namespace Xml
} // namespace Network

template<>
struct Traits<Network::Xml::PUnsignedLong>
{
	static bool parse(const QString& src_, Network::Xml::PUnsignedLong::value_type& dst_);

	static QString generate(Network::Xml::PUnsignedLong::value_type src_);

};


///////////////////////////////////////////////////////////////////////////////
// struct PHexuint

namespace Network
{
namespace Xml
{
struct PHexuint
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Network

template<>
struct Validatable<Network::Xml::PHexuint>: mpl::true_
{
	static bool validate(const Network::Xml::PHexuint::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PPositiveInteger

namespace Network
{
namespace Xml
{
struct PPositiveInteger
{
	typedef quint32 value_type;
};

} // namespace Xml
} // namespace Network

template<>
struct Traits<Network::Xml::PPositiveInteger>
{
	static bool parse(const QString& src_, Network::Xml::PPositiveInteger::value_type& dst_);

	static QString generate(Network::Xml::PPositiveInteger::value_type src_);

};


///////////////////////////////////////////////////////////////////////////////
// struct POctalMode

namespace Network
{
namespace Xml
{
struct POctalMode
{
	typedef quint32 value_type;
};

} // namespace Xml
} // namespace Network

template<>
struct Traits<Network::Xml::POctalMode>
{
	static bool parse(const QString& src_, Network::Xml::POctalMode::value_type& dst_);

	static QString generate(Network::Xml::POctalMode::value_type src_);

};


///////////////////////////////////////////////////////////////////////////////
// struct PData4775

namespace Network
{
namespace Xml
{
struct PData4775
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Network

template<>
struct Validatable<Network::Xml::PData4775>: mpl::true_
{
	static bool validate(const Network::Xml::PData4775::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData2360

namespace Network
{
namespace Xml
{
struct PData2360
{
	typedef qint32 value_type;
};

} // namespace Xml
} // namespace Network

template<>
struct Traits<Network::Xml::PData2360>
{
	static bool parse(const QString& src_, Network::Xml::PData2360::value_type& dst_);

	static QString generate(Network::Xml::PData2360::value_type src_);

};

template<>
struct Validatable<Network::Xml::PData2360>: mpl::true_
{
	static bool validate(Network::Xml::PData2360::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData2362

namespace Network
{
namespace Xml
{
struct PData2362
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Network

template<>
struct Validatable<Network::Xml::PData2362>: mpl::true_
{
	static bool validate(const Network::Xml::PData2362::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData2364

namespace Network
{
namespace Xml
{
struct PData2364
{
	typedef qint32 value_type;
};

} // namespace Xml
} // namespace Network

template<>
struct Traits<Network::Xml::PData2364>
{
	static bool parse(const QString& src_, Network::Xml::PData2364::value_type& dst_);

	static QString generate(Network::Xml::PData2364::value_type src_);

};

template<>
struct Validatable<Network::Xml::PData2364>: mpl::true_
{
	static bool validate(Network::Xml::PData2364::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData2366

namespace Network
{
namespace Xml
{
struct PData2366
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Network

template<>
struct Validatable<Network::Xml::PData2366>: mpl::true_
{
	static bool validate(const Network::Xml::PData2366::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData7120

namespace Network
{
namespace Xml
{
struct PData7120
{
	typedef qint32 value_type;
};

} // namespace Xml
} // namespace Network

template<>
struct Traits<Network::Xml::PData7120>
{
	static bool parse(const QString& src_, Network::Xml::PData7120::value_type& dst_);

	static QString generate(Network::Xml::PData7120::value_type src_);

};

template<>
struct Validatable<Network::Xml::PData7120>: mpl::true_
{
	static bool validate(Network::Xml::PData7120::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData7122

namespace Network
{
namespace Xml
{
struct PData7122
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Network

template<>
struct Validatable<Network::Xml::PData7122>: mpl::true_
{
	static bool validate(const Network::Xml::PData7122::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData2368

namespace Network
{
namespace Xml
{
struct PData2368
{
	typedef qint32 value_type;
};

} // namespace Xml
} // namespace Network

template<>
struct Traits<Network::Xml::PData2368>
{
	static bool parse(const QString& src_, Network::Xml::PData2368::value_type& dst_);

	static QString generate(Network::Xml::PData2368::value_type src_);

};

template<>
struct Validatable<Network::Xml::PData2368>: mpl::true_
{
	static bool validate(Network::Xml::PData2368::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData2370

namespace Network
{
namespace Xml
{
struct PData2370
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Network

template<>
struct Validatable<Network::Xml::PData2370>: mpl::true_
{
	static bool validate(const Network::Xml::PData2370::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData7124

namespace Network
{
namespace Xml
{
struct PData7124
{
	typedef qint32 value_type;
};

} // namespace Xml
} // namespace Network

template<>
struct Traits<Network::Xml::PData7124>
{
	static bool parse(const QString& src_, Network::Xml::PData7124::value_type& dst_);

	static QString generate(Network::Xml::PData7124::value_type src_);

};

template<>
struct Validatable<Network::Xml::PData7124>: mpl::true_
{
	static bool validate(Network::Xml::PData7124::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData7126

namespace Network
{
namespace Xml
{
struct PData7126
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Network

template<>
struct Validatable<Network::Xml::PData7126>: mpl::true_
{
	static bool validate(const Network::Xml::PData7126::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData7128

namespace Network
{
namespace Xml
{
struct PData7128
{
	typedef quint32 value_type;
};

} // namespace Xml
} // namespace Network

template<>
struct Traits<Network::Xml::PData7128>
{
	static bool parse(const QString& src_, Network::Xml::PData7128::value_type& dst_);

	static QString generate(Network::Xml::PData7128::value_type src_);

};


///////////////////////////////////////////////////////////////////////////////
// struct PData7130

namespace Network
{
namespace Xml
{
struct PData7130
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Network

template<>
struct Validatable<Network::Xml::PData7130>: mpl::true_
{
	static bool validate(const Network::Xml::PData7130::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData2372

namespace Network
{
namespace Xml
{
struct PData2372
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Network

template<>
struct Validatable<Network::Xml::PData2372>: mpl::true_
{
	static bool validate(const Network::Xml::PData2372::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PNumaDistanceValue

namespace Network
{
namespace Xml
{
struct PNumaDistanceValue
{
	typedef quint32 value_type;
};

} // namespace Xml
} // namespace Network

template<>
struct Traits<Network::Xml::PNumaDistanceValue>
{
	static bool parse(const QString& src_, Network::Xml::PNumaDistanceValue::value_type& dst_);

	static QString generate(Network::Xml::PNumaDistanceValue::value_type src_);

};

template<>
struct Validatable<Network::Xml::PNumaDistanceValue>: mpl::true_
{
	static bool validate(Network::Xml::PNumaDistanceValue::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PUniMacAddr

namespace Network
{
namespace Xml
{
struct PUniMacAddr
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Network

template<>
struct Validatable<Network::Xml::PUniMacAddr>: mpl::true_
{
	static bool validate(const Network::Xml::PUniMacAddr::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PMultiMacAddr

namespace Network
{
namespace Xml
{
struct PMultiMacAddr
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Network

template<>
struct Validatable<Network::Xml::PMultiMacAddr>: mpl::true_
{
	static bool validate(const Network::Xml::PMultiMacAddr::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PMacAddr

namespace Network
{
namespace Xml
{
struct PMacAddr
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Network

template<>
struct Validatable<Network::Xml::PMacAddr>: mpl::true_
{
	static bool validate(const Network::Xml::PMacAddr::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PDuidLLT

namespace Network
{
namespace Xml
{
struct PDuidLLT
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Network

template<>
struct Validatable<Network::Xml::PDuidLLT>: mpl::true_
{
	static bool validate(const Network::Xml::PDuidLLT::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PDuidEN

namespace Network
{
namespace Xml
{
struct PDuidEN
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Network

template<>
struct Validatable<Network::Xml::PDuidEN>: mpl::true_
{
	static bool validate(const Network::Xml::PDuidEN::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PDuidLL

namespace Network
{
namespace Xml
{
struct PDuidLL
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Network

template<>
struct Validatable<Network::Xml::PDuidLL>: mpl::true_
{
	static bool validate(const Network::Xml::PDuidLL::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PDuidUUID

namespace Network
{
namespace Xml
{
struct PDuidUUID
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Network

template<>
struct Validatable<Network::Xml::PDuidUUID>: mpl::true_
{
	static bool validate(const Network::Xml::PDuidUUID::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PIpv4Addr

namespace Network
{
namespace Xml
{
struct PIpv4Addr
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Network

template<>
struct Validatable<Network::Xml::PIpv4Addr>: mpl::true_
{
	static bool validate(const Network::Xml::PIpv4Addr::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PIpv6Addr

namespace Network
{
namespace Xml
{
struct PIpv6Addr
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Network

template<>
struct Validatable<Network::Xml::PIpv6Addr>: mpl::true_
{
	static bool validate(const Network::Xml::PIpv6Addr::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PIpv4Prefix

namespace Network
{
namespace Xml
{
struct PIpv4Prefix
{
	typedef quint32 value_type;
};

} // namespace Xml
} // namespace Network

template<>
struct Traits<Network::Xml::PIpv4Prefix>
{
	static bool parse(const QString& src_, Network::Xml::PIpv4Prefix::value_type& dst_);

	static QString generate(Network::Xml::PIpv4Prefix::value_type src_);

};

template<>
struct Validatable<Network::Xml::PIpv4Prefix>: mpl::true_
{
	static bool validate(Network::Xml::PIpv4Prefix::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PIpv6Prefix

namespace Network
{
namespace Xml
{
struct PIpv6Prefix
{
	typedef quint32 value_type;
};

} // namespace Xml
} // namespace Network

template<>
struct Traits<Network::Xml::PIpv6Prefix>
{
	static bool parse(const QString& src_, Network::Xml::PIpv6Prefix::value_type& dst_);

	static QString generate(Network::Xml::PIpv6Prefix::value_type src_);

};

template<>
struct Validatable<Network::Xml::PIpv6Prefix>: mpl::true_
{
	static bool validate(Network::Xml::PIpv6Prefix::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PObjectNameWithSlash

namespace Network
{
namespace Xml
{
struct PObjectNameWithSlash
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Network

template<>
struct Validatable<Network::Xml::PObjectNameWithSlash>: mpl::true_
{
	static bool validate(const Network::Xml::PObjectNameWithSlash::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PObjectName

namespace Network
{
namespace Xml
{
struct PObjectName
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Network

template<>
struct Validatable<Network::Xml::PObjectName>: mpl::true_
{
	static bool validate(const Network::Xml::PObjectName::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PGenericName

namespace Network
{
namespace Xml
{
struct PGenericName
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Network

template<>
struct Validatable<Network::Xml::PGenericName>: mpl::true_
{
	static bool validate(const Network::Xml::PGenericName::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PDnsName

namespace Network
{
namespace Xml
{
struct PDnsName
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Network

template<>
struct Validatable<Network::Xml::PDnsName>: mpl::true_
{
	static bool validate(const Network::Xml::PDnsName::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PDeviceName

namespace Network
{
namespace Xml
{
struct PDeviceName
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Network

template<>
struct Validatable<Network::Xml::PDeviceName>: mpl::true_
{
	static bool validate(const Network::Xml::PDeviceName::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PZoneName

namespace Network
{
namespace Xml
{
struct PZoneName
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Network

template<>
struct Validatable<Network::Xml::PZoneName>: mpl::true_
{
	static bool validate(const Network::Xml::PZoneName::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PFilePath

namespace Network
{
namespace Xml
{
struct PFilePath
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Network

template<>
struct Validatable<Network::Xml::PFilePath>: mpl::true_
{
	static bool validate(const Network::Xml::PFilePath::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PDirPath

namespace Network
{
namespace Xml
{
struct PDirPath
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Network

template<>
struct Validatable<Network::Xml::PDirPath>: mpl::true_
{
	static bool validate(const Network::Xml::PDirPath::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PAbsFilePath

namespace Network
{
namespace Xml
{
struct PAbsFilePath
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Network

template<>
struct Validatable<Network::Xml::PAbsFilePath>: mpl::true_
{
	static bool validate(const Network::Xml::PAbsFilePath::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PVmwarePath

namespace Network
{
namespace Xml
{
struct PVmwarePath
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Network

template<>
struct Validatable<Network::Xml::PVmwarePath>: mpl::true_
{
	static bool validate(const Network::Xml::PVmwarePath::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PAbsDirPath

namespace Network
{
namespace Xml
{
struct PAbsDirPath
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Network

template<>
struct Validatable<Network::Xml::PAbsDirPath>: mpl::true_
{
	static bool validate(const Network::Xml::PAbsDirPath::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PUnit

namespace Network
{
namespace Xml
{
struct PUnit
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Network

template<>
struct Validatable<Network::Xml::PUnit>: mpl::true_
{
	static bool validate(const Network::Xml::PUnit::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PPciDomain

namespace Network
{
namespace Xml
{
struct PPciDomain
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Network

template<>
struct Validatable<Network::Xml::PPciDomain>: mpl::true_
{
	static bool validate(const Network::Xml::PPciDomain::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PPciBus

namespace Network
{
namespace Xml
{
struct PPciBus
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Network

template<>
struct Validatable<Network::Xml::PPciBus>: mpl::true_
{
	static bool validate(const Network::Xml::PPciBus::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PPciSlot

namespace Network
{
namespace Xml
{
struct PPciSlot
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Network

template<>
struct Validatable<Network::Xml::PPciSlot>: mpl::true_
{
	static bool validate(const Network::Xml::PPciSlot::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PPciFunc

namespace Network
{
namespace Xml
{
struct PPciFunc
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Network

template<>
struct Validatable<Network::Xml::PPciFunc>: mpl::true_
{
	static bool validate(const Network::Xml::PPciFunc::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PWwn

namespace Network
{
namespace Xml
{
struct PWwn
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Network

template<>
struct Validatable<Network::Xml::PWwn>: mpl::true_
{
	static bool validate(const Network::Xml::PWwn::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData2382

namespace Network
{
namespace Xml
{
struct PData2382
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Network

template<>
struct Validatable<Network::Xml::PData2382>: mpl::true_
{
	static bool validate(const Network::Xml::PData2382::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData3785

namespace Network
{
namespace Xml
{
struct PData3785
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Network

template<>
struct Validatable<Network::Xml::PData3785>: mpl::true_
{
	static bool validate(const Network::Xml::PData3785::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData3787

namespace Network
{
namespace Xml
{
struct PData3787
{
	typedef qint32 value_type;
};

} // namespace Xml
} // namespace Network

template<>
struct Traits<Network::Xml::PData3787>
{
	static bool parse(const QString& src_, Network::Xml::PData3787::value_type& dst_);

	static QString generate(Network::Xml::PData3787::value_type src_);

};

template<>
struct Validatable<Network::Xml::PData3787>: mpl::true_
{
	static bool validate(Network::Xml::PData3787::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PCcwSsidRange

namespace Network
{
namespace Xml
{
struct PCcwSsidRange
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Network

template<>
struct Validatable<Network::Xml::PCcwSsidRange>: mpl::true_
{
	static bool validate(const Network::Xml::PCcwSsidRange::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData3789

namespace Network
{
namespace Xml
{
struct PData3789
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Network

template<>
struct Validatable<Network::Xml::PData3789>: mpl::true_
{
	static bool validate(const Network::Xml::PData3789::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData3791

namespace Network
{
namespace Xml
{
struct PData3791
{
	typedef qint32 value_type;
};

} // namespace Xml
} // namespace Network

template<>
struct Traits<Network::Xml::PData3791>
{
	static bool parse(const QString& src_, Network::Xml::PData3791::value_type& dst_);

	static QString generate(Network::Xml::PData3791::value_type src_);

};

template<>
struct Validatable<Network::Xml::PData3791>: mpl::true_
{
	static bool validate(Network::Xml::PData3791::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PCpuset

namespace Network
{
namespace Xml
{
struct PCpuset
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Network

template<>
struct Validatable<Network::Xml::PCpuset>: mpl::true_
{
	static bool validate(const Network::Xml::PCpuset::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PVolName

namespace Network
{
namespace Xml
{
struct PVolName
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Network

template<>
struct Validatable<Network::Xml::PVolName>: mpl::true_
{
	static bool validate(const Network::Xml::PVolName::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PPortNumber

namespace Network
{
namespace Xml
{
struct PPortNumber
{
	typedef qint32 value_type;
};

} // namespace Xml
} // namespace Network

template<>
struct Traits<Network::Xml::PPortNumber>
{
	static bool parse(const QString& src_, Network::Xml::PPortNumber::value_type& dst_);

	static QString generate(Network::Xml::PPortNumber::value_type src_);

};

template<>
struct Validatable<Network::Xml::PPortNumber>: mpl::true_
{
	static bool validate(Network::Xml::PPortNumber::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PIobase

namespace Network
{
namespace Xml
{
struct PIobase
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Network

template<>
struct Validatable<Network::Xml::PIobase>: mpl::true_
{
	static bool validate(const Network::Xml::PIobase::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PIrq

namespace Network
{
namespace Xml
{
struct PIrq
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Network

template<>
struct Validatable<Network::Xml::PIrq>: mpl::true_
{
	static bool validate(const Network::Xml::PIrq::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PVirtualPortProfileID

namespace Network
{
namespace Xml
{
struct PVirtualPortProfileID
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Network

template<>
struct Validatable<Network::Xml::PVirtualPortProfileID>: mpl::true_
{
	static bool validate(const Network::Xml::PVirtualPortProfileID::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PSpeed

namespace Network
{
namespace Xml
{
struct PSpeed
{
	typedef quint32 value_type;
};

} // namespace Xml
} // namespace Network

template<>
struct Traits<Network::Xml::PSpeed>
{
	static bool parse(const QString& src_, Network::Xml::PSpeed::value_type& dst_);

	static QString generate(Network::Xml::PSpeed::value_type src_);

};

template<>
struct Validatable<Network::Xml::PSpeed>: mpl::true_
{
	static bool validate(Network::Xml::PSpeed::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PBurstSize

namespace Network
{
namespace Xml
{
struct PBurstSize
{
	typedef quint32 value_type;
};

} // namespace Xml
} // namespace Network

template<>
struct Traits<Network::Xml::PBurstSize>
{
	static bool parse(const QString& src_, Network::Xml::PBurstSize::value_type& dst_);

	static QString generate(Network::Xml::PBurstSize::value_type src_);

};

template<>
struct Validatable<Network::Xml::PBurstSize>: mpl::true_
{
	static bool validate(Network::Xml::PBurstSize::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PUnsignedShort

namespace Network
{
namespace Xml
{
struct PUnsignedShort
{
	typedef qint32 value_type;
};

} // namespace Xml
} // namespace Network

template<>
struct Traits<Network::Xml::PUnsignedShort>
{
	static bool parse(const QString& src_, Network::Xml::PUnsignedShort::value_type& dst_);

	static QString generate(Network::Xml::PUnsignedShort::value_type src_);

};

template<>
struct Validatable<Network::Xml::PUnsignedShort>: mpl::true_
{
	static bool validate(Network::Xml::PUnsignedShort::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PProtocol

namespace Network
{
namespace Xml
{
struct PProtocol
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Network

template<>
struct Validatable<Network::Xml::PProtocol>: mpl::true_
{
	static bool validate(const Network::Xml::PProtocol::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PAddrFamily

namespace Network
{
namespace Xml
{
struct PAddrFamily
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Network

template<>
struct Validatable<Network::Xml::PAddrFamily>: mpl::true_
{
	static bool validate(const Network::Xml::PAddrFamily::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PId

namespace Network
{
namespace Xml
{
struct PId
{
	typedef quint32 value_type;
};

} // namespace Xml
} // namespace Network

template<>
struct Traits<Network::Xml::PId>
{
	static bool parse(const QString& src_, Network::Xml::PId::value_type& dst_);

	static QString generate(Network::Xml::PId::value_type src_);

};

template<>
struct Validatable<Network::Xml::PId>: mpl::true_
{
	static bool validate(Network::Xml::PId::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PPort

namespace Network
{
namespace Xml
{
struct PPort
{
	typedef qint32 value_type;
};

} // namespace Xml
} // namespace Network

template<>
struct Traits<Network::Xml::PPort>
{
	static bool parse(const QString& src_, Network::Xml::PPort::value_type& dst_);

	static QString generate(Network::Xml::PPort::value_type src_);

};

template<>
struct Validatable<Network::Xml::PPort>: mpl::true_
{
	static bool validate(Network::Xml::PPort::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PConnections

namespace Network
{
namespace Xml
{
struct PConnections
{
	typedef quint32 value_type;
};

} // namespace Xml
} // namespace Network

template<>
struct Traits<Network::Xml::PConnections>
{
	static bool parse(const QString& src_, Network::Xml::PConnections::value_type& dst_);

	static QString generate(Network::Xml::PConnections::value_type src_);

};


///////////////////////////////////////////////////////////////////////////////
// struct PDelay

namespace Network
{
namespace Xml
{
struct PDelay
{
	typedef ulong value_type;
};

} // namespace Xml
} // namespace Network

template<>
struct Traits<Network::Xml::PDelay>
{
	static bool parse(const QString& src_, Network::Xml::PDelay::value_type& dst_);

	static QString generate(Network::Xml::PDelay::value_type src_);

};


///////////////////////////////////////////////////////////////////////////////
// struct VUUID

namespace Network
{
namespace Xml
{
typedef Choice<mpl::vector<PData7130, PData2372 > > VUUIDImpl;
typedef VUUIDImpl::value_type VUUID;

} // namespace Xml
} // namespace Network

template<>
struct Traits<Network::Xml::VUUID>
{
	static bool parse(const QString& src_, Network::Xml::VUUID& dst_);

	static QString generate(const Network::Xml::VUUID& src_);

};

///////////////////////////////////////////////////////////////////////////////
// struct VUint8

namespace Network
{
namespace Xml
{
typedef Choice<mpl::vector<PData4775, PData2360 > > VUint8Impl;
typedef VUint8Impl::value_type VUint8;

} // namespace Xml
} // namespace Network

template<>
struct Traits<Network::Xml::VUint8>
{
	static bool parse(const QString& src_, Network::Xml::VUint8& dst_);

	static QString generate(const Network::Xml::VUint8& src_);

};

///////////////////////////////////////////////////////////////////////////////
// struct VUint24

namespace Network
{
namespace Xml
{
typedef Choice<mpl::vector<PData2370, PData7124 > > VUint24Impl;
typedef VUint24Impl::value_type VUint24;

} // namespace Xml
} // namespace Network

template<>
struct Traits<Network::Xml::VUint24>
{
	static bool parse(const QString& src_, Network::Xml::VUint24& dst_);

	static QString generate(const Network::Xml::VUint24& src_);

};

///////////////////////////////////////////////////////////////////////////////
// struct VIpAddr

namespace Network
{
namespace Xml
{
typedef Choice<mpl::vector<PIpv4Addr, PIpv6Addr > > VIpAddrImpl;
typedef VIpAddrImpl::value_type VIpAddr;

} // namespace Xml
} // namespace Network

template<>
struct Traits<Network::Xml::VIpAddr>
{
	static bool parse(const QString& src_, Network::Xml::VIpAddr& dst_);

	static QString generate(const Network::Xml::VIpAddr& src_);

};

///////////////////////////////////////////////////////////////////////////////
// struct VIpPrefix

namespace Network
{
namespace Xml
{
typedef Choice<mpl::vector<PIpv4Prefix, PIpv6Prefix > > VIpPrefixImpl;
typedef VIpPrefixImpl::value_type VIpPrefix;

} // namespace Xml
} // namespace Network

template<>
struct Traits<Network::Xml::VIpPrefix>
{
	static bool parse(const QString& src_, Network::Xml::VIpPrefix& dst_);

	static QString generate(const Network::Xml::VIpPrefix& src_);

};

///////////////////////////////////////////////////////////////////////////////
// struct VDUID

namespace Network
{
namespace Xml
{
typedef Choice<mpl::vector<PDuidLLT, PDuidEN, PDuidLL, PDuidUUID > > VDUIDImpl;
typedef VDUIDImpl::value_type VDUID;

} // namespace Xml
} // namespace Network

template<>
struct Traits<Network::Xml::VDUID>
{
	static bool parse(const QString& src_, Network::Xml::VDUID& dst_);

	static QString generate(const Network::Xml::VDUID& src_);

};

} // namespace Libvirt

#endif // __NETWORK_DATA_H__
