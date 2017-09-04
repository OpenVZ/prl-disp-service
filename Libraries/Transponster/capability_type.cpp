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

#include "capability_type.h"

namespace Libvirt
{
///////////////////////////////////////////////////////////////////////////////
// struct Features

namespace Capability
{
namespace Xml
{
Features::Features(): m_pae(), m_nonpae(), m_vmx(), m_svm()
{
}

bool Features::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Features, Name::Strict<150> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Features::save(QDomElement& dst_) const
{
	Element<Features, Name::Strict<150> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Features::save(QDomDocument& dst_) const
{
	Element<Features, Name::Strict<150> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Capability

int Traits<Capability::Xml::Features>::parse(Capability::Xml::Features& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setPae(m.get<0>().getValue());
		dst_.setNonpae(m.get<1>().getValue());
		dst_.setVmx(m.get<2>().getValue());
		dst_.setSvm(m.get<3>().getValue());
	}
	return output;
}

int Traits<Capability::Xml::Features>::generate(const Capability::Xml::Features& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getPae(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getNonpae(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getVmx(), m.get<2>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getSvm(), m.get<3>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Topology

namespace Capability
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
	Element<Topology, Name::Strict<990> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Topology::save(QDomElement& dst_) const
{
	Element<Topology, Name::Strict<990> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Topology::save(QDomDocument& dst_) const
{
	Element<Topology, Name::Strict<990> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Capability

int Traits<Capability::Xml::Topology>::parse(Capability::Xml::Topology& dst_, QStack<QDomElement>& stack_)
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

int Traits<Capability::Xml::Topology>::generate(const Capability::Xml::Topology& src_, QDomElement& dst_)
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
// struct Pages

namespace Capability
{
namespace Xml
{
Pages::Pages(): m_size(), m_ownValue()
{
}

bool Pages::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Pages, Name::Strict<1834> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Pages::save(QDomElement& dst_) const
{
	Element<Pages, Name::Strict<1834> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Pages::save(QDomDocument& dst_) const
{
	Element<Pages, Name::Strict<1834> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Capability

int Traits<Capability::Xml::Pages>::parse(Capability::Xml::Pages& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setUnit(m.get<0>().getValue());
		dst_.setSize(m.get<1>().getValue());
		dst_.setOwnValue(m.get<2>().getValue());
	}
	return output;
}

int Traits<Capability::Xml::Pages>::generate(const Capability::Xml::Pages& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getUnit(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getSize(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getOwnValue(), m.get<2>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Cpuspec

namespace Capability
{
namespace Xml
{
bool Cpuspec::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	k.push(src_.firstChildElement());
	return 0 <= Traits<Cpuspec>::parse(*this, k);
}

bool Cpuspec::save(QDomElement& dst_) const
{
	return 0 <= Traits<Cpuspec>::generate(*this, dst_);
}


} // namespace Xml
} // namespace Capability

int Traits<Capability::Xml::Cpuspec>::parse(Capability::Xml::Cpuspec& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setModel(m.get<0>().getValue());
		dst_.setVendor(m.get<1>().getValue());
		dst_.setTopology(m.get<2>().getValue());
		dst_.setFeatureList(m.get<3>().getValue());
		dst_.setPagesList(m.get<4>().getValue());
	}
	return output;
}

int Traits<Capability::Xml::Cpuspec>::generate(const Capability::Xml::Cpuspec& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getModel(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getVendor(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getTopology(), m.get<2>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getFeatureList(), m.get<3>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getPagesList(), m.get<4>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Cpu

namespace Capability
{
namespace Xml
{
Cpu::Cpu(): m_arch()
{
}

bool Cpu::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Cpu, Name::Strict<212> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Cpu::save(QDomElement& dst_) const
{
	Element<Cpu, Name::Strict<212> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Cpu::save(QDomDocument& dst_) const
{
	Element<Cpu, Name::Strict<212> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Capability

int Traits<Capability::Xml::Cpu>::parse(Capability::Xml::Cpu& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setArch(m.get<0>().getValue());
		dst_.setFeatures(m.get<1>().getValue());
		dst_.setCpuspec(m.get<2>().getValue());
	}
	return output;
}

int Traits<Capability::Xml::Cpu>::generate(const Capability::Xml::Cpu& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getArch(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getFeatures(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getCpuspec(), m.get<2>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct PowerManagement

namespace Capability
{
namespace Xml
{
PowerManagement::PowerManagement(): m_suspendMem(), m_suspendDisk(), m_suspendHybrid()
{
}

bool PowerManagement::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<PowerManagement, Name::Strict<1797> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool PowerManagement::save(QDomElement& dst_) const
{
	Element<PowerManagement, Name::Strict<1797> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool PowerManagement::save(QDomDocument& dst_) const
{
	Element<PowerManagement, Name::Strict<1797> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Capability

int Traits<Capability::Xml::PowerManagement>::parse(Capability::Xml::PowerManagement& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setSuspendMem(m.get<0>().getValue());
		dst_.setSuspendDisk(m.get<1>().getValue());
		dst_.setSuspendHybrid(m.get<2>().getValue());
	}
	return output;
}

int Traits<Capability::Xml::PowerManagement>::generate(const Capability::Xml::PowerManagement& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getSuspendMem(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getSuspendDisk(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getSuspendHybrid(), m.get<2>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct MigrationFeatures

namespace Capability
{
namespace Xml
{
MigrationFeatures::MigrationFeatures(): m_live()
{
}

bool MigrationFeatures::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<MigrationFeatures, Name::Strict<1808> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool MigrationFeatures::save(QDomElement& dst_) const
{
	Element<MigrationFeatures, Name::Strict<1808> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool MigrationFeatures::save(QDomDocument& dst_) const
{
	Element<MigrationFeatures, Name::Strict<1808> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Capability

int Traits<Capability::Xml::MigrationFeatures>::parse(Capability::Xml::MigrationFeatures& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setLive(m.get<0>().getValue());
		dst_.setUriTransports(m.get<1>().getValue());
	}
	return output;
}

int Traits<Capability::Xml::MigrationFeatures>::generate(const Capability::Xml::MigrationFeatures& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getLive(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getUriTransports(), m.get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct ScaledInteger

namespace Capability
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
	Element<ScaledInteger, Name::Strict<318> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool ScaledInteger::save(QDomElement& dst_) const
{
	Element<ScaledInteger, Name::Strict<318> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool ScaledInteger::save(QDomDocument& dst_) const
{
	Element<ScaledInteger, Name::Strict<318> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Capability

int Traits<Capability::Xml::ScaledInteger>::parse(Capability::Xml::ScaledInteger& dst_, QStack<QDomElement>& stack_)
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

int Traits<Capability::Xml::ScaledInteger>::generate(const Capability::Xml::ScaledInteger& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getUnit(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getOwnValue(), m.get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Sibling

namespace Capability
{
namespace Xml
{
Sibling::Sibling(): m_id(), m_value()
{
}

bool Sibling::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Sibling, Name::Strict<1817> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Sibling::save(QDomElement& dst_) const
{
	Element<Sibling, Name::Strict<1817> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Sibling::save(QDomDocument& dst_) const
{
	Element<Sibling, Name::Strict<1817> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Capability

int Traits<Capability::Xml::Sibling>::parse(Capability::Xml::Sibling& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setId(m.get<0>().getValue());
		dst_.setValue(m.get<1>().getValue());
	}
	return output;
}

int Traits<Capability::Xml::Sibling>::generate(const Capability::Xml::Sibling& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getId(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getValue(), m.get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Anonymous1834

namespace Capability
{
namespace Xml
{
Anonymous1834::Anonymous1834(): m_socketId(), m_coreId()
{
}

bool Anonymous1834::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	k.push(src_.firstChildElement());
	return 0 <= Traits<Anonymous1834>::parse(*this, k);
}

bool Anonymous1834::save(QDomElement& dst_) const
{
	return 0 <= Traits<Anonymous1834>::generate(*this, dst_);
}


} // namespace Xml
} // namespace Capability

int Traits<Capability::Xml::Anonymous1834>::parse(Capability::Xml::Anonymous1834& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setSocketId(m.get<0>().getValue());
		dst_.setCoreId(m.get<1>().getValue());
		dst_.setSiblings(m.get<2>().getValue());
	}
	return output;
}

int Traits<Capability::Xml::Anonymous1834>::generate(const Capability::Xml::Anonymous1834& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getSocketId(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getCoreId(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getSiblings(), m.get<2>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Cpu1

namespace Capability
{
namespace Xml
{
Cpu1::Cpu1(): m_id()
{
}

bool Cpu1::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Cpu1, Name::Strict<212> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Cpu1::save(QDomElement& dst_) const
{
	Element<Cpu1, Name::Strict<212> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Cpu1::save(QDomDocument& dst_) const
{
	Element<Cpu1, Name::Strict<212> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Capability

int Traits<Capability::Xml::Cpu1>::parse(Capability::Xml::Cpu1& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setId(m.get<0>().getValue());
		dst_.setAnonymous1834(m.get<1>().getValue());
	}
	return output;
}

int Traits<Capability::Xml::Cpu1>::generate(const Capability::Xml::Cpu1& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getId(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getAnonymous1834(), m.get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Cpus

namespace Capability
{
namespace Xml
{
Cpus::Cpus(): m_num()
{
}

bool Cpus::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Cpus, Name::Strict<996> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Cpus::save(QDomElement& dst_) const
{
	Element<Cpus, Name::Strict<996> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Cpus::save(QDomDocument& dst_) const
{
	Element<Cpus, Name::Strict<996> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Capability

int Traits<Capability::Xml::Cpus>::parse(Capability::Xml::Cpus& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setNum(m.get<0>().getValue());
		dst_.setCpuList(m.get<1>().getValue());
	}
	return output;
}

int Traits<Capability::Xml::Cpus>::generate(const Capability::Xml::Cpus& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getNum(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getCpuList(), m.get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Cell

namespace Capability
{
namespace Xml
{
Cell::Cell(): m_id()
{
}

bool Cell::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Cell, Name::Strict<995> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Cell::save(QDomElement& dst_) const
{
	Element<Cell, Name::Strict<995> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Cell::save(QDomDocument& dst_) const
{
	Element<Cell, Name::Strict<995> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Capability

int Traits<Capability::Xml::Cell>::parse(Capability::Xml::Cell& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setId(m.get<0>().getValue());
		dst_.setMemory(m.get<1>().getValue());
		dst_.setPagesList(m.get<2>().getValue());
		dst_.setDistances(m.get<3>().getValue());
		dst_.setCpus(m.get<4>().getValue());
	}
	return output;
}

int Traits<Capability::Xml::Cell>::generate(const Capability::Xml::Cell& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getId(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getMemory(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getPagesList(), m.get<2>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getDistances(), m.get<3>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getCpus(), m.get<4>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Cells

namespace Capability
{
namespace Xml
{
Cells::Cells(): m_num()
{
}

bool Cells::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Cells, Name::Strict<1814> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Cells::save(QDomElement& dst_) const
{
	Element<Cells, Name::Strict<1814> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Cells::save(QDomDocument& dst_) const
{
	Element<Cells, Name::Strict<1814> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Capability

int Traits<Capability::Xml::Cells>::parse(Capability::Xml::Cells& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setNum(m.get<0>().getValue());
		dst_.setCellList(m.get<1>().getValue());
	}
	return output;
}

int Traits<Capability::Xml::Cells>::generate(const Capability::Xml::Cells& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getNum(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getCellList(), m.get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Baselabel

namespace Capability
{
namespace Xml
{
bool Baselabel::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Baselabel, Name::Strict<228> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Baselabel::save(QDomElement& dst_) const
{
	Element<Baselabel, Name::Strict<228> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Baselabel::save(QDomDocument& dst_) const
{
	Element<Baselabel, Name::Strict<228> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Capability

int Traits<Capability::Xml::Baselabel>::parse(Capability::Xml::Baselabel& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setType(m.get<0>().getValue());
		dst_.setOwnValue(m.get<1>().getValue());
	}
	return output;
}

int Traits<Capability::Xml::Baselabel>::generate(const Capability::Xml::Baselabel& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getType(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getOwnValue(), m.get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Secmodel

namespace Capability
{
namespace Xml
{
bool Secmodel::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Secmodel, Name::Strict<1799> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Secmodel::save(QDomElement& dst_) const
{
	Element<Secmodel, Name::Strict<1799> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Secmodel::save(QDomDocument& dst_) const
{
	Element<Secmodel, Name::Strict<1799> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Capability

int Traits<Capability::Xml::Secmodel>::parse(Capability::Xml::Secmodel& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setModel(m.get<0>().getValue());
		dst_.setDoi(m.get<1>().getValue());
		dst_.setBaselabelList(m.get<2>().getValue());
	}
	return output;
}

int Traits<Capability::Xml::Secmodel>::generate(const Capability::Xml::Secmodel& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getModel(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getDoi(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getBaselabelList(), m.get<2>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Host

namespace Capability
{
namespace Xml
{
bool Host::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Host, Name::Strict<505> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Host::save(QDomElement& dst_) const
{
	Element<Host, Name::Strict<505> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Host::save(QDomDocument& dst_) const
{
	Element<Host, Name::Strict<505> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Capability

int Traits<Capability::Xml::Host>::parse(Capability::Xml::Host& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setUuid(m.get<0>().getValue());
		dst_.setCpu(m.get<1>().getValue());
		dst_.setPowerManagement(m.get<2>().getValue());
		dst_.setMigrationFeatures(m.get<3>().getValue());
		dst_.setTopology(m.get<4>().getValue());
		dst_.setSecmodelList(m.get<5>().getValue());
	}
	return output;
}

int Traits<Capability::Xml::Host>::generate(const Capability::Xml::Host& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getUuid(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getCpu(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getPowerManagement(), m.get<2>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getMigrationFeatures(), m.get<3>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getTopology(), m.get<4>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getSecmodelList(), m.get<5>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Machine

namespace Capability
{
namespace Xml
{
bool Machine::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Machine, Name::Strict<278> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Machine::save(QDomElement& dst_) const
{
	Element<Machine, Name::Strict<278> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Machine::save(QDomDocument& dst_) const
{
	Element<Machine, Name::Strict<278> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Capability

int Traits<Capability::Xml::Machine>::parse(Capability::Xml::Machine& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setCanonical(m.get<0>().getValue());
		dst_.setMaxCpus(m.get<1>().getValue());
		dst_.setOwnValue(m.get<2>().getValue());
	}
	return output;
}

int Traits<Capability::Xml::Machine>::generate(const Capability::Xml::Machine& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getCanonical(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getMaxCpus(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getOwnValue(), m.get<2>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Domain

namespace Capability
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
} // namespace Capability

int Traits<Capability::Xml::Domain>::parse(Capability::Xml::Domain& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setType(m.get<0>().getValue());
		dst_.setEmulator(m.get<1>().getValue());
		dst_.setMachineList(m.get<2>().getValue());
	}
	return output;
}

int Traits<Capability::Xml::Domain>::generate(const Capability::Xml::Domain& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getType(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getEmulator(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getMachineList(), m.get<2>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Arch

namespace Capability
{
namespace Xml
{
Arch::Arch(): m_name(), m_wordsize()
{
}

bool Arch::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Arch, Name::Strict<277> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Arch::save(QDomElement& dst_) const
{
	Element<Arch, Name::Strict<277> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Arch::save(QDomDocument& dst_) const
{
	Element<Arch, Name::Strict<277> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Capability

int Traits<Capability::Xml::Arch>::parse(Capability::Xml::Arch& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setName(m.get<0>().getValue());
		dst_.setWordsize(m.get<1>().getValue());
		dst_.setEmulator(m.get<2>().getValue());
		dst_.setLoader(m.get<3>().getValue());
		dst_.setMachineList(m.get<4>().getValue());
		dst_.setDomainList(m.get<5>().getValue());
	}
	return output;
}

int Traits<Capability::Xml::Arch>::generate(const Capability::Xml::Arch& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getName(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getWordsize(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getEmulator(), m.get<2>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getLoader(), m.get<3>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getMachineList(), m.get<4>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getDomainList(), m.get<5>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Featuretoggle

namespace Capability
{
namespace Xml
{
Featuretoggle::Featuretoggle(): m_toggle(), m_default()
{
}

bool Featuretoggle::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Featuretoggle, Name::Strict<1524> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Featuretoggle::save(QDomElement& dst_) const
{
	Element<Featuretoggle, Name::Strict<1524> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Featuretoggle::save(QDomDocument& dst_) const
{
	Element<Featuretoggle, Name::Strict<1524> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Capability

int Traits<Capability::Xml::Featuretoggle>::parse(Capability::Xml::Featuretoggle& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setToggle(m.get<0>().getValue());
		dst_.setDefault(m.get<1>().getValue());
	}
	return output;
}

int Traits<Capability::Xml::Featuretoggle>::generate(const Capability::Xml::Featuretoggle& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getToggle(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getDefault(), m.get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Features1

namespace Capability
{
namespace Xml
{
Features1::Features1(): m_pae(), m_nonpae(), m_ia64Be(), m_cpuselection(), m_deviceboot()
{
}

bool Features1::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Features1, Name::Strict<150> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Features1::save(QDomElement& dst_) const
{
	Element<Features1, Name::Strict<150> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Features1::save(QDomDocument& dst_) const
{
	Element<Features1, Name::Strict<150> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Capability

int Traits<Capability::Xml::Features1>::parse(Capability::Xml::Features1& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setPae(m.get<0>().getValue());
		dst_.setNonpae(m.get<1>().getValue());
		dst_.setIa64Be(m.get<2>().getValue());
		dst_.setAcpi(m.get<3>().getValue());
		dst_.setApic(m.get<4>().getValue());
		dst_.setCpuselection(m.get<5>().getValue());
		dst_.setDeviceboot(m.get<6>().getValue());
		dst_.setDisksnapshot(m.get<7>().getValue());
	}
	return output;
}

int Traits<Capability::Xml::Features1>::generate(const Capability::Xml::Features1& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getPae(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getNonpae(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getIa64Be(), m.get<2>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getAcpi(), m.get<3>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getApic(), m.get<4>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getCpuselection(), m.get<5>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getDeviceboot(), m.get<6>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getDisksnapshot(), m.get<7>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Guest

namespace Capability
{
namespace Xml
{
Guest::Guest(): m_osType()
{
}

bool Guest::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Guest, Name::Strict<400> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Guest::save(QDomElement& dst_) const
{
	Element<Guest, Name::Strict<400> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Guest::save(QDomDocument& dst_) const
{
	Element<Guest, Name::Strict<400> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Capability

int Traits<Capability::Xml::Guest>::parse(Capability::Xml::Guest& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setOsType(m.get<0>().getValue());
		dst_.setArch(m.get<1>().getValue());
		dst_.setFeatures(m.get<2>().getValue());
	}
	return output;
}

int Traits<Capability::Xml::Guest>::generate(const Capability::Xml::Guest& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getOsType(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getArch(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getFeatures(), m.get<2>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Capabilities

namespace Capability
{
namespace Xml
{
bool Capabilities::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Capabilities, Name::Strict<893> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Capabilities::save(QDomElement& dst_) const
{
	Element<Capabilities, Name::Strict<893> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Capabilities::save(QDomDocument& dst_) const
{
	Element<Capabilities, Name::Strict<893> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Capability

int Traits<Capability::Xml::Capabilities>::parse(Capability::Xml::Capabilities& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setHost(m.get<0>().getValue());
		dst_.setGuestList(m.get<1>().getValue());
	}
	return output;
}

int Traits<Capability::Xml::Capabilities>::generate(const Capability::Xml::Capabilities& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getHost(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getGuestList(), m.get<1>()))
		return -1;

	return m.produce(dst_);
}

} // namespace Libvirt
