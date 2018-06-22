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
// struct PData1830

namespace Capability
{
namespace Xml
{
struct PData1830
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Capability

template<>
struct Validatable<Capability::Xml::PData1830>: mpl::true_
{
	static bool validate(const Capability::Xml::PData1830::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData1832

namespace Capability
{
namespace Xml
{
struct PData1832
{
	typedef qint32 value_type;
};

} // namespace Xml
} // namespace Capability

template<>
struct Traits<Capability::Xml::PData1832>
{
	static bool parse(const QString& src_, Capability::Xml::PData1832::value_type& dst_);

	static QString generate(Capability::Xml::PData1832::value_type src_);

};

template<>
struct Validatable<Capability::Xml::PData1832>: mpl::true_
{
	static bool validate(Capability::Xml::PData1832::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData1834

namespace Capability
{
namespace Xml
{
struct PData1834
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Capability

template<>
struct Validatable<Capability::Xml::PData1834>: mpl::true_
{
	static bool validate(const Capability::Xml::PData1834::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData1836

namespace Capability
{
namespace Xml
{
struct PData1836
{
	typedef qint32 value_type;
};

} // namespace Xml
} // namespace Capability

template<>
struct Traits<Capability::Xml::PData1836>
{
	static bool parse(const QString& src_, Capability::Xml::PData1836::value_type& dst_);

	static QString generate(Capability::Xml::PData1836::value_type src_);

};

template<>
struct Validatable<Capability::Xml::PData1836>: mpl::true_
{
	static bool validate(Capability::Xml::PData1836::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData1838

namespace Capability
{
namespace Xml
{
struct PData1838
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Capability

template<>
struct Validatable<Capability::Xml::PData1838>: mpl::true_
{
	static bool validate(const Capability::Xml::PData1838::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData1840

namespace Capability
{
namespace Xml
{
struct PData1840
{
	typedef qint32 value_type;
};

} // namespace Xml
} // namespace Capability

template<>
struct Traits<Capability::Xml::PData1840>
{
	static bool parse(const QString& src_, Capability::Xml::PData1840::value_type& dst_);

	static QString generate(Capability::Xml::PData1840::value_type src_);

};

template<>
struct Validatable<Capability::Xml::PData1840>: mpl::true_
{
	static bool validate(Capability::Xml::PData1840::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData1842

namespace Capability
{
namespace Xml
{
struct PData1842
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Capability

template<>
struct Validatable<Capability::Xml::PData1842>: mpl::true_
{
	static bool validate(const Capability::Xml::PData1842::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData1844

namespace Capability
{
namespace Xml
{
struct PData1844
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Capability

template<>
struct Validatable<Capability::Xml::PData1844>: mpl::true_
{
	static bool validate(const Capability::Xml::PData1844::value_type& value_);

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

} // namespace Libvirt

#endif // __CAPABILITY_DATA_H__
