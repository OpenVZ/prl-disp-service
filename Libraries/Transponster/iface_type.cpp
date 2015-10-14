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

#include "iface_type.h"

namespace Libvirt
{
///////////////////////////////////////////////////////////////////////////////
// struct Link

namespace Iface
{
namespace Xml
{
bool Link::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Link, Name::Strict<118> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Link::save(QDomElement& dst_) const
{
	Element<Link, Name::Strict<118> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Link::save(QDomDocument& dst_) const
{
	Element<Link, Name::Strict<118> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Iface

int Traits<Iface::Xml::Link>::parse(Iface::Xml::Link& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setSpeed(m.get<0>().getValue());
		dst_.setState(m.get<1>().getValue());
	}
	return output;
}

int Traits<Iface::Xml::Link>::generate(const Iface::Xml::Link& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getSpeed(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getState(), m.get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Dhcp

namespace Iface
{
namespace Xml
{
bool Dhcp::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Dhcp, Name::Strict<1166> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Dhcp::save(QDomElement& dst_) const
{
	Element<Dhcp, Name::Strict<1166> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Dhcp::save(QDomDocument& dst_) const
{
	Element<Dhcp, Name::Strict<1166> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Iface

int Traits<Iface::Xml::Dhcp>::parse(Iface::Xml::Dhcp& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setPeerdns(m.get<0>().getValue());
	}
	return output;
}

int Traits<Iface::Xml::Dhcp>::generate(const Iface::Xml::Dhcp& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getPeerdns(), m.get<0>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Ip

namespace Iface
{
namespace Xml
{
bool Ip::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Ip, Name::Strict<647> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Ip::save(QDomElement& dst_) const
{
	Element<Ip, Name::Strict<647> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Ip::save(QDomDocument& dst_) const
{
	Element<Ip, Name::Strict<647> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Iface

int Traits<Iface::Xml::Ip>::parse(Iface::Xml::Ip& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setAddress(m.get<0>().getValue());
		dst_.setPrefix(m.get<1>().getValue());
	}
	return output;
}

int Traits<Iface::Xml::Ip>::generate(const Iface::Xml::Ip& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getAddress(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getPrefix(), m.get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Variant1263

int Traits<Iface::Xml::Variant1263>::parse(Iface::Xml::Variant1263& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setIp(m.get<0>().getValue());
		dst_.setRoute(m.get<1>().getValue());
	}
	return output;
}

int Traits<Iface::Xml::Variant1263>::generate(const Iface::Xml::Variant1263& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getIp(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getRoute(), m.get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Ip1

namespace Iface
{
namespace Xml
{
bool Ip1::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Ip1, Name::Strict<647> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Ip1::save(QDomElement& dst_) const
{
	Element<Ip1, Name::Strict<647> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Ip1::save(QDomDocument& dst_) const
{
	Element<Ip1, Name::Strict<647> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Iface

int Traits<Iface::Xml::Ip1>::parse(Iface::Xml::Ip1& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setAddress(m.get<0>().getValue());
		dst_.setPrefix(m.get<1>().getValue());
	}
	return output;
}

int Traits<Iface::Xml::Ip1>::generate(const Iface::Xml::Ip1& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getAddress(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getPrefix(), m.get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Protocol

namespace Iface
{
namespace Xml
{
Protocol::Protocol(): m_autoconf()
{
}

bool Protocol::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Protocol, Name::Strict<191> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Protocol::save(QDomElement& dst_) const
{
	Element<Protocol, Name::Strict<191> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Protocol::save(QDomDocument& dst_) const
{
	Element<Protocol, Name::Strict<191> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Iface

int Traits<Iface::Xml::Protocol>::parse(Iface::Xml::Protocol& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setAutoconf(m.get<1>().get<0>().getValue());
		dst_.setDhcp(m.get<1>().get<1>().getValue());
		dst_.setIpList(m.get<1>().get<2>().getValue());
		dst_.setRoute(m.get<1>().get<3>().getValue());
	}
	return output;
}

int Traits<Iface::Xml::Protocol>::generate(const Iface::Xml::Protocol& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getAutoconf(), m.get<1>().get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getDhcp(), m.get<1>().get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getIpList(), m.get<1>().get<2>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getRoute(), m.get<1>().get<3>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct InterfaceAddressing1258

int Traits<Iface::Xml::InterfaceAddressing1258>::parse(Iface::Xml::InterfaceAddressing1258& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setProtocol(m.get<0>().getValue());
		dst_.setProtocol2(m.get<1>().getValue());
	}
	return output;
}

int Traits<Iface::Xml::InterfaceAddressing1258>::generate(const Iface::Xml::InterfaceAddressing1258& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getProtocol(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getProtocol2(), m.get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct InterfaceAddressing1259

int Traits<Iface::Xml::InterfaceAddressing1259>::parse(Iface::Xml::InterfaceAddressing1259& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setProtocol(m.get<0>().getValue());
		dst_.setProtocol2(m.get<1>().getValue());
	}
	return output;
}

int Traits<Iface::Xml::InterfaceAddressing1259>::generate(const Iface::Xml::InterfaceAddressing1259& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getProtocol(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getProtocol2(), m.get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Interface

namespace Iface
{
namespace Xml
{
Interface::Interface(): m_start()
{
}

bool Interface::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Interface, Name::Strict<613> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Interface::save(QDomElement& dst_) const
{
	Element<Interface, Name::Strict<613> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Interface::save(QDomDocument& dst_) const
{
	Element<Interface, Name::Strict<613> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Iface

int Traits<Iface::Xml::Interface>::parse(Iface::Xml::Interface& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setStart(m.get<0>().getValue());
		dst_.setName(m.get<2>().getValue());
		dst_.setMac(m.get<3>().getValue());
		dst_.setLink(m.get<4>().getValue());
		dst_.setMtu(m.get<5>().getValue());
		dst_.setInterfaceAddressing(m.get<6>().getValue());
	}
	return output;
}

int Traits<Iface::Xml::Interface>::generate(const Iface::Xml::Interface& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getStart(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getName(), m.get<2>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getMac(), m.get<3>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getLink(), m.get<4>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getMtu(), m.get<5>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getInterfaceAddressing(), m.get<6>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct BasicEthernetContent

namespace Iface
{
namespace Xml
{
bool BasicEthernetContent::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<BasicEthernetContent, Name::Strict<613> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool BasicEthernetContent::save(QDomElement& dst_) const
{
	Element<BasicEthernetContent, Name::Strict<613> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool BasicEthernetContent::save(QDomDocument& dst_) const
{
	Element<BasicEthernetContent, Name::Strict<613> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Iface

int Traits<Iface::Xml::BasicEthernetContent>::parse(Iface::Xml::BasicEthernetContent& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setName(m.get<1>().getValue());
		dst_.setMac(m.get<2>().getValue());
		dst_.setLink(m.get<3>().getValue());
	}
	return output;
}

int Traits<Iface::Xml::BasicEthernetContent>::generate(const Iface::Xml::BasicEthernetContent& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getName(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getMac(), m.get<2>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getLink(), m.get<3>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct VlanInterfaceCommon

namespace Iface
{
namespace Xml
{
bool VlanInterfaceCommon::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	k.push(src_.firstChildElement());
	return 0 <= Traits<VlanInterfaceCommon>::parse(*this, k);
}

bool VlanInterfaceCommon::save(QDomElement& dst_) const
{
	return 0 <= Traits<VlanInterfaceCommon>::generate(*this, dst_);
}


} // namespace Xml
} // namespace Iface

int Traits<Iface::Xml::VlanInterfaceCommon>::parse(Iface::Xml::VlanInterfaceCommon& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setName(m.get<1>().getValue());
		dst_.setLink(m.get<2>().getValue());
	}
	return output;
}

int Traits<Iface::Xml::VlanInterfaceCommon>::generate(const Iface::Xml::VlanInterfaceCommon& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getName(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getLink(), m.get<2>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Vlan

namespace Iface
{
namespace Xml
{
Vlan::Vlan(): m_tag()
{
}

bool Vlan::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Vlan, Name::Strict<193> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Vlan::save(QDomElement& dst_) const
{
	Element<Vlan, Name::Strict<193> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Vlan::save(QDomDocument& dst_) const
{
	Element<Vlan, Name::Strict<193> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Iface

int Traits<Iface::Xml::Vlan>::parse(Iface::Xml::Vlan& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setTag(m.get<0>().getValue());
		dst_.setInterface(m.get<1>().getValue());
	}
	return output;
}

int Traits<Iface::Xml::Vlan>::generate(const Iface::Xml::Vlan& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getTag(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getInterface(), m.get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Interface1

namespace Iface
{
namespace Xml
{
bool Interface1::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Interface1, Name::Strict<613> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Interface1::save(QDomElement& dst_) const
{
	Element<Interface1, Name::Strict<613> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Interface1::save(QDomDocument& dst_) const
{
	Element<Interface1, Name::Strict<613> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Iface

int Traits<Iface::Xml::Interface1>::parse(Iface::Xml::Interface1& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setVlanInterfaceCommon(m.get<0>().getValue());
		dst_.setVlan(m.get<1>().getValue());
	}
	return output;
}

int Traits<Iface::Xml::Interface1>::generate(const Iface::Xml::Interface1& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getVlanInterfaceCommon(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getVlan(), m.get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct BondInterfaceCommon

namespace Iface
{
namespace Xml
{
bool BondInterfaceCommon::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	k.push(src_.firstChildElement());
	return 0 <= Traits<BondInterfaceCommon>::parse(*this, k);
}

bool BondInterfaceCommon::save(QDomElement& dst_) const
{
	return 0 <= Traits<BondInterfaceCommon>::generate(*this, dst_);
}


} // namespace Xml
} // namespace Iface

int Traits<Iface::Xml::BondInterfaceCommon>::parse(Iface::Xml::BondInterfaceCommon& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setName(m.get<1>().getValue());
		dst_.setLink(m.get<2>().getValue());
	}
	return output;
}

int Traits<Iface::Xml::BondInterfaceCommon>::generate(const Iface::Xml::BondInterfaceCommon& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getName(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getLink(), m.get<2>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Miimon

namespace Iface
{
namespace Xml
{
Miimon::Miimon(): m_freq()
{
}

bool Miimon::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Miimon, Name::Strict<1240> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Miimon::save(QDomElement& dst_) const
{
	Element<Miimon, Name::Strict<1240> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Miimon::save(QDomDocument& dst_) const
{
	Element<Miimon, Name::Strict<1240> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Iface

int Traits<Iface::Xml::Miimon>::parse(Iface::Xml::Miimon& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setFreq(m.get<0>().getValue());
		dst_.setDowndelay(m.get<1>().getValue());
		dst_.setUpdelay(m.get<2>().getValue());
		dst_.setCarrier(m.get<3>().getValue());
	}
	return output;
}

int Traits<Iface::Xml::Miimon>::generate(const Iface::Xml::Miimon& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getFreq(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getDowndelay(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getUpdelay(), m.get<2>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getCarrier(), m.get<3>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Arpmon

namespace Iface
{
namespace Xml
{
Arpmon::Arpmon(): m_interval()
{
}

bool Arpmon::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Arpmon, Name::Strict<1248> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Arpmon::save(QDomElement& dst_) const
{
	Element<Arpmon, Name::Strict<1248> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Arpmon::save(QDomDocument& dst_) const
{
	Element<Arpmon, Name::Strict<1248> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Iface

int Traits<Iface::Xml::Arpmon>::parse(Iface::Xml::Arpmon& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setInterval(m.get<0>().getValue());
		dst_.setTarget(m.get<1>().getValue());
		dst_.setValidate(m.get<2>().getValue());
	}
	return output;
}

int Traits<Iface::Xml::Arpmon>::generate(const Iface::Xml::Arpmon& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getInterval(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getTarget(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getValidate(), m.get<2>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Bond

namespace Iface
{
namespace Xml
{
bool Bond::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Bond, Name::Strict<1231> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Bond::save(QDomElement& dst_) const
{
	Element<Bond, Name::Strict<1231> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Bond::save(QDomDocument& dst_) const
{
	Element<Bond, Name::Strict<1231> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Iface

int Traits<Iface::Xml::Bond>::parse(Iface::Xml::Bond& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setMode(m.get<0>().getValue());
		dst_.setChoice1253(m.get<1>().get<0>().getValue());
		dst_.setInterfaceList(m.get<1>().get<1>().getValue());
	}
	return output;
}

int Traits<Iface::Xml::Bond>::generate(const Iface::Xml::Bond& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getMode(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getChoice1253(), m.get<1>().get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getInterfaceList(), m.get<1>().get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Interface2

namespace Iface
{
namespace Xml
{
bool Interface2::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Interface2, Name::Strict<613> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Interface2::save(QDomElement& dst_) const
{
	Element<Interface2, Name::Strict<613> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Interface2::save(QDomDocument& dst_) const
{
	Element<Interface2, Name::Strict<613> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Iface

int Traits<Iface::Xml::Interface2>::parse(Iface::Xml::Interface2& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setBondInterfaceCommon(m.get<0>().getValue());
		dst_.setBond(m.get<1>().getValue());
	}
	return output;
}

int Traits<Iface::Xml::Interface2>::generate(const Iface::Xml::Interface2& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getBondInterfaceCommon(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getBond(), m.get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Bridge

namespace Iface
{
namespace Xml
{
bool Bridge::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Bridge, Name::Strict<614> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Bridge::save(QDomElement& dst_) const
{
	Element<Bridge, Name::Strict<614> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Bridge::save(QDomDocument& dst_) const
{
	Element<Bridge, Name::Strict<614> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Iface

int Traits<Iface::Xml::Bridge>::parse(Iface::Xml::Bridge& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setStp(m.get<0>().getValue());
		dst_.setDelay(m.get<1>().getValue());
		dst_.setChoice1228List(m.get<2>().getValue());
	}
	return output;
}

int Traits<Iface::Xml::Bridge>::generate(const Iface::Xml::Bridge& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getStp(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getDelay(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getChoice1228List(), m.get<2>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Interface3

namespace Iface
{
namespace Xml
{
Interface3::Interface3(): m_start()
{
}

bool Interface3::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Interface3, Name::Strict<613> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Interface3::save(QDomElement& dst_) const
{
	Element<Interface3, Name::Strict<613> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Interface3::save(QDomDocument& dst_) const
{
	Element<Interface3, Name::Strict<613> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Iface

int Traits<Iface::Xml::Interface3>::parse(Iface::Xml::Interface3& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setName(m.get<1>().getValue());
		dst_.setStart(m.get<2>().getValue());
		dst_.setMtu(m.get<3>().getValue());
		dst_.setInterfaceAddressing(m.get<4>().getValue());
		dst_.setBridge(m.get<5>().getValue());
	}
	return output;
}

int Traits<Iface::Xml::Interface3>::generate(const Iface::Xml::Interface3& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getName(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getStart(), m.get<2>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getMtu(), m.get<3>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getInterfaceAddressing(), m.get<4>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getBridge(), m.get<5>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Interface4

namespace Iface
{
namespace Xml
{
Interface4::Interface4(): m_start()
{
}

bool Interface4::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Interface4, Name::Strict<613> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Interface4::save(QDomElement& dst_) const
{
	Element<Interface4, Name::Strict<613> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Interface4::save(QDomDocument& dst_) const
{
	Element<Interface4, Name::Strict<613> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Iface

int Traits<Iface::Xml::Interface4>::parse(Iface::Xml::Interface4& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setVlanInterfaceCommon(m.get<0>().getValue());
		dst_.setStart(m.get<1>().getValue());
		dst_.setMtu(m.get<2>().getValue());
		dst_.setInterfaceAddressing(m.get<3>().getValue());
		dst_.setVlan(m.get<4>().getValue());
	}
	return output;
}

int Traits<Iface::Xml::Interface4>::generate(const Iface::Xml::Interface4& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getVlanInterfaceCommon(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getStart(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getMtu(), m.get<2>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getInterfaceAddressing(), m.get<3>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getVlan(), m.get<4>()))
		return -1;

	return m.produce(dst_);
}

} // namespace Libvirt
