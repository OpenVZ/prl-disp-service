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

#include "domain_data.h"
#include <QRegExp>

namespace Libvirt
{
///////////////////////////////////////////////////////////////////////////////
// struct PUnsignedInt

bool Traits<Domain::Xml::PUnsignedInt>::parse(const QString& src_, Domain::Xml::PUnsignedInt::value_type& dst_)
{
	QRegExp q("[0-9]+");
	if (!q.exactMatch(src_))
		return false;
	bool output = false;
	dst_ = src_.toUInt(&output);
	return output;
}

QString Traits<Domain::Xml::PUnsignedInt>::generate(Domain::Xml::PUnsignedInt::value_type src_)
{
	return QString::number(src_);
}

///////////////////////////////////////////////////////////////////////////////
// struct PUnsignedLong

bool Traits<Domain::Xml::PUnsignedLong>::parse(const QString& src_, Domain::Xml::PUnsignedLong::value_type& dst_)
{
	QRegExp q("[0-9]+");
	if (!q.exactMatch(src_))
		return false;
	bool output = false;
	dst_ = src_.toULong(&output);
	return output;
}

QString Traits<Domain::Xml::PUnsignedLong>::generate(Domain::Xml::PUnsignedLong::value_type src_)
{
	return QString::number(src_);
}

///////////////////////////////////////////////////////////////////////////////
// struct PHexuint

bool Validatable<Domain::Xml::PHexuint>::validate(const Domain::Xml::PHexuint::value_type& value_)
{
	QRegExp q("(0x)?[0-9a-f]+");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PPositiveInteger

bool Traits<Domain::Xml::PPositiveInteger>::parse(const QString& src_, Domain::Xml::PPositiveInteger::value_type& dst_)
{
	QRegExp q("[0-9]+");
	if (!q.exactMatch(src_))
		return false;
	bool output = false;
	dst_ = src_.toUInt(&output);
	return output;
}

QString Traits<Domain::Xml::PPositiveInteger>::generate(Domain::Xml::PPositiveInteger::value_type src_)
{
	return QString::number(src_);
}

///////////////////////////////////////////////////////////////////////////////
// struct POctalMode

bool Traits<Domain::Xml::POctalMode>::parse(const QString& src_, Domain::Xml::POctalMode::value_type& dst_)
{
	QRegExp q("[0-7]+");
	if (!q.exactMatch(src_))
		return false;
	bool output = false;
	dst_ = src_.toUInt(&output);
	return output;
}

QString Traits<Domain::Xml::POctalMode>::generate(Domain::Xml::POctalMode::value_type src_)
{
	return QString::number(src_);
}

///////////////////////////////////////////////////////////////////////////////
// struct PData7

bool Validatable<Domain::Xml::PData7>::validate(const Domain::Xml::PData7::value_type& value_)
{
	QRegExp q("0x[0-9a-fA-F]{1,2}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PData9

bool Traits<Domain::Xml::PData9>::parse(const QString& src_, Domain::Xml::PData9::value_type& dst_)
{
	bool output = false;
	dst_ = src_.toInt(&output);
	return output;
}

QString Traits<Domain::Xml::PData9>::generate(Domain::Xml::PData9::value_type src_)
{
	return QString::number(src_);
}

bool Validatable<Domain::Xml::PData9>::validate(Domain::Xml::PData9::value_type value_)
{
	if (0 > value_)
		return false;

	if (255 < value_)
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PData12

bool Validatable<Domain::Xml::PData12>::validate(const Domain::Xml::PData12::value_type& value_)
{
	QRegExp q("0x[0-9a-fA-F]{1,6}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PData14

bool Traits<Domain::Xml::PData14>::parse(const QString& src_, Domain::Xml::PData14::value_type& dst_)
{
	bool output = false;
	dst_ = src_.toInt(&output);
	return output;
}

QString Traits<Domain::Xml::PData14>::generate(Domain::Xml::PData14::value_type src_)
{
	return QString::number(src_);
}

bool Validatable<Domain::Xml::PData14>::validate(Domain::Xml::PData14::value_type value_)
{
	if (0 > value_)
		return false;

	if (16777215 < value_)
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PData17

bool Validatable<Domain::Xml::PData17>::validate(const Domain::Xml::PData17::value_type& value_)
{
	QRegExp q("[a-fA-F0-9]{32}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PData19

bool Validatable<Domain::Xml::PData19>::validate(const Domain::Xml::PData19::value_type& value_)
{
	QRegExp q("[a-fA-F0-9]{8}\\-([a-fA-F0-9]{4}\\-){3}[a-fA-F0-9]{12}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PUniMacAddr

bool Validatable<Domain::Xml::PUniMacAddr>::validate(const Domain::Xml::PUniMacAddr::value_type& value_)
{
	QRegExp q("[a-fA-F0-9][02468aAcCeE](:[a-fA-F0-9]{2}){5}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PMultiMacAddr

bool Validatable<Domain::Xml::PMultiMacAddr>::validate(const Domain::Xml::PMultiMacAddr::value_type& value_)
{
	QRegExp q("[a-fA-F0-9][13579bBdDfF](:[a-fA-F0-9]{2}){5}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PMacAddr

bool Validatable<Domain::Xml::PMacAddr>::validate(const Domain::Xml::PMacAddr::value_type& value_)
{
	QRegExp q("[a-fA-F0-9]{2}(:[a-fA-F0-9]{2}){5}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PDuidLLT

bool Validatable<Domain::Xml::PDuidLLT>::validate(const Domain::Xml::PDuidLLT::value_type& value_)
{
	QRegExp q("[0]{1,2}:[0]{0,1}[1]:[0]{1,2}:[0]{0,1}[a-fA-F1-9](:[a-fA-F0-9]{1,2}){4}(:[a-fA-F0-9]{1,2}){6,8}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PDuidEN

bool Validatable<Domain::Xml::PDuidEN>::validate(const Domain::Xml::PDuidEN::value_type& value_)
{
	QRegExp q("[0]{1,2}:[0]{0,1}[2](:[a-fA-F0-9]{1,2}){4}(:[a-fA-F0-9]{1,2}){1,124}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PDuidLL

bool Validatable<Domain::Xml::PDuidLL>::validate(const Domain::Xml::PDuidLL::value_type& value_)
{
	QRegExp q("[0]{1,2}:[0]{0,1}[3]:[0]{1,2}:[0]{0,1}[a-fA-F1-9](:[a-fA-F0-9]{1,2}){6,8}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PDuidUUID

bool Validatable<Domain::Xml::PDuidUUID>::validate(const Domain::Xml::PDuidUUID::value_type& value_)
{
	QRegExp q("[0]{1,2}:[0]{0,1}[4](:[a-fA-F0-9]{1,2}){16}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PIpv4Addr

bool Validatable<Domain::Xml::PIpv4Addr>::validate(const Domain::Xml::PIpv4Addr::value_type& value_)
{
	QRegExp q("(((25[0-5])|(2[0-4][0-9])|(1[0-9]{2})|([1-9][0-9])|([0-9]))\\.){3}((25[0-5])|(2[0-4][0-9])|(1[0-9]{2})|([1-9][0-9])|([0-9]))");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PIpv6Addr

bool Validatable<Domain::Xml::PIpv6Addr>::validate(const Domain::Xml::PIpv6Addr::value_type& value_)
{
	QRegExp q("(([0-9A-Fa-f]{1,4}:){7}[0-9A-Fa-f]{1,4})|(([0-9A-Fa-f]{1,4}:){6}:[0-9A-Fa-f]{1,4})|(([0-9A-Fa-f]{1,4}:){5}:([0-9A-Fa-f]{1,4}:)?[0-9A-Fa-f]{1,4})|(([0-9A-Fa-f]{1,4}:){4}:([0-9A-Fa-f]{1,4}:){0,2}[0-9A-Fa-f]{1,4})|(([0-9A-Fa-f]{1,4}:){3}:([0-9A-Fa-f]{1,4}:){0,3}[0-9A-Fa-f]{1,4})|(([0-9A-Fa-f]{1,4}:){2}:([0-9A-Fa-f]{1,4}:){0,4}[0-9A-Fa-f]{1,4})|(([0-9A-Fa-f]{1,4}:){6}(((25[0-5])|(2[0-4][0-9])|(1[0-9]{2})|([1-9][0-9])|([0-9]))\\.){3}((25[0-5])|(2[0-4][0-9])|(1[0-9]{2})|([1-9][0-9])|([0-9])))|(([0-9A-Fa-f]{1,4}:){0,5}:(((25[0-5])|(2[0-4][0-9])|(1[0-9]{2})|([1-9][0-9])|([0-9]))\\.){3}((25[0-5])|(2[0-4][0-9])|(1[0-9]{2})|([1-9][0-9])|([0-9])))|(::([0-9A-Fa-f]{1,4}:){0,5}(((25[0-5])|(2[0-4][0-9])|(1[0-9]{2})|([1-9][0-9])|([0-9]))\\.){3}((25[0-5])|(2[0-4][0-9])|(1[0-9]{2})|([1-9][0-9])|([0-9])))|([0-9A-Fa-f]{1,4}::([0-9A-Fa-f]{1,4}:){0,5}[0-9A-Fa-f]{1,4})|(::([0-9A-Fa-f]{1,4}:){0,6}[0-9A-Fa-f]{1,4})|(([0-9A-Fa-f]{1,4}:){1,7}:)");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PIpv4Prefix

bool Traits<Domain::Xml::PIpv4Prefix>::parse(const QString& src_, Domain::Xml::PIpv4Prefix::value_type& dst_)
{
	bool output = false;
	dst_ = src_.toUInt(&output);
	return output;
}

QString Traits<Domain::Xml::PIpv4Prefix>::generate(Domain::Xml::PIpv4Prefix::value_type src_)
{
	return QString::number(src_);
}

bool Validatable<Domain::Xml::PIpv4Prefix>::validate(Domain::Xml::PIpv4Prefix::value_type value_)
{
	if (32 < value_)
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PIpv6Prefix

bool Traits<Domain::Xml::PIpv6Prefix>::parse(const QString& src_, Domain::Xml::PIpv6Prefix::value_type& dst_)
{
	bool output = false;
	dst_ = src_.toUInt(&output);
	return output;
}

QString Traits<Domain::Xml::PIpv6Prefix>::generate(Domain::Xml::PIpv6Prefix::value_type src_)
{
	return QString::number(src_);
}

bool Validatable<Domain::Xml::PIpv6Prefix>::validate(Domain::Xml::PIpv6Prefix::value_type value_)
{
	if (128 < value_)
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PGenericName

bool Validatable<Domain::Xml::PGenericName>::validate(const Domain::Xml::PGenericName::value_type& value_)
{
	QRegExp q("[a-zA-Z0-9_\\+\\-]+");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PDnsName

bool Validatable<Domain::Xml::PDnsName>::validate(const Domain::Xml::PDnsName::value_type& value_)
{
	QRegExp q("[a-zA-Z0-9\\.\\-]+");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PDeviceName

bool Validatable<Domain::Xml::PDeviceName>::validate(const Domain::Xml::PDeviceName::value_type& value_)
{
	QRegExp q("[a-zA-Z0-9_\\.\\-\\\\:/]+");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PFilePath

bool Validatable<Domain::Xml::PFilePath>::validate(const Domain::Xml::PFilePath::value_type& value_)
{
	QRegExp q("[a-zA-Z0-9_\\.\\+\\-\\\\&\"{}'<>/%: ]+");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PDirPath

bool Validatable<Domain::Xml::PDirPath>::validate(const Domain::Xml::PDirPath::value_type& value_)
{
	QRegExp q("[a-zA-Z0-9_\\.\\+\\-\\\\&\"{}'<>/%: ]+");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PAbsFilePath

bool Validatable<Domain::Xml::PAbsFilePath>::validate(const Domain::Xml::PAbsFilePath::value_type& value_)
{
	QRegExp q("/[a-zA-Z0-9_\\.\\+\\-\\\\&\"{}'<>/%,: ]+");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PAbsDirPath

bool Validatable<Domain::Xml::PAbsDirPath>::validate(const Domain::Xml::PAbsDirPath::value_type& value_)
{
	QRegExp q("/[a-zA-Z0-9_\\.\\+\\-\\\\&\"{}'<>/%: ]*");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PUnit

bool Validatable<Domain::Xml::PUnit>::validate(const Domain::Xml::PUnit::value_type& value_)
{
	QRegExp q("([bB]([yY][tT][eE][sS]?)?)|([kKmMgGtTpPeE]([iI]?[bB])?)");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PPciDomain

bool Validatable<Domain::Xml::PPciDomain>::validate(const Domain::Xml::PPciDomain::value_type& value_)
{
	QRegExp q("(0x)?[0-9a-fA-F]{1,4}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PPciBus

bool Validatable<Domain::Xml::PPciBus>::validate(const Domain::Xml::PPciBus::value_type& value_)
{
	QRegExp q("(0x)?[0-9a-fA-F]{1,2}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PPciSlot

bool Validatable<Domain::Xml::PPciSlot>::validate(const Domain::Xml::PPciSlot::value_type& value_)
{
	QRegExp q("(0x)?[0-1]?[0-9a-fA-F]");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PPciFunc

bool Validatable<Domain::Xml::PPciFunc>::validate(const Domain::Xml::PPciFunc::value_type& value_)
{
	QRegExp q("(0x)?[0-7]");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PWwn

bool Validatable<Domain::Xml::PWwn>::validate(const Domain::Xml::PWwn::value_type& value_)
{
	QRegExp q("(0x)?[0-9a-fA-F]{16}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PCpuset

bool Validatable<Domain::Xml::PCpuset>::validate(const Domain::Xml::PCpuset::value_type& value_)
{
	QRegExp q("([0-9]+(-[0-9]+)?|\\^[0-9]+)(,([0-9]+(-[0-9]+)?|\\^[0-9]+))*");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PVolName

bool Validatable<Domain::Xml::PVolName>::validate(const Domain::Xml::PVolName::value_type& value_)
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

bool Traits<Domain::Xml::PPortNumber>::parse(const QString& src_, Domain::Xml::PPortNumber::value_type& dst_)
{
	bool output = false;
	dst_ = src_.toInt(&output);
	return output;
}

QString Traits<Domain::Xml::PPortNumber>::generate(Domain::Xml::PPortNumber::value_type src_)
{
	return QString::number(src_);
}

bool Validatable<Domain::Xml::PPortNumber>::validate(Domain::Xml::PPortNumber::value_type value_)
{
	if (-1 > value_)
		return false;

	if (65535 < value_)
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PIobase

bool Validatable<Domain::Xml::PIobase>::validate(const Domain::Xml::PIobase::value_type& value_)
{
	QRegExp q("0x[a-fA-F0-9]{1,4}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PIrq

bool Validatable<Domain::Xml::PIrq>::validate(const Domain::Xml::PIrq::value_type& value_)
{
	QRegExp q("0x[a-fA-F0-9]");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PCompat

bool Validatable<Domain::Xml::PCompat>::validate(const Domain::Xml::PCompat::value_type& value_)
{
	QRegExp q("[0-9]+\\.[0-9]+");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PVirtualPortProfileID

bool Validatable<Domain::Xml::PVirtualPortProfileID>::validate(const Domain::Xml::PVirtualPortProfileID::value_type& value_)
{
	if (39 < value_.length())
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PSpeed

bool Traits<Domain::Xml::PSpeed>::parse(const QString& src_, Domain::Xml::PSpeed::value_type& dst_)
{
	QRegExp q("[0-9]+");
	if (!q.exactMatch(src_))
		return false;
	bool output = false;
	dst_ = src_.toUInt(&output);
	return output;
}

QString Traits<Domain::Xml::PSpeed>::generate(Domain::Xml::PSpeed::value_type src_)
{
	return QString::number(src_);
}

bool Validatable<Domain::Xml::PSpeed>::validate(Domain::Xml::PSpeed::value_type value_)
{
	if (1 > value_)
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PBurstSize

bool Traits<Domain::Xml::PBurstSize>::parse(const QString& src_, Domain::Xml::PBurstSize::value_type& dst_)
{
	QRegExp q("[0-9]+");
	if (!q.exactMatch(src_))
		return false;
	bool output = false;
	dst_ = src_.toUInt(&output);
	return output;
}

QString Traits<Domain::Xml::PBurstSize>::generate(Domain::Xml::PBurstSize::value_type src_)
{
	return QString::number(src_);
}

bool Validatable<Domain::Xml::PBurstSize>::validate(Domain::Xml::PBurstSize::value_type value_)
{
	if (1 > value_)
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PUnsignedShort

bool Traits<Domain::Xml::PUnsignedShort>::parse(const QString& src_, Domain::Xml::PUnsignedShort::value_type& dst_)
{
	bool output = false;
	dst_ = src_.toInt(&output);
	return output;
}

QString Traits<Domain::Xml::PUnsignedShort>::generate(Domain::Xml::PUnsignedShort::value_type src_)
{
	return QString::number(src_);
}

bool Validatable<Domain::Xml::PUnsignedShort>::validate(Domain::Xml::PUnsignedShort::value_type value_)
{
	if (0 > value_)
		return false;

	if (65535 < value_)
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PProtocol

bool Validatable<Domain::Xml::PProtocol>::validate(const Domain::Xml::PProtocol::value_type& value_)
{
	QRegExp q("(tcp)|(udp)");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PAddrFamily

bool Validatable<Domain::Xml::PAddrFamily>::validate(const Domain::Xml::PAddrFamily::value_type& value_)
{
	QRegExp q("(ipv4)|(ipv6)");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PId

bool Traits<Domain::Xml::PId>::parse(const QString& src_, Domain::Xml::PId::value_type& dst_)
{
	bool output = false;
	dst_ = src_.toUInt(&output);
	return output;
}

QString Traits<Domain::Xml::PId>::generate(Domain::Xml::PId::value_type src_)
{
	return QString::number(src_);
}

bool Validatable<Domain::Xml::PId>::validate(Domain::Xml::PId::value_type value_)
{
	if (4095 < value_)
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PPort

bool Traits<Domain::Xml::PPort>::parse(const QString& src_, Domain::Xml::PPort::value_type& dst_)
{
	bool output = false;
	dst_ = src_.toInt(&output);
	return output;
}

QString Traits<Domain::Xml::PPort>::generate(Domain::Xml::PPort::value_type src_)
{
	return QString::number(src_);
}

bool Validatable<Domain::Xml::PPort>::validate(Domain::Xml::PPort::value_type value_)
{
	if (1 > value_)
		return false;

	if (65535 < value_)
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PTitle

bool Validatable<Domain::Xml::PTitle>::validate(const Domain::Xml::PTitle::value_type& value_)
{
	QRegExp q("[^\n]+");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PMachine

bool Validatable<Domain::Xml::PMachine>::validate(const Domain::Xml::PMachine::value_type& value_)
{
	QRegExp q("[a-zA-Z0-9_\\.\\-]+");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PReadIopsSec

bool Traits<Domain::Xml::PReadIopsSec>::parse(const QString& src_, Domain::Xml::PReadIopsSec::value_type& dst_)
{
	bool output = false;
	dst_ = src_.toUInt(&output);
	return output;
}

QString Traits<Domain::Xml::PReadIopsSec>::generate(Domain::Xml::PReadIopsSec::value_type src_)
{
	return QString::number(src_);
}

///////////////////////////////////////////////////////////////////////////////
// struct PWriteIopsSec

bool Traits<Domain::Xml::PWriteIopsSec>::parse(const QString& src_, Domain::Xml::PWriteIopsSec::value_type& dst_)
{
	bool output = false;
	dst_ = src_.toUInt(&output);
	return output;
}

QString Traits<Domain::Xml::PWriteIopsSec>::generate(Domain::Xml::PWriteIopsSec::value_type src_)
{
	return QString::number(src_);
}

///////////////////////////////////////////////////////////////////////////////
// struct PReadBytesSec

bool Traits<Domain::Xml::PReadBytesSec>::parse(const QString& src_, Domain::Xml::PReadBytesSec::value_type& dst_)
{
	bool output = false;
	dst_ = src_.toULong(&output);
	return output;
}

QString Traits<Domain::Xml::PReadBytesSec>::generate(Domain::Xml::PReadBytesSec::value_type src_)
{
	return QString::number(src_);
}

///////////////////////////////////////////////////////////////////////////////
// struct PWriteBytesSec

bool Traits<Domain::Xml::PWriteBytesSec>::parse(const QString& src_, Domain::Xml::PWriteBytesSec::value_type& dst_)
{
	bool output = false;
	dst_ = src_.toULong(&output);
	return output;
}

QString Traits<Domain::Xml::PWriteBytesSec>::generate(Domain::Xml::PWriteBytesSec::value_type src_)
{
	return QString::number(src_);
}

///////////////////////////////////////////////////////////////////////////////
// struct PVendor

bool Validatable<Domain::Xml::PVendor>::validate(const Domain::Xml::PVendor::value_type& value_)
{
	QRegExp q("[\\x20-\\x7E]{0,8}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PProduct

bool Validatable<Domain::Xml::PProduct>::validate(const Domain::Xml::PProduct::value_type& value_)
{
	QRegExp q("[\\x20-\\x7E]{0,16}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PDiskTarget

bool Validatable<Domain::Xml::PDiskTarget>::validate(const Domain::Xml::PDiskTarget::value_type& value_)
{
	QRegExp q("(ioemu:)?(fd|hd|sd|vd|xvd|ubd)[a-zA-Z0-9_]+");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PCyls

bool Traits<Domain::Xml::PCyls>::parse(const QString& src_, Domain::Xml::PCyls::value_type& dst_)
{
	bool output = false;
	dst_ = src_.toInt(&output);
	return output;
}

QString Traits<Domain::Xml::PCyls>::generate(Domain::Xml::PCyls::value_type src_)
{
	return QString::number(src_);
}

///////////////////////////////////////////////////////////////////////////////
// struct PHeads

bool Traits<Domain::Xml::PHeads>::parse(const QString& src_, Domain::Xml::PHeads::value_type& dst_)
{
	bool output = false;
	dst_ = src_.toInt(&output);
	return output;
}

QString Traits<Domain::Xml::PHeads>::generate(Domain::Xml::PHeads::value_type src_)
{
	return QString::number(src_);
}

///////////////////////////////////////////////////////////////////////////////
// struct PSecs

bool Traits<Domain::Xml::PSecs>::parse(const QString& src_, Domain::Xml::PSecs::value_type& dst_)
{
	bool output = false;
	dst_ = src_.toInt(&output);
	return output;
}

QString Traits<Domain::Xml::PSecs>::generate(Domain::Xml::PSecs::value_type src_)
{
	return QString::number(src_);
}

///////////////////////////////////////////////////////////////////////////////
// struct PLogicalBlockSize

bool Traits<Domain::Xml::PLogicalBlockSize>::parse(const QString& src_, Domain::Xml::PLogicalBlockSize::value_type& dst_)
{
	bool output = false;
	dst_ = src_.toInt(&output);
	return output;
}

QString Traits<Domain::Xml::PLogicalBlockSize>::generate(Domain::Xml::PLogicalBlockSize::value_type src_)
{
	return QString::number(src_);
}

///////////////////////////////////////////////////////////////////////////////
// struct PPhysicalBlockSize

bool Traits<Domain::Xml::PPhysicalBlockSize>::parse(const QString& src_, Domain::Xml::PPhysicalBlockSize::value_type& dst_)
{
	bool output = false;
	dst_ = src_.toInt(&output);
	return output;
}

QString Traits<Domain::Xml::PPhysicalBlockSize>::generate(Domain::Xml::PPhysicalBlockSize::value_type src_)
{
	return QString::number(src_);
}

///////////////////////////////////////////////////////////////////////////////
// struct PType

bool Validatable<Domain::Xml::PType>::validate(const Domain::Xml::PType::value_type& value_)
{
	QRegExp q("[a-zA-Z0-9\\-_]+");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PPasswdValidTo

bool Traits<Domain::Xml::PPasswdValidTo>::parse(const QString& src_, Domain::Xml::PPasswdValidTo::value_type& dst_)
{
	dst_ = QDateTime::fromString(src_);
	return !dst_.isNull();
}

QString Traits<Domain::Xml::PPasswdValidTo>::generate(const Domain::Xml::PPasswdValidTo::value_type& src_)
{
	return src_.toString();
}

///////////////////////////////////////////////////////////////////////////////
// struct PVendorId

bool Validatable<Domain::Xml::PVendorId>::validate(const Domain::Xml::PVendorId::value_type& value_)
{
	QRegExp q("[^,]{12}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PSysinfoValue

bool Validatable<Domain::Xml::PSysinfoValue>::validate(const Domain::Xml::PSysinfoValue::value_type& value_)
{
	QRegExp q("[a-zA-Z0-9/\\-_\\. \\(\\)]+");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PTotalBytesSec

bool Traits<Domain::Xml::PTotalBytesSec>::parse(const QString& src_, Domain::Xml::PTotalBytesSec::value_type& dst_)
{
	bool output = false;
	dst_ = src_.toULong(&output);
	return output;
}

QString Traits<Domain::Xml::PTotalBytesSec>::generate(Domain::Xml::PTotalBytesSec::value_type src_)
{
	return QString::number(src_);
}

///////////////////////////////////////////////////////////////////////////////
// struct PTotalIopsSec

bool Traits<Domain::Xml::PTotalIopsSec>::parse(const QString& src_, Domain::Xml::PTotalIopsSec::value_type& dst_)
{
	bool output = false;
	dst_ = src_.toULong(&output);
	return output;
}

QString Traits<Domain::Xml::PTotalIopsSec>::generate(Domain::Xml::PTotalIopsSec::value_type src_)
{
	return QString::number(src_);
}

///////////////////////////////////////////////////////////////////////////////
// struct PReadIopsSec1

bool Traits<Domain::Xml::PReadIopsSec1>::parse(const QString& src_, Domain::Xml::PReadIopsSec1::value_type& dst_)
{
	bool output = false;
	dst_ = src_.toULong(&output);
	return output;
}

QString Traits<Domain::Xml::PReadIopsSec1>::generate(Domain::Xml::PReadIopsSec1::value_type src_)
{
	return QString::number(src_);
}

///////////////////////////////////////////////////////////////////////////////
// struct PWriteIopsSec1

bool Traits<Domain::Xml::PWriteIopsSec1>::parse(const QString& src_, Domain::Xml::PWriteIopsSec1::value_type& dst_)
{
	bool output = false;
	dst_ = src_.toULong(&output);
	return output;
}

QString Traits<Domain::Xml::PWriteIopsSec1>::generate(Domain::Xml::PWriteIopsSec1::value_type src_)
{
	return QString::number(src_);
}

///////////////////////////////////////////////////////////////////////////////
// struct PRetries

bool Traits<Domain::Xml::PRetries>::parse(const QString& src_, Domain::Xml::PRetries::value_type& dst_)
{
	bool output = false;
	dst_ = src_.toUInt(&output);
	return output;
}

QString Traits<Domain::Xml::PRetries>::generate(Domain::Xml::PRetries::value_type src_)
{
	return QString::number(src_);
}

///////////////////////////////////////////////////////////////////////////////
// struct PValue

bool Validatable<Domain::Xml::PValue>::validate(const Domain::Xml::PValue::value_type& value_)
{
	QRegExp q("[^,]{0,12}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PCountCPU

bool Traits<Domain::Xml::PCountCPU>::parse(const QString& src_, Domain::Xml::PCountCPU::value_type& dst_)
{
	QRegExp q("[0-9]+");
	if (!q.exactMatch(src_))
		return false;
	bool output = false;
	dst_ = src_.toUShort(&output);
	return output;
}

QString Traits<Domain::Xml::PCountCPU>::generate(Domain::Xml::PCountCPU::value_type src_)
{
	return QString::number(src_);
}

bool Validatable<Domain::Xml::PCountCPU>::validate(Domain::Xml::PCountCPU::value_type value_)
{
	if (1 > value_)
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PVcpuid

bool Traits<Domain::Xml::PVcpuid>::parse(const QString& src_, Domain::Xml::PVcpuid::value_type& dst_)
{
	QRegExp q("[0-9]+");
	if (!q.exactMatch(src_))
		return false;
	bool output = false;
	dst_ = src_.toUShort(&output);
	return output;
}

QString Traits<Domain::Xml::PVcpuid>::generate(Domain::Xml::PVcpuid::value_type src_)
{
	return QString::number(src_);
}

///////////////////////////////////////////////////////////////////////////////
// struct PCpushares

bool Traits<Domain::Xml::PCpushares>::parse(const QString& src_, Domain::Xml::PCpushares::value_type& dst_)
{
	QRegExp q("[0-9]+");
	if (!q.exactMatch(src_))
		return false;
	bool output = false;
	dst_ = src_.toUInt(&output);
	return output;
}

QString Traits<Domain::Xml::PCpushares>::generate(Domain::Xml::PCpushares::value_type src_)
{
	return QString::number(src_);
}

///////////////////////////////////////////////////////////////////////////////
// struct PCpuperiod

bool Traits<Domain::Xml::PCpuperiod>::parse(const QString& src_, Domain::Xml::PCpuperiod::value_type& dst_)
{
	QRegExp q("[0-9]+");
	if (!q.exactMatch(src_))
		return false;
	bool output = false;
	dst_ = src_.toULong(&output);
	return output;
}

QString Traits<Domain::Xml::PCpuperiod>::generate(Domain::Xml::PCpuperiod::value_type src_)
{
	return QString::number(src_);
}

bool Validatable<Domain::Xml::PCpuperiod>::validate(Domain::Xml::PCpuperiod::value_type value_)
{
	if (1000 > value_)
		return false;

	if (1000000 < value_)
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PCpuquota

bool Traits<Domain::Xml::PCpuquota>::parse(const QString& src_, Domain::Xml::PCpuquota::value_type& dst_)
{
	QRegExp q("-?[0-9]+");
	if (!q.exactMatch(src_))
		return false;
	bool output = false;
	dst_ = src_.toLong(&output);
	return output;
}

QString Traits<Domain::Xml::PCpuquota>::generate(Domain::Xml::PCpuquota::value_type src_)
{
	return QString::number(src_);
}

bool Validatable<Domain::Xml::PCpuquota>::validate(Domain::Xml::PCpuquota::value_type value_)
{
	if (-1 > value_)
		return false;

	if (18446744073709551 < value_)
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PRebootTimeoutDelay

bool Traits<Domain::Xml::PRebootTimeoutDelay>::parse(const QString& src_, Domain::Xml::PRebootTimeoutDelay::value_type& dst_)
{
	bool output = false;
	dst_ = src_.toShort(&output);
	return output;
}

QString Traits<Domain::Xml::PRebootTimeoutDelay>::generate(Domain::Xml::PRebootTimeoutDelay::value_type src_)
{
	return QString::number(src_);
}

bool Validatable<Domain::Xml::PRebootTimeoutDelay>::validate(Domain::Xml::PRebootTimeoutDelay::value_type value_)
{
	if (-1 > value_)
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PWeight

bool Traits<Domain::Xml::PWeight>::parse(const QString& src_, Domain::Xml::PWeight::value_type& dst_)
{
	QRegExp q("[0-9]+");
	if (!q.exactMatch(src_))
		return false;
	bool output = false;
	dst_ = src_.toUInt(&output);
	return output;
}

QString Traits<Domain::Xml::PWeight>::generate(Domain::Xml::PWeight::value_type src_)
{
	return QString::number(src_);
}

bool Validatable<Domain::Xml::PWeight>::validate(Domain::Xml::PWeight::value_type value_)
{
	if (100 > value_)
		return false;

	if (1000 < value_)
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PMemoryKB

bool Traits<Domain::Xml::PMemoryKB>::parse(const QString& src_, Domain::Xml::PMemoryKB::value_type& dst_)
{
	QRegExp q("[0-9]+");
	if (!q.exactMatch(src_))
		return false;
	bool output = false;
	dst_ = src_.toUInt(&output);
	return output;
}

QString Traits<Domain::Xml::PMemoryKB>::generate(Domain::Xml::PMemoryKB::value_type src_)
{
	return QString::number(src_);
}

///////////////////////////////////////////////////////////////////////////////
// struct PDomainName

bool Validatable<Domain::Xml::PDomainName>::validate(const Domain::Xml::PDomainName::value_type& value_)
{
	QRegExp q("[^\n]+");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PDiskSerial

bool Validatable<Domain::Xml::PDiskSerial>::validate(const Domain::Xml::PDiskSerial::value_type& value_)
{
	QRegExp q("[A-Za-z0-9_\\.\\+\\-]*");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PBridgeMode

bool Validatable<Domain::Xml::PBridgeMode>::validate(const Domain::Xml::PBridgeMode::value_type& value_)
{
	QRegExp q("(vepa|bridge|private|passthrough)");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PAddrIPorName

bool Validatable<Domain::Xml::PAddrIPorName>::validate(const Domain::Xml::PAddrIPorName::value_type& value_)
{
	QRegExp q("(([0-2]?[0-9]?[0-9]\\.){3}[0-2]?[0-9]?[0-9])|(([0-9a-fA-F]+|:)+[0-9a-fA-F]+)|([a-zA-Z0-9_\\.\\+\\-]*)");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PUsbIdDefault

bool Validatable<Domain::Xml::PUsbIdDefault>::validate(const Domain::Xml::PUsbIdDefault::value_type& value_)
{
	QRegExp q("-1");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PUsbId

bool Validatable<Domain::Xml::PUsbId>::validate(const Domain::Xml::PUsbId::value_type& value_)
{
	QRegExp q("(0x)?[0-9a-fA-F]{1,4}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PUsbVersion

bool Validatable<Domain::Xml::PUsbVersion>::validate(const Domain::Xml::PUsbVersion::value_type& value_)
{
	QRegExp q("[0-9]{1,2}.[0-9]{1,2}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PUsbAddr

bool Validatable<Domain::Xml::PUsbAddr>::validate(const Domain::Xml::PUsbAddr::value_type& value_)
{
	QRegExp q("(0x)?[0-9a-fA-F]{1,3}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PUsbClass

bool Validatable<Domain::Xml::PUsbClass>::validate(const Domain::Xml::PUsbClass::value_type& value_)
{
	QRegExp q("(0x)?[0-9a-fA-F]{1,2}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PUsbPort

bool Validatable<Domain::Xml::PUsbPort>::validate(const Domain::Xml::PUsbPort::value_type& value_)
{
	QRegExp q("((0x)?[0-9a-fA-F]{1,3}\\.){0,3}(0x)?[0-9a-fA-F]{1,3}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PDriveController

bool Validatable<Domain::Xml::PDriveController>::validate(const Domain::Xml::PDriveController::value_type& value_)
{
	QRegExp q("[0-9]{1,2}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PDriveBus

bool Validatable<Domain::Xml::PDriveBus>::validate(const Domain::Xml::PDriveBus::value_type& value_)
{
	QRegExp q("[0-9]{1,2}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PDriveTarget

bool Validatable<Domain::Xml::PDriveTarget>::validate(const Domain::Xml::PDriveTarget::value_type& value_)
{
	QRegExp q("[0-9]{1,2}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PDriveUnit

bool Validatable<Domain::Xml::PDriveUnit>::validate(const Domain::Xml::PDriveUnit::value_type& value_)
{
	QRegExp q("[0-9]{1,2}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PFeatureName

bool Validatable<Domain::Xml::PFeatureName>::validate(const Domain::Xml::PFeatureName::value_type& value_)
{
	QRegExp q("[a-zA-Z0-9\\-_\\.]+");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PTimeDelta

bool Validatable<Domain::Xml::PTimeDelta>::validate(const Domain::Xml::PTimeDelta::value_type& value_)
{
	QRegExp q("(-|\\+)?[0-9]+");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PTimeZone

bool Validatable<Domain::Xml::PTimeZone>::validate(const Domain::Xml::PTimeZone::value_type& value_)
{
	QRegExp q("[a-zA-Z0-9_\\.\\+\\-/]+");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PFilterParamName

bool Validatable<Domain::Xml::PFilterParamName>::validate(const Domain::Xml::PFilterParamName::value_type& value_)
{
	QRegExp q("[a-zA-Z0-9_]+");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PFilterParamValue

bool Validatable<Domain::Xml::PFilterParamValue>::validate(const Domain::Xml::PFilterParamValue::value_type& value_)
{
	QRegExp q("[a-zA-Z0-9_\\.:]+");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PSpaprvioReg

bool Validatable<Domain::Xml::PSpaprvioReg>::validate(const Domain::Xml::PSpaprvioReg::value_type& value_)
{
	QRegExp q("(0x)?[0-9a-fA-F]{1,16}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PAliasName

bool Validatable<Domain::Xml::PAliasName>::validate(const Domain::Xml::PAliasName::value_type& value_)
{
	QRegExp q("[a-zA-Z0-9_\\-.]+");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PData1126

bool Validatable<Domain::Xml::PData1126>::validate(const Domain::Xml::PData1126::value_type& value_)
{
	QRegExp q("0x[0-9a-eA-E][0-9a-fA-F]?");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PData1128

bool Validatable<Domain::Xml::PData1128>::validate(const Domain::Xml::PData1128::value_type& value_)
{
	QRegExp q("0x[fF][0-9a-eA-E]?");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PData1130

bool Traits<Domain::Xml::PData1130>::parse(const QString& src_, Domain::Xml::PData1130::value_type& dst_)
{
	bool output = false;
	dst_ = src_.toInt(&output);
	return output;
}

QString Traits<Domain::Xml::PData1130>::generate(Domain::Xml::PData1130::value_type src_)
{
	return QString::number(src_);
}

bool Validatable<Domain::Xml::PData1130>::validate(Domain::Xml::PData1130::value_type value_)
{
	if (0 > value_)
		return false;

	if (254 < value_)
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PCcwSsidRange

bool Validatable<Domain::Xml::PCcwSsidRange>::validate(const Domain::Xml::PCcwSsidRange::value_type& value_)
{
	QRegExp q("(0x)?[0-3]");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PData1132

bool Validatable<Domain::Xml::PData1132>::validate(const Domain::Xml::PData1132::value_type& value_)
{
	QRegExp q("0x[0-9a-fA-F]{1,4}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PData1134

bool Traits<Domain::Xml::PData1134>::parse(const QString& src_, Domain::Xml::PData1134::value_type& dst_)
{
	bool output = false;
	dst_ = src_.toInt(&output);
	return output;
}

QString Traits<Domain::Xml::PData1134>::generate(Domain::Xml::PData1134::value_type src_)
{
	return QString::number(src_);
}

bool Validatable<Domain::Xml::PData1134>::validate(Domain::Xml::PData1134::value_type value_)
{
	if (0 > value_)
		return false;

	if (65535 < value_)
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct VUUID

bool Traits<Domain::Xml::VUUID>::parse(const QString& src_, Domain::Xml::VUUID& dst_)
{
	int x;
	mpl::at_c<Domain::Xml::VUUID::types, 0>::type a0;
	x = Marshal<Domain::Xml::PData17>::setString(src_, a0);
	if (0 < x)
	{
		dst_ = a0;
		return true;
	}
	mpl::at_c<Domain::Xml::VUUID::types, 1>::type a1;
	x = Marshal<Domain::Xml::PData19>::setString(src_, a1);
	if (0 < x)
	{
		dst_ = a1;
		return true;
	}

	return false;
}

QString Traits<Domain::Xml::VUUID>::generate(const Domain::Xml::VUUID& src_)
{
	switch (src_.which())
	{
	case 0:
		return Marshal<Domain::Xml::PData17>::getString(boost::get<mpl::at_c<Domain::Xml::VUUID::types, 0>::type>(src_));
	case 1:
		return Marshal<Domain::Xml::PData19>::getString(boost::get<mpl::at_c<Domain::Xml::VUUID::types, 1>::type>(src_));
	}
	return QString();
}

///////////////////////////////////////////////////////////////////////////////
// struct VAdjustment

bool Traits<Domain::Xml::VAdjustment>::parse(const QString& src_, Domain::Xml::VAdjustment& dst_)
{
	int x;
	mpl::at_c<Domain::Xml::VAdjustment::types, 0>::type a0;
	x = Marshal<Domain::Xml::EAdjustment>::setString(src_, a0);
	if (0 < x)
	{
		dst_ = a0;
		return true;
	}
	mpl::at_c<Domain::Xml::VAdjustment::types, 1>::type a1;
	x = Marshal<Domain::Xml::PTimeDelta>::setString(src_, a1);
	if (0 < x)
	{
		dst_ = a1;
		return true;
	}

	return false;
}

QString Traits<Domain::Xml::VAdjustment>::generate(const Domain::Xml::VAdjustment& src_)
{
	switch (src_.which())
	{
	case 0:
		return Marshal<Domain::Xml::EAdjustment>::getString(boost::get<mpl::at_c<Domain::Xml::VAdjustment::types, 0>::type>(src_));
	case 1:
		return Marshal<Domain::Xml::PTimeDelta>::getString(boost::get<mpl::at_c<Domain::Xml::VAdjustment::types, 1>::type>(src_));
	}
	return QString();
}

///////////////////////////////////////////////////////////////////////////////
// struct VIpAddr

bool Traits<Domain::Xml::VIpAddr>::parse(const QString& src_, Domain::Xml::VIpAddr& dst_)
{
	int x;
	mpl::at_c<Domain::Xml::VIpAddr::types, 0>::type a0;
	x = Marshal<Domain::Xml::PIpv4Addr>::setString(src_, a0);
	if (0 < x)
	{
		dst_ = a0;
		return true;
	}
	mpl::at_c<Domain::Xml::VIpAddr::types, 1>::type a1;
	x = Marshal<Domain::Xml::PIpv6Addr>::setString(src_, a1);
	if (0 < x)
	{
		dst_ = a1;
		return true;
	}

	return false;
}

QString Traits<Domain::Xml::VIpAddr>::generate(const Domain::Xml::VIpAddr& src_)
{
	switch (src_.which())
	{
	case 0:
		return Marshal<Domain::Xml::PIpv4Addr>::getString(boost::get<mpl::at_c<Domain::Xml::VIpAddr::types, 0>::type>(src_));
	case 1:
		return Marshal<Domain::Xml::PIpv6Addr>::getString(boost::get<mpl::at_c<Domain::Xml::VIpAddr::types, 1>::type>(src_));
	}
	return QString();
}

///////////////////////////////////////////////////////////////////////////////
// struct VName

bool Traits<Domain::Xml::VName>::parse(const QString& src_, Domain::Xml::VName& dst_)
{
	int x;
	mpl::at_c<Domain::Xml::VName::types, 0>::type a0;
	x = Marshal<Domain::Xml::PDnsName>::setString(src_, a0);
	if (0 < x)
	{
		dst_ = a0;
		return true;
	}
	mpl::at_c<Domain::Xml::VName::types, 1>::type a1;
	x = Marshal<Domain::Xml::VIpAddr>::setString(src_, a1);
	if (0 < x)
	{
		dst_ = a1;
		return true;
	}

	return false;
}

QString Traits<Domain::Xml::VName>::generate(const Domain::Xml::VName& src_)
{
	switch (src_.which())
	{
	case 0:
		return Marshal<Domain::Xml::PDnsName>::getString(boost::get<mpl::at_c<Domain::Xml::VName::types, 0>::type>(src_));
	case 1:
		return Marshal<Domain::Xml::VIpAddr>::getString(boost::get<mpl::at_c<Domain::Xml::VName::types, 1>::type>(src_));
	}
	return QString();
}

///////////////////////////////////////////////////////////////////////////////
// struct VStorageFormat

bool Traits<Domain::Xml::VStorageFormat>::parse(const QString& src_, Domain::Xml::VStorageFormat& dst_)
{
	int x;
	mpl::at_c<Domain::Xml::VStorageFormat::types, 0>::type a0;
	x = Marshal<Domain::Xml::EStorageFormat>::setString(src_, a0);
	if (0 < x)
	{
		dst_ = a0;
		return true;
	}
	mpl::at_c<Domain::Xml::VStorageFormat::types, 1>::type a1;
	x = Marshal<Domain::Xml::EStorageFormatBacking>::setString(src_, a1);
	if (0 < x)
	{
		dst_ = a1;
		return true;
	}

	return false;
}

QString Traits<Domain::Xml::VStorageFormat>::generate(const Domain::Xml::VStorageFormat& src_)
{
	switch (src_.which())
	{
	case 0:
		return Marshal<Domain::Xml::EStorageFormat>::getString(boost::get<mpl::at_c<Domain::Xml::VStorageFormat::types, 0>::type>(src_));
	case 1:
		return Marshal<Domain::Xml::EStorageFormatBacking>::getString(boost::get<mpl::at_c<Domain::Xml::VStorageFormat::types, 1>::type>(src_));
	}
	return QString();
}

///////////////////////////////////////////////////////////////////////////////
// struct VType

bool Traits<Domain::Xml::VType>::parse(const QString& src_, Domain::Xml::VType& dst_)
{
	int x;
	mpl::at_c<Domain::Xml::VType::types, 0>::type a0;
	x = Marshal<Domain::Xml::EType3>::setString(src_, a0);
	if (0 < x)
	{
		dst_ = a0;
		return true;
	}
	mpl::at_c<Domain::Xml::VType::types, 1>::type a1;
	x = Marshal<Domain::Xml::VStorageFormat>::setString(src_, a1);
	if (0 < x)
	{
		dst_ = a1;
		return true;
	}

	return false;
}

QString Traits<Domain::Xml::VType>::generate(const Domain::Xml::VType& src_)
{
	switch (src_.which())
	{
	case 0:
		return Marshal<Domain::Xml::EType3>::getString(boost::get<mpl::at_c<Domain::Xml::VType::types, 0>::type>(src_));
	case 1:
		return Marshal<Domain::Xml::VStorageFormat>::getString(boost::get<mpl::at_c<Domain::Xml::VType::types, 1>::type>(src_));
	}
	return QString();
}

///////////////////////////////////////////////////////////////////////////////
// struct VCcwCssidRange

bool Traits<Domain::Xml::VCcwCssidRange>::parse(const QString& src_, Domain::Xml::VCcwCssidRange& dst_)
{
	int x;
	mpl::at_c<Domain::Xml::VCcwCssidRange::types, 0>::type a0;
	x = Marshal<Domain::Xml::PData1126>::setString(src_, a0);
	if (0 < x)
	{
		dst_ = a0;
		return true;
	}
	mpl::at_c<Domain::Xml::VCcwCssidRange::types, 1>::type a1;
	x = Marshal<Domain::Xml::PData1128>::setString(src_, a1);
	if (0 < x)
	{
		dst_ = a1;
		return true;
	}
	mpl::at_c<Domain::Xml::VCcwCssidRange::types, 2>::type a2;
	x = Marshal<Domain::Xml::PData1130>::setString(src_, a2);
	if (0 < x)
	{
		dst_ = a2;
		return true;
	}

	return false;
}

QString Traits<Domain::Xml::VCcwCssidRange>::generate(const Domain::Xml::VCcwCssidRange& src_)
{
	switch (src_.which())
	{
	case 0:
		return Marshal<Domain::Xml::PData1126>::getString(boost::get<mpl::at_c<Domain::Xml::VCcwCssidRange::types, 0>::type>(src_));
	case 1:
		return Marshal<Domain::Xml::PData1128>::getString(boost::get<mpl::at_c<Domain::Xml::VCcwCssidRange::types, 1>::type>(src_));
	case 2:
		return Marshal<Domain::Xml::PData1130>::getString(boost::get<mpl::at_c<Domain::Xml::VCcwCssidRange::types, 2>::type>(src_));
	}
	return QString();
}

///////////////////////////////////////////////////////////////////////////////
// struct VCcwDevnoRange

bool Traits<Domain::Xml::VCcwDevnoRange>::parse(const QString& src_, Domain::Xml::VCcwDevnoRange& dst_)
{
	int x;
	mpl::at_c<Domain::Xml::VCcwDevnoRange::types, 0>::type a0;
	x = Marshal<Domain::Xml::PData1132>::setString(src_, a0);
	if (0 < x)
	{
		dst_ = a0;
		return true;
	}
	mpl::at_c<Domain::Xml::VCcwDevnoRange::types, 1>::type a1;
	x = Marshal<Domain::Xml::PData1134>::setString(src_, a1);
	if (0 < x)
	{
		dst_ = a1;
		return true;
	}

	return false;
}

QString Traits<Domain::Xml::VCcwDevnoRange>::generate(const Domain::Xml::VCcwDevnoRange& src_)
{
	switch (src_.which())
	{
	case 0:
		return Marshal<Domain::Xml::PData1132>::getString(boost::get<mpl::at_c<Domain::Xml::VCcwDevnoRange::types, 0>::type>(src_));
	case 1:
		return Marshal<Domain::Xml::PData1134>::getString(boost::get<mpl::at_c<Domain::Xml::VCcwDevnoRange::types, 1>::type>(src_));
	}
	return QString();
}

///////////////////////////////////////////////////////////////////////////////
// struct VUint8range

bool Traits<Domain::Xml::VUint8range>::parse(const QString& src_, Domain::Xml::VUint8range& dst_)
{
	int x;
	mpl::at_c<Domain::Xml::VUint8range::types, 0>::type a0;
	x = Marshal<Domain::Xml::PData7>::setString(src_, a0);
	if (0 < x)
	{
		dst_ = a0;
		return true;
	}
	mpl::at_c<Domain::Xml::VUint8range::types, 1>::type a1;
	x = Marshal<Domain::Xml::PData9>::setString(src_, a1);
	if (0 < x)
	{
		dst_ = a1;
		return true;
	}

	return false;
}

QString Traits<Domain::Xml::VUint8range>::generate(const Domain::Xml::VUint8range& src_)
{
	switch (src_.which())
	{
	case 0:
		return Marshal<Domain::Xml::PData7>::getString(boost::get<mpl::at_c<Domain::Xml::VUint8range::types, 0>::type>(src_));
	case 1:
		return Marshal<Domain::Xml::PData9>::getString(boost::get<mpl::at_c<Domain::Xml::VUint8range::types, 1>::type>(src_));
	}
	return QString();
}

///////////////////////////////////////////////////////////////////////////////
// struct VUint24range

bool Traits<Domain::Xml::VUint24range>::parse(const QString& src_, Domain::Xml::VUint24range& dst_)
{
	int x;
	mpl::at_c<Domain::Xml::VUint24range::types, 0>::type a0;
	x = Marshal<Domain::Xml::PData12>::setString(src_, a0);
	if (0 < x)
	{
		dst_ = a0;
		return true;
	}
	mpl::at_c<Domain::Xml::VUint24range::types, 1>::type a1;
	x = Marshal<Domain::Xml::PData14>::setString(src_, a1);
	if (0 < x)
	{
		dst_ = a1;
		return true;
	}

	return false;
}

QString Traits<Domain::Xml::VUint24range>::generate(const Domain::Xml::VUint24range& src_)
{
	switch (src_.which())
	{
	case 0:
		return Marshal<Domain::Xml::PData12>::getString(boost::get<mpl::at_c<Domain::Xml::VUint24range::types, 0>::type>(src_));
	case 1:
		return Marshal<Domain::Xml::PData14>::getString(boost::get<mpl::at_c<Domain::Xml::VUint24range::types, 1>::type>(src_));
	}
	return QString();
}

///////////////////////////////////////////////////////////////////////////////
// struct VIpPrefix

bool Traits<Domain::Xml::VIpPrefix>::parse(const QString& src_, Domain::Xml::VIpPrefix& dst_)
{
	int x;
	mpl::at_c<Domain::Xml::VIpPrefix::types, 0>::type a0;
	x = Marshal<Domain::Xml::PIpv4Prefix>::setString(src_, a0);
	if (0 < x)
	{
		dst_ = a0;
		return true;
	}
	mpl::at_c<Domain::Xml::VIpPrefix::types, 1>::type a1;
	x = Marshal<Domain::Xml::PIpv6Prefix>::setString(src_, a1);
	if (0 < x)
	{
		dst_ = a1;
		return true;
	}

	return false;
}

QString Traits<Domain::Xml::VIpPrefix>::generate(const Domain::Xml::VIpPrefix& src_)
{
	switch (src_.which())
	{
	case 0:
		return Marshal<Domain::Xml::PIpv4Prefix>::getString(boost::get<mpl::at_c<Domain::Xml::VIpPrefix::types, 0>::type>(src_));
	case 1:
		return Marshal<Domain::Xml::PIpv6Prefix>::getString(boost::get<mpl::at_c<Domain::Xml::VIpPrefix::types, 1>::type>(src_));
	}
	return QString();
}

///////////////////////////////////////////////////////////////////////////////
// struct VClass

bool Traits<Domain::Xml::VClass>::parse(const QString& src_, Domain::Xml::VClass& dst_)
{
	int x;
	mpl::at_c<Domain::Xml::VClass::types, 0>::type a0;
	x = Marshal<Domain::Xml::PUsbClass>::setString(src_, a0);
	if (0 < x)
	{
		dst_ = a0;
		return true;
	}
	mpl::at_c<Domain::Xml::VClass::types, 1>::type a1;
	x = Marshal<Domain::Xml::PUsbIdDefault>::setString(src_, a1);
	if (0 < x)
	{
		dst_ = a1;
		return true;
	}

	return false;
}

QString Traits<Domain::Xml::VClass>::generate(const Domain::Xml::VClass& src_)
{
	switch (src_.which())
	{
	case 0:
		return Marshal<Domain::Xml::PUsbClass>::getString(boost::get<mpl::at_c<Domain::Xml::VClass::types, 0>::type>(src_));
	case 1:
		return Marshal<Domain::Xml::PUsbIdDefault>::getString(boost::get<mpl::at_c<Domain::Xml::VClass::types, 1>::type>(src_));
	}
	return QString();
}

///////////////////////////////////////////////////////////////////////////////
// struct VVendor

bool Traits<Domain::Xml::VVendor>::parse(const QString& src_, Domain::Xml::VVendor& dst_)
{
	int x;
	mpl::at_c<Domain::Xml::VVendor::types, 0>::type a0;
	x = Marshal<Domain::Xml::PUsbId>::setString(src_, a0);
	if (0 < x)
	{
		dst_ = a0;
		return true;
	}
	mpl::at_c<Domain::Xml::VVendor::types, 1>::type a1;
	x = Marshal<Domain::Xml::PUsbIdDefault>::setString(src_, a1);
	if (0 < x)
	{
		dst_ = a1;
		return true;
	}

	return false;
}

QString Traits<Domain::Xml::VVendor>::generate(const Domain::Xml::VVendor& src_)
{
	switch (src_.which())
	{
	case 0:
		return Marshal<Domain::Xml::PUsbId>::getString(boost::get<mpl::at_c<Domain::Xml::VVendor::types, 0>::type>(src_));
	case 1:
		return Marshal<Domain::Xml::PUsbIdDefault>::getString(boost::get<mpl::at_c<Domain::Xml::VVendor::types, 1>::type>(src_));
	}
	return QString();
}

///////////////////////////////////////////////////////////////////////////////
// struct VVersion

bool Traits<Domain::Xml::VVersion>::parse(const QString& src_, Domain::Xml::VVersion& dst_)
{
	int x;
	mpl::at_c<Domain::Xml::VVersion::types, 0>::type a0;
	x = Marshal<Domain::Xml::PUsbVersion>::setString(src_, a0);
	if (0 < x)
	{
		dst_ = a0;
		return true;
	}
	mpl::at_c<Domain::Xml::VVersion::types, 1>::type a1;
	x = Marshal<Domain::Xml::PUsbIdDefault>::setString(src_, a1);
	if (0 < x)
	{
		dst_ = a1;
		return true;
	}

	return false;
}

QString Traits<Domain::Xml::VVersion>::generate(const Domain::Xml::VVersion& src_)
{
	switch (src_.which())
	{
	case 0:
		return Marshal<Domain::Xml::PUsbVersion>::getString(boost::get<mpl::at_c<Domain::Xml::VVersion::types, 0>::type>(src_));
	case 1:
		return Marshal<Domain::Xml::PUsbIdDefault>::getString(boost::get<mpl::at_c<Domain::Xml::VVersion::types, 1>::type>(src_));
	}
	return QString();
}

} // namespace Libvirt
