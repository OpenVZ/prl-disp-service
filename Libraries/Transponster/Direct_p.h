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
#include <Libraries/PrlNetworking/PrlNetLibrary.h>
#include <XmlModel/VmConfig/CVmConfiguration.h>

namespace Transponster
{
namespace Boot
{
///////////////////////////////////////////////////////////////////////////////
// struct Slot

struct Slot
{
	explicit Slot(CVmStartupOptions::CVmBootDevice& device_): m_device(&device_)
	{
	}

	void set(PRL_DEVICE_TYPE t_, uint index_)
	{
		m_device->deviceType = t_;
		m_device->deviceIndex = index_;
	}

private:
	CVmStartupOptions::CVmBootDevice* m_device;
};

} // namespace Boot

///////////////////////////////////////////////////////////////////////////////
// struct Clip

struct Clip
{
	typedef QList<Libvirt::Domain::Xml::Controller> controllerList_type;

	Clip(CVmStartupOptions& boot_, const controllerList_type& controllers_)
		: m_bootList(&boot_.m_lstBootDeviceList), m_controllerList(&controllers_)
	{
	}

	Boot::Slot getBootSlot(Libvirt::Domain::Xml::PPositiveInteger::value_type order_);
	size_t getBusSlot(PRL_MASS_STORAGE_INTERFACE_TYPE bus_)
	{
		return m_busIndexMap[bus_]++;
	}

	boost::optional<PRL_CLUSTERED_DEVICE_SUBTYPE> getControllerModel(const Libvirt::Domain::Xml::Disk& disk_) const;

private:
	QList<CVmStartupOptions::CVmBootDevice*>* m_bootList;
	std::map<PRL_MASS_STORAGE_INTERFACE_TYPE, size_t> m_busIndexMap;
	const controllerList_type *m_controllerList;
};

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
		else
			getDevice().setConnected(PVE::DeviceDisconnected);

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
// struct Cpu

struct Cpu: boost::static_visitor<void>
{
	Cpu(CVmHardware& hw_): m_hardware(&hw_)
	{
	}

	template<class T>
	void operator()(const T& ) const
	{
	}
	void operator()(const mpl::at_c<Libvirt::Domain::Xml::VCpu::types, 2>::type& cpu_) const;

private:
	CVmHardware* m_hardware;
};

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
			case Libvirt::Domain::Xml::EBusVirtio:
				getDevice().setInterfaceType(PMS_VIRTIO_BLOCK_DEVICE);
				break;
			default:
				return false;
			}
		}
		Source::Unit<T> v;
		v.setDevice(getDevice());
		if (!boost::apply_visitor(v, disk_.getDiskSource()))
			return false;
		if (PVE::DeviceDisconnected == getDevice().getConnected() &&
			disk_.getSerial())
		{
			QByteArray a = QByteArray::fromHex(disk_.getSerial().get().toUtf8());
			getDevice().setSystemName(QString(a));
			getDevice().setUserFriendlyName(QString(a));
		}
		return true;
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
// struct Iotune

struct Iotune: boost::static_visitor<void>
{
	Iotune(CVmHardDisk* disk_): m_disk(disk_)
	{
	}

	template<class T>
	void operator()(const T& ) const
	{
	}

	void operator()(const mpl::at_c<Libvirt::Domain::Xml::VChoice1045::types, 0>::type& iopsLimit_) const;
	void operator()(const mpl::at_c<Libvirt::Domain::Xml::VChoice1041::types, 0>::type& ioLimit_) const;

private:
	CVmHardDisk* m_disk;
};

///////////////////////////////////////////////////////////////////////////////
// struct Disk

struct Disk: private Clustered<CVmHardDisk>
{
	Disk(CVmHardware& hardware_, Clip& clip_):
		m_hardware(&hardware_), m_clip(&clip_)
	{
	}

	PRL_RESULT operator()(const Libvirt::Domain::Xml::Disk& disk_);

private:
	CVmHardware* m_hardware;
	Clip* m_clip;
};

///////////////////////////////////////////////////////////////////////////////
// struct Cdrom

struct Cdrom: private Clustered<CVmOpticalDisk>
{
	Cdrom(CVmHardware& hardware_, Clip& clip_):
		m_hardware(&hardware_), m_clip(&clip_)
	{
	}

	PRL_RESULT operator()(const Libvirt::Domain::Xml::Disk& disk_);

private:
	CVmHardware* m_hardware;
	Clip* m_clip;
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
// struct Ips

struct Ips
{
	QList<QString> operator()(const QList<Libvirt::Domain::Xml::Ip>& ips_);
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

	static PRL_VM_NET_ADAPTER_TYPE parseAdapterType(const QString& type);

private:
	CVmHardware* m_hardware;
};

///////////////////////////////////////////////////////////////////////////////
// struct Device

struct Device: boost::static_visitor<PRL_RESULT>
{
	Device(CVmConfiguration& vm_, Clip& clip_):
		m_clip(&clip_), m_vm(&vm_)
	{
	}

	template<class T>
	PRL_RESULT operator()(const T& ) const
	{
		return PRL_ERR_SUCCESS;
	}
	PRL_RESULT operator()(const mpl::at_c<Libvirt::Domain::Xml::VChoice932::types, 0>::type& disk_) const;
	PRL_RESULT operator()(const mpl::at_c<Libvirt::Domain::Xml::VChoice932::types, 1>::type& controller_) const;
	PRL_RESULT operator()(const mpl::at_c<Libvirt::Domain::Xml::VChoice932::types, 4>::type& interface_) const;
	PRL_RESULT operator()(const mpl::at_c<Libvirt::Domain::Xml::VChoice932::types, 6>::type& sound_) const;
	PRL_RESULT operator()(const mpl::at_c<Libvirt::Domain::Xml::VChoice932::types, 8>::type& graphics_) const
	{
		return boost::apply_visitor(Graphics(*m_vm), graphics_.getValue()); 
	}
	PRL_RESULT operator()(const mpl::at_c<Libvirt::Domain::Xml::VChoice932::types, 9>::type& video_) const;
	PRL_RESULT operator()(const mpl::at_c<Libvirt::Domain::Xml::VChoice932::types, 11>::type& parallel_) const;
	PRL_RESULT operator()(const mpl::at_c<Libvirt::Domain::Xml::VChoice932::types, 12>::type& serial_) const;

private:
	Clip* m_clip;
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
	void operator()(const mpl::at_c<Libvirt::Iface::Xml::VChoice1266::types, 0>::type& ) const
	{
		m_sink->setConfigureWithDhcp(true);
	}
	void operator()(const mpl::at_c<Libvirt::Iface::Xml::VChoice1266::types, 1>::type& ) const
	{
		m_sink->setConfigureWithDhcp(false);
	}

private:
	void consume(const boost::optional<Libvirt::Iface::Xml::VChoice1266 >& ipv4_) const
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

namespace Controller
{

///////////////////////////////////////////////////////////////////////////////
// struct Collect

struct Collect: boost::static_visitor<void>
{
	typedef QList<Libvirt::Domain::Xml::Controller> output_type;

	explicit Collect(output_type& output_) : m_output(&output_)
	{
	}

	template<class T>
	void operator()(const T& ) const
	{
	}

	void operator()(const mpl::at_c<Libvirt::Domain::Xml::VChoice932::types, 1>::type& controller_) const
	{
		m_output->append(controller_.getValue());
	}

private:
	output_type *m_output;
};

///////////////////////////////////////////////////////////////////////////////
// struct Scsi

struct Scsi: boost::static_visitor<void>
{
	explicit Scsi(boost::optional<PRL_CLUSTERED_DEVICE_SUBTYPE>& model_) : m_model(&model_)
	{
	}

	template<class T>
	void operator()(const T& ) const
	{
	}

	void operator()(const mpl::at_c<Libvirt::Domain::Xml::VChoice587::types, 1>::type& model_) const
	{
		if (!model_.getValue())
			return;
		switch (model_.getValue().get())
		{
		case Libvirt::Domain::Xml::EModelBuslogic:
			m_model->reset(PCD_BUSLOGIC);
			break;
		case Libvirt::Domain::Xml::EModelLsilogic:
			m_model->reset(PCD_LSI_SPI);
			break;
		case Libvirt::Domain::Xml::EModelLsisas1068:
		case Libvirt::Domain::Xml::EModelLsisas1078:
			m_model->reset(PCD_LSI_SAS);
			break;
		case Libvirt::Domain::Xml::EModelVirtioScsi:
			m_model->reset(PCD_VIRTIO_SCSI);
			break;
		default:
			break;
		}
	}

private:
	boost::optional<PRL_CLUSTERED_DEVICE_SUBTYPE> *m_model;
};

///////////////////////////////////////////////////////////////////////////////
// struct Usb

struct Usb: boost::static_visitor<void>
{
	Usb(CVmUsbController& settings_, CVmHardware& hardware_)
		: m_settings(&settings_), m_hardware(&hardware_)
	{
	}

	template<class T>
	void operator()(const T& ) const
	{
	}

	void operator()(const mpl::at_c<Libvirt::Domain::Xml::VChoice587::types, 2>::type& usb_) const;
private:
	CVmUsbController *m_settings;
	CVmHardware *m_hardware;
};

} // namespace Controller

namespace Address
{
///////////////////////////////////////////////////////////////////////////////
// struct Stringify

struct Stringify: boost::static_visitor<QString>
{
	template<class T>
	QString operator()(const T& t) const
	{
		return QString("%1").arg(t.getValue());
	}
	QString operator()(const mpl::at_c<Libvirt::Domain::Xml::VIpPrefix::types, 0>::type& prefix4_) const
	{
		return PrlNet::getIPv4MaskFromPrefix(prefix4_.getValue()).toString();
	}
};

///////////////////////////////////////////////////////////////////////////////
// struct Ip

struct Ip: boost::static_visitor<void>
{
	explicit Ip(CHostOnlyNetwork& result_): m_result(&result_)
	{
	}
	
	template<class T>
	void operator()(const T& ) const
	{
	}
	void operator()(const mpl::at_c<Libvirt::Network::Xml::VIpAddr::types, 0>::type& addr4_) const
	{
		m_result->setHostIPAddress(QHostAddress(addr4_.getValue()));
		m_result->setDhcpIPAddress(QHostAddress(addr4_.getValue()));
	}
	void operator()(const mpl::at_c<Libvirt::Network::Xml::VIpAddr::types, 1>::type& addr6_) const
	{
		m_result->setHostIP6Address(QHostAddress(addr6_.getValue()));
		m_result->setDhcpIP6Address(QHostAddress(addr6_.getValue()));
	}

private:
	CHostOnlyNetwork* m_result;
};

///////////////////////////////////////////////////////////////////////////////
// struct Mask

struct Mask: boost::static_visitor<void>
{
	explicit Mask(CHostOnlyNetwork& result_): m_result(&result_)
	{
	}
	
	template<class T>
	void operator()(const T& ) const
	{
	}
	void operator()(const mpl::at_c<Libvirt::Network::Xml::VChoice1166::types, 0>::type& addr_) const
	{
		m_result->setIPNetMask(QHostAddress(addr_.getValue()));
	}
	void operator()(const mpl::at_c<Libvirt::Network::Xml::VChoice1166::types, 1>::type& prefix_) const
	{
		boost::apply_visitor(*this, prefix_.getValue());
	}
	void operator()(const mpl::at_c<Libvirt::Network::Xml::VIpPrefix::types, 0>::type& prefix4_) const
	{
		m_result->setIPNetMask(PrlNet::getIPv4MaskFromPrefix(prefix4_.getValue()));
	}
	void operator()(const mpl::at_c<Libvirt::Network::Xml::VIpPrefix::types, 1>::type& prefix6_) const
	{
		m_result->setIP6NetMask(PrlNet::getIPv6MaskFromPrefix(prefix6_.getValue()));
	}

private:
	CHostOnlyNetwork* m_result;
};

} // namespace Address

namespace Dhcp
{

///////////////////////////////////////////////////////////////////////////////
// struct Start

struct Start: boost::static_visitor<void>
{
	explicit Start(CHostOnlyNetwork& result_): m_result(&result_)
	{
	}
	
	template<class T>
	void operator()(const T& ) const
	{
	}
	void operator()(const mpl::at_c<Libvirt::Network::Xml::VIpAddr::types, 0>::type& addr4_) const
	{
		m_result->getDHCPServer()->setIPScopeStart(QHostAddress(addr4_.getValue()));
	}
	void operator()(const mpl::at_c<Libvirt::Network::Xml::VIpAddr::types, 1>::type& addr6_) const
	{
		m_result->getDHCPv6Server()->setIPScopeStart(QHostAddress(addr6_.getValue()));
	}

private:
	CHostOnlyNetwork* m_result;
};

///////////////////////////////////////////////////////////////////////////////
// struct End

struct End: boost::static_visitor<void>
{
	explicit End(CHostOnlyNetwork& result_): m_result(&result_)
	{
	}
	
	template<class T>
	void operator()(const T& ) const
	{
	}
	void operator()(const mpl::at_c<Libvirt::Network::Xml::VIpAddr::types, 0>::type& addr4_) const
	{
		m_result->getDHCPServer()->setIPScopeEnd(QHostAddress(addr4_.getValue()));
	}
	void operator()(const mpl::at_c<Libvirt::Network::Xml::VIpAddr::types, 1>::type& addr6_) const
	{
		m_result->getDHCPv6Server()->setIPScopeEnd(QHostAddress(addr6_.getValue()));
	}

private:
	CHostOnlyNetwork* m_result;
};

} // namespace Dhcp

namespace Bridge
{

struct Master: boost::static_visitor<bool>
{
	explicit Master(CHwNetAdapter& result_): m_result(&result_)
	{
	}

	template<class T>
	bool operator()(const T&) const
	{
		return false;
	}
	bool operator()(const mpl::at_c<Libvirt::Iface::Xml::VChoice1230::types, 0>::type& ethernet_) const
	{
		m_result->setDeviceName(ethernet_.getValue().getName());
		if (ethernet_.getValue().getMac())
		{
			m_result->setMacAddress(ethernet_.getValue()
				.getMac().get().toUpper().remove(QString(":")));
		}
		return true;
	}
	bool operator()(const mpl::at_c<Libvirt::Iface::Xml::VChoice1230::types, 1>::type& vlan_) const
	{
		Libvirt::Iface::Xml::VlanInterfaceCommon c = vlan_.getValue().getVlanInterfaceCommon();
		if (c.getName())
			m_result->setDeviceName(c.getName().get());

		Libvirt::Iface::Xml::Vlan v = vlan_.getValue().getVlan();
		m_result->setVLANTag(v.getTag());
		return true;
	}

private:
	CHwNetAdapter* m_result;
};

} // namespace Bridge

} // namespace Visitor
} // namespace Transponster

#endif // __DIRECT_P_H__

