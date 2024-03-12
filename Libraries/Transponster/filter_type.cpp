/*
 * Copyright (c) 2015-2017, Parallels International GmbH
 * Copyright (c) 2017-2021 Virtuozzo International GmbH. All rights reserved.
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

#include "filter_type.h"

namespace Libvirt
{
///////////////////////////////////////////////////////////////////////////////
// struct FilterNodeAttributes

namespace Filter
{
namespace Xml
{
bool FilterNodeAttributes::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	k.push(src_.firstChildElement());
	return 0 <= Traits<FilterNodeAttributes>::parse(*this, k);
}

bool FilterNodeAttributes::save(QDomElement& dst_) const
{
	return 0 <= Traits<FilterNodeAttributes>::generate(*this, dst_);
}


} // namespace Xml
} // namespace Filter

int Traits<Filter::Xml::FilterNodeAttributes>::parse(Filter::Xml::FilterNodeAttributes& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setName(m.get<0>().getValue());
		dst_.setChain(m.get<1>().getValue());
		dst_.setPriority(m.get<2>().getValue());
	}
	return output;
}

int Traits<Filter::Xml::FilterNodeAttributes>::generate(const Filter::Xml::FilterNodeAttributes& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getName(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getChain(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getPriority(), m.get<2>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Parameter

namespace Filter
{
namespace Xml
{
bool Parameter::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Parameter, Name::Strict<1084> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Parameter::save(QDomElement& dst_) const
{
	Element<Parameter, Name::Strict<1084> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Parameter::save(QDomDocument& dst_) const
{
	Element<Parameter, Name::Strict<1084> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Filter

int Traits<Filter::Xml::Parameter>::parse(Filter::Xml::Parameter& dst_, QStack<QDomElement>& stack_)
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

int Traits<Filter::Xml::Parameter>::generate(const Filter::Xml::Parameter& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getName(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getValue(), m.get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct FilterrefNodeAttributes

namespace Filter
{
namespace Xml
{
bool FilterrefNodeAttributes::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<FilterrefNodeAttributes, Name::Strict<706> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool FilterrefNodeAttributes::save(QDomElement& dst_) const
{
	Element<FilterrefNodeAttributes, Name::Strict<706> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool FilterrefNodeAttributes::save(QDomDocument& dst_) const
{
	Element<FilterrefNodeAttributes, Name::Strict<706> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Filter

int Traits<Filter::Xml::FilterrefNodeAttributes>::parse(Filter::Xml::FilterrefNodeAttributes& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setFilter(m.get<0>().getValue());
		dst_.setParameterList(m.get<1>().getValue());
	}
	return output;
}

int Traits<Filter::Xml::FilterrefNodeAttributes>::generate(const Filter::Xml::FilterrefNodeAttributes& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getFilter(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getParameterList(), m.get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct RuleNodeAttributes

namespace Filter
{
namespace Xml
{
RuleNodeAttributes::RuleNodeAttributes(): m_action(), m_direction()
{
}

bool RuleNodeAttributes::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	k.push(src_.firstChildElement());
	return 0 <= Traits<RuleNodeAttributes>::parse(*this, k);
}

bool RuleNodeAttributes::save(QDomElement& dst_) const
{
	return 0 <= Traits<RuleNodeAttributes>::generate(*this, dst_);
}


} // namespace Xml
} // namespace Filter

int Traits<Filter::Xml::RuleNodeAttributes>::parse(Filter::Xml::RuleNodeAttributes& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setAction(m.get<0>().getValue());
		dst_.setDirection(m.get<1>().getValue());
		dst_.setPriority(m.get<2>().getValue());
		dst_.setStatematch(m.get<3>().getValue());
	}
	return output;
}

int Traits<Filter::Xml::RuleNodeAttributes>::generate(const Filter::Xml::RuleNodeAttributes& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getAction(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getDirection(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getPriority(), m.get<2>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getStatematch(), m.get<3>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct CommonL2Attributes

namespace Filter
{
namespace Xml
{
bool CommonL2Attributes::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	k.push(src_.firstChildElement());
	return 0 <= Traits<CommonL2Attributes>::parse(*this, k);
}

bool CommonL2Attributes::save(QDomElement& dst_) const
{
	return 0 <= Traits<CommonL2Attributes>::generate(*this, dst_);
}


} // namespace Xml
} // namespace Filter

int Traits<Filter::Xml::CommonL2Attributes>::parse(Filter::Xml::CommonL2Attributes& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setSrcmacaddr(m.get<0>().getValue());
		dst_.setSrcmacmask(m.get<1>().getValue());
		dst_.setDstmacaddr(m.get<2>().getValue());
		dst_.setDstmacmask(m.get<3>().getValue());
	}
	return output;
}

int Traits<Filter::Xml::CommonL2Attributes>::generate(const Filter::Xml::CommonL2Attributes& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getSrcmacaddr(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getSrcmacmask(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getDstmacaddr(), m.get<2>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getDstmacmask(), m.get<3>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Mac

namespace Filter
{
namespace Xml
{
bool Mac::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Mac, Name::Strict<673> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Mac::save(QDomElement& dst_) const
{
	Element<Mac, Name::Strict<673> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Mac::save(QDomDocument& dst_) const
{
	Element<Mac, Name::Strict<673> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Filter

int Traits<Filter::Xml::Mac>::parse(Filter::Xml::Mac& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setMatch(m.get<0>().getValue());
		dst_.setCommonL2Attributes(m.get<1>().getValue());
		dst_.setProtocolid(m.get<2>().getValue());
		dst_.setComment(m.get<3>().getValue());
	}
	return output;
}

int Traits<Filter::Xml::Mac>::generate(const Filter::Xml::Mac& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getMatch(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getCommonL2Attributes(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getProtocolid(), m.get<2>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getComment(), m.get<3>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct VlanAttributes

namespace Filter
{
namespace Xml
{
bool VlanAttributes::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	k.push(src_.firstChildElement());
	return 0 <= Traits<VlanAttributes>::parse(*this, k);
}

bool VlanAttributes::save(QDomElement& dst_) const
{
	return 0 <= Traits<VlanAttributes>::generate(*this, dst_);
}


} // namespace Xml
} // namespace Filter

int Traits<Filter::Xml::VlanAttributes>::parse(Filter::Xml::VlanAttributes& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setVlanid(m.get<0>().getValue());
		dst_.setEncapProtocol(m.get<1>().getValue());
	}
	return output;
}

int Traits<Filter::Xml::VlanAttributes>::generate(const Filter::Xml::VlanAttributes& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getVlanid(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getEncapProtocol(), m.get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Vlan

namespace Filter
{
namespace Xml
{
bool Vlan::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Vlan, Name::Strict<205> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Vlan::save(QDomElement& dst_) const
{
	Element<Vlan, Name::Strict<205> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Vlan::save(QDomDocument& dst_) const
{
	Element<Vlan, Name::Strict<205> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Filter

int Traits<Filter::Xml::Vlan>::parse(Filter::Xml::Vlan& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setMatch(m.get<0>().getValue());
		dst_.setCommonL2Attributes(m.get<1>().getValue());
		dst_.setVlanAttributes(m.get<2>().getValue());
		dst_.setComment(m.get<3>().getValue());
	}
	return output;
}

int Traits<Filter::Xml::Vlan>::generate(const Filter::Xml::Vlan& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getMatch(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getCommonL2Attributes(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getVlanAttributes(), m.get<2>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getComment(), m.get<3>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct SrcmacandmaskAttributes

namespace Filter
{
namespace Xml
{
bool SrcmacandmaskAttributes::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	k.push(src_.firstChildElement());
	return 0 <= Traits<SrcmacandmaskAttributes>::parse(*this, k);
}

bool SrcmacandmaskAttributes::save(QDomElement& dst_) const
{
	return 0 <= Traits<SrcmacandmaskAttributes>::generate(*this, dst_);
}


} // namespace Xml
} // namespace Filter

int Traits<Filter::Xml::SrcmacandmaskAttributes>::parse(Filter::Xml::SrcmacandmaskAttributes& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setSrcmacaddr(m.get<0>().getValue());
		dst_.setSrcmacmask(m.get<1>().getValue());
	}
	return output;
}

int Traits<Filter::Xml::SrcmacandmaskAttributes>::generate(const Filter::Xml::SrcmacandmaskAttributes& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getSrcmacaddr(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getSrcmacmask(), m.get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct StpAttributes

namespace Filter
{
namespace Xml
{
bool StpAttributes::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	k.push(src_.firstChildElement());
	return 0 <= Traits<StpAttributes>::parse(*this, k);
}

bool StpAttributes::save(QDomElement& dst_) const
{
	return 0 <= Traits<StpAttributes>::generate(*this, dst_);
}


} // namespace Xml
} // namespace Filter

int Traits<Filter::Xml::StpAttributes>::parse(Filter::Xml::StpAttributes& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setType(m.get<0>().getValue());
		dst_.setFlags(m.get<1>().getValue());
		dst_.setRootPriority(m.get<2>().getValue());
		dst_.setRootPriorityHi(m.get<3>().getValue());
		dst_.setRootAddress(m.get<4>().getValue());
		dst_.setRootAddressMask(m.get<5>().getValue());
		dst_.setRootCost(m.get<6>().getValue());
		dst_.setRootCostHi(m.get<7>().getValue());
		dst_.setSenderPriority(m.get<8>().getValue());
		dst_.setSenderPriorityHi(m.get<9>().getValue());
		dst_.setSenderAddress(m.get<10>().getValue());
		dst_.setSenderAddressMask(m.get<11>().getValue());
		dst_.setPort(m.get<12>().getValue());
		dst_.setPortHi(m.get<13>().getValue());
		dst_.setAge(m.get<14>().getValue());
		dst_.setAgeHi(m.get<15>().getValue());
		dst_.setMaxAge(m.get<16>().getValue());
		dst_.setMaxAgeHi(m.get<17>().getValue());
		dst_.setHelloTime(m.get<18>().getValue());
		dst_.setHelloTimeHi(m.get<19>().getValue());
		dst_.setForwardDelay(m.get<20>().getValue());
		dst_.setForwardDelayHi(m.get<21>().getValue());
	}
	return output;
}

int Traits<Filter::Xml::StpAttributes>::generate(const Filter::Xml::StpAttributes& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getType(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getFlags(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getRootPriority(), m.get<2>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getRootPriorityHi(), m.get<3>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getRootAddress(), m.get<4>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getRootAddressMask(), m.get<5>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getRootCost(), m.get<6>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getRootCostHi(), m.get<7>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getSenderPriority(), m.get<8>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getSenderPriorityHi(), m.get<9>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getSenderAddress(), m.get<10>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getSenderAddressMask(), m.get<11>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getPort(), m.get<12>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getPortHi(), m.get<13>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getAge(), m.get<14>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getAgeHi(), m.get<15>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getMaxAge(), m.get<16>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getMaxAgeHi(), m.get<17>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getHelloTime(), m.get<18>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getHelloTimeHi(), m.get<19>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getForwardDelay(), m.get<20>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getForwardDelayHi(), m.get<21>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Stp

namespace Filter
{
namespace Xml
{
bool Stp::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Stp, Name::Strict<1227> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Stp::save(QDomElement& dst_) const
{
	Element<Stp, Name::Strict<1227> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Stp::save(QDomDocument& dst_) const
{
	Element<Stp, Name::Strict<1227> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Filter

int Traits<Filter::Xml::Stp>::parse(Filter::Xml::Stp& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setMatch(m.get<0>().getValue());
		dst_.setSrcmacandmaskAttributes(m.get<1>().getValue());
		dst_.setStpAttributes(m.get<2>().getValue());
		dst_.setComment(m.get<3>().getValue());
	}
	return output;
}

int Traits<Filter::Xml::Stp>::generate(const Filter::Xml::Stp& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getMatch(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getSrcmacandmaskAttributes(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getStpAttributes(), m.get<2>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getComment(), m.get<3>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct ArpAttributes

namespace Filter
{
namespace Xml
{
bool ArpAttributes::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	k.push(src_.firstChildElement());
	return 0 <= Traits<ArpAttributes>::parse(*this, k);
}

bool ArpAttributes::save(QDomElement& dst_) const
{
	return 0 <= Traits<ArpAttributes>::generate(*this, dst_);
}


} // namespace Xml
} // namespace Filter

int Traits<Filter::Xml::ArpAttributes>::parse(Filter::Xml::ArpAttributes& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setArpsrcmacaddr(m.get<0>().getValue());
		dst_.setArpsrcipaddr(m.get<1>().getValue());
		dst_.setArpdstmacaddr(m.get<2>().getValue());
		dst_.setArpdstipaddr(m.get<3>().getValue());
		dst_.setHwtype(m.get<4>().getValue());
		dst_.setOpcode(m.get<5>().getValue());
		dst_.setProtocoltype(m.get<6>().getValue());
		dst_.setGratuitous(m.get<7>().getValue());
	}
	return output;
}

int Traits<Filter::Xml::ArpAttributes>::generate(const Filter::Xml::ArpAttributes& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getArpsrcmacaddr(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getArpsrcipaddr(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getArpdstmacaddr(), m.get<2>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getArpdstipaddr(), m.get<3>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getHwtype(), m.get<4>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getOpcode(), m.get<5>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getProtocoltype(), m.get<6>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getGratuitous(), m.get<7>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Arp

namespace Filter
{
namespace Xml
{
bool Arp::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Arp, Name::Strict<5261> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Arp::save(QDomElement& dst_) const
{
	Element<Arp, Name::Strict<5261> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Arp::save(QDomDocument& dst_) const
{
	Element<Arp, Name::Strict<5261> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Filter

int Traits<Filter::Xml::Arp>::parse(Filter::Xml::Arp& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setMatch(m.get<0>().getValue());
		dst_.setCommonL2Attributes(m.get<1>().getValue());
		dst_.setArpAttributes(m.get<2>().getValue());
		dst_.setComment(m.get<3>().getValue());
	}
	return output;
}

int Traits<Filter::Xml::Arp>::generate(const Filter::Xml::Arp& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getMatch(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getCommonL2Attributes(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getArpAttributes(), m.get<2>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getComment(), m.get<3>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Rarp

namespace Filter
{
namespace Xml
{
bool Rarp::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Rarp, Name::Strict<5263> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Rarp::save(QDomElement& dst_) const
{
	Element<Rarp, Name::Strict<5263> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Rarp::save(QDomDocument& dst_) const
{
	Element<Rarp, Name::Strict<5263> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Filter

int Traits<Filter::Xml::Rarp>::parse(Filter::Xml::Rarp& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setMatch(m.get<0>().getValue());
		dst_.setCommonL2Attributes(m.get<1>().getValue());
		dst_.setArpAttributes(m.get<2>().getValue());
		dst_.setComment(m.get<3>().getValue());
	}
	return output;
}

int Traits<Filter::Xml::Rarp>::generate(const Filter::Xml::Rarp& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getMatch(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getCommonL2Attributes(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getArpAttributes(), m.get<2>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getComment(), m.get<3>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct CommonIpAttributesP1

namespace Filter
{
namespace Xml
{
bool CommonIpAttributesP1::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	k.push(src_.firstChildElement());
	return 0 <= Traits<CommonIpAttributesP1>::parse(*this, k);
}

bool CommonIpAttributesP1::save(QDomElement& dst_) const
{
	return 0 <= Traits<CommonIpAttributesP1>::generate(*this, dst_);
}


} // namespace Xml
} // namespace Filter

int Traits<Filter::Xml::CommonIpAttributesP1>::parse(Filter::Xml::CommonIpAttributesP1& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setSrcipaddr(m.get<0>().getValue());
		dst_.setSrcipmask(m.get<1>().getValue());
		dst_.setDstipaddr(m.get<2>().getValue());
		dst_.setDstipmask(m.get<3>().getValue());
		dst_.setState(m.get<4>().getValue());
	}
	return output;
}

int Traits<Filter::Xml::CommonIpAttributesP1>::generate(const Filter::Xml::CommonIpAttributesP1& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getSrcipaddr(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getSrcipmask(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getDstipaddr(), m.get<2>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getDstipmask(), m.get<3>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getState(), m.get<4>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct CommonPortAttributes

namespace Filter
{
namespace Xml
{
bool CommonPortAttributes::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	k.push(src_.firstChildElement());
	return 0 <= Traits<CommonPortAttributes>::parse(*this, k);
}

bool CommonPortAttributes::save(QDomElement& dst_) const
{
	return 0 <= Traits<CommonPortAttributes>::generate(*this, dst_);
}


} // namespace Xml
} // namespace Filter

int Traits<Filter::Xml::CommonPortAttributes>::parse(Filter::Xml::CommonPortAttributes& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setSrcportstart(m.get<0>().getValue());
		dst_.setSrcportend(m.get<1>().getValue());
		dst_.setDstportstart(m.get<2>().getValue());
		dst_.setDstportend(m.get<3>().getValue());
	}
	return output;
}

int Traits<Filter::Xml::CommonPortAttributes>::generate(const Filter::Xml::CommonPortAttributes& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getSrcportstart(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getSrcportend(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getDstportstart(), m.get<2>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getDstportend(), m.get<3>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Ip

namespace Filter
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
} // namespace Filter

int Traits<Filter::Xml::Ip>::parse(Filter::Xml::Ip& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setMatch(m.get<0>().getValue());
		dst_.setCommonL2Attributes(m.get<1>().getValue());
		dst_.setCommonIpAttributesP1(m.get<2>().getValue());
		dst_.setCommonPortAttributes(m.get<3>().getValue());
		dst_.setProtocol(m.get<4>().getValue());
		dst_.setDscp(m.get<5>().getValue());
		dst_.setComment(m.get<6>().getValue());
	}
	return output;
}

int Traits<Filter::Xml::Ip>::generate(const Filter::Xml::Ip& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getMatch(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getCommonL2Attributes(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getCommonIpAttributesP1(), m.get<2>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getCommonPortAttributes(), m.get<3>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getProtocol(), m.get<4>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getDscp(), m.get<5>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getComment(), m.get<6>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct CommonIpv6AttributesP1

namespace Filter
{
namespace Xml
{
bool CommonIpv6AttributesP1::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	k.push(src_.firstChildElement());
	return 0 <= Traits<CommonIpv6AttributesP1>::parse(*this, k);
}

bool CommonIpv6AttributesP1::save(QDomElement& dst_) const
{
	return 0 <= Traits<CommonIpv6AttributesP1>::generate(*this, dst_);
}


} // namespace Xml
} // namespace Filter

int Traits<Filter::Xml::CommonIpv6AttributesP1>::parse(Filter::Xml::CommonIpv6AttributesP1& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setSrcipaddr(m.get<0>().getValue());
		dst_.setSrcipmask(m.get<1>().getValue());
		dst_.setDstipaddr(m.get<2>().getValue());
		dst_.setDstipmask(m.get<3>().getValue());
		dst_.setState(m.get<4>().getValue());
	}
	return output;
}

int Traits<Filter::Xml::CommonIpv6AttributesP1>::generate(const Filter::Xml::CommonIpv6AttributesP1& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getSrcipaddr(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getSrcipmask(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getDstipaddr(), m.get<2>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getDstipmask(), m.get<3>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getState(), m.get<4>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Ipv6

namespace Filter
{
namespace Xml
{
bool Ipv6::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Ipv6, Name::Strict<1226> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Ipv6::save(QDomElement& dst_) const
{
	Element<Ipv6, Name::Strict<1226> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Ipv6::save(QDomDocument& dst_) const
{
	Element<Ipv6, Name::Strict<1226> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Filter

int Traits<Filter::Xml::Ipv6>::parse(Filter::Xml::Ipv6& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setMatch(m.get<0>().getValue());
		dst_.setCommonL2Attributes(m.get<1>().getValue());
		dst_.setCommonIpv6AttributesP1(m.get<2>().getValue());
		dst_.setCommonPortAttributes(m.get<3>().getValue());
		dst_.setProtocol(m.get<4>().getValue());
		dst_.setComment(m.get<5>().getValue());
	}
	return output;
}

int Traits<Filter::Xml::Ipv6>::generate(const Filter::Xml::Ipv6& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getMatch(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getCommonL2Attributes(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getCommonIpv6AttributesP1(), m.get<2>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getCommonPortAttributes(), m.get<3>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getProtocol(), m.get<4>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getComment(), m.get<5>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Anonymous5679

namespace Filter
{
namespace Xml
{
bool Anonymous5679::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	k.push(src_.firstChildElement());
	return 0 <= Traits<Anonymous5679>::parse(*this, k);
}

bool Anonymous5679::save(QDomElement& dst_) const
{
	return 0 <= Traits<Anonymous5679>::generate(*this, dst_);
}


} // namespace Xml
} // namespace Filter

int Traits<Filter::Xml::Anonymous5679>::parse(Filter::Xml::Anonymous5679& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setIpset(m.get<0>().getValue());
		dst_.setIpsetflags(m.get<1>().getValue());
	}
	return output;
}

int Traits<Filter::Xml::Anonymous5679>::generate(const Filter::Xml::Anonymous5679& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getIpset(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getIpsetflags(), m.get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct CommonIpAttributesP2

namespace Filter
{
namespace Xml
{
bool CommonIpAttributesP2::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	k.push(src_.firstChildElement());
	return 0 <= Traits<CommonIpAttributesP2>::parse(*this, k);
}

bool CommonIpAttributesP2::save(QDomElement& dst_) const
{
	return 0 <= Traits<CommonIpAttributesP2>::generate(*this, dst_);
}


} // namespace Xml
} // namespace Filter

int Traits<Filter::Xml::CommonIpAttributesP2>::parse(Filter::Xml::CommonIpAttributesP2& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setSrcipfrom(m.get<0>().getValue());
		dst_.setSrcipto(m.get<1>().getValue());
		dst_.setDstipfrom(m.get<2>().getValue());
		dst_.setDstipto(m.get<3>().getValue());
		dst_.setDscp(m.get<4>().getValue());
		dst_.setConnlimitAbove(m.get<5>().getValue());
		dst_.setState(m.get<6>().getValue());
		dst_.setAnonymous5679(m.get<7>().getValue());
	}
	return output;
}

int Traits<Filter::Xml::CommonIpAttributesP2>::generate(const Filter::Xml::CommonIpAttributesP2& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getSrcipfrom(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getSrcipto(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getDstipfrom(), m.get<2>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getDstipto(), m.get<3>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getDscp(), m.get<4>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getConnlimitAbove(), m.get<5>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getState(), m.get<6>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getAnonymous5679(), m.get<7>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Tcp

namespace Filter
{
namespace Xml
{
bool Tcp::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Tcp, Name::Strict<515> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Tcp::save(QDomElement& dst_) const
{
	Element<Tcp, Name::Strict<515> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Tcp::save(QDomDocument& dst_) const
{
	Element<Tcp, Name::Strict<515> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Filter

int Traits<Filter::Xml::Tcp>::parse(Filter::Xml::Tcp& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setMatch(m.get<0>().getValue());
		dst_.setSrcmacaddr(m.get<1>().getValue());
		dst_.setCommonPortAttributes(m.get<2>().getValue());
		dst_.setCommonIpAttributesP1(m.get<3>().getValue());
		dst_.setCommonIpAttributesP2(m.get<4>().getValue());
		dst_.setFlags(m.get<5>().getValue());
		dst_.setComment(m.get<6>().getValue());
	}
	return output;
}

int Traits<Filter::Xml::Tcp>::generate(const Filter::Xml::Tcp& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getMatch(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getSrcmacaddr(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getCommonPortAttributes(), m.get<2>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getCommonIpAttributesP1(), m.get<3>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getCommonIpAttributesP2(), m.get<4>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getFlags(), m.get<5>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getComment(), m.get<6>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Udp

namespace Filter
{
namespace Xml
{
bool Udp::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Udp, Name::Strict<831> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Udp::save(QDomElement& dst_) const
{
	Element<Udp, Name::Strict<831> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Udp::save(QDomDocument& dst_) const
{
	Element<Udp, Name::Strict<831> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Filter

int Traits<Filter::Xml::Udp>::parse(Filter::Xml::Udp& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setMatch(m.get<0>().getValue());
		dst_.setSrcmacaddr(m.get<1>().getValue());
		dst_.setCommonPortAttributes(m.get<2>().getValue());
		dst_.setCommonIpAttributesP1(m.get<3>().getValue());
		dst_.setCommonIpAttributesP2(m.get<4>().getValue());
		dst_.setComment(m.get<5>().getValue());
	}
	return output;
}

int Traits<Filter::Xml::Udp>::generate(const Filter::Xml::Udp& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getMatch(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getSrcmacaddr(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getCommonPortAttributes(), m.get<2>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getCommonIpAttributesP1(), m.get<3>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getCommonIpAttributesP2(), m.get<4>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getComment(), m.get<5>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Sctp

namespace Filter
{
namespace Xml
{
bool Sctp::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Sctp, Name::Strict<5272> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Sctp::save(QDomElement& dst_) const
{
	Element<Sctp, Name::Strict<5272> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Sctp::save(QDomDocument& dst_) const
{
	Element<Sctp, Name::Strict<5272> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Filter

int Traits<Filter::Xml::Sctp>::parse(Filter::Xml::Sctp& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setMatch(m.get<0>().getValue());
		dst_.setSrcmacaddr(m.get<1>().getValue());
		dst_.setCommonPortAttributes(m.get<2>().getValue());
		dst_.setCommonIpAttributesP1(m.get<3>().getValue());
		dst_.setCommonIpAttributesP2(m.get<4>().getValue());
		dst_.setComment(m.get<5>().getValue());
	}
	return output;
}

int Traits<Filter::Xml::Sctp>::generate(const Filter::Xml::Sctp& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getMatch(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getSrcmacaddr(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getCommonPortAttributes(), m.get<2>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getCommonIpAttributesP1(), m.get<3>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getCommonIpAttributesP2(), m.get<4>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getComment(), m.get<5>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct IcmpAttributes

namespace Filter
{
namespace Xml
{
bool IcmpAttributes::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	k.push(src_.firstChildElement());
	return 0 <= Traits<IcmpAttributes>::parse(*this, k);
}

bool IcmpAttributes::save(QDomElement& dst_) const
{
	return 0 <= Traits<IcmpAttributes>::generate(*this, dst_);
}


} // namespace Xml
} // namespace Filter

int Traits<Filter::Xml::IcmpAttributes>::parse(Filter::Xml::IcmpAttributes& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setType(m.get<0>().getValue());
		dst_.setCode(m.get<1>().getValue());
	}
	return output;
}

int Traits<Filter::Xml::IcmpAttributes>::generate(const Filter::Xml::IcmpAttributes& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getType(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getCode(), m.get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Icmp

namespace Filter
{
namespace Xml
{
bool Icmp::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Icmp, Name::Strict<5273> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Icmp::save(QDomElement& dst_) const
{
	Element<Icmp, Name::Strict<5273> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Icmp::save(QDomDocument& dst_) const
{
	Element<Icmp, Name::Strict<5273> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Filter

int Traits<Filter::Xml::Icmp>::parse(Filter::Xml::Icmp& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setMatch(m.get<0>().getValue());
		dst_.setSrcmacaddr(m.get<1>().getValue());
		dst_.setCommonIpAttributesP1(m.get<2>().getValue());
		dst_.setCommonIpAttributesP2(m.get<3>().getValue());
		dst_.setIcmpAttributes(m.get<4>().getValue());
		dst_.setComment(m.get<5>().getValue());
	}
	return output;
}

int Traits<Filter::Xml::Icmp>::generate(const Filter::Xml::Icmp& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getMatch(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getSrcmacaddr(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getCommonIpAttributesP1(), m.get<2>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getCommonIpAttributesP2(), m.get<3>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getIcmpAttributes(), m.get<4>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getComment(), m.get<5>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Igmp

namespace Filter
{
namespace Xml
{
bool Igmp::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Igmp, Name::Strict<5275> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Igmp::save(QDomElement& dst_) const
{
	Element<Igmp, Name::Strict<5275> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Igmp::save(QDomDocument& dst_) const
{
	Element<Igmp, Name::Strict<5275> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Filter

int Traits<Filter::Xml::Igmp>::parse(Filter::Xml::Igmp& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setMatch(m.get<0>().getValue());
		dst_.setSrcmacaddr(m.get<1>().getValue());
		dst_.setCommonIpAttributesP1(m.get<2>().getValue());
		dst_.setCommonIpAttributesP2(m.get<3>().getValue());
		dst_.setComment(m.get<4>().getValue());
	}
	return output;
}

int Traits<Filter::Xml::Igmp>::generate(const Filter::Xml::Igmp& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getMatch(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getSrcmacaddr(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getCommonIpAttributesP1(), m.get<2>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getCommonIpAttributesP2(), m.get<3>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getComment(), m.get<4>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct All

namespace Filter
{
namespace Xml
{
bool All::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<All, Name::Strict<765> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool All::save(QDomElement& dst_) const
{
	Element<All, Name::Strict<765> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool All::save(QDomDocument& dst_) const
{
	Element<All, Name::Strict<765> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Filter

int Traits<Filter::Xml::All>::parse(Filter::Xml::All& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setMatch(m.get<0>().getValue());
		dst_.setSrcmacaddr(m.get<1>().getValue());
		dst_.setCommonIpAttributesP1(m.get<2>().getValue());
		dst_.setCommonIpAttributesP2(m.get<3>().getValue());
		dst_.setComment(m.get<4>().getValue());
	}
	return output;
}

int Traits<Filter::Xml::All>::generate(const Filter::Xml::All& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getMatch(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getSrcmacaddr(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getCommonIpAttributesP1(), m.get<2>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getCommonIpAttributesP2(), m.get<3>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getComment(), m.get<4>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Esp

namespace Filter
{
namespace Xml
{
bool Esp::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Esp, Name::Strict<5276> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Esp::save(QDomElement& dst_) const
{
	Element<Esp, Name::Strict<5276> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Esp::save(QDomDocument& dst_) const
{
	Element<Esp, Name::Strict<5276> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Filter

int Traits<Filter::Xml::Esp>::parse(Filter::Xml::Esp& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setMatch(m.get<0>().getValue());
		dst_.setSrcmacaddr(m.get<1>().getValue());
		dst_.setCommonIpAttributesP1(m.get<2>().getValue());
		dst_.setCommonIpAttributesP2(m.get<3>().getValue());
		dst_.setComment(m.get<4>().getValue());
	}
	return output;
}

int Traits<Filter::Xml::Esp>::generate(const Filter::Xml::Esp& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getMatch(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getSrcmacaddr(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getCommonIpAttributesP1(), m.get<2>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getCommonIpAttributesP2(), m.get<3>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getComment(), m.get<4>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Ah

namespace Filter
{
namespace Xml
{
bool Ah::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Ah, Name::Strict<5277> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Ah::save(QDomElement& dst_) const
{
	Element<Ah, Name::Strict<5277> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Ah::save(QDomDocument& dst_) const
{
	Element<Ah, Name::Strict<5277> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Filter

int Traits<Filter::Xml::Ah>::parse(Filter::Xml::Ah& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setMatch(m.get<0>().getValue());
		dst_.setSrcmacaddr(m.get<1>().getValue());
		dst_.setCommonIpAttributesP1(m.get<2>().getValue());
		dst_.setCommonIpAttributesP2(m.get<3>().getValue());
		dst_.setComment(m.get<4>().getValue());
	}
	return output;
}

int Traits<Filter::Xml::Ah>::generate(const Filter::Xml::Ah& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getMatch(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getSrcmacaddr(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getCommonIpAttributesP1(), m.get<2>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getCommonIpAttributesP2(), m.get<3>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getComment(), m.get<4>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Udplite

namespace Filter
{
namespace Xml
{
bool Udplite::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Udplite, Name::Strict<5278> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Udplite::save(QDomElement& dst_) const
{
	Element<Udplite, Name::Strict<5278> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Udplite::save(QDomDocument& dst_) const
{
	Element<Udplite, Name::Strict<5278> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Filter

int Traits<Filter::Xml::Udplite>::parse(Filter::Xml::Udplite& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setMatch(m.get<0>().getValue());
		dst_.setSrcmacaddr(m.get<1>().getValue());
		dst_.setCommonIpAttributesP1(m.get<2>().getValue());
		dst_.setCommonIpAttributesP2(m.get<3>().getValue());
		dst_.setComment(m.get<4>().getValue());
	}
	return output;
}

int Traits<Filter::Xml::Udplite>::generate(const Filter::Xml::Udplite& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getMatch(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getSrcmacaddr(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getCommonIpAttributesP1(), m.get<2>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getCommonIpAttributesP2(), m.get<3>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getComment(), m.get<4>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct CommonIpv6AttributesP2

namespace Filter
{
namespace Xml
{
bool CommonIpv6AttributesP2::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	k.push(src_.firstChildElement());
	return 0 <= Traits<CommonIpv6AttributesP2>::parse(*this, k);
}

bool CommonIpv6AttributesP2::save(QDomElement& dst_) const
{
	return 0 <= Traits<CommonIpv6AttributesP2>::generate(*this, dst_);
}


} // namespace Xml
} // namespace Filter

int Traits<Filter::Xml::CommonIpv6AttributesP2>::parse(Filter::Xml::CommonIpv6AttributesP2& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setSrcipfrom(m.get<0>().getValue());
		dst_.setSrcipto(m.get<1>().getValue());
		dst_.setDstipfrom(m.get<2>().getValue());
		dst_.setDstipto(m.get<3>().getValue());
		dst_.setDscp(m.get<4>().getValue());
	}
	return output;
}

int Traits<Filter::Xml::CommonIpv6AttributesP2>::generate(const Filter::Xml::CommonIpv6AttributesP2& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getSrcipfrom(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getSrcipto(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getDstipfrom(), m.get<2>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getDstipto(), m.get<3>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getDscp(), m.get<4>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct TcpIpv6

namespace Filter
{
namespace Xml
{
bool TcpIpv6::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<TcpIpv6, Name::Strict<5279> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool TcpIpv6::save(QDomElement& dst_) const
{
	Element<TcpIpv6, Name::Strict<5279> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool TcpIpv6::save(QDomDocument& dst_) const
{
	Element<TcpIpv6, Name::Strict<5279> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Filter

int Traits<Filter::Xml::TcpIpv6>::parse(Filter::Xml::TcpIpv6& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setMatch(m.get<0>().getValue());
		dst_.setSrcmacaddr(m.get<1>().getValue());
		dst_.setCommonPortAttributes(m.get<2>().getValue());
		dst_.setCommonIpv6AttributesP1(m.get<3>().getValue());
		dst_.setCommonIpv6AttributesP2(m.get<4>().getValue());
		dst_.setFlags(m.get<5>().getValue());
		dst_.setComment(m.get<6>().getValue());
	}
	return output;
}

int Traits<Filter::Xml::TcpIpv6>::generate(const Filter::Xml::TcpIpv6& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getMatch(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getSrcmacaddr(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getCommonPortAttributes(), m.get<2>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getCommonIpv6AttributesP1(), m.get<3>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getCommonIpv6AttributesP2(), m.get<4>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getFlags(), m.get<5>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getComment(), m.get<6>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct UdpIpv6

namespace Filter
{
namespace Xml
{
bool UdpIpv6::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<UdpIpv6, Name::Strict<5281> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool UdpIpv6::save(QDomElement& dst_) const
{
	Element<UdpIpv6, Name::Strict<5281> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool UdpIpv6::save(QDomDocument& dst_) const
{
	Element<UdpIpv6, Name::Strict<5281> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Filter

int Traits<Filter::Xml::UdpIpv6>::parse(Filter::Xml::UdpIpv6& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setMatch(m.get<0>().getValue());
		dst_.setSrcmacaddr(m.get<1>().getValue());
		dst_.setCommonPortAttributes(m.get<2>().getValue());
		dst_.setCommonIpv6AttributesP1(m.get<3>().getValue());
		dst_.setCommonIpv6AttributesP2(m.get<4>().getValue());
		dst_.setComment(m.get<5>().getValue());
	}
	return output;
}

int Traits<Filter::Xml::UdpIpv6>::generate(const Filter::Xml::UdpIpv6& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getMatch(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getSrcmacaddr(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getCommonPortAttributes(), m.get<2>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getCommonIpv6AttributesP1(), m.get<3>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getCommonIpv6AttributesP2(), m.get<4>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getComment(), m.get<5>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct SctpIpv6

namespace Filter
{
namespace Xml
{
bool SctpIpv6::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<SctpIpv6, Name::Strict<5282> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool SctpIpv6::save(QDomElement& dst_) const
{
	Element<SctpIpv6, Name::Strict<5282> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool SctpIpv6::save(QDomDocument& dst_) const
{
	Element<SctpIpv6, Name::Strict<5282> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Filter

int Traits<Filter::Xml::SctpIpv6>::parse(Filter::Xml::SctpIpv6& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setMatch(m.get<0>().getValue());
		dst_.setSrcmacaddr(m.get<1>().getValue());
		dst_.setCommonPortAttributes(m.get<2>().getValue());
		dst_.setCommonIpv6AttributesP1(m.get<3>().getValue());
		dst_.setCommonIpv6AttributesP2(m.get<4>().getValue());
		dst_.setComment(m.get<5>().getValue());
	}
	return output;
}

int Traits<Filter::Xml::SctpIpv6>::generate(const Filter::Xml::SctpIpv6& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getMatch(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getSrcmacaddr(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getCommonPortAttributes(), m.get<2>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getCommonIpv6AttributesP1(), m.get<3>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getCommonIpv6AttributesP2(), m.get<4>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getComment(), m.get<5>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Icmpv6

namespace Filter
{
namespace Xml
{
bool Icmpv6::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Icmpv6, Name::Strict<5283> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Icmpv6::save(QDomElement& dst_) const
{
	Element<Icmpv6, Name::Strict<5283> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Icmpv6::save(QDomDocument& dst_) const
{
	Element<Icmpv6, Name::Strict<5283> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Filter

int Traits<Filter::Xml::Icmpv6>::parse(Filter::Xml::Icmpv6& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setMatch(m.get<0>().getValue());
		dst_.setSrcmacaddr(m.get<1>().getValue());
		dst_.setCommonIpv6AttributesP1(m.get<2>().getValue());
		dst_.setCommonIpv6AttributesP2(m.get<3>().getValue());
		dst_.setIcmpAttributes(m.get<4>().getValue());
		dst_.setComment(m.get<5>().getValue());
	}
	return output;
}

int Traits<Filter::Xml::Icmpv6>::generate(const Filter::Xml::Icmpv6& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getMatch(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getSrcmacaddr(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getCommonIpv6AttributesP1(), m.get<2>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getCommonIpv6AttributesP2(), m.get<3>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getIcmpAttributes(), m.get<4>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getComment(), m.get<5>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct AllIpv6

namespace Filter
{
namespace Xml
{
bool AllIpv6::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<AllIpv6, Name::Strict<5284> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool AllIpv6::save(QDomElement& dst_) const
{
	Element<AllIpv6, Name::Strict<5284> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool AllIpv6::save(QDomDocument& dst_) const
{
	Element<AllIpv6, Name::Strict<5284> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Filter

int Traits<Filter::Xml::AllIpv6>::parse(Filter::Xml::AllIpv6& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setMatch(m.get<0>().getValue());
		dst_.setSrcmacaddr(m.get<1>().getValue());
		dst_.setCommonIpv6AttributesP1(m.get<2>().getValue());
		dst_.setCommonIpv6AttributesP2(m.get<3>().getValue());
		dst_.setComment(m.get<4>().getValue());
	}
	return output;
}

int Traits<Filter::Xml::AllIpv6>::generate(const Filter::Xml::AllIpv6& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getMatch(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getSrcmacaddr(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getCommonIpv6AttributesP1(), m.get<2>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getCommonIpv6AttributesP2(), m.get<3>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getComment(), m.get<4>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct EspIpv6

namespace Filter
{
namespace Xml
{
bool EspIpv6::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<EspIpv6, Name::Strict<5285> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool EspIpv6::save(QDomElement& dst_) const
{
	Element<EspIpv6, Name::Strict<5285> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool EspIpv6::save(QDomDocument& dst_) const
{
	Element<EspIpv6, Name::Strict<5285> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Filter

int Traits<Filter::Xml::EspIpv6>::parse(Filter::Xml::EspIpv6& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setMatch(m.get<0>().getValue());
		dst_.setSrcmacaddr(m.get<1>().getValue());
		dst_.setCommonIpv6AttributesP1(m.get<2>().getValue());
		dst_.setCommonIpv6AttributesP2(m.get<3>().getValue());
		dst_.setComment(m.get<4>().getValue());
	}
	return output;
}

int Traits<Filter::Xml::EspIpv6>::generate(const Filter::Xml::EspIpv6& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getMatch(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getSrcmacaddr(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getCommonIpv6AttributesP1(), m.get<2>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getCommonIpv6AttributesP2(), m.get<3>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getComment(), m.get<4>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct AhIpv6

namespace Filter
{
namespace Xml
{
bool AhIpv6::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<AhIpv6, Name::Strict<5286> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool AhIpv6::save(QDomElement& dst_) const
{
	Element<AhIpv6, Name::Strict<5286> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool AhIpv6::save(QDomDocument& dst_) const
{
	Element<AhIpv6, Name::Strict<5286> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Filter

int Traits<Filter::Xml::AhIpv6>::parse(Filter::Xml::AhIpv6& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setMatch(m.get<0>().getValue());
		dst_.setSrcmacaddr(m.get<1>().getValue());
		dst_.setCommonIpv6AttributesP1(m.get<2>().getValue());
		dst_.setCommonIpv6AttributesP2(m.get<3>().getValue());
		dst_.setComment(m.get<4>().getValue());
	}
	return output;
}

int Traits<Filter::Xml::AhIpv6>::generate(const Filter::Xml::AhIpv6& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getMatch(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getSrcmacaddr(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getCommonIpv6AttributesP1(), m.get<2>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getCommonIpv6AttributesP2(), m.get<3>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getComment(), m.get<4>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct UdpliteIpv6

namespace Filter
{
namespace Xml
{
bool UdpliteIpv6::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<UdpliteIpv6, Name::Strict<5287> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool UdpliteIpv6::save(QDomElement& dst_) const
{
	Element<UdpliteIpv6, Name::Strict<5287> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool UdpliteIpv6::save(QDomDocument& dst_) const
{
	Element<UdpliteIpv6, Name::Strict<5287> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Filter

int Traits<Filter::Xml::UdpliteIpv6>::parse(Filter::Xml::UdpliteIpv6& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setMatch(m.get<0>().getValue());
		dst_.setSrcmacaddr(m.get<1>().getValue());
		dst_.setCommonIpv6AttributesP1(m.get<2>().getValue());
		dst_.setCommonIpv6AttributesP2(m.get<3>().getValue());
		dst_.setComment(m.get<4>().getValue());
	}
	return output;
}

int Traits<Filter::Xml::UdpliteIpv6>::generate(const Filter::Xml::UdpliteIpv6& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getMatch(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getSrcmacaddr(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getCommonIpv6AttributesP1(), m.get<2>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getCommonIpv6AttributesP2(), m.get<3>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getComment(), m.get<4>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Rule

namespace Filter
{
namespace Xml
{
bool Rule::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Rule, Name::Strict<5252> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Rule::save(QDomElement& dst_) const
{
	Element<Rule, Name::Strict<5252> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Rule::save(QDomDocument& dst_) const
{
	Element<Rule, Name::Strict<5252> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Filter

int Traits<Filter::Xml::Rule>::parse(Filter::Xml::Rule& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setRuleNodeAttributes(m.get<0>().getValue());
		dst_.setMacList(m.get<1>().getValue());
		dst_.setVlanList(m.get<2>().getValue());
		dst_.setStpList(m.get<3>().getValue());
		dst_.setArpList(m.get<4>().getValue());
		dst_.setRarpList(m.get<5>().getValue());
		dst_.setIpList(m.get<6>().getValue());
		dst_.setIpv6List(m.get<7>().getValue());
		dst_.setTcpList(m.get<8>().getValue());
		dst_.setUdpList(m.get<9>().getValue());
		dst_.setSctpList(m.get<10>().getValue());
		dst_.setIcmpList(m.get<11>().getValue());
		dst_.setIgmpList(m.get<12>().getValue());
		dst_.setAllList(m.get<13>().getValue());
		dst_.setEspList(m.get<14>().getValue());
		dst_.setAhList(m.get<15>().getValue());
		dst_.setUdpliteList(m.get<16>().getValue());
		dst_.setTcpIpv6List(m.get<17>().getValue());
		dst_.setUdpIpv6List(m.get<18>().getValue());
		dst_.setSctpIpv6List(m.get<19>().getValue());
		dst_.setIcmpv6List(m.get<20>().getValue());
		dst_.setAllIpv6List(m.get<21>().getValue());
		dst_.setEspIpv6List(m.get<22>().getValue());
		dst_.setAhIpv6List(m.get<23>().getValue());
		dst_.setUdpliteIpv6List(m.get<24>().getValue());
	}
	return output;
}

int Traits<Filter::Xml::Rule>::generate(const Filter::Xml::Rule& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getRuleNodeAttributes(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getMacList(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getVlanList(), m.get<2>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getStpList(), m.get<3>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getArpList(), m.get<4>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getRarpList(), m.get<5>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getIpList(), m.get<6>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getIpv6List(), m.get<7>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getTcpList(), m.get<8>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getUdpList(), m.get<9>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getSctpList(), m.get<10>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getIcmpList(), m.get<11>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getIgmpList(), m.get<12>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getAllList(), m.get<13>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getEspList(), m.get<14>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getAhList(), m.get<15>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getUdpliteList(), m.get<16>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getTcpIpv6List(), m.get<17>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getUdpIpv6List(), m.get<18>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getSctpIpv6List(), m.get<19>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getIcmpv6List(), m.get<20>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getAllIpv6List(), m.get<21>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getEspIpv6List(), m.get<22>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getAhIpv6List(), m.get<23>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getUdpliteIpv6List(), m.get<24>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Filter

namespace Filter
{
namespace Xml
{
bool Filter::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Filter, Name::Strict<764> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Filter::save(QDomElement& dst_) const
{
	Element<Filter, Name::Strict<764> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Filter::save(QDomDocument& dst_) const
{
	Element<Filter, Name::Strict<764> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Filter

int Traits<Filter::Xml::Filter>::parse(Filter::Xml::Filter& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setFilterNodeAttributes(m.get<0>().getValue());
		dst_.setUuid(m.get<1>().getValue());
		dst_.setChoice9242List(m.get<2>().getValue());
	}
	return output;
}

int Traits<Filter::Xml::Filter>::generate(const Filter::Xml::Filter& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getFilterNodeAttributes(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getUuid(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getChoice9242List(), m.get<2>()))
		return -1;

	return m.produce(dst_);
}

} // namespace Libvirt
