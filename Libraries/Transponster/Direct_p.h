///////////////////////////////////////////////////////////////////////////////
///
/// @file Direct_p.h
///
/// Convertor from the libvirt config format into the SDK one. The private part.
///
/// @author shrike
///
/// Copyright (c) 2005-2015 Parallels IP Holdings GmbH
///
/// This file is part of Virtuozzo Core Libraries. Virtuozzo Core
/// Libraries is free software; you can redistribute it and/or modify it
/// under the terms of the GNU Lesser General Public License as published
/// by the Free Software Foundation; either version 2.1 of the License, or
/// (at your option) any later version.
///
/// This library is distributed in the hope that it will be useful,
/// but WITHOUT ANY WARRANTY; without even the implied warranty of
/// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
/// Lesser General Public License for more details.
///
/// You should have received a copy of the GNU Lesser General Public
/// License along with this library.  If not, see
/// <http://www.gnu.org/licenses/> or write to Free Software Foundation,
/// 51 Franklin Street, Fifth Floor Boston, MA 02110, USA.
///
/// Our contact details: Parallels IP Holdings GmbH, Vordergasse 59, 8200
/// Schaffhausen, Switzerland.
///
///////////////////////////////////////////////////////////////////////////////

#ifndef __DIRECT_P_H__
#define __DIRECT_P_H__

#include "domain_type.h"
#include <QFile>
#include <QDomDocument>
#include <boost/function.hpp>
#include <Libraries/PrlUuid/Uuid.h>
#include <XmlModel/VmConfig/CVmConfiguration.h>

namespace Transponster
{
namespace Boot
{
///////////////////////////////////////////////////////////////////////////////
// struct Direct

struct Direct
{
	typedef CVmStartupOptions::CVmBootDevice device_type;
	typedef boost::optional<Libvirt::Domain::Xml::PPositiveInteger::value_type >
		order_type;

	void operator()(const CVmDevice& device_, const order_type& order_);
	QList<device_type*> getResult() const;

private:
	typedef std::map<unsigned, std::pair<PRL_DEVICE_TYPE, unsigned> >
		map_type;

	map_type m_map;
};

} // namespace Boot

namespace Visitor
{
namespace Source
{
///////////////////////////////////////////////////////////////////////////////
// struct Clustered

template<class T, int I, int R>
struct Clustered: boost::static_visitor<bool>
{
	Clustered(): m_device()
	{
	}

	template<class U>
	bool operator()(const U& ) const
	{
		return false;
	}
	bool operator()(const mpl::at_c<Libvirt::Domain::Xml::VDiskSource::types, 0>::type& source_) const
	{
		const Libvirt::Domain::Xml::Source* v = source_.getValue().get_ptr();
		if (NULL == v)
			return false;

		getDevice().setEmulatedType(I);
		if (v->getFile())
		{
			getDevice().setSystemName(v->getFile().get());
			getDevice().setUserFriendlyName(v->getFile().get());
		}
		return true;
	}
	bool operator()(const mpl::at_c<Libvirt::Domain::Xml::VDiskSource::types, 1>::type& source_) const
	{
		const Libvirt::Domain::Xml::Source1* v = source_.getValue().get_ptr();
		if (NULL == v)
			return false;

		getDevice().setEmulatedType(R);
		if (v->getDev())
		{
			getDevice().setSystemName(v->getDev().get());
			getDevice().setUserFriendlyName(v->getDev().get());
		}
		return true;
	}
	void setDevice(T& value_)
	{
		m_device = &value_;
	}

protected:
	T& getDevice() const
	{
		return *m_device;
	}

private:
	T* m_device;
};

///////////////////////////////////////////////////////////////////////////////
// struct Unit

template<class T>
struct Unit;

template<>
struct Unit<CVmFloppyDisk>: Clustered<CVmFloppyDisk, PVE::FloppyDiskImage, PVE::RealFloppyDisk>
{
};

template<>
struct Unit<CVmOpticalDisk>: Clustered<CVmOpticalDisk, PVE::CdRomImage, PVE::RealCdRom>
{
};

template<>
struct Unit<CVmHardDisk>: Clustered<CVmHardDisk, PVE::HardDiskImage, PVE::RealHardDisk>
{
	using Clustered<CVmHardDisk, PVE::HardDiskImage, PVE::RealHardDisk>::operator();

	bool operator()(const mpl::at_c<Libvirt::Domain::Xml::VDiskSource::types, 4>::type& source_) const;
};

} // namespace Source

///////////////////////////////////////////////////////////////////////////////
// struct Clustered

template<class T>
struct Clustered
{
	Clustered(): m_device(new T())
	{
	}

	bool operator()(const Libvirt::Domain::Xml::Disk& disk_)
	{
		if (m_device.isNull())
			return false;

		getDevice().setEnabled(PVE::DeviceEnabled);
		getDevice().setConnected();
		if (disk_.getAlias())
		{
			getDevice().setUserFriendlyName
				(disk_.getAlias().get());
		}
		if (disk_.getTarget().getBus())
		{
			switch (disk_.getTarget().getBus().get())
			{
			case Libvirt::Domain::Xml::EBusFdc:
			case Libvirt::Domain::Xml::EBusIde:
				getDevice().setInterfaceType(PMS_IDE_DEVICE);
				break;
			case Libvirt::Domain::Xml::EBusScsi:
				getDevice().setInterfaceType(PMS_SCSI_DEVICE);
				break;
			case Libvirt::Domain::Xml::EBusSata:
				getDevice().setInterfaceType(PMS_SATA_DEVICE);
				break;
			default:
				return false;
			}
		}
		Source::Unit<T> v;
		v.setDevice(getDevice());
		return boost::apply_visitor(v, disk_.getDiskSource());
	}
	T* getResult()
	{
		return m_device.take();
	}

protected:
	T& getDevice() const
	{
		return *m_device;
	}

private:
	QScopedPointer<T> m_device;
};

///////////////////////////////////////////////////////////////////////////////
// struct Floppy

struct Floppy: private Clustered<CVmFloppyDisk>
{
	explicit Floppy(CVmHardware& hardware_): m_hardware(&hardware_)
	{
	}

	PRL_RESULT operator()(const Libvirt::Domain::Xml::Disk& disk_);

private:
	CVmHardware* m_hardware;
};

///////////////////////////////////////////////////////////////////////////////
// struct Disk

struct Disk: private Clustered<CVmHardDisk>
{
	Disk(CVmHardware& hardware_, Boot::Direct& boot_):
		m_hardware(&hardware_), m_boot(&boot_)
	{
	}

	PRL_RESULT operator()(const Libvirt::Domain::Xml::Disk& disk_);

private:
	CVmHardware* m_hardware;
	Boot::Direct* m_boot;
};

///////////////////////////////////////////////////////////////////////////////
// struct Cdrom

struct Cdrom: private Clustered<CVmOpticalDisk>
{
	Cdrom(CVmHardware& hardware_, Boot::Direct& boot_):
		m_hardware(&hardware_), m_boot(&boot_)
	{
	}

	PRL_RESULT operator()(const Libvirt::Domain::Xml::Disk& disk_);

private:
	CVmHardware* m_hardware;
	Boot::Direct* m_boot;
};

///////////////////////////////////////////////////////////////////////////////
// struct Graphics

struct Graphics: boost::static_visitor<PRL_RESULT>
{
	explicit Graphics(CVmConfiguration& vm_): m_vm(&vm_)
	{
	}

	template<class T>
	PRL_RESULT operator()(const T& ) const
	{
		return PRL_ERR_UNIMPLEMENTED;
	}
	PRL_RESULT operator()(const mpl::at_c<Libvirt::Domain::Xml::VGraphics::types, 1>::type& vnc_) const;

private:
	CVmConfiguration* m_vm;
};

///////////////////////////////////////////////////////////////////////////////
// struct Network

struct Network: boost::static_visitor<PRL_RESULT>
{
	explicit Network(CVmHardware& hardware_): m_hardware(&hardware_)
	{
	}

	template<class T>
	PRL_RESULT operator()(const T& ) const
	{
		return PRL_ERR_UNIMPLEMENTED;
	}
	PRL_RESULT operator()(const mpl::at_c<Libvirt::Domain::Xml::VInterface::types, 0>::type& bridge_) const;
	PRL_RESULT operator()(const mpl::at_c<Libvirt::Domain::Xml::VInterface::types, 3>::type& network_) const;
	PRL_RESULT operator()(const mpl::at_c<Libvirt::Domain::Xml::VInterface::types, 4>::type& direct_) const;

private:
	CVmHardware* m_hardware;
};

///////////////////////////////////////////////////////////////////////////////
// struct Device

struct Device: boost::static_visitor<PRL_RESULT>
{
	Device(CVmConfiguration& vm_, Boot::Direct& boot_):
		m_boot(&boot_), m_vm(&vm_)
	{
	}

	template<class T>
	PRL_RESULT operator()(const T& ) const
	{
		return PRL_ERR_SUCCESS;
	}
	PRL_RESULT operator()(const mpl::at_c<Libvirt::Domain::Xml::VChoice912::types, 0>::type& disk_) const;
	PRL_RESULT operator()(const mpl::at_c<Libvirt::Domain::Xml::VChoice912::types, 4>::type& interface_) const;
	PRL_RESULT operator()(const mpl::at_c<Libvirt::Domain::Xml::VChoice912::types, 6>::type& sound_) const;
	PRL_RESULT operator()(const mpl::at_c<Libvirt::Domain::Xml::VChoice912::types, 8>::type& graphics_) const
	{
		return boost::apply_visitor(Graphics(*m_vm), graphics_.getValue()); 
	}
	PRL_RESULT operator()(const mpl::at_c<Libvirt::Domain::Xml::VChoice912::types, 9>::type& video_) const;
	PRL_RESULT operator()(const mpl::at_c<Libvirt::Domain::Xml::VChoice912::types, 11>::type& parallel_) const;
	PRL_RESULT operator()(const mpl::at_c<Libvirt::Domain::Xml::VChoice912::types, 12>::type& serial_) const;

private:
	Boot::Direct* m_boot;
	CVmConfiguration* m_vm;
};

///////////////////////////////////////////////////////////////////////////////
// struct Clock

struct Clock: boost::static_visitor<void>
{
	explicit Clock(::Clock& clock_): m_clock(&clock_)
	{
	}

	template<class T>
	void operator()(const T& ) const
	{
	}
	void operator()(const mpl::at_c<Libvirt::Domain::Xml::VClock::types, 0>::type& fixed_) const;
	void operator()(const mpl::at_c<Libvirt::Domain::Xml::VClock::types, 2>::type& variable_) const;

private:
	::Clock* m_clock;
};

///////////////////////////////////////////////////////////////////////////////
// struct Uuid

template<class T>
struct Uuid: boost::static_visitor<void>
{
	typedef boost::function1<void, const QString& > sink_type;

	explicit Uuid(const sink_type& sink_): m_sink(sink_)
	{
	}

	void operator()(const typename mpl::at_c<typename T::types, 0>::type& /*string_*/) const
	{
	}
	void operator()(const typename mpl::at_c<typename T::types, 1>::type& string_) const
	{
		QString u = "{" + string_.getValue() + "}";
		m_sink(u);
	}

private:
	sink_type m_sink;
};

///////////////////////////////////////////////////////////////////////////////
// struct Addressing

struct Addressing: boost::static_visitor<void>
{
	explicit Addressing(CHwNetAdapter& sink_): m_sink(&sink_)
	{
	}

	void operator()(const mpl::at_c<Libvirt::Iface::Xml::VInterfaceAddressing::types, 0>::type& group_) const
	{
		consume(group_.getValue().getProtocol());
		consume(group_.getValue().getProtocol2());
	}
	void operator()(const mpl::at_c<Libvirt::Iface::Xml::VInterfaceAddressing::types, 1>::type& group_) const
	{
		consume(group_.getValue().getProtocol());
		consume(group_.getValue().getProtocol2());
	}
	void operator()(const mpl::at_c<Libvirt::Iface::Xml::VChoice1244::types, 0>::type& ) const
	{
		m_sink->setConfigureWithDhcp(true);
	}
	void operator()(const mpl::at_c<Libvirt::Iface::Xml::VChoice1244::types, 1>::type& ) const
	{
		m_sink->setConfigureWithDhcp(false);
	}

private:
	void consume(const boost::optional<Libvirt::Iface::Xml::VChoice1244 >& ipv4_) const
	{
		if (ipv4_)
			boost::apply_visitor(*this, ipv4_.get());
	}
	void consume(const boost::optional<Libvirt::Iface::Xml::Protocol >& ipv6_) const
	{
		m_sink->setConfigureWithDhcpIPv6(ipv6_ && ipv6_.get().getDhcp());
	}

	CHwNetAdapter* m_sink;
};

} // namespace Visitor
} // namespace Transponster

#endif // __DIRECT_P_H__

