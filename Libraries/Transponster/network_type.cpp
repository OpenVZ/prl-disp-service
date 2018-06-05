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

#include "network_type.h"

namespace Libvirt
{
///////////////////////////////////////////////////////////////////////////////
// struct Bridge

namespace Network
{
namespace Xml
{
bool Bridge::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Bridge, Name::Strict<656> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Bridge::save(QDomElement& dst_) const
{
	Element<Bridge, Name::Strict<656> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Bridge::save(QDomDocument& dst_) const
{
	Element<Bridge, Name::Strict<656> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Network

int Traits<Network::Xml::Bridge>::parse(Network::Xml::Bridge& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setName(m.get<0>().getValue());
		dst_.setStp(m.get<1>().getValue());
		dst_.setDelay(m.get<2>().getValue());
	}
	return output;
}

int Traits<Network::Xml::Bridge>::generate(const Network::Xml::Bridge& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getName(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getStp(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getDelay(), m.get<2>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Interface

namespace Network
{
namespace Xml
{
bool Interface::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Interface, Name::Strict<657> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Interface::save(QDomElement& dst_) const
{
	Element<Interface, Name::Strict<657> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Interface::save(QDomDocument& dst_) const
{
	Element<Interface, Name::Strict<657> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Network

int Traits<Network::Xml::Interface>::parse(Network::Xml::Interface& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setDev(m.get<0>().getValue());
		dst_.setConnections(m.get<1>().getValue());
	}
	return output;
}

int Traits<Network::Xml::Interface>::generate(const Network::Xml::Interface& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getDev(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getConnections(), m.get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Pciaddress

namespace Network
{
namespace Xml
{
bool Pciaddress::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	k.push(src_.firstChildElement());
	return 0 <= Traits<Pciaddress>::parse(*this, k);
}

bool Pciaddress::save(QDomElement& dst_) const
{
	return 0 <= Traits<Pciaddress>::generate(*this, dst_);
}


} // namespace Xml
} // namespace Network

int Traits<Network::Xml::Pciaddress>::parse(Network::Xml::Pciaddress& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setDomain(m.get<0>().getValue());
		dst_.setBus(m.get<1>().getValue());
		dst_.setSlot(m.get<2>().getValue());
		dst_.setFunction(m.get<3>().getValue());
		dst_.setMultifunction(m.get<4>().getValue());
	}
	return output;
}

int Traits<Network::Xml::Pciaddress>::generate(const Network::Xml::Pciaddress& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getDomain(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getBus(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getSlot(), m.get<2>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getFunction(), m.get<3>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getMultifunction(), m.get<4>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Address

namespace Network
{
namespace Xml
{
bool Address::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Address, Name::Strict<111> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Address::save(QDomElement& dst_) const
{
	Element<Address, Name::Strict<111> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Address::save(QDomDocument& dst_) const
{
	Element<Address, Name::Strict<111> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Network

int Traits<Network::Xml::Address>::parse(Network::Xml::Address& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setPciaddress(m.get<1>().getValue());
		dst_.setConnections(m.get<2>().getValue());
	}
	return output;
}

int Traits<Network::Xml::Address>::generate(const Network::Xml::Address& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getPciaddress(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getConnections(), m.get<2>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Address1

namespace Network
{
namespace Xml
{
bool Address1::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Address1, Name::Strict<111> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Address1::save(QDomElement& dst_) const
{
	Element<Address1, Name::Strict<111> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Address1::save(QDomDocument& dst_) const
{
	Element<Address1, Name::Strict<111> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Network

int Traits<Network::Xml::Address1>::parse(Network::Xml::Address1& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setStart(m.get<0>().getValue());
		dst_.setEnd(m.get<1>().getValue());
	}
	return output;
}

int Traits<Network::Xml::Address1>::generate(const Network::Xml::Address1& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getStart(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getEnd(), m.get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Port

namespace Network
{
namespace Xml
{
Port::Port(): m_start(), m_end()
{
}

bool Port::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Port, Name::Strict<212> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Port::save(QDomElement& dst_) const
{
	Element<Port, Name::Strict<212> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Port::save(QDomDocument& dst_) const
{
	Element<Port, Name::Strict<212> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Network

int Traits<Network::Xml::Port>::parse(Network::Xml::Port& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setStart(m.get<0>().getValue());
		dst_.setEnd(m.get<1>().getValue());
	}
	return output;
}

int Traits<Network::Xml::Port>::generate(const Network::Xml::Port& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getStart(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getEnd(), m.get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Nat

namespace Network
{
namespace Xml
{
bool Nat::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Nat, Name::Strict<1220> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Nat::save(QDomElement& dst_) const
{
	Element<Nat, Name::Strict<1220> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Nat::save(QDomDocument& dst_) const
{
	Element<Nat, Name::Strict<1220> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Network

int Traits<Network::Xml::Nat>::parse(Network::Xml::Nat& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setAddress(m.get<0>().getValue());
		dst_.setPort(m.get<1>().getValue());
	}
	return output;
}

int Traits<Network::Xml::Nat>::generate(const Network::Xml::Nat& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getAddress(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getPort(), m.get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Forward

namespace Network
{
namespace Xml
{
bool Forward::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Forward, Name::Strict<1219> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Forward::save(QDomElement& dst_) const
{
	Element<Forward, Name::Strict<1219> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Forward::save(QDomDocument& dst_) const
{
	Element<Forward, Name::Strict<1219> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Network

int Traits<Network::Xml::Forward>::parse(Network::Xml::Forward& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setDev(m.get<0>().getValue());
		dst_.setMode(m.get<1>().getValue());
		dst_.setManaged(m.get<2>().getValue());
		dst_.setChoice1224(m.get<3>().get<0>().getValue());
		dst_.setPf(m.get<3>().get<1>().getValue());
		dst_.setDriver(m.get<3>().get<2>().getValue());
		dst_.setNat(m.get<3>().get<3>().getValue());
	}
	return output;
}

int Traits<Network::Xml::Forward>::generate(const Network::Xml::Forward& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getDev(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getMode(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getManaged(), m.get<2>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getChoice1224(), m.get<3>().get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getPf(), m.get<3>().get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getDriver(), m.get<3>().get<2>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getNat(), m.get<3>().get<3>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Parameters

namespace Network
{
namespace Xml
{
bool Parameters::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Parameters, Name::Strict<180> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Parameters::save(QDomElement& dst_) const
{
	Element<Parameters, Name::Strict<180> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Parameters::save(QDomDocument& dst_) const
{
	Element<Parameters, Name::Strict<180> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Network

int Traits<Network::Xml::Parameters>::parse(Network::Xml::Parameters& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setManagerid(m.get<0>().getValue());
		dst_.setTypeid(m.get<1>().getValue());
		dst_.setTypeidversion(m.get<2>().getValue());
		dst_.setInstanceid(m.get<3>().getValue());
	}
	return output;
}

int Traits<Network::Xml::Parameters>::generate(const Network::Xml::Parameters& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getManagerid(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getTypeid(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getTypeidversion(), m.get<2>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getInstanceid(), m.get<3>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Parameters1

namespace Network
{
namespace Xml
{
bool Parameters1::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Parameters1, Name::Strict<180> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Parameters1::save(QDomElement& dst_) const
{
	Element<Parameters1, Name::Strict<180> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Parameters1::save(QDomDocument& dst_) const
{
	Element<Parameters1, Name::Strict<180> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Network

int Traits<Network::Xml::Parameters1>::parse(Network::Xml::Parameters1& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setProfileid(m.get<0>().getValue());
	}
	return output;
}

int Traits<Network::Xml::Parameters1>::generate(const Network::Xml::Parameters1& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getProfileid(), m.get<0>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Parameters2

namespace Network
{
namespace Xml
{
bool Parameters2::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Parameters2, Name::Strict<180> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Parameters2::save(QDomElement& dst_) const
{
	Element<Parameters2, Name::Strict<180> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Parameters2::save(QDomDocument& dst_) const
{
	Element<Parameters2, Name::Strict<180> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Network

int Traits<Network::Xml::Parameters2>::parse(Network::Xml::Parameters2& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setProfileid(m.get<0>().getValue());
		dst_.setInterfaceid(m.get<1>().getValue());
	}
	return output;
}

int Traits<Network::Xml::Parameters2>::generate(const Network::Xml::Parameters2& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getProfileid(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getInterfaceid(), m.get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Parameters3

namespace Network
{
namespace Xml
{
bool Parameters3::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Parameters3, Name::Strict<180> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Parameters3::save(QDomElement& dst_) const
{
	Element<Parameters3, Name::Strict<180> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Parameters3::save(QDomDocument& dst_) const
{
	Element<Parameters3, Name::Strict<180> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Network

int Traits<Network::Xml::Parameters3>::parse(Network::Xml::Parameters3& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setManagerid(m.get<0>().getValue());
		dst_.setTypeid(m.get<1>().getValue());
		dst_.setTypeidversion(m.get<2>().getValue());
		dst_.setInstanceid(m.get<3>().getValue());
		dst_.setProfileid(m.get<4>().getValue());
		dst_.setInterfaceid(m.get<5>().getValue());
	}
	return output;
}

int Traits<Network::Xml::Parameters3>::generate(const Network::Xml::Parameters3& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getManagerid(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getTypeid(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getTypeidversion(), m.get<2>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getInstanceid(), m.get<3>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getProfileid(), m.get<4>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getInterfaceid(), m.get<5>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Virtualport

namespace Network
{
namespace Xml
{
bool Virtualport::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Virtualport, Name::Strict<178> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Virtualport::save(QDomElement& dst_) const
{
	Element<Virtualport, Name::Strict<178> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Virtualport::save(QDomDocument& dst_) const
{
	Element<Virtualport, Name::Strict<178> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Network

int Traits<Network::Xml::Virtualport>::parse(Network::Xml::Virtualport& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setParameters(m.get<0>().getValue());
	}
	return output;
}

int Traits<Network::Xml::Virtualport>::generate(const Network::Xml::Virtualport& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getParameters(), m.get<0>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct BandwidthAttributes

namespace Network
{
namespace Xml
{
BandwidthAttributes::BandwidthAttributes(): m_average()
{
}

bool BandwidthAttributes::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<BandwidthAttributes, Name::Strict<196> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool BandwidthAttributes::save(QDomElement& dst_) const
{
	Element<BandwidthAttributes, Name::Strict<196> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool BandwidthAttributes::save(QDomDocument& dst_) const
{
	Element<BandwidthAttributes, Name::Strict<196> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Network

int Traits<Network::Xml::BandwidthAttributes>::parse(Network::Xml::BandwidthAttributes& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setAverage(m.get<0>().getValue());
		dst_.setPeak(m.get<1>().getValue());
		dst_.setFloor(m.get<2>().getValue());
		dst_.setBurst(m.get<3>().getValue());
	}
	return output;
}

int Traits<Network::Xml::BandwidthAttributes>::generate(const Network::Xml::BandwidthAttributes& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getAverage(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getPeak(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getFloor(), m.get<2>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getBurst(), m.get<3>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Bandwidth

namespace Network
{
namespace Xml
{
bool Bandwidth::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Bandwidth, Name::Strict<193> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Bandwidth::save(QDomElement& dst_) const
{
	Element<Bandwidth, Name::Strict<193> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Bandwidth::save(QDomDocument& dst_) const
{
	Element<Bandwidth, Name::Strict<193> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Network

int Traits<Network::Xml::Bandwidth>::parse(Network::Xml::Bandwidth& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setInbound(m.get<0>().getValue());
		dst_.setOutbound(m.get<1>().getValue());
	}
	return output;
}

int Traits<Network::Xml::Bandwidth>::generate(const Network::Xml::Bandwidth& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getInbound(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getOutbound(), m.get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Tag

namespace Network
{
namespace Xml
{
Tag::Tag(): m_id()
{
}

bool Tag::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Tag, Name::Strict<207> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Tag::save(QDomElement& dst_) const
{
	Element<Tag, Name::Strict<207> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Tag::save(QDomDocument& dst_) const
{
	Element<Tag, Name::Strict<207> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Network

int Traits<Network::Xml::Tag>::parse(Network::Xml::Tag& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setId(m.get<0>().getValue());
		dst_.setNativeMode(m.get<1>().getValue());
	}
	return output;
}

int Traits<Network::Xml::Tag>::generate(const Network::Xml::Tag& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getId(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getNativeMode(), m.get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Portgroup

namespace Network
{
namespace Xml
{
bool Portgroup::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Portgroup, Name::Strict<654> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Portgroup::save(QDomElement& dst_) const
{
	Element<Portgroup, Name::Strict<654> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Portgroup::save(QDomDocument& dst_) const
{
	Element<Portgroup, Name::Strict<654> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Network

int Traits<Network::Xml::Portgroup>::parse(Network::Xml::Portgroup& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setName(m.get<0>().getValue());
		dst_.setDefault(m.get<1>().getValue());
		dst_.setVirtualPortProfile(m.get<2>().get<0>().getValue());
		dst_.setBandwidth(m.get<2>().get<1>().getValue());
		dst_.setVlan(m.get<2>().get<2>().getValue());
	}
	return output;
}

int Traits<Network::Xml::Portgroup>::generate(const Network::Xml::Portgroup& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getName(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getDefault(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getVirtualPortProfile(), m.get<2>().get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getBandwidth(), m.get<2>().get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getVlan(), m.get<2>().get<2>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Txt

namespace Network
{
namespace Xml
{
bool Txt::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Txt, Name::Strict<1232> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Txt::save(QDomElement& dst_) const
{
	Element<Txt, Name::Strict<1232> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Txt::save(QDomDocument& dst_) const
{
	Element<Txt, Name::Strict<1232> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Network

int Traits<Network::Xml::Txt>::parse(Network::Xml::Txt& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setName(m.get<0>().getValue());
		dst_.setValue(m.get<1>().getValue());
	}
	return output;
}

int Traits<Network::Xml::Txt>::generate(const Network::Xml::Txt& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getName(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getValue(), m.get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Anonymous1252

namespace Network
{
namespace Xml
{
Anonymous1252::Anonymous1252(): m_port(), m_priority(), m_weight()
{
}

bool Anonymous1252::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	k.push(src_.firstChildElement());
	return 0 <= Traits<Anonymous1252>::parse(*this, k);
}

bool Anonymous1252::save(QDomElement& dst_) const
{
	return 0 <= Traits<Anonymous1252>::generate(*this, dst_);
}


} // namespace Xml
} // namespace Network

int Traits<Network::Xml::Anonymous1252>::parse(Network::Xml::Anonymous1252& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setDomain(m.get<0>().getValue());
		dst_.setTarget(m.get<1>().getValue());
		dst_.setPort(m.get<2>().getValue());
		dst_.setPriority(m.get<3>().getValue());
		dst_.setWeight(m.get<4>().getValue());
	}
	return output;
}

int Traits<Network::Xml::Anonymous1252>::generate(const Network::Xml::Anonymous1252& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getDomain(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getTarget(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getPort(), m.get<2>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getPriority(), m.get<3>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getWeight(), m.get<4>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Srv

namespace Network
{
namespace Xml
{
bool Srv::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Srv, Name::Strict<1233> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Srv::save(QDomElement& dst_) const
{
	Element<Srv, Name::Strict<1233> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Srv::save(QDomDocument& dst_) const
{
	Element<Srv, Name::Strict<1233> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Network

int Traits<Network::Xml::Srv>::parse(Network::Xml::Srv& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setService(m.get<0>().getValue());
		dst_.setProtocol(m.get<1>().getValue());
		dst_.setAnonymous1252(m.get<2>().getValue());
	}
	return output;
}

int Traits<Network::Xml::Srv>::generate(const Network::Xml::Srv& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getService(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getProtocol(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getAnonymous1252(), m.get<2>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Host

namespace Network
{
namespace Xml
{
bool Host::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Host, Name::Strict<513> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Host::save(QDomElement& dst_) const
{
	Element<Host, Name::Strict<513> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Host::save(QDomDocument& dst_) const
{
	Element<Host, Name::Strict<513> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Network

int Traits<Network::Xml::Host>::parse(Network::Xml::Host& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setIp(m.get<0>().getValue());
		dst_.setHostnameList(m.get<1>().getValue());
	}
	return output;
}

int Traits<Network::Xml::Host>::generate(const Network::Xml::Host& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getIp(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getHostnameList(), m.get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Dns

namespace Network
{
namespace Xml
{
bool Dns::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Dns, Name::Strict<1228> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Dns::save(QDomElement& dst_) const
{
	Element<Dns, Name::Strict<1228> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Dns::save(QDomDocument& dst_) const
{
	Element<Dns, Name::Strict<1228> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Network

int Traits<Network::Xml::Dns>::parse(Network::Xml::Dns& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setEnable(m.get<0>().getValue());
		dst_.setForwardPlainNames(m.get<1>().getValue());
		dst_.setForwarderList(m.get<2>().get<0>().getValue());
		dst_.setTxtList(m.get<2>().get<1>().getValue());
		dst_.setSrvList(m.get<2>().get<2>().getValue());
		dst_.setHostList(m.get<2>().get<3>().getValue());
	}
	return output;
}

int Traits<Network::Xml::Dns>::generate(const Network::Xml::Dns& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getEnable(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getForwardPlainNames(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getForwarderList(), m.get<2>().get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getTxtList(), m.get<2>().get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getSrvList(), m.get<2>().get<2>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getHostList(), m.get<2>().get<3>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Range

namespace Network
{
namespace Xml
{
bool Range::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Range, Name::Strict<1241> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Range::save(QDomElement& dst_) const
{
	Element<Range, Name::Strict<1241> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Range::save(QDomDocument& dst_) const
{
	Element<Range, Name::Strict<1241> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Network

int Traits<Network::Xml::Range>::parse(Network::Xml::Range& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setStart(m.get<0>().getValue());
		dst_.setEnd(m.get<1>().getValue());
	}
	return output;
}

int Traits<Network::Xml::Range>::generate(const Network::Xml::Range& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getStart(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getEnd(), m.get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Host1244

int Traits<Network::Xml::Host1244>::parse(Network::Xml::Host1244& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setChoice1243(m.get<0>().getValue());
		dst_.setName(m.get<1>().getValue());
	}
	return output;
}

int Traits<Network::Xml::Host1244>::generate(const Network::Xml::Host1244& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getChoice1243(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getName(), m.get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Host1

namespace Network
{
namespace Xml
{
bool Host1::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Host1, Name::Strict<513> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Host1::save(QDomElement& dst_) const
{
	Element<Host1, Name::Strict<513> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Host1::save(QDomDocument& dst_) const
{
	Element<Host1, Name::Strict<513> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Network

int Traits<Network::Xml::Host1>::parse(Network::Xml::Host1& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setHost(m.get<0>().getValue());
		dst_.setIp(m.get<1>().getValue());
	}
	return output;
}

int Traits<Network::Xml::Host1>::generate(const Network::Xml::Host1& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getHost(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getIp(), m.get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Bootp

namespace Network
{
namespace Xml
{
bool Bootp::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Bootp, Name::Strict<1247> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Bootp::save(QDomElement& dst_) const
{
	Element<Bootp, Name::Strict<1247> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Bootp::save(QDomDocument& dst_) const
{
	Element<Bootp, Name::Strict<1247> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Network

int Traits<Network::Xml::Bootp>::parse(Network::Xml::Bootp& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setFile(m.get<0>().getValue());
		dst_.setServer(m.get<1>().getValue());
	}
	return output;
}

int Traits<Network::Xml::Bootp>::generate(const Network::Xml::Bootp& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getFile(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getServer(), m.get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Dhcp

namespace Network
{
namespace Xml
{
bool Dhcp::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Dhcp, Name::Strict<1240> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Dhcp::save(QDomElement& dst_) const
{
	Element<Dhcp, Name::Strict<1240> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Dhcp::save(QDomDocument& dst_) const
{
	Element<Dhcp, Name::Strict<1240> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Network

int Traits<Network::Xml::Dhcp>::parse(Network::Xml::Dhcp& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setRangeList(m.get<0>().getValue());
		dst_.setHostList(m.get<1>().getValue());
		dst_.setBootp(m.get<2>().getValue());
	}
	return output;
}

int Traits<Network::Xml::Dhcp>::generate(const Network::Xml::Dhcp& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getRangeList(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getHostList(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getBootp(), m.get<2>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Ip

namespace Network
{
namespace Xml
{
bool Ip::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Ip, Name::Strict<689> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Ip::save(QDomElement& dst_) const
{
	Element<Ip, Name::Strict<689> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Ip::save(QDomDocument& dst_) const
{
	Element<Ip, Name::Strict<689> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Network

int Traits<Network::Xml::Ip>::parse(Network::Xml::Ip& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setAddress(m.get<0>().getValue());
		dst_.setChoice1238(m.get<1>().getValue());
		dst_.setFamily(m.get<2>().getValue());
		dst_.setTftp(m.get<3>().getValue());
		dst_.setDhcp(m.get<4>().getValue());
	}
	return output;
}

int Traits<Network::Xml::Ip>::generate(const Network::Xml::Ip& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getAddress(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getChoice1238(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getFamily(), m.get<2>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getTftp(), m.get<3>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getDhcp(), m.get<4>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Route

namespace Network
{
namespace Xml
{
bool Route::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Route, Name::Strict<1221> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Route::save(QDomElement& dst_) const
{
	Element<Route, Name::Strict<1221> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Route::save(QDomDocument& dst_) const
{
	Element<Route, Name::Strict<1221> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Network

int Traits<Network::Xml::Route>::parse(Network::Xml::Route& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setFamily(m.get<0>().getValue());
		dst_.setAddress(m.get<1>().getValue());
		dst_.setChoice1249(m.get<2>().getValue());
		dst_.setGateway(m.get<3>().getValue());
		dst_.setMetric(m.get<4>().getValue());
	}
	return output;
}

int Traits<Network::Xml::Route>::generate(const Network::Xml::Route& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getFamily(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getAddress(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getChoice1249(), m.get<2>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getGateway(), m.get<3>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getMetric(), m.get<4>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Network

namespace Network
{
namespace Xml
{
bool Network::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Network, Name::Strict<445> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Network::save(QDomElement& dst_) const
{
	Element<Network, Name::Strict<445> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Network::save(QDomDocument& dst_) const
{
	Element<Network, Name::Strict<445> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Network

int Traits<Network::Xml::Network>::parse(Network::Xml::Network& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setConnections(m.get<0>().getValue());
		dst_.setIpv6(m.get<1>().getValue());
		dst_.setName(m.get<2>().get<0>().getValue());
		dst_.setUuid(m.get<2>().get<1>().getValue());
		dst_.setBridge(m.get<2>().get<2>().getValue());
		dst_.setMac(m.get<2>().get<3>().getValue());
		dst_.setForward(m.get<2>().get<4>().getValue());
		dst_.setVirtualPortProfile(m.get<2>().get<5>().getValue());
		dst_.setPortgroupList(m.get<2>().get<6>().getValue());
		dst_.setDomain(m.get<2>().get<7>().getValue());
		dst_.setDns(m.get<2>().get<8>().getValue());
		dst_.setBandwidth(m.get<2>().get<9>().getValue());
		dst_.setVlan(m.get<2>().get<10>().getValue());
		dst_.setIpList(m.get<2>().get<11>().getValue());
		dst_.setRouteList(m.get<2>().get<12>().getValue());
	}
	return output;
}

int Traits<Network::Xml::Network>::generate(const Network::Xml::Network& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getConnections(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getIpv6(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getName(), m.get<2>().get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getUuid(), m.get<2>().get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getBridge(), m.get<2>().get<2>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getMac(), m.get<2>().get<3>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getForward(), m.get<2>().get<4>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getVirtualPortProfile(), m.get<2>().get<5>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getPortgroupList(), m.get<2>().get<6>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getDomain(), m.get<2>().get<7>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getDns(), m.get<2>().get<8>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getBandwidth(), m.get<2>().get<9>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getVlan(), m.get<2>().get<10>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getIpList(), m.get<2>().get<11>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getRouteList(), m.get<2>().get<12>()))
		return -1;

	return m.produce(dst_);
}

} // namespace Libvirt
