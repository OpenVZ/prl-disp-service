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

#ifndef __FILTER_DATA_H__
#define __FILTER_DATA_H__
#include <QString>
#include <QDateTime>
#include "base.h"
#include "patterns.h"
#include "filter_enum.h"

namespace Libvirt
{
///////////////////////////////////////////////////////////////////////////////
// struct PUnsignedInt

namespace Filter
{
namespace Xml
{
struct PUnsignedInt
{
	typedef quint32 value_type;
};

} // namespace Xml
} // namespace Filter

template<>
struct Traits<Filter::Xml::PUnsignedInt>
{
	static bool parse(const QString& src_, Filter::Xml::PUnsignedInt::value_type& dst_);

	static QString generate(Filter::Xml::PUnsignedInt::value_type src_);

};


///////////////////////////////////////////////////////////////////////////////
// struct PUnsignedLong

namespace Filter
{
namespace Xml
{
struct PUnsignedLong
{
	typedef ulong value_type;
};

} // namespace Xml
} // namespace Filter

template<>
struct Traits<Filter::Xml::PUnsignedLong>
{
	static bool parse(const QString& src_, Filter::Xml::PUnsignedLong::value_type& dst_);

	static QString generate(Filter::Xml::PUnsignedLong::value_type src_);

};


///////////////////////////////////////////////////////////////////////////////
// struct PHexuint

namespace Filter
{
namespace Xml
{
struct PHexuint
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Filter

template<>
struct Validatable<Filter::Xml::PHexuint>: mpl::true_
{
	static bool validate(const Filter::Xml::PHexuint::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PPositiveInteger

namespace Filter
{
namespace Xml
{
struct PPositiveInteger
{
	typedef quint32 value_type;
};

} // namespace Xml
} // namespace Filter

template<>
struct Traits<Filter::Xml::PPositiveInteger>
{
	static bool parse(const QString& src_, Filter::Xml::PPositiveInteger::value_type& dst_);

	static QString generate(Filter::Xml::PPositiveInteger::value_type src_);

};


///////////////////////////////////////////////////////////////////////////////
// struct POctalMode

namespace Filter
{
namespace Xml
{
struct POctalMode
{
	typedef quint32 value_type;
};

} // namespace Xml
} // namespace Filter

template<>
struct Traits<Filter::Xml::POctalMode>
{
	static bool parse(const QString& src_, Filter::Xml::POctalMode::value_type& dst_);

	static QString generate(Filter::Xml::POctalMode::value_type src_);

};


///////////////////////////////////////////////////////////////////////////////
// struct PData5567

namespace Filter
{
namespace Xml
{
struct PData5567
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Filter

template<>
struct Validatable<Filter::Xml::PData5567>: mpl::true_
{
	static bool validate(const Filter::Xml::PData5567::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData5569

namespace Filter
{
namespace Xml
{
struct PData5569
{
	typedef qint32 value_type;
};

} // namespace Xml
} // namespace Filter

template<>
struct Traits<Filter::Xml::PData5569>
{
	static bool parse(const QString& src_, Filter::Xml::PData5569::value_type& dst_);

	static QString generate(Filter::Xml::PData5569::value_type src_);

};

template<>
struct Validatable<Filter::Xml::PData5569>: mpl::true_
{
	static bool validate(Filter::Xml::PData5569::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData5571

namespace Filter
{
namespace Xml
{
struct PData5571
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Filter

template<>
struct Validatable<Filter::Xml::PData5571>: mpl::true_
{
	static bool validate(const Filter::Xml::PData5571::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData5573

namespace Filter
{
namespace Xml
{
struct PData5573
{
	typedef qint32 value_type;
};

} // namespace Xml
} // namespace Filter

template<>
struct Traits<Filter::Xml::PData5573>
{
	static bool parse(const QString& src_, Filter::Xml::PData5573::value_type& dst_);

	static QString generate(Filter::Xml::PData5573::value_type src_);

};

template<>
struct Validatable<Filter::Xml::PData5573>: mpl::true_
{
	static bool validate(Filter::Xml::PData5573::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData5575

namespace Filter
{
namespace Xml
{
struct PData5575
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Filter

template<>
struct Validatable<Filter::Xml::PData5575>: mpl::true_
{
	static bool validate(const Filter::Xml::PData5575::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData5577

namespace Filter
{
namespace Xml
{
struct PData5577
{
	typedef qint32 value_type;
};

} // namespace Xml
} // namespace Filter

template<>
struct Traits<Filter::Xml::PData5577>
{
	static bool parse(const QString& src_, Filter::Xml::PData5577::value_type& dst_);

	static QString generate(Filter::Xml::PData5577::value_type src_);

};

template<>
struct Validatable<Filter::Xml::PData5577>: mpl::true_
{
	static bool validate(Filter::Xml::PData5577::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData5579

namespace Filter
{
namespace Xml
{
struct PData5579
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Filter

template<>
struct Validatable<Filter::Xml::PData5579>: mpl::true_
{
	static bool validate(const Filter::Xml::PData5579::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData5581

namespace Filter
{
namespace Xml
{
struct PData5581
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Filter

template<>
struct Validatable<Filter::Xml::PData5581>: mpl::true_
{
	static bool validate(const Filter::Xml::PData5581::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PUniMacAddr

namespace Filter
{
namespace Xml
{
struct PUniMacAddr
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Filter

template<>
struct Validatable<Filter::Xml::PUniMacAddr>: mpl::true_
{
	static bool validate(const Filter::Xml::PUniMacAddr::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PMultiMacAddr

namespace Filter
{
namespace Xml
{
struct PMultiMacAddr
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Filter

template<>
struct Validatable<Filter::Xml::PMultiMacAddr>: mpl::true_
{
	static bool validate(const Filter::Xml::PMultiMacAddr::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PMacAddr

namespace Filter
{
namespace Xml
{
struct PMacAddr
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Filter

template<>
struct Validatable<Filter::Xml::PMacAddr>: mpl::true_
{
	static bool validate(const Filter::Xml::PMacAddr::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PDuidLLT

namespace Filter
{
namespace Xml
{
struct PDuidLLT
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Filter

template<>
struct Validatable<Filter::Xml::PDuidLLT>: mpl::true_
{
	static bool validate(const Filter::Xml::PDuidLLT::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PDuidEN

namespace Filter
{
namespace Xml
{
struct PDuidEN
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Filter

template<>
struct Validatable<Filter::Xml::PDuidEN>: mpl::true_
{
	static bool validate(const Filter::Xml::PDuidEN::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PDuidLL

namespace Filter
{
namespace Xml
{
struct PDuidLL
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Filter

template<>
struct Validatable<Filter::Xml::PDuidLL>: mpl::true_
{
	static bool validate(const Filter::Xml::PDuidLL::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PDuidUUID

namespace Filter
{
namespace Xml
{
struct PDuidUUID
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Filter

template<>
struct Validatable<Filter::Xml::PDuidUUID>: mpl::true_
{
	static bool validate(const Filter::Xml::PDuidUUID::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PIpv4Addr

namespace Filter
{
namespace Xml
{
struct PIpv4Addr
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Filter

template<>
struct Validatable<Filter::Xml::PIpv4Addr>: mpl::true_
{
	static bool validate(const Filter::Xml::PIpv4Addr::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PIpv6Addr

namespace Filter
{
namespace Xml
{
struct PIpv6Addr
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Filter

template<>
struct Validatable<Filter::Xml::PIpv6Addr>: mpl::true_
{
	static bool validate(const Filter::Xml::PIpv6Addr::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PIpv4Prefix

namespace Filter
{
namespace Xml
{
struct PIpv4Prefix
{
	typedef quint32 value_type;
};

} // namespace Xml
} // namespace Filter

template<>
struct Traits<Filter::Xml::PIpv4Prefix>
{
	static bool parse(const QString& src_, Filter::Xml::PIpv4Prefix::value_type& dst_);

	static QString generate(Filter::Xml::PIpv4Prefix::value_type src_);

};

template<>
struct Validatable<Filter::Xml::PIpv4Prefix>: mpl::true_
{
	static bool validate(Filter::Xml::PIpv4Prefix::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PIpv6Prefix

namespace Filter
{
namespace Xml
{
struct PIpv6Prefix
{
	typedef quint32 value_type;
};

} // namespace Xml
} // namespace Filter

template<>
struct Traits<Filter::Xml::PIpv6Prefix>
{
	static bool parse(const QString& src_, Filter::Xml::PIpv6Prefix::value_type& dst_);

	static QString generate(Filter::Xml::PIpv6Prefix::value_type src_);

};

template<>
struct Validatable<Filter::Xml::PIpv6Prefix>: mpl::true_
{
	static bool validate(Filter::Xml::PIpv6Prefix::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PGenericName

namespace Filter
{
namespace Xml
{
struct PGenericName
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Filter

template<>
struct Validatable<Filter::Xml::PGenericName>: mpl::true_
{
	static bool validate(const Filter::Xml::PGenericName::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PDnsName

namespace Filter
{
namespace Xml
{
struct PDnsName
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Filter

template<>
struct Validatable<Filter::Xml::PDnsName>: mpl::true_
{
	static bool validate(const Filter::Xml::PDnsName::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PDeviceName

namespace Filter
{
namespace Xml
{
struct PDeviceName
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Filter

template<>
struct Validatable<Filter::Xml::PDeviceName>: mpl::true_
{
	static bool validate(const Filter::Xml::PDeviceName::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PFilePath

namespace Filter
{
namespace Xml
{
struct PFilePath
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Filter

template<>
struct Validatable<Filter::Xml::PFilePath>: mpl::true_
{
	static bool validate(const Filter::Xml::PFilePath::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PDirPath

namespace Filter
{
namespace Xml
{
struct PDirPath
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Filter

template<>
struct Validatable<Filter::Xml::PDirPath>: mpl::true_
{
	static bool validate(const Filter::Xml::PDirPath::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PAbsFilePath

namespace Filter
{
namespace Xml
{
struct PAbsFilePath
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Filter

template<>
struct Validatable<Filter::Xml::PAbsFilePath>: mpl::true_
{
	static bool validate(const Filter::Xml::PAbsFilePath::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PAbsDirPath

namespace Filter
{
namespace Xml
{
struct PAbsDirPath
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Filter

template<>
struct Validatable<Filter::Xml::PAbsDirPath>: mpl::true_
{
	static bool validate(const Filter::Xml::PAbsDirPath::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PUnit

namespace Filter
{
namespace Xml
{
struct PUnit
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Filter

template<>
struct Validatable<Filter::Xml::PUnit>: mpl::true_
{
	static bool validate(const Filter::Xml::PUnit::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PPciDomain

namespace Filter
{
namespace Xml
{
struct PPciDomain
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Filter

template<>
struct Validatable<Filter::Xml::PPciDomain>: mpl::true_
{
	static bool validate(const Filter::Xml::PPciDomain::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PPciBus

namespace Filter
{
namespace Xml
{
struct PPciBus
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Filter

template<>
struct Validatable<Filter::Xml::PPciBus>: mpl::true_
{
	static bool validate(const Filter::Xml::PPciBus::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PPciSlot

namespace Filter
{
namespace Xml
{
struct PPciSlot
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Filter

template<>
struct Validatable<Filter::Xml::PPciSlot>: mpl::true_
{
	static bool validate(const Filter::Xml::PPciSlot::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PPciFunc

namespace Filter
{
namespace Xml
{
struct PPciFunc
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Filter

template<>
struct Validatable<Filter::Xml::PPciFunc>: mpl::true_
{
	static bool validate(const Filter::Xml::PPciFunc::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PWwn

namespace Filter
{
namespace Xml
{
struct PWwn
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Filter

template<>
struct Validatable<Filter::Xml::PWwn>: mpl::true_
{
	static bool validate(const Filter::Xml::PWwn::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData5591

namespace Filter
{
namespace Xml
{
struct PData5591
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Filter

template<>
struct Validatable<Filter::Xml::PData5591>: mpl::true_
{
	static bool validate(const Filter::Xml::PData5591::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData5593

namespace Filter
{
namespace Xml
{
struct PData5593
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Filter

template<>
struct Validatable<Filter::Xml::PData5593>: mpl::true_
{
	static bool validate(const Filter::Xml::PData5593::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData5595

namespace Filter
{
namespace Xml
{
struct PData5595
{
	typedef qint32 value_type;
};

} // namespace Xml
} // namespace Filter

template<>
struct Traits<Filter::Xml::PData5595>
{
	static bool parse(const QString& src_, Filter::Xml::PData5595::value_type& dst_);

	static QString generate(Filter::Xml::PData5595::value_type src_);

};

template<>
struct Validatable<Filter::Xml::PData5595>: mpl::true_
{
	static bool validate(Filter::Xml::PData5595::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PCcwSsidRange

namespace Filter
{
namespace Xml
{
struct PCcwSsidRange
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Filter

template<>
struct Validatable<Filter::Xml::PCcwSsidRange>: mpl::true_
{
	static bool validate(const Filter::Xml::PCcwSsidRange::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData5597

namespace Filter
{
namespace Xml
{
struct PData5597
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Filter

template<>
struct Validatable<Filter::Xml::PData5597>: mpl::true_
{
	static bool validate(const Filter::Xml::PData5597::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData5599

namespace Filter
{
namespace Xml
{
struct PData5599
{
	typedef qint32 value_type;
};

} // namespace Xml
} // namespace Filter

template<>
struct Traits<Filter::Xml::PData5599>
{
	static bool parse(const QString& src_, Filter::Xml::PData5599::value_type& dst_);

	static QString generate(Filter::Xml::PData5599::value_type src_);

};

template<>
struct Validatable<Filter::Xml::PData5599>: mpl::true_
{
	static bool validate(Filter::Xml::PData5599::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PCpuset

namespace Filter
{
namespace Xml
{
struct PCpuset
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Filter

template<>
struct Validatable<Filter::Xml::PCpuset>: mpl::true_
{
	static bool validate(const Filter::Xml::PCpuset::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PVolName

namespace Filter
{
namespace Xml
{
struct PVolName
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Filter

template<>
struct Validatable<Filter::Xml::PVolName>: mpl::true_
{
	static bool validate(const Filter::Xml::PVolName::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PPortNumber

namespace Filter
{
namespace Xml
{
struct PPortNumber
{
	typedef qint32 value_type;
};

} // namespace Xml
} // namespace Filter

template<>
struct Traits<Filter::Xml::PPortNumber>
{
	static bool parse(const QString& src_, Filter::Xml::PPortNumber::value_type& dst_);

	static QString generate(Filter::Xml::PPortNumber::value_type src_);

};

template<>
struct Validatable<Filter::Xml::PPortNumber>: mpl::true_
{
	static bool validate(Filter::Xml::PPortNumber::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PIobase

namespace Filter
{
namespace Xml
{
struct PIobase
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Filter

template<>
struct Validatable<Filter::Xml::PIobase>: mpl::true_
{
	static bool validate(const Filter::Xml::PIobase::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PIrq

namespace Filter
{
namespace Xml
{
struct PIrq
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Filter

template<>
struct Validatable<Filter::Xml::PIrq>: mpl::true_
{
	static bool validate(const Filter::Xml::PIrq::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PName

namespace Filter
{
namespace Xml
{
struct PName
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Filter


///////////////////////////////////////////////////////////////////////////////
// struct PData5604

namespace Filter
{
namespace Xml
{
struct PData5604
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Filter

template<>
struct Validatable<Filter::Xml::PData5604>: mpl::true_
{
	static bool validate(const Filter::Xml::PData5604::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData5606

namespace Filter
{
namespace Xml
{
struct PData5606
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Filter

template<>
struct Validatable<Filter::Xml::PData5606>: mpl::true_
{
	static bool validate(const Filter::Xml::PData5606::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData5608

namespace Filter
{
namespace Xml
{
struct PData5608
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Filter

template<>
struct Validatable<Filter::Xml::PData5608>: mpl::true_
{
	static bool validate(const Filter::Xml::PData5608::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData5610

namespace Filter
{
namespace Xml
{
struct PData5610
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Filter

template<>
struct Validatable<Filter::Xml::PData5610>: mpl::true_
{
	static bool validate(const Filter::Xml::PData5610::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData5612

namespace Filter
{
namespace Xml
{
struct PData5612
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Filter

template<>
struct Validatable<Filter::Xml::PData5612>: mpl::true_
{
	static bool validate(const Filter::Xml::PData5612::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData5614

namespace Filter
{
namespace Xml
{
struct PData5614
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Filter

template<>
struct Validatable<Filter::Xml::PData5614>: mpl::true_
{
	static bool validate(const Filter::Xml::PData5614::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData5616

namespace Filter
{
namespace Xml
{
struct PData5616
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Filter

template<>
struct Validatable<Filter::Xml::PData5616>: mpl::true_
{
	static bool validate(const Filter::Xml::PData5616::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PFilter

namespace Filter
{
namespace Xml
{
struct PFilter
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Filter


///////////////////////////////////////////////////////////////////////////////
// struct PVariableNameType

namespace Filter
{
namespace Xml
{
struct PVariableNameType
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Filter

template<>
struct Validatable<Filter::Xml::PVariableNameType>: mpl::true_
{
	static bool validate(const Filter::Xml::PVariableNameType::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData5619

namespace Filter
{
namespace Xml
{
struct PData5619
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Filter

template<>
struct Validatable<Filter::Xml::PData5619>: mpl::true_
{
	static bool validate(const Filter::Xml::PData5619::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData5622

namespace Filter
{
namespace Xml
{
struct PData5622
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Filter

template<>
struct Validatable<Filter::Xml::PData5622>: mpl::true_
{
	static bool validate(const Filter::Xml::PData5622::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData5625

namespace Filter
{
namespace Xml
{
struct PData5625
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Filter

template<>
struct Validatable<Filter::Xml::PData5625>: mpl::true_
{
	static bool validate(const Filter::Xml::PData5625::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData5628

namespace Filter
{
namespace Xml
{
struct PData5628
{
	typedef qint32 value_type;
};

} // namespace Xml
} // namespace Filter

template<>
struct Traits<Filter::Xml::PData5628>
{
	static bool parse(const QString& src_, Filter::Xml::PData5628::value_type& dst_);

	static QString generate(Filter::Xml::PData5628::value_type src_);

};

template<>
struct Validatable<Filter::Xml::PData5628>: mpl::true_
{
	static bool validate(Filter::Xml::PData5628::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData5630

namespace Filter
{
namespace Xml
{
struct PData5630
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Filter

template<>
struct Validatable<Filter::Xml::PData5630>: mpl::true_
{
	static bool validate(const Filter::Xml::PData5630::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData5633

namespace Filter
{
namespace Xml
{
struct PData5633
{
	typedef qint32 value_type;
};

} // namespace Xml
} // namespace Filter

template<>
struct Traits<Filter::Xml::PData5633>
{
	static bool parse(const QString& src_, Filter::Xml::PData5633::value_type& dst_);

	static QString generate(Filter::Xml::PData5633::value_type src_);

};

template<>
struct Validatable<Filter::Xml::PData5633>: mpl::true_
{
	static bool validate(Filter::Xml::PData5633::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData5635

namespace Filter
{
namespace Xml
{
struct PData5635
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Filter

template<>
struct Validatable<Filter::Xml::PData5635>: mpl::true_
{
	static bool validate(const Filter::Xml::PData5635::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData5637

namespace Filter
{
namespace Xml
{
struct PData5637
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Filter

template<>
struct Validatable<Filter::Xml::PData5637>: mpl::true_
{
	static bool validate(const Filter::Xml::PData5637::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData5640

namespace Filter
{
namespace Xml
{
struct PData5640
{
	typedef qint32 value_type;
};

} // namespace Xml
} // namespace Filter

template<>
struct Traits<Filter::Xml::PData5640>
{
	static bool parse(const QString& src_, Filter::Xml::PData5640::value_type& dst_);

	static QString generate(Filter::Xml::PData5640::value_type src_);

};

template<>
struct Validatable<Filter::Xml::PData5640>: mpl::true_
{
	static bool validate(Filter::Xml::PData5640::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData5643

namespace Filter
{
namespace Xml
{
struct PData5643
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Filter

template<>
struct Validatable<Filter::Xml::PData5643>: mpl::true_
{
	static bool validate(const Filter::Xml::PData5643::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData5645

namespace Filter
{
namespace Xml
{
struct PData5645
{
	typedef qint32 value_type;
};

} // namespace Xml
} // namespace Filter

template<>
struct Traits<Filter::Xml::PData5645>
{
	static bool parse(const QString& src_, Filter::Xml::PData5645::value_type& dst_);

	static QString generate(Filter::Xml::PData5645::value_type src_);

};

template<>
struct Validatable<Filter::Xml::PData5645>: mpl::true_
{
	static bool validate(Filter::Xml::PData5645::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData5650

namespace Filter
{
namespace Xml
{
struct PData5650
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Filter

template<>
struct Validatable<Filter::Xml::PData5650>: mpl::true_
{
	static bool validate(const Filter::Xml::PData5650::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData5652

namespace Filter
{
namespace Xml
{
struct PData5652
{
	typedef qint32 value_type;
};

} // namespace Xml
} // namespace Filter

template<>
struct Traits<Filter::Xml::PData5652>
{
	static bool parse(const QString& src_, Filter::Xml::PData5652::value_type& dst_);

	static QString generate(Filter::Xml::PData5652::value_type src_);

};

template<>
struct Validatable<Filter::Xml::PData5652>: mpl::true_
{
	static bool validate(Filter::Xml::PData5652::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData5655

namespace Filter
{
namespace Xml
{
struct PData5655
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Filter

template<>
struct Validatable<Filter::Xml::PData5655>: mpl::true_
{
	static bool validate(const Filter::Xml::PData5655::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData5657

namespace Filter
{
namespace Xml
{
struct PData5657
{
	typedef qint32 value_type;
};

} // namespace Xml
} // namespace Filter

template<>
struct Traits<Filter::Xml::PData5657>
{
	static bool parse(const QString& src_, Filter::Xml::PData5657::value_type& dst_);

	static QString generate(Filter::Xml::PData5657::value_type src_);

};

template<>
struct Validatable<Filter::Xml::PData5657>: mpl::true_
{
	static bool validate(Filter::Xml::PData5657::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData5660

namespace Filter
{
namespace Xml
{
struct PData5660
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Filter

template<>
struct Validatable<Filter::Xml::PData5660>: mpl::true_
{
	static bool validate(const Filter::Xml::PData5660::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData5662

namespace Filter
{
namespace Xml
{
struct PData5662
{
	typedef quint32 value_type;
};

} // namespace Xml
} // namespace Filter

template<>
struct Traits<Filter::Xml::PData5662>
{
	static bool parse(const QString& src_, Filter::Xml::PData5662::value_type& dst_);

	static QString generate(Filter::Xml::PData5662::value_type src_);

};


///////////////////////////////////////////////////////////////////////////////
// struct PData5665

namespace Filter
{
namespace Xml
{
struct PData5665
{
	typedef qint32 value_type;
};

} // namespace Xml
} // namespace Filter

template<>
struct Traits<Filter::Xml::PData5665>
{
	static bool parse(const QString& src_, Filter::Xml::PData5665::value_type& dst_);

	static QString generate(Filter::Xml::PData5665::value_type src_);

};

template<>
struct Validatable<Filter::Xml::PData5665>: mpl::true_
{
	static bool validate(Filter::Xml::PData5665::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData5667

namespace Filter
{
namespace Xml
{
struct PData5667
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Filter

template<>
struct Validatable<Filter::Xml::PData5667>: mpl::true_
{
	static bool validate(const Filter::Xml::PData5667::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData5670

namespace Filter
{
namespace Xml
{
struct PData5670
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Filter

template<>
struct Validatable<Filter::Xml::PData5670>: mpl::true_
{
	static bool validate(const Filter::Xml::PData5670::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData5672

namespace Filter
{
namespace Xml
{
struct PData5672
{
	typedef qint32 value_type;
};

} // namespace Xml
} // namespace Filter

template<>
struct Traits<Filter::Xml::PData5672>
{
	static bool parse(const QString& src_, Filter::Xml::PData5672::value_type& dst_);

	static QString generate(Filter::Xml::PData5672::value_type src_);

};

template<>
struct Validatable<Filter::Xml::PData5672>: mpl::true_
{
	static bool validate(Filter::Xml::PData5672::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PFilterParamName

namespace Filter
{
namespace Xml
{
struct PFilterParamName
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Filter

template<>
struct Validatable<Filter::Xml::PFilterParamName>: mpl::true_
{
	static bool validate(const Filter::Xml::PFilterParamName::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PFilterParamValue

namespace Filter
{
namespace Xml
{
struct PFilterParamValue
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Filter

template<>
struct Validatable<Filter::Xml::PFilterParamValue>: mpl::true_
{
	static bool validate(const Filter::Xml::PFilterParamValue::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PPriorityType

namespace Filter
{
namespace Xml
{
struct PPriorityType
{
	typedef qint32 value_type;
};

} // namespace Xml
} // namespace Filter

template<>
struct Traits<Filter::Xml::PPriorityType>
{
	static bool parse(const QString& src_, Filter::Xml::PPriorityType::value_type& dst_);

	static QString generate(Filter::Xml::PPriorityType::value_type src_);

};

template<>
struct Validatable<Filter::Xml::PPriorityType>: mpl::true_
{
	static bool validate(Filter::Xml::PPriorityType::value_type value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PStatematchType

namespace Filter
{
namespace Xml
{
struct PStatematchType
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Filter

template<>
struct Validatable<Filter::Xml::PStatematchType>: mpl::true_
{
	static bool validate(const Filter::Xml::PStatematchType::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PCommentType

namespace Filter
{
namespace Xml
{
struct PCommentType
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Filter


///////////////////////////////////////////////////////////////////////////////
// struct PStateflagsType

namespace Filter
{
namespace Xml
{
struct PStateflagsType
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Filter

template<>
struct Validatable<Filter::Xml::PStateflagsType>: mpl::true_
{
	static bool validate(const Filter::Xml::PStateflagsType::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PTcpflagsType

namespace Filter
{
namespace Xml
{
struct PTcpflagsType
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Filter

template<>
struct Validatable<Filter::Xml::PTcpflagsType>: mpl::true_
{
	static bool validate(const Filter::Xml::PTcpflagsType::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PData5677

namespace Filter
{
namespace Xml
{
struct PData5677
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Filter

template<>
struct Validatable<Filter::Xml::PData5677>: mpl::true_
{
	static bool validate(const Filter::Xml::PData5677::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct PIpsetFlagsType

namespace Filter
{
namespace Xml
{
struct PIpsetFlagsType
{
	typedef QString value_type;
};

} // namespace Xml
} // namespace Filter

template<>
struct Validatable<Filter::Xml::PIpsetFlagsType>: mpl::true_
{
	static bool validate(const Filter::Xml::PIpsetFlagsType::value_type& value_);

};

///////////////////////////////////////////////////////////////////////////////
// struct VChain

namespace Filter
{
namespace Xml
{
typedef Choice<mpl::vector<EChain, PData5604, PData5606, PData5608, PData5610, PData5612, PData5614, PData5616 > > VChainImpl;
typedef VChainImpl::value_type VChain;

} // namespace Xml
} // namespace Filter

template<>
struct Traits<Filter::Xml::VChain>
{
	static bool parse(const QString& src_, Filter::Xml::VChain& dst_);

	static QString generate(const Filter::Xml::VChain& src_);

};

///////////////////////////////////////////////////////////////////////////////
// struct VUUID

namespace Filter
{
namespace Xml
{
typedef Choice<mpl::vector<PData5579, PData5581 > > VUUIDImpl;
typedef VUUIDImpl::value_type VUUID;

} // namespace Xml
} // namespace Filter

template<>
struct Traits<Filter::Xml::VUUID>
{
	static bool parse(const QString& src_, Filter::Xml::VUUID& dst_);

	static QString generate(const Filter::Xml::VUUID& src_);

};

///////////////////////////////////////////////////////////////////////////////
// struct VAddrMAC

namespace Filter
{
namespace Xml
{
typedef Choice<mpl::vector<PVariableNameType, PData5619 > > VAddrMACImpl;
typedef VAddrMACImpl::value_type VAddrMAC;

} // namespace Xml
} // namespace Filter

template<>
struct Traits<Filter::Xml::VAddrMAC>
{
	static bool parse(const QString& src_, Filter::Xml::VAddrMAC& dst_);

	static QString generate(const Filter::Xml::VAddrMAC& src_);

};

///////////////////////////////////////////////////////////////////////////////
// struct VMacProtocolid

namespace Filter
{
namespace Xml
{
typedef Choice<mpl::vector<PVariableNameType, PData5643, PData5645, EChoice5647 > > VMacProtocolidImpl;
typedef VMacProtocolidImpl::value_type VMacProtocolid;

} // namespace Xml
} // namespace Filter

template<>
struct Traits<Filter::Xml::VMacProtocolid>
{
	static bool parse(const QString& src_, Filter::Xml::VMacProtocolid& dst_);

	static QString generate(const Filter::Xml::VMacProtocolid& src_);

};

///////////////////////////////////////////////////////////////////////////////
// struct VVlanVlanid

namespace Filter
{
namespace Xml
{
typedef Choice<mpl::vector<PVariableNameType, PData5650, PData5652 > > VVlanVlanidImpl;
typedef VVlanVlanidImpl::value_type VVlanVlanid;

} // namespace Xml
} // namespace Filter

template<>
struct Traits<Filter::Xml::VVlanVlanid>
{
	static bool parse(const QString& src_, Filter::Xml::VVlanVlanid& dst_);

	static QString generate(const Filter::Xml::VVlanVlanid& src_);

};

///////////////////////////////////////////////////////////////////////////////
// struct VUint8range

namespace Filter
{
namespace Xml
{
typedef Choice<mpl::vector<PData5571, PData5573 > > VUint8rangeImpl;
typedef VUint8rangeImpl::value_type VUint8range;

} // namespace Xml
} // namespace Filter

template<>
struct Traits<Filter::Xml::VUint8range>
{
	static bool parse(const QString& src_, Filter::Xml::VUint8range& dst_);

	static QString generate(const Filter::Xml::VUint8range& src_);

};

///////////////////////////////////////////////////////////////////////////////
// struct VUint16range

namespace Filter
{
namespace Xml
{
typedef Choice<mpl::vector<PVariableNameType, PData5655, PData5657 > > VUint16rangeImpl;
typedef VUint16rangeImpl::value_type VUint16range;

} // namespace Xml
} // namespace Filter

template<>
struct Traits<Filter::Xml::VUint16range>
{
	static bool parse(const QString& src_, Filter::Xml::VUint16range& dst_);

	static QString generate(const Filter::Xml::VUint16range& src_);

};

///////////////////////////////////////////////////////////////////////////////
// struct VUint32range

namespace Filter
{
namespace Xml
{
typedef Choice<mpl::vector<PVariableNameType, PData5660, PData5662 > > VUint32rangeImpl;
typedef VUint32rangeImpl::value_type VUint32range;

} // namespace Xml
} // namespace Filter

template<>
struct Traits<Filter::Xml::VUint32range>
{
	static bool parse(const QString& src_, Filter::Xml::VUint32range& dst_);

	static QString generate(const Filter::Xml::VUint32range& src_);

};

///////////////////////////////////////////////////////////////////////////////
// struct VAddrIP

namespace Filter
{
namespace Xml
{
typedef Choice<mpl::vector<PVariableNameType, PData5622 > > VAddrIPImpl;
typedef VAddrIPImpl::value_type VAddrIP;

} // namespace Xml
} // namespace Filter

template<>
struct Traits<Filter::Xml::VAddrIP>
{
	static bool parse(const QString& src_, Filter::Xml::VAddrIP& dst_);

	static QString generate(const Filter::Xml::VAddrIP& src_);

};

///////////////////////////////////////////////////////////////////////////////
// struct VArpOpcodeType

namespace Filter
{
namespace Xml
{
typedef Choice<mpl::vector<PVariableNameType, PData5665, PData5667 > > VArpOpcodeTypeImpl;
typedef VArpOpcodeTypeImpl::value_type VArpOpcodeType;

} // namespace Xml
} // namespace Filter

template<>
struct Traits<Filter::Xml::VArpOpcodeType>
{
	static bool parse(const QString& src_, Filter::Xml::VArpOpcodeType& dst_);

	static QString generate(const Filter::Xml::VArpOpcodeType& src_);

};

///////////////////////////////////////////////////////////////////////////////
// struct VAddrMask

namespace Filter
{
namespace Xml
{
typedef Choice<mpl::vector<PVariableNameType, PData5628, PData5630 > > VAddrMaskImpl;
typedef VAddrMaskImpl::value_type VAddrMask;

} // namespace Xml
} // namespace Filter

template<>
struct Traits<Filter::Xml::VAddrMask>
{
	static bool parse(const QString& src_, Filter::Xml::VAddrMask& dst_);

	static QString generate(const Filter::Xml::VAddrMask& src_);

};

///////////////////////////////////////////////////////////////////////////////
// struct VIpProtocolType

namespace Filter
{
namespace Xml
{
typedef Choice<mpl::vector<PVariableNameType, PData5670, PData5672, EChoice5674 > > VIpProtocolTypeImpl;
typedef VIpProtocolTypeImpl::value_type VIpProtocolType;

} // namespace Xml
} // namespace Filter

template<>
struct Traits<Filter::Xml::VIpProtocolType>
{
	static bool parse(const QString& src_, Filter::Xml::VIpProtocolType& dst_);

	static QString generate(const Filter::Xml::VIpProtocolType& src_);

};

///////////////////////////////////////////////////////////////////////////////
// struct VSixbitrange

namespace Filter
{
namespace Xml
{
typedef Choice<mpl::vector<PData5637, PVariableNameType, PData5640 > > VSixbitrangeImpl;
typedef VSixbitrangeImpl::value_type VSixbitrange;

} // namespace Xml
} // namespace Filter

template<>
struct Traits<Filter::Xml::VSixbitrange>
{
	static bool parse(const QString& src_, Filter::Xml::VSixbitrange& dst_);

	static QString generate(const Filter::Xml::VSixbitrange& src_);

};

///////////////////////////////////////////////////////////////////////////////
// struct VAddrIPv6

namespace Filter
{
namespace Xml
{
typedef Choice<mpl::vector<PVariableNameType, PData5625 > > VAddrIPv6Impl;
typedef VAddrIPv6Impl::value_type VAddrIPv6;

} // namespace Xml
} // namespace Filter

template<>
struct Traits<Filter::Xml::VAddrIPv6>
{
	static bool parse(const QString& src_, Filter::Xml::VAddrIPv6& dst_);

	static QString generate(const Filter::Xml::VAddrIPv6& src_);

};

///////////////////////////////////////////////////////////////////////////////
// struct VAddrMaskv6

namespace Filter
{
namespace Xml
{
typedef Choice<mpl::vector<PVariableNameType, PData5633, PData5635 > > VAddrMaskv6Impl;
typedef VAddrMaskv6Impl::value_type VAddrMaskv6;

} // namespace Xml
} // namespace Filter

template<>
struct Traits<Filter::Xml::VAddrMaskv6>
{
	static bool parse(const QString& src_, Filter::Xml::VAddrMaskv6& dst_);

	static QString generate(const Filter::Xml::VAddrMaskv6& src_);

};

///////////////////////////////////////////////////////////////////////////////
// struct VIpsetNameType

namespace Filter
{
namespace Xml
{
typedef Choice<mpl::vector<PVariableNameType, PData5677 > > VIpsetNameTypeImpl;
typedef VIpsetNameTypeImpl::value_type VIpsetNameType;

} // namespace Xml
} // namespace Filter

template<>
struct Traits<Filter::Xml::VIpsetNameType>
{
	static bool parse(const QString& src_, Filter::Xml::VIpsetNameType& dst_);

	static QString generate(const Filter::Xml::VIpsetNameType& src_);

};

} // namespace Libvirt

#endif // __FILTER_DATA_H__
