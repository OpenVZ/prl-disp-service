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

#include "iface_data.h"
#include <QRegExp>

namespace Libvirt
{
///////////////////////////////////////////////////////////////////////////////
// struct PUnsignedInt

bool Traits<Iface::Xml::PUnsignedInt>::parse(const QString& src_, Iface::Xml::PUnsignedInt::value_type& dst_)
{
	QRegExp q("[0-9]+");
	if (!q.exactMatch(src_))
		return false;
	bool output = false;
	dst_ = src_.toUInt(&output);
	return output;
}

QString Traits<Iface::Xml::PUnsignedInt>::generate(Iface::Xml::PUnsignedInt::value_type src_)
{
	return QString::number(src_);
}

///////////////////////////////////////////////////////////////////////////////
// struct PUnsignedLong

bool Traits<Iface::Xml::PUnsignedLong>::parse(const QString& src_, Iface::Xml::PUnsignedLong::value_type& dst_)
{
	QRegExp q("[0-9]+");
	if (!q.exactMatch(src_))
		return false;
	bool output = false;
	dst_ = src_.toULong(&output);
	return output;
}

QString Traits<Iface::Xml::PUnsignedLong>::generate(Iface::Xml::PUnsignedLong::value_type src_)
{
	return QString::number(src_);
}

///////////////////////////////////////////////////////////////////////////////
// struct PHexuint

bool Validatable<Iface::Xml::PHexuint>::validate(const Iface::Xml::PHexuint::value_type& value_)
{
	QRegExp q("(0x)?[0-9a-f]+");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PPositiveInteger

bool Traits<Iface::Xml::PPositiveInteger>::parse(const QString& src_, Iface::Xml::PPositiveInteger::value_type& dst_)
{
	QRegExp q("[0-9]+");
	if (!q.exactMatch(src_))
		return false;
	bool output = false;
	dst_ = src_.toUInt(&output);
	return output;
}

QString Traits<Iface::Xml::PPositiveInteger>::generate(Iface::Xml::PPositiveInteger::value_type src_)
{
	return QString::number(src_);
}

///////////////////////////////////////////////////////////////////////////////
// struct POctalMode

bool Traits<Iface::Xml::POctalMode>::parse(const QString& src_, Iface::Xml::POctalMode::value_type& dst_)
{
	QRegExp q("[0-7]+");
	if (!q.exactMatch(src_))
		return false;
	bool output = false;
	dst_ = src_.toUInt(&output);
	return output;
}

QString Traits<Iface::Xml::POctalMode>::generate(Iface::Xml::POctalMode::value_type src_)
{
	return QString::number(src_);
}

///////////////////////////////////////////////////////////////////////////////
// struct PData7137

bool Validatable<Iface::Xml::PData7137>::validate(const Iface::Xml::PData7137::value_type& value_)
{
	QRegExp q("0x[0-9a-fA-F]{1,2}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PData3976

bool Traits<Iface::Xml::PData3976>::parse(const QString& src_, Iface::Xml::PData3976::value_type& dst_)
{
	bool output = false;
	dst_ = src_.toInt(&output);
	return output;
}

QString Traits<Iface::Xml::PData3976>::generate(Iface::Xml::PData3976::value_type src_)
{
	return QString::number(src_);
}

bool Validatable<Iface::Xml::PData3976>::validate(Iface::Xml::PData3976::value_type value_)
{
	if (0 > value_)
		return false;

	if (255 < value_)
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PData3978

bool Validatable<Iface::Xml::PData3978>::validate(const Iface::Xml::PData3978::value_type& value_)
{
	QRegExp q("0x[0-9a-fA-F]{1,2}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PData3980

bool Traits<Iface::Xml::PData3980>::parse(const QString& src_, Iface::Xml::PData3980::value_type& dst_)
{
	bool output = false;
	dst_ = src_.toInt(&output);
	return output;
}

QString Traits<Iface::Xml::PData3980>::generate(Iface::Xml::PData3980::value_type src_)
{
	return QString::number(src_);
}

bool Validatable<Iface::Xml::PData3980>::validate(Iface::Xml::PData3980::value_type value_)
{
	if (0 > value_)
		return false;

	if (255 < value_)
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PData3982

bool Validatable<Iface::Xml::PData3982>::validate(const Iface::Xml::PData3982::value_type& value_)
{
	QRegExp q("(0x)?[0-9a-fA-F]{1,4}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PData7139

bool Traits<Iface::Xml::PData7139>::parse(const QString& src_, Iface::Xml::PData7139::value_type& dst_)
{
	bool output = false;
	dst_ = src_.toInt(&output);
	return output;
}

QString Traits<Iface::Xml::PData7139>::generate(Iface::Xml::PData7139::value_type src_)
{
	return QString::number(src_);
}

bool Validatable<Iface::Xml::PData7139>::validate(Iface::Xml::PData7139::value_type value_)
{
	if (0 > value_)
		return false;

	if (65535 < value_)
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PData7141

bool Validatable<Iface::Xml::PData7141>::validate(const Iface::Xml::PData7141::value_type& value_)
{
	QRegExp q("0x[0-9a-fA-F]{1,6}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PData3984

bool Traits<Iface::Xml::PData3984>::parse(const QString& src_, Iface::Xml::PData3984::value_type& dst_)
{
	bool output = false;
	dst_ = src_.toInt(&output);
	return output;
}

QString Traits<Iface::Xml::PData3984>::generate(Iface::Xml::PData3984::value_type src_)
{
	return QString::number(src_);
}

bool Validatable<Iface::Xml::PData3984>::validate(Iface::Xml::PData3984::value_type value_)
{
	if (0 > value_)
		return false;

	if (16777215 < value_)
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PData3986

bool Validatable<Iface::Xml::PData3986>::validate(const Iface::Xml::PData3986::value_type& value_)
{
	QRegExp q("0x[0-9a-fA-F]{1,6}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PData7143

bool Traits<Iface::Xml::PData7143>::parse(const QString& src_, Iface::Xml::PData7143::value_type& dst_)
{
	bool output = false;
	dst_ = src_.toInt(&output);
	return output;
}

QString Traits<Iface::Xml::PData7143>::generate(Iface::Xml::PData7143::value_type src_)
{
	return QString::number(src_);
}

bool Validatable<Iface::Xml::PData7143>::validate(Iface::Xml::PData7143::value_type value_)
{
	if (0 > value_)
		return false;

	if (16777215 < value_)
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PData7145

bool Validatable<Iface::Xml::PData7145>::validate(const Iface::Xml::PData7145::value_type& value_)
{
	QRegExp q("(0x)?[0-9a-fA-F]{1,8}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PData7147

bool Traits<Iface::Xml::PData7147>::parse(const QString& src_, Iface::Xml::PData7147::value_type& dst_)
{
	bool output = false;
	dst_ = src_.toUInt(&output);
	return output;
}

QString Traits<Iface::Xml::PData7147>::generate(Iface::Xml::PData7147::value_type src_)
{
	return QString::number(src_);
}

///////////////////////////////////////////////////////////////////////////////
// struct PData7149

bool Validatable<Iface::Xml::PData7149>::validate(const Iface::Xml::PData7149::value_type& value_)
{
	QRegExp q("[a-fA-F0-9]{32}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PData3988

bool Validatable<Iface::Xml::PData3988>::validate(const Iface::Xml::PData3988::value_type& value_)
{
	QRegExp q("[a-fA-F0-9]{8}\\-([a-fA-F0-9]{4}\\-){3}[a-fA-F0-9]{12}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PNumaDistanceValue

bool Traits<Iface::Xml::PNumaDistanceValue>::parse(const QString& src_, Iface::Xml::PNumaDistanceValue::value_type& dst_)
{
	bool output = false;
	dst_ = src_.toUInt(&output);
	return output;
}

QString Traits<Iface::Xml::PNumaDistanceValue>::generate(Iface::Xml::PNumaDistanceValue::value_type src_)
{
	return QString::number(src_);
}

bool Validatable<Iface::Xml::PNumaDistanceValue>::validate(Iface::Xml::PNumaDistanceValue::value_type value_)
{
	if (10 > value_)
		return false;

	if (255 < value_)
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PUniMacAddr

bool Validatable<Iface::Xml::PUniMacAddr>::validate(const Iface::Xml::PUniMacAddr::value_type& value_)
{
	QRegExp q("[a-fA-F0-9][02468aAcCeE](:[a-fA-F0-9]{2}){5}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PMultiMacAddr

bool Validatable<Iface::Xml::PMultiMacAddr>::validate(const Iface::Xml::PMultiMacAddr::value_type& value_)
{
	QRegExp q("[a-fA-F0-9][13579bBdDfF](:[a-fA-F0-9]{2}){5}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PMacAddr

bool Validatable<Iface::Xml::PMacAddr>::validate(const Iface::Xml::PMacAddr::value_type& value_)
{
	QRegExp q("[a-fA-F0-9]{2}(:[a-fA-F0-9]{2}){5}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PDuidLLT

bool Validatable<Iface::Xml::PDuidLLT>::validate(const Iface::Xml::PDuidLLT::value_type& value_)
{
	QRegExp q("[0]{1,2}:[0]{0,1}[1]:[0]{1,2}:[0]{0,1}[a-fA-F1-9](:[a-fA-F0-9]{1,2}){4}(:[a-fA-F0-9]{1,2}){6,8}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PDuidEN

bool Validatable<Iface::Xml::PDuidEN>::validate(const Iface::Xml::PDuidEN::value_type& value_)
{
	QRegExp q("[0]{1,2}:[0]{0,1}[2](:[a-fA-F0-9]{1,2}){4}(:[a-fA-F0-9]{1,2}){1,124}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PDuidLL

bool Validatable<Iface::Xml::PDuidLL>::validate(const Iface::Xml::PDuidLL::value_type& value_)
{
	QRegExp q("[0]{1,2}:[0]{0,1}[3]:[0]{1,2}:[0]{0,1}[a-fA-F1-9](:[a-fA-F0-9]{1,2}){6,8}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PDuidUUID

bool Validatable<Iface::Xml::PDuidUUID>::validate(const Iface::Xml::PDuidUUID::value_type& value_)
{
	QRegExp q("[0]{1,2}:[0]{0,1}[4](:[a-fA-F0-9]{1,2}){16}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PIpv4Addr

bool Validatable<Iface::Xml::PIpv4Addr>::validate(const Iface::Xml::PIpv4Addr::value_type& value_)
{
	QRegExp q("(((25[0-5])|(2[0-4][0-9])|(1[0-9]{2})|([1-9][0-9])|([0-9]))\\.){3}((25[0-5])|(2[0-4][0-9])|(1[0-9]{2})|([1-9][0-9])|([0-9]))");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PIpv6Addr

bool Validatable<Iface::Xml::PIpv6Addr>::validate(const Iface::Xml::PIpv6Addr::value_type& value_)
{
	QRegExp q("(([0-9A-Fa-f]{1,4}:){7}[0-9A-Fa-f]{1,4})|(([0-9A-Fa-f]{1,4}:){6}:[0-9A-Fa-f]{1,4})|(([0-9A-Fa-f]{1,4}:){5}:([0-9A-Fa-f]{1,4}:)?[0-9A-Fa-f]{1,4})|(([0-9A-Fa-f]{1,4}:){4}:([0-9A-Fa-f]{1,4}:){0,2}[0-9A-Fa-f]{1,4})|(([0-9A-Fa-f]{1,4}:){3}:([0-9A-Fa-f]{1,4}:){0,3}[0-9A-Fa-f]{1,4})|(([0-9A-Fa-f]{1,4}:){2}:([0-9A-Fa-f]{1,4}:){0,4}[0-9A-Fa-f]{1,4})|(([0-9A-Fa-f]{1,4}:){6}(((25[0-5])|(2[0-4][0-9])|(1[0-9]{2})|([1-9][0-9])|([0-9]))\\.){3}((25[0-5])|(2[0-4][0-9])|(1[0-9]{2})|([1-9][0-9])|([0-9])))|(([0-9A-Fa-f]{1,4}:){0,5}:(((25[0-5])|(2[0-4][0-9])|(1[0-9]{2})|([1-9][0-9])|([0-9]))\\.){3}((25[0-5])|(2[0-4][0-9])|(1[0-9]{2})|([1-9][0-9])|([0-9])))|(::([0-9A-Fa-f]{1,4}:){0,5}(((25[0-5])|(2[0-4][0-9])|(1[0-9]{2})|([1-9][0-9])|([0-9]))\\.){3}((25[0-5])|(2[0-4][0-9])|(1[0-9]{2})|([1-9][0-9])|([0-9])))|([0-9A-Fa-f]{1,4}::([0-9A-Fa-f]{1,4}:){0,5}[0-9A-Fa-f]{1,4})|(::([0-9A-Fa-f]{1,4}:){0,6}[0-9A-Fa-f]{1,4})|(([0-9A-Fa-f]{1,4}:){1,7}:)|(::)");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PIpv4Prefix

bool Traits<Iface::Xml::PIpv4Prefix>::parse(const QString& src_, Iface::Xml::PIpv4Prefix::value_type& dst_)
{
	bool output = false;
	dst_ = src_.toUInt(&output);
	return output;
}

QString Traits<Iface::Xml::PIpv4Prefix>::generate(Iface::Xml::PIpv4Prefix::value_type src_)
{
	return QString::number(src_);
}

bool Validatable<Iface::Xml::PIpv4Prefix>::validate(Iface::Xml::PIpv4Prefix::value_type value_)
{
	if (32 < value_)
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PIpv6Prefix

bool Traits<Iface::Xml::PIpv6Prefix>::parse(const QString& src_, Iface::Xml::PIpv6Prefix::value_type& dst_)
{
	bool output = false;
	dst_ = src_.toUInt(&output);
	return output;
}

QString Traits<Iface::Xml::PIpv6Prefix>::generate(Iface::Xml::PIpv6Prefix::value_type src_)
{
	return QString::number(src_);
}

bool Validatable<Iface::Xml::PIpv6Prefix>::validate(Iface::Xml::PIpv6Prefix::value_type value_)
{
	if (128 < value_)
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PObjectNameWithSlash

bool Validatable<Iface::Xml::PObjectNameWithSlash>::validate(const Iface::Xml::PObjectNameWithSlash::value_type& value_)
{
	QRegExp q("[^\\n]+");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PObjectName

bool Validatable<Iface::Xml::PObjectName>::validate(const Iface::Xml::PObjectName::value_type& value_)
{
	QRegExp q("[^/\\n]+");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PGenericName

bool Validatable<Iface::Xml::PGenericName>::validate(const Iface::Xml::PGenericName::value_type& value_)
{
	QRegExp q("[a-zA-Z0-9_\\+\\-]+");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PDnsName

bool Validatable<Iface::Xml::PDnsName>::validate(const Iface::Xml::PDnsName::value_type& value_)
{
	QRegExp q("[a-zA-Z0-9\\.\\-]+");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PDeviceName

bool Validatable<Iface::Xml::PDeviceName>::validate(const Iface::Xml::PDeviceName::value_type& value_)
{
	QRegExp q("[a-zA-Z0-9_\\.\\-\\\\:/]+");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PZoneName

bool Validatable<Iface::Xml::PZoneName>::validate(const Iface::Xml::PZoneName::value_type& value_)
{
	QRegExp q("[a-zA-Z0-9_\\-]+");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PFilePath

bool Validatable<Iface::Xml::PFilePath>::validate(const Iface::Xml::PFilePath::value_type& value_)
{
	QRegExp q(".+");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PDirPath

bool Validatable<Iface::Xml::PDirPath>::validate(const Iface::Xml::PDirPath::value_type& value_)
{
	QRegExp q(".+");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PAbsFilePath

bool Validatable<Iface::Xml::PAbsFilePath>::validate(const Iface::Xml::PAbsFilePath::value_type& value_)
{
	QRegExp q("(/|[a-zA-Z]:\\\\).+");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PVmwarePath

bool Validatable<Iface::Xml::PVmwarePath>::validate(const Iface::Xml::PVmwarePath::value_type& value_)
{
	QRegExp q("\\[[^\\]]+\\] .+");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PAbsDirPath

bool Validatable<Iface::Xml::PAbsDirPath>::validate(const Iface::Xml::PAbsDirPath::value_type& value_)
{
	QRegExp q("/.*");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PUnit

bool Validatable<Iface::Xml::PUnit>::validate(const Iface::Xml::PUnit::value_type& value_)
{
	QRegExp q("([bB]([yY][tT][eE][sS]?)?)|([kKmMgGtTpPeE]([iI]?[bB])?)");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PPciDomain

bool Validatable<Iface::Xml::PPciDomain>::validate(const Iface::Xml::PPciDomain::value_type& value_)
{
	QRegExp q("(0x)?[0-9a-fA-F]{1,4}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PPciBus

bool Validatable<Iface::Xml::PPciBus>::validate(const Iface::Xml::PPciBus::value_type& value_)
{
	QRegExp q("(0x)?[0-9a-fA-F]{1,2}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PPciSlot

bool Validatable<Iface::Xml::PPciSlot>::validate(const Iface::Xml::PPciSlot::value_type& value_)
{
	QRegExp q("(0x)?[0-1]?[0-9a-fA-F]");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PPciFunc

bool Validatable<Iface::Xml::PPciFunc>::validate(const Iface::Xml::PPciFunc::value_type& value_)
{
	QRegExp q("(0x)?[0-7]");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PWwn

bool Validatable<Iface::Xml::PWwn>::validate(const Iface::Xml::PWwn::value_type& value_)
{
	QRegExp q("(0x)?[0-9a-fA-F]{16}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PData3998

bool Validatable<Iface::Xml::PData3998>::validate(const Iface::Xml::PData3998::value_type& value_)
{
	QRegExp q("0x[0-9a-eA-E][0-9a-fA-F]?");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PData4777

bool Validatable<Iface::Xml::PData4777>::validate(const Iface::Xml::PData4777::value_type& value_)
{
	QRegExp q("0x[fF][0-9a-eA-E]?");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PData4779

bool Traits<Iface::Xml::PData4779>::parse(const QString& src_, Iface::Xml::PData4779::value_type& dst_)
{
	bool output = false;
	dst_ = src_.toInt(&output);
	return output;
}

QString Traits<Iface::Xml::PData4779>::generate(Iface::Xml::PData4779::value_type src_)
{
	return QString::number(src_);
}

bool Validatable<Iface::Xml::PData4779>::validate(Iface::Xml::PData4779::value_type value_)
{
	if (0 > value_)
		return false;

	if (254 < value_)
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PCcwSsidRange

bool Validatable<Iface::Xml::PCcwSsidRange>::validate(const Iface::Xml::PCcwSsidRange::value_type& value_)
{
	QRegExp q("(0x)?[0-3]");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PData4781

bool Validatable<Iface::Xml::PData4781>::validate(const Iface::Xml::PData4781::value_type& value_)
{
	QRegExp q("0x[0-9a-fA-F]{1,4}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PData4783

bool Traits<Iface::Xml::PData4783>::parse(const QString& src_, Iface::Xml::PData4783::value_type& dst_)
{
	bool output = false;
	dst_ = src_.toInt(&output);
	return output;
}

QString Traits<Iface::Xml::PData4783>::generate(Iface::Xml::PData4783::value_type src_)
{
	return QString::number(src_);
}

bool Validatable<Iface::Xml::PData4783>::validate(Iface::Xml::PData4783::value_type value_)
{
	if (0 > value_)
		return false;

	if (65535 < value_)
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PCpuset

bool Validatable<Iface::Xml::PCpuset>::validate(const Iface::Xml::PCpuset::value_type& value_)
{
	QRegExp q("([0-9]+(-[0-9]+)?|\\^[0-9]+)(,([0-9]+(-[0-9]+)?|\\^[0-9]+))*");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PVolName

bool Validatable<Iface::Xml::PVolName>::validate(const Iface::Xml::PVolName::value_type& value_)
{
	if ("\n        " == value_)
		return false;

	QRegExp q("[^/]+");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PPortNumber

bool Traits<Iface::Xml::PPortNumber>::parse(const QString& src_, Iface::Xml::PPortNumber::value_type& dst_)
{
	bool output = false;
	dst_ = src_.toInt(&output);
	return output;
}

QString Traits<Iface::Xml::PPortNumber>::generate(Iface::Xml::PPortNumber::value_type src_)
{
	return QString::number(src_);
}

bool Validatable<Iface::Xml::PPortNumber>::validate(Iface::Xml::PPortNumber::value_type value_)
{
	if (-1 > value_)
		return false;

	if (65535 < value_)
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PIobase

bool Validatable<Iface::Xml::PIobase>::validate(const Iface::Xml::PIobase::value_type& value_)
{
	QRegExp q("0x[a-fA-F0-9]{1,4}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PIrq

bool Validatable<Iface::Xml::PIrq>::validate(const Iface::Xml::PIrq::value_type& value_)
{
	QRegExp q("0x[a-fA-F0-9]");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PTimeval

bool Traits<Iface::Xml::PTimeval>::parse(const QString& src_, Iface::Xml::PTimeval::value_type& dst_)
{
	bool output = false;
	dst_ = src_.toDouble(&output);
	return output;
}

QString Traits<Iface::Xml::PTimeval>::generate(Iface::Xml::PTimeval::value_type src_)
{
	return QString::number(src_);
}

bool Validatable<Iface::Xml::PTimeval>::validate(Iface::Xml::PTimeval::value_type value_)
{
	if (0 > value_)
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PVlanId

bool Traits<Iface::Xml::PVlanId>::parse(const QString& src_, Iface::Xml::PVlanId::value_type& dst_)
{
	bool output = false;
	dst_ = src_.toUInt(&output);
	return output;
}

QString Traits<Iface::Xml::PVlanId>::generate(Iface::Xml::PVlanId::value_type src_)
{
	return QString::number(src_);
}

bool Validatable<Iface::Xml::PVlanId>::validate(Iface::Xml::PVlanId::value_type value_)
{
	if (4095 < value_)
		return false;

	return true;
}

} // namespace Libvirt
