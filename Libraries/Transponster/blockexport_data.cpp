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

#include "blockexport_data.h"
#include <QRegExp>

namespace Libvirt
{
///////////////////////////////////////////////////////////////////////////////
// struct PUnsignedInt

bool Traits<Blockexport::Xml::PUnsignedInt>::parse(const QString& src_, Blockexport::Xml::PUnsignedInt::value_type& dst_)
{
	QRegExp q("[0-9]+");
	if (!q.exactMatch(src_))
		return false;
	bool output = false;
	dst_ = src_.toUInt(&output);
	return output;
}

QString Traits<Blockexport::Xml::PUnsignedInt>::generate(Blockexport::Xml::PUnsignedInt::value_type src_)
{
	return QString::number(src_);
}

///////////////////////////////////////////////////////////////////////////////
// struct PUnsignedLong

bool Traits<Blockexport::Xml::PUnsignedLong>::parse(const QString& src_, Blockexport::Xml::PUnsignedLong::value_type& dst_)
{
	QRegExp q("[0-9]+");
	if (!q.exactMatch(src_))
		return false;
	bool output = false;
	dst_ = src_.toULong(&output);
	return output;
}

QString Traits<Blockexport::Xml::PUnsignedLong>::generate(Blockexport::Xml::PUnsignedLong::value_type src_)
{
	return QString::number(src_);
}

///////////////////////////////////////////////////////////////////////////////
// struct PHexuint

bool Validatable<Blockexport::Xml::PHexuint>::validate(const Blockexport::Xml::PHexuint::value_type& value_)
{
	QRegExp q("(0x)?[0-9a-f]+");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PPositiveInteger

bool Traits<Blockexport::Xml::PPositiveInteger>::parse(const QString& src_, Blockexport::Xml::PPositiveInteger::value_type& dst_)
{
	QRegExp q("[0-9]+");
	if (!q.exactMatch(src_))
		return false;
	bool output = false;
	dst_ = src_.toUInt(&output);
	return output;
}

QString Traits<Blockexport::Xml::PPositiveInteger>::generate(Blockexport::Xml::PPositiveInteger::value_type src_)
{
	return QString::number(src_);
}

///////////////////////////////////////////////////////////////////////////////
// struct POctalMode

bool Traits<Blockexport::Xml::POctalMode>::parse(const QString& src_, Blockexport::Xml::POctalMode::value_type& dst_)
{
	QRegExp q("[0-7]+");
	if (!q.exactMatch(src_))
		return false;
	bool output = false;
	dst_ = src_.toUInt(&output);
	return output;
}

QString Traits<Blockexport::Xml::POctalMode>::generate(Blockexport::Xml::POctalMode::value_type src_)
{
	return QString::number(src_);
}

///////////////////////////////////////////////////////////////////////////////
// struct PData1881

bool Validatable<Blockexport::Xml::PData1881>::validate(const Blockexport::Xml::PData1881::value_type& value_)
{
	QRegExp q("0x[0-9a-fA-F]{1,2}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PData1883

bool Traits<Blockexport::Xml::PData1883>::parse(const QString& src_, Blockexport::Xml::PData1883::value_type& dst_)
{
	bool output = false;
	dst_ = src_.toInt(&output);
	return output;
}

QString Traits<Blockexport::Xml::PData1883>::generate(Blockexport::Xml::PData1883::value_type src_)
{
	return QString::number(src_);
}

bool Validatable<Blockexport::Xml::PData1883>::validate(Blockexport::Xml::PData1883::value_type value_)
{
	if (0 > value_)
		return false;

	if (255 < value_)
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PData1885

bool Validatable<Blockexport::Xml::PData1885>::validate(const Blockexport::Xml::PData1885::value_type& value_)
{
	QRegExp q("0x[0-9a-fA-F]{1,2}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PData1887

bool Traits<Blockexport::Xml::PData1887>::parse(const QString& src_, Blockexport::Xml::PData1887::value_type& dst_)
{
	bool output = false;
	dst_ = src_.toInt(&output);
	return output;
}

QString Traits<Blockexport::Xml::PData1887>::generate(Blockexport::Xml::PData1887::value_type src_)
{
	return QString::number(src_);
}

bool Validatable<Blockexport::Xml::PData1887>::validate(Blockexport::Xml::PData1887::value_type value_)
{
	if (0 > value_)
		return false;

	if (255 < value_)
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PData1889

bool Validatable<Blockexport::Xml::PData1889>::validate(const Blockexport::Xml::PData1889::value_type& value_)
{
	QRegExp q("0x[0-9a-fA-F]{1,6}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PData1891

bool Traits<Blockexport::Xml::PData1891>::parse(const QString& src_, Blockexport::Xml::PData1891::value_type& dst_)
{
	bool output = false;
	dst_ = src_.toInt(&output);
	return output;
}

QString Traits<Blockexport::Xml::PData1891>::generate(Blockexport::Xml::PData1891::value_type src_)
{
	return QString::number(src_);
}

bool Validatable<Blockexport::Xml::PData1891>::validate(Blockexport::Xml::PData1891::value_type value_)
{
	if (0 > value_)
		return false;

	if (16777215 < value_)
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PData1893

bool Validatable<Blockexport::Xml::PData1893>::validate(const Blockexport::Xml::PData1893::value_type& value_)
{
	QRegExp q("[a-fA-F0-9]{32}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PData1895

bool Validatable<Blockexport::Xml::PData1895>::validate(const Blockexport::Xml::PData1895::value_type& value_)
{
	QRegExp q("[a-fA-F0-9]{8}\\-([a-fA-F0-9]{4}\\-){3}[a-fA-F0-9]{12}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PUniMacAddr

bool Validatable<Blockexport::Xml::PUniMacAddr>::validate(const Blockexport::Xml::PUniMacAddr::value_type& value_)
{
	QRegExp q("[a-fA-F0-9][02468aAcCeE](:[a-fA-F0-9]{2}){5}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PMultiMacAddr

bool Validatable<Blockexport::Xml::PMultiMacAddr>::validate(const Blockexport::Xml::PMultiMacAddr::value_type& value_)
{
	QRegExp q("[a-fA-F0-9][13579bBdDfF](:[a-fA-F0-9]{2}){5}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PMacAddr

bool Validatable<Blockexport::Xml::PMacAddr>::validate(const Blockexport::Xml::PMacAddr::value_type& value_)
{
	QRegExp q("[a-fA-F0-9]{2}(:[a-fA-F0-9]{2}){5}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PDuidLLT

bool Validatable<Blockexport::Xml::PDuidLLT>::validate(const Blockexport::Xml::PDuidLLT::value_type& value_)
{
	QRegExp q("[0]{1,2}:[0]{0,1}[1]:[0]{1,2}:[0]{0,1}[a-fA-F1-9](:[a-fA-F0-9]{1,2}){4}(:[a-fA-F0-9]{1,2}){6,8}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PDuidEN

bool Validatable<Blockexport::Xml::PDuidEN>::validate(const Blockexport::Xml::PDuidEN::value_type& value_)
{
	QRegExp q("[0]{1,2}:[0]{0,1}[2](:[a-fA-F0-9]{1,2}){4}(:[a-fA-F0-9]{1,2}){1,124}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PDuidLL

bool Validatable<Blockexport::Xml::PDuidLL>::validate(const Blockexport::Xml::PDuidLL::value_type& value_)
{
	QRegExp q("[0]{1,2}:[0]{0,1}[3]:[0]{1,2}:[0]{0,1}[a-fA-F1-9](:[a-fA-F0-9]{1,2}){6,8}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PDuidUUID

bool Validatable<Blockexport::Xml::PDuidUUID>::validate(const Blockexport::Xml::PDuidUUID::value_type& value_)
{
	QRegExp q("[0]{1,2}:[0]{0,1}[4](:[a-fA-F0-9]{1,2}){16}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PIpv4Addr

bool Validatable<Blockexport::Xml::PIpv4Addr>::validate(const Blockexport::Xml::PIpv4Addr::value_type& value_)
{
	QRegExp q("(((25[0-5])|(2[0-4][0-9])|(1[0-9]{2})|([1-9][0-9])|([0-9]))\\.){3}((25[0-5])|(2[0-4][0-9])|(1[0-9]{2})|([1-9][0-9])|([0-9]))");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PIpv6Addr

bool Validatable<Blockexport::Xml::PIpv6Addr>::validate(const Blockexport::Xml::PIpv6Addr::value_type& value_)
{
	QRegExp q("(([0-9A-Fa-f]{1,4}:){7}[0-9A-Fa-f]{1,4})|(([0-9A-Fa-f]{1,4}:){6}:[0-9A-Fa-f]{1,4})|(([0-9A-Fa-f]{1,4}:){5}:([0-9A-Fa-f]{1,4}:)?[0-9A-Fa-f]{1,4})|(([0-9A-Fa-f]{1,4}:){4}:([0-9A-Fa-f]{1,4}:){0,2}[0-9A-Fa-f]{1,4})|(([0-9A-Fa-f]{1,4}:){3}:([0-9A-Fa-f]{1,4}:){0,3}[0-9A-Fa-f]{1,4})|(([0-9A-Fa-f]{1,4}:){2}:([0-9A-Fa-f]{1,4}:){0,4}[0-9A-Fa-f]{1,4})|(([0-9A-Fa-f]{1,4}:){6}(((25[0-5])|(2[0-4][0-9])|(1[0-9]{2})|([1-9][0-9])|([0-9]))\\.){3}((25[0-5])|(2[0-4][0-9])|(1[0-9]{2})|([1-9][0-9])|([0-9])))|(([0-9A-Fa-f]{1,4}:){0,5}:(((25[0-5])|(2[0-4][0-9])|(1[0-9]{2})|([1-9][0-9])|([0-9]))\\.){3}((25[0-5])|(2[0-4][0-9])|(1[0-9]{2})|([1-9][0-9])|([0-9])))|(::([0-9A-Fa-f]{1,4}:){0,5}(((25[0-5])|(2[0-4][0-9])|(1[0-9]{2})|([1-9][0-9])|([0-9]))\\.){3}((25[0-5])|(2[0-4][0-9])|(1[0-9]{2})|([1-9][0-9])|([0-9])))|([0-9A-Fa-f]{1,4}::([0-9A-Fa-f]{1,4}:){0,5}[0-9A-Fa-f]{1,4})|(::([0-9A-Fa-f]{1,4}:){0,6}[0-9A-Fa-f]{1,4})|(([0-9A-Fa-f]{1,4}:){1,7}:)");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PIpv4Prefix

bool Traits<Blockexport::Xml::PIpv4Prefix>::parse(const QString& src_, Blockexport::Xml::PIpv4Prefix::value_type& dst_)
{
	bool output = false;
	dst_ = src_.toUInt(&output);
	return output;
}

QString Traits<Blockexport::Xml::PIpv4Prefix>::generate(Blockexport::Xml::PIpv4Prefix::value_type src_)
{
	return QString::number(src_);
}

bool Validatable<Blockexport::Xml::PIpv4Prefix>::validate(Blockexport::Xml::PIpv4Prefix::value_type value_)
{
	if (32 < value_)
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PIpv6Prefix

bool Traits<Blockexport::Xml::PIpv6Prefix>::parse(const QString& src_, Blockexport::Xml::PIpv6Prefix::value_type& dst_)
{
	bool output = false;
	dst_ = src_.toUInt(&output);
	return output;
}

QString Traits<Blockexport::Xml::PIpv6Prefix>::generate(Blockexport::Xml::PIpv6Prefix::value_type src_)
{
	return QString::number(src_);
}

bool Validatable<Blockexport::Xml::PIpv6Prefix>::validate(Blockexport::Xml::PIpv6Prefix::value_type value_)
{
	if (128 < value_)
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PGenericName

bool Validatable<Blockexport::Xml::PGenericName>::validate(const Blockexport::Xml::PGenericName::value_type& value_)
{
	QRegExp q("[a-zA-Z0-9_\\+\\-]+");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PDnsName

bool Validatable<Blockexport::Xml::PDnsName>::validate(const Blockexport::Xml::PDnsName::value_type& value_)
{
	QRegExp q("[a-zA-Z0-9\\.\\-]+");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PDeviceName

bool Validatable<Blockexport::Xml::PDeviceName>::validate(const Blockexport::Xml::PDeviceName::value_type& value_)
{
	QRegExp q("[a-zA-Z0-9_\\.\\-\\\\:/]+");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PFilePath

bool Validatable<Blockexport::Xml::PFilePath>::validate(const Blockexport::Xml::PFilePath::value_type& value_)
{
	QRegExp q("[a-zA-Z0-9_\\.\\+\\-\\\\&\"{}'<>/%: ]+");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PDirPath

bool Validatable<Blockexport::Xml::PDirPath>::validate(const Blockexport::Xml::PDirPath::value_type& value_)
{
	QRegExp q("[a-zA-Z0-9_\\.\\+\\-\\\\&\"{}'<>/%: ]+");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PAbsFilePath

bool Validatable<Blockexport::Xml::PAbsFilePath>::validate(const Blockexport::Xml::PAbsFilePath::value_type& value_)
{
	QRegExp q("/[a-zA-Z0-9_\\.\\+\\-\\\\&\"{}'<>/%,: ]+");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PAbsDirPath

bool Validatable<Blockexport::Xml::PAbsDirPath>::validate(const Blockexport::Xml::PAbsDirPath::value_type& value_)
{
	QRegExp q("/[a-zA-Z0-9_\\.\\+\\-\\\\&\"{}'<>/%: ]*");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PUnit

bool Validatable<Blockexport::Xml::PUnit>::validate(const Blockexport::Xml::PUnit::value_type& value_)
{
	QRegExp q("([bB]([yY][tT][eE][sS]?)?)|([kKmMgGtTpPeE]([iI]?[bB])?)");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PPciDomain

bool Validatable<Blockexport::Xml::PPciDomain>::validate(const Blockexport::Xml::PPciDomain::value_type& value_)
{
	QRegExp q("(0x)?[0-9a-fA-F]{1,4}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PPciBus

bool Validatable<Blockexport::Xml::PPciBus>::validate(const Blockexport::Xml::PPciBus::value_type& value_)
{
	QRegExp q("(0x)?[0-9a-fA-F]{1,2}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PPciSlot

bool Validatable<Blockexport::Xml::PPciSlot>::validate(const Blockexport::Xml::PPciSlot::value_type& value_)
{
	QRegExp q("(0x)?[0-1]?[0-9a-fA-F]");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PPciFunc

bool Validatable<Blockexport::Xml::PPciFunc>::validate(const Blockexport::Xml::PPciFunc::value_type& value_)
{
	QRegExp q("(0x)?[0-7]");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PWwn

bool Validatable<Blockexport::Xml::PWwn>::validate(const Blockexport::Xml::PWwn::value_type& value_)
{
	QRegExp q("(0x)?[0-9a-fA-F]{16}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PCpuset

bool Validatable<Blockexport::Xml::PCpuset>::validate(const Blockexport::Xml::PCpuset::value_type& value_)
{
	QRegExp q("([0-9]+(-[0-9]+)?|\\^[0-9]+)(,([0-9]+(-[0-9]+)?|\\^[0-9]+))*");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PVolName

bool Validatable<Blockexport::Xml::PVolName>::validate(const Blockexport::Xml::PVolName::value_type& value_)
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

bool Traits<Blockexport::Xml::PPortNumber>::parse(const QString& src_, Blockexport::Xml::PPortNumber::value_type& dst_)
{
	bool output = false;
	dst_ = src_.toInt(&output);
	return output;
}

QString Traits<Blockexport::Xml::PPortNumber>::generate(Blockexport::Xml::PPortNumber::value_type src_)
{
	return QString::number(src_);
}

bool Validatable<Blockexport::Xml::PPortNumber>::validate(Blockexport::Xml::PPortNumber::value_type value_)
{
	if (-1 > value_)
		return false;

	if (65535 < value_)
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PIobase

bool Validatable<Blockexport::Xml::PIobase>::validate(const Blockexport::Xml::PIobase::value_type& value_)
{
	QRegExp q("0x[a-fA-F0-9]{1,4}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PIrq

bool Validatable<Blockexport::Xml::PIrq>::validate(const Blockexport::Xml::PIrq::value_type& value_)
{
	QRegExp q("0x[a-fA-F0-9]");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PCompat

bool Validatable<Blockexport::Xml::PCompat>::validate(const Blockexport::Xml::PCompat::value_type& value_)
{
	QRegExp q("[0-9]+\\.[0-9]+");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PVirtualPortProfileID

bool Validatable<Blockexport::Xml::PVirtualPortProfileID>::validate(const Blockexport::Xml::PVirtualPortProfileID::value_type& value_)
{
	if (39 < value_.length())
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PSpeed

bool Traits<Blockexport::Xml::PSpeed>::parse(const QString& src_, Blockexport::Xml::PSpeed::value_type& dst_)
{
	QRegExp q("[0-9]+");
	if (!q.exactMatch(src_))
		return false;
	bool output = false;
	dst_ = src_.toUInt(&output);
	return output;
}

QString Traits<Blockexport::Xml::PSpeed>::generate(Blockexport::Xml::PSpeed::value_type src_)
{
	return QString::number(src_);
}

bool Validatable<Blockexport::Xml::PSpeed>::validate(Blockexport::Xml::PSpeed::value_type value_)
{
	if (1 > value_)
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PBurstSize

bool Traits<Blockexport::Xml::PBurstSize>::parse(const QString& src_, Blockexport::Xml::PBurstSize::value_type& dst_)
{
	QRegExp q("[0-9]+");
	if (!q.exactMatch(src_))
		return false;
	bool output = false;
	dst_ = src_.toUInt(&output);
	return output;
}

QString Traits<Blockexport::Xml::PBurstSize>::generate(Blockexport::Xml::PBurstSize::value_type src_)
{
	return QString::number(src_);
}

bool Validatable<Blockexport::Xml::PBurstSize>::validate(Blockexport::Xml::PBurstSize::value_type value_)
{
	if (1 > value_)
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PUnsignedShort

bool Traits<Blockexport::Xml::PUnsignedShort>::parse(const QString& src_, Blockexport::Xml::PUnsignedShort::value_type& dst_)
{
	bool output = false;
	dst_ = src_.toInt(&output);
	return output;
}

QString Traits<Blockexport::Xml::PUnsignedShort>::generate(Blockexport::Xml::PUnsignedShort::value_type src_)
{
	return QString::number(src_);
}

bool Validatable<Blockexport::Xml::PUnsignedShort>::validate(Blockexport::Xml::PUnsignedShort::value_type value_)
{
	if (0 > value_)
		return false;

	if (65535 < value_)
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PProtocol

bool Validatable<Blockexport::Xml::PProtocol>::validate(const Blockexport::Xml::PProtocol::value_type& value_)
{
	QRegExp q("(tcp)|(udp)");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PAddrFamily

bool Validatable<Blockexport::Xml::PAddrFamily>::validate(const Blockexport::Xml::PAddrFamily::value_type& value_)
{
	QRegExp q("(ipv4)|(ipv6)");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PId

bool Traits<Blockexport::Xml::PId>::parse(const QString& src_, Blockexport::Xml::PId::value_type& dst_)
{
	bool output = false;
	dst_ = src_.toUInt(&output);
	return output;
}

QString Traits<Blockexport::Xml::PId>::generate(Blockexport::Xml::PId::value_type src_)
{
	return QString::number(src_);
}

bool Validatable<Blockexport::Xml::PId>::validate(Blockexport::Xml::PId::value_type value_)
{
	if (4095 < value_)
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PPort

bool Traits<Blockexport::Xml::PPort>::parse(const QString& src_, Blockexport::Xml::PPort::value_type& dst_)
{
	bool output = false;
	dst_ = src_.toInt(&output);
	return output;
}

QString Traits<Blockexport::Xml::PPort>::generate(Blockexport::Xml::PPort::value_type src_)
{
	return QString::number(src_);
}

bool Validatable<Blockexport::Xml::PPort>::validate(Blockexport::Xml::PPort::value_type value_)
{
	if (1 > value_)
		return false;

	if (65535 < value_)
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PTitle

bool Validatable<Blockexport::Xml::PTitle>::validate(const Blockexport::Xml::PTitle::value_type& value_)
{
	QRegExp q("[^\n]+");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PMachine

bool Validatable<Blockexport::Xml::PMachine>::validate(const Blockexport::Xml::PMachine::value_type& value_)
{
	QRegExp q("[a-zA-Z0-9_\\.\\-]+");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PReadIopsSec

bool Traits<Blockexport::Xml::PReadIopsSec>::parse(const QString& src_, Blockexport::Xml::PReadIopsSec::value_type& dst_)
{
	bool output = false;
	dst_ = src_.toUInt(&output);
	return output;
}

QString Traits<Blockexport::Xml::PReadIopsSec>::generate(Blockexport::Xml::PReadIopsSec::value_type src_)
{
	return QString::number(src_);
}

///////////////////////////////////////////////////////////////////////////////
// struct PWriteIopsSec

bool Traits<Blockexport::Xml::PWriteIopsSec>::parse(const QString& src_, Blockexport::Xml::PWriteIopsSec::value_type& dst_)
{
	bool output = false;
	dst_ = src_.toUInt(&output);
	return output;
}

QString Traits<Blockexport::Xml::PWriteIopsSec>::generate(Blockexport::Xml::PWriteIopsSec::value_type src_)
{
	return QString::number(src_);
}

///////////////////////////////////////////////////////////////////////////////
// struct PReadBytesSec

bool Traits<Blockexport::Xml::PReadBytesSec>::parse(const QString& src_, Blockexport::Xml::PReadBytesSec::value_type& dst_)
{
	bool output = false;
	dst_ = src_.toULong(&output);
	return output;
}

QString Traits<Blockexport::Xml::PReadBytesSec>::generate(Blockexport::Xml::PReadBytesSec::value_type src_)
{
	return QString::number(src_);
}

///////////////////////////////////////////////////////////////////////////////
// struct PWriteBytesSec

bool Traits<Blockexport::Xml::PWriteBytesSec>::parse(const QString& src_, Blockexport::Xml::PWriteBytesSec::value_type& dst_)
{
	bool output = false;
	dst_ = src_.toULong(&output);
	return output;
}

QString Traits<Blockexport::Xml::PWriteBytesSec>::generate(Blockexport::Xml::PWriteBytesSec::value_type src_)
{
	return QString::number(src_);
}

///////////////////////////////////////////////////////////////////////////////
// struct PVendor

bool Validatable<Blockexport::Xml::PVendor>::validate(const Blockexport::Xml::PVendor::value_type& value_)
{
	QRegExp q("[\\x20-\\x7E]{0,8}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PProduct

bool Validatable<Blockexport::Xml::PProduct>::validate(const Blockexport::Xml::PProduct::value_type& value_)
{
	QRegExp q("[\\x20-\\x7E]{0,16}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PDiskTarget

bool Validatable<Blockexport::Xml::PDiskTarget>::validate(const Blockexport::Xml::PDiskTarget::value_type& value_)
{
	QRegExp q("(ioemu:)?(fd|hd|sd|vd|xvd|ubd)[a-zA-Z0-9_]+");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PCyls

bool Traits<Blockexport::Xml::PCyls>::parse(const QString& src_, Blockexport::Xml::PCyls::value_type& dst_)
{
	bool output = false;
	dst_ = src_.toInt(&output);
	return output;
}

QString Traits<Blockexport::Xml::PCyls>::generate(Blockexport::Xml::PCyls::value_type src_)
{
	return QString::number(src_);
}

///////////////////////////////////////////////////////////////////////////////
// struct PHeads

bool Traits<Blockexport::Xml::PHeads>::parse(const QString& src_, Blockexport::Xml::PHeads::value_type& dst_)
{
	bool output = false;
	dst_ = src_.toInt(&output);
	return output;
}

QString Traits<Blockexport::Xml::PHeads>::generate(Blockexport::Xml::PHeads::value_type src_)
{
	return QString::number(src_);
}

///////////////////////////////////////////////////////////////////////////////
// struct PSecs

bool Traits<Blockexport::Xml::PSecs>::parse(const QString& src_, Blockexport::Xml::PSecs::value_type& dst_)
{
	bool output = false;
	dst_ = src_.toInt(&output);
	return output;
}

QString Traits<Blockexport::Xml::PSecs>::generate(Blockexport::Xml::PSecs::value_type src_)
{
	return QString::number(src_);
}

///////////////////////////////////////////////////////////////////////////////
// struct PLogicalBlockSize

bool Traits<Blockexport::Xml::PLogicalBlockSize>::parse(const QString& src_, Blockexport::Xml::PLogicalBlockSize::value_type& dst_)
{
	bool output = false;
	dst_ = src_.toInt(&output);
	return output;
}

QString Traits<Blockexport::Xml::PLogicalBlockSize>::generate(Blockexport::Xml::PLogicalBlockSize::value_type src_)
{
	return QString::number(src_);
}

///////////////////////////////////////////////////////////////////////////////
// struct PPhysicalBlockSize

bool Traits<Blockexport::Xml::PPhysicalBlockSize>::parse(const QString& src_, Blockexport::Xml::PPhysicalBlockSize::value_type& dst_)
{
	bool output = false;
	dst_ = src_.toInt(&output);
	return output;
}

QString Traits<Blockexport::Xml::PPhysicalBlockSize>::generate(Blockexport::Xml::PPhysicalBlockSize::value_type src_)
{
	return QString::number(src_);
}

///////////////////////////////////////////////////////////////////////////////
// struct PType

bool Validatable<Blockexport::Xml::PType>::validate(const Blockexport::Xml::PType::value_type& value_)
{
	QRegExp q("[a-zA-Z0-9\\-_]+");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PPasswdValidTo

bool Traits<Blockexport::Xml::PPasswdValidTo>::parse(const QString& src_, Blockexport::Xml::PPasswdValidTo::value_type& dst_)
{
	dst_ = QDateTime::fromString(src_);
	return !dst_.isNull();
}

QString Traits<Blockexport::Xml::PPasswdValidTo>::generate(const Blockexport::Xml::PPasswdValidTo::value_type& src_)
{
	return src_.toString();
}

///////////////////////////////////////////////////////////////////////////////
// struct PVendorId

bool Validatable<Blockexport::Xml::PVendorId>::validate(const Blockexport::Xml::PVendorId::value_type& value_)
{
	QRegExp q("[^,]{12}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PSysinfoValue

bool Validatable<Blockexport::Xml::PSysinfoValue>::validate(const Blockexport::Xml::PSysinfoValue::value_type& value_)
{
	QRegExp q("[a-zA-Z0-9/\\-_\\. \\(\\)]+");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PTotalBytesSec

bool Traits<Blockexport::Xml::PTotalBytesSec>::parse(const QString& src_, Blockexport::Xml::PTotalBytesSec::value_type& dst_)
{
	bool output = false;
	dst_ = src_.toULong(&output);
	return output;
}

QString Traits<Blockexport::Xml::PTotalBytesSec>::generate(Blockexport::Xml::PTotalBytesSec::value_type src_)
{
	return QString::number(src_);
}

///////////////////////////////////////////////////////////////////////////////
// struct PTotalIopsSec

bool Traits<Blockexport::Xml::PTotalIopsSec>::parse(const QString& src_, Blockexport::Xml::PTotalIopsSec::value_type& dst_)
{
	bool output = false;
	dst_ = src_.toULong(&output);
	return output;
}

QString Traits<Blockexport::Xml::PTotalIopsSec>::generate(Blockexport::Xml::PTotalIopsSec::value_type src_)
{
	return QString::number(src_);
}

///////////////////////////////////////////////////////////////////////////////
// struct PReadIopsSec1

bool Traits<Blockexport::Xml::PReadIopsSec1>::parse(const QString& src_, Blockexport::Xml::PReadIopsSec1::value_type& dst_)
{
	bool output = false;
	dst_ = src_.toULong(&output);
	return output;
}

QString Traits<Blockexport::Xml::PReadIopsSec1>::generate(Blockexport::Xml::PReadIopsSec1::value_type src_)
{
	return QString::number(src_);
}

///////////////////////////////////////////////////////////////////////////////
// struct PWriteIopsSec1

bool Traits<Blockexport::Xml::PWriteIopsSec1>::parse(const QString& src_, Blockexport::Xml::PWriteIopsSec1::value_type& dst_)
{
	bool output = false;
	dst_ = src_.toULong(&output);
	return output;
}

QString Traits<Blockexport::Xml::PWriteIopsSec1>::generate(Blockexport::Xml::PWriteIopsSec1::value_type src_)
{
	return QString::number(src_);
}

///////////////////////////////////////////////////////////////////////////////
// struct PRetries

bool Traits<Blockexport::Xml::PRetries>::parse(const QString& src_, Blockexport::Xml::PRetries::value_type& dst_)
{
	bool output = false;
	dst_ = src_.toUInt(&output);
	return output;
}

QString Traits<Blockexport::Xml::PRetries>::generate(Blockexport::Xml::PRetries::value_type src_)
{
	return QString::number(src_);
}

///////////////////////////////////////////////////////////////////////////////
// struct PValue

bool Validatable<Blockexport::Xml::PValue>::validate(const Blockexport::Xml::PValue::value_type& value_)
{
	QRegExp q("[^,]{0,12}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PCountCPU

bool Traits<Blockexport::Xml::PCountCPU>::parse(const QString& src_, Blockexport::Xml::PCountCPU::value_type& dst_)
{
	QRegExp q("[0-9]+");
	if (!q.exactMatch(src_))
		return false;
	bool output = false;
	dst_ = src_.toUShort(&output);
	return output;
}

QString Traits<Blockexport::Xml::PCountCPU>::generate(Blockexport::Xml::PCountCPU::value_type src_)
{
	return QString::number(src_);
}

bool Validatable<Blockexport::Xml::PCountCPU>::validate(Blockexport::Xml::PCountCPU::value_type value_)
{
	if (1 > value_)
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PVcpuid

bool Traits<Blockexport::Xml::PVcpuid>::parse(const QString& src_, Blockexport::Xml::PVcpuid::value_type& dst_)
{
	QRegExp q("[0-9]+");
	if (!q.exactMatch(src_))
		return false;
	bool output = false;
	dst_ = src_.toUShort(&output);
	return output;
}

QString Traits<Blockexport::Xml::PVcpuid>::generate(Blockexport::Xml::PVcpuid::value_type src_)
{
	return QString::number(src_);
}

///////////////////////////////////////////////////////////////////////////////
// struct PCpushares

bool Traits<Blockexport::Xml::PCpushares>::parse(const QString& src_, Blockexport::Xml::PCpushares::value_type& dst_)
{
	QRegExp q("[0-9]+");
	if (!q.exactMatch(src_))
		return false;
	bool output = false;
	dst_ = src_.toUInt(&output);
	return output;
}

QString Traits<Blockexport::Xml::PCpushares>::generate(Blockexport::Xml::PCpushares::value_type src_)
{
	return QString::number(src_);
}

///////////////////////////////////////////////////////////////////////////////
// struct PCpuperiod

bool Traits<Blockexport::Xml::PCpuperiod>::parse(const QString& src_, Blockexport::Xml::PCpuperiod::value_type& dst_)
{
	QRegExp q("[0-9]+");
	if (!q.exactMatch(src_))
		return false;
	bool output = false;
	dst_ = src_.toULong(&output);
	return output;
}

QString Traits<Blockexport::Xml::PCpuperiod>::generate(Blockexport::Xml::PCpuperiod::value_type src_)
{
	return QString::number(src_);
}

bool Validatable<Blockexport::Xml::PCpuperiod>::validate(Blockexport::Xml::PCpuperiod::value_type value_)
{
	if (1000 > value_)
		return false;

	if (1000000 < value_)
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PCpuquota

bool Traits<Blockexport::Xml::PCpuquota>::parse(const QString& src_, Blockexport::Xml::PCpuquota::value_type& dst_)
{
	QRegExp q("-?[0-9]+");
	if (!q.exactMatch(src_))
		return false;
	bool output = false;
	dst_ = src_.toLong(&output);
	return output;
}

QString Traits<Blockexport::Xml::PCpuquota>::generate(Blockexport::Xml::PCpuquota::value_type src_)
{
	return QString::number(src_);
}

bool Validatable<Blockexport::Xml::PCpuquota>::validate(Blockexport::Xml::PCpuquota::value_type value_)
{
	if (-1 > value_)
		return false;

	if (18446744073709551 < value_)
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PRebootTimeoutDelay

bool Traits<Blockexport::Xml::PRebootTimeoutDelay>::parse(const QString& src_, Blockexport::Xml::PRebootTimeoutDelay::value_type& dst_)
{
	bool output = false;
	dst_ = src_.toShort(&output);
	return output;
}

QString Traits<Blockexport::Xml::PRebootTimeoutDelay>::generate(Blockexport::Xml::PRebootTimeoutDelay::value_type src_)
{
	return QString::number(src_);
}

bool Validatable<Blockexport::Xml::PRebootTimeoutDelay>::validate(Blockexport::Xml::PRebootTimeoutDelay::value_type value_)
{
	if (-1 > value_)
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PWeight

bool Traits<Blockexport::Xml::PWeight>::parse(const QString& src_, Blockexport::Xml::PWeight::value_type& dst_)
{
	QRegExp q("[0-9]+");
	if (!q.exactMatch(src_))
		return false;
	bool output = false;
	dst_ = src_.toUInt(&output);
	return output;
}

QString Traits<Blockexport::Xml::PWeight>::generate(Blockexport::Xml::PWeight::value_type src_)
{
	return QString::number(src_);
}

bool Validatable<Blockexport::Xml::PWeight>::validate(Blockexport::Xml::PWeight::value_type value_)
{
	if (100 > value_)
		return false;

	if (1000 < value_)
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PMemoryKB

bool Traits<Blockexport::Xml::PMemoryKB>::parse(const QString& src_, Blockexport::Xml::PMemoryKB::value_type& dst_)
{
	QRegExp q("[0-9]+");
	if (!q.exactMatch(src_))
		return false;
	bool output = false;
	dst_ = src_.toUInt(&output);
	return output;
}

QString Traits<Blockexport::Xml::PMemoryKB>::generate(Blockexport::Xml::PMemoryKB::value_type src_)
{
	return QString::number(src_);
}

///////////////////////////////////////////////////////////////////////////////
// struct PDomainName

bool Validatable<Blockexport::Xml::PDomainName>::validate(const Blockexport::Xml::PDomainName::value_type& value_)
{
	QRegExp q("[^\n]+");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PDiskSerial

bool Validatable<Blockexport::Xml::PDiskSerial>::validate(const Blockexport::Xml::PDiskSerial::value_type& value_)
{
	QRegExp q("[A-Za-z0-9_\\.\\+\\-]*");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PBridgeMode

bool Validatable<Blockexport::Xml::PBridgeMode>::validate(const Blockexport::Xml::PBridgeMode::value_type& value_)
{
	QRegExp q("(vepa|bridge|private|passthrough)");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PAddrIPorName

bool Validatable<Blockexport::Xml::PAddrIPorName>::validate(const Blockexport::Xml::PAddrIPorName::value_type& value_)
{
	QRegExp q("(([0-2]?[0-9]?[0-9]\\.){3}[0-2]?[0-9]?[0-9])|(([0-9a-fA-F]+|:)+[0-9a-fA-F]+)|([a-zA-Z0-9_\\.\\+\\-]*)");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PUsbIdDefault

bool Validatable<Blockexport::Xml::PUsbIdDefault>::validate(const Blockexport::Xml::PUsbIdDefault::value_type& value_)
{
	QRegExp q("-1");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PUsbId

bool Validatable<Blockexport::Xml::PUsbId>::validate(const Blockexport::Xml::PUsbId::value_type& value_)
{
	QRegExp q("(0x)?[0-9a-fA-F]{1,4}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PUsbVersion

bool Validatable<Blockexport::Xml::PUsbVersion>::validate(const Blockexport::Xml::PUsbVersion::value_type& value_)
{
	QRegExp q("[0-9]{1,2}.[0-9]{1,2}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PUsbAddr

bool Validatable<Blockexport::Xml::PUsbAddr>::validate(const Blockexport::Xml::PUsbAddr::value_type& value_)
{
	QRegExp q("(0x)?[0-9a-fA-F]{1,3}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PUsbClass

bool Validatable<Blockexport::Xml::PUsbClass>::validate(const Blockexport::Xml::PUsbClass::value_type& value_)
{
	QRegExp q("(0x)?[0-9a-fA-F]{1,2}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PUsbPort

bool Validatable<Blockexport::Xml::PUsbPort>::validate(const Blockexport::Xml::PUsbPort::value_type& value_)
{
	QRegExp q("((0x)?[0-9a-fA-F]{1,3}\\.){0,3}(0x)?[0-9a-fA-F]{1,3}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PDriveController

bool Validatable<Blockexport::Xml::PDriveController>::validate(const Blockexport::Xml::PDriveController::value_type& value_)
{
	QRegExp q("[0-9]{1,2}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PDriveBus

bool Validatable<Blockexport::Xml::PDriveBus>::validate(const Blockexport::Xml::PDriveBus::value_type& value_)
{
	QRegExp q("[0-9]{1,2}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PDriveTarget

bool Validatable<Blockexport::Xml::PDriveTarget>::validate(const Blockexport::Xml::PDriveTarget::value_type& value_)
{
	QRegExp q("[0-9]{1,2}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PDriveUnit

bool Validatable<Blockexport::Xml::PDriveUnit>::validate(const Blockexport::Xml::PDriveUnit::value_type& value_)
{
	QRegExp q("[0-9]{1,2}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PFeatureName

bool Validatable<Blockexport::Xml::PFeatureName>::validate(const Blockexport::Xml::PFeatureName::value_type& value_)
{
	QRegExp q("[a-zA-Z0-9\\-_\\.]+");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PTimeDelta

bool Validatable<Blockexport::Xml::PTimeDelta>::validate(const Blockexport::Xml::PTimeDelta::value_type& value_)
{
	QRegExp q("(-|\\+)?[0-9]+");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PTimeZone

bool Validatable<Blockexport::Xml::PTimeZone>::validate(const Blockexport::Xml::PTimeZone::value_type& value_)
{
	QRegExp q("[a-zA-Z0-9_\\.\\+\\-/]+");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PFilterParamName

bool Validatable<Blockexport::Xml::PFilterParamName>::validate(const Blockexport::Xml::PFilterParamName::value_type& value_)
{
	QRegExp q("[a-zA-Z0-9_]+");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PFilterParamValue

bool Validatable<Blockexport::Xml::PFilterParamValue>::validate(const Blockexport::Xml::PFilterParamValue::value_type& value_)
{
	QRegExp q("[a-zA-Z0-9_\\.:]+");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PSpaprvioReg

bool Validatable<Blockexport::Xml::PSpaprvioReg>::validate(const Blockexport::Xml::PSpaprvioReg::value_type& value_)
{
	QRegExp q("(0x)?[0-9a-fA-F]{1,16}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PAliasName

bool Validatable<Blockexport::Xml::PAliasName>::validate(const Blockexport::Xml::PAliasName::value_type& value_)
{
	QRegExp q("[a-zA-Z0-9_\\-.]+");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PData2090

bool Validatable<Blockexport::Xml::PData2090>::validate(const Blockexport::Xml::PData2090::value_type& value_)
{
	QRegExp q("0x[0-9a-eA-E][0-9a-fA-F]?");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PData2092

bool Validatable<Blockexport::Xml::PData2092>::validate(const Blockexport::Xml::PData2092::value_type& value_)
{
	QRegExp q("0x[fF][0-9a-eA-E]?");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PData2094

bool Traits<Blockexport::Xml::PData2094>::parse(const QString& src_, Blockexport::Xml::PData2094::value_type& dst_)
{
	bool output = false;
	dst_ = src_.toInt(&output);
	return output;
}

QString Traits<Blockexport::Xml::PData2094>::generate(Blockexport::Xml::PData2094::value_type src_)
{
	return QString::number(src_);
}

bool Validatable<Blockexport::Xml::PData2094>::validate(Blockexport::Xml::PData2094::value_type value_)
{
	if (0 > value_)
		return false;

	if (254 < value_)
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PCcwSsidRange

bool Validatable<Blockexport::Xml::PCcwSsidRange>::validate(const Blockexport::Xml::PCcwSsidRange::value_type& value_)
{
	QRegExp q("(0x)?[0-3]");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PData2096

bool Validatable<Blockexport::Xml::PData2096>::validate(const Blockexport::Xml::PData2096::value_type& value_)
{
	QRegExp q("0x[0-9a-fA-F]{1,4}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PData2098

bool Traits<Blockexport::Xml::PData2098>::parse(const QString& src_, Blockexport::Xml::PData2098::value_type& dst_)
{
	bool output = false;
	dst_ = src_.toInt(&output);
	return output;
}

QString Traits<Blockexport::Xml::PData2098>::generate(Blockexport::Xml::PData2098::value_type src_)
{
	return QString::number(src_);
}

bool Validatable<Blockexport::Xml::PData2098>::validate(Blockexport::Xml::PData2098::value_type value_)
{
	if (0 > value_)
		return false;

	if (65535 < value_)
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct VName

bool Traits<Blockexport::Xml::VName>::parse(const QString& src_, Blockexport::Xml::VName& dst_)
{
	int x;
	mpl::at_c<Blockexport::Xml::VName::types, 0>::type a0;
	x = Marshal<Blockexport::Xml::PDiskTarget>::setString(src_, a0);
	if (0 < x)
	{
		dst_ = a0;
		return true;
	}
	mpl::at_c<Blockexport::Xml::VName::types, 1>::type a1;
	x = Marshal<Blockexport::Xml::PAbsFilePath>::setString(src_, a1);
	if (0 < x)
	{
		dst_ = a1;
		return true;
	}

	return false;
}

QString Traits<Blockexport::Xml::VName>::generate(const Blockexport::Xml::VName& src_)
{
	switch (src_.which())
	{
	case 0:
		return Marshal<Blockexport::Xml::PDiskTarget>::getString(boost::get<mpl::at_c<Blockexport::Xml::VName::types, 0>::type>(src_));
	case 1:
		return Marshal<Blockexport::Xml::PAbsFilePath>::getString(boost::get<mpl::at_c<Blockexport::Xml::VName::types, 1>::type>(src_));
	}
	return QString();
}

} // namespace Libvirt
