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

#include "capability_data.h"
#include <QRegExp>

namespace Libvirt
{
///////////////////////////////////////////////////////////////////////////////
// struct PUnsignedInt

bool Traits<Capability::Xml::PUnsignedInt>::parse(const QString& src_, Capability::Xml::PUnsignedInt::value_type& dst_)
{
	QRegExp q("[0-9]+");
	if (!q.exactMatch(src_))
		return false;
	bool output = false;
	dst_ = src_.toUInt(&output);
	return output;
}

QString Traits<Capability::Xml::PUnsignedInt>::generate(Capability::Xml::PUnsignedInt::value_type src_)
{
	return QString::number(src_);
}

///////////////////////////////////////////////////////////////////////////////
// struct PUnsignedLong

bool Traits<Capability::Xml::PUnsignedLong>::parse(const QString& src_, Capability::Xml::PUnsignedLong::value_type& dst_)
{
	QRegExp q("[0-9]+");
	if (!q.exactMatch(src_))
		return false;
	bool output = false;
	dst_ = src_.toULong(&output);
	return output;
}

QString Traits<Capability::Xml::PUnsignedLong>::generate(Capability::Xml::PUnsignedLong::value_type src_)
{
	return QString::number(src_);
}

///////////////////////////////////////////////////////////////////////////////
// struct PHexuint

bool Validatable<Capability::Xml::PHexuint>::validate(const Capability::Xml::PHexuint::value_type& value_)
{
	QRegExp q("(0x)?[0-9a-f]+");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PPositiveInteger

bool Traits<Capability::Xml::PPositiveInteger>::parse(const QString& src_, Capability::Xml::PPositiveInteger::value_type& dst_)
{
	QRegExp q("[0-9]+");
	if (!q.exactMatch(src_))
		return false;
	bool output = false;
	dst_ = src_.toUInt(&output);
	return output;
}

QString Traits<Capability::Xml::PPositiveInteger>::generate(Capability::Xml::PPositiveInteger::value_type src_)
{
	return QString::number(src_);
}

///////////////////////////////////////////////////////////////////////////////
// struct POctalMode

bool Traits<Capability::Xml::POctalMode>::parse(const QString& src_, Capability::Xml::POctalMode::value_type& dst_)
{
	QRegExp q("[0-7]+");
	if (!q.exactMatch(src_))
		return false;
	bool output = false;
	dst_ = src_.toUInt(&output);
	return output;
}

QString Traits<Capability::Xml::POctalMode>::generate(Capability::Xml::POctalMode::value_type src_)
{
	return QString::number(src_);
}

///////////////////////////////////////////////////////////////////////////////
// struct PData2928

bool Validatable<Capability::Xml::PData2928>::validate(const Capability::Xml::PData2928::value_type& value_)
{
	QRegExp q("0x[0-9a-fA-F]{1,2}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PData2930

bool Traits<Capability::Xml::PData2930>::parse(const QString& src_, Capability::Xml::PData2930::value_type& dst_)
{
	bool output = false;
	dst_ = src_.toInt(&output);
	return output;
}

QString Traits<Capability::Xml::PData2930>::generate(Capability::Xml::PData2930::value_type src_)
{
	return QString::number(src_);
}

bool Validatable<Capability::Xml::PData2930>::validate(Capability::Xml::PData2930::value_type value_)
{
	if (0 > value_)
		return false;

	if (255 < value_)
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PData2932

bool Validatable<Capability::Xml::PData2932>::validate(const Capability::Xml::PData2932::value_type& value_)
{
	QRegExp q("0x[0-9a-fA-F]{1,2}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PData2934

bool Traits<Capability::Xml::PData2934>::parse(const QString& src_, Capability::Xml::PData2934::value_type& dst_)
{
	bool output = false;
	dst_ = src_.toInt(&output);
	return output;
}

QString Traits<Capability::Xml::PData2934>::generate(Capability::Xml::PData2934::value_type src_)
{
	return QString::number(src_);
}

bool Validatable<Capability::Xml::PData2934>::validate(Capability::Xml::PData2934::value_type value_)
{
	if (0 > value_)
		return false;

	if (255 < value_)
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PData2936

bool Validatable<Capability::Xml::PData2936>::validate(const Capability::Xml::PData2936::value_type& value_)
{
	QRegExp q("0x[0-9a-fA-F]{1,6}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PData2938

bool Traits<Capability::Xml::PData2938>::parse(const QString& src_, Capability::Xml::PData2938::value_type& dst_)
{
	bool output = false;
	dst_ = src_.toInt(&output);
	return output;
}

QString Traits<Capability::Xml::PData2938>::generate(Capability::Xml::PData2938::value_type src_)
{
	return QString::number(src_);
}

bool Validatable<Capability::Xml::PData2938>::validate(Capability::Xml::PData2938::value_type value_)
{
	if (0 > value_)
		return false;

	if (16777215 < value_)
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PData2940

bool Validatable<Capability::Xml::PData2940>::validate(const Capability::Xml::PData2940::value_type& value_)
{
	QRegExp q("[a-fA-F0-9]{32}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PData2942

bool Validatable<Capability::Xml::PData2942>::validate(const Capability::Xml::PData2942::value_type& value_)
{
	QRegExp q("[a-fA-F0-9]{8}\\-([a-fA-F0-9]{4}\\-){3}[a-fA-F0-9]{12}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PUniMacAddr

bool Validatable<Capability::Xml::PUniMacAddr>::validate(const Capability::Xml::PUniMacAddr::value_type& value_)
{
	QRegExp q("[a-fA-F0-9][02468aAcCeE](:[a-fA-F0-9]{2}){5}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PMultiMacAddr

bool Validatable<Capability::Xml::PMultiMacAddr>::validate(const Capability::Xml::PMultiMacAddr::value_type& value_)
{
	QRegExp q("[a-fA-F0-9][13579bBdDfF](:[a-fA-F0-9]{2}){5}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PMacAddr

bool Validatable<Capability::Xml::PMacAddr>::validate(const Capability::Xml::PMacAddr::value_type& value_)
{
	QRegExp q("[a-fA-F0-9]{2}(:[a-fA-F0-9]{2}){5}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PDuidLLT

bool Validatable<Capability::Xml::PDuidLLT>::validate(const Capability::Xml::PDuidLLT::value_type& value_)
{
	QRegExp q("[0]{1,2}:[0]{0,1}[1]:[0]{1,2}:[0]{0,1}[a-fA-F1-9](:[a-fA-F0-9]{1,2}){4}(:[a-fA-F0-9]{1,2}){6,8}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PDuidEN

bool Validatable<Capability::Xml::PDuidEN>::validate(const Capability::Xml::PDuidEN::value_type& value_)
{
	QRegExp q("[0]{1,2}:[0]{0,1}[2](:[a-fA-F0-9]{1,2}){4}(:[a-fA-F0-9]{1,2}){1,124}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PDuidLL

bool Validatable<Capability::Xml::PDuidLL>::validate(const Capability::Xml::PDuidLL::value_type& value_)
{
	QRegExp q("[0]{1,2}:[0]{0,1}[3]:[0]{1,2}:[0]{0,1}[a-fA-F1-9](:[a-fA-F0-9]{1,2}){6,8}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PDuidUUID

bool Validatable<Capability::Xml::PDuidUUID>::validate(const Capability::Xml::PDuidUUID::value_type& value_)
{
	QRegExp q("[0]{1,2}:[0]{0,1}[4](:[a-fA-F0-9]{1,2}){16}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PIpv4Addr

bool Validatable<Capability::Xml::PIpv4Addr>::validate(const Capability::Xml::PIpv4Addr::value_type& value_)
{
	QRegExp q("(((25[0-5])|(2[0-4][0-9])|(1[0-9]{2})|([1-9][0-9])|([0-9]))\\.){3}((25[0-5])|(2[0-4][0-9])|(1[0-9]{2})|([1-9][0-9])|([0-9]))");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PIpv6Addr

bool Validatable<Capability::Xml::PIpv6Addr>::validate(const Capability::Xml::PIpv6Addr::value_type& value_)
{
	QRegExp q("(([0-9A-Fa-f]{1,4}:){7}[0-9A-Fa-f]{1,4})|(([0-9A-Fa-f]{1,4}:){6}:[0-9A-Fa-f]{1,4})|(([0-9A-Fa-f]{1,4}:){5}:([0-9A-Fa-f]{1,4}:)?[0-9A-Fa-f]{1,4})|(([0-9A-Fa-f]{1,4}:){4}:([0-9A-Fa-f]{1,4}:){0,2}[0-9A-Fa-f]{1,4})|(([0-9A-Fa-f]{1,4}:){3}:([0-9A-Fa-f]{1,4}:){0,3}[0-9A-Fa-f]{1,4})|(([0-9A-Fa-f]{1,4}:){2}:([0-9A-Fa-f]{1,4}:){0,4}[0-9A-Fa-f]{1,4})|(([0-9A-Fa-f]{1,4}:){6}(((25[0-5])|(2[0-4][0-9])|(1[0-9]{2})|([1-9][0-9])|([0-9]))\\.){3}((25[0-5])|(2[0-4][0-9])|(1[0-9]{2})|([1-9][0-9])|([0-9])))|(([0-9A-Fa-f]{1,4}:){0,5}:(((25[0-5])|(2[0-4][0-9])|(1[0-9]{2})|([1-9][0-9])|([0-9]))\\.){3}((25[0-5])|(2[0-4][0-9])|(1[0-9]{2})|([1-9][0-9])|([0-9])))|(::([0-9A-Fa-f]{1,4}:){0,5}(((25[0-5])|(2[0-4][0-9])|(1[0-9]{2})|([1-9][0-9])|([0-9]))\\.){3}((25[0-5])|(2[0-4][0-9])|(1[0-9]{2})|([1-9][0-9])|([0-9])))|([0-9A-Fa-f]{1,4}::([0-9A-Fa-f]{1,4}:){0,5}[0-9A-Fa-f]{1,4})|(::([0-9A-Fa-f]{1,4}:){0,6}[0-9A-Fa-f]{1,4})|(([0-9A-Fa-f]{1,4}:){1,7}:)");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PIpv4Prefix

bool Traits<Capability::Xml::PIpv4Prefix>::parse(const QString& src_, Capability::Xml::PIpv4Prefix::value_type& dst_)
{
	bool output = false;
	dst_ = src_.toUInt(&output);
	return output;
}

QString Traits<Capability::Xml::PIpv4Prefix>::generate(Capability::Xml::PIpv4Prefix::value_type src_)
{
	return QString::number(src_);
}

bool Validatable<Capability::Xml::PIpv4Prefix>::validate(Capability::Xml::PIpv4Prefix::value_type value_)
{
	if (32 < value_)
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PIpv6Prefix

bool Traits<Capability::Xml::PIpv6Prefix>::parse(const QString& src_, Capability::Xml::PIpv6Prefix::value_type& dst_)
{
	bool output = false;
	dst_ = src_.toUInt(&output);
	return output;
}

QString Traits<Capability::Xml::PIpv6Prefix>::generate(Capability::Xml::PIpv6Prefix::value_type src_)
{
	return QString::number(src_);
}

bool Validatable<Capability::Xml::PIpv6Prefix>::validate(Capability::Xml::PIpv6Prefix::value_type value_)
{
	if (128 < value_)
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PGenericName

bool Validatable<Capability::Xml::PGenericName>::validate(const Capability::Xml::PGenericName::value_type& value_)
{
	QRegExp q("[a-zA-Z0-9_\\+\\-]+");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PDnsName

bool Validatable<Capability::Xml::PDnsName>::validate(const Capability::Xml::PDnsName::value_type& value_)
{
	QRegExp q("[a-zA-Z0-9\\.\\-]+");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PDeviceName

bool Validatable<Capability::Xml::PDeviceName>::validate(const Capability::Xml::PDeviceName::value_type& value_)
{
	QRegExp q("[a-zA-Z0-9_\\.\\-\\\\:/]+");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PFilePath

bool Validatable<Capability::Xml::PFilePath>::validate(const Capability::Xml::PFilePath::value_type& value_)
{
	QRegExp q("[a-zA-Z0-9_\\.\\+\\-\\\\&\"{}'<>/%: ]+");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PDirPath

bool Validatable<Capability::Xml::PDirPath>::validate(const Capability::Xml::PDirPath::value_type& value_)
{
	QRegExp q("[a-zA-Z0-9_\\.\\+\\-\\\\&\"{}'<>/%: ]+");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PAbsFilePath

bool Validatable<Capability::Xml::PAbsFilePath>::validate(const Capability::Xml::PAbsFilePath::value_type& value_)
{
	QRegExp q("/[a-zA-Z0-9_\\.\\+\\-\\\\&\"{}'<>/%,: ]+");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PAbsDirPath

bool Validatable<Capability::Xml::PAbsDirPath>::validate(const Capability::Xml::PAbsDirPath::value_type& value_)
{
	QRegExp q("/[a-zA-Z0-9_\\.\\+\\-\\\\&\"{}'<>/%: ]*");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PUnit

bool Validatable<Capability::Xml::PUnit>::validate(const Capability::Xml::PUnit::value_type& value_)
{
	QRegExp q("([bB]([yY][tT][eE][sS]?)?)|([kKmMgGtTpPeE]([iI]?[bB])?)");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PPciDomain

bool Validatable<Capability::Xml::PPciDomain>::validate(const Capability::Xml::PPciDomain::value_type& value_)
{
	QRegExp q("(0x)?[0-9a-fA-F]{1,4}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PPciBus

bool Validatable<Capability::Xml::PPciBus>::validate(const Capability::Xml::PPciBus::value_type& value_)
{
	QRegExp q("(0x)?[0-9a-fA-F]{1,2}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PPciSlot

bool Validatable<Capability::Xml::PPciSlot>::validate(const Capability::Xml::PPciSlot::value_type& value_)
{
	QRegExp q("(0x)?[0-1]?[0-9a-fA-F]");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PPciFunc

bool Validatable<Capability::Xml::PPciFunc>::validate(const Capability::Xml::PPciFunc::value_type& value_)
{
	QRegExp q("(0x)?[0-7]");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PWwn

bool Validatable<Capability::Xml::PWwn>::validate(const Capability::Xml::PWwn::value_type& value_)
{
	QRegExp q("(0x)?[0-9a-fA-F]{16}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PData2952

bool Validatable<Capability::Xml::PData2952>::validate(const Capability::Xml::PData2952::value_type& value_)
{
	QRegExp q("0x[0-9a-eA-E][0-9a-fA-F]?");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PData2954

bool Validatable<Capability::Xml::PData2954>::validate(const Capability::Xml::PData2954::value_type& value_)
{
	QRegExp q("0x[fF][0-9a-eA-E]?");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PData2956

bool Traits<Capability::Xml::PData2956>::parse(const QString& src_, Capability::Xml::PData2956::value_type& dst_)
{
	bool output = false;
	dst_ = src_.toInt(&output);
	return output;
}

QString Traits<Capability::Xml::PData2956>::generate(Capability::Xml::PData2956::value_type src_)
{
	return QString::number(src_);
}

bool Validatable<Capability::Xml::PData2956>::validate(Capability::Xml::PData2956::value_type value_)
{
	if (0 > value_)
		return false;

	if (254 < value_)
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PCcwSsidRange

bool Validatable<Capability::Xml::PCcwSsidRange>::validate(const Capability::Xml::PCcwSsidRange::value_type& value_)
{
	QRegExp q("(0x)?[0-3]");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PData2958

bool Validatable<Capability::Xml::PData2958>::validate(const Capability::Xml::PData2958::value_type& value_)
{
	QRegExp q("0x[0-9a-fA-F]{1,4}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PData2960

bool Traits<Capability::Xml::PData2960>::parse(const QString& src_, Capability::Xml::PData2960::value_type& dst_)
{
	bool output = false;
	dst_ = src_.toInt(&output);
	return output;
}

QString Traits<Capability::Xml::PData2960>::generate(Capability::Xml::PData2960::value_type src_)
{
	return QString::number(src_);
}

bool Validatable<Capability::Xml::PData2960>::validate(Capability::Xml::PData2960::value_type value_)
{
	if (0 > value_)
		return false;

	if (65535 < value_)
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PCpuset

bool Validatable<Capability::Xml::PCpuset>::validate(const Capability::Xml::PCpuset::value_type& value_)
{
	QRegExp q("([0-9]+(-[0-9]+)?|\\^[0-9]+)(,([0-9]+(-[0-9]+)?|\\^[0-9]+))*");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PVolName

bool Validatable<Capability::Xml::PVolName>::validate(const Capability::Xml::PVolName::value_type& value_)
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

bool Traits<Capability::Xml::PPortNumber>::parse(const QString& src_, Capability::Xml::PPortNumber::value_type& dst_)
{
	bool output = false;
	dst_ = src_.toInt(&output);
	return output;
}

QString Traits<Capability::Xml::PPortNumber>::generate(Capability::Xml::PPortNumber::value_type src_)
{
	return QString::number(src_);
}

bool Validatable<Capability::Xml::PPortNumber>::validate(Capability::Xml::PPortNumber::value_type value_)
{
	if (-1 > value_)
		return false;

	if (65535 < value_)
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PIobase

bool Validatable<Capability::Xml::PIobase>::validate(const Capability::Xml::PIobase::value_type& value_)
{
	QRegExp q("0x[a-fA-F0-9]{1,4}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PIrq

bool Validatable<Capability::Xml::PIrq>::validate(const Capability::Xml::PIrq::value_type& value_)
{
	QRegExp q("0x[a-fA-F0-9]");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PVendorId

bool Validatable<Capability::Xml::PVendorId>::validate(const Capability::Xml::PVendorId::value_type& value_)
{
	QRegExp q("[^,]{12}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PMemoryKB

bool Traits<Capability::Xml::PMemoryKB>::parse(const QString& src_, Capability::Xml::PMemoryKB::value_type& dst_)
{
	bool output = false;
	dst_ = src_.toULong(&output);
	return output;
}

QString Traits<Capability::Xml::PMemoryKB>::generate(Capability::Xml::PMemoryKB::value_type src_)
{
	return QString::number(src_);
}

///////////////////////////////////////////////////////////////////////////////
// struct PFeatureName

bool Validatable<Capability::Xml::PFeatureName>::validate(const Capability::Xml::PFeatureName::value_type& value_)
{
	QRegExp q("[a-zA-Z0-9\\-_\\.]+");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PCbitpos

bool Traits<Capability::Xml::PCbitpos>::parse(const QString& src_, Capability::Xml::PCbitpos::value_type& dst_)
{
	bool output = false;
	dst_ = src_.toUInt(&output);
	return output;
}

QString Traits<Capability::Xml::PCbitpos>::generate(Capability::Xml::PCbitpos::value_type src_)
{
	return QString::number(src_);
}

///////////////////////////////////////////////////////////////////////////////
// struct PReducedPhysBits

bool Traits<Capability::Xml::PReducedPhysBits>::parse(const QString& src_, Capability::Xml::PReducedPhysBits::value_type& dst_)
{
	bool output = false;
	dst_ = src_.toUInt(&output);
	return output;
}

QString Traits<Capability::Xml::PReducedPhysBits>::generate(Capability::Xml::PReducedPhysBits::value_type src_)
{
	return QString::number(src_);
}

} // namespace Libvirt
