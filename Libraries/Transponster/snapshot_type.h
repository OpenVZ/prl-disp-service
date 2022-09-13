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
typedef Choice<mpl::vector<Attribute<Snapshot::Xml::ESnapshot, Name::Strict<462> >, Ordered<mpl::vector<Optional<Attribute<mpl::int_<464>, Name::Strict<462> > >, Attribute<Snapshot::Xml::PAbsFilePath, Name::Strict<500> > > > > > VMemoryImpl;
typedef VMemoryImpl::value_type VMemory;

} // namespace Xml
} // namespace Snapshot

///////////////////////////////////////////////////////////////////////////////
// struct Disk4374

namespace Snapshot
{
namespace Xml
{
struct Disk4374
{
};

} // namespace Xml
} // namespace Snapshot

///////////////////////////////////////////////////////////////////////////////
// struct Disk4375

namespace Snapshot
{
namespace Xml
{
struct Disk4375
{
};

} // namespace Xml
} // namespace Snapshot

///////////////////////////////////////////////////////////////////////////////
// struct Seclabel4345

namespace Snapshot
{
namespace Xml
{
struct Seclabel4345
{
};

} // namespace Xml
} // namespace Snapshot

///////////////////////////////////////////////////////////////////////////////
// struct Seclabel4346

namespace Snapshot
{
namespace Xml
{
struct Seclabel4346
{
};

} // namespace Xml
} // namespace Snapshot

///////////////////////////////////////////////////////////////////////////////
// struct VSeclabel

namespace Snapshot
{
namespace Xml
{
typedef Choice<mpl::vector<Seclabel4345, Seclabel4346, Ordered<mpl::vector<Optional<Attribute<mpl::int_<135>, Name::Strict<233> > >, OneOrMore<Element<Text<QString >, Name::Strict<234> > > > > > > VSeclabelImpl;
typedef VSeclabelImpl::value_type VSeclabel;

} // namespace Xml
} // namespace Snapshot

///////////////////////////////////////////////////////////////////////////////
// struct Seclabel

namespace Snapshot
{
namespace Xml
{
struct Seclabel
{
	const boost::optional<QString >& getModel() const
	{
		return m_model;
	}
	void setModel(const boost::optional<QString >& value_)
	{
		m_model = value_;
	}
	const VSeclabel& getSeclabel() const
	{
		return m_seclabel;
	}
	void setSeclabel(const VSeclabel& value_)
	{
		m_seclabel = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	boost::optional<QString > m_model;
	VSeclabel m_seclabel;
};

} // namespace Xml
} // namespace Snapshot

///////////////////////////////////////////////////////////////////////////////
// struct DiskSourceSlice

namespace Snapshot
{
namespace Xml
{
struct DiskSourceSlice
{
	DiskSourceSlice();

	PUnsignedInt::value_type getOffset() const
	{
		return m_offset;
	}
	void setOffset(PUnsignedInt::value_type value_)
	{
		m_offset = value_;
	}
	PPositiveInteger::value_type getSize() const
	{
		return m_size;
	}
	void setSize(PPositiveInteger::value_type value_)
	{
		m_size = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;

private:
	PUnsignedInt::value_type m_offset;
	PPositiveInteger::value_type m_size;
};

} // namespace Xml
} // namespace Snapshot

///////////////////////////////////////////////////////////////////////////////
// struct DiskSourceCommon

namespace Snapshot
{
namespace Xml
{
struct DiskSourceCommon
{
	const boost::optional<PPositiveInteger::value_type >& getIndex() const
	{
		return m_index;
	}
	void setIndex(const boost::optional<PPositiveInteger::value_type >& value_)
	{
		m_index = value_;
	}
	const boost::optional<DiskSourceSlice >& getSlices() const
	{
		return m_slices;
	}
	void setSlices(const boost::optional<DiskSourceSlice >& value_)
	{
		m_slices = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;

private:
	boost::optional<PPositiveInteger::value_type > m_index;
	boost::optional<DiskSourceSlice > m_slices;
};

} // namespace Xml
} // namespace Snapshot

///////////////////////////////////////////////////////////////////////////////
// struct VSecret

namespace Snapshot
{
namespace Xml
{
typedef Choice<mpl::vector<Attribute<Snapshot::Xml::VUUID, Name::Strict<151> >, Attribute<QString, Name::Strict<640> > > > VSecretImpl;
typedef VSecretImpl::value_type VSecret;

} // namespace Xml
} // namespace Snapshot

///////////////////////////////////////////////////////////////////////////////
// struct Anonymous2410

namespace Snapshot
{
namespace Xml
{
struct Anonymous2410
{
	const QString& getMode() const
	{
		return m_mode;
	}
	void setMode(const QString& value_)
	{
		m_mode = value_;
	}
	const QString& getHash() const
	{
		return m_hash;
	}
	void setHash(const QString& value_)
	{
		m_hash = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;

private:
	QString m_mode;
	QString m_hash;
};

} // namespace Xml
} // namespace Snapshot

///////////////////////////////////////////////////////////////////////////////
// struct Keycipher

namespace Snapshot
{
namespace Xml
{
struct Keycipher
{
	Keycipher();

	const QString& getName() const
	{
		return m_name;
	}
	void setName(const QString& value_)
	{
		m_name = value_;
	}
	PUnsignedInt::value_type getSize() const
	{
		return m_size;
	}
	void setSize(PUnsignedInt::value_type value_)
	{
		m_size = value_;
	}
	const boost::optional<Anonymous2410 >& getAnonymous2410() const
	{
		return m_anonymous2410;
	}
	void setAnonymous2410(const boost::optional<Anonymous2410 >& value_)
	{
		m_anonymous2410 = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	QString m_name;
	PUnsignedInt::value_type m_size;
	boost::optional<Anonymous2410 > m_anonymous2410;
};

} // namespace Xml
} // namespace Snapshot

///////////////////////////////////////////////////////////////////////////////
// struct Keyivgen

namespace Snapshot
{
namespace Xml
{
struct Keyivgen
{
	const QString& getName() const
	{
		return m_name;
	}
	void setName(const QString& value_)
	{
		m_name = value_;
	}
	const boost::optional<QString >& getHash() const
	{
		return m_hash;
	}
	void setHash(const boost::optional<QString >& value_)
	{
		m_hash = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	QString m_name;
	boost::optional<QString > m_hash;
};

} // namespace Xml
} // namespace Snapshot

///////////////////////////////////////////////////////////////////////////////
// struct Anonymous4935

namespace Snapshot
{
namespace Xml
{
struct Anonymous4935
{
	const Keycipher& getCipher() const
	{
		return m_cipher;
	}
	void setCipher(const Keycipher& value_)
	{
		m_cipher = value_;
	}
	const Keyivgen& getIvgen() const
	{
		return m_ivgen;
	}
	void setIvgen(const Keyivgen& value_)
	{
		m_ivgen = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;

private:
	Keycipher m_cipher;
	Keyivgen m_ivgen;
};

} // namespace Xml
} // namespace Snapshot

///////////////////////////////////////////////////////////////////////////////
// struct Encryption

namespace Snapshot
{
namespace Xml
{
struct Encryption
{
	Encryption();

	EFormat getFormat() const
	{
		return m_format;
	}
	void setFormat(EFormat value_)
	{
		m_format = value_;
	}
	const VSecret& getSecret() const
	{
		return m_secret;
	}
	void setSecret(const VSecret& value_)
	{
		m_secret = value_;
	}
	const boost::optional<Anonymous4935 >& getAnonymous4935() const
	{
		return m_anonymous4935;
	}
	void setAnonymous4935(const boost::optional<Anonymous4935 >& value_)
	{
		m_anonymous4935 = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	EFormat m_format;
	VSecret m_secret;
	boost::optional<Anonymous4935 > m_anonymous4935;
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
	const QList<Seclabel >& getSeclabelList() const
	{
		return m_seclabelList;
	}
	void setSeclabelList(const QList<Seclabel >& value_)
	{
		m_seclabelList = value_;
	}
	const boost::optional<DiskSourceCommon >& getDiskSourceCommon() const
	{
		return m_diskSourceCommon;
	}
	void setDiskSourceCommon(const boost::optional<DiskSourceCommon >& value_)
	{
		m_diskSourceCommon = value_;
	}
	const boost::optional<Encryption >& getEncryption() const
	{
		return m_encryption;
	}
	void setEncryption(const boost::optional<Encryption >& value_)
	{
		m_encryption = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	boost::optional<PAbsFilePath::value_type > m_file;
	boost::optional<EStartupPolicy > m_startupPolicy;
	QList<Seclabel > m_seclabelList;
	boost::optional<DiskSourceCommon > m_diskSourceCommon;
	boost::optional<Encryption > m_encryption;
};

} // namespace Xml
} // namespace Snapshot

///////////////////////////////////////////////////////////////////////////////
// struct ScaledInteger

namespace Snapshot
{
namespace Xml
{
struct ScaledInteger
{
	ScaledInteger();

	const boost::optional<PUnit::value_type >& getUnit() const
	{
		return m_unit;
	}
	void setUnit(const boost::optional<PUnit::value_type >& value_)
	{
		m_unit = value_;
	}
	PUnsignedLong::value_type getOwnValue() const
	{
		return m_ownValue;
	}
	void setOwnValue(PUnsignedLong::value_type value_)
	{
		m_ownValue = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	boost::optional<PUnit::value_type > m_unit;
	PUnsignedLong::value_type m_ownValue;
};

} // namespace Xml
} // namespace Snapshot

///////////////////////////////////////////////////////////////////////////////
// struct MetadataCache

namespace Snapshot
{
namespace Xml
{
struct MetadataCache
{
	const boost::optional<ScaledInteger >& getMaxSize() const
	{
		return m_maxSize;
	}
	void setMaxSize(const boost::optional<ScaledInteger >& value_)
	{
		m_maxSize = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	boost::optional<ScaledInteger > m_maxSize;
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
	const boost::optional<MetadataCache >& getMetadataCache() const
	{
		return m_metadataCache;
	}
	void setMetadataCache(const boost::optional<MetadataCache >& value_)
	{
		m_metadataCache = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	boost::optional<EStorageFormatBacking > m_type;
	boost::optional<MetadataCache > m_metadataCache;
};

} // namespace Xml
} // namespace Snapshot

///////////////////////////////////////////////////////////////////////////////
// struct Variant8729

namespace Snapshot
{
namespace Xml
{
struct Variant8729
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
// struct Source1

namespace Snapshot
{
namespace Xml
{
struct Source1
{
	const PAbsFilePath::value_type& getDev() const
	{
		return m_dev;
	}
	void setDev(const PAbsFilePath::value_type& value_)
	{
		m_dev = value_;
	}
	const QList<Seclabel >& getSeclabelList() const
	{
		return m_seclabelList;
	}
	void setSeclabelList(const QList<Seclabel >& value_)
	{
		m_seclabelList = value_;
	}
	const boost::optional<DiskSourceCommon >& getDiskSourceCommon() const
	{
		return m_diskSourceCommon;
	}
	void setDiskSourceCommon(const boost::optional<DiskSourceCommon >& value_)
	{
		m_diskSourceCommon = value_;
	}
	const boost::optional<Encryption >& getEncryption() const
	{
		return m_encryption;
	}
	void setEncryption(const boost::optional<Encryption >& value_)
	{
		m_encryption = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	PAbsFilePath::value_type m_dev;
	QList<Seclabel > m_seclabelList;
	boost::optional<DiskSourceCommon > m_diskSourceCommon;
	boost::optional<Encryption > m_encryption;
};

} // namespace Xml
} // namespace Snapshot

///////////////////////////////////////////////////////////////////////////////
// struct Variant8730

namespace Snapshot
{
namespace Xml
{
struct Variant8730
{
	const boost::optional<Source1 >& getSource() const
	{
		return m_source;
	}
	void setSource(const boost::optional<Source1 >& value_)
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
	boost::optional<Source1 > m_source;
	boost::optional<Driver > m_driver;
};

} // namespace Xml
} // namespace Snapshot

///////////////////////////////////////////////////////////////////////////////
// struct Host7324

namespace Snapshot
{
namespace Xml
{
struct Host7324
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
typedef Choice<mpl::vector<Host7324, Ordered<mpl::vector<Attribute<mpl::int_<520>, Name::Strict<514> >, Attribute<Snapshot::Xml::PAbsFilePath, Name::Strict<521> > > > > > VHostImpl;
typedef VHostImpl::value_type VHost;

} // namespace Xml
} // namespace Snapshot

///////////////////////////////////////////////////////////////////////////////
// struct Source2

namespace Snapshot
{
namespace Xml
{
struct Source2
{
	Source2();

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
// struct VChoice8731

namespace Snapshot
{
namespace Xml
{
typedef Choice<mpl::vector<Variant8729, Variant8730, Ordered<mpl::vector<Attribute<mpl::int_<445>, Name::Strict<105> >, Element<Snapshot::Xml::Source2, Name::Strict<501> > > > > > VChoice8731Impl;
typedef VChoice8731Impl::value_type VChoice8731;

} // namespace Xml
} // namespace Snapshot

///////////////////////////////////////////////////////////////////////////////
// struct VDisk

namespace Snapshot
{
namespace Xml
{
typedef Choice<mpl::vector<Disk4374, Disk4375, Ordered<mpl::vector<Optional<Attribute<mpl::int_<464>, Name::Strict<462> > >, Snapshot::Xml::VChoice8731Impl > > > > VDiskImpl;
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
// struct VChoice7295

namespace Snapshot
{
namespace Xml
{
typedef Choice<mpl::vector<Element<Element<Text<Snapshot::Xml::VUUID >, Name::Strict<151> >, Name::Strict<1> >, Element<Domain::Xml::Domain, Name::Strict<1> > > > VChoice7295Impl;
typedef VChoice7295Impl::value_type VChoice7295;

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
	const boost::optional<VChoice7295 >& getChoice7295() const
	{
		return m_choice7295;
	}
	void setChoice7295(const boost::optional<VChoice7295 >& value_)
	{
		m_choice7295 = value_;
	}
	const boost::optional<Domain::Xml::Domain >& getXPersistent() const
	{
		return m_xPersistent;
	}
	void setXPersistent(const boost::optional<Domain::Xml::Domain >& value_)
	{
		m_xPersistent = value_;
	}
	const boost::optional<QList<QDomElement > >& getInactiveDomain() const
	{
		return m_inactiveDomain;
	}
	void setInactiveDomain(const boost::optional<QList<QDomElement > >& value_)
	{
		m_inactiveDomain = value_;
	}
	const boost::optional<QString >& getParent() const
	{
		return m_parent;
	}
	void setParent(const boost::optional<QString >& value_)
	{
		m_parent = value_;
	}
	const boost::optional<QList<QDomElement > >& getCookie() const
	{
		return m_cookie;
	}
	void setCookie(const boost::optional<QList<QDomElement > >& value_)
	{
		m_cookie = value_;
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
	boost::optional<VChoice7295 > m_choice7295;
	boost::optional<Domain::Xml::Domain > m_xPersistent;
	boost::optional<QList<QDomElement > > m_inactiveDomain;
	boost::optional<QString > m_parent;
	boost::optional<QList<QDomElement > > m_cookie;
};

} // namespace Xml
} // namespace Snapshot

///////////////////////////////////////////////////////////////////////////////
// struct Disk4374 traits

template<>
struct Traits<Snapshot::Xml::Disk4374>
{
	typedef Attribute<mpl::int_<136>, Name::Strict<462> > marshal_type;

	static int parse(Snapshot::Xml::Disk4374& , QStack<QDomElement>& );
	static int generate(const Snapshot::Xml::Disk4374& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Disk4375 traits

template<>
struct Traits<Snapshot::Xml::Disk4375>
{
	typedef Attribute<mpl::int_<463>, Name::Strict<462> > marshal_type;

	static int parse(Snapshot::Xml::Disk4375& , QStack<QDomElement>& );
	static int generate(const Snapshot::Xml::Disk4375& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Seclabel4345 traits

template<>
struct Traits<Snapshot::Xml::Seclabel4345>
{
	typedef Ordered<mpl::vector<Attribute<mpl::int_<136>, Name::Strict<233> > > > marshal_type;

	static int parse(Snapshot::Xml::Seclabel4345& , QStack<QDomElement>& );
	static int generate(const Snapshot::Xml::Seclabel4345& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Seclabel4346 traits

template<>
struct Traits<Snapshot::Xml::Seclabel4346>
{
	typedef Ordered<mpl::vector<Attribute<mpl::int_<135>, Name::Strict<244> > > > marshal_type;

	static int parse(Snapshot::Xml::Seclabel4346& , QStack<QDomElement>& );
	static int generate(const Snapshot::Xml::Seclabel4346& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Seclabel traits

template<>
struct Traits<Snapshot::Xml::Seclabel>
{
	typedef Ordered<mpl::vector<Optional<Attribute<QString, Name::Strict<231> > >, Snapshot::Xml::VSeclabelImpl > > marshal_type;

	static int parse(Snapshot::Xml::Seclabel& , QStack<QDomElement>& );
	static int generate(const Snapshot::Xml::Seclabel& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct DiskSourceSlice traits

template<>
struct Traits<Snapshot::Xml::DiskSourceSlice>
{
	typedef Ordered<mpl::vector<Attribute<Snapshot::Xml::PUnsignedInt, Name::Strict<389> >, Attribute<Snapshot::Xml::PPositiveInteger, Name::Strict<334> > > > marshal_type;

	static int parse(Snapshot::Xml::DiskSourceSlice& , QStack<QDomElement>& );
	static int generate(const Snapshot::Xml::DiskSourceSlice& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct DiskSourceCommon traits

template<>
struct Traits<Snapshot::Xml::DiskSourceCommon>
{
	typedef Ordered<mpl::vector<Optional<Attribute<Snapshot::Xml::PPositiveInteger, Name::Strict<488> > >, Optional<Element<Element<Ordered<mpl::vector<Attribute<mpl::int_<938>, Name::Strict<105> >, Fragment<Snapshot::Xml::DiskSourceSlice > > >, Name::Strict<5842> >, Name::Strict<5841> > > > > marshal_type;

	static int parse(Snapshot::Xml::DiskSourceCommon& , QStack<QDomElement>& );
	static int generate(const Snapshot::Xml::DiskSourceCommon& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Anonymous2410 traits

template<>
struct Traits<Snapshot::Xml::Anonymous2410>
{
	typedef Ordered<mpl::vector<Attribute<QString, Name::Strict<379> >, Attribute<QString, Name::Strict<5769> > > > marshal_type;

	static int parse(Snapshot::Xml::Anonymous2410& , QStack<QDomElement>& );
	static int generate(const Snapshot::Xml::Anonymous2410& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Keycipher traits

template<>
struct Traits<Snapshot::Xml::Keycipher>
{
	typedef Ordered<mpl::vector<Attribute<QString, Name::Strict<107> >, Attribute<Snapshot::Xml::PUnsignedInt, Name::Strict<334> >, Optional<Fragment<Snapshot::Xml::Anonymous2410 > > > > marshal_type;

	static int parse(Snapshot::Xml::Keycipher& , QStack<QDomElement>& );
	static int generate(const Snapshot::Xml::Keycipher& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Keyivgen traits

template<>
struct Traits<Snapshot::Xml::Keyivgen>
{
	typedef Ordered<mpl::vector<Attribute<QString, Name::Strict<107> >, Optional<Attribute<QString, Name::Strict<5769> > > > > marshal_type;

	static int parse(Snapshot::Xml::Keyivgen& , QStack<QDomElement>& );
	static int generate(const Snapshot::Xml::Keyivgen& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Anonymous4935 traits

template<>
struct Traits<Snapshot::Xml::Anonymous4935>
{
	typedef Ordered<mpl::vector<Element<Snapshot::Xml::Keycipher, Name::Strict<5049> >, Element<Snapshot::Xml::Keyivgen, Name::Strict<5751> > > > marshal_type;

	static int parse(Snapshot::Xml::Anonymous4935& , QStack<QDomElement>& );
	static int generate(const Snapshot::Xml::Anonymous4935& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Encryption traits

template<>
struct Traits<Snapshot::Xml::Encryption>
{
	typedef Ordered<mpl::vector<Attribute<Snapshot::Xml::EFormat, Name::Strict<146> >, Unordered<mpl::vector<Element<Ordered<mpl::vector<Attribute<mpl::int_<150>, Name::Strict<105> >, Snapshot::Xml::VSecretImpl > >, Name::Strict<149> >, Optional<Fragment<Snapshot::Xml::Anonymous4935 > > > > > > marshal_type;

	static int parse(Snapshot::Xml::Encryption& , QStack<QDomElement>& );
	static int generate(const Snapshot::Xml::Encryption& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Source traits

template<>
struct Traits<Snapshot::Xml::Source>
{
	typedef Ordered<mpl::vector<Optional<Attribute<Snapshot::Xml::PAbsFilePath, Name::Strict<500> > >, Optional<Attribute<Snapshot::Xml::EStartupPolicy, Name::Strict<468> > >, ZeroOrMore<Element<Snapshot::Xml::Seclabel, Name::Strict<229> > >, Optional<Fragment<Snapshot::Xml::DiskSourceCommon > >, Optional<Element<Snapshot::Xml::Encryption, Name::Strict<145> > > > > marshal_type;

	static int parse(Snapshot::Xml::Source& , QStack<QDomElement>& );
	static int generate(const Snapshot::Xml::Source& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct ScaledInteger traits

template<>
struct Traits<Snapshot::Xml::ScaledInteger>
{
	typedef Ordered<mpl::vector<Optional<Attribute<Snapshot::Xml::PUnit, Name::Strict<66> > >, Text<Snapshot::Xml::PUnsignedLong > > > marshal_type;

	static int parse(Snapshot::Xml::ScaledInteger& , QStack<QDomElement>& );
	static int generate(const Snapshot::Xml::ScaledInteger& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct MetadataCache traits

template<>
struct Traits<Snapshot::Xml::MetadataCache>
{
	typedef Ordered<mpl::vector<Optional<Element<Snapshot::Xml::ScaledInteger, Name::Strict<6547> > > > > marshal_type;

	static int parse(Snapshot::Xml::MetadataCache& , QStack<QDomElement>& );
	static int generate(const Snapshot::Xml::MetadataCache& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Driver traits

template<>
struct Traits<Snapshot::Xml::Driver>
{
	typedef Ordered<mpl::vector<Optional<Attribute<Snapshot::Xml::EStorageFormatBacking, Name::Strict<105> > >, Optional<Element<Snapshot::Xml::MetadataCache, Name::Strict<6546> > > > > marshal_type;

	static int parse(Snapshot::Xml::Driver& , QStack<QDomElement>& );
	static int generate(const Snapshot::Xml::Driver& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Variant8729 traits

template<>
struct Traits<Snapshot::Xml::Variant8729>
{
	typedef Ordered<mpl::vector<Optional<Attribute<mpl::int_<500>, Name::Strict<105> > >, Unordered<mpl::vector<Optional<Element<Snapshot::Xml::Source, Name::Strict<501> > >, Optional<Element<Snapshot::Xml::Driver, Name::Strict<546> > > > > > > marshal_type;

	static int parse(Snapshot::Xml::Variant8729& , QStack<QDomElement>& );
	static int generate(const Snapshot::Xml::Variant8729& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Source1 traits

template<>
struct Traits<Snapshot::Xml::Source1>
{
	typedef Ordered<mpl::vector<Attribute<Snapshot::Xml::PAbsFilePath, Name::Strict<441> >, ZeroOrMore<Element<Snapshot::Xml::Seclabel, Name::Strict<229> > >, Optional<Fragment<Snapshot::Xml::DiskSourceCommon > >, Optional<Element<Snapshot::Xml::Encryption, Name::Strict<145> > > > > marshal_type;

	static int parse(Snapshot::Xml::Source1& , QStack<QDomElement>& );
	static int generate(const Snapshot::Xml::Source1& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Variant8730 traits

template<>
struct Traits<Snapshot::Xml::Variant8730>
{
	typedef Ordered<mpl::vector<Attribute<mpl::int_<502>, Name::Strict<105> >, Unordered<mpl::vector<Optional<Element<Snapshot::Xml::Source1, Name::Strict<501> > >, Optional<Element<Snapshot::Xml::Driver, Name::Strict<546> > > > > > > marshal_type;

	static int parse(Snapshot::Xml::Variant8730& , QStack<QDomElement>& );
	static int generate(const Snapshot::Xml::Variant8730& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Host7324 traits

template<>
struct Traits<Snapshot::Xml::Host7324>
{
	typedef Ordered<mpl::vector<Optional<Attribute<Snapshot::Xml::ETransport, Name::Strict<514> > >, Attribute<Snapshot::Xml::VName1, Name::Strict<107> >, Optional<Attribute<Snapshot::Xml::PUnsignedInt, Name::Strict<212> > > > > marshal_type;

	static int parse(Snapshot::Xml::Host7324& , QStack<QDomElement>& );
	static int generate(const Snapshot::Xml::Host7324& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Source2 traits

template<>
struct Traits<Snapshot::Xml::Source2>
{
	typedef Ordered<mpl::vector<Attribute<Snapshot::Xml::EProtocol, Name::Strict<203> >, Optional<Attribute<QString, Name::Strict<107> > >, ZeroOrMore<Element<Snapshot::Xml::VHostImpl, Name::Strict<513> > > > > marshal_type;

	static int parse(Snapshot::Xml::Source2& , QStack<QDomElement>& );
	static int generate(const Snapshot::Xml::Source2& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Disk traits

template<>
struct Traits<Snapshot::Xml::Disk>
{
	typedef Ordered<mpl::vector<Attribute<Snapshot::Xml::VName, Name::Strict<107> >, Snapshot::Xml::VDiskImpl > > marshal_type;

	static int parse(Snapshot::Xml::Disk& , QStack<QDomElement>& );
	static int generate(const Snapshot::Xml::Disk& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Domainsnapshot traits

template<>
struct Traits<Snapshot::Xml::Domainsnapshot>
{
	typedef Unordered<mpl::vector<Optional<Element<Text<QString >, Name::Strict<107> > >, Optional<Element<Text<QString >, Name::Strict<216> > >, Optional<Element<Text<Snapshot::Xml::EState >, Name::Strict<126> > >, Optional<Element<Text<QString >, Name::Strict<1584> > >, Optional<Element<Snapshot::Xml::VMemoryImpl, Name::Strict<326> > >, Optional<Element<ZeroOrMore<Element<Snapshot::Xml::Disk, Name::Strict<472> > >, Name::Strict<1587> > >, Optional<Element<Text<Snapshot::Xml::EActive >, Name::Strict<1342> > >, Optional<Snapshot::Xml::VChoice7295Impl >, Optional<Element<Element<Domain::Xml::Domain, Name::Strict<1> >, Name::Strict<4295> > >, Optional<Element<ZeroOrMore<Pod >, Name::Strict<5699> > >, Optional<Element<Element<Text<QString >, Name::Strict<107> >, Name::Strict<116> > >, Optional<Element<ZeroOrMore<Pod >, Name::Strict<1817> > > > > marshal_type;

	static int parse(Snapshot::Xml::Domainsnapshot& , QStack<QDomElement>& );
	static int generate(const Snapshot::Xml::Domainsnapshot& , QDomElement& );
};

} // namespace Libvirt

#endif // __SNAPSHOT_TYPE_H__
