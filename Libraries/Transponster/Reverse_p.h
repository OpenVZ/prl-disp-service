///////////////////////////////////////////////////////////////////////////////
///
/// @file Reverse_p.h
///
/// Convertor from the SDK config format into the libvirt one. The private part.
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

#ifndef __REVERSE_P_H__
#define __REVERSE_P_H__

#include "domain_type.h"
#include <QDomDocument>
#include <XmlModel/VmConfig/CVmConfiguration.h>

namespace Transponster
{
namespace Boot
{
///////////////////////////////////////////////////////////////////////////////
// struct Reverse

struct Reverse
{
	typedef CVmStartupOptions::CVmBootDevice device_type;
	typedef boost::optional<Libvirt::Domain::Xml::PPositiveInteger::value_type >
		order_type;

	explicit Reverse(const QList<device_type* >& list_);

	order_type operator()(const CVmDevice& device_) const;

private:
	typedef QPair<PRL_DEVICE_TYPE, unsigned> key_type;
	typedef QHash<key_type, order_type::value_type> map_type;

	map_type m_map;
};

} // namespace Boot

namespace Device
{
///////////////////////////////////////////////////////////////////////////////
// struct Flavor

template<class T>
struct Flavor;

///////////////////////////////////////////////////////////////////////////////
// struct Clustered

template<class T>
struct Clustered
{
	typedef Boot::Reverse::order_type boot_type;

	Clustered(): m_device()
	{
	}

	QString getAlias() const
	{
		return getDevice().getUserFriendlyName();
	}
	boost::optional<Libvirt::Domain::Xml::EBus> getBus() const
	{
		switch (getDevice().getInterfaceType())
		{
		case PMS_IDE_DEVICE:
			return Libvirt::Domain::Xml::EBusIde;
		case PMS_SCSI_DEVICE:
			return Libvirt::Domain::Xml::EBusScsi;
		case PMS_SATA_DEVICE:
			return Libvirt::Domain::Xml::EBusSata;
		default:
			return boost::optional<Libvirt::Domain::Xml::EBus>();
		}
	}
	Libvirt::Domain::Xml::VDiskSource getSource() const
	{
		switch (getDevice().getEmulatedType())
		{
		case Flavor<T>::image:
		{
			Libvirt::Domain::Xml::Source s;
			QString f = getDevice().getSystemName();
			if (f.isEmpty())
				f = getDevice().getUserFriendlyName();

			s.setFile(f);
			mpl::at_c<Libvirt::Domain::Xml::VDiskSource::types, 0>::type x;
			x.setValue(s);
			return x;
		}
		case Flavor<T>::real:
		{
			Libvirt::Domain::Xml::Source1 s;
			QRegExp r("[(]([^)]+)[)]");
			if (-1 < r.indexIn(getDevice().getUserFriendlyName()))
				s.setDev(r.cap(1));
			else
				s.setDev(getDevice().getSystemName());

			mpl::at_c<Libvirt::Domain::Xml::VDiskSource::types, 1>::type x;
			x.setValue(s);
			return x;
		}
		default:
			return Libvirt::Domain::Xml::VDiskSource();
		}
	}
	QString getTarget() const
	{
		return QString(Flavor<T>::getTarget())
			.append(QString::number(getDevice().getIndex() + 1));
	}
	boost::optional<Libvirt::Domain::Xml::Driver> getDriver() const
	{
		if (Flavor<T>::image != getDevice().getEmulatedType())
			return boost::optional<Libvirt::Domain::Xml::Driver>();

		mpl::at_c<Libvirt::Domain::Xml::VType::types, 1>::type a;
		a.setValue(Libvirt::Domain::Xml::VStorageFormat(Flavor<T>::getDriverFormat()));
		Libvirt::Domain::Xml::DriverFormat b;
		b.setName("qemu");
		b.setType(Libvirt::Domain::Xml::VType(a));
		Libvirt::Domain::Xml::Driver output;
		output.setDriverFormat(b);
		return output;
	}
	static bool isReadonly()
	{
		return Flavor<T>::readonly;
	}
	static Libvirt::Domain::Xml::EDevice getKind()
	{
		return Flavor<T>::kind;
	}
	boost::optional<Libvirt::Domain::Xml::ETray> getTray() const
	{
		return Flavor<T>::getTray(getDevice().getEmulatedType());
	}
	const boot_type& getBoot() const
	{
		return m_boot;
	}


protected:
	void setBoot(const boot_type& value_)
	{
		m_boot = value_;
	}
	void setDevice(const T& value_)
	{
		m_device = &value_;
	}
	const T& getDevice() const
	{
		return *m_device;
	}

private:
	const T* m_device;
	boot_type m_boot;
};

///////////////////////////////////////////////////////////////////////////////
// struct Disk

struct Disk: Clustered<CVmHardDisk>
{
	Disk(const CVmHardDisk& device_, const boot_type& boot_)
	{
		setBoot(boot_);
		setDevice(device_);
	}

	Libvirt::Domain::Xml::VDiskSource getSource() const;
};

///////////////////////////////////////////////////////////////////////////////
// struct Cdrom

struct Cdrom: Clustered<CVmOpticalDisk>
{
	Cdrom(const CVmOpticalDisk& device_, const boot_type& boot_)
	{
		setBoot(boot_);
		setDevice(device_);
	}
};

///////////////////////////////////////////////////////////////////////////////
// struct Floppy

struct Floppy: Clustered<CVmFloppyDisk>
{
	explicit Floppy(const CVmFloppyDisk& device_)
	{
		setDevice(device_);
	}
};

///////////////////////////////////////////////////////////////////////////////
// struct Network

template<int N>
struct Network
{
	Libvirt::Domain::Xml::VInterface operator()(const CVmGenericNetworkAdapter& network_);

private:
	typedef typename mpl::at_c<Libvirt::Domain::Xml::VInterface::types, N>::type
		access_type;

	static typename Libvirt::Details::Value::Grab<access_type>::type
		prepare(const CVmGenericNetworkAdapter& network_);
};

///////////////////////////////////////////////////////////////////////////////
// struct List

struct List
{
	explicit List(const Boot::Reverse& boot_): m_boot(boot_)
	{
	}

	Libvirt::Domain::Xml::Devices getResult() const;
	void add(const CVmRemoteDisplay* vnc_);
	void add(const CVmHardDisk* disk_);
	void add(const CVmParallelPort* port_);
	void add(const CVmSerialPort* port_);
	void add(const CVmOpticalDisk* cdrom_);
	void add(const CVmSoundDevice* sound_);
	void add(const CVmVideo* video_);
	void add(const CVmFloppyDisk* floppy_);
	void add(const CVmGenericNetworkAdapter* network_);

private:
	template<int N, class T>
	void add(const T& value_)
	{
		typename mpl::at_c<Libvirt::Domain::Xml::VChoice912::types, N>::type x;
		x.setValue(value_);
		m_devices << x;
	}
	template<class T>
	typename boost::disable_if<boost::is_pointer<T> >::type
		add(T clustered_)
	{
		Libvirt::Domain::Xml::Target t;
		t.setDev(clustered_.getTarget());
		t.setBus(clustered_.getBus());
		t.setTray(clustered_.getTray());

		mpl::at_c<Libvirt::Domain::Xml::VDisk::types, 0>::type y;
		y.setValue(clustered_.getKind());
		mpl::at_c<Libvirt::Domain::Xml::VDiskBackingChain::types, 1>::type c;
		c.setValue(false);

		Libvirt::Domain::Xml::Disk d;
		d.setBoot(clustered_.getBoot());
		d.setDisk(Libvirt::Domain::Xml::VDisk(y));
		d.setTarget(t);
		d.setReadonly(clustered_.isReadonly());
		QString a = clustered_.getAlias();
		if (!a.isEmpty())
			d.setAlias(a);

		d.setDriver(clustered_.getDriver());
		d.setDiskSource(clustered_.getSource());
		d.setDiskBackingChain(c);
		add<0>(d);
	}

	Boot::Reverse m_boot;
	QList<Libvirt::Domain::Xml::VChoice912 > m_devices;
};

} // namespace Device

///////////////////////////////////////////////////////////////////////////////
// struct Resources

struct Resources
{
	explicit Resources(CVmHardware& hardware_): m_hardware(&hardware_)
	{
	}

	void setCpu(const Libvirt::Domain::Xml::Vcpu& src_);
	bool getCpu(Libvirt::Domain::Xml::Vcpu& dst_);
	void setClock(const Libvirt::Domain::Xml::Clock& src_);
	bool getClock(Libvirt::Domain::Xml::Clock& dst_);
	void setMemory(const Libvirt::Domain::Xml::Memory& src_);
	bool getMemory(Libvirt::Domain::Xml::Memory& dst_);
	void setChipset(const Libvirt::Domain::Xml::Sysinfo& src_);
private:
	CVmHardware* m_hardware;
};

namespace Network
{
namespace Address
{
///////////////////////////////////////////////////////////////////////////////
// struct IPv4

struct IPv4
{
	static const int index = 0;
	static QHostAddress patchEnd(const QHostAddress& start_, const QHostAddress& end_);
	static int getMask(const QHostAddress& mask_);
	static const char* getFamily();
};

///////////////////////////////////////////////////////////////////////////////
// struct IPv6

struct IPv6
{
	static const int index = 1;
	static QHostAddress patchEnd(const QHostAddress& start_, const QHostAddress& end_);
	static int getMask(const QHostAddress& mask_);
	static const char* getFamily();
};

} // namespace Address
} // namespace Network
} // namespace Transponster

#endif // __REVERSE_P_H__

