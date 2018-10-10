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

#include "nodedev_data.h"
#include <QRegExp>

namespace Libvirt
{
///////////////////////////////////////////////////////////////////////////////
// struct PUnsignedInt

bool Traits<Nodedev::Xml::PUnsignedInt>::parse(const QString& src_, Nodedev::Xml::PUnsignedInt::value_type& dst_)
{
	QRegExp q("[0-9]+");
	if (!q.exactMatch(src_))
		return false;
	bool output = false;
	dst_ = src_.toUInt(&output);
	return output;
}

QString Traits<Nodedev::Xml::PUnsignedInt>::generate(Nodedev::Xml::PUnsignedInt::value_type src_)
{
	return QString::number(src_);
}

///////////////////////////////////////////////////////////////////////////////
// struct PUnsignedLong

bool Traits<Nodedev::Xml::PUnsignedLong>::parse(const QString& src_, Nodedev::Xml::PUnsignedLong::value_type& dst_)
{
	QRegExp q("[0-9]+");
	if (!q.exactMatch(src_))
		return false;
	bool output = false;
	dst_ = src_.toULong(&output);
	return output;
}

QString Traits<Nodedev::Xml::PUnsignedLong>::generate(Nodedev::Xml::PUnsignedLong::value_type src_)
{
	return QString::number(src_);
}

///////////////////////////////////////////////////////////////////////////////
// struct PHexuint

bool Validatable<Nodedev::Xml::PHexuint>::validate(const Nodedev::Xml::PHexuint::value_type& value_)
{
	QRegExp q("(0x)?[0-9a-f]+");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PPositiveInteger

bool Traits<Nodedev::Xml::PPositiveInteger>::parse(const QString& src_, Nodedev::Xml::PPositiveInteger::value_type& dst_)
{
	QRegExp q("[0-9]+");
	if (!q.exactMatch(src_))
		return false;
	bool output = false;
	dst_ = src_.toUInt(&output);
	return output;
}

QString Traits<Nodedev::Xml::PPositiveInteger>::generate(Nodedev::Xml::PPositiveInteger::value_type src_)
{
	return QString::number(src_);
}

///////////////////////////////////////////////////////////////////////////////
// struct POctalMode

bool Traits<Nodedev::Xml::POctalMode>::parse(const QString& src_, Nodedev::Xml::POctalMode::value_type& dst_)
{
	QRegExp q("[0-7]+");
	if (!q.exactMatch(src_))
		return false;
	bool output = false;
	dst_ = src_.toUInt(&output);
	return output;
}

QString Traits<Nodedev::Xml::POctalMode>::generate(Nodedev::Xml::POctalMode::value_type src_)
{
	return QString::number(src_);
}

///////////////////////////////////////////////////////////////////////////////
// struct PData3422

bool Validatable<Nodedev::Xml::PData3422>::validate(const Nodedev::Xml::PData3422::value_type& value_)
{
	QRegExp q("0x[0-9a-fA-F]{1,2}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PData3424

bool Traits<Nodedev::Xml::PData3424>::parse(const QString& src_, Nodedev::Xml::PData3424::value_type& dst_)
{
	bool output = false;
	dst_ = src_.toInt(&output);
	return output;
}

QString Traits<Nodedev::Xml::PData3424>::generate(Nodedev::Xml::PData3424::value_type src_)
{
	return QString::number(src_);
}

bool Validatable<Nodedev::Xml::PData3424>::validate(Nodedev::Xml::PData3424::value_type value_)
{
	if (0 > value_)
		return false;

	if (255 < value_)
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PData3426

bool Validatable<Nodedev::Xml::PData3426>::validate(const Nodedev::Xml::PData3426::value_type& value_)
{
	QRegExp q("0x[0-9a-fA-F]{1,2}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PData3428

bool Traits<Nodedev::Xml::PData3428>::parse(const QString& src_, Nodedev::Xml::PData3428::value_type& dst_)
{
	bool output = false;
	dst_ = src_.toInt(&output);
	return output;
}

QString Traits<Nodedev::Xml::PData3428>::generate(Nodedev::Xml::PData3428::value_type src_)
{
	return QString::number(src_);
}

bool Validatable<Nodedev::Xml::PData3428>::validate(Nodedev::Xml::PData3428::value_type value_)
{
	if (0 > value_)
		return false;

	if (255 < value_)
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PData3430

bool Validatable<Nodedev::Xml::PData3430>::validate(const Nodedev::Xml::PData3430::value_type& value_)
{
	QRegExp q("0x[0-9a-fA-F]{1,6}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PData3432

bool Traits<Nodedev::Xml::PData3432>::parse(const QString& src_, Nodedev::Xml::PData3432::value_type& dst_)
{
	bool output = false;
	dst_ = src_.toInt(&output);
	return output;
}

QString Traits<Nodedev::Xml::PData3432>::generate(Nodedev::Xml::PData3432::value_type src_)
{
	return QString::number(src_);
}

bool Validatable<Nodedev::Xml::PData3432>::validate(Nodedev::Xml::PData3432::value_type value_)
{
	if (0 > value_)
		return false;

	if (16777215 < value_)
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PData3434

bool Validatable<Nodedev::Xml::PData3434>::validate(const Nodedev::Xml::PData3434::value_type& value_)
{
	QRegExp q("[a-fA-F0-9]{32}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PData3436

bool Validatable<Nodedev::Xml::PData3436>::validate(const Nodedev::Xml::PData3436::value_type& value_)
{
	QRegExp q("[a-fA-F0-9]{8}\\-([a-fA-F0-9]{4}\\-){3}[a-fA-F0-9]{12}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PUniMacAddr

bool Validatable<Nodedev::Xml::PUniMacAddr>::validate(const Nodedev::Xml::PUniMacAddr::value_type& value_)
{
	QRegExp q("[a-fA-F0-9][02468aAcCeE](:[a-fA-F0-9]{2}){5}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PMultiMacAddr

bool Validatable<Nodedev::Xml::PMultiMacAddr>::validate(const Nodedev::Xml::PMultiMacAddr::value_type& value_)
{
	QRegExp q("[a-fA-F0-9][13579bBdDfF](:[a-fA-F0-9]{2}){5}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PMacAddr

bool Validatable<Nodedev::Xml::PMacAddr>::validate(const Nodedev::Xml::PMacAddr::value_type& value_)
{
	QRegExp q("[a-fA-F0-9]{2}(:[a-fA-F0-9]{2}){5}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PDuidLLT

bool Validatable<Nodedev::Xml::PDuidLLT>::validate(const Nodedev::Xml::PDuidLLT::value_type& value_)
{
	QRegExp q("[0]{1,2}:[0]{0,1}[1]:[0]{1,2}:[0]{0,1}[a-fA-F1-9](:[a-fA-F0-9]{1,2}){4}(:[a-fA-F0-9]{1,2}){6,8}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PDuidEN

bool Validatable<Nodedev::Xml::PDuidEN>::validate(const Nodedev::Xml::PDuidEN::value_type& value_)
{
	QRegExp q("[0]{1,2}:[0]{0,1}[2](:[a-fA-F0-9]{1,2}){4}(:[a-fA-F0-9]{1,2}){1,124}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PDuidLL

bool Validatable<Nodedev::Xml::PDuidLL>::validate(const Nodedev::Xml::PDuidLL::value_type& value_)
{
	QRegExp q("[0]{1,2}:[0]{0,1}[3]:[0]{1,2}:[0]{0,1}[a-fA-F1-9](:[a-fA-F0-9]{1,2}){6,8}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PDuidUUID

bool Validatable<Nodedev::Xml::PDuidUUID>::validate(const Nodedev::Xml::PDuidUUID::value_type& value_)
{
	QRegExp q("[0]{1,2}:[0]{0,1}[4](:[a-fA-F0-9]{1,2}){16}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PIpv4Addr

bool Validatable<Nodedev::Xml::PIpv4Addr>::validate(const Nodedev::Xml::PIpv4Addr::value_type& value_)
{
	QRegExp q("(((25[0-5])|(2[0-4][0-9])|(1[0-9]{2})|([1-9][0-9])|([0-9]))\\.){3}((25[0-5])|(2[0-4][0-9])|(1[0-9]{2})|([1-9][0-9])|([0-9]))");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PIpv6Addr

bool Validatable<Nodedev::Xml::PIpv6Addr>::validate(const Nodedev::Xml::PIpv6Addr::value_type& value_)
{
	QRegExp q("(([0-9A-Fa-f]{1,4}:){7}[0-9A-Fa-f]{1,4})|(([0-9A-Fa-f]{1,4}:){6}:[0-9A-Fa-f]{1,4})|(([0-9A-Fa-f]{1,4}:){5}:([0-9A-Fa-f]{1,4}:)?[0-9A-Fa-f]{1,4})|(([0-9A-Fa-f]{1,4}:){4}:([0-9A-Fa-f]{1,4}:){0,2}[0-9A-Fa-f]{1,4})|(([0-9A-Fa-f]{1,4}:){3}:([0-9A-Fa-f]{1,4}:){0,3}[0-9A-Fa-f]{1,4})|(([0-9A-Fa-f]{1,4}:){2}:([0-9A-Fa-f]{1,4}:){0,4}[0-9A-Fa-f]{1,4})|(([0-9A-Fa-f]{1,4}:){6}(((25[0-5])|(2[0-4][0-9])|(1[0-9]{2})|([1-9][0-9])|([0-9]))\\.){3}((25[0-5])|(2[0-4][0-9])|(1[0-9]{2})|([1-9][0-9])|([0-9])))|(([0-9A-Fa-f]{1,4}:){0,5}:(((25[0-5])|(2[0-4][0-9])|(1[0-9]{2})|([1-9][0-9])|([0-9]))\\.){3}((25[0-5])|(2[0-4][0-9])|(1[0-9]{2})|([1-9][0-9])|([0-9])))|(::([0-9A-Fa-f]{1,4}:){0,5}(((25[0-5])|(2[0-4][0-9])|(1[0-9]{2})|([1-9][0-9])|([0-9]))\\.){3}((25[0-5])|(2[0-4][0-9])|(1[0-9]{2})|([1-9][0-9])|([0-9])))|([0-9A-Fa-f]{1,4}::([0-9A-Fa-f]{1,4}:){0,5}[0-9A-Fa-f]{1,4})|(::([0-9A-Fa-f]{1,4}:){0,6}[0-9A-Fa-f]{1,4})|(([0-9A-Fa-f]{1,4}:){1,7}:)");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PIpv4Prefix

bool Traits<Nodedev::Xml::PIpv4Prefix>::parse(const QString& src_, Nodedev::Xml::PIpv4Prefix::value_type& dst_)
{
	bool output = false;
	dst_ = src_.toUInt(&output);
	return output;
}

QString Traits<Nodedev::Xml::PIpv4Prefix>::generate(Nodedev::Xml::PIpv4Prefix::value_type src_)
{
	return QString::number(src_);
}

bool Validatable<Nodedev::Xml::PIpv4Prefix>::validate(Nodedev::Xml::PIpv4Prefix::value_type value_)
{
	if (32 < value_)
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PIpv6Prefix

bool Traits<Nodedev::Xml::PIpv6Prefix>::parse(const QString& src_, Nodedev::Xml::PIpv6Prefix::value_type& dst_)
{
	bool output = false;
	dst_ = src_.toUInt(&output);
	return output;
}

QString Traits<Nodedev::Xml::PIpv6Prefix>::generate(Nodedev::Xml::PIpv6Prefix::value_type src_)
{
	return QString::number(src_);
}

bool Validatable<Nodedev::Xml::PIpv6Prefix>::validate(Nodedev::Xml::PIpv6Prefix::value_type value_)
{
	if (128 < value_)
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PGenericName

bool Validatable<Nodedev::Xml::PGenericName>::validate(const Nodedev::Xml::PGenericName::value_type& value_)
{
	QRegExp q("[a-zA-Z0-9_\\+\\-]+");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PDnsName

bool Validatable<Nodedev::Xml::PDnsName>::validate(const Nodedev::Xml::PDnsName::value_type& value_)
{
	QRegExp q("[a-zA-Z0-9\\.\\-]+");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PDeviceName

bool Validatable<Nodedev::Xml::PDeviceName>::validate(const Nodedev::Xml::PDeviceName::value_type& value_)
{
	QRegExp q("[a-zA-Z0-9_\\.\\-\\\\:/]+");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PFilePath

bool Validatable<Nodedev::Xml::PFilePath>::validate(const Nodedev::Xml::PFilePath::value_type& value_)
{
	QRegExp q("[a-zA-Z0-9_\\.\\+\\-\\\\&\"{}'<>/%: ]+");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PDirPath

bool Validatable<Nodedev::Xml::PDirPath>::validate(const Nodedev::Xml::PDirPath::value_type& value_)
{
	QRegExp q("[a-zA-Z0-9_\\.\\+\\-\\\\&\"{}'<>/%: ]+");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PAbsFilePath

bool Validatable<Nodedev::Xml::PAbsFilePath>::validate(const Nodedev::Xml::PAbsFilePath::value_type& value_)
{
	QRegExp q("/[a-zA-Z0-9_\\.\\+\\-\\\\&\"{}'<>/%,: ]+");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PAbsDirPath

bool Validatable<Nodedev::Xml::PAbsDirPath>::validate(const Nodedev::Xml::PAbsDirPath::value_type& value_)
{
	QRegExp q("/[a-zA-Z0-9_\\.\\+\\-\\\\&\"{}'<>/%: ]*");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PUnit

bool Validatable<Nodedev::Xml::PUnit>::validate(const Nodedev::Xml::PUnit::value_type& value_)
{
	QRegExp q("([bB]([yY][tT][eE][sS]?)?)|([kKmMgGtTpPeE]([iI]?[bB])?)");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PPciDomain

bool Validatable<Nodedev::Xml::PPciDomain>::validate(const Nodedev::Xml::PPciDomain::value_type& value_)
{
	QRegExp q("(0x)?[0-9a-fA-F]{1,4}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PPciBus

bool Validatable<Nodedev::Xml::PPciBus>::validate(const Nodedev::Xml::PPciBus::value_type& value_)
{
	QRegExp q("(0x)?[0-9a-fA-F]{1,2}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PPciSlot

bool Validatable<Nodedev::Xml::PPciSlot>::validate(const Nodedev::Xml::PPciSlot::value_type& value_)
{
	QRegExp q("(0x)?[0-1]?[0-9a-fA-F]");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PPciFunc

bool Validatable<Nodedev::Xml::PPciFunc>::validate(const Nodedev::Xml::PPciFunc::value_type& value_)
{
	QRegExp q("(0x)?[0-7]");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PWwn

bool Validatable<Nodedev::Xml::PWwn>::validate(const Nodedev::Xml::PWwn::value_type& value_)
{
	QRegExp q("(0x)?[0-9a-fA-F]{16}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PData3446

bool Validatable<Nodedev::Xml::PData3446>::validate(const Nodedev::Xml::PData3446::value_type& value_)
{
	QRegExp q("0x[0-9a-eA-E][0-9a-fA-F]?");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PData3448

bool Validatable<Nodedev::Xml::PData3448>::validate(const Nodedev::Xml::PData3448::value_type& value_)
{
	QRegExp q("0x[fF][0-9a-eA-E]?");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PData3450

bool Traits<Nodedev::Xml::PData3450>::parse(const QString& src_, Nodedev::Xml::PData3450::value_type& dst_)
{
	bool output = false;
	dst_ = src_.toInt(&output);
	return output;
}

QString Traits<Nodedev::Xml::PData3450>::generate(Nodedev::Xml::PData3450::value_type src_)
{
	return QString::number(src_);
}

bool Validatable<Nodedev::Xml::PData3450>::validate(Nodedev::Xml::PData3450::value_type value_)
{
	if (0 > value_)
		return false;

	if (254 < value_)
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PCcwSsidRange

bool Validatable<Nodedev::Xml::PCcwSsidRange>::validate(const Nodedev::Xml::PCcwSsidRange::value_type& value_)
{
	QRegExp q("(0x)?[0-3]");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PData3452

bool Validatable<Nodedev::Xml::PData3452>::validate(const Nodedev::Xml::PData3452::value_type& value_)
{
	QRegExp q("0x[0-9a-fA-F]{1,4}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PData3454

bool Traits<Nodedev::Xml::PData3454>::parse(const QString& src_, Nodedev::Xml::PData3454::value_type& dst_)
{
	bool output = false;
	dst_ = src_.toInt(&output);
	return output;
}

QString Traits<Nodedev::Xml::PData3454>::generate(Nodedev::Xml::PData3454::value_type src_)
{
	return QString::number(src_);
}

bool Validatable<Nodedev::Xml::PData3454>::validate(Nodedev::Xml::PData3454::value_type value_)
{
	if (0 > value_)
		return false;

	if (65535 < value_)
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PCpuset

bool Validatable<Nodedev::Xml::PCpuset>::validate(const Nodedev::Xml::PCpuset::value_type& value_)
{
	QRegExp q("([0-9]+(-[0-9]+)?|\\^[0-9]+)(,([0-9]+(-[0-9]+)?|\\^[0-9]+))*");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PVolName

bool Validatable<Nodedev::Xml::PVolName>::validate(const Nodedev::Xml::PVolName::value_type& value_)
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

bool Traits<Nodedev::Xml::PPortNumber>::parse(const QString& src_, Nodedev::Xml::PPortNumber::value_type& dst_)
{
	bool output = false;
	dst_ = src_.toInt(&output);
	return output;
}

QString Traits<Nodedev::Xml::PPortNumber>::generate(Nodedev::Xml::PPortNumber::value_type src_)
{
	return QString::number(src_);
}

bool Validatable<Nodedev::Xml::PPortNumber>::validate(Nodedev::Xml::PPortNumber::value_type value_)
{
	if (-1 > value_)
		return false;

	if (65535 < value_)
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PIobase

bool Validatable<Nodedev::Xml::PIobase>::validate(const Nodedev::Xml::PIobase::value_type& value_)
{
	QRegExp q("0x[a-fA-F0-9]{1,4}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PIrq

bool Validatable<Nodedev::Xml::PIrq>::validate(const Nodedev::Xml::PIrq::value_type& value_)
{
	QRegExp q("0x[a-fA-F0-9]");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PNode

bool Traits<Nodedev::Xml::PNode>::parse(const QString& src_, Nodedev::Xml::PNode::value_type& dst_)
{
	bool output = false;
	dst_ = src_.toInt(&output);
	return output;
}

QString Traits<Nodedev::Xml::PNode>::generate(Nodedev::Xml::PNode::value_type src_)
{
	return QString::number(src_);
}

///////////////////////////////////////////////////////////////////////////////
// struct PSpeed

bool Validatable<Nodedev::Xml::PSpeed>::validate(const Nodedev::Xml::PSpeed::value_type& value_)
{
	QRegExp q("[0-9]+(.[0-9]+)?");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PNetfeaturename

bool Validatable<Nodedev::Xml::PNetfeaturename>::validate(const Nodedev::Xml::PNetfeaturename::value_type& value_)
{
	QRegExp q("[a-zA-Z\\-_]+");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PMac

bool Validatable<Nodedev::Xml::PMac>::validate(const Nodedev::Xml::PMac::value_type& value_)
{
	QRegExp q("([a-fA-F0-9]{2}:){5}[a-fA-F0-9]{2}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PPath

bool Validatable<Nodedev::Xml::PPath>::validate(const Nodedev::Xml::PPath::value_type& value_)
{
	QRegExp q("/[a-zA-Z0-9_\\+\\-/%]+");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct VUUID

bool Traits<Nodedev::Xml::VUUID>::parse(const QString& src_, Nodedev::Xml::VUUID& dst_)
{
	int x;
	mpl::at_c<Nodedev::Xml::VUUID::types, 0>::type a0;
	x = Marshal<Nodedev::Xml::PData3434>::setString(src_, a0);
	if (0 < x)
	{
		dst_ = a0;
		return true;
	}
	mpl::at_c<Nodedev::Xml::VUUID::types, 1>::type a1;
	x = Marshal<Nodedev::Xml::PData3436>::setString(src_, a1);
	if (0 < x)
	{
		dst_ = a1;
		return true;
	}

	return false;
}

QString Traits<Nodedev::Xml::VUUID>::generate(const Nodedev::Xml::VUUID& src_)
{
	switch (src_.which())
	{
	case 0:
		return Marshal<Nodedev::Xml::PData3434>::getString(boost::get<mpl::at_c<Nodedev::Xml::VUUID::types, 0>::type>(src_));
	case 1:
		return Marshal<Nodedev::Xml::PData3436>::getString(boost::get<mpl::at_c<Nodedev::Xml::VUUID::types, 1>::type>(src_));
	}
	return QString();
}

///////////////////////////////////////////////////////////////////////////////
// struct VCcwCssidRange

bool Traits<Nodedev::Xml::VCcwCssidRange>::parse(const QString& src_, Nodedev::Xml::VCcwCssidRange& dst_)
{
	int x;
	mpl::at_c<Nodedev::Xml::VCcwCssidRange::types, 0>::type a0;
	x = Marshal<Nodedev::Xml::PData3446>::setString(src_, a0);
	if (0 < x)
	{
		dst_ = a0;
		return true;
	}
	mpl::at_c<Nodedev::Xml::VCcwCssidRange::types, 1>::type a1;
	x = Marshal<Nodedev::Xml::PData3448>::setString(src_, a1);
	if (0 < x)
	{
		dst_ = a1;
		return true;
	}
	mpl::at_c<Nodedev::Xml::VCcwCssidRange::types, 2>::type a2;
	x = Marshal<Nodedev::Xml::PData3450>::setString(src_, a2);
	if (0 < x)
	{
		dst_ = a2;
		return true;
	}

	return false;
}

QString Traits<Nodedev::Xml::VCcwCssidRange>::generate(const Nodedev::Xml::VCcwCssidRange& src_)
{
	switch (src_.which())
	{
	case 0:
		return Marshal<Nodedev::Xml::PData3446>::getString(boost::get<mpl::at_c<Nodedev::Xml::VCcwCssidRange::types, 0>::type>(src_));
	case 1:
		return Marshal<Nodedev::Xml::PData3448>::getString(boost::get<mpl::at_c<Nodedev::Xml::VCcwCssidRange::types, 1>::type>(src_));
	case 2:
		return Marshal<Nodedev::Xml::PData3450>::getString(boost::get<mpl::at_c<Nodedev::Xml::VCcwCssidRange::types, 2>::type>(src_));
	}
	return QString();
}

///////////////////////////////////////////////////////////////////////////////
// struct VCcwDevnoRange

bool Traits<Nodedev::Xml::VCcwDevnoRange>::parse(const QString& src_, Nodedev::Xml::VCcwDevnoRange& dst_)
{
	int x;
	mpl::at_c<Nodedev::Xml::VCcwDevnoRange::types, 0>::type a0;
	x = Marshal<Nodedev::Xml::PData3452>::setString(src_, a0);
	if (0 < x)
	{
		dst_ = a0;
		return true;
	}
	mpl::at_c<Nodedev::Xml::VCcwDevnoRange::types, 1>::type a1;
	x = Marshal<Nodedev::Xml::PData3454>::setString(src_, a1);
	if (0 < x)
	{
		dst_ = a1;
		return true;
	}

	return false;
}

QString Traits<Nodedev::Xml::VCcwDevnoRange>::generate(const Nodedev::Xml::VCcwDevnoRange& src_)
{
	switch (src_.which())
	{
	case 0:
		return Marshal<Nodedev::Xml::PData3452>::getString(boost::get<mpl::at_c<Nodedev::Xml::VCcwDevnoRange::types, 0>::type>(src_));
	case 1:
		return Marshal<Nodedev::Xml::PData3454>::getString(boost::get<mpl::at_c<Nodedev::Xml::VCcwDevnoRange::types, 1>::type>(src_));
	}
	return QString();
}

} // namespace Libvirt
