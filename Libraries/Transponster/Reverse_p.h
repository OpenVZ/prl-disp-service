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
#include <XmlModel/VtInfo/VtInfo.h>
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

struct List;
typedef QList<Libvirt::Domain::Xml::VChoice932 > deviceList_type;

namespace Clustered
{
///////////////////////////////////////////////////////////////////////////////
// struct Flavor

template<class T>
struct Flavor;

template<>
struct Flavor<CVmHardDisk>
{
	typedef PVE::HardDiskEmulatedType emulated_type;

	static const Libvirt::Domain::Xml::EDevice kind;
	static const emulated_type real;
	static const emulated_type image;
	static const bool readonly = false;
	static const boost::none_t snapshot;

	static const char* getTarget()
	{
		return "hd";
	}
	static mpl::at_c<Libvirt::Domain::Xml::VStorageFormat::types, 1>::type
		getDriverFormat()
	{
		mpl::at_c<Libvirt::Domain::Xml::VStorageFormat::types, 1>::type output;
		output.setValue(Libvirt::Domain::Xml::EStorageFormatBackingQcow2);
		return output;
	}
	static boost::optional<Libvirt::Domain::Xml::ETray> getTray(int )
	{
		return boost::optional<Libvirt::Domain::Xml::ETray>();
	}
};

///////////////////////////////////////////////////////////////////////////////
// struct Model

template<class T>
struct Model
{
	explicit Model(const T& dataSource_): m_dataSource(&dataSource_)
	{
	}

	QString getTargetName() const
	{
		int i = m_dataSource->getIndex();
		// convert digital index to string
		// 1 -> "a", 2 -> "b", .., 26 -> "ba"

		// 26 is the alphabet size
		QString e('a' + i % 26);

		while ((i /= 26) > 0)
			e.prepend('a' + i % 26);

		return QString(Flavor<T>::getTarget()) + e;
	}
	QString getImageFile() const
	{
		QString f = m_dataSource->getUserFriendlyName();
		if (!f.isEmpty())
			return f;

		return m_dataSource->getSystemName();
	}
	QString getRealDeviceName() const
	{
		QRegExp r("[(]([^)]+)[)]");
		if (-1 < r.indexIn(m_dataSource->getUserFriendlyName()))
			return r.cap(1);

		return m_dataSource->getSystemName();
	}
	typename Flavor<T>::emulated_type getEmulatedType() const
	{
		return m_dataSource->getEmulatedType();
	}
	boost::optional<Libvirt::Domain::Xml::EBus> getBus() const
	{
		switch (m_dataSource->getInterfaceType())
		{
		case PMS_IDE_DEVICE:
			return Libvirt::Domain::Xml::EBusIde;
		case PMS_SCSI_DEVICE:
			return Libvirt::Domain::Xml::EBusScsi;
		case PMS_SATA_DEVICE:
			return Libvirt::Domain::Xml::EBusSata;
		case PMS_VIRTIO_BLOCK_DEVICE:
			return Libvirt::Domain::Xml::EBusVirtio;
		default:
			return boost::optional<Libvirt::Domain::Xml::EBus>();
		}
	}
	bool isConnected() const
	{
		return PVE::DeviceConnected == m_dataSource->getConnected();
	}

	boost::optional<Libvirt::Domain::Xml::EModel> getScsiModel() const
	{
		switch (m_dataSource->getSubType())
		{
		case PCD_BUSLOGIC:
			return Libvirt::Domain::Xml::EModelBuslogic;
		case PCD_LSI_SAS:
			return Libvirt::Domain::Xml::EModelLsisas1078;
		case PCD_LSI_SPI:
			return Libvirt::Domain::Xml::EModelLsilogic;
		case PCD_VIRTIO_SCSI:
			return Libvirt::Domain::Xml::EModelVirtioScsi;
		default:
			return boost::optional<Libvirt::Domain::Xml::EModel>();
		}
	}

protected:
	const T* getDataSource() const
	{
		return m_dataSource;
	}
private:
	const T* m_dataSource;
};

namespace Builder
{
///////////////////////////////////////////////////////////////////////////////
// struct Ordinary

template<class T>
struct Ordinary
{
	typedef Boot::Reverse::order_type boot_type;
	typedef Libvirt::Domain::Xml::Disk result_type;

	Ordinary(const T& dataSource_, const boot_type& boot_ = boot_type()):
		m_model(dataSource_), m_boot(boot_)
	{
	}

	void setDisk();
	void setFlags();
	void setDriver();
	void setSource()
	{
		m_result.setDiskSource(getSource());
	}
	void setTarget();
	void setBackingChain();

	const result_type& getResult() const
	{
		return m_result;
	}

	const Model<T>& getModel() const
	{
		return m_model;
	}

protected:
	result_type& getResult()
	{
		return m_result;
	}

	Libvirt::Domain::Xml::VDiskSource getSource() const;

private:
	Model<T> m_model;
	boot_type m_boot;
	result_type m_result;
};

template<class T>
void Ordinary<T>::setDisk()
{
	mpl::at_c<Libvirt::Domain::Xml::VDisk::types, 0>::type x;
	x.setValue(Flavor<T>::kind);
	m_result.setDisk(Libvirt::Domain::Xml::VDisk(x));
}

template<class T>
void Ordinary<T>::setFlags()
{
	// boot
	m_result.setBoot(m_boot);
	// connected
	if (Flavor<T>::image == getModel().getEmulatedType() &&
		!getModel().isConnected() && !getModel().getImageFile().isEmpty())
		m_result.setSerial(QString(getModel().getImageFile().toUtf8().toHex()));

	// readonly
	m_result.setReadonly(Flavor<T>::readonly);
	// snapshot
	if (Flavor<T>::image == getModel().getEmulatedType())
		m_result.setSnapshot(Flavor<T>::snapshot);
	else
		m_result.setSnapshot(Libvirt::Domain::Xml::ESnapshotNo);
}

template<class T>
void Ordinary<T>::setDriver()
{
	if (Flavor<T>::image != getModel().getEmulatedType())
		return;

	mpl::at_c<Libvirt::Domain::Xml::VType::types, 1>::type a;
	a.setValue(Libvirt::Domain::Xml::VStorageFormat(Flavor<T>::getDriverFormat()));
	Libvirt::Domain::Xml::DriverFormat b;
	b.setName("qemu");
	b.setType(Libvirt::Domain::Xml::VType(a));
	Libvirt::Domain::Xml::Driver d;
	d.setCache(Libvirt::Domain::Xml::ECacheNone);
	d.setIo(Libvirt::Domain::Xml::EIoNative);
	d.setDiscard(Libvirt::Domain::Xml::EDiscardUnmap);
	d.setDriverFormat(b);
	m_result.setDriver(d);
}

template<class T>
Libvirt::Domain::Xml::VDiskSource Ordinary<T>::getSource() const
{
	switch (getModel().getEmulatedType())
	{
	case Flavor<T>::image:
	{
		Libvirt::Domain::Xml::Source s;
		QString f = getModel().getImageFile();
		if (!f.isEmpty() && getModel().isConnected())
			s.setFile(f);
		s.setStartupPolicy(Libvirt::Domain::Xml::EStartupPolicyOptional);
		mpl::at_c<Libvirt::Domain::Xml::VDiskSource::types, 0>::type x;
		x.setValue(s);
		return x;
	}
	case Flavor<T>::real:
	{
		Libvirt::Domain::Xml::Source1 s;
		s.setDev(getModel().getRealDeviceName());
		mpl::at_c<Libvirt::Domain::Xml::VDiskSource::types, 1>::type x;
		x.setValue(s);
		return x;
	}
	default:
		return Libvirt::Domain::Xml::VDiskSource();
	}
}

template<class T>
void Ordinary<T>::setTarget()
{
	Libvirt::Domain::Xml::Target t;
	t.setBus(getModel().getBus());
	t.setDev(getModel().getTargetName());
	t.setTray(Flavor<T>::getTray(getModel().getEmulatedType()));
	m_result.setTarget(t);
}

template<class T>
void Ordinary<T>::setBackingChain()
{
	mpl::at_c<Libvirt::Domain::Xml::VDiskBackingChain::types, 1>::type x;
	x.setValue(false);
	m_result.setDiskBackingChain(Libvirt::Domain::Xml::VDiskBackingChain(x));
}

///////////////////////////////////////////////////////////////////////////////
// struct Hdd

struct Hdd: Ordinary<CVmHardDisk>
{
	Hdd(const CVmHardDisk& dataSource_, const boot_type& boot_ = boot_type()):
		Ordinary<CVmHardDisk>(dataSource_, boot_), m_hdd(dataSource_)
	{
	}

	void setIoLimit(const CVmIoLimit* global_);
	void setIopsLimit(const CVmRunTimeOptions& runtime_);

private:
	const CVmHardDisk& m_hdd;
};

} // namespace Builder
} // namespace Clustered

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
// struct Address

struct Address
{
	Address& setUnit(quint16 unit_)
	{
		m_address.setUnit(QString::number(unit_));
		return *this;
	}
	Address& setTarget(quint16 target_)
	{
		m_address.setTarget(QString::number(target_));
		return *this;
	}
	Address& setBus(quint16 bus_)
	{
		m_address.setBus(QString::number(bus_));
		return *this;
	}	
	Libvirt::Domain::Xml::VAddress operator()(quint16 controller_)
	{
		address_type a;
		std::swap(a, m_address);
		a.setController(QString::number(controller_));
		mpl::at_c<Libvirt::Domain::Xml::VAddress::types, 1>::type v;
		v.setValue(a);
		return Libvirt::Domain::Xml::VAddress(v);
	}

private:
	typedef Libvirt::Domain::Xml::Driveaddress address_type;

	address_type m_address;
};

///////////////////////////////////////////////////////////////////////////////
// struct Attachment

struct Attachment
{
	Attachment(): m_ide(0), m_sata(0)
	{
	}

	Libvirt::Domain::Xml::VAddress craftIde();
	Libvirt::Domain::Xml::VAddress craftSata();
	Libvirt::Domain::Xml::VAddress craftScsi(const boost::optional<Libvirt::Domain::Xml::EModel>& model_);
	deviceList_type getControllers() const
	{
		return m_controllerList;
	}

private:
	enum
	{
		IDE_UNITS = 2,
		IDE_BUSES = 2,
		SATA_UNITS = 6,
		SCSI_TARGETS = 256
	};

	void craftController(const Libvirt::Domain::Xml::VChoice587& bus_, quint16 index_);

	quint16 m_ide;
	quint16 m_sata;
	QMap<Libvirt::Domain::Xml::EModel, quint16> m_scsi;
	deviceList_type m_controllerList;
};

///////////////////////////////////////////////////////////////////////////////
// struct List

struct List
{
	explicit List();

	Libvirt::Domain::Xml::Devices getResult() const;
	void add(const CVmRemoteDisplay* vnc_);
	void add(const Libvirt::Domain::Xml::Disk& disk_);
	void add(const CVmParallelPort* port_);
	void add(const CVmSerialPort* port_);
	void add(const CVmSoundDevice* sound_);
	void add(const CVmVideo* video_);
	void add(const CVmGenericNetworkAdapter* network_);

private:
	template<int N, class T>
	void add(const T& value_)
	{
		typename mpl::at_c<deviceList_type::value_type::types, N>::type x;
		x.setValue(value_);
		m_deviceList << x;
	}
	Libvirt::Domain::Xml::Panic craftPanic() const;

	deviceList_type m_deviceList;
};

namespace Clustered
{

///////////////////////////////////////////////////////////////////////////////
// struct List

struct List
{
	explicit List(const Boot::Reverse& boot_, Device::List& list_);

	const Attachment& getAttachment() const;

	void add(const CVmHardDisk* hdd_, const CVmRunTimeOptions* runtime_);
	void add(const CVmOpticalDisk* cdrom_);
	void add(const CVmFloppyDisk* floppy_);

private:
	template<class T>
	void build(T builder_);

private:
	Boot::Reverse m_boot;
	Device::List& m_deviceList;
	Attachment m_attachment;
};

template<class T>
void List::build(T builder_)
{
	builder_.setDisk();
	builder_.setFlags();
	builder_.setDriver();
	builder_.setSource();
	builder_.setTarget();
	builder_.setBackingChain();

	Libvirt::Domain::Xml::Disk d = static_cast<const T&>(builder_).getResult();
	switch (d.getTarget().getBus().get())
	{
	case Libvirt::Domain::Xml::EBusIde:
		d.setAddress(m_attachment.craftIde());
		break;
	case Libvirt::Domain::Xml::EBusSata:
		d.setAddress(m_attachment.craftSata());
		break;
	case Libvirt::Domain::Xml::EBusScsi:
		d.setAddress(m_attachment.craftScsi(builder_.getModel().getScsiModel()));
		break;
	default:
		break;
	}
	m_deviceList.add(d);
}

} // namespace Clustered

namespace Usb
{

///////////////////////////////////////////////////////////////////////////////
// struct List
struct List
{
	List(const CVmUsbController* settings_) : m_settings(settings_), m_controller(0)
	{
	}

	deviceList_type getDevices() const
	{
		return m_deviceList;
	}

	void add(const CVmUsbDevice* usb_);
	void addKeyboard();
	void addMouse();

private:
	template<int N, class T>
	void add(const T& value_)
	{
		typename mpl::at_c<deviceList_type::value_type::types, N>::type x;
		x.setValue(value_);
		m_deviceList << x;
	}

	void craftController(Libvirt::Domain::Xml::EModel1 model_);
	void add(Libvirt::Domain::Xml::EType10 type_);

	const CVmUsbController* m_settings;
	deviceList_type m_deviceList;
	quint16 m_controller;
};

} // namespace Usb
} // namespace Device

///////////////////////////////////////////////////////////////////////////////
// struct Resources

struct Resources
{
	explicit Resources(CVmConfiguration& config_): m_config(&config_)
	{
	}

	void setVCpu(const Libvirt::Domain::Xml::VCpu& src_);
	bool getVCpu(Libvirt::Domain::Xml::VCpu& dst_);
	void setCpu(const Libvirt::Domain::Xml::Domain& vm_, const VtInfo& vt_);
	bool getCpu(const VtInfo& vt_, Libvirt::Domain::Xml::Domain& dst_);
	void setClock(const Libvirt::Domain::Xml::Clock& src_);
	bool getClock(Libvirt::Domain::Xml::Clock& dst_);
	void setMemory(const Libvirt::Domain::Xml::Memory& src_);
	bool getMemory(Libvirt::Domain::Xml::Memory& dst_);
	void setChipset(const Libvirt::Domain::Xml::Sysinfo& src_);

private:
	CVmHardware* getHardware()
	{
		return m_config->getVmHardwareList();
	}

	CVmConfiguration* m_config;
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

