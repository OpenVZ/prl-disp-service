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

#include "blocksnapshot_type.h"

namespace Libvirt
{
///////////////////////////////////////////////////////////////////////////////
// struct Disk

namespace Blocksnapshot
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
} // namespace Blocksnapshot

int Traits<Blocksnapshot::Xml::Disk>::parse(Blocksnapshot::Xml::Disk& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setName(m.get<0>().getValue());
		dst_.setFleece(m.get<2>().getValue());
	}
	return output;
}

int Traits<Blocksnapshot::Xml::Disk>::generate(const Blocksnapshot::Xml::Disk& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getName(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getFleece(), m.get<2>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Domainblocksnapshot

namespace Blocksnapshot
{
namespace Xml
{
bool Domainblocksnapshot::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Domainblocksnapshot, Name::Strict<2107> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Domainblocksnapshot::save(QDomElement& dst_) const
{
	Element<Domainblocksnapshot, Name::Strict<2107> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Domainblocksnapshot::save(QDomDocument& dst_) const
{
	Element<Domainblocksnapshot, Name::Strict<2107> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Blocksnapshot

int Traits<Blocksnapshot::Xml::Domainblocksnapshot>::parse(Blocksnapshot::Xml::Domainblocksnapshot& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setName(m.get<0>().getValue());
		dst_.setDiskList(m.get<1>().getValue());
	}
	return output;
}

int Traits<Blocksnapshot::Xml::Domainblocksnapshot>::generate(const Blocksnapshot::Xml::Domainblocksnapshot& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getName(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getDiskList(), m.get<1>()))
		return -1;

	return m.produce(dst_);
}

} // namespace Libvirt
