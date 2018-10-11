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

#include "nodedev_type.h"

namespace Libvirt
{
///////////////////////////////////////////////////////////////////////////////
// struct Parent3467

int Traits<Nodedev::Xml::Parent3467>::parse(Nodedev::Xml::Parent3467& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setWwnn(m.get<0>().getValue());
		dst_.setWwpn(m.get<1>().getValue());
	}
	return output;
}

int Traits<Nodedev::Xml::Parent3467>::generate(const Nodedev::Xml::Parent3467& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getWwnn(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getWwpn(), m.get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Hardware

namespace Nodedev
{
namespace Xml
{
bool Hardware::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Hardware, Name::Strict<3496> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Hardware::save(QDomElement& dst_) const
{
	Element<Hardware, Name::Strict<3496> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Hardware::save(QDomDocument& dst_) const
{
	Element<Hardware, Name::Strict<3496> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Nodedev

int Traits<Nodedev::Xml::Hardware>::parse(Nodedev::Xml::Hardware& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setVendor(m.get<0>().getValue());
		dst_.setVersion(m.get<1>().getValue());
		dst_.setSerial(m.get<2>().getValue());
		dst_.setUuid(m.get<3>().getValue());
	}
	return output;
}

int Traits<Nodedev::Xml::Hardware>::generate(const Nodedev::Xml::Hardware& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getVendor(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getVersion(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getSerial(), m.get<2>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getUuid(), m.get<3>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Firmware

namespace Nodedev
{
namespace Xml
{
bool Firmware::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Firmware, Name::Strict<3497> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Firmware::save(QDomElement& dst_) const
{
	Element<Firmware, Name::Strict<3497> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Firmware::save(QDomDocument& dst_) const
{
	Element<Firmware, Name::Strict<3497> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Nodedev

int Traits<Nodedev::Xml::Firmware>::parse(Nodedev::Xml::Firmware& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setVendor(m.get<0>().getValue());
		dst_.setVersion(m.get<1>().getValue());
		dst_.setReleaseDate(m.get<2>().getValue());
	}
	return output;
}

int Traits<Nodedev::Xml::Firmware>::generate(const Nodedev::Xml::Firmware& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getVendor(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getVersion(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getReleaseDate(), m.get<2>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Capsystem

int Traits<Nodedev::Xml::Capsystem>::parse(Nodedev::Xml::Capsystem& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setProduct(m.get<1>().getValue());
		dst_.setHardware(m.get<2>().getValue());
		dst_.setFirmware(m.get<3>().getValue());
	}
	return output;
}

int Traits<Nodedev::Xml::Capsystem>::generate(const Nodedev::Xml::Capsystem& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getProduct(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getHardware(), m.get<2>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getFirmware(), m.get<3>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Product

namespace Nodedev
{
namespace Xml
{
bool Product::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Product, Name::Strict<460> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Product::save(QDomElement& dst_) const
{
	Element<Product, Name::Strict<460> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Product::save(QDomDocument& dst_) const
{
	Element<Product, Name::Strict<460> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Nodedev

int Traits<Nodedev::Xml::Product>::parse(Nodedev::Xml::Product& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setId(m.get<0>().getValue());
		dst_.setOwnValue(m.get<1>().getValue());
	}
	return output;
}

int Traits<Nodedev::Xml::Product>::generate(const Nodedev::Xml::Product& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getId(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getOwnValue(), m.get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Vendor

namespace Nodedev
{
namespace Xml
{
bool Vendor::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Vendor, Name::Strict<459> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Vendor::save(QDomElement& dst_) const
{
	Element<Vendor, Name::Strict<459> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Vendor::save(QDomDocument& dst_) const
{
	Element<Vendor, Name::Strict<459> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Nodedev

int Traits<Nodedev::Xml::Vendor>::parse(Nodedev::Xml::Vendor& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setId(m.get<0>().getValue());
		dst_.setOwnValue(m.get<1>().getValue());
	}
	return output;
}

int Traits<Nodedev::Xml::Vendor>::generate(const Nodedev::Xml::Vendor& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getId(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getOwnValue(), m.get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Address

namespace Nodedev
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
} // namespace Nodedev

int Traits<Nodedev::Xml::Address>::parse(Nodedev::Xml::Address& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setDomain(m.get<0>().getValue());
		dst_.setBus(m.get<1>().getValue());
		dst_.setSlot(m.get<2>().getValue());
		dst_.setFunction(m.get<3>().getValue());
	}
	return output;
}

int Traits<Nodedev::Xml::Address>::generate(const Nodedev::Xml::Address& src_, QDomElement& dst_)
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

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Capability

namespace Nodedev
{
namespace Xml
{
bool Capability::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Capability, Name::Strict<3467> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Capability::save(QDomElement& dst_) const
{
	Element<Capability, Name::Strict<3467> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Capability::save(QDomDocument& dst_) const
{
	Element<Capability, Name::Strict<3467> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Nodedev

int Traits<Nodedev::Xml::Capability>::parse(Nodedev::Xml::Capability& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setMaxCount(m.get<1>().getValue());
		dst_.setAddressList(m.get<2>().getValue());
	}
	return output;
}

int Traits<Nodedev::Xml::Capability>::generate(const Nodedev::Xml::Capability& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getMaxCount(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getAddressList(), m.get<2>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Type

namespace Nodedev
{
namespace Xml
{
Type::Type(): m_availableInstances()
{
}

bool Type::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Type, Name::Strict<105> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Type::save(QDomElement& dst_) const
{
	Element<Type, Name::Strict<105> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Type::save(QDomDocument& dst_) const
{
	Element<Type, Name::Strict<105> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Nodedev

int Traits<Nodedev::Xml::Type>::parse(Nodedev::Xml::Type& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setId(m.get<0>().getValue());
		dst_.setName(m.get<1>().getValue());
		dst_.setAvailableInstances(m.get<3>().getValue());
	}
	return output;
}

int Traits<Nodedev::Xml::Type>::generate(const Nodedev::Xml::Type& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getId(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getName(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getAvailableInstances(), m.get<3>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct IommuGroup

namespace Nodedev
{
namespace Xml
{
IommuGroup::IommuGroup(): m_number()
{
}

bool IommuGroup::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<IommuGroup, Name::Strict<3509> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool IommuGroup::save(QDomElement& dst_) const
{
	Element<IommuGroup, Name::Strict<3509> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool IommuGroup::save(QDomDocument& dst_) const
{
	Element<IommuGroup, Name::Strict<3509> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Nodedev

int Traits<Nodedev::Xml::IommuGroup>::parse(Nodedev::Xml::IommuGroup& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setNumber(m.get<0>().getValue());
		dst_.setAddressList(m.get<1>().getValue());
	}
	return output;
}

int Traits<Nodedev::Xml::IommuGroup>::generate(const Nodedev::Xml::IommuGroup& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getNumber(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getAddressList(), m.get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Numa

namespace Nodedev
{
namespace Xml
{
bool Numa::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Numa, Name::Strict<1031> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Numa::save(QDomElement& dst_) const
{
	Element<Numa, Name::Strict<1031> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Numa::save(QDomDocument& dst_) const
{
	Element<Numa, Name::Strict<1031> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Nodedev

int Traits<Nodedev::Xml::Numa>::parse(Nodedev::Xml::Numa& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setNode(m.get<0>().getValue());
	}
	return output;
}

int Traits<Nodedev::Xml::Numa>::generate(const Nodedev::Xml::Numa& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getNode(), m.get<0>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Link

namespace Nodedev
{
namespace Xml
{
Link::Link(): m_validity(), m_width()
{
}

bool Link::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Link, Name::Strict<124> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Link::save(QDomElement& dst_) const
{
	Element<Link, Name::Strict<124> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Link::save(QDomDocument& dst_) const
{
	Element<Link, Name::Strict<124> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Nodedev

int Traits<Nodedev::Xml::Link>::parse(Nodedev::Xml::Link& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setValidity(m.get<0>().getValue());
		dst_.setPort(m.get<1>().getValue());
		dst_.setSpeed(m.get<2>().getValue());
		dst_.setWidth(m.get<3>().getValue());
	}
	return output;
}

int Traits<Nodedev::Xml::Link>::generate(const Nodedev::Xml::Link& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getValidity(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getPort(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getSpeed(), m.get<2>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getWidth(), m.get<3>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Cappcidev

namespace Nodedev
{
namespace Xml
{
Cappcidev::Cappcidev(): m_domain(), m_bus(), m_slot(), m_function()
{
}


} // namespace Xml
} // namespace Nodedev

int Traits<Nodedev::Xml::Cappcidev>::parse(Nodedev::Xml::Cappcidev& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setDomain(m.get<1>().getValue());
		dst_.setBus(m.get<2>().getValue());
		dst_.setSlot(m.get<3>().getValue());
		dst_.setFunction(m.get<4>().getValue());
		dst_.setProduct(m.get<5>().getValue());
		dst_.setVendor(m.get<6>().getValue());
		dst_.setCapability(m.get<7>().getValue());
		dst_.setCapability2(m.get<8>().getValue());
		dst_.setCapability3(m.get<9>().getValue());
		dst_.setCapability4(m.get<10>().getValue());
		dst_.setIommuGroup(m.get<11>().getValue());
		dst_.setNuma(m.get<12>().getValue());
		dst_.setPciExpress(m.get<13>().getValue());
	}
	return output;
}

int Traits<Nodedev::Xml::Cappcidev>::generate(const Nodedev::Xml::Cappcidev& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getDomain(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getBus(), m.get<2>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getSlot(), m.get<3>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getFunction(), m.get<4>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getProduct(), m.get<5>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getVendor(), m.get<6>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getCapability(), m.get<7>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getCapability2(), m.get<8>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getCapability3(), m.get<9>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getCapability4(), m.get<10>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getIommuGroup(), m.get<11>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getNuma(), m.get<12>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getPciExpress(), m.get<13>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Product1

namespace Nodedev
{
namespace Xml
{
bool Product1::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Product1, Name::Strict<460> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Product1::save(QDomElement& dst_) const
{
	Element<Product1, Name::Strict<460> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Product1::save(QDomDocument& dst_) const
{
	Element<Product1, Name::Strict<460> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Nodedev

int Traits<Nodedev::Xml::Product1>::parse(Nodedev::Xml::Product1& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setId(m.get<0>().getValue());
		dst_.setOwnValue(m.get<1>().getValue());
	}
	return output;
}

int Traits<Nodedev::Xml::Product1>::generate(const Nodedev::Xml::Product1& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getId(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getOwnValue(), m.get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Vendor1

namespace Nodedev
{
namespace Xml
{
bool Vendor1::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Vendor1, Name::Strict<459> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Vendor1::save(QDomElement& dst_) const
{
	Element<Vendor1, Name::Strict<459> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Vendor1::save(QDomDocument& dst_) const
{
	Element<Vendor1, Name::Strict<459> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Nodedev

int Traits<Nodedev::Xml::Vendor1>::parse(Nodedev::Xml::Vendor1& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setId(m.get<0>().getValue());
		dst_.setOwnValue(m.get<1>().getValue());
	}
	return output;
}

int Traits<Nodedev::Xml::Vendor1>::generate(const Nodedev::Xml::Vendor1& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getId(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getOwnValue(), m.get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Capusbdev

namespace Nodedev
{
namespace Xml
{
Capusbdev::Capusbdev(): m_bus(), m_device()
{
}


} // namespace Xml
} // namespace Nodedev

int Traits<Nodedev::Xml::Capusbdev>::parse(Nodedev::Xml::Capusbdev& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setBus(m.get<1>().getValue());
		dst_.setDevice(m.get<2>().getValue());
		dst_.setProduct(m.get<3>().getValue());
		dst_.setVendor(m.get<4>().getValue());
	}
	return output;
}

int Traits<Nodedev::Xml::Capusbdev>::generate(const Nodedev::Xml::Capusbdev& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getBus(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getDevice(), m.get<2>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getProduct(), m.get<3>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getVendor(), m.get<4>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Capusbinterface

namespace Nodedev
{
namespace Xml
{
Capusbinterface::Capusbinterface(): m_number(), m_class(), m_subclass(), m_protocol()
{
}


} // namespace Xml
} // namespace Nodedev

int Traits<Nodedev::Xml::Capusbinterface>::parse(Nodedev::Xml::Capusbinterface& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setNumber(m.get<1>().getValue());
		dst_.setClass(m.get<2>().getValue());
		dst_.setSubclass(m.get<3>().getValue());
		dst_.setProtocol(m.get<4>().getValue());
		dst_.setDescription(m.get<5>().getValue());
	}
	return output;
}

int Traits<Nodedev::Xml::Capusbinterface>::generate(const Nodedev::Xml::Capusbinterface& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getNumber(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getClass(), m.get<2>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getSubclass(), m.get<3>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getProtocol(), m.get<4>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getDescription(), m.get<5>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Link1

namespace Nodedev
{
namespace Xml
{
bool Link1::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Link1, Name::Strict<124> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Link1::save(QDomElement& dst_) const
{
	Element<Link1, Name::Strict<124> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Link1::save(QDomDocument& dst_) const
{
	Element<Link1, Name::Strict<124> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Nodedev

int Traits<Nodedev::Xml::Link1>::parse(Nodedev::Xml::Link1& dst_, QStack<QDomElement>& stack_)
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

int Traits<Nodedev::Xml::Link1>::generate(const Nodedev::Xml::Link1& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getSpeed(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getState(), m.get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Capability3550

int Traits<Nodedev::Xml::Capability3550>::parse(Nodedev::Xml::Capability3550& , QStack<QDomElement>& stack_)
{
	marshal_type m;
	return m.consume(stack_);
}

int Traits<Nodedev::Xml::Capability3550>::generate(const Nodedev::Xml::Capability3550& , QDomElement& dst_)
{
	marshal_type m;
	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Capability3551

int Traits<Nodedev::Xml::Capability3551>::parse(Nodedev::Xml::Capability3551& , QStack<QDomElement>& stack_)
{
	marshal_type m;
	return m.consume(stack_);
}

int Traits<Nodedev::Xml::Capability3551>::generate(const Nodedev::Xml::Capability3551& , QDomElement& dst_)
{
	marshal_type m;
	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Capnet

int Traits<Nodedev::Xml::Capnet>::parse(Nodedev::Xml::Capnet& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setInterface(m.get<1>().getValue());
		dst_.setAddress(m.get<2>().getValue());
		dst_.setLink(m.get<3>().getValue());
		dst_.setFeatureList(m.get<4>().getValue());
		dst_.setCapabilityList(m.get<5>().getValue());
	}
	return output;
}

int Traits<Nodedev::Xml::Capnet>::generate(const Nodedev::Xml::Capnet& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getInterface(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getAddress(), m.get<2>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getLink(), m.get<3>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getFeatureList(), m.get<4>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getCapabilityList(), m.get<5>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Capsfchost

int Traits<Nodedev::Xml::Capsfchost>::parse(Nodedev::Xml::Capsfchost& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setWwnn(m.get<1>().getValue());
		dst_.setWwpn(m.get<2>().getValue());
		dst_.setFabricWwn(m.get<3>().getValue());
	}
	return output;
}

int Traits<Nodedev::Xml::Capsfchost>::generate(const Nodedev::Xml::Capsfchost& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getWwnn(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getWwpn(), m.get<2>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getFabricWwn(), m.get<3>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Capsvports

namespace Nodedev
{
namespace Xml
{
Capsvports::Capsvports(): m_maxVports(), m_vports()
{
}


} // namespace Xml
} // namespace Nodedev

int Traits<Nodedev::Xml::Capsvports>::parse(Nodedev::Xml::Capsvports& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setMaxVports(m.get<1>().getValue());
		dst_.setVports(m.get<2>().getValue());
	}
	return output;
}

int Traits<Nodedev::Xml::Capsvports>::generate(const Nodedev::Xml::Capsvports& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getMaxVports(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getVports(), m.get<2>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Capscsihost

namespace Nodedev
{
namespace Xml
{
Capscsihost::Capscsihost(): m_host()
{
}


} // namespace Xml
} // namespace Nodedev

int Traits<Nodedev::Xml::Capscsihost>::parse(Nodedev::Xml::Capscsihost& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setHost(m.get<1>().getValue());
		dst_.setUniqueId(m.get<2>().getValue());
		dst_.setCapabilityList(m.get<3>().getValue());
	}
	return output;
}

int Traits<Nodedev::Xml::Capscsihost>::generate(const Nodedev::Xml::Capscsihost& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getHost(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getUniqueId(), m.get<2>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getCapabilityList(), m.get<3>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Capsfcrport

namespace Nodedev
{
namespace Xml
{
bool Capsfcrport::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Capsfcrport, Name::Strict<3467> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Capsfcrport::save(QDomElement& dst_) const
{
	Element<Capsfcrport, Name::Strict<3467> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Capsfcrport::save(QDomDocument& dst_) const
{
	Element<Capsfcrport, Name::Strict<3467> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Nodedev

int Traits<Nodedev::Xml::Capsfcrport>::parse(Nodedev::Xml::Capsfcrport& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setRport(m.get<1>().getValue());
		dst_.setWwpn(m.get<2>().getValue());
	}
	return output;
}

int Traits<Nodedev::Xml::Capsfcrport>::generate(const Nodedev::Xml::Capsfcrport& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getRport(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getWwpn(), m.get<2>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Capscsitarget

int Traits<Nodedev::Xml::Capscsitarget>::parse(Nodedev::Xml::Capscsitarget& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setTarget(m.get<1>().getValue());
		dst_.setCapability(m.get<2>().getValue());
	}
	return output;
}

int Traits<Nodedev::Xml::Capscsitarget>::generate(const Nodedev::Xml::Capscsitarget& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getTarget(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getCapability(), m.get<2>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Capscsi

namespace Nodedev
{
namespace Xml
{
Capscsi::Capscsi(): m_host(), m_bus(), m_target(), m_lun()
{
}


} // namespace Xml
} // namespace Nodedev

int Traits<Nodedev::Xml::Capscsi>::parse(Nodedev::Xml::Capscsi& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setHost(m.get<1>().getValue());
		dst_.setBus(m.get<2>().getValue());
		dst_.setTarget(m.get<3>().getValue());
		dst_.setLun(m.get<4>().getValue());
		dst_.setType(m.get<5>().getValue());
	}
	return output;
}

int Traits<Nodedev::Xml::Capscsi>::generate(const Nodedev::Xml::Capscsi& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getHost(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getBus(), m.get<2>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getTarget(), m.get<3>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getLun(), m.get<4>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getType(), m.get<5>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Capability1

namespace Nodedev
{
namespace Xml
{
Capability1::Capability1(): m_mediaAvailable(), m_mediaSize()
{
}

bool Capability1::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Capability1, Name::Strict<3467> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Capability1::save(QDomElement& dst_) const
{
	Element<Capability1, Name::Strict<3467> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Capability1::save(QDomDocument& dst_) const
{
	Element<Capability1, Name::Strict<3467> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Nodedev

int Traits<Nodedev::Xml::Capability1>::parse(Nodedev::Xml::Capability1& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setMediaAvailable(m.get<1>().getValue());
		dst_.setMediaSize(m.get<2>().getValue());
		dst_.setMediaLabel(m.get<3>().getValue());
	}
	return output;
}

int Traits<Nodedev::Xml::Capability1>::generate(const Nodedev::Xml::Capability1& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getMediaAvailable(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getMediaSize(), m.get<2>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getMediaLabel(), m.get<3>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Capstorage

int Traits<Nodedev::Xml::Capstorage>::parse(Nodedev::Xml::Capstorage& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setBlock(m.get<1>().getValue());
		dst_.setBus(m.get<2>().getValue());
		dst_.setDriveType(m.get<3>().getValue());
		dst_.setModel(m.get<4>().getValue());
		dst_.setVendor(m.get<5>().getValue());
		dst_.setSerial(m.get<6>().getValue());
		dst_.setCapstorage(m.get<7>().getValue());
	}
	return output;
}

int Traits<Nodedev::Xml::Capstorage>::generate(const Nodedev::Xml::Capstorage& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getBlock(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getBus(), m.get<2>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getDriveType(), m.get<3>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getModel(), m.get<4>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getVendor(), m.get<5>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getSerial(), m.get<6>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getCapstorage(), m.get<7>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Capmdev

namespace Nodedev
{
namespace Xml
{
Capmdev::Capmdev(): m_iommuGroup()
{
}


} // namespace Xml
} // namespace Nodedev

int Traits<Nodedev::Xml::Capmdev>::parse(Nodedev::Xml::Capmdev& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setType(m.get<1>().getValue());
		dst_.setIommuGroup(m.get<2>().getValue());
	}
	return output;
}

int Traits<Nodedev::Xml::Capmdev>::generate(const Nodedev::Xml::Capmdev& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getType(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getIommuGroup(), m.get<2>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Capccwdev

int Traits<Nodedev::Xml::Capccwdev>::parse(Nodedev::Xml::Capccwdev& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setCssid(m.get<1>().getValue());
		dst_.setSsid(m.get<2>().getValue());
		dst_.setDevno(m.get<3>().getValue());
	}
	return output;
}

int Traits<Nodedev::Xml::Capccwdev>::generate(const Nodedev::Xml::Capccwdev& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getCssid(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getSsid(), m.get<2>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getDevno(), m.get<3>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Device

namespace Nodedev
{
namespace Xml
{
bool Device::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Device, Name::Strict<354> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Device::save(QDomElement& dst_) const
{
	Element<Device, Name::Strict<354> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Device::save(QDomDocument& dst_) const
{
	Element<Device, Name::Strict<354> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Nodedev

int Traits<Nodedev::Xml::Device>::parse(Nodedev::Xml::Device& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setName(m.get<0>().getValue());
		dst_.setPath(m.get<1>().getValue());
		dst_.setDevnode(m.get<2>().getValue());
		dst_.setDevnodeList(m.get<3>().getValue());
		dst_.setParent(m.get<4>().getValue());
		dst_.setDriver(m.get<5>().getValue());
		dst_.setCapabilityList(m.get<6>().getValue());
	}
	return output;
}

int Traits<Nodedev::Xml::Device>::generate(const Nodedev::Xml::Device& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getName(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getPath(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getDevnode(), m.get<2>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getDevnodeList(), m.get<3>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getParent(), m.get<4>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getDriver(), m.get<5>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getCapabilityList(), m.get<6>()))
		return -1;

	return m.produce(dst_);
}

} // namespace Libvirt
