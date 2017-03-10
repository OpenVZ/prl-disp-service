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

#include "domain_type.h"

namespace Libvirt
{
///////////////////////////////////////////////////////////////////////////////
// struct Ids

namespace Domain
{
namespace Xml
{
bool Ids::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	k.push(src_.firstChildElement());
	return 0 <= Traits<Ids>::parse(*this, k);
}

bool Ids::save(QDomElement& dst_) const
{
	return 0 <= Traits<Ids>::generate(*this, dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Ids>::parse(Domain::Xml::Ids& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setId(m.get<0>().getValue());
		dst_.setName(m.get<1>().get<0>().getValue());
		dst_.setUuid(m.get<1>().get<1>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Ids>::generate(const Domain::Xml::Ids& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getId(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getName(), m.get<1>().get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getUuid(), m.get<1>().get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Model

namespace Domain
{
namespace Xml
{
bool Model::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Model, Name::Strict<217> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Model::save(QDomElement& dst_) const
{
	Element<Model, Name::Strict<217> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Model::save(QDomDocument& dst_) const
{
	Element<Model, Name::Strict<217> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Model>::parse(Domain::Xml::Model& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setFallback(m.get<0>().getValue());
		dst_.setVendorId(m.get<1>().getValue());
		dst_.setOwnValue(m.get<2>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Model>::generate(const Domain::Xml::Model& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getFallback(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getVendorId(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getOwnValue(), m.get<2>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Topology

namespace Domain
{
namespace Xml
{
Topology::Topology(): m_sockets(), m_cores(), m_threads()
{
}

bool Topology::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Topology, Name::Strict<979> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Topology::save(QDomElement& dst_) const
{
	Element<Topology, Name::Strict<979> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Topology::save(QDomDocument& dst_) const
{
	Element<Topology, Name::Strict<979> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Topology>::parse(Domain::Xml::Topology& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setSockets(m.get<0>().getValue());
		dst_.setCores(m.get<1>().getValue());
		dst_.setThreads(m.get<2>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Topology>::generate(const Domain::Xml::Topology& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getSockets(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getCores(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getThreads(), m.get<2>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Feature

namespace Domain
{
namespace Xml
{
Feature::Feature(): m_policy()
{
}

bool Feature::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Feature, Name::Strict<973> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Feature::save(QDomElement& dst_) const
{
	Element<Feature, Name::Strict<973> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Feature::save(QDomDocument& dst_) const
{
	Element<Feature, Name::Strict<973> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Feature>::parse(Domain::Xml::Feature& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setPolicy(m.get<0>().getValue());
		dst_.setName(m.get<1>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Feature>::generate(const Domain::Xml::Feature& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getPolicy(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getName(), m.get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Cell

namespace Domain
{
namespace Xml
{
Cell::Cell(): m_memory()
{
}

bool Cell::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Cell, Name::Strict<984> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Cell::save(QDomElement& dst_) const
{
	Element<Cell, Name::Strict<984> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Cell::save(QDomDocument& dst_) const
{
	Element<Cell, Name::Strict<984> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Cell>::parse(Domain::Xml::Cell& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setId(m.get<0>().getValue());
		dst_.setCpus(m.get<1>().getValue());
		dst_.setMemory(m.get<2>().getValue());
		dst_.setMemAccess(m.get<3>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Cell>::generate(const Domain::Xml::Cell& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getId(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getCpus(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getMemory(), m.get<2>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getMemAccess(), m.get<3>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Cpu

namespace Domain
{
namespace Xml
{
bool Cpu::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Cpu, Name::Strict<206> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Cpu::save(QDomElement& dst_) const
{
	Element<Cpu, Name::Strict<206> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Cpu::save(QDomDocument& dst_) const
{
	Element<Cpu, Name::Strict<206> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Cpu>::parse(Domain::Xml::Cpu& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setMode(m.get<0>().getValue());
		dst_.setMatch(m.get<1>().getValue());
		dst_.setModel(m.get<2>().get<0>().getValue());
		dst_.setVendor(m.get<2>().get<1>().getValue());
		dst_.setTopology(m.get<2>().get<2>().getValue());
		dst_.setFeatureList(m.get<2>().get<3>().getValue());
		dst_.setNuma(m.get<2>().get<4>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Cpu>::generate(const Domain::Xml::Cpu& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getMode(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getMatch(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getModel(), m.get<2>().get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getVendor(), m.get<2>().get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getTopology(), m.get<2>().get<2>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getFeatureList(), m.get<2>().get<3>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getNuma(), m.get<2>().get<4>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Entry

namespace Domain
{
namespace Xml
{
Entry::Entry(): m_name()
{
}

bool Entry::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Entry, Name::Strict<990> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Entry::save(QDomElement& dst_) const
{
	Element<Entry, Name::Strict<990> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Entry::save(QDomDocument& dst_) const
{
	Element<Entry, Name::Strict<990> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Entry>::parse(Domain::Xml::Entry& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setName(m.get<0>().getValue());
		dst_.setOwnValue(m.get<1>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Entry>::generate(const Domain::Xml::Entry& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getName(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getOwnValue(), m.get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Entry1

namespace Domain
{
namespace Xml
{
Entry1::Entry1(): m_name()
{
}

bool Entry1::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Entry1, Name::Strict<990> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Entry1::save(QDomElement& dst_) const
{
	Element<Entry1, Name::Strict<990> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Entry1::save(QDomDocument& dst_) const
{
	Element<Entry1, Name::Strict<990> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Entry1>::parse(Domain::Xml::Entry1& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setName(m.get<0>().getValue());
		dst_.setOwnValue(m.get<1>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Entry1>::generate(const Domain::Xml::Entry1& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getName(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getOwnValue(), m.get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Sysinfo

namespace Domain
{
namespace Xml
{
bool Sysinfo::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Sysinfo, Name::Strict<207> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Sysinfo::save(QDomElement& dst_) const
{
	Element<Sysinfo, Name::Strict<207> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Sysinfo::save(QDomDocument& dst_) const
{
	Element<Sysinfo, Name::Strict<207> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Sysinfo>::parse(Domain::Xml::Sysinfo& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setBios(m.get<1>().get<0>().getValue());
		dst_.setSystem(m.get<1>().get<1>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Sysinfo>::generate(const Domain::Xml::Sysinfo& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getBios(), m.get<1>().get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getSystem(), m.get<1>().get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Bootloader

namespace Domain
{
namespace Xml
{
bool Bootloader::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	k.push(src_.firstChildElement());
	return 0 <= Traits<Bootloader>::parse(*this, k);
}

bool Bootloader::save(QDomElement& dst_) const
{
	return 0 <= Traits<Bootloader>::generate(*this, dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Bootloader>::parse(Domain::Xml::Bootloader& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setBootloader(m.get<0>().getValue());
		dst_.setBootloaderArgs(m.get<1>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Bootloader>::generate(const Domain::Xml::Bootloader& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getBootloader(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getBootloaderArgs(), m.get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Type

namespace Domain
{
namespace Xml
{
Type::Type(): m_type()
{
}

bool Type::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Type, Name::Strict<100> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Type::save(QDomElement& dst_) const
{
	Element<Type, Name::Strict<100> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Type::save(QDomDocument& dst_) const
{
	Element<Type, Name::Strict<100> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Type>::parse(Domain::Xml::Type& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setArch(m.get<0>().getValue());
		dst_.setMachine(m.get<1>().getValue());
		dst_.setType(m.get<2>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Type>::generate(const Domain::Xml::Type& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getArch(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getMachine(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getType(), m.get<2>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Osbootkernel

namespace Domain
{
namespace Xml
{
bool Osbootkernel::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	k.push(src_.firstChildElement());
	return 0 <= Traits<Osbootkernel>::parse(*this, k);
}

bool Osbootkernel::save(QDomElement& dst_) const
{
	return 0 <= Traits<Osbootkernel>::generate(*this, dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Osbootkernel>::parse(Domain::Xml::Osbootkernel& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setKernel(m.get<0>().getValue());
		dst_.setInitrd(m.get<1>().getValue());
		dst_.setRoot(m.get<2>().getValue());
		dst_.setCmdline(m.get<3>().getValue());
		dst_.setDtb(m.get<4>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Osbootkernel>::generate(const Domain::Xml::Osbootkernel& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getKernel(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getInitrd(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getRoot(), m.get<2>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getCmdline(), m.get<3>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getDtb(), m.get<4>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Os

namespace Domain
{
namespace Xml
{
bool Os::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Os, Name::Strict<208> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Os::save(QDomElement& dst_) const
{
	Element<Os, Name::Strict<208> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Os::save(QDomDocument& dst_) const
{
	Element<Os, Name::Strict<208> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Os>::parse(Domain::Xml::Os& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setType(m.get<0>().getValue());
		dst_.setOsbootkernel(m.get<1>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Os>::generate(const Domain::Xml::Os& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getType(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getOsbootkernel(), m.get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Osxen255

int Traits<Domain::Xml::Osxen255>::parse(Domain::Xml::Osxen255& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setBootloader(m.get<0>().getValue());
		dst_.setOs(m.get<1>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Osxen255>::generate(const Domain::Xml::Osxen255& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getBootloader(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getOs(), m.get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Os1

namespace Domain
{
namespace Xml
{
bool Os1::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Os1, Name::Strict<208> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Os1::save(QDomElement& dst_) const
{
	Element<Os1, Name::Strict<208> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Os1::save(QDomDocument& dst_) const
{
	Element<Os1, Name::Strict<208> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Os1>::parse(Domain::Xml::Os1& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setType(m.get<0>().getValue());
		dst_.setOsbootkernel(m.get<1>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Os1>::generate(const Domain::Xml::Os1& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getType(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getOsbootkernel(), m.get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Osxen256

int Traits<Domain::Xml::Osxen256>::parse(Domain::Xml::Osxen256& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setBootloader(m.get<0>().getValue());
		dst_.setOs(m.get<1>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Osxen256>::generate(const Domain::Xml::Osxen256& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getBootloader(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getOs(), m.get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Hvmx86

int Traits<Domain::Xml::Hvmx86>::parse(Domain::Xml::Hvmx86& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setArch(m.get<0>().getValue());
		dst_.setMachine(m.get<1>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Hvmx86>::generate(const Domain::Xml::Hvmx86& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getArch(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getMachine(), m.get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Hvmmips

int Traits<Domain::Xml::Hvmmips>::parse(Domain::Xml::Hvmmips& , QStack<QDomElement>& stack_)
{
	marshal_type m;
	return m.consume(stack_);
}

int Traits<Domain::Xml::Hvmmips>::generate(const Domain::Xml::Hvmmips& , QDomElement& dst_)
{
	marshal_type m;
	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Hvmsparc

int Traits<Domain::Xml::Hvmsparc>::parse(Domain::Xml::Hvmsparc& , QStack<QDomElement>& stack_)
{
	marshal_type m;
	return m.consume(stack_);
}

int Traits<Domain::Xml::Hvmsparc>::generate(const Domain::Xml::Hvmsparc& , QDomElement& dst_)
{
	marshal_type m;
	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Hvms390

int Traits<Domain::Xml::Hvms390>::parse(Domain::Xml::Hvms390& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setArch(m.get<0>().getValue());
		dst_.setMachine(m.get<1>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Hvms390>::generate(const Domain::Xml::Hvms390& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getArch(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getMachine(), m.get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Hvmarm

int Traits<Domain::Xml::Hvmarm>::parse(Domain::Xml::Hvmarm& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setArch(m.get<0>().getValue());
		dst_.setMachine(m.get<1>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Hvmarm>::generate(const Domain::Xml::Hvmarm& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getArch(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getMachine(), m.get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Hvmaarch64

int Traits<Domain::Xml::Hvmaarch64>::parse(Domain::Xml::Hvmaarch64& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setArch(m.get<0>().getValue());
		dst_.setMachine(m.get<1>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Hvmaarch64>::generate(const Domain::Xml::Hvmaarch64& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getArch(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getMachine(), m.get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Loader

namespace Domain
{
namespace Xml
{
bool Loader::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Loader, Name::Strict<259> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Loader::save(QDomElement& dst_) const
{
	Element<Loader, Name::Strict<259> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Loader::save(QDomDocument& dst_) const
{
	Element<Loader, Name::Strict<259> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Loader>::parse(Domain::Xml::Loader& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setReadonly(m.get<0>().getValue());
		dst_.setType(m.get<1>().getValue());
		dst_.setOwnValue(m.get<2>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Loader>::generate(const Domain::Xml::Loader& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getReadonly(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getType(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getOwnValue(), m.get<2>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Nvram

namespace Domain
{
namespace Xml
{
bool Nvram::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Nvram, Name::Strict<263> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Nvram::save(QDomElement& dst_) const
{
	Element<Nvram, Name::Strict<263> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Nvram::save(QDomDocument& dst_) const
{
	Element<Nvram, Name::Strict<263> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Nvram>::parse(Domain::Xml::Nvram& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setTemplate(m.get<0>().getValue());
		dst_.setFormat(m.get<1>().getValue());
		dst_.setOwnValue(m.get<2>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Nvram>::generate(const Domain::Xml::Nvram& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getTemplate(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getFormat(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getOwnValue(), m.get<2>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Bootmenu

namespace Domain
{
namespace Xml
{
Bootmenu::Bootmenu(): m_enable()
{
}

bool Bootmenu::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Bootmenu, Name::Strict<266> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Bootmenu::save(QDomElement& dst_) const
{
	Element<Bootmenu, Name::Strict<266> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Bootmenu::save(QDomDocument& dst_) const
{
	Element<Bootmenu, Name::Strict<266> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Bootmenu>::parse(Domain::Xml::Bootmenu& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setEnable(m.get<0>().getValue());
		dst_.setTimeout(m.get<1>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Bootmenu>::generate(const Domain::Xml::Bootmenu& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getEnable(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getTimeout(), m.get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Bios

namespace Domain
{
namespace Xml
{
bool Bios::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Bios, Name::Strict<270> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Bios::save(QDomElement& dst_) const
{
	Element<Bios, Name::Strict<270> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Bios::save(QDomDocument& dst_) const
{
	Element<Bios, Name::Strict<270> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Bios>::parse(Domain::Xml::Bios& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setUseserial(m.get<0>().getValue());
		dst_.setRebootTimeout(m.get<1>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Bios>::generate(const Domain::Xml::Bios& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getUseserial(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getRebootTimeout(), m.get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Os2

namespace Domain
{
namespace Xml
{
bool Os2::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Os2, Name::Strict<208> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Os2::save(QDomElement& dst_) const
{
	Element<Os2, Name::Strict<208> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Os2::save(QDomDocument& dst_) const
{
	Element<Os2, Name::Strict<208> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Os2>::parse(Domain::Xml::Os2& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setType(m.get<0>().getValue());
		dst_.setLoader(m.get<1>().get<0>().getValue());
		dst_.setNvram(m.get<1>().get<1>().getValue());
		dst_.setOsbootkernel(m.get<1>().get<2>().getValue());
		dst_.setBootList(m.get<1>().get<3>().getValue());
		dst_.setBootmenu(m.get<1>().get<4>().getValue());
		dst_.setSmbios(m.get<1>().get<5>().getValue());
		dst_.setBios(m.get<1>().get<6>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Os2>::generate(const Domain::Xml::Os2& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getType(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getLoader(), m.get<1>().get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getNvram(), m.get<1>().get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getOsbootkernel(), m.get<1>().get<2>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getBootList(), m.get<1>().get<3>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getBootmenu(), m.get<1>().get<4>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getSmbios(), m.get<1>().get<5>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getBios(), m.get<1>().get<6>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Os3

namespace Domain
{
namespace Xml
{
bool Os3::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Os3, Name::Strict<208> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Os3::save(QDomElement& dst_) const
{
	Element<Os3, Name::Strict<208> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Os3::save(QDomDocument& dst_) const
{
	Element<Os3, Name::Strict<208> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Os3>::parse(Domain::Xml::Os3& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setType(m.get<0>().getValue());
		dst_.setInit(m.get<1>().get<0>().getValue());
		dst_.setInitargList(m.get<1>().get<1>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Os3>::generate(const Domain::Xml::Os3& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getType(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getInit(), m.get<1>().get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getInitargList(), m.get<1>().get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Clock377

namespace Domain
{
namespace Xml
{
Clock377::Clock377(): m_offset()
{
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Clock377>::parse(Domain::Xml::Clock377& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setOffset(m.get<0>().getValue());
		dst_.setAdjustment(m.get<1>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Clock377>::generate(const Domain::Xml::Clock377& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getOffset(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getAdjustment(), m.get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Clock383

int Traits<Domain::Xml::Clock383>::parse(Domain::Xml::Clock383& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setAdjustment(m.get<1>().getValue());
		dst_.setBasis(m.get<2>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Clock383>::generate(const Domain::Xml::Clock383& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getAdjustment(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getBasis(), m.get<2>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Catchup

namespace Domain
{
namespace Xml
{
bool Catchup::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Catchup, Name::Strict<412> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Catchup::save(QDomElement& dst_) const
{
	Element<Catchup, Name::Strict<412> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Catchup::save(QDomDocument& dst_) const
{
	Element<Catchup, Name::Strict<412> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Catchup>::parse(Domain::Xml::Catchup& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setThreshold(m.get<0>().getValue());
		dst_.setSlew(m.get<1>().getValue());
		dst_.setLimit(m.get<2>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Catchup>::generate(const Domain::Xml::Catchup& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getThreshold(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getSlew(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getLimit(), m.get<2>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Timer392

namespace Domain
{
namespace Xml
{
Timer392::Timer392(): m_name()
{
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Timer392>::parse(Domain::Xml::Timer392& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setName(m.get<0>().getValue());
		dst_.setTrack(m.get<1>().getValue());
		dst_.setTickpolicy(m.get<2>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Timer392>::generate(const Domain::Xml::Timer392& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getName(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getTrack(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getTickpolicy(), m.get<2>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Timer399

int Traits<Domain::Xml::Timer399>::parse(Domain::Xml::Timer399& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setTickpolicy(m.get<1>().getValue());
		dst_.setFrequency(m.get<2>().getValue());
		dst_.setMode(m.get<3>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Timer399>::generate(const Domain::Xml::Timer399& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getTickpolicy(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getFrequency(), m.get<2>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getMode(), m.get<3>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Timer402

namespace Domain
{
namespace Xml
{
Timer402::Timer402(): m_name()
{
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Timer402>::parse(Domain::Xml::Timer402& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setName(m.get<0>().getValue());
		dst_.setTickpolicy(m.get<1>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Timer402>::generate(const Domain::Xml::Timer402& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getName(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getTickpolicy(), m.get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Timer

namespace Domain
{
namespace Xml
{
bool Timer::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Timer, Name::Strict<385> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Timer::save(QDomElement& dst_) const
{
	Element<Timer, Name::Strict<385> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Timer::save(QDomDocument& dst_) const
{
	Element<Timer, Name::Strict<385> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Timer>::parse(Domain::Xml::Timer& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setTimer(m.get<0>().getValue());
		dst_.setPresent(m.get<1>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Timer>::generate(const Domain::Xml::Timer& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getTimer(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getPresent(), m.get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Clock

namespace Domain
{
namespace Xml
{
bool Clock::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Clock, Name::Strict<209> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Clock::save(QDomElement& dst_) const
{
	Element<Clock, Name::Strict<209> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Clock::save(QDomDocument& dst_) const
{
	Element<Clock, Name::Strict<209> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Clock>::parse(Domain::Xml::Clock& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setClock(m.get<0>().getValue());
		dst_.setTimerList(m.get<1>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Clock>::generate(const Domain::Xml::Clock& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getClock(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getTimerList(), m.get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct ScaledInteger

namespace Domain
{
namespace Xml
{
ScaledInteger::ScaledInteger(): m_ownValue()
{
}

bool ScaledInteger::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<ScaledInteger, Name::Strict<320> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool ScaledInteger::save(QDomElement& dst_) const
{
	Element<ScaledInteger, Name::Strict<320> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool ScaledInteger::save(QDomDocument& dst_) const
{
	Element<ScaledInteger, Name::Strict<320> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::ScaledInteger>::parse(Domain::Xml::ScaledInteger& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setUnit(m.get<0>().getValue());
		dst_.setOwnValue(m.get<1>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::ScaledInteger>::generate(const Domain::Xml::ScaledInteger& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getUnit(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getOwnValue(), m.get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Memory

namespace Domain
{
namespace Xml
{
bool Memory::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Memory, Name::Strict<312> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Memory::save(QDomElement& dst_) const
{
	Element<Memory, Name::Strict<312> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Memory::save(QDomDocument& dst_) const
{
	Element<Memory, Name::Strict<312> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Memory>::parse(Domain::Xml::Memory& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setScaledInteger(m.get<0>().getValue());
		dst_.setDumpCore(m.get<1>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Memory>::generate(const Domain::Xml::Memory& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getScaledInteger(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getDumpCore(), m.get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct MaxMemory

namespace Domain
{
namespace Xml
{
MaxMemory::MaxMemory(): m_slots()
{
}

bool MaxMemory::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<MaxMemory, Name::Strict<314> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool MaxMemory::save(QDomElement& dst_) const
{
	Element<MaxMemory, Name::Strict<314> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool MaxMemory::save(QDomDocument& dst_) const
{
	Element<MaxMemory, Name::Strict<314> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::MaxMemory>::parse(Domain::Xml::MaxMemory& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setScaledInteger(m.get<0>().getValue());
		dst_.setSlots(m.get<1>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::MaxMemory>::generate(const Domain::Xml::MaxMemory& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getScaledInteger(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getSlots(), m.get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Page

namespace Domain
{
namespace Xml
{
Page::Page(): m_size()
{
}

bool Page::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Page, Name::Strict<319> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Page::save(QDomElement& dst_) const
{
	Element<Page, Name::Strict<319> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Page::save(QDomDocument& dst_) const
{
	Element<Page, Name::Strict<319> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Page>::parse(Domain::Xml::Page& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setSize(m.get<0>().getValue());
		dst_.setUnit(m.get<1>().getValue());
		dst_.setNodeset(m.get<2>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Page>::generate(const Domain::Xml::Page& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getSize(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getUnit(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getNodeset(), m.get<2>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct MemoryBacking

namespace Domain
{
namespace Xml
{
MemoryBacking::MemoryBacking(): m_nosharepages(), m_locked()
{
}

bool MemoryBacking::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<MemoryBacking, Name::Strict<317> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool MemoryBacking::save(QDomElement& dst_) const
{
	Element<MemoryBacking, Name::Strict<317> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool MemoryBacking::save(QDomDocument& dst_) const
{
	Element<MemoryBacking, Name::Strict<317> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::MemoryBacking>::parse(Domain::Xml::MemoryBacking& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setHugepages(m.get<0>().getValue());
		dst_.setNosharepages(m.get<1>().getValue());
		dst_.setLocked(m.get<2>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::MemoryBacking>::generate(const Domain::Xml::MemoryBacking& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getHugepages(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getNosharepages(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getLocked(), m.get<2>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Vcpu

namespace Domain
{
namespace Xml
{
Vcpu::Vcpu(): m_ownValue()
{
}

bool Vcpu::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Vcpu, Name::Strict<324> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Vcpu::save(QDomElement& dst_) const
{
	Element<Vcpu, Name::Strict<324> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Vcpu::save(QDomDocument& dst_) const
{
	Element<Vcpu, Name::Strict<324> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Vcpu>::parse(Domain::Xml::Vcpu& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setPlacement(m.get<0>().getValue());
		dst_.setCpuset(m.get<1>().getValue());
		dst_.setCurrent(m.get<2>().getValue());
		dst_.setOwnValue(m.get<3>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Vcpu>::generate(const Domain::Xml::Vcpu& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getPlacement(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getCpuset(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getCurrent(), m.get<2>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getOwnValue(), m.get<3>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Device

namespace Domain
{
namespace Xml
{
bool Device::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Device, Name::Strict<336> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Device::save(QDomElement& dst_) const
{
	Element<Device, Name::Strict<336> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Device::save(QDomDocument& dst_) const
{
	Element<Device, Name::Strict<336> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Device>::parse(Domain::Xml::Device& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setPath(m.get<0>().getValue());
		dst_.setWeight(m.get<1>().getValue());
		dst_.setReadIopsSec(m.get<2>().getValue());
		dst_.setWriteIopsSec(m.get<3>().getValue());
		dst_.setReadBytesSec(m.get<4>().getValue());
		dst_.setWriteBytesSec(m.get<5>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Device>::generate(const Domain::Xml::Device& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getPath(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getWeight(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getReadIopsSec(), m.get<2>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getWriteIopsSec(), m.get<3>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getReadBytesSec(), m.get<4>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getWriteBytesSec(), m.get<5>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Blkiotune

namespace Domain
{
namespace Xml
{
bool Blkiotune::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Blkiotune, Name::Strict<330> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Blkiotune::save(QDomElement& dst_) const
{
	Element<Blkiotune, Name::Strict<330> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Blkiotune::save(QDomDocument& dst_) const
{
	Element<Blkiotune, Name::Strict<330> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Blkiotune>::parse(Domain::Xml::Blkiotune& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setWeight(m.get<0>().getValue());
		dst_.setDeviceList(m.get<1>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Blkiotune>::generate(const Domain::Xml::Blkiotune& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getWeight(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getDeviceList(), m.get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Memtune

namespace Domain
{
namespace Xml
{
bool Memtune::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Memtune, Name::Strict<331> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Memtune::save(QDomElement& dst_) const
{
	Element<Memtune, Name::Strict<331> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Memtune::save(QDomDocument& dst_) const
{
	Element<Memtune, Name::Strict<331> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Memtune>::parse(Domain::Xml::Memtune& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setHardLimit(m.get<0>().getValue());
		dst_.setSoftLimit(m.get<1>().getValue());
		dst_.setMinGuarantee(m.get<2>().getValue());
		dst_.setSwapHardLimit(m.get<3>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Memtune>::generate(const Domain::Xml::Memtune& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getHardLimit(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getSoftLimit(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getMinGuarantee(), m.get<2>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getSwapHardLimit(), m.get<3>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Vcpupin

namespace Domain
{
namespace Xml
{
Vcpupin::Vcpupin(): m_vcpu()
{
}

bool Vcpupin::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Vcpupin, Name::Strict<356> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Vcpupin::save(QDomElement& dst_) const
{
	Element<Vcpupin, Name::Strict<356> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Vcpupin::save(QDomDocument& dst_) const
{
	Element<Vcpupin, Name::Strict<356> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Vcpupin>::parse(Domain::Xml::Vcpupin& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setVcpu(m.get<0>().getValue());
		dst_.setCpuset(m.get<1>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Vcpupin>::generate(const Domain::Xml::Vcpupin& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getVcpu(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getCpuset(), m.get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Iothreadpin

namespace Domain
{
namespace Xml
{
Iothreadpin::Iothreadpin(): m_iothread()
{
}

bool Iothreadpin::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Iothreadpin, Name::Strict<359> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Iothreadpin::save(QDomElement& dst_) const
{
	Element<Iothreadpin, Name::Strict<359> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Iothreadpin::save(QDomDocument& dst_) const
{
	Element<Iothreadpin, Name::Strict<359> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Iothreadpin>::parse(Domain::Xml::Iothreadpin& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setIothread(m.get<0>().getValue());
		dst_.setCpuset(m.get<1>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Iothreadpin>::generate(const Domain::Xml::Iothreadpin& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getIothread(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getCpuset(), m.get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Cputune

namespace Domain
{
namespace Xml
{
bool Cputune::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Cputune, Name::Strict<332> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Cputune::save(QDomElement& dst_) const
{
	Element<Cputune, Name::Strict<332> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Cputune::save(QDomDocument& dst_) const
{
	Element<Cputune, Name::Strict<332> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Cputune>::parse(Domain::Xml::Cputune& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setShares(m.get<0>().getValue());
		dst_.setGlobalPeriod(m.get<1>().getValue());
		dst_.setGlobalQuota(m.get<2>().getValue());
		dst_.setPeriod(m.get<3>().getValue());
		dst_.setQuota(m.get<4>().getValue());
		dst_.setEmulatorPeriod(m.get<5>().getValue());
		dst_.setEmulatorQuota(m.get<6>().getValue());
		dst_.setVcpupinList(m.get<7>().getValue());
		dst_.setEmulatorpin(m.get<8>().getValue());
		dst_.setIothreadpinList(m.get<9>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Cputune>::generate(const Domain::Xml::Cputune& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getShares(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getGlobalPeriod(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getGlobalQuota(), m.get<2>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getPeriod(), m.get<3>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getQuota(), m.get<4>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getEmulatorPeriod(), m.get<5>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getEmulatorQuota(), m.get<6>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getVcpupinList(), m.get<7>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getEmulatorpin(), m.get<8>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getIothreadpinList(), m.get<9>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Memory1131

int Traits<Domain::Xml::Memory1131>::parse(Domain::Xml::Memory1131& , QStack<QDomElement>& stack_)
{
	marshal_type m;
	return m.consume(stack_);
}

int Traits<Domain::Xml::Memory1131>::generate(const Domain::Xml::Memory1131& , QDomElement& dst_)
{
	marshal_type m;
	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Memory1

namespace Domain
{
namespace Xml
{
bool Memory1::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Memory1, Name::Strict<312> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Memory1::save(QDomElement& dst_) const
{
	Element<Memory1, Name::Strict<312> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Memory1::save(QDomDocument& dst_) const
{
	Element<Memory1, Name::Strict<312> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Memory1>::parse(Domain::Xml::Memory1& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setMode(m.get<0>().getValue());
		dst_.setMemory(m.get<1>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Memory1>::generate(const Domain::Xml::Memory1& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getMode(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getMemory(), m.get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Memnode

namespace Domain
{
namespace Xml
{
Memnode::Memnode(): m_cellid(), m_mode()
{
}

bool Memnode::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Memnode, Name::Strict<367> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Memnode::save(QDomElement& dst_) const
{
	Element<Memnode, Name::Strict<367> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Memnode::save(QDomDocument& dst_) const
{
	Element<Memnode, Name::Strict<367> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Memnode>::parse(Domain::Xml::Memnode& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setCellid(m.get<0>().getValue());
		dst_.setMode(m.get<1>().getValue());
		dst_.setNodeset(m.get<2>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Memnode>::generate(const Domain::Xml::Memnode& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getCellid(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getMode(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getNodeset(), m.get<2>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Numatune

namespace Domain
{
namespace Xml
{
bool Numatune::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Numatune, Name::Strict<333> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Numatune::save(QDomElement& dst_) const
{
	Element<Numatune, Name::Strict<333> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Numatune::save(QDomDocument& dst_) const
{
	Element<Numatune, Name::Strict<333> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Numatune>::parse(Domain::Xml::Numatune& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setMemory(m.get<0>().getValue());
		dst_.setMemnodeList(m.get<1>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Numatune>::generate(const Domain::Xml::Numatune& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getMemory(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getMemnodeList(), m.get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Apic

namespace Domain
{
namespace Xml
{
bool Apic::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Apic, Name::Strict<945> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Apic::save(QDomElement& dst_) const
{
	Element<Apic, Name::Strict<945> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Apic::save(QDomDocument& dst_) const
{
	Element<Apic, Name::Strict<945> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Apic>::parse(Domain::Xml::Apic& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setEoi(m.get<0>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Apic>::generate(const Domain::Xml::Apic& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getEoi(), m.get<0>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Spinlocks

namespace Domain
{
namespace Xml
{
Spinlocks::Spinlocks(): m_state()
{
}

bool Spinlocks::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Spinlocks, Name::Strict<1065> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Spinlocks::save(QDomElement& dst_) const
{
	Element<Spinlocks, Name::Strict<1065> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Spinlocks::save(QDomDocument& dst_) const
{
	Element<Spinlocks, Name::Strict<1065> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Spinlocks>::parse(Domain::Xml::Spinlocks& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setState(m.get<0>().getValue());
		dst_.setRetries(m.get<1>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Spinlocks>::generate(const Domain::Xml::Spinlocks& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getState(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getRetries(), m.get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct VendorId

namespace Domain
{
namespace Xml
{
VendorId::VendorId(): m_state()
{
}

bool VendorId::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<VendorId, Name::Strict<971> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool VendorId::save(QDomElement& dst_) const
{
	Element<VendorId, Name::Strict<971> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool VendorId::save(QDomDocument& dst_) const
{
	Element<VendorId, Name::Strict<971> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::VendorId>::parse(Domain::Xml::VendorId& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setState(m.get<0>().getValue());
		dst_.setValue(m.get<1>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::VendorId>::generate(const Domain::Xml::VendorId& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getState(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getValue(), m.get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Hyperv

namespace Domain
{
namespace Xml
{
bool Hyperv::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Hyperv, Name::Strict<242> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Hyperv::save(QDomElement& dst_) const
{
	Element<Hyperv, Name::Strict<242> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Hyperv::save(QDomDocument& dst_) const
{
	Element<Hyperv, Name::Strict<242> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Hyperv>::parse(Domain::Xml::Hyperv& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setRelaxed(m.get<0>().getValue());
		dst_.setVapic(m.get<1>().getValue());
		dst_.setSpinlocks(m.get<2>().getValue());
		dst_.setVpindex(m.get<3>().getValue());
		dst_.setRuntime(m.get<4>().getValue());
		dst_.setSynic(m.get<5>().getValue());
		dst_.setStimer(m.get<6>().getValue());
		dst_.setReset(m.get<7>().getValue());
		dst_.setVendorId(m.get<8>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Hyperv>::generate(const Domain::Xml::Hyperv& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getRelaxed(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getVapic(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getSpinlocks(), m.get<2>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getVpindex(), m.get<3>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getRuntime(), m.get<4>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getSynic(), m.get<5>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getStimer(), m.get<6>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getReset(), m.get<7>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getVendorId(), m.get<8>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Kvm

namespace Domain
{
namespace Xml
{
bool Kvm::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Kvm, Name::Strict<235> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Kvm::save(QDomElement& dst_) const
{
	Element<Kvm, Name::Strict<235> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Kvm::save(QDomDocument& dst_) const
{
	Element<Kvm, Name::Strict<235> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Kvm>::parse(Domain::Xml::Kvm& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setHidden(m.get<0>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Kvm>::generate(const Domain::Xml::Kvm& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getHidden(), m.get<0>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Pvspinlock

namespace Domain
{
namespace Xml
{
bool Pvspinlock::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Pvspinlock, Name::Strict<951> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Pvspinlock::save(QDomElement& dst_) const
{
	Element<Pvspinlock, Name::Strict<951> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Pvspinlock::save(QDomDocument& dst_) const
{
	Element<Pvspinlock, Name::Strict<951> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Pvspinlock>::parse(Domain::Xml::Pvspinlock& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setState(m.get<0>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Pvspinlock>::generate(const Domain::Xml::Pvspinlock& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getState(), m.get<0>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Capabilities

namespace Domain
{
namespace Xml
{
Capabilities::Capabilities(): m_policy()
{
}

bool Capabilities::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Capabilities, Name::Strict<883> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Capabilities::save(QDomElement& dst_) const
{
	Element<Capabilities, Name::Strict<883> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Capabilities::save(QDomDocument& dst_) const
{
	Element<Capabilities, Name::Strict<883> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Capabilities>::parse(Domain::Xml::Capabilities& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setPolicy(m.get<0>().getValue());
		dst_.setAuditControl(m.get<1>().get<0>().getValue());
		dst_.setAuditWrite(m.get<1>().get<1>().getValue());
		dst_.setBlockSuspend(m.get<1>().get<2>().getValue());
		dst_.setChown(m.get<1>().get<3>().getValue());
		dst_.setDacOverride(m.get<1>().get<4>().getValue());
		dst_.setDacReadSearch(m.get<1>().get<5>().getValue());
		dst_.setFowner(m.get<1>().get<6>().getValue());
		dst_.setFsetid(m.get<1>().get<7>().getValue());
		dst_.setIpcLock(m.get<1>().get<8>().getValue());
		dst_.setIpcOwner(m.get<1>().get<9>().getValue());
		dst_.setKill(m.get<1>().get<10>().getValue());
		dst_.setLease(m.get<1>().get<11>().getValue());
		dst_.setLinuxImmutable(m.get<1>().get<12>().getValue());
		dst_.setMacAdmin(m.get<1>().get<13>().getValue());
		dst_.setMacOverride(m.get<1>().get<14>().getValue());
		dst_.setMknod(m.get<1>().get<15>().getValue());
		dst_.setNetAdmin(m.get<1>().get<16>().getValue());
		dst_.setNetBindService(m.get<1>().get<17>().getValue());
		dst_.setNetBroadcast(m.get<1>().get<18>().getValue());
		dst_.setNetRaw(m.get<1>().get<19>().getValue());
		dst_.setSetgid(m.get<1>().get<20>().getValue());
		dst_.setSetfcap(m.get<1>().get<21>().getValue());
		dst_.setSetpcap(m.get<1>().get<22>().getValue());
		dst_.setSetuid(m.get<1>().get<23>().getValue());
		dst_.setSysAdmin(m.get<1>().get<24>().getValue());
		dst_.setSysBoot(m.get<1>().get<25>().getValue());
		dst_.setSysChroot(m.get<1>().get<26>().getValue());
		dst_.setSysModule(m.get<1>().get<27>().getValue());
		dst_.setSysNice(m.get<1>().get<28>().getValue());
		dst_.setSysPacct(m.get<1>().get<29>().getValue());
		dst_.setSysPtrace(m.get<1>().get<30>().getValue());
		dst_.setSysRawio(m.get<1>().get<31>().getValue());
		dst_.setSysResource(m.get<1>().get<32>().getValue());
		dst_.setSysTime(m.get<1>().get<33>().getValue());
		dst_.setSysTtyConfig(m.get<1>().get<34>().getValue());
		dst_.setSyslog(m.get<1>().get<35>().getValue());
		dst_.setWakeAlarm(m.get<1>().get<36>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Capabilities>::generate(const Domain::Xml::Capabilities& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getPolicy(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getAuditControl(), m.get<1>().get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getAuditWrite(), m.get<1>().get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getBlockSuspend(), m.get<1>().get<2>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getChown(), m.get<1>().get<3>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getDacOverride(), m.get<1>().get<4>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getDacReadSearch(), m.get<1>().get<5>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getFowner(), m.get<1>().get<6>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getFsetid(), m.get<1>().get<7>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getIpcLock(), m.get<1>().get<8>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getIpcOwner(), m.get<1>().get<9>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getKill(), m.get<1>().get<10>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getLease(), m.get<1>().get<11>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getLinuxImmutable(), m.get<1>().get<12>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getMacAdmin(), m.get<1>().get<13>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getMacOverride(), m.get<1>().get<14>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getMknod(), m.get<1>().get<15>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getNetAdmin(), m.get<1>().get<16>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getNetBindService(), m.get<1>().get<17>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getNetBroadcast(), m.get<1>().get<18>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getNetRaw(), m.get<1>().get<19>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getSetgid(), m.get<1>().get<20>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getSetfcap(), m.get<1>().get<21>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getSetpcap(), m.get<1>().get<22>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getSetuid(), m.get<1>().get<23>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getSysAdmin(), m.get<1>().get<24>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getSysBoot(), m.get<1>().get<25>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getSysChroot(), m.get<1>().get<26>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getSysModule(), m.get<1>().get<27>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getSysNice(), m.get<1>().get<28>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getSysPacct(), m.get<1>().get<29>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getSysPtrace(), m.get<1>().get<30>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getSysRawio(), m.get<1>().get<31>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getSysResource(), m.get<1>().get<32>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getSysTime(), m.get<1>().get<33>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getSysTtyConfig(), m.get<1>().get<34>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getSyslog(), m.get<1>().get<35>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getWakeAlarm(), m.get<1>().get<36>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Pmu

namespace Domain
{
namespace Xml
{
bool Pmu::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Pmu, Name::Strict<953> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Pmu::save(QDomElement& dst_) const
{
	Element<Pmu, Name::Strict<953> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Pmu::save(QDomDocument& dst_) const
{
	Element<Pmu, Name::Strict<953> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Pmu>::parse(Domain::Xml::Pmu& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setState(m.get<0>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Pmu>::generate(const Domain::Xml::Pmu& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getState(), m.get<0>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Vmport

namespace Domain
{
namespace Xml
{
bool Vmport::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Vmport, Name::Strict<954> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Vmport::save(QDomElement& dst_) const
{
	Element<Vmport, Name::Strict<954> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Vmport::save(QDomDocument& dst_) const
{
	Element<Vmport, Name::Strict<954> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Vmport>::parse(Domain::Xml::Vmport& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setState(m.get<0>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Vmport>::generate(const Domain::Xml::Vmport& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getState(), m.get<0>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Gic

namespace Domain
{
namespace Xml
{
bool Gic::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Gic, Name::Strict<955> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Gic::save(QDomElement& dst_) const
{
	Element<Gic, Name::Strict<955> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Gic::save(QDomDocument& dst_) const
{
	Element<Gic, Name::Strict<955> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Gic>::parse(Domain::Xml::Gic& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setVersion(m.get<0>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Gic>::generate(const Domain::Xml::Gic& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getVersion(), m.get<0>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Features

namespace Domain
{
namespace Xml
{
Features::Features(): m_pae(), m_acpi(), m_hap(), m_viridian(), m_privnet()
{
}

bool Features::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Features, Name::Strict<144> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Features::save(QDomElement& dst_) const
{
	Element<Features, Name::Strict<144> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Features::save(QDomDocument& dst_) const
{
	Element<Features, Name::Strict<144> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Features>::parse(Domain::Xml::Features& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setPae(m.get<0>().getValue());
		dst_.setApic(m.get<1>().getValue());
		dst_.setAcpi(m.get<2>().getValue());
		dst_.setHap(m.get<3>().getValue());
		dst_.setHyperv(m.get<4>().getValue());
		dst_.setViridian(m.get<5>().getValue());
		dst_.setKvm(m.get<6>().getValue());
		dst_.setPrivnet(m.get<7>().getValue());
		dst_.setPvspinlock(m.get<8>().getValue());
		dst_.setCapabilities(m.get<9>().getValue());
		dst_.setPmu(m.get<10>().getValue());
		dst_.setVmport(m.get<11>().getValue());
		dst_.setGic(m.get<12>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Features>::generate(const Domain::Xml::Features& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getPae(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getApic(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getAcpi(), m.get<2>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getHap(), m.get<3>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getHyperv(), m.get<4>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getViridian(), m.get<5>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getKvm(), m.get<6>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getPrivnet(), m.get<7>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getPvspinlock(), m.get<8>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getCapabilities(), m.get<9>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getPmu(), m.get<10>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getVmport(), m.get<11>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getGic(), m.get<12>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct SuspendToMem

namespace Domain
{
namespace Xml
{
bool SuspendToMem::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<SuspendToMem, Name::Strict<769> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool SuspendToMem::save(QDomElement& dst_) const
{
	Element<SuspendToMem, Name::Strict<769> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool SuspendToMem::save(QDomDocument& dst_) const
{
	Element<SuspendToMem, Name::Strict<769> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::SuspendToMem>::parse(Domain::Xml::SuspendToMem& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setEnabled(m.get<0>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::SuspendToMem>::generate(const Domain::Xml::SuspendToMem& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getEnabled(), m.get<0>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct SuspendToDisk

namespace Domain
{
namespace Xml
{
bool SuspendToDisk::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<SuspendToDisk, Name::Strict<771> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool SuspendToDisk::save(QDomElement& dst_) const
{
	Element<SuspendToDisk, Name::Strict<771> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool SuspendToDisk::save(QDomDocument& dst_) const
{
	Element<SuspendToDisk, Name::Strict<771> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::SuspendToDisk>::parse(Domain::Xml::SuspendToDisk& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setEnabled(m.get<0>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::SuspendToDisk>::generate(const Domain::Xml::SuspendToDisk& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getEnabled(), m.get<0>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Pm

namespace Domain
{
namespace Xml
{
bool Pm::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Pm, Name::Strict<212> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Pm::save(QDomElement& dst_) const
{
	Element<Pm, Name::Strict<212> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Pm::save(QDomDocument& dst_) const
{
	Element<Pm, Name::Strict<212> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Pm>::parse(Domain::Xml::Pm& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setSuspendToMem(m.get<0>().getValue());
		dst_.setSuspendToDisk(m.get<1>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Pm>::generate(const Domain::Xml::Pm& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getSuspendToMem(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getSuspendToDisk(), m.get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Uid

namespace Domain
{
namespace Xml
{
Uid::Uid(): m_start(), m_target(), m_count()
{
}

bool Uid::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Uid, Name::Strict<307> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Uid::save(QDomElement& dst_) const
{
	Element<Uid, Name::Strict<307> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Uid::save(QDomDocument& dst_) const
{
	Element<Uid, Name::Strict<307> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Uid>::parse(Domain::Xml::Uid& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setStart(m.get<0>().getValue());
		dst_.setTarget(m.get<1>().getValue());
		dst_.setCount(m.get<2>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Uid>::generate(const Domain::Xml::Uid& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getStart(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getTarget(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getCount(), m.get<2>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Gid

namespace Domain
{
namespace Xml
{
Gid::Gid(): m_start(), m_target(), m_count()
{
}

bool Gid::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Gid, Name::Strict<311> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Gid::save(QDomElement& dst_) const
{
	Element<Gid, Name::Strict<311> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Gid::save(QDomDocument& dst_) const
{
	Element<Gid, Name::Strict<311> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Gid>::parse(Domain::Xml::Gid& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setStart(m.get<0>().getValue());
		dst_.setTarget(m.get<1>().getValue());
		dst_.setCount(m.get<2>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Gid>::generate(const Domain::Xml::Gid& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getStart(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getTarget(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getCount(), m.get<2>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Idmap

namespace Domain
{
namespace Xml
{
bool Idmap::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Idmap, Name::Strict<213> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Idmap::save(QDomElement& dst_) const
{
	Element<Idmap, Name::Strict<213> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Idmap::save(QDomDocument& dst_) const
{
	Element<Idmap, Name::Strict<213> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Idmap>::parse(Domain::Xml::Idmap& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setUidList(m.get<0>().getValue());
		dst_.setGidList(m.get<1>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Idmap>::generate(const Domain::Xml::Idmap& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getUidList(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getGidList(), m.get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Disk461

namespace Domain
{
namespace Xml
{
Disk461::Disk461(): m_device()
{
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Disk461>::parse(Domain::Xml::Disk461& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setDevice(m.get<0>().getValue());
		dst_.setRawio(m.get<1>().getValue());
		dst_.setSgio(m.get<2>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Disk461>::generate(const Domain::Xml::Disk461& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getDevice(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getRawio(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getSgio(), m.get<2>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Seclabel228

int Traits<Domain::Xml::Seclabel228>::parse(Domain::Xml::Seclabel228& , QStack<QDomElement>& stack_)
{
	marshal_type m;
	return m.consume(stack_);
}

int Traits<Domain::Xml::Seclabel228>::generate(const Domain::Xml::Seclabel228& , QDomElement& dst_)
{
	marshal_type m;
	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Seclabel230

int Traits<Domain::Xml::Seclabel230>::parse(Domain::Xml::Seclabel230& , QStack<QDomElement>& stack_)
{
	marshal_type m;
	return m.consume(stack_);
}

int Traits<Domain::Xml::Seclabel230>::generate(const Domain::Xml::Seclabel230& , QDomElement& dst_)
{
	marshal_type m;
	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Seclabel

namespace Domain
{
namespace Xml
{
bool Seclabel::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Seclabel, Name::Strict<215> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Seclabel::save(QDomElement& dst_) const
{
	Element<Seclabel, Name::Strict<215> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Seclabel::save(QDomDocument& dst_) const
{
	Element<Seclabel, Name::Strict<215> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Seclabel>::parse(Domain::Xml::Seclabel& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setModel(m.get<0>().getValue());
		dst_.setSeclabel(m.get<1>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Seclabel>::generate(const Domain::Xml::Seclabel& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getModel(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getSeclabel(), m.get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Source

namespace Domain
{
namespace Xml
{
bool Source::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Source, Name::Strict<483> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Source::save(QDomElement& dst_) const
{
	Element<Source, Name::Strict<483> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Source::save(QDomDocument& dst_) const
{
	Element<Source, Name::Strict<483> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Source>::parse(Domain::Xml::Source& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setDev(m.get<0>().getValue());
		dst_.setStartupPolicy(m.get<1>().getValue());
		dst_.setSeclabel(m.get<2>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Source>::generate(const Domain::Xml::Source& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getDev(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getStartupPolicy(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getSeclabel(), m.get<2>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Source1

namespace Domain
{
namespace Xml
{
bool Source1::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Source1, Name::Strict<483> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Source1::save(QDomElement& dst_) const
{
	Element<Source1, Name::Strict<483> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Source1::save(QDomDocument& dst_) const
{
	Element<Source1, Name::Strict<483> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Source1>::parse(Domain::Xml::Source1& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setDir(m.get<0>().getValue());
		dst_.setStartupPolicy(m.get<1>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Source1>::generate(const Domain::Xml::Source1& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getDir(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getStartupPolicy(), m.get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Host500

int Traits<Domain::Xml::Host500>::parse(Domain::Xml::Host500& dst_, QStack<QDomElement>& stack_)
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

int Traits<Domain::Xml::Host500>::generate(const Domain::Xml::Host500& src_, QDomElement& dst_)
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
// struct Source2

namespace Domain
{
namespace Xml
{
Source2::Source2(): m_protocol()
{
}

bool Source2::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Source2, Name::Strict<483> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Source2::save(QDomElement& dst_) const
{
	Element<Source2, Name::Strict<483> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Source2::save(QDomDocument& dst_) const
{
	Element<Source2, Name::Strict<483> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Source2>::parse(Domain::Xml::Source2& dst_, QStack<QDomElement>& stack_)
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

int Traits<Domain::Xml::Source2>::generate(const Domain::Xml::Source2& src_, QDomElement& dst_)
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
// struct Source3

namespace Domain
{
namespace Xml
{
bool Source3::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Source3, Name::Strict<483> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Source3::save(QDomElement& dst_) const
{
	Element<Source3, Name::Strict<483> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Source3::save(QDomDocument& dst_) const
{
	Element<Source3, Name::Strict<483> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Source3>::parse(Domain::Xml::Source3& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setPool(m.get<0>().getValue());
		dst_.setVolume(m.get<1>().getValue());
		dst_.setMode(m.get<2>().getValue());
		dst_.setStartupPolicy(m.get<3>().getValue());
		dst_.setSeclabel(m.get<4>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Source3>::generate(const Domain::Xml::Source3& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getPool(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getVolume(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getMode(), m.get<2>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getStartupPolicy(), m.get<3>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getSeclabel(), m.get<4>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Source4

namespace Domain
{
namespace Xml
{
bool Source4::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Source4, Name::Strict<483> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Source4::save(QDomElement& dst_) const
{
	Element<Source4, Name::Strict<483> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Source4::save(QDomDocument& dst_) const
{
	Element<Source4, Name::Strict<483> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Source4>::parse(Domain::Xml::Source4& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setFile(m.get<0>().getValue());
		dst_.setStartupPolicy(m.get<1>().getValue());
		dst_.setSeclabel(m.get<2>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Source4>::generate(const Domain::Xml::Source4& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getFile(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getStartupPolicy(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getSeclabel(), m.get<2>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct DriverFormat

namespace Domain
{
namespace Xml
{
bool DriverFormat::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	k.push(src_.firstChildElement());
	return 0 <= Traits<DriverFormat>::parse(*this, k);
}

bool DriverFormat::save(QDomElement& dst_) const
{
	return 0 <= Traits<DriverFormat>::generate(*this, dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::DriverFormat>::parse(Domain::Xml::DriverFormat& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setName(m.get<0>().getValue());
		dst_.setType(m.get<1>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::DriverFormat>::generate(const Domain::Xml::DriverFormat& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getName(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getType(), m.get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Driver

namespace Domain
{
namespace Xml
{
bool Driver::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Driver, Name::Strict<528> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Driver::save(QDomElement& dst_) const
{
	Element<Driver, Name::Strict<528> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Driver::save(QDomDocument& dst_) const
{
	Element<Driver, Name::Strict<528> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Driver>::parse(Domain::Xml::Driver& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setDriverFormat(m.get<0>().getValue());
		dst_.setCache(m.get<1>().getValue());
		dst_.setErrorPolicy(m.get<2>().getValue());
		dst_.setRerrorPolicy(m.get<3>().getValue());
		dst_.setIo(m.get<4>().getValue());
		dst_.setIoeventfd(m.get<5>().getValue());
		dst_.setEventIdx(m.get<6>().getValue());
		dst_.setCopyOnRead(m.get<7>().getValue());
		dst_.setDiscard(m.get<8>().getValue());
		dst_.setIothread(m.get<9>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Driver>::generate(const Domain::Xml::Driver& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getDriverFormat(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getCache(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getErrorPolicy(), m.get<2>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getRerrorPolicy(), m.get<3>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getIo(), m.get<4>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getIoeventfd(), m.get<5>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getEventIdx(), m.get<6>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getCopyOnRead(), m.get<7>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getDiscard(), m.get<8>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getIothread(), m.get<9>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Anonymous1132

namespace Domain
{
namespace Xml
{
bool Anonymous1132::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	k.push(src_.firstChildElement());
	return 0 <= Traits<Anonymous1132>::parse(*this, k);
}

bool Anonymous1132::save(QDomElement& dst_) const
{
	return 0 <= Traits<Anonymous1132>::generate(*this, dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Anonymous1132>::parse(Domain::Xml::Anonymous1132& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setSource(m.get<0>().get<1>().getValue());
		dst_.setFormat(m.get<1>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Anonymous1132>::generate(const Domain::Xml::Anonymous1132& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getSource(), m.get<0>().get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getFormat(), m.get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Mirror1041

int Traits<Domain::Xml::Mirror1041>::parse(Domain::Xml::Mirror1041& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setFile(m.get<0>().getValue());
		dst_.setFormat(m.get<1>().getValue());
		dst_.setJob(m.get<2>().getValue());
		dst_.setAnonymous1132(m.get<3>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Mirror1041>::generate(const Domain::Xml::Mirror1041& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getFile(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getFormat(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getJob(), m.get<2>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getAnonymous1132(), m.get<3>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Mirror1043

namespace Domain
{
namespace Xml
{
Mirror1043::Mirror1043(): m_job()
{
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Mirror1043>::parse(Domain::Xml::Mirror1043& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setJob(m.get<0>().getValue());
		dst_.setDiskSource(m.get<1>().get<0>().getValue());
		dst_.setFormat(m.get<1>().get<1>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Mirror1043>::generate(const Domain::Xml::Mirror1043& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getJob(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getDiskSource(), m.get<1>().get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getFormat(), m.get<1>().get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Mirror

namespace Domain
{
namespace Xml
{
bool Mirror::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Mirror, Name::Strict<1039> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Mirror::save(QDomElement& dst_) const
{
	Element<Mirror, Name::Strict<1039> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Mirror::save(QDomDocument& dst_) const
{
	Element<Mirror, Name::Strict<1039> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Mirror>::parse(Domain::Xml::Mirror& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setMirror(m.get<0>().getValue());
		dst_.setReady(m.get<1>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Mirror>::generate(const Domain::Xml::Mirror& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getMirror(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getReady(), m.get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Secret

namespace Domain
{
namespace Xml
{
Secret::Secret(): m_type()
{
}

bool Secret::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Secret, Name::Strict<138> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Secret::save(QDomElement& dst_) const
{
	Element<Secret, Name::Strict<138> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Secret::save(QDomDocument& dst_) const
{
	Element<Secret, Name::Strict<138> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Secret>::parse(Domain::Xml::Secret& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setType(m.get<0>().getValue());
		dst_.setSecret(m.get<1>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Secret>::generate(const Domain::Xml::Secret& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getType(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getSecret(), m.get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Auth

namespace Domain
{
namespace Xml
{
bool Auth::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Auth, Name::Strict<1048> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Auth::save(QDomElement& dst_) const
{
	Element<Auth, Name::Strict<1048> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Auth::save(QDomDocument& dst_) const
{
	Element<Auth, Name::Strict<1048> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Auth>::parse(Domain::Xml::Auth& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setUsername(m.get<0>().getValue());
		dst_.setSecret(m.get<1>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Auth>::generate(const Domain::Xml::Auth& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getUsername(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getSecret(), m.get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Target

namespace Domain
{
namespace Xml
{
bool Target::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Target, Name::Strict<309> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Target::save(QDomElement& dst_) const
{
	Element<Target, Name::Strict<309> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Target::save(QDomDocument& dst_) const
{
	Element<Target, Name::Strict<309> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Target>::parse(Domain::Xml::Target& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setDev(m.get<0>().getValue());
		dst_.setBus(m.get<1>().getValue());
		dst_.setTray(m.get<2>().getValue());
		dst_.setRemovable(m.get<3>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Target>::generate(const Domain::Xml::Target& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getDev(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getBus(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getTray(), m.get<2>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getRemovable(), m.get<3>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Secret1

namespace Domain
{
namespace Xml
{
Secret1::Secret1(): m_type()
{
}

bool Secret1::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Secret1, Name::Strict<138> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Secret1::save(QDomElement& dst_) const
{
	Element<Secret1, Name::Strict<138> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Secret1::save(QDomDocument& dst_) const
{
	Element<Secret1, Name::Strict<138> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Secret1>::parse(Domain::Xml::Secret1& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setType(m.get<0>().getValue());
		dst_.setUuid(m.get<1>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Secret1>::generate(const Domain::Xml::Secret1& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getType(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getUuid(), m.get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Encryption

namespace Domain
{
namespace Xml
{
Encryption::Encryption(): m_format()
{
}

bool Encryption::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Encryption, Name::Strict<134> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Encryption::save(QDomElement& dst_) const
{
	Element<Encryption, Name::Strict<134> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Encryption::save(QDomDocument& dst_) const
{
	Element<Encryption, Name::Strict<134> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Encryption>::parse(Domain::Xml::Encryption& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setFormat(m.get<0>().getValue());
		dst_.setSecretList(m.get<1>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Encryption>::generate(const Domain::Xml::Encryption& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getFormat(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getSecretList(), m.get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Variant1056

int Traits<Domain::Xml::Variant1056>::parse(Domain::Xml::Variant1056& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setReadBytesSec(m.get<0>().get<0>().getValue());
		dst_.setWriteBytesSec(m.get<0>().get<1>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Variant1056>::generate(const Domain::Xml::Variant1056& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getReadBytesSec(), m.get<0>().get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getWriteBytesSec(), m.get<0>().get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Variant1060

int Traits<Domain::Xml::Variant1060>::parse(Domain::Xml::Variant1060& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setReadIopsSec(m.get<0>().get<0>().getValue());
		dst_.setWriteIopsSec(m.get<0>().get<1>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Variant1060>::generate(const Domain::Xml::Variant1060& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getReadIopsSec(), m.get<0>().get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getWriteIopsSec(), m.get<0>().get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Iotune

namespace Domain
{
namespace Xml
{
bool Iotune::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Iotune, Name::Strict<1054> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Iotune::save(QDomElement& dst_) const
{
	Element<Iotune, Name::Strict<1054> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Iotune::save(QDomDocument& dst_) const
{
	Element<Iotune, Name::Strict<1054> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Iotune>::parse(Domain::Xml::Iotune& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setChoice1057(m.get<0>().getValue());
		dst_.setChoice1061(m.get<1>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Iotune>::generate(const Domain::Xml::Iotune& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getChoice1057(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getChoice1061(), m.get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Pciaddress

namespace Domain
{
namespace Xml
{
bool Pciaddress::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Pciaddress, Name::Strict<106> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Pciaddress::save(QDomElement& dst_) const
{
	Element<Pciaddress, Name::Strict<106> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Pciaddress::save(QDomDocument& dst_) const
{
	Element<Pciaddress, Name::Strict<106> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Pciaddress>::parse(Domain::Xml::Pciaddress& dst_, QStack<QDomElement>& stack_)
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

int Traits<Domain::Xml::Pciaddress>::generate(const Domain::Xml::Pciaddress& src_, QDomElement& dst_)
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
// struct Driveaddress

namespace Domain
{
namespace Xml
{
bool Driveaddress::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	k.push(src_.firstChildElement());
	return 0 <= Traits<Driveaddress>::parse(*this, k);
}

bool Driveaddress::save(QDomElement& dst_) const
{
	return 0 <= Traits<Driveaddress>::generate(*this, dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Driveaddress>::parse(Domain::Xml::Driveaddress& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setController(m.get<0>().getValue());
		dst_.setBus(m.get<1>().getValue());
		dst_.setTarget(m.get<2>().getValue());
		dst_.setUnit(m.get<3>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Driveaddress>::generate(const Domain::Xml::Driveaddress& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getController(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getBus(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getTarget(), m.get<2>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getUnit(), m.get<3>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Virtioserialaddress

namespace Domain
{
namespace Xml
{
bool Virtioserialaddress::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	k.push(src_.firstChildElement());
	return 0 <= Traits<Virtioserialaddress>::parse(*this, k);
}

bool Virtioserialaddress::save(QDomElement& dst_) const
{
	return 0 <= Traits<Virtioserialaddress>::generate(*this, dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Virtioserialaddress>::parse(Domain::Xml::Virtioserialaddress& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setController(m.get<0>().getValue());
		dst_.setBus(m.get<1>().getValue());
		dst_.setPort(m.get<2>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Virtioserialaddress>::generate(const Domain::Xml::Virtioserialaddress& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getController(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getBus(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getPort(), m.get<2>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Ccidaddress

namespace Domain
{
namespace Xml
{
bool Ccidaddress::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	k.push(src_.firstChildElement());
	return 0 <= Traits<Ccidaddress>::parse(*this, k);
}

bool Ccidaddress::save(QDomElement& dst_) const
{
	return 0 <= Traits<Ccidaddress>::generate(*this, dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Ccidaddress>::parse(Domain::Xml::Ccidaddress& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setController(m.get<0>().getValue());
		dst_.setSlot(m.get<1>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Ccidaddress>::generate(const Domain::Xml::Ccidaddress& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getController(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getSlot(), m.get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Usbportaddress

namespace Domain
{
namespace Xml
{
bool Usbportaddress::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	k.push(src_.firstChildElement());
	return 0 <= Traits<Usbportaddress>::parse(*this, k);
}

bool Usbportaddress::save(QDomElement& dst_) const
{
	return 0 <= Traits<Usbportaddress>::generate(*this, dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Usbportaddress>::parse(Domain::Xml::Usbportaddress& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setBus(m.get<0>().getValue());
		dst_.setPort(m.get<1>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Usbportaddress>::generate(const Domain::Xml::Usbportaddress& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getBus(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getPort(), m.get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Anonymous1133

namespace Domain
{
namespace Xml
{
bool Anonymous1133::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	k.push(src_.firstChildElement());
	return 0 <= Traits<Anonymous1133>::parse(*this, k);
}

bool Anonymous1133::save(QDomElement& dst_) const
{
	return 0 <= Traits<Anonymous1133>::generate(*this, dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Anonymous1133>::parse(Domain::Xml::Anonymous1133& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setCssid(m.get<0>().getValue());
		dst_.setSsid(m.get<1>().getValue());
		dst_.setDevno(m.get<2>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Anonymous1133>::generate(const Domain::Xml::Anonymous1133& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getCssid(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getSsid(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getDevno(), m.get<2>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Isaaddress

namespace Domain
{
namespace Xml
{
bool Isaaddress::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	k.push(src_.firstChildElement());
	return 0 <= Traits<Isaaddress>::parse(*this, k);
}

bool Isaaddress::save(QDomElement& dst_) const
{
	return 0 <= Traits<Isaaddress>::generate(*this, dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Isaaddress>::parse(Domain::Xml::Isaaddress& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setIobase(m.get<0>().getValue());
		dst_.setIrq(m.get<1>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Isaaddress>::generate(const Domain::Xml::Isaaddress& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getIobase(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getIrq(), m.get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Dimmaddress

namespace Domain
{
namespace Xml
{
bool Dimmaddress::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	k.push(src_.firstChildElement());
	return 0 <= Traits<Dimmaddress>::parse(*this, k);
}

bool Dimmaddress::save(QDomElement& dst_) const
{
	return 0 <= Traits<Dimmaddress>::generate(*this, dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Dimmaddress>::parse(Domain::Xml::Dimmaddress& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setSlot(m.get<0>().getValue());
		dst_.setBase(m.get<1>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Dimmaddress>::generate(const Domain::Xml::Dimmaddress& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getSlot(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getBase(), m.get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Geometry

namespace Domain
{
namespace Xml
{
Geometry::Geometry(): m_cyls(), m_heads(), m_secs()
{
}

bool Geometry::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Geometry, Name::Strict<440> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Geometry::save(QDomElement& dst_) const
{
	Element<Geometry, Name::Strict<440> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Geometry::save(QDomDocument& dst_) const
{
	Element<Geometry, Name::Strict<440> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Geometry>::parse(Domain::Xml::Geometry& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setCyls(m.get<0>().getValue());
		dst_.setHeads(m.get<1>().getValue());
		dst_.setSecs(m.get<2>().getValue());
		dst_.setTrans(m.get<3>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Geometry>::generate(const Domain::Xml::Geometry& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getCyls(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getHeads(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getSecs(), m.get<2>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getTrans(), m.get<3>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Blockio

namespace Domain
{
namespace Xml
{
bool Blockio::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Blockio, Name::Strict<525> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Blockio::save(QDomElement& dst_) const
{
	Element<Blockio, Name::Strict<525> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Blockio::save(QDomDocument& dst_) const
{
	Element<Blockio, Name::Strict<525> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Blockio>::parse(Domain::Xml::Blockio& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setLogicalBlockSize(m.get<0>().getValue());
		dst_.setPhysicalBlockSize(m.get<1>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Blockio>::generate(const Domain::Xml::Blockio& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getLogicalBlockSize(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getPhysicalBlockSize(), m.get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct BackingStore

namespace Domain
{
namespace Xml
{
BackingStore::BackingStore(): m_index()
{
}

const VDiskBackingChainBin* BackingStore::getDiskBackingChain() const
{
	if (m_diskBackingChain.empty())
		return NULL;
	
	return boost::any_cast<VDiskBackingChainBin >(&m_diskBackingChain);
}

void BackingStore::setDiskBackingChain(const VDiskBackingChainBin& value_)
{
	m_diskBackingChain = value_;
}

bool BackingStore::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<BackingStore, Name::Strict<469> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool BackingStore::save(QDomElement& dst_) const
{
	Element<BackingStore, Name::Strict<469> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool BackingStore::save(QDomDocument& dst_) const
{
	Element<BackingStore, Name::Strict<469> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::BackingStore>::parse(Domain::Xml::BackingStore& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setIndex(m.get<0>().getValue());
		dst_.setDiskSource(m.get<1>().get<0>().getValue());
		Domain::Xml::VDiskBackingChainBin b;
		b.value = m.get<1>().get<1>().getValue();
		dst_.setDiskBackingChain(b);
		dst_.setFormat(m.get<1>().get<2>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::BackingStore>::generate(const Domain::Xml::BackingStore& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getIndex(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getDiskSource(), m.get<1>().get<0>()))
		return -1;
	const Domain::Xml::VDiskBackingChain* d = NULL;
	const Domain::Xml::VDiskBackingChainBin* v = src_.getDiskBackingChain();
	if (NULL != v)
		d = &v->value;
	if (NULL == d)
		return -1;
	if (0 > Details::Marshal::assign(*d, m.get<1>().get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getFormat(), m.get<1>().get<2>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Disk

namespace Domain
{
namespace Xml
{
Disk::Disk(): m_readonly(), m_shareable(), m_transient()
{
}

bool Disk::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Disk, Name::Strict<454> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Disk::save(QDomElement& dst_) const
{
	Element<Disk, Name::Strict<454> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Disk::save(QDomDocument& dst_) const
{
	Element<Disk, Name::Strict<454> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Disk>::parse(Domain::Xml::Disk& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setDisk(m.get<0>().getValue());
		dst_.setSnapshot(m.get<1>().getValue());
		dst_.setDiskSource(m.get<2>().get<0>().getValue());
		dst_.setDriver(m.get<2>().get<1>().getValue());
		dst_.setMirror(m.get<2>().get<2>().getValue());
		dst_.setAuth(m.get<2>().get<3>().getValue());
		dst_.setTarget(m.get<2>().get<4>().getValue());
		dst_.setBoot(m.get<2>().get<5>().getValue());
		dst_.setReadonly(m.get<2>().get<6>().getValue());
		dst_.setShareable(m.get<2>().get<7>().getValue());
		dst_.setTransient(m.get<2>().get<8>().getValue());
		dst_.setSerial(m.get<2>().get<9>().getValue());
		dst_.setEncryption(m.get<2>().get<10>().getValue());
		dst_.setIotune(m.get<2>().get<11>().getValue());
		dst_.setAlias(m.get<2>().get<12>().getValue());
		dst_.setAddress(m.get<2>().get<13>().getValue());
		dst_.setGeometry(m.get<2>().get<14>().getValue());
		dst_.setBlockio(m.get<2>().get<15>().getValue());
		dst_.setWwn(m.get<2>().get<16>().getValue());
		dst_.setVendor(m.get<2>().get<17>().getValue());
		dst_.setProduct(m.get<2>().get<18>().getValue());
		dst_.setDiskBackingChain(m.get<2>().get<19>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Disk>::generate(const Domain::Xml::Disk& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getDisk(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getSnapshot(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getDiskSource(), m.get<2>().get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getDriver(), m.get<2>().get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getMirror(), m.get<2>().get<2>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getAuth(), m.get<2>().get<3>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getTarget(), m.get<2>().get<4>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getBoot(), m.get<2>().get<5>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getReadonly(), m.get<2>().get<6>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getShareable(), m.get<2>().get<7>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getTransient(), m.get<2>().get<8>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getSerial(), m.get<2>().get<9>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getEncryption(), m.get<2>().get<10>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getIotune(), m.get<2>().get<11>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getAlias(), m.get<2>().get<12>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getAddress(), m.get<2>().get<13>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getGeometry(), m.get<2>().get<14>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getBlockio(), m.get<2>().get<15>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getWwn(), m.get<2>().get<16>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getVendor(), m.get<2>().get<17>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getProduct(), m.get<2>().get<18>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getDiskBackingChain(), m.get<2>().get<19>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Variant575

int Traits<Domain::Xml::Variant575>::parse(Domain::Xml::Variant575& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setModel(m.get<1>().getValue());
		dst_.setMaster(m.get<2>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Variant575>::generate(const Domain::Xml::Variant575& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getModel(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getMaster(), m.get<2>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Variant580

namespace Domain
{
namespace Xml
{
Variant580::Variant580(): m_model()
{
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Variant580>::parse(Domain::Xml::Variant580& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setModel(m.get<0>().getValue());
		dst_.setPcihole64(m.get<1>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Variant580>::generate(const Domain::Xml::Variant580& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getModel(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getPcihole64(), m.get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Variant589

int Traits<Domain::Xml::Variant589>::parse(Domain::Xml::Variant589& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setPorts(m.get<1>().getValue());
		dst_.setVectors(m.get<2>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Variant589>::generate(const Domain::Xml::Variant589& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getPorts(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getVectors(), m.get<2>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Driver1

namespace Domain
{
namespace Xml
{
bool Driver1::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Driver1, Name::Strict<528> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Driver1::save(QDomElement& dst_) const
{
	Element<Driver1, Name::Strict<528> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Driver1::save(QDomDocument& dst_) const
{
	Element<Driver1, Name::Strict<528> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Driver1>::parse(Domain::Xml::Driver1& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setQueues(m.get<0>().getValue());
		dst_.setCmdPerLun(m.get<1>().getValue());
		dst_.setMaxSectors(m.get<2>().getValue());
		dst_.setIoeventfd(m.get<3>().getValue());
		dst_.setIothread(m.get<4>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Driver1>::generate(const Domain::Xml::Driver1& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getQueues(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getCmdPerLun(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getMaxSectors(), m.get<2>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getIoeventfd(), m.get<3>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getIothread(), m.get<4>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Controller

namespace Domain
{
namespace Xml
{
Controller::Controller(): m_index()
{
}

bool Controller::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Controller, Name::Strict<554> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Controller::save(QDomElement& dst_) const
{
	Element<Controller, Name::Strict<554> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Controller::save(QDomDocument& dst_) const
{
	Element<Controller, Name::Strict<554> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Controller>::parse(Domain::Xml::Controller& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setIndex(m.get<0>().getValue());
		dst_.setAlias(m.get<1>().get<0>().getValue());
		dst_.setAddress(m.get<1>().get<1>().getValue());
		dst_.setChoice590(m.get<1>().get<2>().getValue());
		dst_.setDriver(m.get<1>().get<3>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Controller>::generate(const Domain::Xml::Controller& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getIndex(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getAlias(), m.get<1>().get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getAddress(), m.get<1>().get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getChoice590(), m.get<1>().get<2>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getDriver(), m.get<1>().get<3>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Target1

namespace Domain
{
namespace Xml
{
bool Target1::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Target1, Name::Strict<309> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Target1::save(QDomElement& dst_) const
{
	Element<Target1, Name::Strict<309> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Target1::save(QDomDocument& dst_) const
{
	Element<Target1, Name::Strict<309> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Target1>::parse(Domain::Xml::Target1& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setPath(m.get<0>().getValue());
		dst_.setOffset(m.get<1>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Target1>::generate(const Domain::Xml::Target1& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getPath(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getOffset(), m.get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Lease

namespace Domain
{
namespace Xml
{
bool Lease::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Lease, Name::Strict<447> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Lease::save(QDomElement& dst_) const
{
	Element<Lease, Name::Strict<447> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Lease::save(QDomDocument& dst_) const
{
	Element<Lease, Name::Strict<447> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Lease>::parse(Domain::Xml::Lease& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setLockspace(m.get<0>().getValue());
		dst_.setKey(m.get<1>().getValue());
		dst_.setTarget(m.get<2>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Lease>::generate(const Domain::Xml::Lease& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getLockspace(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getKey(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getTarget(), m.get<2>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Driver2

namespace Domain
{
namespace Xml
{
bool Driver2::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Driver2, Name::Strict<528> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Driver2::save(QDomElement& dst_) const
{
	Element<Driver2, Name::Strict<528> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Driver2::save(QDomDocument& dst_) const
{
	Element<Driver2, Name::Strict<528> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Driver2>::parse(Domain::Xml::Driver2& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setType(m.get<0>().getValue());
		dst_.setFormat(m.get<1>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Driver2>::generate(const Domain::Xml::Driver2& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getType(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getFormat(), m.get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Filesystem596

int Traits<Domain::Xml::Filesystem596>::parse(Domain::Xml::Filesystem596& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setDriver(m.get<1>().getValue());
		dst_.setSource(m.get<2>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Filesystem596>::generate(const Domain::Xml::Filesystem596& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getDriver(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getSource(), m.get<2>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Filesystem597

int Traits<Domain::Xml::Filesystem597>::parse(Domain::Xml::Filesystem597& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setDriver(m.get<1>().getValue());
		dst_.setSource(m.get<2>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Filesystem597>::generate(const Domain::Xml::Filesystem597& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getDriver(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getSource(), m.get<2>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Filesystem599

int Traits<Domain::Xml::Filesystem599>::parse(Domain::Xml::Filesystem599& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setDriver(m.get<1>().getValue());
		dst_.setSource(m.get<2>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Filesystem599>::generate(const Domain::Xml::Filesystem599& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getDriver(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getSource(), m.get<2>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Filesystem601

int Traits<Domain::Xml::Filesystem601>::parse(Domain::Xml::Filesystem601& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setDriver(m.get<1>().getValue());
		dst_.setSource(m.get<2>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Filesystem601>::generate(const Domain::Xml::Filesystem601& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getDriver(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getSource(), m.get<2>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Filesystem602

int Traits<Domain::Xml::Filesystem602>::parse(Domain::Xml::Filesystem602& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setDriver(m.get<1>().getValue());
		dst_.setSource(m.get<2>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Filesystem602>::generate(const Domain::Xml::Filesystem602& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getDriver(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getSource(), m.get<2>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Source5

namespace Domain
{
namespace Xml
{
Source5::Source5(): m_usage()
{
}

bool Source5::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Source5, Name::Strict<483> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Source5::save(QDomElement& dst_) const
{
	Element<Source5, Name::Strict<483> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Source5::save(QDomDocument& dst_) const
{
	Element<Source5, Name::Strict<483> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Source5>::parse(Domain::Xml::Source5& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setUsage(m.get<0>().getValue());
		dst_.setUnits(m.get<1>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Source5>::generate(const Domain::Xml::Source5& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getUsage(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getUnits(), m.get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Filesystem606

int Traits<Domain::Xml::Filesystem606>::parse(Domain::Xml::Filesystem606& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setDriver(m.get<1>().getValue());
		dst_.setSource(m.get<2>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Filesystem606>::generate(const Domain::Xml::Filesystem606& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getDriver(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getSource(), m.get<2>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Filesystem

namespace Domain
{
namespace Xml
{
Filesystem::Filesystem(): m_readonly()
{
}

bool Filesystem::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Filesystem, Name::Strict<595> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Filesystem::save(QDomElement& dst_) const
{
	Element<Filesystem, Name::Strict<595> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Filesystem::save(QDomDocument& dst_) const
{
	Element<Filesystem, Name::Strict<595> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Filesystem>::parse(Domain::Xml::Filesystem& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setFilesystem(m.get<0>().getValue());
		dst_.setTarget(m.get<1>().get<0>().getValue());
		dst_.setAccessmode(m.get<1>().get<1>().getValue());
		dst_.setReadonly(m.get<1>().get<2>().getValue());
		dst_.setAlias(m.get<1>().get<3>().getValue());
		dst_.setAddress(m.get<1>().get<4>().getValue());
		dst_.setSpaceHardLimit(m.get<2>().get<0>().getValue());
		dst_.setSpaceSoftLimit(m.get<2>().get<1>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Filesystem>::generate(const Domain::Xml::Filesystem& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getFilesystem(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getTarget(), m.get<1>().get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getAccessmode(), m.get<1>().get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getReadonly(), m.get<1>().get<2>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getAlias(), m.get<1>().get<3>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getAddress(), m.get<1>().get<4>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getSpaceHardLimit(), m.get<2>().get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getSpaceSoftLimit(), m.get<2>().get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Source6

namespace Domain
{
namespace Xml
{
bool Source6::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Source6, Name::Strict<483> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Source6::save(QDomElement& dst_) const
{
	Element<Source6, Name::Strict<483> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Source6::save(QDomDocument& dst_) const
{
	Element<Source6, Name::Strict<483> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Source6>::parse(Domain::Xml::Source6& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setBridge(m.get<0>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Source6>::generate(const Domain::Xml::Source6& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getBridge(), m.get<0>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Parameters

namespace Domain
{
namespace Xml
{
bool Parameters::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Parameters, Name::Strict<169> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Parameters::save(QDomElement& dst_) const
{
	Element<Parameters, Name::Strict<169> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Parameters::save(QDomDocument& dst_) const
{
	Element<Parameters, Name::Strict<169> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Parameters>::parse(Domain::Xml::Parameters& dst_, QStack<QDomElement>& stack_)
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

int Traits<Domain::Xml::Parameters>::generate(const Domain::Xml::Parameters& src_, QDomElement& dst_)
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

namespace Domain
{
namespace Xml
{
bool Parameters1::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Parameters1, Name::Strict<169> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Parameters1::save(QDomElement& dst_) const
{
	Element<Parameters1, Name::Strict<169> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Parameters1::save(QDomDocument& dst_) const
{
	Element<Parameters1, Name::Strict<169> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Parameters1>::parse(Domain::Xml::Parameters1& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setProfileid(m.get<0>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Parameters1>::generate(const Domain::Xml::Parameters1& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getProfileid(), m.get<0>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Parameters2

namespace Domain
{
namespace Xml
{
bool Parameters2::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Parameters2, Name::Strict<169> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Parameters2::save(QDomElement& dst_) const
{
	Element<Parameters2, Name::Strict<169> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Parameters2::save(QDomDocument& dst_) const
{
	Element<Parameters2, Name::Strict<169> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Parameters2>::parse(Domain::Xml::Parameters2& dst_, QStack<QDomElement>& stack_)
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

int Traits<Domain::Xml::Parameters2>::generate(const Domain::Xml::Parameters2& src_, QDomElement& dst_)
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

namespace Domain
{
namespace Xml
{
bool Parameters3::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Parameters3, Name::Strict<169> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Parameters3::save(QDomElement& dst_) const
{
	Element<Parameters3, Name::Strict<169> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Parameters3::save(QDomDocument& dst_) const
{
	Element<Parameters3, Name::Strict<169> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Parameters3>::parse(Domain::Xml::Parameters3& dst_, QStack<QDomElement>& stack_)
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

int Traits<Domain::Xml::Parameters3>::generate(const Domain::Xml::Parameters3& src_, QDomElement& dst_)
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

namespace Domain
{
namespace Xml
{
bool Virtualport::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Virtualport, Name::Strict<167> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Virtualport::save(QDomElement& dst_) const
{
	Element<Virtualport, Name::Strict<167> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Virtualport::save(QDomDocument& dst_) const
{
	Element<Virtualport, Name::Strict<167> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Virtualport>::parse(Domain::Xml::Virtualport& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setParameters(m.get<0>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Virtualport>::generate(const Domain::Xml::Virtualport& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getParameters(), m.get<0>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Guest

namespace Domain
{
namespace Xml
{
bool Guest::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Guest, Name::Strict<390> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Guest::save(QDomElement& dst_) const
{
	Element<Guest, Name::Strict<390> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Guest::save(QDomDocument& dst_) const
{
	Element<Guest, Name::Strict<390> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Guest>::parse(Domain::Xml::Guest& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setDev(m.get<0>().getValue());
		dst_.setActual(m.get<1>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Guest>::generate(const Domain::Xml::Guest& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getDev(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getActual(), m.get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Ip

namespace Domain
{
namespace Xml
{
bool Ip::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Ip, Name::Strict<652> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Ip::save(QDomElement& dst_) const
{
	Element<Ip, Name::Strict<652> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Ip::save(QDomDocument& dst_) const
{
	Element<Ip, Name::Strict<652> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Ip>::parse(Domain::Xml::Ip& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setAddress(m.get<0>().getValue());
		dst_.setFamily(m.get<1>().getValue());
		dst_.setPrefix(m.get<2>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Ip>::generate(const Domain::Xml::Ip& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getAddress(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getFamily(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getPrefix(), m.get<2>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Backend

namespace Domain
{
namespace Xml
{
bool Backend::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Backend, Name::Strict<656> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Backend::save(QDomElement& dst_) const
{
	Element<Backend, Name::Strict<656> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Backend::save(QDomDocument& dst_) const
{
	Element<Backend, Name::Strict<656> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Backend>::parse(Domain::Xml::Backend& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setTap(m.get<0>().getValue());
		dst_.setVhost(m.get<1>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Backend>::generate(const Domain::Xml::Backend& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getTap(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getVhost(), m.get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Driver661

int Traits<Domain::Xml::Driver661>::parse(Domain::Xml::Driver661& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setName(m.get<0>().getValue());
		dst_.setQueues(m.get<1>().getValue());
		dst_.setTxmode(m.get<2>().getValue());
		dst_.setIoeventfd(m.get<3>().getValue());
		dst_.setEventIdx(m.get<4>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Driver661>::generate(const Domain::Xml::Driver661& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getName(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getQueues(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getTxmode(), m.get<2>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getIoeventfd(), m.get<3>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getEventIdx(), m.get<4>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Host

namespace Domain
{
namespace Xml
{
bool Host::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Host, Name::Strict<495> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Host::save(QDomElement& dst_) const
{
	Element<Host, Name::Strict<495> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Host::save(QDomDocument& dst_) const
{
	Element<Host, Name::Strict<495> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Host>::parse(Domain::Xml::Host& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setCsum(m.get<0>().getValue());
		dst_.setGso(m.get<1>().getValue());
		dst_.setTso4(m.get<2>().getValue());
		dst_.setTso6(m.get<3>().getValue());
		dst_.setEcn(m.get<4>().getValue());
		dst_.setUfo(m.get<5>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Host>::generate(const Domain::Xml::Host& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getCsum(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getGso(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getTso4(), m.get<2>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getTso6(), m.get<3>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getEcn(), m.get<4>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getUfo(), m.get<5>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Guest1

namespace Domain
{
namespace Xml
{
bool Guest1::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Guest1, Name::Strict<390> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Guest1::save(QDomElement& dst_) const
{
	Element<Guest1, Name::Strict<390> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Guest1::save(QDomDocument& dst_) const
{
	Element<Guest1, Name::Strict<390> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Guest1>::parse(Domain::Xml::Guest1& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setCsum(m.get<0>().getValue());
		dst_.setTso4(m.get<1>().getValue());
		dst_.setTso6(m.get<2>().getValue());
		dst_.setEcn(m.get<3>().getValue());
		dst_.setUfo(m.get<4>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Guest1>::generate(const Domain::Xml::Guest1& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getCsum(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getTso4(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getTso6(), m.get<2>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getEcn(), m.get<3>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getUfo(), m.get<4>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Driver3

namespace Domain
{
namespace Xml
{
bool Driver3::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Driver3, Name::Strict<528> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Driver3::save(QDomElement& dst_) const
{
	Element<Driver3, Name::Strict<528> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Driver3::save(QDomDocument& dst_) const
{
	Element<Driver3, Name::Strict<528> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Driver3>::parse(Domain::Xml::Driver3& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setDriver(m.get<0>().getValue());
		dst_.setHost(m.get<1>().get<0>().getValue());
		dst_.setGuest(m.get<1>().get<1>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Driver3>::generate(const Domain::Xml::Driver3& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getDriver(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getHost(), m.get<1>().get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getGuest(), m.get<1>().get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Parameter

namespace Domain
{
namespace Xml
{
bool Parameter::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Parameter, Name::Strict<1034> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Parameter::save(QDomElement& dst_) const
{
	Element<Parameter, Name::Strict<1034> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Parameter::save(QDomDocument& dst_) const
{
	Element<Parameter, Name::Strict<1034> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Parameter>::parse(Domain::Xml::Parameter& dst_, QStack<QDomElement>& stack_)
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

int Traits<Domain::Xml::Parameter>::generate(const Domain::Xml::Parameter& src_, QDomElement& dst_)
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

namespace Domain
{
namespace Xml
{
bool FilterrefNodeAttributes::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<FilterrefNodeAttributes, Name::Strict<669> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool FilterrefNodeAttributes::save(QDomElement& dst_) const
{
	Element<FilterrefNodeAttributes, Name::Strict<669> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool FilterrefNodeAttributes::save(QDomDocument& dst_) const
{
	Element<FilterrefNodeAttributes, Name::Strict<669> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::FilterrefNodeAttributes>::parse(Domain::Xml::FilterrefNodeAttributes& dst_, QStack<QDomElement>& stack_)
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

int Traits<Domain::Xml::FilterrefNodeAttributes>::generate(const Domain::Xml::FilterrefNodeAttributes& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getFilter(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getParameterList(), m.get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Tune

namespace Domain
{
namespace Xml
{
bool Tune::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Tune, Name::Strict<671> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Tune::save(QDomElement& dst_) const
{
	Element<Tune, Name::Strict<671> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Tune::save(QDomDocument& dst_) const
{
	Element<Tune, Name::Strict<671> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Tune>::parse(Domain::Xml::Tune& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setSndbuf(m.get<0>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Tune>::generate(const Domain::Xml::Tune& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getSndbuf(), m.get<0>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Rom

namespace Domain
{
namespace Xml
{
bool Rom::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Rom, Name::Strict<261> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Rom::save(QDomElement& dst_) const
{
	Element<Rom, Name::Strict<261> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Rom::save(QDomDocument& dst_) const
{
	Element<Rom, Name::Strict<261> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Rom>::parse(Domain::Xml::Rom& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setBar(m.get<0>().getValue());
		dst_.setFile(m.get<1>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Rom>::generate(const Domain::Xml::Rom& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getBar(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getFile(), m.get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct BandwidthAttributes

namespace Domain
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
	Element<BandwidthAttributes, Name::Strict<185> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool BandwidthAttributes::save(QDomElement& dst_) const
{
	Element<BandwidthAttributes, Name::Strict<185> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool BandwidthAttributes::save(QDomDocument& dst_) const
{
	Element<BandwidthAttributes, Name::Strict<185> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::BandwidthAttributes>::parse(Domain::Xml::BandwidthAttributes& dst_, QStack<QDomElement>& stack_)
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

int Traits<Domain::Xml::BandwidthAttributes>::generate(const Domain::Xml::BandwidthAttributes& src_, QDomElement& dst_)
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

namespace Domain
{
namespace Xml
{
bool Bandwidth::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Bandwidth, Name::Strict<182> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Bandwidth::save(QDomElement& dst_) const
{
	Element<Bandwidth, Name::Strict<182> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Bandwidth::save(QDomDocument& dst_) const
{
	Element<Bandwidth, Name::Strict<182> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Bandwidth>::parse(Domain::Xml::Bandwidth& dst_, QStack<QDomElement>& stack_)
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

int Traits<Domain::Xml::Bandwidth>::generate(const Domain::Xml::Bandwidth& src_, QDomElement& dst_)
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

namespace Domain
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
	Element<Tag, Name::Strict<196> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Tag::save(QDomElement& dst_) const
{
	Element<Tag, Name::Strict<196> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Tag::save(QDomDocument& dst_) const
{
	Element<Tag, Name::Strict<196> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Tag>::parse(Domain::Xml::Tag& dst_, QStack<QDomElement>& stack_)
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

int Traits<Domain::Xml::Tag>::generate(const Domain::Xml::Tag& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getId(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getNativeMode(), m.get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Interface620

int Traits<Domain::Xml::Interface620>::parse(Domain::Xml::Interface620& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setSource(m.get<1>().get<0>().getValue());
		dst_.setVirtualPortProfile(m.get<1>().get<1>().getValue());
		dst_.setLink(m.get<1>().get<2>().getValue());
		dst_.setTarget(m.get<1>().get<3>().getValue());
		dst_.setGuest(m.get<1>().get<4>().getValue());
		dst_.setMac(m.get<1>().get<5>().getValue());
		dst_.setIpList(m.get<1>().get<6>().getValue());
		dst_.setScript(m.get<1>().get<7>().getValue());
		dst_.setModel(m.get<1>().get<8>().getValue());
		dst_.setBackend(m.get<1>().get<9>().getValue());
		dst_.setDriver(m.get<1>().get<10>().getValue());
		dst_.setAlias(m.get<1>().get<11>().getValue());
		dst_.setAddress(m.get<1>().get<12>().getValue());
		dst_.setFilterref(m.get<1>().get<13>().getValue());
		dst_.setTune(m.get<1>().get<14>().getValue());
		dst_.setBoot(m.get<1>().get<15>().getValue());
		dst_.setRom(m.get<1>().get<16>().getValue());
		dst_.setBandwidth(m.get<1>().get<17>().getValue());
		dst_.setVlan(m.get<1>().get<18>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Interface620>::generate(const Domain::Xml::Interface620& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getSource(), m.get<1>().get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getVirtualPortProfile(), m.get<1>().get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getLink(), m.get<1>().get<2>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getTarget(), m.get<1>().get<3>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getGuest(), m.get<1>().get<4>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getMac(), m.get<1>().get<5>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getIpList(), m.get<1>().get<6>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getScript(), m.get<1>().get<7>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getModel(), m.get<1>().get<8>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getBackend(), m.get<1>().get<9>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getDriver(), m.get<1>().get<10>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getAlias(), m.get<1>().get<11>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getAddress(), m.get<1>().get<12>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getFilterref(), m.get<1>().get<13>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getTune(), m.get<1>().get<14>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getBoot(), m.get<1>().get<15>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getRom(), m.get<1>().get<16>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getBandwidth(), m.get<1>().get<17>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getVlan(), m.get<1>().get<18>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Interface622

int Traits<Domain::Xml::Interface622>::parse(Domain::Xml::Interface622& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setSource(m.get<1>().get<0>().getValue());
		dst_.setLink(m.get<1>().get<1>().getValue());
		dst_.setTarget(m.get<1>().get<2>().getValue());
		dst_.setGuest(m.get<1>().get<3>().getValue());
		dst_.setMac(m.get<1>().get<4>().getValue());
		dst_.setIpList(m.get<1>().get<5>().getValue());
		dst_.setScript(m.get<1>().get<6>().getValue());
		dst_.setModel(m.get<1>().get<7>().getValue());
		dst_.setBackend(m.get<1>().get<8>().getValue());
		dst_.setDriver(m.get<1>().get<9>().getValue());
		dst_.setAlias(m.get<1>().get<10>().getValue());
		dst_.setAddress(m.get<1>().get<11>().getValue());
		dst_.setFilterref(m.get<1>().get<12>().getValue());
		dst_.setTune(m.get<1>().get<13>().getValue());
		dst_.setBoot(m.get<1>().get<14>().getValue());
		dst_.setRom(m.get<1>().get<15>().getValue());
		dst_.setBandwidth(m.get<1>().get<16>().getValue());
		dst_.setVlan(m.get<1>().get<17>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Interface622>::generate(const Domain::Xml::Interface622& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getSource(), m.get<1>().get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getLink(), m.get<1>().get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getTarget(), m.get<1>().get<2>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getGuest(), m.get<1>().get<3>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getMac(), m.get<1>().get<4>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getIpList(), m.get<1>().get<5>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getScript(), m.get<1>().get<6>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getModel(), m.get<1>().get<7>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getBackend(), m.get<1>().get<8>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getDriver(), m.get<1>().get<9>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getAlias(), m.get<1>().get<10>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getAddress(), m.get<1>().get<11>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getFilterref(), m.get<1>().get<12>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getTune(), m.get<1>().get<13>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getBoot(), m.get<1>().get<14>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getRom(), m.get<1>().get<15>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getBandwidth(), m.get<1>().get<16>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getVlan(), m.get<1>().get<17>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Source7

namespace Domain
{
namespace Xml
{
Source7::Source7(): m_type(), m_mode()
{
}

bool Source7::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Source7, Name::Strict<483> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Source7::save(QDomElement& dst_) const
{
	Element<Source7, Name::Strict<483> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Source7::save(QDomDocument& dst_) const
{
	Element<Source7, Name::Strict<483> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Source7>::parse(Domain::Xml::Source7& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setType(m.get<0>().getValue());
		dst_.setPath(m.get<1>().getValue());
		dst_.setMode(m.get<2>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Source7>::generate(const Domain::Xml::Source7& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getType(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getPath(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getMode(), m.get<2>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Interface626

int Traits<Domain::Xml::Interface626>::parse(Domain::Xml::Interface626& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setSource(m.get<1>().get<0>().getValue());
		dst_.setLink(m.get<1>().get<1>().getValue());
		dst_.setTarget(m.get<1>().get<2>().getValue());
		dst_.setGuest(m.get<1>().get<3>().getValue());
		dst_.setMac(m.get<1>().get<4>().getValue());
		dst_.setIpList(m.get<1>().get<5>().getValue());
		dst_.setScript(m.get<1>().get<6>().getValue());
		dst_.setModel(m.get<1>().get<7>().getValue());
		dst_.setBackend(m.get<1>().get<8>().getValue());
		dst_.setDriver(m.get<1>().get<9>().getValue());
		dst_.setAlias(m.get<1>().get<10>().getValue());
		dst_.setAddress(m.get<1>().get<11>().getValue());
		dst_.setFilterref(m.get<1>().get<12>().getValue());
		dst_.setTune(m.get<1>().get<13>().getValue());
		dst_.setBoot(m.get<1>().get<14>().getValue());
		dst_.setRom(m.get<1>().get<15>().getValue());
		dst_.setBandwidth(m.get<1>().get<16>().getValue());
		dst_.setVlan(m.get<1>().get<17>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Interface626>::generate(const Domain::Xml::Interface626& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getSource(), m.get<1>().get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getLink(), m.get<1>().get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getTarget(), m.get<1>().get<2>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getGuest(), m.get<1>().get<3>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getMac(), m.get<1>().get<4>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getIpList(), m.get<1>().get<5>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getScript(), m.get<1>().get<6>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getModel(), m.get<1>().get<7>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getBackend(), m.get<1>().get<8>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getDriver(), m.get<1>().get<9>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getAlias(), m.get<1>().get<10>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getAddress(), m.get<1>().get<11>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getFilterref(), m.get<1>().get<12>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getTune(), m.get<1>().get<13>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getBoot(), m.get<1>().get<14>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getRom(), m.get<1>().get<15>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getBandwidth(), m.get<1>().get<16>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getVlan(), m.get<1>().get<17>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Source8

namespace Domain
{
namespace Xml
{
bool Source8::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Source8, Name::Strict<483> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Source8::save(QDomElement& dst_) const
{
	Element<Source8, Name::Strict<483> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Source8::save(QDomDocument& dst_) const
{
	Element<Source8, Name::Strict<483> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Source8>::parse(Domain::Xml::Source8& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setNetwork(m.get<0>().getValue());
		dst_.setPortgroup(m.get<1>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Source8>::generate(const Domain::Xml::Source8& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getNetwork(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getPortgroup(), m.get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Interface628

int Traits<Domain::Xml::Interface628>::parse(Domain::Xml::Interface628& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setSource(m.get<1>().get<0>().getValue());
		dst_.setVirtualPortProfile(m.get<1>().get<1>().getValue());
		dst_.setLink(m.get<1>().get<2>().getValue());
		dst_.setTarget(m.get<1>().get<3>().getValue());
		dst_.setGuest(m.get<1>().get<4>().getValue());
		dst_.setMac(m.get<1>().get<5>().getValue());
		dst_.setIpList(m.get<1>().get<6>().getValue());
		dst_.setScript(m.get<1>().get<7>().getValue());
		dst_.setModel(m.get<1>().get<8>().getValue());
		dst_.setBackend(m.get<1>().get<9>().getValue());
		dst_.setDriver(m.get<1>().get<10>().getValue());
		dst_.setAlias(m.get<1>().get<11>().getValue());
		dst_.setAddress(m.get<1>().get<12>().getValue());
		dst_.setFilterref(m.get<1>().get<13>().getValue());
		dst_.setTune(m.get<1>().get<14>().getValue());
		dst_.setBoot(m.get<1>().get<15>().getValue());
		dst_.setRom(m.get<1>().get<16>().getValue());
		dst_.setBandwidth(m.get<1>().get<17>().getValue());
		dst_.setVlan(m.get<1>().get<18>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Interface628>::generate(const Domain::Xml::Interface628& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getSource(), m.get<1>().get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getVirtualPortProfile(), m.get<1>().get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getLink(), m.get<1>().get<2>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getTarget(), m.get<1>().get<3>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getGuest(), m.get<1>().get<4>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getMac(), m.get<1>().get<5>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getIpList(), m.get<1>().get<6>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getScript(), m.get<1>().get<7>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getModel(), m.get<1>().get<8>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getBackend(), m.get<1>().get<9>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getDriver(), m.get<1>().get<10>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getAlias(), m.get<1>().get<11>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getAddress(), m.get<1>().get<12>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getFilterref(), m.get<1>().get<13>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getTune(), m.get<1>().get<14>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getBoot(), m.get<1>().get<15>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getRom(), m.get<1>().get<16>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getBandwidth(), m.get<1>().get<17>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getVlan(), m.get<1>().get<18>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Source9

namespace Domain
{
namespace Xml
{
bool Source9::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Source9, Name::Strict<483> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Source9::save(QDomElement& dst_) const
{
	Element<Source9, Name::Strict<483> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Source9::save(QDomDocument& dst_) const
{
	Element<Source9, Name::Strict<483> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Source9>::parse(Domain::Xml::Source9& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setDev(m.get<0>().getValue());
		dst_.setMode(m.get<1>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Source9>::generate(const Domain::Xml::Source9& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getDev(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getMode(), m.get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Interface630

int Traits<Domain::Xml::Interface630>::parse(Domain::Xml::Interface630& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setSource(m.get<1>().get<0>().getValue());
		dst_.setVirtualPortProfile(m.get<1>().get<1>().getValue());
		dst_.setLink(m.get<1>().get<2>().getValue());
		dst_.setTarget(m.get<1>().get<3>().getValue());
		dst_.setGuest(m.get<1>().get<4>().getValue());
		dst_.setMac(m.get<1>().get<5>().getValue());
		dst_.setIpList(m.get<1>().get<6>().getValue());
		dst_.setScript(m.get<1>().get<7>().getValue());
		dst_.setModel(m.get<1>().get<8>().getValue());
		dst_.setBackend(m.get<1>().get<9>().getValue());
		dst_.setDriver(m.get<1>().get<10>().getValue());
		dst_.setAlias(m.get<1>().get<11>().getValue());
		dst_.setAddress(m.get<1>().get<12>().getValue());
		dst_.setFilterref(m.get<1>().get<13>().getValue());
		dst_.setTune(m.get<1>().get<14>().getValue());
		dst_.setBoot(m.get<1>().get<15>().getValue());
		dst_.setRom(m.get<1>().get<16>().getValue());
		dst_.setBandwidth(m.get<1>().get<17>().getValue());
		dst_.setVlan(m.get<1>().get<18>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Interface630>::generate(const Domain::Xml::Interface630& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getSource(), m.get<1>().get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getVirtualPortProfile(), m.get<1>().get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getLink(), m.get<1>().get<2>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getTarget(), m.get<1>().get<3>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getGuest(), m.get<1>().get<4>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getMac(), m.get<1>().get<5>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getIpList(), m.get<1>().get<6>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getScript(), m.get<1>().get<7>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getModel(), m.get<1>().get<8>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getBackend(), m.get<1>().get<9>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getDriver(), m.get<1>().get<10>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getAlias(), m.get<1>().get<11>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getAddress(), m.get<1>().get<12>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getFilterref(), m.get<1>().get<13>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getTune(), m.get<1>().get<14>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getBoot(), m.get<1>().get<15>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getRom(), m.get<1>().get<16>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getBandwidth(), m.get<1>().get<17>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getVlan(), m.get<1>().get<18>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct InterfaceOptions

namespace Domain
{
namespace Xml
{
bool InterfaceOptions::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	k.push(src_.firstChildElement());
	return 0 <= Traits<InterfaceOptions>::parse(*this, k);
}

bool InterfaceOptions::save(QDomElement& dst_) const
{
	return 0 <= Traits<InterfaceOptions>::generate(*this, dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::InterfaceOptions>::parse(Domain::Xml::InterfaceOptions& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setLink(m.get<0>().getValue());
		dst_.setTarget(m.get<1>().getValue());
		dst_.setGuest(m.get<2>().getValue());
		dst_.setMac(m.get<3>().getValue());
		dst_.setIpList(m.get<4>().getValue());
		dst_.setScript(m.get<5>().getValue());
		dst_.setModel(m.get<6>().getValue());
		dst_.setBackend(m.get<7>().getValue());
		dst_.setDriver(m.get<8>().getValue());
		dst_.setAlias(m.get<9>().getValue());
		dst_.setAddress(m.get<10>().getValue());
		dst_.setFilterref(m.get<11>().getValue());
		dst_.setTune(m.get<12>().getValue());
		dst_.setBoot(m.get<13>().getValue());
		dst_.setRom(m.get<14>().getValue());
		dst_.setBandwidth(m.get<15>().getValue());
		dst_.setVlan(m.get<16>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::InterfaceOptions>::generate(const Domain::Xml::InterfaceOptions& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getLink(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getTarget(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getGuest(), m.get<2>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getMac(), m.get<3>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getIpList(), m.get<4>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getScript(), m.get<5>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getModel(), m.get<6>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getBackend(), m.get<7>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getDriver(), m.get<8>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getAlias(), m.get<9>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getAddress(), m.get<10>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getFilterref(), m.get<11>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getTune(), m.get<12>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getBoot(), m.get<13>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getRom(), m.get<14>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getBandwidth(), m.get<15>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getVlan(), m.get<16>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Interface633

int Traits<Domain::Xml::Interface633>::parse(Domain::Xml::Interface633& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setSource(m.get<1>().get<0>().getValue());
		dst_.setLink(m.get<1>().get<1>().getValue());
		dst_.setTarget(m.get<1>().get<2>().getValue());
		dst_.setGuest(m.get<1>().get<3>().getValue());
		dst_.setMac(m.get<1>().get<4>().getValue());
		dst_.setIpList(m.get<1>().get<5>().getValue());
		dst_.setScript(m.get<1>().get<6>().getValue());
		dst_.setModel(m.get<1>().get<7>().getValue());
		dst_.setBackend(m.get<1>().get<8>().getValue());
		dst_.setDriver(m.get<1>().get<9>().getValue());
		dst_.setAlias(m.get<1>().get<10>().getValue());
		dst_.setAddress(m.get<1>().get<11>().getValue());
		dst_.setFilterref(m.get<1>().get<12>().getValue());
		dst_.setTune(m.get<1>().get<13>().getValue());
		dst_.setBoot(m.get<1>().get<14>().getValue());
		dst_.setRom(m.get<1>().get<15>().getValue());
		dst_.setBandwidth(m.get<1>().get<16>().getValue());
		dst_.setVlan(m.get<1>().get<17>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Interface633>::generate(const Domain::Xml::Interface633& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getSource(), m.get<1>().get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getLink(), m.get<1>().get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getTarget(), m.get<1>().get<2>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getGuest(), m.get<1>().get<3>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getMac(), m.get<1>().get<4>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getIpList(), m.get<1>().get<5>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getScript(), m.get<1>().get<6>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getModel(), m.get<1>().get<7>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getBackend(), m.get<1>().get<8>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getDriver(), m.get<1>().get<9>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getAlias(), m.get<1>().get<10>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getAddress(), m.get<1>().get<11>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getFilterref(), m.get<1>().get<12>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getTune(), m.get<1>().get<13>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getBoot(), m.get<1>().get<14>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getRom(), m.get<1>().get<15>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getBandwidth(), m.get<1>().get<16>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getVlan(), m.get<1>().get<17>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Source10

namespace Domain
{
namespace Xml
{
Source10::Source10(): m_port()
{
}

bool Source10::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Source10, Name::Strict<483> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Source10::save(QDomElement& dst_) const
{
	Element<Source10, Name::Strict<483> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Source10::save(QDomDocument& dst_) const
{
	Element<Source10, Name::Strict<483> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Source10>::parse(Domain::Xml::Source10& dst_, QStack<QDomElement>& stack_)
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

int Traits<Domain::Xml::Source10>::generate(const Domain::Xml::Source10& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getAddress(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getPort(), m.get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Interface636

namespace Domain
{
namespace Xml
{
Interface636::Interface636(): m_type()
{
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Interface636>::parse(Domain::Xml::Interface636& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setType(m.get<0>().getValue());
		dst_.setSource(m.get<1>().get<0>().getValue());
		dst_.setMac(m.get<1>().get<1>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Interface636>::generate(const Domain::Xml::Interface636& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getType(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getSource(), m.get<1>().get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getMac(), m.get<1>().get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Source11

namespace Domain
{
namespace Xml
{
Source11::Source11(): m_port()
{
}

bool Source11::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Source11, Name::Strict<483> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Source11::save(QDomElement& dst_) const
{
	Element<Source11, Name::Strict<483> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Source11::save(QDomDocument& dst_) const
{
	Element<Source11, Name::Strict<483> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Source11>::parse(Domain::Xml::Source11& dst_, QStack<QDomElement>& stack_)
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

int Traits<Domain::Xml::Source11>::generate(const Domain::Xml::Source11& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getAddress(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getPort(), m.get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Interface637

int Traits<Domain::Xml::Interface637>::parse(Domain::Xml::Interface637& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setSource(m.get<1>().get<0>().getValue());
		dst_.setMac(m.get<1>().get<1>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Interface637>::generate(const Domain::Xml::Interface637& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getSource(), m.get<1>().get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getMac(), m.get<1>().get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Usbproduct

namespace Domain
{
namespace Xml
{
bool Usbproduct::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	k.push(src_.firstChildElement());
	return 0 <= Traits<Usbproduct>::parse(*this, k);
}

bool Usbproduct::save(QDomElement& dst_) const
{
	return 0 <= Traits<Usbproduct>::generate(*this, dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Usbproduct>::parse(Domain::Xml::Usbproduct& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setVendor(m.get<0>().getValue());
		dst_.setProduct(m.get<1>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Usbproduct>::generate(const Domain::Xml::Usbproduct& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getVendor(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getProduct(), m.get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Address

namespace Domain
{
namespace Xml
{
bool Address::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Address, Name::Strict<106> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Address::save(QDomElement& dst_) const
{
	Element<Address, Name::Strict<106> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Address::save(QDomDocument& dst_) const
{
	Element<Address, Name::Strict<106> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Address>::parse(Domain::Xml::Address& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setBus(m.get<0>().getValue());
		dst_.setDevice(m.get<1>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Address>::generate(const Domain::Xml::Address& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getBus(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getDevice(), m.get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Source643

int Traits<Domain::Xml::Source643>::parse(Domain::Xml::Source643& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setUsbproduct(m.get<0>().getValue());
		dst_.setAddress(m.get<1>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Source643>::generate(const Domain::Xml::Source643& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getUsbproduct(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getAddress(), m.get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Address647

int Traits<Domain::Xml::Address647>::parse(Domain::Xml::Address647& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setBus(m.get<1>().getValue());
		dst_.setDevice(m.get<2>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Address647>::generate(const Domain::Xml::Address647& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getBus(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getDevice(), m.get<2>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Source12

namespace Domain
{
namespace Xml
{
bool Source12::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Source12, Name::Strict<483> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Source12::save(QDomElement& dst_) const
{
	Element<Source12, Name::Strict<483> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Source12::save(QDomDocument& dst_) const
{
	Element<Source12, Name::Strict<483> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Source12>::parse(Domain::Xml::Source12& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setMissing(m.get<0>().getValue());
		dst_.setSource(m.get<1>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Source12>::generate(const Domain::Xml::Source12& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getMissing(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getSource(), m.get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Interface649

int Traits<Domain::Xml::Interface649>::parse(Domain::Xml::Interface649& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setManaged(m.get<1>().getValue());
		dst_.setSource(m.get<2>().get<0>().getValue());
		dst_.setVirtualPortProfile(m.get<2>().get<1>().getValue());
		dst_.setLink(m.get<2>().get<2>().getValue());
		dst_.setTarget(m.get<2>().get<3>().getValue());
		dst_.setGuest(m.get<2>().get<4>().getValue());
		dst_.setMac(m.get<2>().get<5>().getValue());
		dst_.setIpList(m.get<2>().get<6>().getValue());
		dst_.setScript(m.get<2>().get<7>().getValue());
		dst_.setModel(m.get<2>().get<8>().getValue());
		dst_.setBackend(m.get<2>().get<9>().getValue());
		dst_.setDriver(m.get<2>().get<10>().getValue());
		dst_.setAlias(m.get<2>().get<11>().getValue());
		dst_.setAddress(m.get<2>().get<12>().getValue());
		dst_.setFilterref(m.get<2>().get<13>().getValue());
		dst_.setTune(m.get<2>().get<14>().getValue());
		dst_.setBoot(m.get<2>().get<15>().getValue());
		dst_.setRom(m.get<2>().get<16>().getValue());
		dst_.setBandwidth(m.get<2>().get<17>().getValue());
		dst_.setVlan(m.get<2>().get<18>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Interface649>::generate(const Domain::Xml::Interface649& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getManaged(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getSource(), m.get<2>().get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getVirtualPortProfile(), m.get<2>().get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getLink(), m.get<2>().get<2>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getTarget(), m.get<2>().get<3>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getGuest(), m.get<2>().get<4>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getMac(), m.get<2>().get<5>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getIpList(), m.get<2>().get<6>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getScript(), m.get<2>().get<7>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getModel(), m.get<2>().get<8>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getBackend(), m.get<2>().get<9>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getDriver(), m.get<2>().get<10>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getAlias(), m.get<2>().get<11>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getAddress(), m.get<2>().get<12>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getFilterref(), m.get<2>().get<13>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getTune(), m.get<2>().get<14>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getBoot(), m.get<2>().get<15>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getRom(), m.get<2>().get<16>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getBandwidth(), m.get<2>().get<17>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getVlan(), m.get<2>().get<18>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Input

namespace Domain
{
namespace Xml
{
Input::Input(): m_type()
{
}

bool Input::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Input, Name::Strict<864> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Input::save(QDomElement& dst_) const
{
	Element<Input, Name::Strict<864> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Input::save(QDomDocument& dst_) const
{
	Element<Input, Name::Strict<864> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Input>::parse(Domain::Xml::Input& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setType(m.get<0>().getValue());
		dst_.setBus(m.get<1>().getValue());
		dst_.setAlias(m.get<2>().getValue());
		dst_.setAddress(m.get<3>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Input>::generate(const Domain::Xml::Input& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getType(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getBus(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getAlias(), m.get<2>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getAddress(), m.get<3>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Sound

namespace Domain
{
namespace Xml
{
Sound::Sound(): m_model()
{
}

bool Sound::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Sound, Name::Strict<829> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Sound::save(QDomElement& dst_) const
{
	Element<Sound, Name::Strict<829> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Sound::save(QDomDocument& dst_) const
{
	Element<Sound, Name::Strict<829> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Sound>::parse(Domain::Xml::Sound& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setModel(m.get<0>().getValue());
		dst_.setAlias(m.get<1>().get<0>().getValue());
		dst_.setAddress(m.get<1>().get<1>().getValue());
		dst_.setCodecList(m.get<1>().get<2>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Sound>::generate(const Domain::Xml::Sound& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getModel(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getAlias(), m.get<1>().get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getAddress(), m.get<1>().get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getCodecList(), m.get<1>().get<2>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Source13

namespace Domain
{
namespace Xml
{
bool Source13::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Source13, Name::Strict<483> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Source13::save(QDomElement& dst_) const
{
	Element<Source13, Name::Strict<483> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Source13::save(QDomDocument& dst_) const
{
	Element<Source13, Name::Strict<483> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Source13>::parse(Domain::Xml::Source13& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setStartupPolicy(m.get<0>().getValue());
		dst_.setAddress(m.get<1>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Source13>::generate(const Domain::Xml::Source13& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getStartupPolicy(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getAddress(), m.get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Hostdevsubsyspci

int Traits<Domain::Xml::Hostdevsubsyspci>::parse(Domain::Xml::Hostdevsubsyspci& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setDriver(m.get<1>().get<0>().getValue());
		dst_.setSource(m.get<1>().get<1>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Hostdevsubsyspci>::generate(const Domain::Xml::Hostdevsubsyspci& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getDriver(), m.get<1>().get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getSource(), m.get<1>().get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Source889

int Traits<Domain::Xml::Source889>::parse(Domain::Xml::Source889& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setUsbproduct(m.get<0>().getValue());
		dst_.setAddress(m.get<1>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Source889>::generate(const Domain::Xml::Source889& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getUsbproduct(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getAddress(), m.get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Source14

namespace Domain
{
namespace Xml
{
bool Source14::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Source14, Name::Strict<483> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Source14::save(QDomElement& dst_) const
{
	Element<Source14, Name::Strict<483> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Source14::save(QDomDocument& dst_) const
{
	Element<Source14, Name::Strict<483> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Source14>::parse(Domain::Xml::Source14& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setStartupPolicy(m.get<0>().getValue());
		dst_.setSource(m.get<1>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Source14>::generate(const Domain::Xml::Source14& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getStartupPolicy(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getSource(), m.get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Parentaddr

namespace Domain
{
namespace Xml
{
bool Parentaddr::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Parentaddr, Name::Strict<104> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Parentaddr::save(QDomElement& dst_) const
{
	Element<Parentaddr, Name::Strict<104> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Parentaddr::save(QDomDocument& dst_) const
{
	Element<Parentaddr, Name::Strict<104> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Parentaddr>::parse(Domain::Xml::Parentaddr& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setUniqueId(m.get<0>().getValue());
		dst_.setAddress(m.get<1>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Parentaddr>::generate(const Domain::Xml::Parentaddr& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getUniqueId(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getAddress(), m.get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Adapter113

int Traits<Domain::Xml::Adapter113>::parse(Domain::Xml::Adapter113& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setParent(m.get<1>().getValue());
		dst_.setWwnn(m.get<2>().getValue());
		dst_.setWwpn(m.get<3>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Adapter113>::generate(const Domain::Xml::Adapter113& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getParent(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getWwnn(), m.get<2>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getWwpn(), m.get<3>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Scsiaddress

namespace Domain
{
namespace Xml
{
bool Scsiaddress::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Scsiaddress, Name::Strict<106> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Scsiaddress::save(QDomElement& dst_) const
{
	Element<Scsiaddress, Name::Strict<106> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Scsiaddress::save(QDomDocument& dst_) const
{
	Element<Scsiaddress, Name::Strict<106> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Scsiaddress>::parse(Domain::Xml::Scsiaddress& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setBus(m.get<0>().getValue());
		dst_.setTarget(m.get<1>().getValue());
		dst_.setUnit(m.get<2>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Scsiaddress>::generate(const Domain::Xml::Scsiaddress& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getBus(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getTarget(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getUnit(), m.get<2>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Source892

int Traits<Domain::Xml::Source892>::parse(Domain::Xml::Source892& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setProtocol(m.get<0>().getValue());
		dst_.setAdapter(m.get<1>().get<0>().getValue());
		dst_.setAddress(m.get<1>().get<1>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Source892>::generate(const Domain::Xml::Source892& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getProtocol(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getAdapter(), m.get<1>().get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getAddress(), m.get<1>().get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Host1

namespace Domain
{
namespace Xml
{
bool Host1::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Host1, Name::Strict<495> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Host1::save(QDomElement& dst_) const
{
	Element<Host1, Name::Strict<495> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Host1::save(QDomDocument& dst_) const
{
	Element<Host1, Name::Strict<495> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Host1>::parse(Domain::Xml::Host1& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setName(m.get<0>().getValue());
		dst_.setPort(m.get<1>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Host1>::generate(const Domain::Xml::Host1& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getName(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getPort(), m.get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Source893

namespace Domain
{
namespace Xml
{
Source893::Source893(): m_protocol()
{
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Source893>::parse(Domain::Xml::Source893& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setProtocol(m.get<0>().getValue());
		dst_.setName(m.get<1>().getValue());
		dst_.setHostList(m.get<2>().get<0>().getValue());
		dst_.setAuth(m.get<2>().get<1>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Source893>::generate(const Domain::Xml::Source893& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getProtocol(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getName(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getHostList(), m.get<2>().get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getAuth(), m.get<2>().get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Hostdevsubsysscsi

int Traits<Domain::Xml::Hostdevsubsysscsi>::parse(Domain::Xml::Hostdevsubsysscsi& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setSgio(m.get<1>().getValue());
		dst_.setRawio(m.get<2>().getValue());
		dst_.setSource(m.get<3>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Hostdevsubsysscsi>::generate(const Domain::Xml::Hostdevsubsysscsi& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getSgio(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getRawio(), m.get<2>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getSource(), m.get<3>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Hostdevsubsys

namespace Domain
{
namespace Xml
{
bool Hostdevsubsys::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	k.push(src_.firstChildElement());
	return 0 <= Traits<Hostdevsubsys>::parse(*this, k);
}

bool Hostdevsubsys::save(QDomElement& dst_) const
{
	return 0 <= Traits<Hostdevsubsys>::generate(*this, dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Hostdevsubsys>::parse(Domain::Xml::Hostdevsubsys& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setManaged(m.get<1>().getValue());
		dst_.setHostdevsubsys(m.get<2>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Hostdevsubsys>::generate(const Domain::Xml::Hostdevsubsys& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getManaged(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getHostdevsubsys(), m.get<2>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Hostdev

namespace Domain
{
namespace Xml
{
Hostdev::Hostdev(): m_readonly(), m_shareable()
{
}

bool Hostdev::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Hostdev, Name::Strict<639> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Hostdev::save(QDomElement& dst_) const
{
	Element<Hostdev, Name::Strict<639> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Hostdev::save(QDomDocument& dst_) const
{
	Element<Hostdev, Name::Strict<639> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Hostdev>::parse(Domain::Xml::Hostdev& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setChoice874(m.get<0>().getValue());
		dst_.setAlias(m.get<1>().getValue());
		dst_.setBoot(m.get<2>().getValue());
		dst_.setRom(m.get<3>().getValue());
		dst_.setAddress(m.get<4>().getValue());
		dst_.setReadonly(m.get<5>().getValue());
		dst_.setShareable(m.get<6>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Hostdev>::generate(const Domain::Xml::Hostdev& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getChoice874(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getAlias(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getBoot(), m.get<2>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getRom(), m.get<3>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getAddress(), m.get<4>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getReadonly(), m.get<5>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getShareable(), m.get<6>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Graphics679

int Traits<Domain::Xml::Graphics679>::parse(Domain::Xml::Graphics679& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setDisplay(m.get<1>().getValue());
		dst_.setXauth(m.get<2>().getValue());
		dst_.setFullscreen(m.get<3>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Graphics679>::generate(const Domain::Xml::Graphics679& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getDisplay(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getXauth(), m.get<2>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getFullscreen(), m.get<3>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Variant688

int Traits<Domain::Xml::Variant688>::parse(Domain::Xml::Variant688& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setPort(m.get<0>().getValue());
		dst_.setAutoport(m.get<1>().getValue());
		dst_.setWebsocket(m.get<2>().getValue());
		dst_.setListen(m.get<3>().getValue());
		dst_.setSharePolicy(m.get<4>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Variant688>::generate(const Domain::Xml::Variant688& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getPort(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getAutoport(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getWebsocket(), m.get<2>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getListen(), m.get<3>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getSharePolicy(), m.get<4>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Listen740

int Traits<Domain::Xml::Listen740>::parse(Domain::Xml::Listen740& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setNetwork(m.get<1>().getValue());
		dst_.setAddress(m.get<2>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Listen740>::generate(const Domain::Xml::Listen740& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getNetwork(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getAddress(), m.get<2>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Graphics697

int Traits<Domain::Xml::Graphics697>::parse(Domain::Xml::Graphics697& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setChoice690(m.get<1>().getValue());
		dst_.setPasswd(m.get<2>().getValue());
		dst_.setKeymap(m.get<3>().getValue());
		dst_.setPasswdValidTo(m.get<4>().getValue());
		dst_.setConnected(m.get<5>().getValue());
		dst_.setListenList(m.get<6>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Graphics697>::generate(const Domain::Xml::Graphics697& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getChoice690(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getPasswd(), m.get<2>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getKeymap(), m.get<3>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getPasswdValidTo(), m.get<4>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getConnected(), m.get<5>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getListenList(), m.get<6>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Channel

namespace Domain
{
namespace Xml
{
Channel::Channel(): m_name(), m_mode()
{
}

bool Channel::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Channel, Name::Strict<707> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Channel::save(QDomElement& dst_) const
{
	Element<Channel, Name::Strict<707> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Channel::save(QDomDocument& dst_) const
{
	Element<Channel, Name::Strict<707> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Channel>::parse(Domain::Xml::Channel& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setName(m.get<0>().getValue());
		dst_.setMode(m.get<1>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Channel>::generate(const Domain::Xml::Channel& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getName(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getMode(), m.get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Graphics732

int Traits<Domain::Xml::Graphics732>::parse(Domain::Xml::Graphics732& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setPort(m.get<1>().getValue());
		dst_.setTlsPort(m.get<2>().getValue());
		dst_.setAutoport(m.get<3>().getValue());
		dst_.setListen(m.get<4>().getValue());
		dst_.setPasswd(m.get<5>().getValue());
		dst_.setKeymap(m.get<6>().getValue());
		dst_.setPasswdValidTo(m.get<7>().getValue());
		dst_.setConnected(m.get<8>().getValue());
		dst_.setDefaultMode(m.get<9>().getValue());
		dst_.setListenList(m.get<10>().get<0>().getValue());
		dst_.setChannelList(m.get<10>().get<1>().getValue());
		dst_.setImage(m.get<10>().get<2>().getValue());
		dst_.setJpeg(m.get<10>().get<3>().getValue());
		dst_.setZlib(m.get<10>().get<4>().getValue());
		dst_.setPlayback(m.get<10>().get<5>().getValue());
		dst_.setStreaming(m.get<10>().get<6>().getValue());
		dst_.setClipboard(m.get<10>().get<7>().getValue());
		dst_.setMouse(m.get<10>().get<8>().getValue());
		dst_.setFiletransfer(m.get<10>().get<9>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Graphics732>::generate(const Domain::Xml::Graphics732& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getPort(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getTlsPort(), m.get<2>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getAutoport(), m.get<3>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getListen(), m.get<4>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getPasswd(), m.get<5>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getKeymap(), m.get<6>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getPasswdValidTo(), m.get<7>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getConnected(), m.get<8>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getDefaultMode(), m.get<9>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getListenList(), m.get<10>().get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getChannelList(), m.get<10>().get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getImage(), m.get<10>().get<2>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getJpeg(), m.get<10>().get<3>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getZlib(), m.get<10>().get<4>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getPlayback(), m.get<10>().get<5>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getStreaming(), m.get<10>().get<6>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getClipboard(), m.get<10>().get<7>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getMouse(), m.get<10>().get<8>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getFiletransfer(), m.get<10>().get<9>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Graphics736

int Traits<Domain::Xml::Graphics736>::parse(Domain::Xml::Graphics736& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setPort(m.get<1>().getValue());
		dst_.setAutoport(m.get<2>().getValue());
		dst_.setReplaceUser(m.get<3>().getValue());
		dst_.setMultiUser(m.get<4>().getValue());
		dst_.setListen(m.get<5>().getValue());
		dst_.setListenList(m.get<6>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Graphics736>::generate(const Domain::Xml::Graphics736& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getPort(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getAutoport(), m.get<2>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getReplaceUser(), m.get<3>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getMultiUser(), m.get<4>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getListen(), m.get<5>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getListenList(), m.get<6>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Graphics738

int Traits<Domain::Xml::Graphics738>::parse(Domain::Xml::Graphics738& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setDisplay(m.get<1>().getValue());
		dst_.setFullscreen(m.get<2>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Graphics738>::generate(const Domain::Xml::Graphics738& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getDisplay(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getFullscreen(), m.get<2>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Acceleration

namespace Domain
{
namespace Xml
{
bool Acceleration::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Acceleration, Name::Strict<751> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Acceleration::save(QDomElement& dst_) const
{
	Element<Acceleration, Name::Strict<751> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Acceleration::save(QDomDocument& dst_) const
{
	Element<Acceleration, Name::Strict<751> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Acceleration>::parse(Domain::Xml::Acceleration& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setAccel3d(m.get<0>().getValue());
		dst_.setAccel2d(m.get<1>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Acceleration>::generate(const Domain::Xml::Acceleration& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getAccel3d(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getAccel2d(), m.get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Model1

namespace Domain
{
namespace Xml
{
bool Model1::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Model1, Name::Strict<217> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Model1::save(QDomElement& dst_) const
{
	Element<Model1, Name::Strict<217> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Model1::save(QDomDocument& dst_) const
{
	Element<Model1, Name::Strict<217> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Model1>::parse(Domain::Xml::Model1& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setModel(m.get<0>().getValue());
		dst_.setVram(m.get<1>().getValue());
		dst_.setHeads(m.get<2>().getValue());
		dst_.setPrimary(m.get<3>().getValue());
		dst_.setAcceleration(m.get<4>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Model1>::generate(const Domain::Xml::Model1& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getModel(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getVram(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getHeads(), m.get<2>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getPrimary(), m.get<3>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getAcceleration(), m.get<4>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Video

namespace Domain
{
namespace Xml
{
bool Video::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Video, Name::Strict<742> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Video::save(QDomElement& dst_) const
{
	Element<Video, Name::Strict<742> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Video::save(QDomDocument& dst_) const
{
	Element<Video, Name::Strict<742> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Video>::parse(Domain::Xml::Video& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setModel(m.get<0>().getValue());
		dst_.setAlias(m.get<1>().getValue());
		dst_.setAddress(m.get<2>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Video>::generate(const Domain::Xml::Video& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getModel(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getAlias(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getAddress(), m.get<2>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Source15

namespace Domain
{
namespace Xml
{
bool Source15::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Source15, Name::Strict<483> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Source15::save(QDomElement& dst_) const
{
	Element<Source15, Name::Strict<483> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Source15::save(QDomDocument& dst_) const
{
	Element<Source15, Name::Strict<483> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Source15>::parse(Domain::Xml::Source15& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setMode(m.get<0>().getValue());
		dst_.setPath(m.get<1>().getValue());
		dst_.setHost(m.get<2>().getValue());
		dst_.setService(m.get<3>().getValue());
		dst_.setWiremode(m.get<4>().getValue());
		dst_.setChannel(m.get<5>().getValue());
		dst_.setMaster(m.get<6>().getValue());
		dst_.setSlave(m.get<7>().getValue());
		dst_.setAppend(m.get<8>().getValue());
		dst_.setSeclabel(m.get<9>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Source15>::generate(const Domain::Xml::Source15& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getMode(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getPath(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getHost(), m.get<2>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getService(), m.get<3>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getWiremode(), m.get<4>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getChannel(), m.get<5>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getMaster(), m.get<6>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getSlave(), m.get<7>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getAppend(), m.get<8>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getSeclabel(), m.get<9>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Protocol

namespace Domain
{
namespace Xml
{
bool Protocol::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Protocol, Name::Strict<192> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Protocol::save(QDomElement& dst_) const
{
	Element<Protocol, Name::Strict<192> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Protocol::save(QDomDocument& dst_) const
{
	Element<Protocol, Name::Strict<192> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Protocol>::parse(Domain::Xml::Protocol& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setType(m.get<0>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Protocol>::generate(const Domain::Xml::Protocol& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getType(), m.get<0>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct QemucdevSrcDef

namespace Domain
{
namespace Xml
{
bool QemucdevSrcDef::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	k.push(src_.firstChildElement());
	return 0 <= Traits<QemucdevSrcDef>::parse(*this, k);
}

bool QemucdevSrcDef::save(QDomElement& dst_) const
{
	return 0 <= Traits<QemucdevSrcDef>::generate(*this, dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::QemucdevSrcDef>::parse(Domain::Xml::QemucdevSrcDef& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setSourceList(m.get<0>().getValue());
		dst_.setProtocol(m.get<1>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::QemucdevSrcDef>::generate(const Domain::Xml::QemucdevSrcDef& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getSourceList(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getProtocol(), m.get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Target2

namespace Domain
{
namespace Xml
{
bool Target2::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Target2, Name::Strict<309> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Target2::save(QDomElement& dst_) const
{
	Element<Target2, Name::Strict<309> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Target2::save(QDomDocument& dst_) const
{
	Element<Target2, Name::Strict<309> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Target2>::parse(Domain::Xml::Target2& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setChoice785(m.get<0>().getValue());
		dst_.setPort(m.get<1>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Target2>::generate(const Domain::Xml::Target2& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getChoice785(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getPort(), m.get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Qemucdev

namespace Domain
{
namespace Xml
{
Qemucdev::Qemucdev(): m_type()
{
}

bool Qemucdev::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Qemucdev, Name::Strict<436> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Qemucdev::save(QDomElement& dst_) const
{
	Element<Qemucdev, Name::Strict<436> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Qemucdev::save(QDomDocument& dst_) const
{
	Element<Qemucdev, Name::Strict<436> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Qemucdev>::parse(Domain::Xml::Qemucdev& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setType(m.get<0>().getValue());
		dst_.setTty(m.get<1>().getValue());
		dst_.setQemucdevSrcDef(m.get<2>().get<0>().getValue());
		dst_.setTarget(m.get<2>().get<1>().getValue());
		dst_.setAlias(m.get<2>().get<2>().getValue());
		dst_.setAddress(m.get<2>().get<3>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Qemucdev>::generate(const Domain::Xml::Qemucdev& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getType(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getTty(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getQemucdevSrcDef(), m.get<2>().get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getTarget(), m.get<2>().get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getAlias(), m.get<2>().get<2>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getAddress(), m.get<2>().get<3>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Target3

namespace Domain
{
namespace Xml
{
bool Target3::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Target3, Name::Strict<309> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Target3::save(QDomElement& dst_) const
{
	Element<Target3, Name::Strict<309> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Target3::save(QDomDocument& dst_) const
{
	Element<Target3, Name::Strict<309> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Target3>::parse(Domain::Xml::Target3& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setAddress(m.get<1>().getValue());
		dst_.setPort(m.get<2>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Target3>::generate(const Domain::Xml::Target3& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getAddress(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getPort(), m.get<2>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Channel1

namespace Domain
{
namespace Xml
{
Channel1::Channel1(): m_type()
{
}

bool Channel1::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Channel1, Name::Strict<707> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Channel1::save(QDomElement& dst_) const
{
	Element<Channel1, Name::Strict<707> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Channel1::save(QDomDocument& dst_) const
{
	Element<Channel1, Name::Strict<707> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Channel1>::parse(Domain::Xml::Channel1& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setType(m.get<0>().getValue());
		dst_.setQemucdevSrcDef(m.get<1>().get<0>().getValue());
		dst_.setChoice851(m.get<1>().get<1>().getValue());
		dst_.setAlias(m.get<1>().get<2>().getValue());
		dst_.setAddress(m.get<1>().get<3>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Channel1>::generate(const Domain::Xml::Channel1& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getType(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getQemucdevSrcDef(), m.get<1>().get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getChoice851(), m.get<1>().get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getAlias(), m.get<1>().get<2>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getAddress(), m.get<1>().get<3>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Smartcard852

int Traits<Domain::Xml::Smartcard852>::parse(Domain::Xml::Smartcard852& , QStack<QDomElement>& stack_)
{
	marshal_type m;
	return m.consume(stack_);
}

int Traits<Domain::Xml::Smartcard852>::generate(const Domain::Xml::Smartcard852& , QDomElement& dst_)
{
	marshal_type m;
	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Smartcard856

int Traits<Domain::Xml::Smartcard856>::parse(Domain::Xml::Smartcard856& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setCertificate(m.get<1>().getValue());
		dst_.setCertificate2(m.get<2>().getValue());
		dst_.setCertificate3(m.get<3>().getValue());
		dst_.setDatabase(m.get<4>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Smartcard856>::generate(const Domain::Xml::Smartcard856& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getCertificate(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getCertificate2(), m.get<2>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getCertificate3(), m.get<3>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getDatabase(), m.get<4>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Smartcard857

namespace Domain
{
namespace Xml
{
Smartcard857::Smartcard857(): m_type()
{
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Smartcard857>::parse(Domain::Xml::Smartcard857& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setType(m.get<1>().getValue());
		dst_.setQemucdevSrcDef(m.get<2>().get<0>().getValue());
		dst_.setTarget(m.get<2>().get<1>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Smartcard857>::generate(const Domain::Xml::Smartcard857& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getType(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getQemucdevSrcDef(), m.get<2>().get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getTarget(), m.get<2>().get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Smartcard

namespace Domain
{
namespace Xml
{
bool Smartcard::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Smartcard, Name::Strict<713> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Smartcard::save(QDomElement& dst_) const
{
	Element<Smartcard, Name::Strict<713> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Smartcard::save(QDomDocument& dst_) const
{
	Element<Smartcard, Name::Strict<713> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Smartcard>::parse(Domain::Xml::Smartcard& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setSmartcard(m.get<0>().getValue());
		dst_.setAlias(m.get<1>().getValue());
		dst_.setAddress(m.get<2>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Smartcard>::generate(const Domain::Xml::Smartcard& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getSmartcard(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getAlias(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getAddress(), m.get<2>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Hub

namespace Domain
{
namespace Xml
{
Hub::Hub(): m_type()
{
}

bool Hub::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Hub, Name::Strict<868> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Hub::save(QDomElement& dst_) const
{
	Element<Hub, Name::Strict<868> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Hub::save(QDomDocument& dst_) const
{
	Element<Hub, Name::Strict<868> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Hub>::parse(Domain::Xml::Hub& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setType(m.get<0>().getValue());
		dst_.setAlias(m.get<1>().getValue());
		dst_.setAddress(m.get<2>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Hub>::generate(const Domain::Xml::Hub& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getType(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getAlias(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getAddress(), m.get<2>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Redirdev

namespace Domain
{
namespace Xml
{
Redirdev::Redirdev(): m_bus(), m_type()
{
}

bool Redirdev::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Redirdev, Name::Strict<869> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Redirdev::save(QDomElement& dst_) const
{
	Element<Redirdev, Name::Strict<869> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Redirdev::save(QDomDocument& dst_) const
{
	Element<Redirdev, Name::Strict<869> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Redirdev>::parse(Domain::Xml::Redirdev& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setBus(m.get<0>().getValue());
		dst_.setType(m.get<1>().getValue());
		dst_.setQemucdevSrcDef(m.get<2>().getValue());
		dst_.setAlias(m.get<3>().getValue());
		dst_.setAddress(m.get<4>().getValue());
		dst_.setBoot(m.get<5>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Redirdev>::generate(const Domain::Xml::Redirdev& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getBus(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getType(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getQemucdevSrcDef(), m.get<2>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getAlias(), m.get<3>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getAddress(), m.get<4>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getBoot(), m.get<5>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Usbdev

namespace Domain
{
namespace Xml
{
Usbdev::Usbdev(): m_allow()
{
}

bool Usbdev::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Usbdev, Name::Strict<798> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Usbdev::save(QDomElement& dst_) const
{
	Element<Usbdev, Name::Strict<798> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Usbdev::save(QDomDocument& dst_) const
{
	Element<Usbdev, Name::Strict<798> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Usbdev>::parse(Domain::Xml::Usbdev& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setAllow(m.get<0>().getValue());
		dst_.setClass(m.get<1>().getValue());
		dst_.setVendor(m.get<2>().getValue());
		dst_.setProduct(m.get<3>().getValue());
		dst_.setVersion(m.get<4>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Usbdev>::generate(const Domain::Xml::Usbdev& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getAllow(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getClass(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getVendor(), m.get<2>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getProduct(), m.get<3>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getVersion(), m.get<4>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Backend1029

namespace Domain
{
namespace Xml
{
Backend1029::Backend1029(): m_type()
{
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Backend1029>::parse(Domain::Xml::Backend1029& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setType(m.get<1>().getValue());
		dst_.setQemucdevSrcDef(m.get<2>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Backend1029>::generate(const Domain::Xml::Backend1029& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getType(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getQemucdevSrcDef(), m.get<2>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Rate

namespace Domain
{
namespace Xml
{
Rate::Rate(): m_bytes()
{
}

bool Rate::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Rate, Name::Strict<1031> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Rate::save(QDomElement& dst_) const
{
	Element<Rate, Name::Strict<1031> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Rate::save(QDomDocument& dst_) const
{
	Element<Rate, Name::Strict<1031> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Rate>::parse(Domain::Xml::Rate& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setBytes(m.get<0>().getValue());
		dst_.setPeriod(m.get<1>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Rate>::generate(const Domain::Xml::Rate& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getBytes(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getPeriod(), m.get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Rng

namespace Domain
{
namespace Xml
{
Rng::Rng(): m_model()
{
}

bool Rng::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Rng, Name::Strict<937> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Rng::save(QDomElement& dst_) const
{
	Element<Rng, Name::Strict<937> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Rng::save(QDomDocument& dst_) const
{
	Element<Rng, Name::Strict<937> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Rng>::parse(Domain::Xml::Rng& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setModel(m.get<0>().getValue());
		dst_.setBackend(m.get<1>().get<0>().getValue());
		dst_.setRate(m.get<1>().get<1>().getValue());
		dst_.setAlias(m.get<1>().get<2>().getValue());
		dst_.setAddress(m.get<1>().get<3>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Rng>::generate(const Domain::Xml::Rng& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getModel(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getBackend(), m.get<1>().get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getRate(), m.get<1>().get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getAlias(), m.get<1>().get<2>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getAddress(), m.get<1>().get<3>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Device1

namespace Domain
{
namespace Xml
{
bool Device1::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Device1, Name::Strict<336> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Device1::save(QDomElement& dst_) const
{
	Element<Device1, Name::Strict<336> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Device1::save(QDomDocument& dst_) const
{
	Element<Device1, Name::Strict<336> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Device1>::parse(Domain::Xml::Device1& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setPath(m.get<0>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Device1>::generate(const Domain::Xml::Device1& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getPath(), m.get<0>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Tpm

namespace Domain
{
namespace Xml
{
bool Tpm::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Tpm, Name::Strict<859> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Tpm::save(QDomElement& dst_) const
{
	Element<Tpm, Name::Strict<859> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Tpm::save(QDomDocument& dst_) const
{
	Element<Tpm, Name::Strict<859> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Tpm>::parse(Domain::Xml::Tpm& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setModel(m.get<0>().getValue());
		dst_.setBackend(m.get<1>().getValue());
		dst_.setAlias(m.get<2>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Tpm>::generate(const Domain::Xml::Tpm& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getModel(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getBackend(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getAlias(), m.get<2>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Source16

namespace Domain
{
namespace Xml
{
bool Source16::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Source16, Name::Strict<483> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Source16::save(QDomElement& dst_) const
{
	Element<Source16, Name::Strict<483> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Source16::save(QDomDocument& dst_) const
{
	Element<Source16, Name::Strict<483> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Source16>::parse(Domain::Xml::Source16& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setPagesize(m.get<0>().getValue());
		dst_.setNodemask(m.get<1>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Source16>::generate(const Domain::Xml::Source16& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getPagesize(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getNodemask(), m.get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Target4

namespace Domain
{
namespace Xml
{
Target4::Target4(): m_node()
{
}

bool Target4::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Target4, Name::Strict<309> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Target4::save(QDomElement& dst_) const
{
	Element<Target4, Name::Strict<309> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Target4::save(QDomDocument& dst_) const
{
	Element<Target4, Name::Strict<309> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Target4>::parse(Domain::Xml::Target4& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setSize(m.get<0>().getValue());
		dst_.setNode(m.get<1>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Target4>::generate(const Domain::Xml::Target4& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getSize(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getNode(), m.get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Memory2

namespace Domain
{
namespace Xml
{
Memory2::Memory2(): m_model()
{
}

bool Memory2::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Memory2, Name::Strict<312> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Memory2::save(QDomElement& dst_) const
{
	Element<Memory2, Name::Strict<312> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Memory2::save(QDomDocument& dst_) const
{
	Element<Memory2, Name::Strict<312> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Memory2>::parse(Domain::Xml::Memory2& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setModel(m.get<0>().getValue());
		dst_.setSource(m.get<1>().get<0>().getValue());
		dst_.setTarget(m.get<1>().get<1>().getValue());
		dst_.setAddress(m.get<1>().get<2>().getValue());
		dst_.setAlias(m.get<1>().get<3>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Memory2>::generate(const Domain::Xml::Memory2& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getModel(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getSource(), m.get<1>().get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getTarget(), m.get<1>().get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getAddress(), m.get<1>().get<2>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getAlias(), m.get<1>().get<3>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Watchdog

namespace Domain
{
namespace Xml
{
Watchdog::Watchdog(): m_model()
{
}

bool Watchdog::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Watchdog, Name::Strict<837> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Watchdog::save(QDomElement& dst_) const
{
	Element<Watchdog, Name::Strict<837> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Watchdog::save(QDomDocument& dst_) const
{
	Element<Watchdog, Name::Strict<837> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Watchdog>::parse(Domain::Xml::Watchdog& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setModel(m.get<0>().getValue());
		dst_.setAction(m.get<1>().getValue());
		dst_.setAlias(m.get<2>().getValue());
		dst_.setAddress(m.get<3>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Watchdog>::generate(const Domain::Xml::Watchdog& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getModel(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getAction(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getAlias(), m.get<2>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getAddress(), m.get<3>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Memballoon

namespace Domain
{
namespace Xml
{
Memballoon::Memballoon(): m_model()
{
}

bool Memballoon::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Memballoon, Name::Strict<843> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Memballoon::save(QDomElement& dst_) const
{
	Element<Memballoon, Name::Strict<843> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Memballoon::save(QDomDocument& dst_) const
{
	Element<Memballoon, Name::Strict<843> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Memballoon>::parse(Domain::Xml::Memballoon& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setModel(m.get<0>().getValue());
		dst_.setAutodeflate(m.get<1>().getValue());
		dst_.setAlias(m.get<2>().get<0>().getValue());
		dst_.setAddress(m.get<2>().get<1>().getValue());
		dst_.setStats(m.get<2>().get<2>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Memballoon>::generate(const Domain::Xml::Memballoon& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getModel(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getAutodeflate(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getAlias(), m.get<2>().get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getAddress(), m.get<2>().get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getStats(), m.get<2>().get<2>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Nvram1

namespace Domain
{
namespace Xml
{
bool Nvram1::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Nvram1, Name::Strict<263> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Nvram1::save(QDomElement& dst_) const
{
	Element<Nvram1, Name::Strict<263> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Nvram1::save(QDomDocument& dst_) const
{
	Element<Nvram1, Name::Strict<263> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Nvram1>::parse(Domain::Xml::Nvram1& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setAddress(m.get<0>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Nvram1>::generate(const Domain::Xml::Nvram1& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getAddress(), m.get<0>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Panic

namespace Domain
{
namespace Xml
{
bool Panic::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Panic, Name::Strict<943> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Panic::save(QDomElement& dst_) const
{
	Element<Panic, Name::Strict<943> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Panic::save(QDomDocument& dst_) const
{
	Element<Panic, Name::Strict<943> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Panic>::parse(Domain::Xml::Panic& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setModel(m.get<0>().getValue());
		dst_.setAddress(m.get<1>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Panic>::generate(const Domain::Xml::Panic& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getModel(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getAddress(), m.get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Devices

namespace Domain
{
namespace Xml
{
bool Devices::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Devices, Name::Strict<214> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Devices::save(QDomElement& dst_) const
{
	Element<Devices, Name::Strict<214> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Devices::save(QDomDocument& dst_) const
{
	Element<Devices, Name::Strict<214> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Devices>::parse(Domain::Xml::Devices& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setEmulator(m.get<0>().getValue());
		dst_.setChoice941List(m.get<1>().getValue());
		dst_.setWatchdog(m.get<2>().getValue());
		dst_.setMemballoon(m.get<3>().getValue());
		dst_.setNvram(m.get<4>().getValue());
		dst_.setPanicList(m.get<5>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Devices>::generate(const Domain::Xml::Devices& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getEmulator(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getChoice941List(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getWatchdog(), m.get<2>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getMemballoon(), m.get<3>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getNvram(), m.get<4>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getPanicList(), m.get<5>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Seclabel222

int Traits<Domain::Xml::Seclabel222>::parse(Domain::Xml::Seclabel222& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setLabel(m.get<2>().get<0>().getValue());
		dst_.setImagelabel(m.get<2>().get<1>().getValue());
		dst_.setBaselabel(m.get<2>().get<2>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Seclabel222>::generate(const Domain::Xml::Seclabel222& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getLabel(), m.get<2>().get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getImagelabel(), m.get<2>().get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getBaselabel(), m.get<2>().get<2>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Seclabel224

int Traits<Domain::Xml::Seclabel224>::parse(Domain::Xml::Seclabel224& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setRelabel(m.get<1>().getValue());
		dst_.setLabel(m.get<2>().get<0>().getValue());
		dst_.setImagelabel(m.get<2>().get<1>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Seclabel224>::generate(const Domain::Xml::Seclabel224& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getRelabel(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getLabel(), m.get<2>().get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getImagelabel(), m.get<2>().get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Seclabel226

int Traits<Domain::Xml::Seclabel226>::parse(Domain::Xml::Seclabel226& , QStack<QDomElement>& stack_)
{
	marshal_type m;
	return m.consume(stack_);
}

int Traits<Domain::Xml::Seclabel226>::generate(const Domain::Xml::Seclabel226& , QDomElement& dst_)
{
	marshal_type m;
	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Seclabel1

namespace Domain
{
namespace Xml
{
bool Seclabel1::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Seclabel1, Name::Strict<215> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Seclabel1::save(QDomElement& dst_) const
{
	Element<Seclabel1, Name::Strict<215> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Seclabel1::save(QDomDocument& dst_) const
{
	Element<Seclabel1, Name::Strict<215> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Seclabel1>::parse(Domain::Xml::Seclabel1& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setModel(m.get<0>().getValue());
		dst_.setSeclabel(m.get<1>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Seclabel1>::generate(const Domain::Xml::Seclabel1& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getModel(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getSeclabel(), m.get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Env

namespace Domain
{
namespace Xml
{
bool Env::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Env, Name::Strict<1112> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Env::save(QDomElement& dst_) const
{
	Element<Env, Name::Strict<1112> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Env::save(QDomDocument& dst_) const
{
	Element<Env, Name::Strict<1112> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Env>::parse(Domain::Xml::Env& dst_, QStack<QDomElement>& stack_)
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

int Traits<Domain::Xml::Env>::generate(const Domain::Xml::Env& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getName(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getValue(), m.get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Commandline

namespace Domain
{
namespace Xml
{
bool Commandline::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Commandline, Name::Scoped<1110, 1113> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Commandline::save(QDomElement& dst_) const
{
	Element<Commandline, Name::Scoped<1110, 1113> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Commandline::save(QDomDocument& dst_) const
{
	Element<Commandline, Name::Scoped<1110, 1113> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Commandline>::parse(Domain::Xml::Commandline& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setArgList(m.get<0>().getValue());
		dst_.setEnvList(m.get<1>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Commandline>::generate(const Domain::Xml::Commandline& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getArgList(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getEnvList(), m.get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Domain

namespace Domain
{
namespace Xml
{
Domain::Domain(): m_type()
{
}

bool Domain::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Domain, Name::Strict<1> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Domain::save(QDomElement& dst_) const
{
	Element<Domain, Name::Strict<1> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Domain::save(QDomDocument& dst_) const
{
	Element<Domain, Name::Strict<1> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Domain

int Traits<Domain::Xml::Domain>::parse(Domain::Xml::Domain& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setType(m.get<0>().getValue());
		dst_.setIds(m.get<1>().getValue());
		dst_.setTitle(m.get<2>().get<0>().getValue());
		dst_.setDescription(m.get<2>().get<1>().getValue());
		dst_.setCpu(m.get<2>().get<2>().getValue());
		dst_.setSysinfo(m.get<2>().get<3>().getValue());
		dst_.setOs(m.get<2>().get<4>().getValue());
		dst_.setClock(m.get<2>().get<5>().getValue());
		dst_.setMemory(m.get<2>().get<6>().getValue());
		dst_.setMaxMemory(m.get<2>().get<7>().getValue());
		dst_.setCurrentMemory(m.get<2>().get<8>().getValue());
		dst_.setMemoryBacking(m.get<2>().get<9>().getValue());
		dst_.setVcpu(m.get<2>().get<10>().getValue());
		dst_.setIothreads(m.get<2>().get<11>().getValue());
		dst_.setBlkiotune(m.get<2>().get<12>().getValue());
		dst_.setMemtune(m.get<2>().get<13>().getValue());
		dst_.setCputune(m.get<2>().get<14>().getValue());
		dst_.setNumatune(m.get<2>().get<15>().getValue());
		dst_.setResource(m.get<2>().get<16>().getValue());
		dst_.setFeatures(m.get<2>().get<17>().getValue());
		dst_.setOnReboot(m.get<2>().get<18>().getValue());
		dst_.setOnPoweroff(m.get<2>().get<19>().getValue());
		dst_.setOnCrash(m.get<2>().get<20>().getValue());
		dst_.setOnLockfailure(m.get<2>().get<21>().getValue());
		dst_.setPm(m.get<2>().get<22>().getValue());
		dst_.setIdmap(m.get<2>().get<23>().getValue());
		dst_.setDevices(m.get<2>().get<24>().getValue());
		dst_.setSeclabelList(m.get<2>().get<25>().getValue());
		dst_.setCommandline(m.get<2>().get<26>().getValue());
	}
	return output;
}

int Traits<Domain::Xml::Domain>::generate(const Domain::Xml::Domain& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getType(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getIds(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getTitle(), m.get<2>().get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getDescription(), m.get<2>().get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getCpu(), m.get<2>().get<2>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getSysinfo(), m.get<2>().get<3>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getOs(), m.get<2>().get<4>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getClock(), m.get<2>().get<5>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getMemory(), m.get<2>().get<6>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getMaxMemory(), m.get<2>().get<7>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getCurrentMemory(), m.get<2>().get<8>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getMemoryBacking(), m.get<2>().get<9>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getVcpu(), m.get<2>().get<10>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getIothreads(), m.get<2>().get<11>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getBlkiotune(), m.get<2>().get<12>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getMemtune(), m.get<2>().get<13>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getCputune(), m.get<2>().get<14>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getNumatune(), m.get<2>().get<15>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getResource(), m.get<2>().get<16>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getFeatures(), m.get<2>().get<17>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getOnReboot(), m.get<2>().get<18>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getOnPoweroff(), m.get<2>().get<19>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getOnCrash(), m.get<2>().get<20>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getOnLockfailure(), m.get<2>().get<21>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getPm(), m.get<2>().get<22>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getIdmap(), m.get<2>().get<23>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getDevices(), m.get<2>().get<24>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getSeclabelList(), m.get<2>().get<25>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getCommandline(), m.get<2>().get<26>()))
		return -1;

	return m.produce(dst_);
}

} // namespace Libvirt
