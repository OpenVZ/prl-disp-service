/*
 * Copyright (c) 2015 Parallels IP Holdings GmbH
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
 * Our contact details: Parallels IP Holdings GmbH, Vordergasse 59, 8200
 * Schaffhausen, Switzerland.
 */

#ifndef __IFACE_TYPE_H__
#define __IFACE_TYPE_H__
#define BOOST_MPL_CFG_NO_PREPROCESSED_HEADERS
#define BOOST_MPL_LIMIT_VECTOR_SIZE 40
#include "base.h"
#include "iface_data.h"
#include "iface_enum.h"
#include "patterns.h"
#include <boost/any.hpp>

namespace Libvirt
{

///////////////////////////////////////////////////////////////////////////////
// struct Link

namespace Iface
{
namespace Xml
{
struct Link
{
	const boost::optional<PUnsignedInt::value_type >& getSpeed() const
	{
		return m_speed;
	}
	void setSpeed(const boost::optional<PUnsignedInt::value_type >& value_)
	{
		m_speed = value_;
	}
	const boost::optional<EState >& getState() const
	{
		return m_state;
	}
	void setState(const boost::optional<EState >& value_)
	{
		m_state = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	boost::optional<PUnsignedInt::value_type > m_speed;
	boost::optional<EState > m_state;
};

} // namespace Xml
} // namespace Iface

///////////////////////////////////////////////////////////////////////////////
// struct Dhcp

namespace Iface
{
namespace Xml
{
struct Dhcp
{
	const boost::optional<EYesOrNo >& getPeerdns() const
	{
		return m_peerdns;
	}
	void setPeerdns(const boost::optional<EYesOrNo >& value_)
	{
		m_peerdns = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	boost::optional<EYesOrNo > m_peerdns;
};

} // namespace Xml
} // namespace Iface

///////////////////////////////////////////////////////////////////////////////
// struct Ip

namespace Iface
{
namespace Xml
{
struct Ip
{
	const PIpv4Addr::value_type& getAddress() const
	{
		return m_address;
	}
	void setAddress(const PIpv4Addr::value_type& value_)
	{
		m_address = value_;
	}
	const boost::optional<PIpv4Prefix::value_type >& getPrefix() const
	{
		return m_prefix;
	}
	void setPrefix(const boost::optional<PIpv4Prefix::value_type >& value_)
	{
		m_prefix = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	PIpv4Addr::value_type m_address;
	boost::optional<PIpv4Prefix::value_type > m_prefix;
};

} // namespace Xml
} // namespace Iface

///////////////////////////////////////////////////////////////////////////////
// struct Variant1243

namespace Iface
{
namespace Xml
{
struct Variant1243
{
	const Ip& getIp() const
	{
		return m_ip;
	}
	void setIp(const Ip& value_)
	{
		m_ip = value_;
	}
	const boost::optional<PIpv4Addr::value_type >& getRoute() const
	{
		return m_route;
	}
	void setRoute(const boost::optional<PIpv4Addr::value_type >& value_)
	{
		m_route = value_;
	}

private:
	Ip m_ip;
	boost::optional<PIpv4Addr::value_type > m_route;
};

} // namespace Xml
} // namespace Iface

///////////////////////////////////////////////////////////////////////////////
// struct VChoice1244

namespace Iface
{
namespace Xml
{
typedef Choice<mpl::vector<Element<Iface::Xml::Dhcp, Name::Strict<1145> >, Variant1243 > > VChoice1244Impl;
typedef VChoice1244Impl::value_type VChoice1244;

} // namespace Xml
} // namespace Iface

///////////////////////////////////////////////////////////////////////////////
// struct Ip1

namespace Iface
{
namespace Xml
{
struct Ip1
{
	const PIpv6Addr::value_type& getAddress() const
	{
		return m_address;
	}
	void setAddress(const PIpv6Addr::value_type& value_)
	{
		m_address = value_;
	}
	const boost::optional<PIpv6Prefix::value_type >& getPrefix() const
	{
		return m_prefix;
	}
	void setPrefix(const boost::optional<PIpv6Prefix::value_type >& value_)
	{
		m_prefix = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	PIpv6Addr::value_type m_address;
	boost::optional<PIpv6Prefix::value_type > m_prefix;
};

} // namespace Xml
} // namespace Iface

///////////////////////////////////////////////////////////////////////////////
// struct Protocol

namespace Iface
{
namespace Xml
{
struct Protocol
{
	Protocol();

	bool getAutoconf() const
	{
		return m_autoconf;
	}
	void setAutoconf(bool value_)
	{
		m_autoconf = value_;
	}
	const boost::optional<Dhcp >& getDhcp() const
	{
		return m_dhcp;
	}
	void setDhcp(const boost::optional<Dhcp >& value_)
	{
		m_dhcp = value_;
	}
	const QList<Ip1>& getIpList() const
	{
		return m_ipList;
	}
	void setIpList(const QList<Ip1>& value_)
	{
		m_ipList = value_;
	}
	const boost::optional<PIpv6Addr::value_type >& getRoute() const
	{
		return m_route;
	}
	void setRoute(const boost::optional<PIpv6Addr::value_type >& value_)
	{
		m_route = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	bool m_autoconf;
	boost::optional<Dhcp > m_dhcp;
	QList<Ip1> m_ipList;
	boost::optional<PIpv6Addr::value_type > m_route;
};

} // namespace Xml
} // namespace Iface

///////////////////////////////////////////////////////////////////////////////
// struct InterfaceAddressing1238

namespace Iface
{
namespace Xml
{
struct InterfaceAddressing1238
{
	const boost::optional<VChoice1244 >& getProtocol() const
	{
		return m_protocol;
	}
	void setProtocol(const boost::optional<VChoice1244 >& value_)
	{
		m_protocol = value_;
	}
	const boost::optional<Protocol >& getProtocol2() const
	{
		return m_protocol2;
	}
	void setProtocol2(const boost::optional<Protocol >& value_)
	{
		m_protocol2 = value_;
	}

private:
	boost::optional<VChoice1244 > m_protocol;
	boost::optional<Protocol > m_protocol2;
};

} // namespace Xml
} // namespace Iface

///////////////////////////////////////////////////////////////////////////////
// struct InterfaceAddressing1239

namespace Iface
{
namespace Xml
{
struct InterfaceAddressing1239
{
	const boost::optional<Protocol >& getProtocol() const
	{
		return m_protocol;
	}
	void setProtocol(const boost::optional<Protocol >& value_)
	{
		m_protocol = value_;
	}
	const boost::optional<VChoice1244 >& getProtocol2() const
	{
		return m_protocol2;
	}
	void setProtocol2(const boost::optional<VChoice1244 >& value_)
	{
		m_protocol2 = value_;
	}

private:
	boost::optional<Protocol > m_protocol;
	boost::optional<VChoice1244 > m_protocol2;
};

} // namespace Xml
} // namespace Iface

///////////////////////////////////////////////////////////////////////////////
// struct VInterfaceAddressing

namespace Iface
{
namespace Xml
{
typedef Choice<mpl::vector<InterfaceAddressing1238, InterfaceAddressing1239 > > VInterfaceAddressingImpl;
typedef VInterfaceAddressingImpl::value_type VInterfaceAddressing;

} // namespace Xml
} // namespace Iface

///////////////////////////////////////////////////////////////////////////////
// struct Interface

namespace Iface
{
namespace Xml
{
struct Interface
{
	Interface();

	EMode getStart() const
	{
		return m_start;
	}
	void setStart(EMode value_)
	{
		m_start = value_;
	}
	const PDeviceName::value_type& getName() const
	{
		return m_name;
	}
	void setName(const PDeviceName::value_type& value_)
	{
		m_name = value_;
	}
	const boost::optional<PMacAddr::value_type >& getMac() const
	{
		return m_mac;
	}
	void setMac(const boost::optional<PMacAddr::value_type >& value_)
	{
		m_mac = value_;
	}
	const boost::optional<Link >& getLink() const
	{
		return m_link;
	}
	void setLink(const boost::optional<Link >& value_)
	{
		m_link = value_;
	}
	const boost::optional<PUnsignedInt::value_type >& getMtu() const
	{
		return m_mtu;
	}
	void setMtu(const boost::optional<PUnsignedInt::value_type >& value_)
	{
		m_mtu = value_;
	}
	const VInterfaceAddressing& getInterfaceAddressing() const
	{
		return m_interfaceAddressing;
	}
	void setInterfaceAddressing(const VInterfaceAddressing& value_)
	{
		m_interfaceAddressing = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	EMode m_start;
	PDeviceName::value_type m_name;
	boost::optional<PMacAddr::value_type > m_mac;
	boost::optional<Link > m_link;
	boost::optional<PUnsignedInt::value_type > m_mtu;
	VInterfaceAddressing m_interfaceAddressing;
};

} // namespace Xml
} // namespace Iface

///////////////////////////////////////////////////////////////////////////////
// struct BasicEthernetContent

namespace Iface
{
namespace Xml
{
struct BasicEthernetContent
{
	const PDeviceName::value_type& getName() const
	{
		return m_name;
	}
	void setName(const PDeviceName::value_type& value_)
	{
		m_name = value_;
	}
	const boost::optional<PMacAddr::value_type >& getMac() const
	{
		return m_mac;
	}
	void setMac(const boost::optional<PMacAddr::value_type >& value_)
	{
		m_mac = value_;
	}
	const boost::optional<Link >& getLink() const
	{
		return m_link;
	}
	void setLink(const boost::optional<Link >& value_)
	{
		m_link = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	PDeviceName::value_type m_name;
	boost::optional<PMacAddr::value_type > m_mac;
	boost::optional<Link > m_link;
};

} // namespace Xml
} // namespace Iface

///////////////////////////////////////////////////////////////////////////////
// struct VlanInterfaceCommon

namespace Iface
{
namespace Xml
{
struct VlanInterfaceCommon
{
	const boost::optional<PDeviceName::value_type >& getName() const
	{
		return m_name;
	}
	void setName(const boost::optional<PDeviceName::value_type >& value_)
	{
		m_name = value_;
	}
	const boost::optional<Link >& getLink() const
	{
		return m_link;
	}
	void setLink(const boost::optional<Link >& value_)
	{
		m_link = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;

private:
	boost::optional<PDeviceName::value_type > m_name;
	boost::optional<Link > m_link;
};

} // namespace Xml
} // namespace Iface

///////////////////////////////////////////////////////////////////////////////
// struct Vlan

namespace Iface
{
namespace Xml
{
struct Vlan
{
	Vlan();

	PVlanId::value_type getTag() const
	{
		return m_tag;
	}
	void setTag(PVlanId::value_type value_)
	{
		m_tag = value_;
	}
	const PDeviceName::value_type& getInterface() const
	{
		return m_interface;
	}
	void setInterface(const PDeviceName::value_type& value_)
	{
		m_interface = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	PVlanId::value_type m_tag;
	PDeviceName::value_type m_interface;
};

} // namespace Xml
} // namespace Iface

///////////////////////////////////////////////////////////////////////////////
// struct Interface1

namespace Iface
{
namespace Xml
{
struct Interface1
{
	const VlanInterfaceCommon& getVlanInterfaceCommon() const
	{
		return m_vlanInterfaceCommon;
	}
	void setVlanInterfaceCommon(const VlanInterfaceCommon& value_)
	{
		m_vlanInterfaceCommon = value_;
	}
	const Vlan& getVlan() const
	{
		return m_vlan;
	}
	void setVlan(const Vlan& value_)
	{
		m_vlan = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	VlanInterfaceCommon m_vlanInterfaceCommon;
	Vlan m_vlan;
};

} // namespace Xml
} // namespace Iface

///////////////////////////////////////////////////////////////////////////////
// struct BondInterfaceCommon

namespace Iface
{
namespace Xml
{
struct BondInterfaceCommon
{
	const PDeviceName::value_type& getName() const
	{
		return m_name;
	}
	void setName(const PDeviceName::value_type& value_)
	{
		m_name = value_;
	}
	const boost::optional<Link >& getLink() const
	{
		return m_link;
	}
	void setLink(const boost::optional<Link >& value_)
	{
		m_link = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;

private:
	PDeviceName::value_type m_name;
	boost::optional<Link > m_link;
};

} // namespace Xml
} // namespace Iface

///////////////////////////////////////////////////////////////////////////////
// struct Miimon

namespace Iface
{
namespace Xml
{
struct Miimon
{
	Miimon();

	PUnsignedInt::value_type getFreq() const
	{
		return m_freq;
	}
	void setFreq(PUnsignedInt::value_type value_)
	{
		m_freq = value_;
	}
	const boost::optional<PUnsignedInt::value_type >& getDowndelay() const
	{
		return m_downdelay;
	}
	void setDowndelay(const boost::optional<PUnsignedInt::value_type >& value_)
	{
		m_downdelay = value_;
	}
	const boost::optional<PUnsignedInt::value_type >& getUpdelay() const
	{
		return m_updelay;
	}
	void setUpdelay(const boost::optional<PUnsignedInt::value_type >& value_)
	{
		m_updelay = value_;
	}
	const boost::optional<ECarrier >& getCarrier() const
	{
		return m_carrier;
	}
	void setCarrier(const boost::optional<ECarrier >& value_)
	{
		m_carrier = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	PUnsignedInt::value_type m_freq;
	boost::optional<PUnsignedInt::value_type > m_downdelay;
	boost::optional<PUnsignedInt::value_type > m_updelay;
	boost::optional<ECarrier > m_carrier;
};

} // namespace Xml
} // namespace Iface

///////////////////////////////////////////////////////////////////////////////
// struct Arpmon

namespace Iface
{
namespace Xml
{
struct Arpmon
{
	Arpmon();

	PUnsignedInt::value_type getInterval() const
	{
		return m_interval;
	}
	void setInterval(PUnsignedInt::value_type value_)
	{
		m_interval = value_;
	}
	const PIpv4Addr::value_type& getTarget() const
	{
		return m_target;
	}
	void setTarget(const PIpv4Addr::value_type& value_)
	{
		m_target = value_;
	}
	const boost::optional<EValidate >& getValidate() const
	{
		return m_validate;
	}
	void setValidate(const boost::optional<EValidate >& value_)
	{
		m_validate = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	PUnsignedInt::value_type m_interval;
	PIpv4Addr::value_type m_target;
	boost::optional<EValidate > m_validate;
};

} // namespace Xml
} // namespace Iface

///////////////////////////////////////////////////////////////////////////////
// struct VChoice1233

namespace Iface
{
namespace Xml
{
typedef Choice<mpl::vector<Element<Iface::Xml::Miimon, Name::Strict<1220> >, Element<Iface::Xml::Arpmon, Name::Strict<1228> > > > VChoice1233Impl;
typedef VChoice1233Impl::value_type VChoice1233;

} // namespace Xml
} // namespace Iface

///////////////////////////////////////////////////////////////////////////////
// struct Bond

namespace Iface
{
namespace Xml
{
struct Bond
{
	const boost::optional<EMode1 >& getMode() const
	{
		return m_mode;
	}
	void setMode(const boost::optional<EMode1 >& value_)
	{
		m_mode = value_;
	}
	const boost::optional<VChoice1233 >& getChoice1233() const
	{
		return m_choice1233;
	}
	void setChoice1233(const boost::optional<VChoice1233 >& value_)
	{
		m_choice1233 = value_;
	}
	const QList<BasicEthernetContent>& getInterfaceList() const
	{
		return m_interfaceList;
	}
	void setInterfaceList(const QList<BasicEthernetContent>& value_)
	{
		m_interfaceList = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	boost::optional<EMode1 > m_mode;
	boost::optional<VChoice1233 > m_choice1233;
	QList<BasicEthernetContent> m_interfaceList;
};

} // namespace Xml
} // namespace Iface

///////////////////////////////////////////////////////////////////////////////
// struct Interface2

namespace Iface
{
namespace Xml
{
struct Interface2
{
	const BondInterfaceCommon& getBondInterfaceCommon() const
	{
		return m_bondInterfaceCommon;
	}
	void setBondInterfaceCommon(const BondInterfaceCommon& value_)
	{
		m_bondInterfaceCommon = value_;
	}
	const Bond& getBond() const
	{
		return m_bond;
	}
	void setBond(const Bond& value_)
	{
		m_bond = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	BondInterfaceCommon m_bondInterfaceCommon;
	Bond m_bond;
};

} // namespace Xml
} // namespace Iface

///////////////////////////////////////////////////////////////////////////////
// struct VChoice1208

namespace Iface
{
namespace Xml
{
typedef Choice<mpl::vector<Element<Iface::Xml::BasicEthernetContent, Name::Strict<603> >, Element<Iface::Xml::Interface1, Name::Strict<603> >, Element<Iface::Xml::Interface2, Name::Strict<603> > > > VChoice1208Impl;
typedef VChoice1208Impl::value_type VChoice1208;

} // namespace Xml
} // namespace Iface

///////////////////////////////////////////////////////////////////////////////
// struct Bridge

namespace Iface
{
namespace Xml
{
struct Bridge
{
	const boost::optional<EOnOrOff >& getStp() const
	{
		return m_stp;
	}
	void setStp(const boost::optional<EOnOrOff >& value_)
	{
		m_stp = value_;
	}
	const boost::optional<PTimeval::value_type >& getDelay() const
	{
		return m_delay;
	}
	void setDelay(const boost::optional<PTimeval::value_type >& value_)
	{
		m_delay = value_;
	}
	const QList<VChoice1208>& getChoice1208List() const
	{
		return m_choice1208List;
	}
	void setChoice1208List(const QList<VChoice1208>& value_)
	{
		m_choice1208List = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	boost::optional<EOnOrOff > m_stp;
	boost::optional<PTimeval::value_type > m_delay;
	QList<VChoice1208> m_choice1208List;
};

} // namespace Xml
} // namespace Iface

///////////////////////////////////////////////////////////////////////////////
// struct Interface3

namespace Iface
{
namespace Xml
{
struct Interface3
{
	Interface3();

	const PDeviceName::value_type& getName() const
	{
		return m_name;
	}
	void setName(const PDeviceName::value_type& value_)
	{
		m_name = value_;
	}
	EMode getStart() const
	{
		return m_start;
	}
	void setStart(EMode value_)
	{
		m_start = value_;
	}
	const boost::optional<PUnsignedInt::value_type >& getMtu() const
	{
		return m_mtu;
	}
	void setMtu(const boost::optional<PUnsignedInt::value_type >& value_)
	{
		m_mtu = value_;
	}
	const VInterfaceAddressing& getInterfaceAddressing() const
	{
		return m_interfaceAddressing;
	}
	void setInterfaceAddressing(const VInterfaceAddressing& value_)
	{
		m_interfaceAddressing = value_;
	}
	const Bridge& getBridge() const
	{
		return m_bridge;
	}
	void setBridge(const Bridge& value_)
	{
		m_bridge = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	PDeviceName::value_type m_name;
	EMode m_start;
	boost::optional<PUnsignedInt::value_type > m_mtu;
	VInterfaceAddressing m_interfaceAddressing;
	Bridge m_bridge;
};

} // namespace Xml
} // namespace Iface

///////////////////////////////////////////////////////////////////////////////
// struct Interface4

namespace Iface
{
namespace Xml
{
struct Interface4
{
	Interface4();

	const VlanInterfaceCommon& getVlanInterfaceCommon() const
	{
		return m_vlanInterfaceCommon;
	}
	void setVlanInterfaceCommon(const VlanInterfaceCommon& value_)
	{
		m_vlanInterfaceCommon = value_;
	}
	EMode getStart() const
	{
		return m_start;
	}
	void setStart(EMode value_)
	{
		m_start = value_;
	}
	const boost::optional<PUnsignedInt::value_type >& getMtu() const
	{
		return m_mtu;
	}
	void setMtu(const boost::optional<PUnsignedInt::value_type >& value_)
	{
		m_mtu = value_;
	}
	const VInterfaceAddressing& getInterfaceAddressing() const
	{
		return m_interfaceAddressing;
	}
	void setInterfaceAddressing(const VInterfaceAddressing& value_)
	{
		m_interfaceAddressing = value_;
	}
	const Vlan& getVlan() const
	{
		return m_vlan;
	}
	void setVlan(const Vlan& value_)
	{
		m_vlan = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	VlanInterfaceCommon m_vlanInterfaceCommon;
	EMode m_start;
	boost::optional<PUnsignedInt::value_type > m_mtu;
	VInterfaceAddressing m_interfaceAddressing;
	Vlan m_vlan;
};

} // namespace Xml
} // namespace Iface

///////////////////////////////////////////////////////////////////////////////
// struct Link traits

template<>
struct Traits<Iface::Xml::Link>
{
	typedef Ordered<mpl::vector<Optional<Attribute<Iface::Xml::PUnsignedInt, Name::Strict<120> > >, Optional<Attribute<Iface::Xml::EState, Name::Strict<121> > > > > marshal_type;

	static int parse(Iface::Xml::Link& , QStack<QDomElement>& );
	static int generate(const Iface::Xml::Link& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Dhcp traits

template<>
struct Traits<Iface::Xml::Dhcp>
{
	typedef Ordered<mpl::vector<Optional<Attribute<Iface::Xml::EYesOrNo, Name::Strict<1247> > > > > marshal_type;

	static int parse(Iface::Xml::Dhcp& , QStack<QDomElement>& );
	static int generate(const Iface::Xml::Dhcp& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Ip traits

template<>
struct Traits<Iface::Xml::Ip>
{
	typedef Ordered<mpl::vector<Attribute<Iface::Xml::PIpv4Addr, Name::Strict<106> >, Optional<Attribute<Iface::Xml::PIpv4Prefix, Name::Strict<1142> > > > > marshal_type;

	static int parse(Iface::Xml::Ip& , QStack<QDomElement>& );
	static int generate(const Iface::Xml::Ip& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Variant1243 traits

template<>
struct Traits<Iface::Xml::Variant1243>
{
	typedef Ordered<mpl::vector<Element<Iface::Xml::Ip, Name::Strict<637> >, Optional<Element<Attribute<Iface::Xml::PIpv4Addr, Name::Strict<1156> >, Name::Strict<1124> > > > > marshal_type;

	static int parse(Iface::Xml::Variant1243& , QStack<QDomElement>& );
	static int generate(const Iface::Xml::Variant1243& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Ip1 traits

template<>
struct Traits<Iface::Xml::Ip1>
{
	typedef Ordered<mpl::vector<Attribute<Iface::Xml::PIpv6Addr, Name::Strict<106> >, Optional<Attribute<Iface::Xml::PIpv6Prefix, Name::Strict<1142> > > > > marshal_type;

	static int parse(Iface::Xml::Ip1& , QStack<QDomElement>& );
	static int generate(const Iface::Xml::Ip1& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Protocol traits

template<>
struct Traits<Iface::Xml::Protocol>
{
	typedef Ordered<mpl::vector<Attribute<mpl::int_<1120>, Name::Strict<967> >, Unordered<mpl::vector<Optional<Element<Empty, Name::Strict<1246> > >, Optional<Element<Iface::Xml::Dhcp, Name::Strict<1145> > >, ZeroOrMore<Element<Iface::Xml::Ip1, Name::Strict<637> > >, Optional<Element<Attribute<Iface::Xml::PIpv6Addr, Name::Strict<1156> >, Name::Strict<1124> > > > > > > marshal_type;

	static int parse(Iface::Xml::Protocol& , QStack<QDomElement>& );
	static int generate(const Iface::Xml::Protocol& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct InterfaceAddressing1238 traits

template<>
struct Traits<Iface::Xml::InterfaceAddressing1238>
{
	typedef Ordered<mpl::vector<Optional<Element<Ordered<mpl::vector<Attribute<mpl::int_<1241>, Name::Strict<967> >, Iface::Xml::VChoice1244Impl > >, Name::Strict<187> > >, Optional<Element<Iface::Xml::Protocol, Name::Strict<187> > > > > marshal_type;

	static int parse(Iface::Xml::InterfaceAddressing1238& , QStack<QDomElement>& );
	static int generate(const Iface::Xml::InterfaceAddressing1238& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct InterfaceAddressing1239 traits

template<>
struct Traits<Iface::Xml::InterfaceAddressing1239>
{
	typedef Ordered<mpl::vector<Optional<Element<Iface::Xml::Protocol, Name::Strict<187> > >, Optional<Element<Ordered<mpl::vector<Attribute<mpl::int_<1241>, Name::Strict<967> >, Iface::Xml::VChoice1244Impl > >, Name::Strict<187> > > > > marshal_type;

	static int parse(Iface::Xml::InterfaceAddressing1239& , QStack<QDomElement>& );
	static int generate(const Iface::Xml::InterfaceAddressing1239& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Interface traits

template<>
struct Traits<Iface::Xml::Interface>
{
	typedef Unordered<mpl::vector<Element<Attribute<Iface::Xml::EMode, Name::Strict<345> >, Name::Strict<299> >, Attribute<mpl::int_<607>, Name::Strict<100> >, Attribute<Iface::Xml::PDeviceName, Name::Strict<102> >, Optional<Element<Attribute<Iface::Xml::PMacAddr, Name::Strict<106> >, Name::Strict<621> > >, Optional<Element<Iface::Xml::Link, Name::Strict<119> > >, Optional<Element<Attribute<Iface::Xml::PUnsignedInt, Name::Strict<309> >, Name::Strict<1197> > >, Iface::Xml::VInterfaceAddressingImpl > > marshal_type;

	static int parse(Iface::Xml::Interface& , QStack<QDomElement>& );
	static int generate(const Iface::Xml::Interface& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct BasicEthernetContent traits

template<>
struct Traits<Iface::Xml::BasicEthernetContent>
{
	typedef Unordered<mpl::vector<Attribute<mpl::int_<607>, Name::Strict<100> >, Attribute<Iface::Xml::PDeviceName, Name::Strict<102> >, Optional<Element<Attribute<Iface::Xml::PMacAddr, Name::Strict<106> >, Name::Strict<621> > >, Optional<Element<Iface::Xml::Link, Name::Strict<119> > > > > marshal_type;

	static int parse(Iface::Xml::BasicEthernetContent& , QStack<QDomElement>& );
	static int generate(const Iface::Xml::BasicEthernetContent& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct VlanInterfaceCommon traits

template<>
struct Traits<Iface::Xml::VlanInterfaceCommon>
{
	typedef Ordered<mpl::vector<Attribute<mpl::int_<189>, Name::Strict<100> >, Optional<Attribute<Iface::Xml::PDeviceName, Name::Strict<102> > >, Optional<Element<Iface::Xml::Link, Name::Strict<119> > > > > marshal_type;

	static int parse(Iface::Xml::VlanInterfaceCommon& , QStack<QDomElement>& );
	static int generate(const Iface::Xml::VlanInterfaceCommon& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Vlan traits

template<>
struct Traits<Iface::Xml::Vlan>
{
	typedef Ordered<mpl::vector<Attribute<Iface::Xml::PVlanId, Name::Strict<192> >, Element<Attribute<Iface::Xml::PDeviceName, Name::Strict<102> >, Name::Strict<603> > > > marshal_type;

	static int parse(Iface::Xml::Vlan& , QStack<QDomElement>& );
	static int generate(const Iface::Xml::Vlan& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Interface1 traits

template<>
struct Traits<Iface::Xml::Interface1>
{
	typedef Unordered<mpl::vector<Fragment<Iface::Xml::VlanInterfaceCommon >, Element<Iface::Xml::Vlan, Name::Strict<189> > > > marshal_type;

	static int parse(Iface::Xml::Interface1& , QStack<QDomElement>& );
	static int generate(const Iface::Xml::Interface1& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct BondInterfaceCommon traits

template<>
struct Traits<Iface::Xml::BondInterfaceCommon>
{
	typedef Ordered<mpl::vector<Attribute<mpl::int_<1211>, Name::Strict<100> >, Attribute<Iface::Xml::PDeviceName, Name::Strict<102> >, Optional<Element<Iface::Xml::Link, Name::Strict<119> > > > > marshal_type;

	static int parse(Iface::Xml::BondInterfaceCommon& , QStack<QDomElement>& );
	static int generate(const Iface::Xml::BondInterfaceCommon& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Miimon traits

template<>
struct Traits<Iface::Xml::Miimon>
{
	typedef Ordered<mpl::vector<Attribute<Iface::Xml::PUnsignedInt, Name::Strict<1221> >, Optional<Attribute<Iface::Xml::PUnsignedInt, Name::Strict<1222> > >, Optional<Attribute<Iface::Xml::PUnsignedInt, Name::Strict<1223> > >, Optional<Attribute<Iface::Xml::ECarrier, Name::Strict<1224> > > > > marshal_type;

	static int parse(Iface::Xml::Miimon& , QStack<QDomElement>& );
	static int generate(const Iface::Xml::Miimon& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Arpmon traits

template<>
struct Traits<Iface::Xml::Arpmon>
{
	typedef Ordered<mpl::vector<Attribute<Iface::Xml::PUnsignedInt, Name::Strict<1229> >, Attribute<Iface::Xml::PIpv4Addr, Name::Strict<300> >, Optional<Attribute<Iface::Xml::EValidate, Name::Strict<1230> > > > > marshal_type;

	static int parse(Iface::Xml::Arpmon& , QStack<QDomElement>& );
	static int generate(const Iface::Xml::Arpmon& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Bond traits

template<>
struct Traits<Iface::Xml::Bond>
{
	typedef Ordered<mpl::vector<Optional<Attribute<Iface::Xml::EMode1, Name::Strict<345> > >, Unordered<mpl::vector<Optional<Iface::Xml::VChoice1233Impl >, OneOrMore<Element<Iface::Xml::BasicEthernetContent, Name::Strict<603> > > > > > > marshal_type;

	static int parse(Iface::Xml::Bond& , QStack<QDomElement>& );
	static int generate(const Iface::Xml::Bond& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Interface2 traits

template<>
struct Traits<Iface::Xml::Interface2>
{
	typedef Unordered<mpl::vector<Fragment<Iface::Xml::BondInterfaceCommon >, Element<Iface::Xml::Bond, Name::Strict<1211> > > > marshal_type;

	static int parse(Iface::Xml::Interface2& , QStack<QDomElement>& );
	static int generate(const Iface::Xml::Interface2& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Bridge traits

template<>
struct Traits<Iface::Xml::Bridge>
{
	typedef Ordered<mpl::vector<Optional<Attribute<Iface::Xml::EOnOrOff, Name::Strict<1121> > >, Optional<Attribute<Iface::Xml::PTimeval, Name::Strict<392> > >, ZeroOrMore<Iface::Xml::VChoice1208Impl > > > marshal_type;

	static int parse(Iface::Xml::Bridge& , QStack<QDomElement>& );
	static int generate(const Iface::Xml::Bridge& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Interface3 traits

template<>
struct Traits<Iface::Xml::Interface3>
{
	typedef Unordered<mpl::vector<Attribute<mpl::int_<604>, Name::Strict<100> >, Attribute<Iface::Xml::PDeviceName, Name::Strict<102> >, Element<Attribute<Iface::Xml::EMode, Name::Strict<345> >, Name::Strict<299> >, Optional<Element<Attribute<Iface::Xml::PUnsignedInt, Name::Strict<309> >, Name::Strict<1197> > >, Iface::Xml::VInterfaceAddressingImpl, Element<Iface::Xml::Bridge, Name::Strict<604> > > > marshal_type;

	static int parse(Iface::Xml::Interface3& , QStack<QDomElement>& );
	static int generate(const Iface::Xml::Interface3& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Interface4 traits

template<>
struct Traits<Iface::Xml::Interface4>
{
	typedef Unordered<mpl::vector<Fragment<Iface::Xml::VlanInterfaceCommon >, Element<Attribute<Iface::Xml::EMode, Name::Strict<345> >, Name::Strict<299> >, Optional<Element<Attribute<Iface::Xml::PUnsignedInt, Name::Strict<309> >, Name::Strict<1197> > >, Iface::Xml::VInterfaceAddressingImpl, Element<Iface::Xml::Vlan, Name::Strict<189> > > > marshal_type;

	static int parse(Iface::Xml::Interface4& , QStack<QDomElement>& );
	static int generate(const Iface::Xml::Interface4& , QDomElement& );
};

} // namespace Libvirt

#endif // __IFACE_TYPE_H__
