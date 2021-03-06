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

#include "snapshot_type.h"

namespace Libvirt
{
///////////////////////////////////////////////////////////////////////////////
// struct Disk4021

int Traits<Snapshot::Xml::Disk4021>::parse(Snapshot::Xml::Disk4021& , QStack<QDomElement>& stack_)
{
	marshal_type m;
	return m.consume(stack_);
}

int Traits<Snapshot::Xml::Disk4021>::generate(const Snapshot::Xml::Disk4021& , QDomElement& dst_)
{
	marshal_type m;
	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Disk4022

int Traits<Snapshot::Xml::Disk4022>::parse(Snapshot::Xml::Disk4022& , QStack<QDomElement>& stack_)
{
	marshal_type m;
	return m.consume(stack_);
}

int Traits<Snapshot::Xml::Disk4022>::generate(const Snapshot::Xml::Disk4022& , QDomElement& dst_)
{
	marshal_type m;
	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Source

namespace Snapshot
{
namespace Xml
{
bool Source::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Source, Name::Strict<501> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Source::save(QDomElement& dst_) const
{
	Element<Source, Name::Strict<501> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Source::save(QDomDocument& dst_) const
{
	Element<Source, Name::Strict<501> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Snapshot

int Traits<Snapshot::Xml::Source>::parse(Snapshot::Xml::Source& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setFile(m.get<0>().getValue());
		dst_.setStartupPolicy(m.get<1>().getValue());
	}
	return output;
}

int Traits<Snapshot::Xml::Source>::generate(const Snapshot::Xml::Source& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getFile(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getStartupPolicy(), m.get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Driver

namespace Snapshot
{
namespace Xml
{
bool Driver::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Driver, Name::Strict<546> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Driver::save(QDomElement& dst_) const
{
	Element<Driver, Name::Strict<546> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Driver::save(QDomDocument& dst_) const
{
	Element<Driver, Name::Strict<546> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Snapshot

int Traits<Snapshot::Xml::Driver>::parse(Snapshot::Xml::Driver& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setType(m.get<0>().getValue());
	}
	return output;
}

int Traits<Snapshot::Xml::Driver>::generate(const Snapshot::Xml::Driver& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getType(), m.get<0>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Variant4016

int Traits<Snapshot::Xml::Variant4016>::parse(Snapshot::Xml::Variant4016& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setSource(m.get<1>().get<0>().getValue());
		dst_.setDriver(m.get<1>().get<1>().getValue());
	}
	return output;
}

int Traits<Snapshot::Xml::Variant4016>::generate(const Snapshot::Xml::Variant4016& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getSource(), m.get<1>().get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getDriver(), m.get<1>().get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Variant4017

int Traits<Snapshot::Xml::Variant4017>::parse(Snapshot::Xml::Variant4017& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setSource(m.get<1>().get<0>().getValue());
		dst_.setDriver(m.get<1>().get<1>().getValue());
	}
	return output;
}

int Traits<Snapshot::Xml::Variant4017>::generate(const Snapshot::Xml::Variant4017& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getSource(), m.get<1>().get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getDriver(), m.get<1>().get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Host4383

int Traits<Snapshot::Xml::Host4383>::parse(Snapshot::Xml::Host4383& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setTransport(m.get<0>().getValue());
		dst_.setName(m.get<1>().getValue());
		dst_.setPort(m.get<2>().getValue());
	}
	return output;
}

int Traits<Snapshot::Xml::Host4383>::generate(const Snapshot::Xml::Host4383& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getTransport(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getName(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getPort(), m.get<2>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Source1

namespace Snapshot
{
namespace Xml
{
Source1::Source1(): m_protocol()
{
}

bool Source1::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Source1, Name::Strict<501> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Source1::save(QDomElement& dst_) const
{
	Element<Source1, Name::Strict<501> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Source1::save(QDomDocument& dst_) const
{
	Element<Source1, Name::Strict<501> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Snapshot

int Traits<Snapshot::Xml::Source1>::parse(Snapshot::Xml::Source1& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setProtocol(m.get<0>().getValue());
		dst_.setName(m.get<1>().getValue());
		dst_.setHostList(m.get<2>().getValue());
	}
	return output;
}

int Traits<Snapshot::Xml::Source1>::generate(const Snapshot::Xml::Source1& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getProtocol(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getName(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getHostList(), m.get<2>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Disk

namespace Snapshot
{
namespace Xml
{
bool Disk::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Disk, Name::Strict<472> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Disk::save(QDomElement& dst_) const
{
	Element<Disk, Name::Strict<472> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Disk::save(QDomDocument& dst_) const
{
	Element<Disk, Name::Strict<472> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Snapshot

int Traits<Snapshot::Xml::Disk>::parse(Snapshot::Xml::Disk& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setName(m.get<0>().getValue());
		dst_.setDisk(m.get<1>().getValue());
	}
	return output;
}

int Traits<Snapshot::Xml::Disk>::generate(const Snapshot::Xml::Disk& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getName(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getDisk(), m.get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Domainsnapshot

namespace Snapshot
{
namespace Xml
{
bool Domainsnapshot::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Domainsnapshot, Name::Strict<1359> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Domainsnapshot::save(QDomElement& dst_) const
{
	Element<Domainsnapshot, Name::Strict<1359> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Domainsnapshot::save(QDomDocument& dst_) const
{
	Element<Domainsnapshot, Name::Strict<1359> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Snapshot

int Traits<Snapshot::Xml::Domainsnapshot>::parse(Snapshot::Xml::Domainsnapshot& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setName(m.get<0>().getValue());
		dst_.setDescription(m.get<1>().getValue());
		dst_.setState(m.get<2>().getValue());
		dst_.setCreationTime(m.get<3>().getValue());
		dst_.setMemory(m.get<4>().getValue());
		dst_.setDisks(m.get<5>().getValue());
		dst_.setActive(m.get<6>().getValue());
		dst_.setChoice4011(m.get<7>().getValue());
		dst_.setInactiveDomain(m.get<8>().getValue());
		dst_.setXPersistent(m.get<9>().getValue());
		dst_.setParent(m.get<10>().getValue());
		dst_.setCookie(m.get<11>().getValue());
	}
	return output;
}

int Traits<Snapshot::Xml::Domainsnapshot>::generate(const Snapshot::Xml::Domainsnapshot& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getName(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getDescription(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getState(), m.get<2>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getCreationTime(), m.get<3>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getMemory(), m.get<4>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getDisks(), m.get<5>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getActive(), m.get<6>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getChoice4011(), m.get<7>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getInactiveDomain(), m.get<8>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getXPersistent(), m.get<9>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getParent(), m.get<10>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getCookie(), m.get<11>()))
		return -1;

	return m.produce(dst_);
}

} // namespace Libvirt
