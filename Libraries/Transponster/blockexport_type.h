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

#ifndef __BLOCKEXPORT_TYPE_H__
#define __BLOCKEXPORT_TYPE_H__
#include "base.h"
#include "blockexport_data.h"
#include "blockexport_enum.h"
#include "patterns.h"
#include <boost/any.hpp>

namespace Libvirt
{

///////////////////////////////////////////////////////////////////////////////
// struct Address

namespace Blockexport
{
namespace Xml
{
struct Address
{
	const PAddrIPorName::value_type& getHost() const
	{
		return m_host;
	}
	void setHost(const PAddrIPorName::value_type& value_)
	{
		m_host = value_;
	}
	const boost::optional<PPortNumber::value_type >& getPort() const
	{
		return m_port;
	}
	void setPort(const boost::optional<PPortNumber::value_type >& value_)
	{
		m_port = value_;
	}
	const boost::optional<EVirYesNo >& getAutoport() const
	{
		return m_autoport;
	}
	void setAutoport(const boost::optional<EVirYesNo >& value_)
	{
		m_autoport = value_;
	}
	const boost::optional<EVirYesNo >& getTls() const
	{
		return m_tls;
	}
	void setTls(const boost::optional<EVirYesNo >& value_)
	{
		m_tls = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	PAddrIPorName::value_type m_host;
	boost::optional<PPortNumber::value_type > m_port;
	boost::optional<EVirYesNo > m_autoport;
	boost::optional<EVirYesNo > m_tls;
};

} // namespace Xml
} // namespace Blockexport

///////////////////////////////////////////////////////////////////////////////
// struct VChoice3969

namespace Blockexport
{
namespace Xml
{
typedef Choice<mpl::vector<Attribute<QString, Name::Strict<1180> >, Attribute<Blockexport::Xml::EVirYesNo, Name::Strict<1182> > > > VChoice3969Impl;
typedef VChoice3969Impl::value_type VChoice3969;

} // namespace Xml
} // namespace Blockexport

///////////////////////////////////////////////////////////////////////////////
// struct Disk

namespace Blockexport
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
	const boost::optional<QString >& getExportname() const
	{
		return m_exportname;
	}
	void setExportname(const boost::optional<QString >& value_)
	{
		m_exportname = value_;
	}
	const boost::optional<QString >& getSnapshot() const
	{
		return m_snapshot;
	}
	void setSnapshot(const boost::optional<QString >& value_)
	{
		m_snapshot = value_;
	}
	const boost::optional<VChoice3969 >& getChoice3969() const
	{
		return m_choice3969;
	}
	void setChoice3969(const boost::optional<VChoice3969 >& value_)
	{
		m_choice3969 = value_;
	}
	const boost::optional<EVirYesNo >& getReadonly() const
	{
		return m_readonly;
	}
	void setReadonly(const boost::optional<EVirYesNo >& value_)
	{
		m_readonly = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	VName m_name;
	boost::optional<QString > m_exportname;
	boost::optional<QString > m_snapshot;
	boost::optional<VChoice3969 > m_choice3969;
	boost::optional<EVirYesNo > m_readonly;
};

} // namespace Xml
} // namespace Blockexport

///////////////////////////////////////////////////////////////////////////////
// struct Domainblockexport_

namespace Blockexport
{
namespace Xml
{
struct Domainblockexport_
{
	const Address& getAddress() const
	{
		return m_address;
	}
	void setAddress(const Address& value_)
	{
		m_address = value_;
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
	Address m_address;
	QList<Disk > m_diskList;
};

} // namespace Xml
} // namespace Blockexport

///////////////////////////////////////////////////////////////////////////////
// struct Address traits

template<>
struct Traits<Blockexport::Xml::Address>
{
	typedef Ordered<mpl::vector<Attribute<mpl::int_<689>, Name::Strict<105> >, Attribute<Blockexport::Xml::PAddrIPorName, Name::Strict<513> >, Optional<Attribute<Blockexport::Xml::PPortNumber, Name::Strict<212> > >, Optional<Attribute<Blockexport::Xml::EVirYesNo, Name::Strict<719> > >, Optional<Attribute<Blockexport::Xml::EVirYesNo, Name::Strict<863> > > > > marshal_type;

	static int parse(Blockexport::Xml::Address& , QStack<QDomElement>& );
	static int generate(const Blockexport::Xml::Address& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Disk traits

template<>
struct Traits<Blockexport::Xml::Disk>
{
	typedef Ordered<mpl::vector<Attribute<Blockexport::Xml::VName, Name::Strict<107> >, Optional<Attribute<QString, Name::Strict<1179> > >, Optional<Attribute<QString, Name::Strict<462> > >, Optional<Blockexport::Xml::VChoice3969Impl >, Optional<Attribute<Blockexport::Xml::EVirYesNo, Name::Strict<274> > > > > marshal_type;

	static int parse(Blockexport::Xml::Disk& , QStack<QDomElement>& );
	static int generate(const Blockexport::Xml::Disk& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Domainblockexport_ traits

template<>
struct Traits<Blockexport::Xml::Domainblockexport_>
{
	typedef Ordered<mpl::vector<Attribute<mpl::int_<503>, Name::Strict<105> >, Unordered<mpl::vector<Element<Blockexport::Xml::Address, Name::Strict<111> >, OneOrMore<Element<Blockexport::Xml::Disk, Name::Strict<472> > > > > > > marshal_type;

	static int parse(Blockexport::Xml::Domainblockexport_& , QStack<QDomElement>& );
	static int generate(const Blockexport::Xml::Domainblockexport_& , QDomElement& );
};

} // namespace Libvirt

#endif // __BLOCKEXPORT_TYPE_H__
