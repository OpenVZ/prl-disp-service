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

#include "blocksnapshot_data.h"
#include <QRegExp>

namespace Libvirt
{
///////////////////////////////////////////////////////////////////////////////
// struct PUnsignedInt

bool Traits<Blocksnapshot::Xml::PUnsignedInt>::parse(const QString& src_, Blocksnapshot::Xml::PUnsignedInt::value_type& dst_)
{
	QRegExp q("[0-9]+");
	if (!q.exactMatch(src_))
		return false;
	bool output = false;
	dst_ = src_.toUInt(&output);
	return output;
}

QString Traits<Blocksnapshot::Xml::PUnsignedInt>::generate(Blocksnapshot::Xml::PUnsignedInt::value_type src_)
{
	return QString::number(src_);
}

///////////////////////////////////////////////////////////////////////////////
// struct PUnsignedLong

bool Traits<Blocksnapshot::Xml::PUnsignedLong>::parse(const QString& src_, Blocksnapshot::Xml::PUnsignedLong::value_type& dst_)
{
	QRegExp q("[0-9]+");
	if (!q.exactMatch(src_))
		return false;
	bool output = false;
	dst_ = src_.toULong(&output);
	return output;
}

QString Traits<Blocksnapshot::Xml::PUnsignedLong>::generate(Blocksnapshot::Xml::PUnsignedLong::value_type src_)
{
	return QString::number(src_);
}

///////////////////////////////////////////////////////////////////////////////
// struct PHexuint

bool Validatable<Blocksnapshot::Xml::PHexuint>::validate(const Blocksnapshot::Xml::PHexuint::value_type& value_)
{
	QRegExp q("(0x)?[0-9a-f]+");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PPositiveInteger

bool Traits<Blocksnapshot::Xml::PPositiveInteger>::parse(const QString& src_, Blocksnapshot::Xml::PPositiveInteger::value_type& dst_)
{
	QRegExp q("[0-9]+");
	if (!q.exactMatch(src_))
		return false;
	bool output = false;
	dst_ = src_.toUInt(&output);
	return output;
}

QString Traits<Blocksnapshot::Xml::PPositiveInteger>::generate(Blocksnapshot::Xml::PPositiveInteger::value_type src_)
{
	return QString::number(src_);
}

///////////////////////////////////////////////////////////////////////////////
// struct POctalMode

bool Traits<Blocksnapshot::Xml::POctalMode>::parse(const QString& src_, Blocksnapshot::Xml::POctalMode::value_type& dst_)
{
	QRegExp q("[0-7]+");
	if (!q.exactMatch(src_))
		return false;
	bool output = false;
	dst_ = src_.toUInt(&output);
	return output;
}

QString Traits<Blocksnapshot::Xml::POctalMode>::generate(Blocksnapshot::Xml::POctalMode::value_type src_)
{
	return QString::number(src_);
}

///////////////////////////////////////////////////////////////////////////////
// struct PData3970

bool Validatable<Blocksnapshot::Xml::PData3970>::validate(const Blocksnapshot::Xml::PData3970::value_type& value_)
{
	QRegExp q("0x[0-9a-fA-F]{1,2}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PData2974

bool Traits<Blocksnapshot::Xml::PData2974>::parse(const QString& src_, Blocksnapshot::Xml::PData2974::value_type& dst_)
{
	bool output = false;
	dst_ = src_.toInt(&output);
	return output;
}

QString Traits<Blocksnapshot::Xml::PData2974>::generate(Blocksnapshot::Xml::PData2974::value_type src_)
{
	return QString::number(src_);
}

bool Validatable<Blocksnapshot::Xml::PData2974>::validate(Blocksnapshot::Xml::PData2974::value_type value_)
{
	if (0 > value_)
		return false;

	if (255 < value_)
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PData2976

bool Validatable<Blocksnapshot::Xml::PData2976>::validate(const Blocksnapshot::Xml::PData2976::value_type& value_)
{
	QRegExp q("0x[0-9a-fA-F]{1,2}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PData2978

bool Traits<Blocksnapshot::Xml::PData2978>::parse(const QString& src_, Blocksnapshot::Xml::PData2978::value_type& dst_)
{
	bool output = false;
	dst_ = src_.toInt(&output);
	return output;
}

QString Traits<Blocksnapshot::Xml::PData2978>::generate(Blocksnapshot::Xml::PData2978::value_type src_)
{
	return QString::number(src_);
}

bool Validatable<Blocksnapshot::Xml::PData2978>::validate(Blocksnapshot::Xml::PData2978::value_type value_)
{
	if (0 > value_)
		return false;

	if (255 < value_)
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PData2980

bool Validatable<Blocksnapshot::Xml::PData2980>::validate(const Blocksnapshot::Xml::PData2980::value_type& value_)
{
	QRegExp q("0x[0-9a-fA-F]{1,6}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PData2982

bool Traits<Blocksnapshot::Xml::PData2982>::parse(const QString& src_, Blocksnapshot::Xml::PData2982::value_type& dst_)
{
	bool output = false;
	dst_ = src_.toInt(&output);
	return output;
}

QString Traits<Blocksnapshot::Xml::PData2982>::generate(Blocksnapshot::Xml::PData2982::value_type src_)
{
	return QString::number(src_);
}

bool Validatable<Blocksnapshot::Xml::PData2982>::validate(Blocksnapshot::Xml::PData2982::value_type value_)
{
	if (0 > value_)
		return false;

	if (16777215 < value_)
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PData2984

bool Validatable<Blocksnapshot::Xml::PData2984>::validate(const Blocksnapshot::Xml::PData2984::value_type& value_)
{
	QRegExp q("[a-fA-F0-9]{32}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PData2986

bool Validatable<Blocksnapshot::Xml::PData2986>::validate(const Blocksnapshot::Xml::PData2986::value_type& value_)
{
	QRegExp q("[a-fA-F0-9]{8}\\-([a-fA-F0-9]{4}\\-){3}[a-fA-F0-9]{12}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PUniMacAddr

bool Validatable<Blocksnapshot::Xml::PUniMacAddr>::validate(const Blocksnapshot::Xml::PUniMacAddr::value_type& value_)
{
	QRegExp q("[a-fA-F0-9][02468aAcCeE](:[a-fA-F0-9]{2}){5}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PMultiMacAddr

bool Validatable<Blocksnapshot::Xml::PMultiMacAddr>::validate(const Blocksnapshot::Xml::PMultiMacAddr::value_type& value_)
{
	QRegExp q("[a-fA-F0-9][13579bBdDfF](:[a-fA-F0-9]{2}){5}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PMacAddr

bool Validatable<Blocksnapshot::Xml::PMacAddr>::validate(const Blocksnapshot::Xml::PMacAddr::value_type& value_)
{
	QRegExp q("[a-fA-F0-9]{2}(:[a-fA-F0-9]{2}){5}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PDuidLLT

bool Validatable<Blocksnapshot::Xml::PDuidLLT>::validate(const Blocksnapshot::Xml::PDuidLLT::value_type& value_)
{
	QRegExp q("[0]{1,2}:[0]{0,1}[1]:[0]{1,2}:[0]{0,1}[a-fA-F1-9](:[a-fA-F0-9]{1,2}){4}(:[a-fA-F0-9]{1,2}){6,8}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PDuidEN

bool Validatable<Blocksnapshot::Xml::PDuidEN>::validate(const Blocksnapshot::Xml::PDuidEN::value_type& value_)
{
	QRegExp q("[0]{1,2}:[0]{0,1}[2](:[a-fA-F0-9]{1,2}){4}(:[a-fA-F0-9]{1,2}){1,124}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PDuidLL

bool Validatable<Blocksnapshot::Xml::PDuidLL>::validate(const Blocksnapshot::Xml::PDuidLL::value_type& value_)
{
	QRegExp q("[0]{1,2}:[0]{0,1}[3]:[0]{1,2}:[0]{0,1}[a-fA-F1-9](:[a-fA-F0-9]{1,2}){6,8}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PDuidUUID

bool Validatable<Blocksnapshot::Xml::PDuidUUID>::validate(const Blocksnapshot::Xml::PDuidUUID::value_type& value_)
{
	QRegExp q("[0]{1,2}:[0]{0,1}[4](:[a-fA-F0-9]{1,2}){16}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PIpv4Addr

bool Validatable<Blocksnapshot::Xml::PIpv4Addr>::validate(const Blocksnapshot::Xml::PIpv4Addr::value_type& value_)
{
	QRegExp q("(((25[0-5])|(2[0-4][0-9])|(1[0-9]{2})|([1-9][0-9])|([0-9]))\\.){3}((25[0-5])|(2[0-4][0-9])|(1[0-9]{2})|([1-9][0-9])|([0-9]))");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PIpv6Addr

bool Validatable<Blocksnapshot::Xml::PIpv6Addr>::validate(const Blocksnapshot::Xml::PIpv6Addr::value_type& value_)
{
	QRegExp q("(([0-9A-Fa-f]{1,4}:){7}[0-9A-Fa-f]{1,4})|(([0-9A-Fa-f]{1,4}:){6}:[0-9A-Fa-f]{1,4})|(([0-9A-Fa-f]{1,4}:){5}:([0-9A-Fa-f]{1,4}:)?[0-9A-Fa-f]{1,4})|(([0-9A-Fa-f]{1,4}:){4}:([0-9A-Fa-f]{1,4}:){0,2}[0-9A-Fa-f]{1,4})|(([0-9A-Fa-f]{1,4}:){3}:([0-9A-Fa-f]{1,4}:){0,3}[0-9A-Fa-f]{1,4})|(([0-9A-Fa-f]{1,4}:){2}:([0-9A-Fa-f]{1,4}:){0,4}[0-9A-Fa-f]{1,4})|(([0-9A-Fa-f]{1,4}:){6}(((25[0-5])|(2[0-4][0-9])|(1[0-9]{2})|([1-9][0-9])|([0-9]))\\.){3}((25[0-5])|(2[0-4][0-9])|(1[0-9]{2})|([1-9][0-9])|([0-9])))|(([0-9A-Fa-f]{1,4}:){0,5}:(((25[0-5])|(2[0-4][0-9])|(1[0-9]{2})|([1-9][0-9])|([0-9]))\\.){3}((25[0-5])|(2[0-4][0-9])|(1[0-9]{2})|([1-9][0-9])|([0-9])))|(::([0-9A-Fa-f]{1,4}:){0,5}(((25[0-5])|(2[0-4][0-9])|(1[0-9]{2})|([1-9][0-9])|([0-9]))\\.){3}((25[0-5])|(2[0-4][0-9])|(1[0-9]{2})|([1-9][0-9])|([0-9])))|([0-9A-Fa-f]{1,4}::([0-9A-Fa-f]{1,4}:){0,5}[0-9A-Fa-f]{1,4})|(::([0-9A-Fa-f]{1,4}:){0,6}[0-9A-Fa-f]{1,4})|(([0-9A-Fa-f]{1,4}:){1,7}:)");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PIpv4Prefix

bool Traits<Blocksnapshot::Xml::PIpv4Prefix>::parse(const QString& src_, Blocksnapshot::Xml::PIpv4Prefix::value_type& dst_)
{
	bool output = false;
	dst_ = src_.toUInt(&output);
	return output;
}

QString Traits<Blocksnapshot::Xml::PIpv4Prefix>::generate(Blocksnapshot::Xml::PIpv4Prefix::value_type src_)
{
	return QString::number(src_);
}

bool Validatable<Blocksnapshot::Xml::PIpv4Prefix>::validate(Blocksnapshot::Xml::PIpv4Prefix::value_type value_)
{
	if (32 < value_)
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PIpv6Prefix

bool Traits<Blocksnapshot::Xml::PIpv6Prefix>::parse(const QString& src_, Blocksnapshot::Xml::PIpv6Prefix::value_type& dst_)
{
	bool output = false;
	dst_ = src_.toUInt(&output);
	return output;
}

QString Traits<Blocksnapshot::Xml::PIpv6Prefix>::generate(Blocksnapshot::Xml::PIpv6Prefix::value_type src_)
{
	return QString::number(src_);
}

bool Validatable<Blocksnapshot::Xml::PIpv6Prefix>::validate(Blocksnapshot::Xml::PIpv6Prefix::value_type value_)
{
	if (128 < value_)
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PGenericName

bool Validatable<Blocksnapshot::Xml::PGenericName>::validate(const Blocksnapshot::Xml::PGenericName::value_type& value_)
{
	QRegExp q("[a-zA-Z0-9_\\+\\-]+");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PDnsName

bool Validatable<Blocksnapshot::Xml::PDnsName>::validate(const Blocksnapshot::Xml::PDnsName::value_type& value_)
{
	QRegExp q("[a-zA-Z0-9\\.\\-]+");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PDeviceName

bool Validatable<Blocksnapshot::Xml::PDeviceName>::validate(const Blocksnapshot::Xml::PDeviceName::value_type& value_)
{
	QRegExp q("[a-zA-Z0-9_\\.\\-\\\\:/]+");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PFilePath

bool Validatable<Blocksnapshot::Xml::PFilePath>::validate(const Blocksnapshot::Xml::PFilePath::value_type& value_)
{
	QRegExp q("[a-zA-Z0-9_\\.\\+\\-\\\\&\"{}'<>/%: ]+");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PDirPath

bool Validatable<Blocksnapshot::Xml::PDirPath>::validate(const Blocksnapshot::Xml::PDirPath::value_type& value_)
{
	QRegExp q("[a-zA-Z0-9_\\.\\+\\-\\\\&\"{}'<>/%: ]+");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PAbsFilePath

bool Validatable<Blocksnapshot::Xml::PAbsFilePath>::validate(const Blocksnapshot::Xml::PAbsFilePath::value_type& value_)
{
	QRegExp q("/[a-zA-Z0-9_\\.\\+\\-\\\\&\"{}'<>/%,: ]+");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PAbsDirPath

bool Validatable<Blocksnapshot::Xml::PAbsDirPath>::validate(const Blocksnapshot::Xml::PAbsDirPath::value_type& value_)
{
	QRegExp q("/[a-zA-Z0-9_\\.\\+\\-\\\\&\"{}'<>/%: ]*");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PUnit

bool Validatable<Blocksnapshot::Xml::PUnit>::validate(const Blocksnapshot::Xml::PUnit::value_type& value_)
{
	QRegExp q("([bB]([yY][tT][eE][sS]?)?)|([kKmMgGtTpPeE]([iI]?[bB])?)");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PPciDomain

bool Validatable<Blocksnapshot::Xml::PPciDomain>::validate(const Blocksnapshot::Xml::PPciDomain::value_type& value_)
{
	QRegExp q("(0x)?[0-9a-fA-F]{1,4}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PPciBus

bool Validatable<Blocksnapshot::Xml::PPciBus>::validate(const Blocksnapshot::Xml::PPciBus::value_type& value_)
{
	QRegExp q("(0x)?[0-9a-fA-F]{1,2}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PPciSlot

bool Validatable<Blocksnapshot::Xml::PPciSlot>::validate(const Blocksnapshot::Xml::PPciSlot::value_type& value_)
{
	QRegExp q("(0x)?[0-1]?[0-9a-fA-F]");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PPciFunc

bool Validatable<Blocksnapshot::Xml::PPciFunc>::validate(const Blocksnapshot::Xml::PPciFunc::value_type& value_)
{
	QRegExp q("(0x)?[0-7]");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PWwn

bool Validatable<Blocksnapshot::Xml::PWwn>::validate(const Blocksnapshot::Xml::PWwn::value_type& value_)
{
	QRegExp q("(0x)?[0-9a-fA-F]{16}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PData2996

bool Validatable<Blocksnapshot::Xml::PData2996>::validate(const Blocksnapshot::Xml::PData2996::value_type& value_)
{
	QRegExp q("0x[0-9a-eA-E][0-9a-fA-F]?");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PData2998

bool Validatable<Blocksnapshot::Xml::PData2998>::validate(const Blocksnapshot::Xml::PData2998::value_type& value_)
{
	QRegExp q("0x[fF][0-9a-eA-E]?");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PData3000

bool Traits<Blocksnapshot::Xml::PData3000>::parse(const QString& src_, Blocksnapshot::Xml::PData3000::value_type& dst_)
{
	bool output = false;
	dst_ = src_.toInt(&output);
	return output;
}

QString Traits<Blocksnapshot::Xml::PData3000>::generate(Blocksnapshot::Xml::PData3000::value_type src_)
{
	return QString::number(src_);
}

bool Validatable<Blocksnapshot::Xml::PData3000>::validate(Blocksnapshot::Xml::PData3000::value_type value_)
{
	if (0 > value_)
		return false;

	if (254 < value_)
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PCcwSsidRange

bool Validatable<Blocksnapshot::Xml::PCcwSsidRange>::validate(const Blocksnapshot::Xml::PCcwSsidRange::value_type& value_)
{
	QRegExp q("(0x)?[0-3]");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PData3002

bool Validatable<Blocksnapshot::Xml::PData3002>::validate(const Blocksnapshot::Xml::PData3002::value_type& value_)
{
	QRegExp q("0x[0-9a-fA-F]{1,4}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PData3004

bool Traits<Blocksnapshot::Xml::PData3004>::parse(const QString& src_, Blocksnapshot::Xml::PData3004::value_type& dst_)
{
	bool output = false;
	dst_ = src_.toInt(&output);
	return output;
}

QString Traits<Blocksnapshot::Xml::PData3004>::generate(Blocksnapshot::Xml::PData3004::value_type src_)
{
	return QString::number(src_);
}

bool Validatable<Blocksnapshot::Xml::PData3004>::validate(Blocksnapshot::Xml::PData3004::value_type value_)
{
	if (0 > value_)
		return false;

	if (65535 < value_)
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PCpuset

bool Validatable<Blocksnapshot::Xml::PCpuset>::validate(const Blocksnapshot::Xml::PCpuset::value_type& value_)
{
	QRegExp q("([0-9]+(-[0-9]+)?|\\^[0-9]+)(,([0-9]+(-[0-9]+)?|\\^[0-9]+))*");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PVolName

bool Validatable<Blocksnapshot::Xml::PVolName>::validate(const Blocksnapshot::Xml::PVolName::value_type& value_)
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

bool Traits<Blocksnapshot::Xml::PPortNumber>::parse(const QString& src_, Blocksnapshot::Xml::PPortNumber::value_type& dst_)
{
	bool output = false;
	dst_ = src_.toInt(&output);
	return output;
}

QString Traits<Blocksnapshot::Xml::PPortNumber>::generate(Blocksnapshot::Xml::PPortNumber::value_type src_)
{
	return QString::number(src_);
}

bool Validatable<Blocksnapshot::Xml::PPortNumber>::validate(Blocksnapshot::Xml::PPortNumber::value_type value_)
{
	if (-1 > value_)
		return false;

	if (65535 < value_)
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PIobase

bool Validatable<Blocksnapshot::Xml::PIobase>::validate(const Blocksnapshot::Xml::PIobase::value_type& value_)
{
	QRegExp q("0x[a-fA-F0-9]{1,4}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PIrq

bool Validatable<Blocksnapshot::Xml::PIrq>::validate(const Blocksnapshot::Xml::PIrq::value_type& value_)
{
	QRegExp q("0x[a-fA-F0-9]");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PCompat

bool Validatable<Blocksnapshot::Xml::PCompat>::validate(const Blocksnapshot::Xml::PCompat::value_type& value_)
{
	QRegExp q("[0-9]+\\.[0-9]+");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PVirtualPortProfileID

bool Validatable<Blocksnapshot::Xml::PVirtualPortProfileID>::validate(const Blocksnapshot::Xml::PVirtualPortProfileID::value_type& value_)
{
	if (39 < value_.length())
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PSpeed

bool Traits<Blocksnapshot::Xml::PSpeed>::parse(const QString& src_, Blocksnapshot::Xml::PSpeed::value_type& dst_)
{
	QRegExp q("[0-9]+");
	if (!q.exactMatch(src_))
		return false;
	bool output = false;
	dst_ = src_.toUInt(&output);
	return output;
}

QString Traits<Blocksnapshot::Xml::PSpeed>::generate(Blocksnapshot::Xml::PSpeed::value_type src_)
{
	return QString::number(src_);
}

bool Validatable<Blocksnapshot::Xml::PSpeed>::validate(Blocksnapshot::Xml::PSpeed::value_type value_)
{
	if (1 > value_)
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PBurstSize

bool Traits<Blocksnapshot::Xml::PBurstSize>::parse(const QString& src_, Blocksnapshot::Xml::PBurstSize::value_type& dst_)
{
	QRegExp q("[0-9]+");
	if (!q.exactMatch(src_))
		return false;
	bool output = false;
	dst_ = src_.toUInt(&output);
	return output;
}

QString Traits<Blocksnapshot::Xml::PBurstSize>::generate(Blocksnapshot::Xml::PBurstSize::value_type src_)
{
	return QString::number(src_);
}

bool Validatable<Blocksnapshot::Xml::PBurstSize>::validate(Blocksnapshot::Xml::PBurstSize::value_type value_)
{
	if (1 > value_)
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PUnsignedShort

bool Traits<Blocksnapshot::Xml::PUnsignedShort>::parse(const QString& src_, Blocksnapshot::Xml::PUnsignedShort::value_type& dst_)
{
	bool output = false;
	dst_ = src_.toInt(&output);
	return output;
}

QString Traits<Blocksnapshot::Xml::PUnsignedShort>::generate(Blocksnapshot::Xml::PUnsignedShort::value_type src_)
{
	return QString::number(src_);
}

bool Validatable<Blocksnapshot::Xml::PUnsignedShort>::validate(Blocksnapshot::Xml::PUnsignedShort::value_type value_)
{
	if (0 > value_)
		return false;

	if (65535 < value_)
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PProtocol

bool Validatable<Blocksnapshot::Xml::PProtocol>::validate(const Blocksnapshot::Xml::PProtocol::value_type& value_)
{
	QRegExp q("(tcp)|(udp)");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PAddrFamily

bool Validatable<Blocksnapshot::Xml::PAddrFamily>::validate(const Blocksnapshot::Xml::PAddrFamily::value_type& value_)
{
	QRegExp q("(ipv4)|(ipv6)");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PId

bool Traits<Blocksnapshot::Xml::PId>::parse(const QString& src_, Blocksnapshot::Xml::PId::value_type& dst_)
{
	bool output = false;
	dst_ = src_.toUInt(&output);
	return output;
}

QString Traits<Blocksnapshot::Xml::PId>::generate(Blocksnapshot::Xml::PId::value_type src_)
{
	return QString::number(src_);
}

bool Validatable<Blocksnapshot::Xml::PId>::validate(Blocksnapshot::Xml::PId::value_type value_)
{
	if (4095 < value_)
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PPort

bool Traits<Blocksnapshot::Xml::PPort>::parse(const QString& src_, Blocksnapshot::Xml::PPort::value_type& dst_)
{
	bool output = false;
	dst_ = src_.toInt(&output);
	return output;
}

QString Traits<Blocksnapshot::Xml::PPort>::generate(Blocksnapshot::Xml::PPort::value_type src_)
{
	return QString::number(src_);
}

bool Validatable<Blocksnapshot::Xml::PPort>::validate(Blocksnapshot::Xml::PPort::value_type value_)
{
	if (1 > value_)
		return false;

	if (65535 < value_)
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PTitle

bool Validatable<Blocksnapshot::Xml::PTitle>::validate(const Blocksnapshot::Xml::PTitle::value_type& value_)
{
	QRegExp q("[^\n]+");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PMachine

bool Validatable<Blocksnapshot::Xml::PMachine>::validate(const Blocksnapshot::Xml::PMachine::value_type& value_)
{
	QRegExp q("[a-zA-Z0-9_\\.\\-]+");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PReadIopsSec

bool Traits<Blocksnapshot::Xml::PReadIopsSec>::parse(const QString& src_, Blocksnapshot::Xml::PReadIopsSec::value_type& dst_)
{
	bool output = false;
	dst_ = src_.toUInt(&output);
	return output;
}

QString Traits<Blocksnapshot::Xml::PReadIopsSec>::generate(Blocksnapshot::Xml::PReadIopsSec::value_type src_)
{
	return QString::number(src_);
}

///////////////////////////////////////////////////////////////////////////////
// struct PWriteIopsSec

bool Traits<Blocksnapshot::Xml::PWriteIopsSec>::parse(const QString& src_, Blocksnapshot::Xml::PWriteIopsSec::value_type& dst_)
{
	bool output = false;
	dst_ = src_.toUInt(&output);
	return output;
}

QString Traits<Blocksnapshot::Xml::PWriteIopsSec>::generate(Blocksnapshot::Xml::PWriteIopsSec::value_type src_)
{
	return QString::number(src_);
}

///////////////////////////////////////////////////////////////////////////////
// struct PReadBytesSec

bool Traits<Blocksnapshot::Xml::PReadBytesSec>::parse(const QString& src_, Blocksnapshot::Xml::PReadBytesSec::value_type& dst_)
{
	bool output = false;
	dst_ = src_.toULong(&output);
	return output;
}

QString Traits<Blocksnapshot::Xml::PReadBytesSec>::generate(Blocksnapshot::Xml::PReadBytesSec::value_type src_)
{
	return QString::number(src_);
}

///////////////////////////////////////////////////////////////////////////////
// struct PWriteBytesSec

bool Traits<Blocksnapshot::Xml::PWriteBytesSec>::parse(const QString& src_, Blocksnapshot::Xml::PWriteBytesSec::value_type& dst_)
{
	bool output = false;
	dst_ = src_.toULong(&output);
	return output;
}

QString Traits<Blocksnapshot::Xml::PWriteBytesSec>::generate(Blocksnapshot::Xml::PWriteBytesSec::value_type src_)
{
	return QString::number(src_);
}

///////////////////////////////////////////////////////////////////////////////
// struct PVendor

bool Validatable<Blocksnapshot::Xml::PVendor>::validate(const Blocksnapshot::Xml::PVendor::value_type& value_)
{
	QRegExp q("[\\x20-\\x7E]{0,8}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PProduct

bool Validatable<Blocksnapshot::Xml::PProduct>::validate(const Blocksnapshot::Xml::PProduct::value_type& value_)
{
	QRegExp q("[\\x20-\\x7E]{0,16}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PDiskTarget

bool Validatable<Blocksnapshot::Xml::PDiskTarget>::validate(const Blocksnapshot::Xml::PDiskTarget::value_type& value_)
{
	QRegExp q("(ioemu:)?(fd|hd|sd|vd|xvd|ubd)[a-zA-Z0-9_]+");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PCyls

bool Traits<Blocksnapshot::Xml::PCyls>::parse(const QString& src_, Blocksnapshot::Xml::PCyls::value_type& dst_)
{
	bool output = false;
	dst_ = src_.toInt(&output);
	return output;
}

QString Traits<Blocksnapshot::Xml::PCyls>::generate(Blocksnapshot::Xml::PCyls::value_type src_)
{
	return QString::number(src_);
}

///////////////////////////////////////////////////////////////////////////////
// struct PHeads

bool Traits<Blocksnapshot::Xml::PHeads>::parse(const QString& src_, Blocksnapshot::Xml::PHeads::value_type& dst_)
{
	bool output = false;
	dst_ = src_.toInt(&output);
	return output;
}

QString Traits<Blocksnapshot::Xml::PHeads>::generate(Blocksnapshot::Xml::PHeads::value_type src_)
{
	return QString::number(src_);
}

///////////////////////////////////////////////////////////////////////////////
// struct PSecs

bool Traits<Blocksnapshot::Xml::PSecs>::parse(const QString& src_, Blocksnapshot::Xml::PSecs::value_type& dst_)
{
	bool output = false;
	dst_ = src_.toInt(&output);
	return output;
}

QString Traits<Blocksnapshot::Xml::PSecs>::generate(Blocksnapshot::Xml::PSecs::value_type src_)
{
	return QString::number(src_);
}

///////////////////////////////////////////////////////////////////////////////
// struct PLogicalBlockSize

bool Traits<Blocksnapshot::Xml::PLogicalBlockSize>::parse(const QString& src_, Blocksnapshot::Xml::PLogicalBlockSize::value_type& dst_)
{
	bool output = false;
	dst_ = src_.toInt(&output);
	return output;
}

QString Traits<Blocksnapshot::Xml::PLogicalBlockSize>::generate(Blocksnapshot::Xml::PLogicalBlockSize::value_type src_)
{
	return QString::number(src_);
}

///////////////////////////////////////////////////////////////////////////////
// struct PPhysicalBlockSize

bool Traits<Blocksnapshot::Xml::PPhysicalBlockSize>::parse(const QString& src_, Blocksnapshot::Xml::PPhysicalBlockSize::value_type& dst_)
{
	bool output = false;
	dst_ = src_.toInt(&output);
	return output;
}

QString Traits<Blocksnapshot::Xml::PPhysicalBlockSize>::generate(Blocksnapshot::Xml::PPhysicalBlockSize::value_type src_)
{
	return QString::number(src_);
}

///////////////////////////////////////////////////////////////////////////////
// struct PType

bool Validatable<Blocksnapshot::Xml::PType>::validate(const Blocksnapshot::Xml::PType::value_type& value_)
{
	QRegExp q("[a-zA-Z0-9\\-_]+");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PPasswdValidTo

bool Traits<Blocksnapshot::Xml::PPasswdValidTo>::parse(const QString& src_, Blocksnapshot::Xml::PPasswdValidTo::value_type& dst_)
{
	dst_ = QDateTime::fromString(src_);
	return !dst_.isNull();
}

QString Traits<Blocksnapshot::Xml::PPasswdValidTo>::generate(const Blocksnapshot::Xml::PPasswdValidTo::value_type& src_)
{
	return src_.toString();
}

///////////////////////////////////////////////////////////////////////////////
// struct PVendorId

bool Validatable<Blocksnapshot::Xml::PVendorId>::validate(const Blocksnapshot::Xml::PVendorId::value_type& value_)
{
	QRegExp q("[^,]{12}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PSysinfoValue

bool Validatable<Blocksnapshot::Xml::PSysinfoValue>::validate(const Blocksnapshot::Xml::PSysinfoValue::value_type& value_)
{
	QRegExp q("[a-zA-Z0-9/\\-_\\. \\(\\)]+");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PTotalBytesSec

bool Traits<Blocksnapshot::Xml::PTotalBytesSec>::parse(const QString& src_, Blocksnapshot::Xml::PTotalBytesSec::value_type& dst_)
{
	bool output = false;
	dst_ = src_.toULong(&output);
	return output;
}

QString Traits<Blocksnapshot::Xml::PTotalBytesSec>::generate(Blocksnapshot::Xml::PTotalBytesSec::value_type src_)
{
	return QString::number(src_);
}

///////////////////////////////////////////////////////////////////////////////
// struct PTotalIopsSec

bool Traits<Blocksnapshot::Xml::PTotalIopsSec>::parse(const QString& src_, Blocksnapshot::Xml::PTotalIopsSec::value_type& dst_)
{
	bool output = false;
	dst_ = src_.toULong(&output);
	return output;
}

QString Traits<Blocksnapshot::Xml::PTotalIopsSec>::generate(Blocksnapshot::Xml::PTotalIopsSec::value_type src_)
{
	return QString::number(src_);
}

///////////////////////////////////////////////////////////////////////////////
// struct PReadIopsSec1

bool Traits<Blocksnapshot::Xml::PReadIopsSec1>::parse(const QString& src_, Blocksnapshot::Xml::PReadIopsSec1::value_type& dst_)
{
	bool output = false;
	dst_ = src_.toULong(&output);
	return output;
}

QString Traits<Blocksnapshot::Xml::PReadIopsSec1>::generate(Blocksnapshot::Xml::PReadIopsSec1::value_type src_)
{
	return QString::number(src_);
}

///////////////////////////////////////////////////////////////////////////////
// struct PWriteIopsSec1

bool Traits<Blocksnapshot::Xml::PWriteIopsSec1>::parse(const QString& src_, Blocksnapshot::Xml::PWriteIopsSec1::value_type& dst_)
{
	bool output = false;
	dst_ = src_.toULong(&output);
	return output;
}

QString Traits<Blocksnapshot::Xml::PWriteIopsSec1>::generate(Blocksnapshot::Xml::PWriteIopsSec1::value_type src_)
{
	return QString::number(src_);
}

///////////////////////////////////////////////////////////////////////////////
// struct PSizeIopsSec

bool Traits<Blocksnapshot::Xml::PSizeIopsSec>::parse(const QString& src_, Blocksnapshot::Xml::PSizeIopsSec::value_type& dst_)
{
	bool output = false;
	dst_ = src_.toULong(&output);
	return output;
}

QString Traits<Blocksnapshot::Xml::PSizeIopsSec>::generate(Blocksnapshot::Xml::PSizeIopsSec::value_type src_)
{
	return QString::number(src_);
}

///////////////////////////////////////////////////////////////////////////////
// struct PRetries

bool Traits<Blocksnapshot::Xml::PRetries>::parse(const QString& src_, Blocksnapshot::Xml::PRetries::value_type& dst_)
{
	bool output = false;
	dst_ = src_.toUInt(&output);
	return output;
}

QString Traits<Blocksnapshot::Xml::PRetries>::generate(Blocksnapshot::Xml::PRetries::value_type src_)
{
	return QString::number(src_);
}

///////////////////////////////////////////////////////////////////////////////
// struct PValue

bool Validatable<Blocksnapshot::Xml::PValue>::validate(const Blocksnapshot::Xml::PValue::value_type& value_)
{
	QRegExp q("[^,]{0,12}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PCountCPU

bool Traits<Blocksnapshot::Xml::PCountCPU>::parse(const QString& src_, Blocksnapshot::Xml::PCountCPU::value_type& dst_)
{
	QRegExp q("[0-9]+");
	if (!q.exactMatch(src_))
		return false;
	bool output = false;
	dst_ = src_.toUShort(&output);
	return output;
}

QString Traits<Blocksnapshot::Xml::PCountCPU>::generate(Blocksnapshot::Xml::PCountCPU::value_type src_)
{
	return QString::number(src_);
}

bool Validatable<Blocksnapshot::Xml::PCountCPU>::validate(Blocksnapshot::Xml::PCountCPU::value_type value_)
{
	if (1 > value_)
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PVcpuid

bool Traits<Blocksnapshot::Xml::PVcpuid>::parse(const QString& src_, Blocksnapshot::Xml::PVcpuid::value_type& dst_)
{
	QRegExp q("[0-9]+");
	if (!q.exactMatch(src_))
		return false;
	bool output = false;
	dst_ = src_.toUShort(&output);
	return output;
}

QString Traits<Blocksnapshot::Xml::PVcpuid>::generate(Blocksnapshot::Xml::PVcpuid::value_type src_)
{
	return QString::number(src_);
}

///////////////////////////////////////////////////////////////////////////////
// struct PCpushares

bool Traits<Blocksnapshot::Xml::PCpushares>::parse(const QString& src_, Blocksnapshot::Xml::PCpushares::value_type& dst_)
{
	QRegExp q("[0-9]+");
	if (!q.exactMatch(src_))
		return false;
	bool output = false;
	dst_ = src_.toUInt(&output);
	return output;
}

QString Traits<Blocksnapshot::Xml::PCpushares>::generate(Blocksnapshot::Xml::PCpushares::value_type src_)
{
	return QString::number(src_);
}

///////////////////////////////////////////////////////////////////////////////
// struct PCpuperiod

bool Traits<Blocksnapshot::Xml::PCpuperiod>::parse(const QString& src_, Blocksnapshot::Xml::PCpuperiod::value_type& dst_)
{
	QRegExp q("[0-9]+");
	if (!q.exactMatch(src_))
		return false;
	bool output = false;
	dst_ = src_.toULong(&output);
	return output;
}

QString Traits<Blocksnapshot::Xml::PCpuperiod>::generate(Blocksnapshot::Xml::PCpuperiod::value_type src_)
{
	return QString::number(src_);
}

bool Validatable<Blocksnapshot::Xml::PCpuperiod>::validate(Blocksnapshot::Xml::PCpuperiod::value_type value_)
{
	if (1000 > value_)
		return false;

	if (1000000 < value_)
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PCpuquota

bool Traits<Blocksnapshot::Xml::PCpuquota>::parse(const QString& src_, Blocksnapshot::Xml::PCpuquota::value_type& dst_)
{
	QRegExp q("-?[0-9]+");
	if (!q.exactMatch(src_))
		return false;
	bool output = false;
	dst_ = src_.toLong(&output);
	return output;
}

QString Traits<Blocksnapshot::Xml::PCpuquota>::generate(Blocksnapshot::Xml::PCpuquota::value_type src_)
{
	return QString::number(src_);
}

bool Validatable<Blocksnapshot::Xml::PCpuquota>::validate(Blocksnapshot::Xml::PCpuquota::value_type value_)
{
	if (-1 > value_)
		return false;

	if (18446744073709551 < value_)
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PRebootTimeoutDelay

bool Traits<Blocksnapshot::Xml::PRebootTimeoutDelay>::parse(const QString& src_, Blocksnapshot::Xml::PRebootTimeoutDelay::value_type& dst_)
{
	bool output = false;
	dst_ = src_.toShort(&output);
	return output;
}

QString Traits<Blocksnapshot::Xml::PRebootTimeoutDelay>::generate(Blocksnapshot::Xml::PRebootTimeoutDelay::value_type src_)
{
	return QString::number(src_);
}

bool Validatable<Blocksnapshot::Xml::PRebootTimeoutDelay>::validate(Blocksnapshot::Xml::PRebootTimeoutDelay::value_type value_)
{
	if (-1 > value_)
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PWeight

bool Traits<Blocksnapshot::Xml::PWeight>::parse(const QString& src_, Blocksnapshot::Xml::PWeight::value_type& dst_)
{
	QRegExp q("[0-9]+");
	if (!q.exactMatch(src_))
		return false;
	bool output = false;
	dst_ = src_.toUInt(&output);
	return output;
}

QString Traits<Blocksnapshot::Xml::PWeight>::generate(Blocksnapshot::Xml::PWeight::value_type src_)
{
	return QString::number(src_);
}

bool Validatable<Blocksnapshot::Xml::PWeight>::validate(Blocksnapshot::Xml::PWeight::value_type value_)
{
	if (100 > value_)
		return false;

	if (1000 < value_)
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PMemoryKB

bool Traits<Blocksnapshot::Xml::PMemoryKB>::parse(const QString& src_, Blocksnapshot::Xml::PMemoryKB::value_type& dst_)
{
	QRegExp q("[0-9]+");
	if (!q.exactMatch(src_))
		return false;
	bool output = false;
	dst_ = src_.toUInt(&output);
	return output;
}

QString Traits<Blocksnapshot::Xml::PMemoryKB>::generate(Blocksnapshot::Xml::PMemoryKB::value_type src_)
{
	return QString::number(src_);
}

///////////////////////////////////////////////////////////////////////////////
// struct PDomainName

bool Validatable<Blocksnapshot::Xml::PDomainName>::validate(const Blocksnapshot::Xml::PDomainName::value_type& value_)
{
	QRegExp q("[^\n]+");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PDiskSerial

bool Validatable<Blocksnapshot::Xml::PDiskSerial>::validate(const Blocksnapshot::Xml::PDiskSerial::value_type& value_)
{
	QRegExp q("[A-Za-z0-9_\\.\\+\\-]*");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PBridgeMode

bool Validatable<Blocksnapshot::Xml::PBridgeMode>::validate(const Blocksnapshot::Xml::PBridgeMode::value_type& value_)
{
	QRegExp q("(vepa|bridge|private|passthrough)");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PAddrIPorName

bool Validatable<Blocksnapshot::Xml::PAddrIPorName>::validate(const Blocksnapshot::Xml::PAddrIPorName::value_type& value_)
{
	QRegExp q("(([0-2]?[0-9]?[0-9]\\.){3}[0-2]?[0-9]?[0-9])|(([0-9a-fA-F]+|:)+[0-9a-fA-F]+)|([a-zA-Z0-9_\\.\\+\\-]*)");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PUsbIdDefault

bool Validatable<Blocksnapshot::Xml::PUsbIdDefault>::validate(const Blocksnapshot::Xml::PUsbIdDefault::value_type& value_)
{
	QRegExp q("-1");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PUsbId

bool Validatable<Blocksnapshot::Xml::PUsbId>::validate(const Blocksnapshot::Xml::PUsbId::value_type& value_)
{
	QRegExp q("(0x)?[0-9a-fA-F]{1,4}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PUsbVersion

bool Validatable<Blocksnapshot::Xml::PUsbVersion>::validate(const Blocksnapshot::Xml::PUsbVersion::value_type& value_)
{
	QRegExp q("[0-9]{1,2}.[0-9]{1,2}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PUsbAddr

bool Validatable<Blocksnapshot::Xml::PUsbAddr>::validate(const Blocksnapshot::Xml::PUsbAddr::value_type& value_)
{
	QRegExp q("(0x)?[0-9a-fA-F]{1,3}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PUsbClass

bool Validatable<Blocksnapshot::Xml::PUsbClass>::validate(const Blocksnapshot::Xml::PUsbClass::value_type& value_)
{
	QRegExp q("(0x)?[0-9a-fA-F]{1,2}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PUsbPort

bool Validatable<Blocksnapshot::Xml::PUsbPort>::validate(const Blocksnapshot::Xml::PUsbPort::value_type& value_)
{
	QRegExp q("((0x)?[0-9a-fA-F]{1,3}\\.){0,3}(0x)?[0-9a-fA-F]{1,3}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PDriveController

bool Validatable<Blocksnapshot::Xml::PDriveController>::validate(const Blocksnapshot::Xml::PDriveController::value_type& value_)
{
	QRegExp q("[0-9]{1,2}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PDriveBus

bool Validatable<Blocksnapshot::Xml::PDriveBus>::validate(const Blocksnapshot::Xml::PDriveBus::value_type& value_)
{
	QRegExp q("[0-9]{1,2}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PDriveTarget

bool Validatable<Blocksnapshot::Xml::PDriveTarget>::validate(const Blocksnapshot::Xml::PDriveTarget::value_type& value_)
{
	QRegExp q("[0-9]{1,2}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PDriveUnit

bool Validatable<Blocksnapshot::Xml::PDriveUnit>::validate(const Blocksnapshot::Xml::PDriveUnit::value_type& value_)
{
	QRegExp q("[0-9]{1,2}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PFeatureName

bool Validatable<Blocksnapshot::Xml::PFeatureName>::validate(const Blocksnapshot::Xml::PFeatureName::value_type& value_)
{
	QRegExp q("[a-zA-Z0-9\\-_\\.]+");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PTimeDelta

bool Validatable<Blocksnapshot::Xml::PTimeDelta>::validate(const Blocksnapshot::Xml::PTimeDelta::value_type& value_)
{
	QRegExp q("(-|\\+)?[0-9]+");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PTimeZone

bool Validatable<Blocksnapshot::Xml::PTimeZone>::validate(const Blocksnapshot::Xml::PTimeZone::value_type& value_)
{
	QRegExp q("[a-zA-Z0-9_\\.\\+\\-/]+");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PFilterParamName

bool Validatable<Blocksnapshot::Xml::PFilterParamName>::validate(const Blocksnapshot::Xml::PFilterParamName::value_type& value_)
{
	QRegExp q("[a-zA-Z0-9_]+");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PFilterParamValue

bool Validatable<Blocksnapshot::Xml::PFilterParamValue>::validate(const Blocksnapshot::Xml::PFilterParamValue::value_type& value_)
{
	QRegExp q("[a-zA-Z0-9_\\.:]+");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PSpaprvioReg

bool Validatable<Blocksnapshot::Xml::PSpaprvioReg>::validate(const Blocksnapshot::Xml::PSpaprvioReg::value_type& value_)
{
	QRegExp q("(0x)?[0-9a-fA-F]{1,16}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PAliasName

bool Validatable<Blocksnapshot::Xml::PAliasName>::validate(const Blocksnapshot::Xml::PAliasName::value_type& value_)
{
	QRegExp q("[a-zA-Z0-9_\\-.]+");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct VName

bool Traits<Blocksnapshot::Xml::VName>::parse(const QString& src_, Blocksnapshot::Xml::VName& dst_)
{
	int x;
	mpl::at_c<Blocksnapshot::Xml::VName::types, 0>::type a0;
	x = Marshal<Blocksnapshot::Xml::PDiskTarget>::setString(src_, a0);
	if (0 < x)
	{
		dst_ = a0;
		return true;
	}
	mpl::at_c<Blocksnapshot::Xml::VName::types, 1>::type a1;
	x = Marshal<Blocksnapshot::Xml::PAbsFilePath>::setString(src_, a1);
	if (0 < x)
	{
		dst_ = a1;
		return true;
	}

	return false;
}

QString Traits<Blocksnapshot::Xml::VName>::generate(const Blocksnapshot::Xml::VName& src_)
{
	switch (src_.which())
	{
	case 0:
		return Marshal<Blocksnapshot::Xml::PDiskTarget>::getString(boost::get<mpl::at_c<Blocksnapshot::Xml::VName::types, 0>::type>(src_));
	case 1:
		return Marshal<Blocksnapshot::Xml::PAbsFilePath>::getString(boost::get<mpl::at_c<Blocksnapshot::Xml::VName::types, 1>::type>(src_));
	}
	return QString();
}

} // namespace Libvirt
