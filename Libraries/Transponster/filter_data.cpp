/*
 * Copyright (c) 2020 Virtuozzo International GmbH. All rights reserved.
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

#include "filter_data.h"
#include <QRegExp>

namespace Libvirt
{
///////////////////////////////////////////////////////////////////////////////
// struct PUnsignedInt

bool Traits<Filter::Xml::PUnsignedInt>::parse(const QString& src_, Filter::Xml::PUnsignedInt::value_type& dst_)
{
	QRegExp q("[0-9]+");
	if (!q.exactMatch(src_))
		return false;
	bool output = false;
	dst_ = src_.toUInt(&output);
	return output;
}

QString Traits<Filter::Xml::PUnsignedInt>::generate(Filter::Xml::PUnsignedInt::value_type src_)
{
	return QString::number(src_);
}

///////////////////////////////////////////////////////////////////////////////
// struct PUnsignedLong

bool Traits<Filter::Xml::PUnsignedLong>::parse(const QString& src_, Filter::Xml::PUnsignedLong::value_type& dst_)
{
	QRegExp q("[0-9]+");
	if (!q.exactMatch(src_))
		return false;
	bool output = false;
	dst_ = src_.toULong(&output);
	return output;
}

QString Traits<Filter::Xml::PUnsignedLong>::generate(Filter::Xml::PUnsignedLong::value_type src_)
{
	return QString::number(src_);
}

///////////////////////////////////////////////////////////////////////////////
// struct PHexuint

bool Validatable<Filter::Xml::PHexuint>::validate(const Filter::Xml::PHexuint::value_type& value_)
{
	QRegExp q("(0x)?[0-9a-f]+");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PPositiveInteger

bool Traits<Filter::Xml::PPositiveInteger>::parse(const QString& src_, Filter::Xml::PPositiveInteger::value_type& dst_)
{
	QRegExp q("[0-9]+");
	if (!q.exactMatch(src_))
		return false;
	bool output = false;
	dst_ = src_.toUInt(&output);
	return output;
}

QString Traits<Filter::Xml::PPositiveInteger>::generate(Filter::Xml::PPositiveInteger::value_type src_)
{
	return QString::number(src_);
}

///////////////////////////////////////////////////////////////////////////////
// struct POctalMode

bool Traits<Filter::Xml::POctalMode>::parse(const QString& src_, Filter::Xml::POctalMode::value_type& dst_)
{
	QRegExp q("[0-7]+");
	if (!q.exactMatch(src_))
		return false;
	bool output = false;
	dst_ = src_.toUInt(&output);
	return output;
}

QString Traits<Filter::Xml::POctalMode>::generate(Filter::Xml::POctalMode::value_type src_)
{
	return QString::number(src_);
}

///////////////////////////////////////////////////////////////////////////////
// struct PData5567

bool Validatable<Filter::Xml::PData5567>::validate(const Filter::Xml::PData5567::value_type& value_)
{
	QRegExp q("0x[0-9a-fA-F]{1,2}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PData5569

bool Traits<Filter::Xml::PData5569>::parse(const QString& src_, Filter::Xml::PData5569::value_type& dst_)
{
	bool output = false;
	dst_ = src_.toInt(&output);
	return output;
}

QString Traits<Filter::Xml::PData5569>::generate(Filter::Xml::PData5569::value_type src_)
{
	return QString::number(src_);
}

bool Validatable<Filter::Xml::PData5569>::validate(Filter::Xml::PData5569::value_type value_)
{
	if (0 > value_)
		return false;

	if (255 < value_)
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PData5571

bool Validatable<Filter::Xml::PData5571>::validate(const Filter::Xml::PData5571::value_type& value_)
{
	QRegExp q("0x[0-9a-fA-F]{1,2}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PData5573

bool Traits<Filter::Xml::PData5573>::parse(const QString& src_, Filter::Xml::PData5573::value_type& dst_)
{
	bool output = false;
	dst_ = src_.toInt(&output);
	return output;
}

QString Traits<Filter::Xml::PData5573>::generate(Filter::Xml::PData5573::value_type src_)
{
	return QString::number(src_);
}

bool Validatable<Filter::Xml::PData5573>::validate(Filter::Xml::PData5573::value_type value_)
{
	if (0 > value_)
		return false;

	if (255 < value_)
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PData5575

bool Validatable<Filter::Xml::PData5575>::validate(const Filter::Xml::PData5575::value_type& value_)
{
	QRegExp q("0x[0-9a-fA-F]{1,6}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PData5577

bool Traits<Filter::Xml::PData5577>::parse(const QString& src_, Filter::Xml::PData5577::value_type& dst_)
{
	bool output = false;
	dst_ = src_.toInt(&output);
	return output;
}

QString Traits<Filter::Xml::PData5577>::generate(Filter::Xml::PData5577::value_type src_)
{
	return QString::number(src_);
}

bool Validatable<Filter::Xml::PData5577>::validate(Filter::Xml::PData5577::value_type value_)
{
	if (0 > value_)
		return false;

	if (16777215 < value_)
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PData5579

bool Validatable<Filter::Xml::PData5579>::validate(const Filter::Xml::PData5579::value_type& value_)
{
	QRegExp q("[a-fA-F0-9]{32}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PData5581

bool Validatable<Filter::Xml::PData5581>::validate(const Filter::Xml::PData5581::value_type& value_)
{
	QRegExp q("[a-fA-F0-9]{8}\\-([a-fA-F0-9]{4}\\-){3}[a-fA-F0-9]{12}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PUniMacAddr

bool Validatable<Filter::Xml::PUniMacAddr>::validate(const Filter::Xml::PUniMacAddr::value_type& value_)
{
	QRegExp q("[a-fA-F0-9][02468aAcCeE](:[a-fA-F0-9]{2}){5}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PMultiMacAddr

bool Validatable<Filter::Xml::PMultiMacAddr>::validate(const Filter::Xml::PMultiMacAddr::value_type& value_)
{
	QRegExp q("[a-fA-F0-9][13579bBdDfF](:[a-fA-F0-9]{2}){5}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PMacAddr

bool Validatable<Filter::Xml::PMacAddr>::validate(const Filter::Xml::PMacAddr::value_type& value_)
{
	QRegExp q("[a-fA-F0-9]{2}(:[a-fA-F0-9]{2}){5}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PDuidLLT

bool Validatable<Filter::Xml::PDuidLLT>::validate(const Filter::Xml::PDuidLLT::value_type& value_)
{
	QRegExp q("[0]{1,2}:[0]{0,1}[1]:[0]{1,2}:[0]{0,1}[a-fA-F1-9](:[a-fA-F0-9]{1,2}){4}(:[a-fA-F0-9]{1,2}){6,8}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PDuidEN

bool Validatable<Filter::Xml::PDuidEN>::validate(const Filter::Xml::PDuidEN::value_type& value_)
{
	QRegExp q("[0]{1,2}:[0]{0,1}[2](:[a-fA-F0-9]{1,2}){4}(:[a-fA-F0-9]{1,2}){1,124}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PDuidLL

bool Validatable<Filter::Xml::PDuidLL>::validate(const Filter::Xml::PDuidLL::value_type& value_)
{
	QRegExp q("[0]{1,2}:[0]{0,1}[3]:[0]{1,2}:[0]{0,1}[a-fA-F1-9](:[a-fA-F0-9]{1,2}){6,8}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PDuidUUID

bool Validatable<Filter::Xml::PDuidUUID>::validate(const Filter::Xml::PDuidUUID::value_type& value_)
{
	QRegExp q("[0]{1,2}:[0]{0,1}[4](:[a-fA-F0-9]{1,2}){16}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PIpv4Addr

bool Validatable<Filter::Xml::PIpv4Addr>::validate(const Filter::Xml::PIpv4Addr::value_type& value_)
{
	QRegExp q("(((25[0-5])|(2[0-4][0-9])|(1[0-9]{2})|([1-9][0-9])|([0-9]))\\.){3}((25[0-5])|(2[0-4][0-9])|(1[0-9]{2})|([1-9][0-9])|([0-9]))");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PIpv6Addr

bool Validatable<Filter::Xml::PIpv6Addr>::validate(const Filter::Xml::PIpv6Addr::value_type& value_)
{
	QRegExp q("(([0-9A-Fa-f]{1,4}:){7}[0-9A-Fa-f]{1,4})|(([0-9A-Fa-f]{1,4}:){6}:[0-9A-Fa-f]{1,4})|(([0-9A-Fa-f]{1,4}:){5}:([0-9A-Fa-f]{1,4}:)?[0-9A-Fa-f]{1,4})|(([0-9A-Fa-f]{1,4}:){4}:([0-9A-Fa-f]{1,4}:){0,2}[0-9A-Fa-f]{1,4})|(([0-9A-Fa-f]{1,4}:){3}:([0-9A-Fa-f]{1,4}:){0,3}[0-9A-Fa-f]{1,4})|(([0-9A-Fa-f]{1,4}:){2}:([0-9A-Fa-f]{1,4}:){0,4}[0-9A-Fa-f]{1,4})|(([0-9A-Fa-f]{1,4}:){6}(((25[0-5])|(2[0-4][0-9])|(1[0-9]{2})|([1-9][0-9])|([0-9]))\\.){3}((25[0-5])|(2[0-4][0-9])|(1[0-9]{2})|([1-9][0-9])|([0-9])))|(([0-9A-Fa-f]{1,4}:){0,5}:(((25[0-5])|(2[0-4][0-9])|(1[0-9]{2})|([1-9][0-9])|([0-9]))\\.){3}((25[0-5])|(2[0-4][0-9])|(1[0-9]{2})|([1-9][0-9])|([0-9])))|(::([0-9A-Fa-f]{1,4}:){0,5}(((25[0-5])|(2[0-4][0-9])|(1[0-9]{2})|([1-9][0-9])|([0-9]))\\.){3}((25[0-5])|(2[0-4][0-9])|(1[0-9]{2})|([1-9][0-9])|([0-9])))|([0-9A-Fa-f]{1,4}::([0-9A-Fa-f]{1,4}:){0,5}[0-9A-Fa-f]{1,4})|(::([0-9A-Fa-f]{1,4}:){0,6}[0-9A-Fa-f]{1,4})|(([0-9A-Fa-f]{1,4}:){1,7}:)");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PIpv4Prefix

bool Traits<Filter::Xml::PIpv4Prefix>::parse(const QString& src_, Filter::Xml::PIpv4Prefix::value_type& dst_)
{
	bool output = false;
	dst_ = src_.toUInt(&output);
	return output;
}

QString Traits<Filter::Xml::PIpv4Prefix>::generate(Filter::Xml::PIpv4Prefix::value_type src_)
{
	return QString::number(src_);
}

bool Validatable<Filter::Xml::PIpv4Prefix>::validate(Filter::Xml::PIpv4Prefix::value_type value_)
{
	if (32 < value_)
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PIpv6Prefix

bool Traits<Filter::Xml::PIpv6Prefix>::parse(const QString& src_, Filter::Xml::PIpv6Prefix::value_type& dst_)
{
	bool output = false;
	dst_ = src_.toUInt(&output);
	return output;
}

QString Traits<Filter::Xml::PIpv6Prefix>::generate(Filter::Xml::PIpv6Prefix::value_type src_)
{
	return QString::number(src_);
}

bool Validatable<Filter::Xml::PIpv6Prefix>::validate(Filter::Xml::PIpv6Prefix::value_type value_)
{
	if (128 < value_)
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PGenericName

bool Validatable<Filter::Xml::PGenericName>::validate(const Filter::Xml::PGenericName::value_type& value_)
{
	QRegExp q("[a-zA-Z0-9_\\+\\-]+");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PDnsName

bool Validatable<Filter::Xml::PDnsName>::validate(const Filter::Xml::PDnsName::value_type& value_)
{
	QRegExp q("[a-zA-Z0-9\\.\\-]+");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PDeviceName

bool Validatable<Filter::Xml::PDeviceName>::validate(const Filter::Xml::PDeviceName::value_type& value_)
{
	QRegExp q("[a-zA-Z0-9_\\.\\-\\\\:/]+");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PFilePath

bool Validatable<Filter::Xml::PFilePath>::validate(const Filter::Xml::PFilePath::value_type& value_)
{
	QRegExp q("[a-zA-Z0-9_\\.\\+\\-\\\\&\"{}'<>/%: ()]+");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PDirPath

bool Validatable<Filter::Xml::PDirPath>::validate(const Filter::Xml::PDirPath::value_type& value_)
{
	QRegExp q("[a-zA-Z0-9_\\.\\+\\-\\\\&\"{}'<>/%: ()]+");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PAbsFilePath

bool Validatable<Filter::Xml::PAbsFilePath>::validate(const Filter::Xml::PAbsFilePath::value_type& value_)
{
	QRegExp q("/[a-zA-Z0-9_\\.\\+\\-\\\\&\"{}'<>/%,: ()]+");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PAbsDirPath

bool Validatable<Filter::Xml::PAbsDirPath>::validate(const Filter::Xml::PAbsDirPath::value_type& value_)
{
	QRegExp q("/[a-zA-Z0-9_\\.\\+\\-\\\\&\"{}'<>/%: ()]*");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PUnit

bool Validatable<Filter::Xml::PUnit>::validate(const Filter::Xml::PUnit::value_type& value_)
{
	QRegExp q("([bB]([yY][tT][eE][sS]?)?)|([kKmMgGtTpPeE]([iI]?[bB])?)");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PPciDomain

bool Validatable<Filter::Xml::PPciDomain>::validate(const Filter::Xml::PPciDomain::value_type& value_)
{
	QRegExp q("(0x)?[0-9a-fA-F]{1,4}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PPciBus

bool Validatable<Filter::Xml::PPciBus>::validate(const Filter::Xml::PPciBus::value_type& value_)
{
	QRegExp q("(0x)?[0-9a-fA-F]{1,2}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PPciSlot

bool Validatable<Filter::Xml::PPciSlot>::validate(const Filter::Xml::PPciSlot::value_type& value_)
{
	QRegExp q("(0x)?[0-1]?[0-9a-fA-F]");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PPciFunc

bool Validatable<Filter::Xml::PPciFunc>::validate(const Filter::Xml::PPciFunc::value_type& value_)
{
	QRegExp q("(0x)?[0-7]");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PWwn

bool Validatable<Filter::Xml::PWwn>::validate(const Filter::Xml::PWwn::value_type& value_)
{
	QRegExp q("(0x)?[0-9a-fA-F]{16}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PData5591

bool Validatable<Filter::Xml::PData5591>::validate(const Filter::Xml::PData5591::value_type& value_)
{
	QRegExp q("0x[0-9a-eA-E][0-9a-fA-F]?");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PData5593

bool Validatable<Filter::Xml::PData5593>::validate(const Filter::Xml::PData5593::value_type& value_)
{
	QRegExp q("0x[fF][0-9a-eA-E]?");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PData5595

bool Traits<Filter::Xml::PData5595>::parse(const QString& src_, Filter::Xml::PData5595::value_type& dst_)
{
	bool output = false;
	dst_ = src_.toInt(&output);
	return output;
}

QString Traits<Filter::Xml::PData5595>::generate(Filter::Xml::PData5595::value_type src_)
{
	return QString::number(src_);
}

bool Validatable<Filter::Xml::PData5595>::validate(Filter::Xml::PData5595::value_type value_)
{
	if (0 > value_)
		return false;

	if (254 < value_)
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PCcwSsidRange

bool Validatable<Filter::Xml::PCcwSsidRange>::validate(const Filter::Xml::PCcwSsidRange::value_type& value_)
{
	QRegExp q("(0x)?[0-3]");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PData5597

bool Validatable<Filter::Xml::PData5597>::validate(const Filter::Xml::PData5597::value_type& value_)
{
	QRegExp q("0x[0-9a-fA-F]{1,4}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PData5599

bool Traits<Filter::Xml::PData5599>::parse(const QString& src_, Filter::Xml::PData5599::value_type& dst_)
{
	bool output = false;
	dst_ = src_.toInt(&output);
	return output;
}

QString Traits<Filter::Xml::PData5599>::generate(Filter::Xml::PData5599::value_type src_)
{
	return QString::number(src_);
}

bool Validatable<Filter::Xml::PData5599>::validate(Filter::Xml::PData5599::value_type value_)
{
	if (0 > value_)
		return false;

	if (65535 < value_)
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PCpuset

bool Validatable<Filter::Xml::PCpuset>::validate(const Filter::Xml::PCpuset::value_type& value_)
{
	QRegExp q("([0-9]+(-[0-9]+)?|\\^[0-9]+)(,([0-9]+(-[0-9]+)?|\\^[0-9]+))*");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PVolName

bool Validatable<Filter::Xml::PVolName>::validate(const Filter::Xml::PVolName::value_type& value_)
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

bool Traits<Filter::Xml::PPortNumber>::parse(const QString& src_, Filter::Xml::PPortNumber::value_type& dst_)
{
	bool output = false;
	dst_ = src_.toInt(&output);
	return output;
}

QString Traits<Filter::Xml::PPortNumber>::generate(Filter::Xml::PPortNumber::value_type src_)
{
	return QString::number(src_);
}

bool Validatable<Filter::Xml::PPortNumber>::validate(Filter::Xml::PPortNumber::value_type value_)
{
	if (-1 > value_)
		return false;

	if (65535 < value_)
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PIobase

bool Validatable<Filter::Xml::PIobase>::validate(const Filter::Xml::PIobase::value_type& value_)
{
	QRegExp q("0x[a-fA-F0-9]{1,4}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PIrq

bool Validatable<Filter::Xml::PIrq>::validate(const Filter::Xml::PIrq::value_type& value_)
{
	QRegExp q("0x[a-fA-F0-9]");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PData5604

bool Validatable<Filter::Xml::PData5604>::validate(const Filter::Xml::PData5604::value_type& value_)
{
	QRegExp q("mac[a-zA-Z0-9_\\.:\\-]{0,9}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PData5606

bool Validatable<Filter::Xml::PData5606>::validate(const Filter::Xml::PData5606::value_type& value_)
{
	QRegExp q("stp[a-zA-Z0-9_\\.:\\-]{0,9}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PData5608

bool Validatable<Filter::Xml::PData5608>::validate(const Filter::Xml::PData5608::value_type& value_)
{
	QRegExp q("vlan[a-zA-Z0-9_\\.:\\-]{0,8}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PData5610

bool Validatable<Filter::Xml::PData5610>::validate(const Filter::Xml::PData5610::value_type& value_)
{
	QRegExp q("arp[a-zA-Z0-9_\\.:\\-]{0,9}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PData5612

bool Validatable<Filter::Xml::PData5612>::validate(const Filter::Xml::PData5612::value_type& value_)
{
	QRegExp q("rarp[a-zA-Z0-9_\\.:\\-]{0,8}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PData5614

bool Validatable<Filter::Xml::PData5614>::validate(const Filter::Xml::PData5614::value_type& value_)
{
	QRegExp q("ipv4[a-zA-Z0-9_\\.:\\-]{0,8}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PData5616

bool Validatable<Filter::Xml::PData5616>::validate(const Filter::Xml::PData5616::value_type& value_)
{
	QRegExp q("ipv6[a-zA-Z0-9_\\.:\\-]{0,8}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PVariableNameType

bool Validatable<Filter::Xml::PVariableNameType>::validate(const Filter::Xml::PVariableNameType::value_type& value_)
{
	QRegExp q("$[a-zA-Z0-9_]+(\\[[ ]*[@]?[0-9]+[ ]*\\])?");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PData5619

bool Validatable<Filter::Xml::PData5619>::validate(const Filter::Xml::PData5619::value_type& value_)
{
	QRegExp q("([a-fA-F0-9]{1,2}:){5}[a-fA-F0-9]{1,2}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PData5622

bool Validatable<Filter::Xml::PData5622>::validate(const Filter::Xml::PData5622::value_type& value_)
{
	QRegExp q("([0-2]?[0-9]?[0-9]\\.){3}[0-2]?[0-9]?[0-9]");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PData5625

bool Validatable<Filter::Xml::PData5625>::validate(const Filter::Xml::PData5625::value_type& value_)
{
	QRegExp q("([a-fA-F0-9]{0,4}:){2,7}([a-fA-F0-9]*)(([0-2]?[0-9]?[0-9]\\.){3}[0-2]?[0-9]?[0-9])?");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PData5628

bool Traits<Filter::Xml::PData5628>::parse(const QString& src_, Filter::Xml::PData5628::value_type& dst_)
{
	bool output = false;
	dst_ = src_.toInt(&output);
	return output;
}

QString Traits<Filter::Xml::PData5628>::generate(Filter::Xml::PData5628::value_type src_)
{
	return QString::number(src_);
}

bool Validatable<Filter::Xml::PData5628>::validate(Filter::Xml::PData5628::value_type value_)
{
	if (0 > value_)
		return false;

	if (32 < value_)
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PData5630

bool Validatable<Filter::Xml::PData5630>::validate(const Filter::Xml::PData5630::value_type& value_)
{
	QRegExp q("([0-2]?[0-9]?[0-9]\\.){3}[0-2]?[0-9]?[0-9]");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PData5633

bool Traits<Filter::Xml::PData5633>::parse(const QString& src_, Filter::Xml::PData5633::value_type& dst_)
{
	bool output = false;
	dst_ = src_.toInt(&output);
	return output;
}

QString Traits<Filter::Xml::PData5633>::generate(Filter::Xml::PData5633::value_type src_)
{
	return QString::number(src_);
}

bool Validatable<Filter::Xml::PData5633>::validate(Filter::Xml::PData5633::value_type value_)
{
	if (0 > value_)
		return false;

	if (128 < value_)
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PData5635

bool Validatable<Filter::Xml::PData5635>::validate(const Filter::Xml::PData5635::value_type& value_)
{
	QRegExp q("([a-fA-F0-9]{0,4}:){2,7}([a-fA-F0-9]*)");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PData5637

bool Validatable<Filter::Xml::PData5637>::validate(const Filter::Xml::PData5637::value_type& value_)
{
	QRegExp q("0x([0-3][0-9a-fA-F]|[0-9a-fA-F])");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PData5640

bool Traits<Filter::Xml::PData5640>::parse(const QString& src_, Filter::Xml::PData5640::value_type& dst_)
{
	bool output = false;
	dst_ = src_.toInt(&output);
	return output;
}

QString Traits<Filter::Xml::PData5640>::generate(Filter::Xml::PData5640::value_type src_)
{
	return QString::number(src_);
}

bool Validatable<Filter::Xml::PData5640>::validate(Filter::Xml::PData5640::value_type value_)
{
	if (0 > value_)
		return false;

	if (63 < value_)
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PData5643

bool Validatable<Filter::Xml::PData5643>::validate(const Filter::Xml::PData5643::value_type& value_)
{
	QRegExp q("0x([6-9a-fA-F][0-9a-fA-F]{2}|[0-9a-fA-F]{4})");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PData5645

bool Traits<Filter::Xml::PData5645>::parse(const QString& src_, Filter::Xml::PData5645::value_type& dst_)
{
	bool output = false;
	dst_ = src_.toInt(&output);
	return output;
}

QString Traits<Filter::Xml::PData5645>::generate(Filter::Xml::PData5645::value_type src_)
{
	return QString::number(src_);
}

bool Validatable<Filter::Xml::PData5645>::validate(Filter::Xml::PData5645::value_type value_)
{
	if (1536 > value_)
		return false;

	if (65535 < value_)
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PData5650

bool Validatable<Filter::Xml::PData5650>::validate(const Filter::Xml::PData5650::value_type& value_)
{
	QRegExp q("0x([0-9a-fA-F]{1,3})");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PData5652

bool Traits<Filter::Xml::PData5652>::parse(const QString& src_, Filter::Xml::PData5652::value_type& dst_)
{
	bool output = false;
	dst_ = src_.toInt(&output);
	return output;
}

QString Traits<Filter::Xml::PData5652>::generate(Filter::Xml::PData5652::value_type src_)
{
	return QString::number(src_);
}

bool Validatable<Filter::Xml::PData5652>::validate(Filter::Xml::PData5652::value_type value_)
{
	if (0 > value_)
		return false;

	if (4095 < value_)
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PData5655

bool Validatable<Filter::Xml::PData5655>::validate(const Filter::Xml::PData5655::value_type& value_)
{
	QRegExp q("0x[0-9a-fA-F]{1,4}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PData5657

bool Traits<Filter::Xml::PData5657>::parse(const QString& src_, Filter::Xml::PData5657::value_type& dst_)
{
	bool output = false;
	dst_ = src_.toInt(&output);
	return output;
}

QString Traits<Filter::Xml::PData5657>::generate(Filter::Xml::PData5657::value_type src_)
{
	return QString::number(src_);
}

bool Validatable<Filter::Xml::PData5657>::validate(Filter::Xml::PData5657::value_type value_)
{
	if (0 > value_)
		return false;

	if (65535 < value_)
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PData5660

bool Validatable<Filter::Xml::PData5660>::validate(const Filter::Xml::PData5660::value_type& value_)
{
	QRegExp q("0x[0-9a-fA-F]{1,8}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PData5662

bool Traits<Filter::Xml::PData5662>::parse(const QString& src_, Filter::Xml::PData5662::value_type& dst_)
{
	bool output = false;
	dst_ = src_.toUInt(&output);
	return output;
}

QString Traits<Filter::Xml::PData5662>::generate(Filter::Xml::PData5662::value_type src_)
{
	return QString::number(src_);
}

///////////////////////////////////////////////////////////////////////////////
// struct PData5665

bool Traits<Filter::Xml::PData5665>::parse(const QString& src_, Filter::Xml::PData5665::value_type& dst_)
{
	bool output = false;
	dst_ = src_.toInt(&output);
	return output;
}

QString Traits<Filter::Xml::PData5665>::generate(Filter::Xml::PData5665::value_type src_)
{
	return QString::number(src_);
}

bool Validatable<Filter::Xml::PData5665>::validate(Filter::Xml::PData5665::value_type value_)
{
	if (0 > value_)
		return false;

	if (65535 < value_)
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PData5667

bool Validatable<Filter::Xml::PData5667>::validate(const Filter::Xml::PData5667::value_type& value_)
{
	QRegExp q("([Rr]eply|[Rr]equest|[Rr]equest_[Rr]everse|[Rr]eply_[Rr]everse|DRARP_[Rr]equest|DRARP_[Rr]eply|DRARP_[Ee]rror|InARP_[Rr]equest|ARP_NAK)");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PData5670

bool Validatable<Filter::Xml::PData5670>::validate(const Filter::Xml::PData5670::value_type& value_)
{
	QRegExp q("0x[0-9a-fA-F]{1,2}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PData5672

bool Traits<Filter::Xml::PData5672>::parse(const QString& src_, Filter::Xml::PData5672::value_type& dst_)
{
	bool output = false;
	dst_ = src_.toInt(&output);
	return output;
}

QString Traits<Filter::Xml::PData5672>::generate(Filter::Xml::PData5672::value_type src_)
{
	return QString::number(src_);
}

bool Validatable<Filter::Xml::PData5672>::validate(Filter::Xml::PData5672::value_type value_)
{
	if (0 > value_)
		return false;

	if (255 < value_)
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PFilterParamName

bool Validatable<Filter::Xml::PFilterParamName>::validate(const Filter::Xml::PFilterParamName::value_type& value_)
{
	QRegExp q("[a-zA-Z0-9_]+");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PFilterParamValue

bool Validatable<Filter::Xml::PFilterParamValue>::validate(const Filter::Xml::PFilterParamValue::value_type& value_)
{
	QRegExp q("[a-zA-Z0-9_\\.:]+");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PPriorityType

bool Traits<Filter::Xml::PPriorityType>::parse(const QString& src_, Filter::Xml::PPriorityType::value_type& dst_)
{
	bool output = false;
	dst_ = src_.toInt(&output);
	return output;
}

QString Traits<Filter::Xml::PPriorityType>::generate(Filter::Xml::PPriorityType::value_type src_)
{
	return QString::number(src_);
}

bool Validatable<Filter::Xml::PPriorityType>::validate(Filter::Xml::PPriorityType::value_type value_)
{
	if (-1000 > value_)
		return false;

	if (1000 < value_)
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PStatematchType

bool Validatable<Filter::Xml::PStatematchType>::validate(const Filter::Xml::PStatematchType::value_type& value_)
{
	QRegExp q("([Ff][Aa][Ll][Ss][Ee]|0)");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PStateflagsType

bool Validatable<Filter::Xml::PStateflagsType>::validate(const Filter::Xml::PStateflagsType::value_type& value_)
{
	QRegExp q("((NEW|ESTABLISHED|RELATED|INVALID)(,(NEW|ESTABLISHED|RELATED|INVALID))*|NONE)");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PTcpflagsType

bool Validatable<Filter::Xml::PTcpflagsType>::validate(const Filter::Xml::PTcpflagsType::value_type& value_)
{
	QRegExp q("((SYN|ACK|URG|PSH|FIN|RST)(,(SYN|ACK|URG|PSH|FIN|RST))*|ALL|NONE)/((SYN|ACK|URG|PSH|FIN|RST)(,(SYN|ACK|URG|PSH|FIN|RST))*|ALL|NONE)");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PData5677

bool Validatable<Filter::Xml::PData5677>::validate(const Filter::Xml::PData5677::value_type& value_)
{
	QRegExp q("[a-zA-Z0-9_\\.:\\-\\+ ]{1,31}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PIpsetFlagsType

bool Validatable<Filter::Xml::PIpsetFlagsType>::validate(const Filter::Xml::PIpsetFlagsType::value_type& value_)
{
	QRegExp q("(src|dst)(,(src|dst)){0,5}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct VChain

bool Traits<Filter::Xml::VChain>::parse(const QString& src_, Filter::Xml::VChain& dst_)
{
	int x;
	mpl::at_c<Filter::Xml::VChain::types, 0>::type a0;
	x = Marshal<Filter::Xml::EChain>::setString(src_, a0);
	if (0 < x)
	{
		dst_ = a0;
		return true;
	}
	mpl::at_c<Filter::Xml::VChain::types, 1>::type a1;
	x = Marshal<Filter::Xml::PData5604>::setString(src_, a1);
	if (0 < x)
	{
		dst_ = a1;
		return true;
	}
	mpl::at_c<Filter::Xml::VChain::types, 2>::type a2;
	x = Marshal<Filter::Xml::PData5606>::setString(src_, a2);
	if (0 < x)
	{
		dst_ = a2;
		return true;
	}
	mpl::at_c<Filter::Xml::VChain::types, 3>::type a3;
	x = Marshal<Filter::Xml::PData5608>::setString(src_, a3);
	if (0 < x)
	{
		dst_ = a3;
		return true;
	}
	mpl::at_c<Filter::Xml::VChain::types, 4>::type a4;
	x = Marshal<Filter::Xml::PData5610>::setString(src_, a4);
	if (0 < x)
	{
		dst_ = a4;
		return true;
	}
	mpl::at_c<Filter::Xml::VChain::types, 5>::type a5;
	x = Marshal<Filter::Xml::PData5612>::setString(src_, a5);
	if (0 < x)
	{
		dst_ = a5;
		return true;
	}
	mpl::at_c<Filter::Xml::VChain::types, 6>::type a6;
	x = Marshal<Filter::Xml::PData5614>::setString(src_, a6);
	if (0 < x)
	{
		dst_ = a6;
		return true;
	}
	mpl::at_c<Filter::Xml::VChain::types, 7>::type a7;
	x = Marshal<Filter::Xml::PData5616>::setString(src_, a7);
	if (0 < x)
	{
		dst_ = a7;
		return true;
	}

	return false;
}

QString Traits<Filter::Xml::VChain>::generate(const Filter::Xml::VChain& src_)
{
	switch (src_.which())
	{
	case 0:
		return Marshal<Filter::Xml::EChain>::getString(boost::get<mpl::at_c<Filter::Xml::VChain::types, 0>::type>(src_));
	case 1:
		return Marshal<Filter::Xml::PData5604>::getString(boost::get<mpl::at_c<Filter::Xml::VChain::types, 1>::type>(src_));
	case 2:
		return Marshal<Filter::Xml::PData5606>::getString(boost::get<mpl::at_c<Filter::Xml::VChain::types, 2>::type>(src_));
	case 3:
		return Marshal<Filter::Xml::PData5608>::getString(boost::get<mpl::at_c<Filter::Xml::VChain::types, 3>::type>(src_));
	case 4:
		return Marshal<Filter::Xml::PData5610>::getString(boost::get<mpl::at_c<Filter::Xml::VChain::types, 4>::type>(src_));
	case 5:
		return Marshal<Filter::Xml::PData5612>::getString(boost::get<mpl::at_c<Filter::Xml::VChain::types, 5>::type>(src_));
	case 6:
		return Marshal<Filter::Xml::PData5614>::getString(boost::get<mpl::at_c<Filter::Xml::VChain::types, 6>::type>(src_));
	case 7:
		return Marshal<Filter::Xml::PData5616>::getString(boost::get<mpl::at_c<Filter::Xml::VChain::types, 7>::type>(src_));
	}
	return QString();
}

///////////////////////////////////////////////////////////////////////////////
// struct VUUID

bool Traits<Filter::Xml::VUUID>::parse(const QString& src_, Filter::Xml::VUUID& dst_)
{
	int x;
	mpl::at_c<Filter::Xml::VUUID::types, 0>::type a0;
	x = Marshal<Filter::Xml::PData5579>::setString(src_, a0);
	if (0 < x)
	{
		dst_ = a0;
		return true;
	}
	mpl::at_c<Filter::Xml::VUUID::types, 1>::type a1;
	x = Marshal<Filter::Xml::PData5581>::setString(src_, a1);
	if (0 < x)
	{
		dst_ = a1;
		return true;
	}

	return false;
}

QString Traits<Filter::Xml::VUUID>::generate(const Filter::Xml::VUUID& src_)
{
	switch (src_.which())
	{
	case 0:
		return Marshal<Filter::Xml::PData5579>::getString(boost::get<mpl::at_c<Filter::Xml::VUUID::types, 0>::type>(src_));
	case 1:
		return Marshal<Filter::Xml::PData5581>::getString(boost::get<mpl::at_c<Filter::Xml::VUUID::types, 1>::type>(src_));
	}
	return QString();
}

///////////////////////////////////////////////////////////////////////////////
// struct VAddrMAC

bool Traits<Filter::Xml::VAddrMAC>::parse(const QString& src_, Filter::Xml::VAddrMAC& dst_)
{
	int x;
	mpl::at_c<Filter::Xml::VAddrMAC::types, 0>::type a0;
	x = Marshal<Filter::Xml::PVariableNameType>::setString(src_, a0);
	if (0 < x)
	{
		dst_ = a0;
		return true;
	}
	mpl::at_c<Filter::Xml::VAddrMAC::types, 1>::type a1;
	x = Marshal<Filter::Xml::PData5619>::setString(src_, a1);
	if (0 < x)
	{
		dst_ = a1;
		return true;
	}

	return false;
}

QString Traits<Filter::Xml::VAddrMAC>::generate(const Filter::Xml::VAddrMAC& src_)
{
	switch (src_.which())
	{
	case 0:
		return Marshal<Filter::Xml::PVariableNameType>::getString(boost::get<mpl::at_c<Filter::Xml::VAddrMAC::types, 0>::type>(src_));
	case 1:
		return Marshal<Filter::Xml::PData5619>::getString(boost::get<mpl::at_c<Filter::Xml::VAddrMAC::types, 1>::type>(src_));
	}
	return QString();
}

///////////////////////////////////////////////////////////////////////////////
// struct VMacProtocolid

bool Traits<Filter::Xml::VMacProtocolid>::parse(const QString& src_, Filter::Xml::VMacProtocolid& dst_)
{
	int x;
	mpl::at_c<Filter::Xml::VMacProtocolid::types, 0>::type a0;
	x = Marshal<Filter::Xml::PVariableNameType>::setString(src_, a0);
	if (0 < x)
	{
		dst_ = a0;
		return true;
	}
	mpl::at_c<Filter::Xml::VMacProtocolid::types, 1>::type a1;
	x = Marshal<Filter::Xml::PData5643>::setString(src_, a1);
	if (0 < x)
	{
		dst_ = a1;
		return true;
	}
	mpl::at_c<Filter::Xml::VMacProtocolid::types, 2>::type a2;
	x = Marshal<Filter::Xml::PData5645>::setString(src_, a2);
	if (0 < x)
	{
		dst_ = a2;
		return true;
	}
	mpl::at_c<Filter::Xml::VMacProtocolid::types, 3>::type a3;
	x = Marshal<Filter::Xml::EChoice5647>::setString(src_, a3);
	if (0 < x)
	{
		dst_ = a3;
		return true;
	}

	return false;
}

QString Traits<Filter::Xml::VMacProtocolid>::generate(const Filter::Xml::VMacProtocolid& src_)
{
	switch (src_.which())
	{
	case 0:
		return Marshal<Filter::Xml::PVariableNameType>::getString(boost::get<mpl::at_c<Filter::Xml::VMacProtocolid::types, 0>::type>(src_));
	case 1:
		return Marshal<Filter::Xml::PData5643>::getString(boost::get<mpl::at_c<Filter::Xml::VMacProtocolid::types, 1>::type>(src_));
	case 2:
		return Marshal<Filter::Xml::PData5645>::getString(boost::get<mpl::at_c<Filter::Xml::VMacProtocolid::types, 2>::type>(src_));
	case 3:
		return Marshal<Filter::Xml::EChoice5647>::getString(boost::get<mpl::at_c<Filter::Xml::VMacProtocolid::types, 3>::type>(src_));
	}
	return QString();
}

///////////////////////////////////////////////////////////////////////////////
// struct VVlanVlanid

bool Traits<Filter::Xml::VVlanVlanid>::parse(const QString& src_, Filter::Xml::VVlanVlanid& dst_)
{
	int x;
	mpl::at_c<Filter::Xml::VVlanVlanid::types, 0>::type a0;
	x = Marshal<Filter::Xml::PVariableNameType>::setString(src_, a0);
	if (0 < x)
	{
		dst_ = a0;
		return true;
	}
	mpl::at_c<Filter::Xml::VVlanVlanid::types, 1>::type a1;
	x = Marshal<Filter::Xml::PData5650>::setString(src_, a1);
	if (0 < x)
	{
		dst_ = a1;
		return true;
	}
	mpl::at_c<Filter::Xml::VVlanVlanid::types, 2>::type a2;
	x = Marshal<Filter::Xml::PData5652>::setString(src_, a2);
	if (0 < x)
	{
		dst_ = a2;
		return true;
	}

	return false;
}

QString Traits<Filter::Xml::VVlanVlanid>::generate(const Filter::Xml::VVlanVlanid& src_)
{
	switch (src_.which())
	{
	case 0:
		return Marshal<Filter::Xml::PVariableNameType>::getString(boost::get<mpl::at_c<Filter::Xml::VVlanVlanid::types, 0>::type>(src_));
	case 1:
		return Marshal<Filter::Xml::PData5650>::getString(boost::get<mpl::at_c<Filter::Xml::VVlanVlanid::types, 1>::type>(src_));
	case 2:
		return Marshal<Filter::Xml::PData5652>::getString(boost::get<mpl::at_c<Filter::Xml::VVlanVlanid::types, 2>::type>(src_));
	}
	return QString();
}

///////////////////////////////////////////////////////////////////////////////
// struct VUint8range

bool Traits<Filter::Xml::VUint8range>::parse(const QString& src_, Filter::Xml::VUint8range& dst_)
{
	int x;
	mpl::at_c<Filter::Xml::VUint8range::types, 0>::type a0;
	x = Marshal<Filter::Xml::PData5571>::setString(src_, a0);
	if (0 < x)
	{
		dst_ = a0;
		return true;
	}
	mpl::at_c<Filter::Xml::VUint8range::types, 1>::type a1;
	x = Marshal<Filter::Xml::PData5573>::setString(src_, a1);
	if (0 < x)
	{
		dst_ = a1;
		return true;
	}

	return false;
}

QString Traits<Filter::Xml::VUint8range>::generate(const Filter::Xml::VUint8range& src_)
{
	switch (src_.which())
	{
	case 0:
		return Marshal<Filter::Xml::PData5571>::getString(boost::get<mpl::at_c<Filter::Xml::VUint8range::types, 0>::type>(src_));
	case 1:
		return Marshal<Filter::Xml::PData5573>::getString(boost::get<mpl::at_c<Filter::Xml::VUint8range::types, 1>::type>(src_));
	}
	return QString();
}

///////////////////////////////////////////////////////////////////////////////
// struct VUint16range

bool Traits<Filter::Xml::VUint16range>::parse(const QString& src_, Filter::Xml::VUint16range& dst_)
{
	int x;
	mpl::at_c<Filter::Xml::VUint16range::types, 0>::type a0;
	x = Marshal<Filter::Xml::PVariableNameType>::setString(src_, a0);
	if (0 < x)
	{
		dst_ = a0;
		return true;
	}
	mpl::at_c<Filter::Xml::VUint16range::types, 1>::type a1;
	x = Marshal<Filter::Xml::PData5655>::setString(src_, a1);
	if (0 < x)
	{
		dst_ = a1;
		return true;
	}
	mpl::at_c<Filter::Xml::VUint16range::types, 2>::type a2;
	x = Marshal<Filter::Xml::PData5657>::setString(src_, a2);
	if (0 < x)
	{
		dst_ = a2;
		return true;
	}

	return false;
}

QString Traits<Filter::Xml::VUint16range>::generate(const Filter::Xml::VUint16range& src_)
{
	switch (src_.which())
	{
	case 0:
		return Marshal<Filter::Xml::PVariableNameType>::getString(boost::get<mpl::at_c<Filter::Xml::VUint16range::types, 0>::type>(src_));
	case 1:
		return Marshal<Filter::Xml::PData5655>::getString(boost::get<mpl::at_c<Filter::Xml::VUint16range::types, 1>::type>(src_));
	case 2:
		return Marshal<Filter::Xml::PData5657>::getString(boost::get<mpl::at_c<Filter::Xml::VUint16range::types, 2>::type>(src_));
	}
	return QString();
}

///////////////////////////////////////////////////////////////////////////////
// struct VUint32range

bool Traits<Filter::Xml::VUint32range>::parse(const QString& src_, Filter::Xml::VUint32range& dst_)
{
	int x;
	mpl::at_c<Filter::Xml::VUint32range::types, 0>::type a0;
	x = Marshal<Filter::Xml::PVariableNameType>::setString(src_, a0);
	if (0 < x)
	{
		dst_ = a0;
		return true;
	}
	mpl::at_c<Filter::Xml::VUint32range::types, 1>::type a1;
	x = Marshal<Filter::Xml::PData5660>::setString(src_, a1);
	if (0 < x)
	{
		dst_ = a1;
		return true;
	}
	mpl::at_c<Filter::Xml::VUint32range::types, 2>::type a2;
	x = Marshal<Filter::Xml::PData5662>::setString(src_, a2);
	if (0 < x)
	{
		dst_ = a2;
		return true;
	}

	return false;
}

QString Traits<Filter::Xml::VUint32range>::generate(const Filter::Xml::VUint32range& src_)
{
	switch (src_.which())
	{
	case 0:
		return Marshal<Filter::Xml::PVariableNameType>::getString(boost::get<mpl::at_c<Filter::Xml::VUint32range::types, 0>::type>(src_));
	case 1:
		return Marshal<Filter::Xml::PData5660>::getString(boost::get<mpl::at_c<Filter::Xml::VUint32range::types, 1>::type>(src_));
	case 2:
		return Marshal<Filter::Xml::PData5662>::getString(boost::get<mpl::at_c<Filter::Xml::VUint32range::types, 2>::type>(src_));
	}
	return QString();
}

///////////////////////////////////////////////////////////////////////////////
// struct VAddrIP

bool Traits<Filter::Xml::VAddrIP>::parse(const QString& src_, Filter::Xml::VAddrIP& dst_)
{
	int x;
	mpl::at_c<Filter::Xml::VAddrIP::types, 0>::type a0;
	x = Marshal<Filter::Xml::PVariableNameType>::setString(src_, a0);
	if (0 < x)
	{
		dst_ = a0;
		return true;
	}
	mpl::at_c<Filter::Xml::VAddrIP::types, 1>::type a1;
	x = Marshal<Filter::Xml::PData5622>::setString(src_, a1);
	if (0 < x)
	{
		dst_ = a1;
		return true;
	}

	return false;
}

QString Traits<Filter::Xml::VAddrIP>::generate(const Filter::Xml::VAddrIP& src_)
{
	switch (src_.which())
	{
	case 0:
		return Marshal<Filter::Xml::PVariableNameType>::getString(boost::get<mpl::at_c<Filter::Xml::VAddrIP::types, 0>::type>(src_));
	case 1:
		return Marshal<Filter::Xml::PData5622>::getString(boost::get<mpl::at_c<Filter::Xml::VAddrIP::types, 1>::type>(src_));
	}
	return QString();
}

///////////////////////////////////////////////////////////////////////////////
// struct VArpOpcodeType

bool Traits<Filter::Xml::VArpOpcodeType>::parse(const QString& src_, Filter::Xml::VArpOpcodeType& dst_)
{
	int x;
	mpl::at_c<Filter::Xml::VArpOpcodeType::types, 0>::type a0;
	x = Marshal<Filter::Xml::PVariableNameType>::setString(src_, a0);
	if (0 < x)
	{
		dst_ = a0;
		return true;
	}
	mpl::at_c<Filter::Xml::VArpOpcodeType::types, 1>::type a1;
	x = Marshal<Filter::Xml::PData5665>::setString(src_, a1);
	if (0 < x)
	{
		dst_ = a1;
		return true;
	}
	mpl::at_c<Filter::Xml::VArpOpcodeType::types, 2>::type a2;
	x = Marshal<Filter::Xml::PData5667>::setString(src_, a2);
	if (0 < x)
	{
		dst_ = a2;
		return true;
	}

	return false;
}

QString Traits<Filter::Xml::VArpOpcodeType>::generate(const Filter::Xml::VArpOpcodeType& src_)
{
	switch (src_.which())
	{
	case 0:
		return Marshal<Filter::Xml::PVariableNameType>::getString(boost::get<mpl::at_c<Filter::Xml::VArpOpcodeType::types, 0>::type>(src_));
	case 1:
		return Marshal<Filter::Xml::PData5665>::getString(boost::get<mpl::at_c<Filter::Xml::VArpOpcodeType::types, 1>::type>(src_));
	case 2:
		return Marshal<Filter::Xml::PData5667>::getString(boost::get<mpl::at_c<Filter::Xml::VArpOpcodeType::types, 2>::type>(src_));
	}
	return QString();
}

///////////////////////////////////////////////////////////////////////////////
// struct VAddrMask

bool Traits<Filter::Xml::VAddrMask>::parse(const QString& src_, Filter::Xml::VAddrMask& dst_)
{
	int x;
	mpl::at_c<Filter::Xml::VAddrMask::types, 0>::type a0;
	x = Marshal<Filter::Xml::PVariableNameType>::setString(src_, a0);
	if (0 < x)
	{
		dst_ = a0;
		return true;
	}
	mpl::at_c<Filter::Xml::VAddrMask::types, 1>::type a1;
	x = Marshal<Filter::Xml::PData5628>::setString(src_, a1);
	if (0 < x)
	{
		dst_ = a1;
		return true;
	}
	mpl::at_c<Filter::Xml::VAddrMask::types, 2>::type a2;
	x = Marshal<Filter::Xml::PData5630>::setString(src_, a2);
	if (0 < x)
	{
		dst_ = a2;
		return true;
	}

	return false;
}

QString Traits<Filter::Xml::VAddrMask>::generate(const Filter::Xml::VAddrMask& src_)
{
	switch (src_.which())
	{
	case 0:
		return Marshal<Filter::Xml::PVariableNameType>::getString(boost::get<mpl::at_c<Filter::Xml::VAddrMask::types, 0>::type>(src_));
	case 1:
		return Marshal<Filter::Xml::PData5628>::getString(boost::get<mpl::at_c<Filter::Xml::VAddrMask::types, 1>::type>(src_));
	case 2:
		return Marshal<Filter::Xml::PData5630>::getString(boost::get<mpl::at_c<Filter::Xml::VAddrMask::types, 2>::type>(src_));
	}
	return QString();
}

///////////////////////////////////////////////////////////////////////////////
// struct VIpProtocolType

bool Traits<Filter::Xml::VIpProtocolType>::parse(const QString& src_, Filter::Xml::VIpProtocolType& dst_)
{
	int x;
	mpl::at_c<Filter::Xml::VIpProtocolType::types, 0>::type a0;
	x = Marshal<Filter::Xml::PVariableNameType>::setString(src_, a0);
	if (0 < x)
	{
		dst_ = a0;
		return true;
	}
	mpl::at_c<Filter::Xml::VIpProtocolType::types, 1>::type a1;
	x = Marshal<Filter::Xml::PData5670>::setString(src_, a1);
	if (0 < x)
	{
		dst_ = a1;
		return true;
	}
	mpl::at_c<Filter::Xml::VIpProtocolType::types, 2>::type a2;
	x = Marshal<Filter::Xml::PData5672>::setString(src_, a2);
	if (0 < x)
	{
		dst_ = a2;
		return true;
	}
	mpl::at_c<Filter::Xml::VIpProtocolType::types, 3>::type a3;
	x = Marshal<Filter::Xml::EChoice5674>::setString(src_, a3);
	if (0 < x)
	{
		dst_ = a3;
		return true;
	}

	return false;
}

QString Traits<Filter::Xml::VIpProtocolType>::generate(const Filter::Xml::VIpProtocolType& src_)
{
	switch (src_.which())
	{
	case 0:
		return Marshal<Filter::Xml::PVariableNameType>::getString(boost::get<mpl::at_c<Filter::Xml::VIpProtocolType::types, 0>::type>(src_));
	case 1:
		return Marshal<Filter::Xml::PData5670>::getString(boost::get<mpl::at_c<Filter::Xml::VIpProtocolType::types, 1>::type>(src_));
	case 2:
		return Marshal<Filter::Xml::PData5672>::getString(boost::get<mpl::at_c<Filter::Xml::VIpProtocolType::types, 2>::type>(src_));
	case 3:
		return Marshal<Filter::Xml::EChoice5674>::getString(boost::get<mpl::at_c<Filter::Xml::VIpProtocolType::types, 3>::type>(src_));
	}
	return QString();
}

///////////////////////////////////////////////////////////////////////////////
// struct VSixbitrange

bool Traits<Filter::Xml::VSixbitrange>::parse(const QString& src_, Filter::Xml::VSixbitrange& dst_)
{
	int x;
	mpl::at_c<Filter::Xml::VSixbitrange::types, 0>::type a0;
	x = Marshal<Filter::Xml::PData5637>::setString(src_, a0);
	if (0 < x)
	{
		dst_ = a0;
		return true;
	}
	mpl::at_c<Filter::Xml::VSixbitrange::types, 1>::type a1;
	x = Marshal<Filter::Xml::PVariableNameType>::setString(src_, a1);
	if (0 < x)
	{
		dst_ = a1;
		return true;
	}
	mpl::at_c<Filter::Xml::VSixbitrange::types, 2>::type a2;
	x = Marshal<Filter::Xml::PData5640>::setString(src_, a2);
	if (0 < x)
	{
		dst_ = a2;
		return true;
	}

	return false;
}

QString Traits<Filter::Xml::VSixbitrange>::generate(const Filter::Xml::VSixbitrange& src_)
{
	switch (src_.which())
	{
	case 0:
		return Marshal<Filter::Xml::PData5637>::getString(boost::get<mpl::at_c<Filter::Xml::VSixbitrange::types, 0>::type>(src_));
	case 1:
		return Marshal<Filter::Xml::PVariableNameType>::getString(boost::get<mpl::at_c<Filter::Xml::VSixbitrange::types, 1>::type>(src_));
	case 2:
		return Marshal<Filter::Xml::PData5640>::getString(boost::get<mpl::at_c<Filter::Xml::VSixbitrange::types, 2>::type>(src_));
	}
	return QString();
}

///////////////////////////////////////////////////////////////////////////////
// struct VAddrIPv6

bool Traits<Filter::Xml::VAddrIPv6>::parse(const QString& src_, Filter::Xml::VAddrIPv6& dst_)
{
	int x;
	mpl::at_c<Filter::Xml::VAddrIPv6::types, 0>::type a0;
	x = Marshal<Filter::Xml::PVariableNameType>::setString(src_, a0);
	if (0 < x)
	{
		dst_ = a0;
		return true;
	}
	mpl::at_c<Filter::Xml::VAddrIPv6::types, 1>::type a1;
	x = Marshal<Filter::Xml::PData5625>::setString(src_, a1);
	if (0 < x)
	{
		dst_ = a1;
		return true;
	}

	return false;
}

QString Traits<Filter::Xml::VAddrIPv6>::generate(const Filter::Xml::VAddrIPv6& src_)
{
	switch (src_.which())
	{
	case 0:
		return Marshal<Filter::Xml::PVariableNameType>::getString(boost::get<mpl::at_c<Filter::Xml::VAddrIPv6::types, 0>::type>(src_));
	case 1:
		return Marshal<Filter::Xml::PData5625>::getString(boost::get<mpl::at_c<Filter::Xml::VAddrIPv6::types, 1>::type>(src_));
	}
	return QString();
}

///////////////////////////////////////////////////////////////////////////////
// struct VAddrMaskv6

bool Traits<Filter::Xml::VAddrMaskv6>::parse(const QString& src_, Filter::Xml::VAddrMaskv6& dst_)
{
	int x;
	mpl::at_c<Filter::Xml::VAddrMaskv6::types, 0>::type a0;
	x = Marshal<Filter::Xml::PVariableNameType>::setString(src_, a0);
	if (0 < x)
	{
		dst_ = a0;
		return true;
	}
	mpl::at_c<Filter::Xml::VAddrMaskv6::types, 1>::type a1;
	x = Marshal<Filter::Xml::PData5633>::setString(src_, a1);
	if (0 < x)
	{
		dst_ = a1;
		return true;
	}
	mpl::at_c<Filter::Xml::VAddrMaskv6::types, 2>::type a2;
	x = Marshal<Filter::Xml::PData5635>::setString(src_, a2);
	if (0 < x)
	{
		dst_ = a2;
		return true;
	}

	return false;
}

QString Traits<Filter::Xml::VAddrMaskv6>::generate(const Filter::Xml::VAddrMaskv6& src_)
{
	switch (src_.which())
	{
	case 0:
		return Marshal<Filter::Xml::PVariableNameType>::getString(boost::get<mpl::at_c<Filter::Xml::VAddrMaskv6::types, 0>::type>(src_));
	case 1:
		return Marshal<Filter::Xml::PData5633>::getString(boost::get<mpl::at_c<Filter::Xml::VAddrMaskv6::types, 1>::type>(src_));
	case 2:
		return Marshal<Filter::Xml::PData5635>::getString(boost::get<mpl::at_c<Filter::Xml::VAddrMaskv6::types, 2>::type>(src_));
	}
	return QString();
}

///////////////////////////////////////////////////////////////////////////////
// struct VIpsetNameType

bool Traits<Filter::Xml::VIpsetNameType>::parse(const QString& src_, Filter::Xml::VIpsetNameType& dst_)
{
	int x;
	mpl::at_c<Filter::Xml::VIpsetNameType::types, 0>::type a0;
	x = Marshal<Filter::Xml::PVariableNameType>::setString(src_, a0);
	if (0 < x)
	{
		dst_ = a0;
		return true;
	}
	mpl::at_c<Filter::Xml::VIpsetNameType::types, 1>::type a1;
	x = Marshal<Filter::Xml::PData5677>::setString(src_, a1);
	if (0 < x)
	{
		dst_ = a1;
		return true;
	}

	return false;
}

QString Traits<Filter::Xml::VIpsetNameType>::generate(const Filter::Xml::VIpsetNameType& src_)
{
	switch (src_.which())
	{
	case 0:
		return Marshal<Filter::Xml::PVariableNameType>::getString(boost::get<mpl::at_c<Filter::Xml::VIpsetNameType::types, 0>::type>(src_));
	case 1:
		return Marshal<Filter::Xml::PData5677>::getString(boost::get<mpl::at_c<Filter::Xml::VIpsetNameType::types, 1>::type>(src_));
	}
	return QString();
}

} // namespace Libvirt
