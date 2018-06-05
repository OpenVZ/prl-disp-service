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

#include "blockexport_type.h"

namespace Libvirt
{
///////////////////////////////////////////////////////////////////////////////
// struct Address

namespace Blockexport
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
} // namespace Blockexport

int Traits<Blockexport::Xml::Address>::parse(Blockexport::Xml::Address& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setHost(m.get<1>().getValue());
		dst_.setPort(m.get<2>().getValue());
		dst_.setAutoport(m.get<3>().getValue());
		dst_.setTls(m.get<4>().getValue());
	}
	return output;
}

int Traits<Blockexport::Xml::Address>::generate(const Blockexport::Xml::Address& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getHost(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getPort(), m.get<2>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getAutoport(), m.get<3>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getTls(), m.get<4>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Disk

namespace Blockexport
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
} // namespace Blockexport

int Traits<Blockexport::Xml::Disk>::parse(Blockexport::Xml::Disk& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setName(m.get<0>().getValue());
		dst_.setExportname(m.get<1>().getValue());
		dst_.setSnapshot(m.get<2>().getValue());
		dst_.setChoice2101(m.get<3>().getValue());
		dst_.setReadonly(m.get<4>().getValue());
	}
	return output;
}

int Traits<Blockexport::Xml::Disk>::generate(const Blockexport::Xml::Disk& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getName(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getExportname(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getSnapshot(), m.get<2>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getChoice2101(), m.get<3>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getReadonly(), m.get<4>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Domainblockexport_

namespace Blockexport
{
namespace Xml
{
bool Domainblockexport_::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Domainblockexport_, Name::Strict<2103> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Domainblockexport_::save(QDomElement& dst_) const
{
	Element<Domainblockexport_, Name::Strict<2103> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Domainblockexport_::save(QDomDocument& dst_) const
{
	Element<Domainblockexport_, Name::Strict<2103> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Blockexport

int Traits<Blockexport::Xml::Domainblockexport_>::parse(Blockexport::Xml::Domainblockexport_& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setAddress(m.get<1>().get<0>().getValue());
		dst_.setDiskList(m.get<1>().get<1>().getValue());
	}
	return output;
}

int Traits<Blockexport::Xml::Domainblockexport_>::generate(const Blockexport::Xml::Domainblockexport_& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getAddress(), m.get<1>().get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getDiskList(), m.get<1>().get<1>()))
		return -1;

	return m.produce(dst_);
}

} // namespace Libvirt
