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
// struct Enum

namespace Capability
{
namespace Xml
{
bool Enum::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Enum, Name::Strict<1873> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Enum::save(QDomElement& dst_) const
{
	Element<Enum, Name::Strict<1873> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Enum::save(QDomDocument& dst_) const
{
	Element<Enum, Name::Strict<1873> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Capability

int Traits<Capability::Xml::Enum>::parse(Capability::Xml::Enum& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setName(m.get<0>().getValue());
		dst_.setValueList(m.get<1>().getValue());
	}
	return output;
}

int Traits<Capability::Xml::Enum>::generate(const Capability::Xml::Enum& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getName(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getValueList(), m.get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Loader

namespace Capability
{
namespace Xml
{
Loader::Loader(): m_supported()
{
}

bool Loader::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Loader, Name::Strict<273> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Loader::save(QDomElement& dst_) const
{
	Element<Loader, Name::Strict<273> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Loader::save(QDomDocument& dst_) const
{
	Element<Loader, Name::Strict<273> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Capability

int Traits<Capability::Xml::Loader>::parse(Capability::Xml::Loader& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setSupported(m.get<0>().getValue());
		dst_.setValueList(m.get<1>().getValue());
		dst_.setEnumList(m.get<2>().getValue());
	}
	return output;
}

int Traits<Capability::Xml::Loader>::generate(const Capability::Xml::Loader& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getSupported(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getValueList(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getEnumList(), m.get<2>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Os

namespace Capability
{
namespace Xml
{
Os::Os(): m_supported()
{
}

bool Os::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Os, Name::Strict<222> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Os::save(QDomElement& dst_) const
{
	Element<Os, Name::Strict<222> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Os::save(QDomDocument& dst_) const
{
	Element<Os, Name::Strict<222> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Capability

int Traits<Capability::Xml::Os>::parse(Capability::Xml::Os& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setSupported(m.get<0>().getValue());
		dst_.setLoader(m.get<1>().getValue());
	}
	return output;
}

int Traits<Capability::Xml::Os>::generate(const Capability::Xml::Os& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getSupported(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getLoader(), m.get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Model

namespace Capability
{
namespace Xml
{
bool Model::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Model, Name::Strict<231> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Model::save(QDomElement& dst_) const
{
	Element<Model, Name::Strict<231> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Model::save(QDomDocument& dst_) const
{
	Element<Model, Name::Strict<231> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Capability

int Traits<Capability::Xml::Model>::parse(Capability::Xml::Model& dst_, QStack<QDomElement>& stack_)
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

int Traits<Capability::Xml::Model>::generate(const Capability::Xml::Model& src_, QDomElement& dst_)
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
// struct Feature

namespace Capability
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
	Element<Feature, Name::Strict<1013> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Feature::save(QDomElement& dst_) const
{
	Element<Feature, Name::Strict<1013> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Feature::save(QDomDocument& dst_) const
{
	Element<Feature, Name::Strict<1013> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Capability

int Traits<Capability::Xml::Feature>::parse(Capability::Xml::Feature& dst_, QStack<QDomElement>& stack_)
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

int Traits<Capability::Xml::Feature>::generate(const Capability::Xml::Feature& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getPolicy(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getName(), m.get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Anonymous1877

namespace Capability
{
namespace Xml
{
bool Anonymous1877::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	k.push(src_.firstChildElement());
	return 0 <= Traits<Anonymous1877>::parse(*this, k);
}

bool Anonymous1877::save(QDomElement& dst_) const
{
	return 0 <= Traits<Anonymous1877>::generate(*this, dst_);
}


} // namespace Xml
} // namespace Capability

int Traits<Capability::Xml::Anonymous1877>::parse(Capability::Xml::Anonymous1877& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setModel(m.get<0>().getValue());
		dst_.setVendor(m.get<1>().getValue());
		dst_.setFeatureList(m.get<2>().getValue());
	}
	return output;
}

int Traits<Capability::Xml::Anonymous1877>::generate(const Capability::Xml::Anonymous1877& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getModel(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getVendor(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getFeatureList(), m.get<2>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Mode

namespace Capability
{
namespace Xml
{
Mode::Mode(): m_supported()
{
}

bool Mode::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Mode, Name::Strict<379> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Mode::save(QDomElement& dst_) const
{
	Element<Mode, Name::Strict<379> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Mode::save(QDomDocument& dst_) const
{
	Element<Mode, Name::Strict<379> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Capability

int Traits<Capability::Xml::Mode>::parse(Capability::Xml::Mode& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setSupported(m.get<1>().getValue());
		dst_.setAnonymous1877(m.get<2>().getValue());
	}
	return output;
}

int Traits<Capability::Xml::Mode>::generate(const Capability::Xml::Mode& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getSupported(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getAnonymous1877(), m.get<2>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Model1

namespace Capability
{
namespace Xml
{
Model1::Model1(): m_usable()
{
}

bool Model1::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Model1, Name::Strict<231> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Model1::save(QDomElement& dst_) const
{
	Element<Model1, Name::Strict<231> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Model1::save(QDomDocument& dst_) const
{
	Element<Model1, Name::Strict<231> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Capability

int Traits<Capability::Xml::Model1>::parse(Capability::Xml::Model1& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setUsable(m.get<0>().getValue());
		dst_.setOwnValue(m.get<1>().getValue());
	}
	return output;
}

int Traits<Capability::Xml::Model1>::generate(const Capability::Xml::Model1& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getUsable(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getOwnValue(), m.get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Mode1

namespace Capability
{
namespace Xml
{
Mode1::Mode1(): m_supported()
{
}

bool Mode1::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Mode1, Name::Strict<379> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Mode1::save(QDomElement& dst_) const
{
	Element<Mode1, Name::Strict<379> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Mode1::save(QDomDocument& dst_) const
{
	Element<Mode1, Name::Strict<379> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Capability

int Traits<Capability::Xml::Mode1>::parse(Capability::Xml::Mode1& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setSupported(m.get<1>().getValue());
		dst_.setModelList(m.get<2>().getValue());
	}
	return output;
}

int Traits<Capability::Xml::Mode1>::generate(const Capability::Xml::Mode1& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getSupported(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getModelList(), m.get<2>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Cpu

namespace Capability
{
namespace Xml
{
Cpu::Cpu(): m_mode()
{
}

bool Cpu::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Cpu, Name::Strict<220> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Cpu::save(QDomElement& dst_) const
{
	Element<Cpu, Name::Strict<220> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Cpu::save(QDomDocument& dst_) const
{
	Element<Cpu, Name::Strict<220> > m;
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
		dst_.setMode(m.get<0>().getValue());
		dst_.setMode2(m.get<1>().getValue());
		dst_.setMode3(m.get<2>().getValue());
	}
	return output;
}

int Traits<Capability::Xml::Cpu>::generate(const Capability::Xml::Cpu& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getMode(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getMode2(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getMode3(), m.get<2>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Disk

namespace Capability
{
namespace Xml
{
Disk::Disk(): m_supported()
{
}

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
} // namespace Capability

int Traits<Capability::Xml::Disk>::parse(Capability::Xml::Disk& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setSupported(m.get<0>().getValue());
		dst_.setEnumList(m.get<1>().getValue());
	}
	return output;
}

int Traits<Capability::Xml::Disk>::generate(const Capability::Xml::Disk& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getSupported(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getEnumList(), m.get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Graphics

namespace Capability
{
namespace Xml
{
Graphics::Graphics(): m_supported()
{
}

bool Graphics::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Graphics, Name::Strict<712> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Graphics::save(QDomElement& dst_) const
{
	Element<Graphics, Name::Strict<712> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Graphics::save(QDomDocument& dst_) const
{
	Element<Graphics, Name::Strict<712> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Capability

int Traits<Capability::Xml::Graphics>::parse(Capability::Xml::Graphics& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setSupported(m.get<0>().getValue());
		dst_.setEnumList(m.get<1>().getValue());
	}
	return output;
}

int Traits<Capability::Xml::Graphics>::generate(const Capability::Xml::Graphics& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getSupported(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getEnumList(), m.get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Video

namespace Capability
{
namespace Xml
{
Video::Video(): m_supported()
{
}

bool Video::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Video, Name::Strict<779> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Video::save(QDomElement& dst_) const
{
	Element<Video, Name::Strict<779> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Video::save(QDomDocument& dst_) const
{
	Element<Video, Name::Strict<779> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Capability

int Traits<Capability::Xml::Video>::parse(Capability::Xml::Video& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setSupported(m.get<0>().getValue());
		dst_.setEnumList(m.get<1>().getValue());
	}
	return output;
}

int Traits<Capability::Xml::Video>::generate(const Capability::Xml::Video& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getSupported(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getEnumList(), m.get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Hostdev

namespace Capability
{
namespace Xml
{
Hostdev::Hostdev(): m_supported()
{
}

bool Hostdev::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Hostdev, Name::Strict<676> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Hostdev::save(QDomElement& dst_) const
{
	Element<Hostdev, Name::Strict<676> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Hostdev::save(QDomDocument& dst_) const
{
	Element<Hostdev, Name::Strict<676> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Capability

int Traits<Capability::Xml::Hostdev>::parse(Capability::Xml::Hostdev& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setSupported(m.get<0>().getValue());
		dst_.setEnumList(m.get<1>().getValue());
	}
	return output;
}

int Traits<Capability::Xml::Hostdev>::generate(const Capability::Xml::Hostdev& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getSupported(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getEnumList(), m.get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Devices

namespace Capability
{
namespace Xml
{
bool Devices::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Devices, Name::Strict<228> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Devices::save(QDomElement& dst_) const
{
	Element<Devices, Name::Strict<228> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Devices::save(QDomDocument& dst_) const
{
	Element<Devices, Name::Strict<228> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Capability

int Traits<Capability::Xml::Devices>::parse(Capability::Xml::Devices& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setDisk(m.get<0>().getValue());
		dst_.setGraphics(m.get<1>().getValue());
		dst_.setVideo(m.get<2>().getValue());
		dst_.setHostdev(m.get<3>().getValue());
	}
	return output;
}

int Traits<Capability::Xml::Devices>::generate(const Capability::Xml::Devices& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getDisk(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getGraphics(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getVideo(), m.get<2>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getHostdev(), m.get<3>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Gic

namespace Capability
{
namespace Xml
{
Gic::Gic(): m_supported()
{
}

bool Gic::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<Gic, Name::Strict<995> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool Gic::save(QDomElement& dst_) const
{
	Element<Gic, Name::Strict<995> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool Gic::save(QDomDocument& dst_) const
{
	Element<Gic, Name::Strict<995> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Capability

int Traits<Capability::Xml::Gic>::parse(Capability::Xml::Gic& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setSupported(m.get<0>().getValue());
		dst_.setEnumList(m.get<1>().getValue());
	}
	return output;
}

int Traits<Capability::Xml::Gic>::generate(const Capability::Xml::Gic& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getSupported(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getEnumList(), m.get<1>()))
		return -1;

	return m.produce(dst_);
}

///////////////////////////////////////////////////////////////////////////////
// struct DomainCapabilities

namespace Capability
{
namespace Xml
{
bool DomainCapabilities::load(const QDomElement& src_)
{
	QStack<QDomElement> k;
	k.push(src_);
	Element<DomainCapabilities, Name::Strict<1870> > m;
	if (0 > m.consume(k))
		return false;
	
	*this = m.getValue();
	return true;
}

bool DomainCapabilities::save(QDomElement& dst_) const
{
	Element<DomainCapabilities, Name::Strict<1870> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}

bool DomainCapabilities::save(QDomDocument& dst_) const
{
	Element<DomainCapabilities, Name::Strict<1870> > m;
	m.setValue(*this);
	return 0 <= m.produce(dst_);
}


} // namespace Xml
} // namespace Capability

int Traits<Capability::Xml::DomainCapabilities>::parse(Capability::Xml::DomainCapabilities& dst_, QStack<QDomElement>& stack_)
{
	marshal_type m;
	int output = m.consume(stack_);
	if (0 <= output)
	{
		dst_.setPath(m.get<0>().getValue());
		dst_.setDomain(m.get<1>().getValue());
		dst_.setMachine(m.get<2>().getValue());
		dst_.setArch(m.get<3>().getValue());
		dst_.setVcpu(m.get<4>().getValue());
		dst_.setOs(m.get<5>().getValue());
		dst_.setCpu(m.get<6>().getValue());
		dst_.setDevices(m.get<7>().getValue());
		dst_.setFeatures(m.get<8>().getValue());
	}
	return output;
}

int Traits<Capability::Xml::DomainCapabilities>::generate(const Capability::Xml::DomainCapabilities& src_, QDomElement& dst_)
{
	marshal_type m;
	if (0 > Details::Marshal::assign(src_.getPath(), m.get<0>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getDomain(), m.get<1>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getMachine(), m.get<2>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getArch(), m.get<3>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getVcpu(), m.get<4>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getOs(), m.get<5>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getCpu(), m.get<6>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getDevices(), m.get<7>()))
		return -1;
	if (0 > Details::Marshal::assign(src_.getFeatures(), m.get<8>()))
		return -1;

	return m.produce(dst_);
}

} // namespace Libvirt
