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

#ifndef __BLOCKSNAPSHOT_TYPE_H__
#define __BLOCKSNAPSHOT_TYPE_H__
#include "base.h"
#include "blocksnapshot_data.h"
#include "blocksnapshot_enum.h"
#include "patterns.h"
#include <boost/any.hpp>

namespace Libvirt
{

///////////////////////////////////////////////////////////////////////////////
// struct Disk

namespace Blocksnapshot
{
namespace Xml
{
struct Disk
{
	const VName& getName() const
	{
		return m_name;
	}
	void setName(const VName& value_)
	{
		m_name = value_;
	}
	const PAbsFilePath::value_type& getFleece() const
	{
		return m_fleece;
	}
	void setFleece(const PAbsFilePath::value_type& value_)
	{
		m_fleece = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	VName m_name;
	PAbsFilePath::value_type m_fleece;
};

} // namespace Xml
} // namespace Blocksnapshot

///////////////////////////////////////////////////////////////////////////////
// struct Domainblocksnapshot

namespace Blocksnapshot
{
namespace Xml
{
struct Domainblocksnapshot
{
	const boost::optional<QString >& getName() const
	{
		return m_name;
	}
	void setName(const boost::optional<QString >& value_)
	{
		m_name = value_;
	}
	const QList<Disk >& getDiskList() const
	{
		return m_diskList;
	}
	void setDiskList(const QList<Disk >& value_)
	{
		m_diskList = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	boost::optional<QString > m_name;
	QList<Disk > m_diskList;
};

} // namespace Xml
} // namespace Blocksnapshot

///////////////////////////////////////////////////////////////////////////////
// struct Disk traits

template<>
struct Traits<Blocksnapshot::Xml::Disk>
{
	typedef Ordered<mpl::vector<Attribute<Blocksnapshot::Xml::VName, Name::Strict<107> >, Optional<Attribute<mpl::int_<500>, Name::Strict<105> > >, Element<Attribute<Blocksnapshot::Xml::PAbsFilePath, Name::Strict<500> >, Name::Strict<2334> > > > marshal_type;

	static int parse(Blocksnapshot::Xml::Disk& , QStack<QDomElement>& );
	static int generate(const Blocksnapshot::Xml::Disk& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Domainblocksnapshot traits

template<>
struct Traits<Blocksnapshot::Xml::Domainblocksnapshot>
{
	typedef Unordered<mpl::vector<Optional<Element<Text<QString >, Name::Strict<107> > >, OneOrMore<Element<Blocksnapshot::Xml::Disk, Name::Strict<472> > > > > marshal_type;

	static int parse(Blocksnapshot::Xml::Domainblocksnapshot& , QStack<QDomElement>& );
	static int generate(const Blocksnapshot::Xml::Domainblocksnapshot& , QDomElement& );
};

} // namespace Libvirt

#endif // __BLOCKSNAPSHOT_TYPE_H__
