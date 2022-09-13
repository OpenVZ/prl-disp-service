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
// struct Disk4374

int Traits<Snapshot::Xml::Disk4374>::parse(Snapshot::Xml::Disk4374& , QStack<QDomElement>& stack_)
{
	marshal_type m;
	return m.consume(stack_);
}

int Traits<Snapshot::Xml::Disk4374>::generate(const Snapshot::Xml::Disk4374& , QDomElement& dst_)
{
	marshal_type m;
	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Disk4375

int Traits<Snapshot::Xml::Disk4375>::parse(Snapshot::Xml::Disk4375& , QStack<QDomElement>& stack_)
{
	marshal_type m;
	return m.consume(stack_);
}

int Traits<Snapshot::Xml::Disk4375>::generate(const Snapshot::Xml::Disk4375& , QDomElement& dst_)
{
	marshal_type m;
	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Seclabel4345

int Traits<Snapshot::Xml::Seclabel4345>::parse(Snapshot::Xml::Seclabel4345& , QStack<QDomElement>& stack_)
{
	marshal_type m;
	return m.consume(stack_);
}

int Traits<Snapshot::Xml::Seclabel4345>::generate(const Snapshot::Xml::Seclabel4345& , QDomElement& dst_)
{
	marshal_type m;
	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Seclabel4346

int Traits<Snapshot::Xml::Seclabel4346>::parse(Snapshot::Xml::Seclabel4346& , QStack<QDomElement>& stack_)
{
	marshal_type m;
	return m.consume(stack_);
}

int Traits<Snapshot::Xml::Seclabel4346>::generate(const Snapshot::Xml::Seclabel4346& , QDomElement& dst_)
{
	marshal_type m;
	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Seclabel

namespace Snapshot
{
namespace Xml
{
bool Seclabel::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Seclabel, Name::Strict<229> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Seclabel::save(QDomElement& dst_) const
{
	Element<Seclabel, Name::Strict<229> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Seclabel::save(QDomDocument& dst_) const
{
	Element<Seclabel, Name::Strict<229> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Snapshot

int Traits<Snapshot::Xml::Seclabel>::parse(Snapshot::Xml::Seclabel& dst_, QStack<QDomElement>& stack_)
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

int Traits<Snapshot::Xml::Seclabel>::generate(const Snapshot::Xml::Seclabel& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getModel(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getSeclabel(), m.get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct DiskSourceSlice

namespace Snapshot
{
namespace Xml
{
DiskSourceSlice::DiskSourceSlice(): m_offset(), m_size()
{
}

bool DiskSourceSlice::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	k.push(src_.firstChildElement());
	return 0 <= Traits<DiskSourceSlice>::parse(*this, k);
}

bool DiskSourceSlice::save(QDomElement& dst_) const
{
	return 0 <= Traits<DiskSourceSlice>::generate(*this, dst_);
}


} // namespace Xml
} // namespace Snapshot

int Traits<Snapshot::Xml::DiskSourceSlice>::parse(Snapshot::Xml::DiskSourceSlice& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setOffset(m.get<0>().getValue());
		dst_.setSize(m.get<1>().getValue());
	}
	return output;
}

int Traits<Snapshot::Xml::DiskSourceSlice>::generate(const Snapshot::Xml::DiskSourceSlice& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getOffset(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getSize(), m.get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct DiskSourceCommon

namespace Snapshot
{
namespace Xml
{
bool DiskSourceCommon::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	k.push(src_.firstChildElement());
	return 0 <= Traits<DiskSourceCommon>::parse(*this, k);
}

bool DiskSourceCommon::save(QDomElement& dst_) const
{
	return 0 <= Traits<DiskSourceCommon>::generate(*this, dst_);
}


} // namespace Xml
} // namespace Snapshot

int Traits<Snapshot::Xml::DiskSourceCommon>::parse(Snapshot::Xml::DiskSourceCommon& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setIndex(m.get<0>().getValue());
		dst_.setSlices(m.get<1>().getValue());
	}
	return output;
}

int Traits<Snapshot::Xml::DiskSourceCommon>::generate(const Snapshot::Xml::DiskSourceCommon& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getIndex(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getSlices(), m.get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Anonymous2410

namespace Snapshot
{
namespace Xml
{
bool Anonymous2410::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	k.push(src_.firstChildElement());
	return 0 <= Traits<Anonymous2410>::parse(*this, k);
}

bool Anonymous2410::save(QDomElement& dst_) const
{
	return 0 <= Traits<Anonymous2410>::generate(*this, dst_);
}


} // namespace Xml
} // namespace Snapshot

int Traits<Snapshot::Xml::Anonymous2410>::parse(Snapshot::Xml::Anonymous2410& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setMode(m.get<0>().getValue());
		dst_.setHash(m.get<1>().getValue());
	}
	return output;
}

int Traits<Snapshot::Xml::Anonymous2410>::generate(const Snapshot::Xml::Anonymous2410& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getMode(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getHash(), m.get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Keycipher

namespace Snapshot
{
namespace Xml
{
Keycipher::Keycipher(): m_size()
{
}

bool Keycipher::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Keycipher, Name::Strict<5049> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Keycipher::save(QDomElement& dst_) const
{
	Element<Keycipher, Name::Strict<5049> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Keycipher::save(QDomDocument& dst_) const
{
	Element<Keycipher, Name::Strict<5049> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Snapshot

int Traits<Snapshot::Xml::Keycipher>::parse(Snapshot::Xml::Keycipher& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setName(m.get<0>().getValue());
		dst_.setSize(m.get<1>().getValue());
		dst_.setAnonymous2410(m.get<2>().getValue());
	}
	return output;
}

int Traits<Snapshot::Xml::Keycipher>::generate(const Snapshot::Xml::Keycipher& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getName(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getSize(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getAnonymous2410(), m.get<2>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Keyivgen

namespace Snapshot
{
namespace Xml
{
bool Keyivgen::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Keyivgen, Name::Strict<5751> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Keyivgen::save(QDomElement& dst_) const
{
	Element<Keyivgen, Name::Strict<5751> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Keyivgen::save(QDomDocument& dst_) const
{
	Element<Keyivgen, Name::Strict<5751> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Snapshot

int Traits<Snapshot::Xml::Keyivgen>::parse(Snapshot::Xml::Keyivgen& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setName(m.get<0>().getValue());
		dst_.setHash(m.get<1>().getValue());
	}
	return output;
}

int Traits<Snapshot::Xml::Keyivgen>::generate(const Snapshot::Xml::Keyivgen& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getName(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getHash(), m.get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Anonymous4935

namespace Snapshot
{
namespace Xml
{
bool Anonymous4935::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	k.push(src_.firstChildElement());
	return 0 <= Traits<Anonymous4935>::parse(*this, k);
}

bool Anonymous4935::save(QDomElement& dst_) const
{
	return 0 <= Traits<Anonymous4935>::generate(*this, dst_);
}


} // namespace Xml
} // namespace Snapshot

int Traits<Snapshot::Xml::Anonymous4935>::parse(Snapshot::Xml::Anonymous4935& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setCipher(m.get<0>().getValue());
		dst_.setIvgen(m.get<1>().getValue());
	}
	return output;
}

int Traits<Snapshot::Xml::Anonymous4935>::generate(const Snapshot::Xml::Anonymous4935& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getCipher(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getIvgen(), m.get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Encryption

namespace Snapshot
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
	Element<Encryption, Name::Strict<145> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Encryption::save(QDomElement& dst_) const
{
	Element<Encryption, Name::Strict<145> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Encryption::save(QDomDocument& dst_) const
{
	Element<Encryption, Name::Strict<145> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Snapshot

int Traits<Snapshot::Xml::Encryption>::parse(Snapshot::Xml::Encryption& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setFormat(m.get<0>().getValue());
		dst_.setSecret(m.get<1>().get<0>().getValue());
		dst_.setAnonymous4935(m.get<1>().get<1>().getValue());
	}
	return output;
}

int Traits<Snapshot::Xml::Encryption>::generate(const Snapshot::Xml::Encryption& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getFormat(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getSecret(), m.get<1>().get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getAnonymous4935(), m.get<1>().get<1>()))
		return -1;

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
		dst_.setSeclabelList(m.get<2>().getValue());
		dst_.setDiskSourceCommon(m.get<3>().getValue());
		dst_.setEncryption(m.get<4>().getValue());
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
	if (0 > Details::Marshal::assign(src_.getSeclabelList(), m.get<2>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getDiskSourceCommon(), m.get<3>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getEncryption(), m.get<4>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct ScaledInteger

namespace Snapshot
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
	Element<ScaledInteger, Name::Strict<6547> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool ScaledInteger::save(QDomElement& dst_) const
{
	Element<ScaledInteger, Name::Strict<6547> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool ScaledInteger::save(QDomDocument& dst_) const
{
	Element<ScaledInteger, Name::Strict<6547> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Snapshot

int Traits<Snapshot::Xml::ScaledInteger>::parse(Snapshot::Xml::ScaledInteger& dst_, QStack<QDomElement>& stack_)
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

int Traits<Snapshot::Xml::ScaledInteger>::generate(const Snapshot::Xml::ScaledInteger& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getUnit(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getOwnValue(), m.get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct MetadataCache

namespace Snapshot
{
namespace Xml
{
bool MetadataCache::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<MetadataCache, Name::Strict<6546> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool MetadataCache::save(QDomElement& dst_) const
{
	Element<MetadataCache, Name::Strict<6546> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool MetadataCache::save(QDomDocument& dst_) const
{
	Element<MetadataCache, Name::Strict<6546> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Snapshot

int Traits<Snapshot::Xml::MetadataCache>::parse(Snapshot::Xml::MetadataCache& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setMaxSize(m.get<0>().getValue());
	}
	return output;
}

int Traits<Snapshot::Xml::MetadataCache>::generate(const Snapshot::Xml::MetadataCache& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getMaxSize(), m.get<0>()))
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
		dst_.setMetadataCache(m.get<1>().getValue());
	}
	return output;
}

int Traits<Snapshot::Xml::Driver>::generate(const Snapshot::Xml::Driver& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getType(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getMetadataCache(), m.get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Variant8729

int Traits<Snapshot::Xml::Variant8729>::parse(Snapshot::Xml::Variant8729& dst_, QStack<QDomElement>& stack_)
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

int Traits<Snapshot::Xml::Variant8729>::generate(const Snapshot::Xml::Variant8729& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getSource(), m.get<1>().get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getDriver(), m.get<1>().get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Source1

namespace Snapshot
{
namespace Xml
{
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
		dst_.setDev(m.get<0>().getValue());
		dst_.setSeclabelList(m.get<1>().getValue());
		dst_.setDiskSourceCommon(m.get<2>().getValue());
		dst_.setEncryption(m.get<3>().getValue());
	}
	return output;
}

int Traits<Snapshot::Xml::Source1>::generate(const Snapshot::Xml::Source1& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getDev(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getSeclabelList(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getDiskSourceCommon(), m.get<2>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getEncryption(), m.get<3>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Variant8730

int Traits<Snapshot::Xml::Variant8730>::parse(Snapshot::Xml::Variant8730& dst_, QStack<QDomElement>& stack_)
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

int Traits<Snapshot::Xml::Variant8730>::generate(const Snapshot::Xml::Variant8730& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getSource(), m.get<1>().get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getDriver(), m.get<1>().get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Host7324

int Traits<Snapshot::Xml::Host7324>::parse(Snapshot::Xml::Host7324& dst_, QStack<QDomElement>& stack_)
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

int Traits<Snapshot::Xml::Host7324>::generate(const Snapshot::Xml::Host7324& src_, QDomElement& dst_)
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

namespace Snapshot
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
	Element<Source2, Name::Strict<501> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Source2::save(QDomElement& dst_) const
{
	Element<Source2, Name::Strict<501> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Source2::save(QDomDocument& dst_) const
{
	Element<Source2, Name::Strict<501> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Snapshot

int Traits<Snapshot::Xml::Source2>::parse(Snapshot::Xml::Source2& dst_, QStack<QDomElement>& stack_)
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

int Traits<Snapshot::Xml::Source2>::generate(const Snapshot::Xml::Source2& src_, QDomElement& dst_)
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
		dst_.setChoice7295(m.get<7>().getValue());
		dst_.setXPersistent(m.get<8>().getValue());
		dst_.setInactiveDomain(m.get<9>().getValue());
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
	if (0 > Details::Marshal::assign(src_.getChoice7295(), m.get<7>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getXPersistent(), m.get<8>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getInactiveDomain(), m.get<9>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getParent(), m.get<10>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getCookie(), m.get<11>()))
		return -1;

	return m.produce(dst_);
}

} // namespace Libvirt
