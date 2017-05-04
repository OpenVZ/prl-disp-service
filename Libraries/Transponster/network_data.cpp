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

#include "network_data.h"
#include <QRegExp>

namespace Libvirt
{
///////////////////////////////////////////////////////////////////////////////
// struct PUnsignedInt

bool Traits<Network::Xml::PUnsignedInt>::parse(const QString& src_, Network::Xml::PUnsignedInt::value_type& dst_)
{
	QRegExp q("[0-9]+");
	if (!q.exactMatch(src_))
		return false;
	bool output = false;
	dst_ = src_.toUInt(&output);
	return output;
}

QString Traits<Network::Xml::PUnsignedInt>::generate(Network::Xml::PUnsignedInt::value_type src_)
{
	return QString::number(src_);
}

///////////////////////////////////////////////////////////////////////////////
// struct PUnsignedLong

bool Traits<Network::Xml::PUnsignedLong>::parse(const QString& src_, Network::Xml::PUnsignedLong::value_type& dst_)
{
	QRegExp q("[0-9]+");
	if (!q.exactMatch(src_))
		return false;
	bool output = false;
	dst_ = src_.toULong(&output);
	return output;
}

QString Traits<Network::Xml::PUnsignedLong>::generate(Network::Xml::PUnsignedLong::value_type src_)
{
	return QString::number(src_);
}

///////////////////////////////////////////////////////////////////////////////
// struct PHexuint

bool Validatable<Network::Xml::PHexuint>::validate(const Network::Xml::PHexuint::value_type& value_)
{
	QRegExp q("(0x)?[0-9a-f]+");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PPositiveInteger

bool Traits<Network::Xml::PPositiveInteger>::parse(const QString& src_, Network::Xml::PPositiveInteger::value_type& dst_)
{
	QRegExp q("[0-9]+");
	if (!q.exactMatch(src_))
		return false;
	bool output = false;
	dst_ = src_.toUInt(&output);
	return output;
}

QString Traits<Network::Xml::PPositiveInteger>::generate(Network::Xml::PPositiveInteger::value_type src_)
{
	return QString::number(src_);
}

///////////////////////////////////////////////////////////////////////////////
// struct POctalMode

bool Traits<Network::Xml::POctalMode>::parse(const QString& src_, Network::Xml::POctalMode::value_type& dst_)
{
	QRegExp q("[0-7]+");
	if (!q.exactMatch(src_))
		return false;
	bool output = false;
	dst_ = src_.toUInt(&output);
	return output;
}

QString Traits<Network::Xml::POctalMode>::generate(Network::Xml::POctalMode::value_type src_)
{
	return QString::number(src_);
}

///////////////////////////////////////////////////////////////////////////////
// struct PData1140

bool Validatable<Network::Xml::PData1140>::validate(const Network::Xml::PData1140::value_type& value_)
{
	QRegExp q("0x[0-9a-fA-F]{1,2}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PData1142

bool Traits<Network::Xml::PData1142>::parse(const QString& src_, Network::Xml::PData1142::value_type& dst_)
{
	bool output = false;
	dst_ = src_.toInt(&output);
	return output;
}

QString Traits<Network::Xml::PData1142>::generate(Network::Xml::PData1142::value_type src_)
{
	return QString::number(src_);
}

bool Validatable<Network::Xml::PData1142>::validate(Network::Xml::PData1142::value_type value_)
{
	if (0 > value_)
		return false;

	if (255 < value_)
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PData1144

bool Validatable<Network::Xml::PData1144>::validate(const Network::Xml::PData1144::value_type& value_)
{
	QRegExp q("0x[0-9a-fA-F]{1,6}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PData1146

bool Traits<Network::Xml::PData1146>::parse(const QString& src_, Network::Xml::PData1146::value_type& dst_)
{
	bool output = false;
	dst_ = src_.toInt(&output);
	return output;
}

QString Traits<Network::Xml::PData1146>::generate(Network::Xml::PData1146::value_type src_)
{
	return QString::number(src_);
}

bool Validatable<Network::Xml::PData1146>::validate(Network::Xml::PData1146::value_type value_)
{
	if (0 > value_)
		return false;

	if (16777215 < value_)
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PData1148

bool Validatable<Network::Xml::PData1148>::validate(const Network::Xml::PData1148::value_type& value_)
{
	QRegExp q("[a-fA-F0-9]{32}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PData1150

bool Validatable<Network::Xml::PData1150>::validate(const Network::Xml::PData1150::value_type& value_)
{
	QRegExp q("[a-fA-F0-9]{8}\\-([a-fA-F0-9]{4}\\-){3}[a-fA-F0-9]{12}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PUniMacAddr

bool Validatable<Network::Xml::PUniMacAddr>::validate(const Network::Xml::PUniMacAddr::value_type& value_)
{
	QRegExp q("[a-fA-F0-9][02468aAcCeE](:[a-fA-F0-9]{2}){5}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PMultiMacAddr

bool Validatable<Network::Xml::PMultiMacAddr>::validate(const Network::Xml::PMultiMacAddr::value_type& value_)
{
	QRegExp q("[a-fA-F0-9][13579bBdDfF](:[a-fA-F0-9]{2}){5}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PMacAddr

bool Validatable<Network::Xml::PMacAddr>::validate(const Network::Xml::PMacAddr::value_type& value_)
{
	QRegExp q("[a-fA-F0-9]{2}(:[a-fA-F0-9]{2}){5}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PDuidLLT

bool Validatable<Network::Xml::PDuidLLT>::validate(const Network::Xml::PDuidLLT::value_type& value_)
{
	QRegExp q("[0]{1,2}:[0]{0,1}[1]:[0]{1,2}:[0]{0,1}[a-fA-F1-9](:[a-fA-F0-9]{1,2}){4}(:[a-fA-F0-9]{1,2}){6,8}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PDuidEN

bool Validatable<Network::Xml::PDuidEN>::validate(const Network::Xml::PDuidEN::value_type& value_)
{
	QRegExp q("[0]{1,2}:[0]{0,1}[2](:[a-fA-F0-9]{1,2}){4}(:[a-fA-F0-9]{1,2}){1,124}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PDuidLL

bool Validatable<Network::Xml::PDuidLL>::validate(const Network::Xml::PDuidLL::value_type& value_)
{
	QRegExp q("[0]{1,2}:[0]{0,1}[3]:[0]{1,2}:[0]{0,1}[a-fA-F1-9](:[a-fA-F0-9]{1,2}){6,8}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PDuidUUID

bool Validatable<Network::Xml::PDuidUUID>::validate(const Network::Xml::PDuidUUID::value_type& value_)
{
	QRegExp q("[0]{1,2}:[0]{0,1}[4](:[a-fA-F0-9]{1,2}){16}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PIpv4Addr

bool Validatable<Network::Xml::PIpv4Addr>::validate(const Network::Xml::PIpv4Addr::value_type& value_)
{
	QRegExp q("(((25[0-5])|(2[0-4][0-9])|(1[0-9]{2})|([1-9][0-9])|([0-9]))\\.){3}((25[0-5])|(2[0-4][0-9])|(1[0-9]{2})|([1-9][0-9])|([0-9]))");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PIpv6Addr

bool Validatable<Network::Xml::PIpv6Addr>::validate(const Network::Xml::PIpv6Addr::value_type& value_)
{
	QRegExp q("(([0-9A-Fa-f]{1,4}:){7}[0-9A-Fa-f]{1,4})|(([0-9A-Fa-f]{1,4}:){6}:[0-9A-Fa-f]{1,4})|(([0-9A-Fa-f]{1,4}:){5}:([0-9A-Fa-f]{1,4}:)?[0-9A-Fa-f]{1,4})|(([0-9A-Fa-f]{1,4}:){4}:([0-9A-Fa-f]{1,4}:){0,2}[0-9A-Fa-f]{1,4})|(([0-9A-Fa-f]{1,4}:){3}:([0-9A-Fa-f]{1,4}:){0,3}[0-9A-Fa-f]{1,4})|(([0-9A-Fa-f]{1,4}:){2}:([0-9A-Fa-f]{1,4}:){0,4}[0-9A-Fa-f]{1,4})|(([0-9A-Fa-f]{1,4}:){6}(((25[0-5])|(2[0-4][0-9])|(1[0-9]{2})|([1-9][0-9])|([0-9]))\\.){3}((25[0-5])|(2[0-4][0-9])|(1[0-9]{2})|([1-9][0-9])|([0-9])))|(([0-9A-Fa-f]{1,4}:){0,5}:(((25[0-5])|(2[0-4][0-9])|(1[0-9]{2})|([1-9][0-9])|([0-9]))\\.){3}((25[0-5])|(2[0-4][0-9])|(1[0-9]{2})|([1-9][0-9])|([0-9])))|(::([0-9A-Fa-f]{1,4}:){0,5}(((25[0-5])|(2[0-4][0-9])|(1[0-9]{2})|([1-9][0-9])|([0-9]))\\.){3}((25[0-5])|(2[0-4][0-9])|(1[0-9]{2})|([1-9][0-9])|([0-9])))|([0-9A-Fa-f]{1,4}::([0-9A-Fa-f]{1,4}:){0,5}[0-9A-Fa-f]{1,4})|(::([0-9A-Fa-f]{1,4}:){0,6}[0-9A-Fa-f]{1,4})|(([0-9A-Fa-f]{1,4}:){1,7}:)");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PIpv4Prefix

bool Traits<Network::Xml::PIpv4Prefix>::parse(const QString& src_, Network::Xml::PIpv4Prefix::value_type& dst_)
{
	bool output = false;
	dst_ = src_.toUInt(&output);
	return output;
}

QString Traits<Network::Xml::PIpv4Prefix>::generate(Network::Xml::PIpv4Prefix::value_type src_)
{
	return QString::number(src_);
}

bool Validatable<Network::Xml::PIpv4Prefix>::validate(Network::Xml::PIpv4Prefix::value_type value_)
{
	if (32 < value_)
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PIpv6Prefix

bool Traits<Network::Xml::PIpv6Prefix>::parse(const QString& src_, Network::Xml::PIpv6Prefix::value_type& dst_)
{
	bool output = false;
	dst_ = src_.toUInt(&output);
	return output;
}

QString Traits<Network::Xml::PIpv6Prefix>::generate(Network::Xml::PIpv6Prefix::value_type src_)
{
	return QString::number(src_);
}

bool Validatable<Network::Xml::PIpv6Prefix>::validate(Network::Xml::PIpv6Prefix::value_type value_)
{
	if (128 < value_)
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PGenericName

bool Validatable<Network::Xml::PGenericName>::validate(const Network::Xml::PGenericName::value_type& value_)
{
	QRegExp q("[a-zA-Z0-9_\\+\\-]+");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PDnsName

bool Validatable<Network::Xml::PDnsName>::validate(const Network::Xml::PDnsName::value_type& value_)
{
	QRegExp q("[a-zA-Z0-9\\.\\-]+");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PDeviceName

bool Validatable<Network::Xml::PDeviceName>::validate(const Network::Xml::PDeviceName::value_type& value_)
{
	QRegExp q("[a-zA-Z0-9_\\.\\-\\\\:/]+");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PFilePath

bool Validatable<Network::Xml::PFilePath>::validate(const Network::Xml::PFilePath::value_type& value_)
{
	QRegExp q("[a-zA-Z0-9_\\.\\+\\-\\\\&\"{}'<>/%:\\s]+");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PDirPath

bool Validatable<Network::Xml::PDirPath>::validate(const Network::Xml::PDirPath::value_type& value_)
{
	QRegExp q("[a-zA-Z0-9_\\.\\+\\-\\\\&\"{}'<>/%:\\s]+");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PAbsFilePath

bool Validatable<Network::Xml::PAbsFilePath>::validate(const Network::Xml::PAbsFilePath::value_type& value_)
{
	QRegExp q("/[a-zA-Z0-9_\\.\\+\\-\\\\&\"{}'<>/%,:\\s]+");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PAbsDirPath

bool Validatable<Network::Xml::PAbsDirPath>::validate(const Network::Xml::PAbsDirPath::value_type& value_)
{
	QRegExp q("/[a-zA-Z0-9_\\.\\+\\-\\\\&\"{}'<>/%:\\s]*");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PUnit

bool Validatable<Network::Xml::PUnit>::validate(const Network::Xml::PUnit::value_type& value_)
{
	QRegExp q("([bB]([yY][tT][eE][sS]?)?)|([kKmMgGtTpPeE]([iI]?[bB])?)");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PPciDomain

bool Validatable<Network::Xml::PPciDomain>::validate(const Network::Xml::PPciDomain::value_type& value_)
{
	QRegExp q("(0x)?[0-9a-fA-F]{1,4}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PPciBus

bool Validatable<Network::Xml::PPciBus>::validate(const Network::Xml::PPciBus::value_type& value_)
{
	QRegExp q("(0x)?[0-9a-fA-F]{1,2}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PPciSlot

bool Validatable<Network::Xml::PPciSlot>::validate(const Network::Xml::PPciSlot::value_type& value_)
{
	QRegExp q("(0x)?[0-1]?[0-9a-fA-F]");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PPciFunc

bool Validatable<Network::Xml::PPciFunc>::validate(const Network::Xml::PPciFunc::value_type& value_)
{
	QRegExp q("(0x)?[0-7]");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PWwn

bool Validatable<Network::Xml::PWwn>::validate(const Network::Xml::PWwn::value_type& value_)
{
	QRegExp q("(0x)?[0-9a-fA-F]{16}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PCpuset

bool Validatable<Network::Xml::PCpuset>::validate(const Network::Xml::PCpuset::value_type& value_)
{
	QRegExp q("([0-9]+(-[0-9]+)?|\\^[0-9]+)(,([0-9]+(-[0-9]+)?|\\^[0-9]+))*");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PVolName

bool Validatable<Network::Xml::PVolName>::validate(const Network::Xml::PVolName::value_type& value_)
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

bool Traits<Network::Xml::PPortNumber>::parse(const QString& src_, Network::Xml::PPortNumber::value_type& dst_)
{
	bool output = false;
	dst_ = src_.toInt(&output);
	return output;
}

QString Traits<Network::Xml::PPortNumber>::generate(Network::Xml::PPortNumber::value_type src_)
{
	return QString::number(src_);
}

bool Validatable<Network::Xml::PPortNumber>::validate(Network::Xml::PPortNumber::value_type value_)
{
	if (-1 > value_)
		return false;

	if (65535 < value_)
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PIobase

bool Validatable<Network::Xml::PIobase>::validate(const Network::Xml::PIobase::value_type& value_)
{
	QRegExp q("0x[a-fA-F0-9]{1,4}");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PIrq

bool Validatable<Network::Xml::PIrq>::validate(const Network::Xml::PIrq::value_type& value_)
{
	QRegExp q("0x[a-fA-F0-9]");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PVirtualPortProfileID

bool Validatable<Network::Xml::PVirtualPortProfileID>::validate(const Network::Xml::PVirtualPortProfileID::value_type& value_)
{
	if (39 < value_.length())
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PSpeed

bool Traits<Network::Xml::PSpeed>::parse(const QString& src_, Network::Xml::PSpeed::value_type& dst_)
{
	QRegExp q("[0-9]+");
	if (!q.exactMatch(src_))
		return false;
	bool output = false;
	dst_ = src_.toUInt(&output);
	return output;
}

QString Traits<Network::Xml::PSpeed>::generate(Network::Xml::PSpeed::value_type src_)
{
	return QString::number(src_);
}

bool Validatable<Network::Xml::PSpeed>::validate(Network::Xml::PSpeed::value_type value_)
{
	if (1 > value_)
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PBurstSize

bool Traits<Network::Xml::PBurstSize>::parse(const QString& src_, Network::Xml::PBurstSize::value_type& dst_)
{
	QRegExp q("[0-9]+");
	if (!q.exactMatch(src_))
		return false;
	bool output = false;
	dst_ = src_.toUInt(&output);
	return output;
}

QString Traits<Network::Xml::PBurstSize>::generate(Network::Xml::PBurstSize::value_type src_)
{
	return QString::number(src_);
}

bool Validatable<Network::Xml::PBurstSize>::validate(Network::Xml::PBurstSize::value_type value_)
{
	if (1 > value_)
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PUnsignedShort

bool Traits<Network::Xml::PUnsignedShort>::parse(const QString& src_, Network::Xml::PUnsignedShort::value_type& dst_)
{
	bool output = false;
	dst_ = src_.toInt(&output);
	return output;
}

QString Traits<Network::Xml::PUnsignedShort>::generate(Network::Xml::PUnsignedShort::value_type src_)
{
	return QString::number(src_);
}

bool Validatable<Network::Xml::PUnsignedShort>::validate(Network::Xml::PUnsignedShort::value_type value_)
{
	if (0 > value_)
		return false;

	if (65535 < value_)
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PProtocol

bool Validatable<Network::Xml::PProtocol>::validate(const Network::Xml::PProtocol::value_type& value_)
{
	QRegExp q("(tcp)|(udp)");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PAddrFamily

bool Validatable<Network::Xml::PAddrFamily>::validate(const Network::Xml::PAddrFamily::value_type& value_)
{
	QRegExp q("(ipv4)|(ipv6)");
	if (!q.exactMatch(value_))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PId

bool Traits<Network::Xml::PId>::parse(const QString& src_, Network::Xml::PId::value_type& dst_)
{
	bool output = false;
	dst_ = src_.toUInt(&output);
	return output;
}

QString Traits<Network::Xml::PId>::generate(Network::Xml::PId::value_type src_)
{
	return QString::number(src_);
}

bool Validatable<Network::Xml::PId>::validate(Network::Xml::PId::value_type value_)
{
	if (4095 < value_)
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PPort

bool Traits<Network::Xml::PPort>::parse(const QString& src_, Network::Xml::PPort::value_type& dst_)
{
	bool output = false;
	dst_ = src_.toInt(&output);
	return output;
}

QString Traits<Network::Xml::PPort>::generate(Network::Xml::PPort::value_type src_)
{
	return QString::number(src_);
}

bool Validatable<Network::Xml::PPort>::validate(Network::Xml::PPort::value_type value_)
{
	if (1 > value_)
		return false;

	if (65535 < value_)
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct PConnections

bool Traits<Network::Xml::PConnections>::parse(const QString& src_, Network::Xml::PConnections::value_type& dst_)
{
	bool output = false;
	dst_ = src_.toUInt(&output);
	return output;
}

QString Traits<Network::Xml::PConnections>::generate(Network::Xml::PConnections::value_type src_)
{
	return QString::number(src_);
}

///////////////////////////////////////////////////////////////////////////////
// struct PDelay

bool Traits<Network::Xml::PDelay>::parse(const QString& src_, Network::Xml::PDelay::value_type& dst_)
{
	bool output = false;
	dst_ = src_.toULong(&output);
	return output;
}

QString Traits<Network::Xml::PDelay>::generate(Network::Xml::PDelay::value_type src_)
{
	return QString::number(src_);
}

///////////////////////////////////////////////////////////////////////////////
// struct VUUID

bool Traits<Network::Xml::VUUID>::parse(const QString& src_, Network::Xml::VUUID& dst_)
{
	int x;
	mpl::at_c<Network::Xml::VUUID::types, 0>::type a0;
	x = Marshal<Network::Xml::PData1148>::setString(src_, a0);
	if (0 < x)
	{
		dst_ = a0;
		return true;
	}
	mpl::at_c<Network::Xml::VUUID::types, 1>::type a1;
	x = Marshal<Network::Xml::PData1150>::setString(src_, a1);
	if (0 < x)
	{
		dst_ = a1;
		return true;
	}

	return false;
}

QString Traits<Network::Xml::VUUID>::generate(const Network::Xml::VUUID& src_)
{
	switch (src_.which())
	{
	case 0:
		return Marshal<Network::Xml::PData1148>::getString(boost::get<mpl::at_c<Network::Xml::VUUID::types, 0>::type>(src_));
	case 1:
		return Marshal<Network::Xml::PData1150>::getString(boost::get<mpl::at_c<Network::Xml::VUUID::types, 1>::type>(src_));
	}
	return QString();
}

///////////////////////////////////////////////////////////////////////////////
// struct VUint8range

bool Traits<Network::Xml::VUint8range>::parse(const QString& src_, Network::Xml::VUint8range& dst_)
{
	int x;
	mpl::at_c<Network::Xml::VUint8range::types, 0>::type a0;
	x = Marshal<Network::Xml::PData1140>::setString(src_, a0);
	if (0 < x)
	{
		dst_ = a0;
		return true;
	}
	mpl::at_c<Network::Xml::VUint8range::types, 1>::type a1;
	x = Marshal<Network::Xml::PData1142>::setString(src_, a1);
	if (0 < x)
	{
		dst_ = a1;
		return true;
	}

	return false;
}

QString Traits<Network::Xml::VUint8range>::generate(const Network::Xml::VUint8range& src_)
{
	switch (src_.which())
	{
	case 0:
		return Marshal<Network::Xml::PData1140>::getString(boost::get<mpl::at_c<Network::Xml::VUint8range::types, 0>::type>(src_));
	case 1:
		return Marshal<Network::Xml::PData1142>::getString(boost::get<mpl::at_c<Network::Xml::VUint8range::types, 1>::type>(src_));
	}
	return QString();
}

///////////////////////////////////////////////////////////////////////////////
// struct VUint24range

bool Traits<Network::Xml::VUint24range>::parse(const QString& src_, Network::Xml::VUint24range& dst_)
{
	int x;
	mpl::at_c<Network::Xml::VUint24range::types, 0>::type a0;
	x = Marshal<Network::Xml::PData1144>::setString(src_, a0);
	if (0 < x)
	{
		dst_ = a0;
		return true;
	}
	mpl::at_c<Network::Xml::VUint24range::types, 1>::type a1;
	x = Marshal<Network::Xml::PData1146>::setString(src_, a1);
	if (0 < x)
	{
		dst_ = a1;
		return true;
	}

	return false;
}

QString Traits<Network::Xml::VUint24range>::generate(const Network::Xml::VUint24range& src_)
{
	switch (src_.which())
	{
	case 0:
		return Marshal<Network::Xml::PData1144>::getString(boost::get<mpl::at_c<Network::Xml::VUint24range::types, 0>::type>(src_));
	case 1:
		return Marshal<Network::Xml::PData1146>::getString(boost::get<mpl::at_c<Network::Xml::VUint24range::types, 1>::type>(src_));
	}
	return QString();
}

///////////////////////////////////////////////////////////////////////////////
// struct VIpAddr

bool Traits<Network::Xml::VIpAddr>::parse(const QString& src_, Network::Xml::VIpAddr& dst_)
{
	int x;
	mpl::at_c<Network::Xml::VIpAddr::types, 0>::type a0;
	x = Marshal<Network::Xml::PIpv4Addr>::setString(src_, a0);
	if (0 < x)
	{
		dst_ = a0;
		return true;
	}
	mpl::at_c<Network::Xml::VIpAddr::types, 1>::type a1;
	x = Marshal<Network::Xml::PIpv6Addr>::setString(src_, a1);
	if (0 < x)
	{
		dst_ = a1;
		return true;
	}

	return false;
}

QString Traits<Network::Xml::VIpAddr>::generate(const Network::Xml::VIpAddr& src_)
{
	switch (src_.which())
	{
	case 0:
		return Marshal<Network::Xml::PIpv4Addr>::getString(boost::get<mpl::at_c<Network::Xml::VIpAddr::types, 0>::type>(src_));
	case 1:
		return Marshal<Network::Xml::PIpv6Addr>::getString(boost::get<mpl::at_c<Network::Xml::VIpAddr::types, 1>::type>(src_));
	}
	return QString();
}

///////////////////////////////////////////////////////////////////////////////
// struct VIpPrefix

bool Traits<Network::Xml::VIpPrefix>::parse(const QString& src_, Network::Xml::VIpPrefix& dst_)
{
	int x;
	mpl::at_c<Network::Xml::VIpPrefix::types, 0>::type a0;
	x = Marshal<Network::Xml::PIpv4Prefix>::setString(src_, a0);
	if (0 < x)
	{
		dst_ = a0;
		return true;
	}
	mpl::at_c<Network::Xml::VIpPrefix::types, 1>::type a1;
	x = Marshal<Network::Xml::PIpv6Prefix>::setString(src_, a1);
	if (0 < x)
	{
		dst_ = a1;
		return true;
	}

	return false;
}

QString Traits<Network::Xml::VIpPrefix>::generate(const Network::Xml::VIpPrefix& src_)
{
	switch (src_.which())
	{
	case 0:
		return Marshal<Network::Xml::PIpv4Prefix>::getString(boost::get<mpl::at_c<Network::Xml::VIpPrefix::types, 0>::type>(src_));
	case 1:
		return Marshal<Network::Xml::PIpv6Prefix>::getString(boost::get<mpl::at_c<Network::Xml::VIpPrefix::types, 1>::type>(src_));
	}
	return QString();
}

///////////////////////////////////////////////////////////////////////////////
// struct VDUID

bool Traits<Network::Xml::VDUID>::parse(const QString& src_, Network::Xml::VDUID& dst_)
{
	int x;
	mpl::at_c<Network::Xml::VDUID::types, 0>::type a0;
	x = Marshal<Network::Xml::PDuidLLT>::setString(src_, a0);
	if (0 < x)
	{
		dst_ = a0;
		return true;
	}
	mpl::at_c<Network::Xml::VDUID::types, 1>::type a1;
	x = Marshal<Network::Xml::PDuidEN>::setString(src_, a1);
	if (0 < x)
	{
		dst_ = a1;
		return true;
	}
	mpl::at_c<Network::Xml::VDUID::types, 2>::type a2;
	x = Marshal<Network::Xml::PDuidLL>::setString(src_, a2);
	if (0 < x)
	{
		dst_ = a2;
		return true;
	}
	mpl::at_c<Network::Xml::VDUID::types, 3>::type a3;
	x = Marshal<Network::Xml::PDuidUUID>::setString(src_, a3);
	if (0 < x)
	{
		dst_ = a3;
		return true;
	}

	return false;
}

QString Traits<Network::Xml::VDUID>::generate(const Network::Xml::VDUID& src_)
{
	switch (src_.which())
	{
	case 0:
		return Marshal<Network::Xml::PDuidLLT>::getString(boost::get<mpl::at_c<Network::Xml::VDUID::types, 0>::type>(src_));
	case 1:
		return Marshal<Network::Xml::PDuidEN>::getString(boost::get<mpl::at_c<Network::Xml::VDUID::types, 1>::type>(src_));
	case 2:
		return Marshal<Network::Xml::PDuidLL>::getString(boost::get<mpl::at_c<Network::Xml::VDUID::types, 2>::type>(src_));
	case 3:
		return Marshal<Network::Xml::PDuidUUID>::getString(boost::get<mpl::at_c<Network::Xml::VDUID::types, 3>::type>(src_));
	}
	return QString();
}

} // namespace Libvirt
