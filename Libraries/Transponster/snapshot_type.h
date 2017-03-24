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

#ifndef __SNAPSHOT_TYPE_H__
#define __SNAPSHOT_TYPE_H__
#include "base.h"
#include "snapshot_data.h"
#include "snapshot_enum.h"
#include "patterns.h"
#include "domain_type.h"
#include <boost/any.hpp>

namespace Libvirt
{

///////////////////////////////////////////////////////////////////////////////
// struct VMemory

namespace Snapshot
{
namespace Xml
{
typedef Choice<mpl::vector<Attribute<Snapshot::Xml::ESnapshot, Name::Strict<448> >, Ordered<mpl::vector<Optional<Attribute<mpl::int_<450>, Name::Strict<448> > >, Attribute<Snapshot::Xml::PAbsFilePath, Name::Strict<486> > > > > > VMemoryImpl;
typedef VMemoryImpl::value_type VMemory;

} // namespace Xml
} // namespace Snapshot

///////////////////////////////////////////////////////////////////////////////
// struct Disk1752

namespace Snapshot
{
namespace Xml
{
struct Disk1752
{
};

} // namespace Xml
} // namespace Snapshot

///////////////////////////////////////////////////////////////////////////////
// struct Disk1753

namespace Snapshot
{
namespace Xml
{
struct Disk1753
{
};

} // namespace Xml
} // namespace Snapshot

///////////////////////////////////////////////////////////////////////////////
// struct Source

namespace Snapshot
{
namespace Xml
{
struct Source
{
	const boost::optional<PAbsFilePath::value_type >& getFile() const
	{
		return m_file;
	}
	void setFile(const boost::optional<PAbsFilePath::value_type >& value_)
	{
		m_file = value_;
	}
	const boost::optional<EStartupPolicy >& getStartupPolicy() const
	{
		return m_startupPolicy;
	}
	void setStartupPolicy(const boost::optional<EStartupPolicy >& value_)
	{
		m_startupPolicy = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	boost::optional<PAbsFilePath::value_type > m_file;
	boost::optional<EStartupPolicy > m_startupPolicy;
};

} // namespace Xml
} // namespace Snapshot

///////////////////////////////////////////////////////////////////////////////
// struct Driver

namespace Snapshot
{
namespace Xml
{
struct Driver
{
	const boost::optional<EStorageFormatBacking >& getType() const
	{
		return m_type;
	}
	void setType(const boost::optional<EStorageFormatBacking >& value_)
	{
		m_type = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	boost::optional<EStorageFormatBacking > m_type;
};

} // namespace Xml
} // namespace Snapshot

///////////////////////////////////////////////////////////////////////////////
// struct Variant1747

namespace Snapshot
{
namespace Xml
{
struct Variant1747
{
	const boost::optional<Source >& getSource() const
	{
		return m_source;
	}
	void setSource(const boost::optional<Source >& value_)
	{
		m_source = value_;
	}
	const boost::optional<Driver >& getDriver() const
	{
		return m_driver;
	}
	void setDriver(const boost::optional<Driver >& value_)
	{
		m_driver = value_;
	}

private:
	boost::optional<Source > m_source;
	boost::optional<Driver > m_driver;
};

} // namespace Xml
} // namespace Snapshot

///////////////////////////////////////////////////////////////////////////////
// struct Variant1748

namespace Snapshot
{
namespace Xml
{
struct Variant1748
{
	const boost::optional<PAbsFilePath::value_type >& getSource() const
	{
		return m_source;
	}
	void setSource(const boost::optional<PAbsFilePath::value_type >& value_)
	{
		m_source = value_;
	}
	const boost::optional<Driver >& getDriver() const
	{
		return m_driver;
	}
	void setDriver(const boost::optional<Driver >& value_)
	{
		m_driver = value_;
	}

private:
	boost::optional<PAbsFilePath::value_type > m_source;
	boost::optional<Driver > m_driver;
};

} // namespace Xml
} // namespace Snapshot

///////////////////////////////////////////////////////////////////////////////
// struct Host1593

namespace Snapshot
{
namespace Xml
{
struct Host1593
{
	const boost::optional<ETransport >& getTransport() const
	{
		return m_transport;
	}
	void setTransport(const boost::optional<ETransport >& value_)
	{
		m_transport = value_;
	}
	const VName1& getName() const
	{
		return m_name;
	}
	void setName(const VName1& value_)
	{
		m_name = value_;
	}
	const boost::optional<PUnsignedInt::value_type >& getPort() const
	{
		return m_port;
	}
	void setPort(const boost::optional<PUnsignedInt::value_type >& value_)
	{
		m_port = value_;
	}

private:
	boost::optional<ETransport > m_transport;
	VName1 m_name;
	boost::optional<PUnsignedInt::value_type > m_port;
};

} // namespace Xml
} // namespace Snapshot

///////////////////////////////////////////////////////////////////////////////
// struct VHost

namespace Snapshot
{
namespace Xml
{
typedef Choice<mpl::vector<Host1593, Ordered<mpl::vector<Attribute<mpl::int_<506>, Name::Strict<500> >, Attribute<Snapshot::Xml::PAbsFilePath, Name::Strict<507> > > > > > VHostImpl;
typedef VHostImpl::value_type VHost;

} // namespace Xml
} // namespace Snapshot

///////////////////////////////////////////////////////////////////////////////
// struct Source1

namespace Snapshot
{
namespace Xml
{
struct Source1
{
	Source1();

	EProtocol getProtocol() const
	{
		return m_protocol;
	}
	void setProtocol(EProtocol value_)
	{
		m_protocol = value_;
	}
	const boost::optional<QString >& getName() const
	{
		return m_name;
	}
	void setName(const boost::optional<QString >& value_)
	{
		m_name = value_;
	}
	const QList<VHost >& getHostList() const
	{
		return m_hostList;
	}
	void setHostList(const QList<VHost >& value_)
	{
		m_hostList = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	EProtocol m_protocol;
	boost::optional<QString > m_name;
	QList<VHost > m_hostList;
};

} // namespace Xml
} // namespace Snapshot

///////////////////////////////////////////////////////////////////////////////
// struct VChoice1750

namespace Snapshot
{
namespace Xml
{
typedef Choice<mpl::vector<Variant1747, Variant1748, Ordered<mpl::vector<Attribute<mpl::int_<432>, Name::Strict<100> >, Element<Snapshot::Xml::Source1, Name::Strict<487> > > > > > VChoice1750Impl;
typedef VChoice1750Impl::value_type VChoice1750;

} // namespace Xml
} // namespace Snapshot

///////////////////////////////////////////////////////////////////////////////
// struct VDisk

namespace Snapshot
{
namespace Xml
{
typedef Choice<mpl::vector<Disk1752, Disk1753, Ordered<mpl::vector<Optional<Attribute<mpl::int_<450>, Name::Strict<448> > >, Snapshot::Xml::VChoice1750Impl > > > > VDiskImpl;
typedef VDiskImpl::value_type VDisk;

} // namespace Xml
} // namespace Snapshot

///////////////////////////////////////////////////////////////////////////////
// struct Disk

namespace Snapshot
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
	const VDisk& getDisk() const
	{
		return m_disk;
	}
	void setDisk(const VDisk& value_)
	{
		m_disk = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	VName m_name;
	VDisk m_disk;
};

} // namespace Xml
} // namespace Snapshot

///////////////////////////////////////////////////////////////////////////////
// struct VChoice1734

namespace Snapshot
{
namespace Xml
{
typedef Choice<mpl::vector<Element<Element<Text<Snapshot::Xml::VUUID >, Name::Strict<140> >, Name::Strict<1> >, Element<Domain::Xml::Domain, Name::Strict<1> > > > VChoice1734Impl;
typedef VChoice1734Impl::value_type VChoice1734;

} // namespace Xml
} // namespace Snapshot

///////////////////////////////////////////////////////////////////////////////
// struct Domainsnapshot

namespace Snapshot
{
namespace Xml
{
struct Domainsnapshot
{
	const boost::optional<QString >& getName() const
	{
		return m_name;
	}
	void setName(const boost::optional<QString >& value_)
	{
		m_name = value_;
	}
	const boost::optional<QString >& getDescription() const
	{
		return m_description;
	}
	void setDescription(const boost::optional<QString >& value_)
	{
		m_description = value_;
	}
	const boost::optional<EState >& getState() const
	{
		return m_state;
	}
	void setState(const boost::optional<EState >& value_)
	{
		m_state = value_;
	}
	const boost::optional<QString >& getCreationTime() const
	{
		return m_creationTime;
	}
	void setCreationTime(const boost::optional<QString >& value_)
	{
		m_creationTime = value_;
	}
	const boost::optional<VMemory >& getMemory() const
	{
		return m_memory;
	}
	void setMemory(const boost::optional<VMemory >& value_)
	{
		m_memory = value_;
	}
	const boost::optional<QList<Disk > >& getDisks() const
	{
		return m_disks;
	}
	void setDisks(const boost::optional<QList<Disk > >& value_)
	{
		m_disks = value_;
	}
	const boost::optional<EActive >& getActive() const
	{
		return m_active;
	}
	void setActive(const boost::optional<EActive >& value_)
	{
		m_active = value_;
	}
	const boost::optional<VChoice1734 >& getChoice1734() const
	{
		return m_choice1734;
	}
	void setChoice1734(const boost::optional<VChoice1734 >& value_)
	{
		m_choice1734 = value_;
	}
	const boost::optional<QString >& getParent() const
	{
		return m_parent;
	}
	void setParent(const boost::optional<QString >& value_)
	{
		m_parent = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	boost::optional<QString > m_name;
	boost::optional<QString > m_description;
	boost::optional<EState > m_state;
	boost::optional<QString > m_creationTime;
	boost::optional<VMemory > m_memory;
	boost::optional<QList<Disk > > m_disks;
	boost::optional<EActive > m_active;
	boost::optional<VChoice1734 > m_choice1734;
	boost::optional<QString > m_parent;
};

} // namespace Xml
} // namespace Snapshot

///////////////////////////////////////////////////////////////////////////////
// struct Disk1752 traits

template<>
struct Traits<Snapshot::Xml::Disk1752>
{
	typedef Attribute<mpl::int_<131>, Name::Strict<448> > marshal_type;

	static int parse(Snapshot::Xml::Disk1752& , QStack<QDomElement>& );
	static int generate(const Snapshot::Xml::Disk1752& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Disk1753 traits

template<>
struct Traits<Snapshot::Xml::Disk1753>
{
	typedef Attribute<mpl::int_<449>, Name::Strict<448> > marshal_type;

	static int parse(Snapshot::Xml::Disk1753& , QStack<QDomElement>& );
	static int generate(const Snapshot::Xml::Disk1753& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Source traits

template<>
struct Traits<Snapshot::Xml::Source>
{
	typedef Ordered<mpl::vector<Optional<Attribute<Snapshot::Xml::PAbsFilePath, Name::Strict<486> > >, Optional<Attribute<Snapshot::Xml::EStartupPolicy, Name::Strict<454> > > > > marshal_type;

	static int parse(Snapshot::Xml::Source& , QStack<QDomElement>& );
	static int generate(const Snapshot::Xml::Source& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Driver traits

template<>
struct Traits<Snapshot::Xml::Driver>
{
	typedef Ordered<mpl::vector<Optional<Attribute<Snapshot::Xml::EStorageFormatBacking, Name::Strict<100> > > > > marshal_type;

	static int parse(Snapshot::Xml::Driver& , QStack<QDomElement>& );
	static int generate(const Snapshot::Xml::Driver& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Variant1747 traits

template<>
struct Traits<Snapshot::Xml::Variant1747>
{
	typedef Ordered<mpl::vector<Optional<Attribute<mpl::int_<486>, Name::Strict<100> > >, Unordered<mpl::vector<Optional<Element<Snapshot::Xml::Source, Name::Strict<487> > >, Optional<Element<Snapshot::Xml::Driver, Name::Strict<532> > > > > > > marshal_type;

	static int parse(Snapshot::Xml::Variant1747& , QStack<QDomElement>& );
	static int generate(const Snapshot::Xml::Variant1747& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Variant1748 traits

template<>
struct Traits<Snapshot::Xml::Variant1748>
{
	typedef Ordered<mpl::vector<Attribute<mpl::int_<488>, Name::Strict<100> >, Unordered<mpl::vector<Optional<Element<Attribute<Snapshot::Xml::PAbsFilePath, Name::Strict<428> >, Name::Strict<487> > >, Optional<Element<Snapshot::Xml::Driver, Name::Strict<532> > > > > > > marshal_type;

	static int parse(Snapshot::Xml::Variant1748& , QStack<QDomElement>& );
	static int generate(const Snapshot::Xml::Variant1748& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Host1593 traits

template<>
struct Traits<Snapshot::Xml::Host1593>
{
	typedef Ordered<mpl::vector<Optional<Attribute<Snapshot::Xml::ETransport, Name::Strict<500> > >, Attribute<Snapshot::Xml::VName1, Name::Strict<102> >, Optional<Attribute<Snapshot::Xml::PUnsignedInt, Name::Strict<201> > > > > marshal_type;

	static int parse(Snapshot::Xml::Host1593& , QStack<QDomElement>& );
	static int generate(const Snapshot::Xml::Host1593& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Source1 traits

template<>
struct Traits<Snapshot::Xml::Source1>
{
	typedef Ordered<mpl::vector<Attribute<Snapshot::Xml::EProtocol, Name::Strict<192> >, Optional<Attribute<QString, Name::Strict<102> > >, ZeroOrMore<Element<Snapshot::Xml::VHostImpl, Name::Strict<499> > > > > marshal_type;

	static int parse(Snapshot::Xml::Source1& , QStack<QDomElement>& );
	static int generate(const Snapshot::Xml::Source1& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Disk traits

template<>
struct Traits<Snapshot::Xml::Disk>
{
	typedef Ordered<mpl::vector<Attribute<Snapshot::Xml::VName, Name::Strict<102> >, Snapshot::Xml::VDiskImpl > > marshal_type;

	static int parse(Snapshot::Xml::Disk& , QStack<QDomElement>& );
	static int generate(const Snapshot::Xml::Disk& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Domainsnapshot traits

template<>
struct Traits<Snapshot::Xml::Domainsnapshot>
{
	typedef Unordered<mpl::vector<Optional<Element<Text<QString >, Name::Strict<102> > >, Optional<Element<Text<QString >, Name::Strict<202> > >, Optional<Element<Text<Snapshot::Xml::EState >, Name::Strict<121> > >, Optional<Element<Text<QString >, Name::Strict<1512> > >, Optional<Element<Snapshot::Xml::VMemoryImpl, Name::Strict<312> > >, Optional<Element<ZeroOrMore<Element<Snapshot::Xml::Disk, Name::Strict<458> > >, Name::Strict<1515> > >, Optional<Element<Text<Snapshot::Xml::EActive >, Name::Strict<1279> > >, Optional<Snapshot::Xml::VChoice1734Impl >, Optional<Element<Element<Text<QString >, Name::Strict<102> >, Name::Strict<111> > > > > marshal_type;

	static int parse(Snapshot::Xml::Domainsnapshot& , QStack<QDomElement>& );
	static int generate(const Snapshot::Xml::Domainsnapshot& , QDomElement& );
};

} // namespace Libvirt

#endif // __SNAPSHOT_TYPE_H__
