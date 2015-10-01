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
	typedef Libvirt::Domain::Xml::Disk result_type;

	Clustered(const T& device_, const boot_type& boot_ = boot_type()):
		m_device(&device_), m_boot(boot_)
	{
	}

	void setBoot(result_type& result_)
	{
		result_.setBoot(m_boot);
	}
	void setDisk(result_type& result_);
	void setAlias(result_type& result_)
	{
		result_.setAlias(getDevice().getUserFriendlyName());
	}
	void setReadonly(result_type& result_)
	{
		result_.setReadonly(Flavor<T>::readonly);
	}
	void setDriver(result_type& result_);
	void setSource(result_type& result_)
	{
		result_.setDiskSource(getSource());
	}
	void setTarget(result_type& result_);
	void setBackingChain(result_type& result_);

protected:
	const T& getDevice() const
	{
		return *m_device;
	}
	Libvirt::Domain::Xml::VDiskSource getSource() const;
	boost::optional<Libvirt::Domain::Xml::EBus> getBus() const;

private:
	const T* m_device;
	boot_type m_boot;
};

template<class T>
void Clustered<T>::setDisk(result_type& result_)
{
	mpl::at_c<Libvirt::Domain::Xml::VDisk::types, 0>::type x;
	x.setValue(Flavor<T>::kind);
	result_.setDisk(Libvirt::Domain::Xml::VDisk(x));
}

template<class T>
void Clustered<T>::setDriver(result_type& result_)
{
	if (Flavor<T>::image != getDevice().getEmulatedType())
		return;

	mpl::at_c<Libvirt::Domain::Xml::VType::types, 1>::type a;
	a.setValue(Libvirt::Domain::Xml::VStorageFormat(Flavor<T>::getDriverFormat()));
	Libvirt::Domain::Xml::DriverFormat b;
	b.setName("qemu");
	b.setType(Libvirt::Domain::Xml::VType(a));
	Libvirt::Domain::Xml::Driver d;
	d.setDriverFormat(b);
	result_.setDriver(d);
}

template<class T>
Libvirt::Domain::Xml::VDiskSource Clustered<T>::getSource() const
{
	switch (getDevice().getEmulatedType())
	{
	case Flavor<T>::image:
	{
		Libvirt::Domain::Xml::Source s;
		QString f = getDevice().getUserFriendlyName();
		if (f.isEmpty())
			f = getDevice().getSystemName();

		s.setFile(f);
		s.setStartupPolicy(Libvirt::Domain::Xml::EStartupPolicyOptional);
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

template<class T>
boost::optional<Libvirt::Domain::Xml::EBus> Clustered<T>::getBus() const
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

template<class T>
void Clustered<T>::setTarget(result_type& result_)
{
	Libvirt::Domain::Xml::Target t;
	t.setBus(getBus());
	t.setDev(QString(Flavor<T>::getTarget())
		.append(QString::number(getDevice().getIndex() + 1)));
	t.setTray(Flavor<T>::getTray(getDevice().getEmulatedType()));
	result_.setTarget(t);
}

template<class T>
void Clustered<T>::setBackingChain(result_type& result_)
{
	mpl::at_c<Libvirt::Domain::Xml::VDiskBackingChain::types, 1>::type x;
	x.setValue(false);
	result_.setDiskBackingChain(Libvirt::Domain::Xml::VDiskBackingChain(x));
}

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
// struct Attachment

struct Attachment
{
	Attachment(): m_ide(), m_sata(), m_implicit()
	{
	}

	Libvirt::Domain::Xml::VAddress craftIde();
	Libvirt::Domain::Xml::VAddress craftSata();
	QList<Libvirt::Domain::Xml::VChoice928 > getControllers() const
	{
		return m_controllerList;
	}

private:
	enum
	{
		IDE_BUSES = 2,
		SATA_BUSES = 6
	};

	static Libvirt::Domain::Xml::VAddress craft(quint16 controller_, quint16 unit_);
	template<Libvirt::Domain::Xml::EType6 T>
	void craftController(quint16 index_);

	quint16 m_ide;
	quint16 m_sata;
	quint8 m_implicit;
	QList<Libvirt::Domain::Xml::VChoice928 > m_controllerList;
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
	typedef QList<Libvirt::Domain::Xml::VChoice928 > list_type;

	template<int N, class T>
	void add(const T& value_)
	{
		typename mpl::at_c<Libvirt::Domain::Xml::VChoice928::types, N>::type x;
		x.setValue(value_);
		m_deviceList << x;
	}
	template<class T>
	typename boost::disable_if<boost::is_pointer<T> >::type
		add(T builder_);

	Libvirt::Domain::Xml::Panic craftPanic() const;

	Boot::Reverse m_boot;
	list_type m_deviceList;
	Attachment m_attachment;
};

template<class T>
typename boost::disable_if<boost::is_pointer<T> >::type List::add(T builder_)
{
	Libvirt::Domain::Xml::Disk d;
	builder_.setBoot(d);
	builder_.setDisk(d);
	builder_.setAlias(d);
	builder_.setDriver(d);
	builder_.setSource(d);
	builder_.setTarget(d);
	builder_.setReadonly(d);
	builder_.setBackingChain(d);
	switch (d.getTarget().getBus().get())
	{
	case Libvirt::Domain::Xml::EBusIde:
		d.setAddress(m_attachment.craftIde());
		break;
	case Libvirt::Domain::Xml::EBusSata:
		d.setAddress(m_attachment.craftSata());
		break;
	default:
		break;
	}
	return add<0>(d);
}

} // namespace Device

///////////////////////////////////////////////////////////////////////////////
// struct Resources

struct Resources
{
	explicit Resources(CVmHardware& hardware_): m_hardware(&hardware_)
	{
	}

	void setVCpu(const Libvirt::Domain::Xml::VCpu& src_);
	bool getVCpu(Libvirt::Domain::Xml::VCpu& dst_);
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

