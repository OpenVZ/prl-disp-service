/*
 * Copyright (c) 2015 Parallels IP Holdings GmbH
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
 * Our contact details: Parallels IP Holdings GmbH, Vordergasse 59, 8200
 * Schaffhausen, Switzerland.
 */

#include "Reverse.h"
#include "Reverse_p.h"
#include "Direct_p.h"

namespace Transponster
{
///////////////////////////////////////////////////////////////////////////////
// struct Resources

void Resources::setCpu(const Libvirt::Domain::Xml::Vcpu& src_)
{
	CVmCpu* u = new CVmCpu();
	u->setNumber(src_.getOwnValue());
	if (src_.getCpuset())
		u->setCpuMask(src_.getCpuset().get());

	m_hardware->setCpu(u);
}

bool Resources::getCpu(Libvirt::Domain::Xml::Vcpu& dst_)
{
	CVmCpu* u = m_hardware->getCpu();
	if (NULL == u)
		return false;

	dst_.setOwnValue(u->getNumber());
	QString m = u->getCpuMask();
	if (!m.isEmpty())
		dst_.setCpuset(m);

	return true;
}

void Resources::setClock(const Libvirt::Domain::Xml::Clock& src_)
{
	::Clock* c = new ::Clock();
	boost::apply_visitor(Visitor::Clock(*c), src_.getClock());
	m_hardware->setClock(c);
}

bool Resources::getClock(Libvirt::Domain::Xml::Clock& dst_)
{
	::Clock* c = m_hardware->getClock();
	if (NULL == c)
		return false;

	if (0 == c->getTimeShift())
		return false;

	Libvirt::Domain::Xml::Clock367 k;
	k.setAdjustment(QString::number(c->getTimeShift()));
	mpl::at_c<Libvirt::Domain::Xml::VClock::types, 2>::type a;
	a.setValue(k);
	dst_.setClock(a);
	return true;
}

void Resources::setMemory(const Libvirt::Domain::Xml::Memory& src_)
{
	CVmMemory* m = new CVmMemory();
	m->setRamSize(src_.getScaledInteger().getOwnValue() >> 10);
	m_hardware->setMemory(m);
}

bool Resources::getMemory(Libvirt::Domain::Xml::Memory& dst_)
{
	CVmMemory* m = m_hardware->getMemory();
	if (NULL == m)
		return false;

	Libvirt::Domain::Xml::ScaledInteger v;
	v.setOwnValue(m->getRamSize() << 10);
	dst_.setScaledInteger(v);
	return true;
}

void Resources::setChipset(const Libvirt::Domain::Xml::Sysinfo& src_)
{
	(void)src_;
	Chipset* c = new Chipset();
	m_hardware->setChipset(c);
}

namespace Device
{
///////////////////////////////////////////////////////////////////////////////
// struct Clustered

template<>
boost::optional<Libvirt::Domain::Xml::EBus> Clustered<CVmFloppyDisk>::getBus() const
{
	return Libvirt::Domain::Xml::EBusFdc;
}

template<>
boost::optional<Libvirt::Domain::Xml::ETray> Clustered<CVmHardDisk>::getTray() const
{
	return boost::optional<Libvirt::Domain::Xml::ETray>();
}

///////////////////////////////////////////////////////////////////////////////
// struct Flavor

template<>
struct Flavor<CVmHardDisk>
{
	static const Libvirt::Domain::Xml::EDevice kind = Libvirt::Domain::Xml::EDeviceDisk;
	static const int real = PVE::RealHardDisk;
	static const int image = PVE::HardDiskImage;
	static const bool readonly = false;

	static const char* getTarget()
	{
		return "hda";
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

template<>
struct Flavor<CVmOpticalDisk>
{
	static const Libvirt::Domain::Xml::EDevice kind = Libvirt::Domain::Xml::EDeviceCdrom;
	static const int real = PVE::RealCdRom;
	static const int image = PVE::CdRomImage;
	static const bool readonly = true;

	static const char* getTarget()
	{
		return "sda";
	}
	static mpl::at_c<Libvirt::Domain::Xml::VStorageFormat::types, 0>::type
		getDriverFormat()
	{
		mpl::at_c<Libvirt::Domain::Xml::VStorageFormat::types, 0>::type output;
		output.setValue(Libvirt::Domain::Xml::EStorageFormatRaw);
		return output;
	}
	static boost::optional<Libvirt::Domain::Xml::ETray> getTray(int type_)
	{
		switch (type_)
		{
		case real:
			return Libvirt::Domain::Xml::ETrayOpen;
		case image:
			return Libvirt::Domain::Xml::ETrayClosed;
		default:
			return boost::optional<Libvirt::Domain::Xml::ETray>(); 
		}
	}
};

template<>
struct Flavor<CVmFloppyDisk>
{
	static const Libvirt::Domain::Xml::EDevice kind = Libvirt::Domain::Xml::EDeviceFloppy;
	static const int real = PVE::RealFloppyDisk;
	static const int image = PVE::FloppyDiskImage;
	static const bool readonly = false;

	static const char* getTarget()
	{
		return "fda";
	}
	static mpl::at_c<Libvirt::Domain::Xml::VStorageFormat::types, 0>::type
		getDriverFormat()
	{
		mpl::at_c<Libvirt::Domain::Xml::VStorageFormat::types, 0>::type output;
		output.setValue(Libvirt::Domain::Xml::EStorageFormatRaw);
		return output;
	}
	static boost::optional<Libvirt::Domain::Xml::ETray> getTray(int type_)
	{
		switch (type_)
		{
		case real:
			return Libvirt::Domain::Xml::ETrayOpen;
		default:
			return boost::optional<Libvirt::Domain::Xml::ETray>(); 
		}
	}
};

///////////////////////////////////////////////////////////////////////////////
// struct Disk

Libvirt::Domain::Xml::VDiskSource Disk::getSource() const
{
	Libvirt::Domain::Xml::VDiskSource x = Clustered<CVmHardDisk>::getSource();
	if (!x.empty())
		return x;

	if (PVE::BootCampHardDisk == getDevice().getEmulatedType())
	{
		Libvirt::Domain::Xml::Source4 s;
		s.setVolume(getDevice().getSystemName());
		mpl::at_c<Libvirt::Domain::Xml::VDiskSource::types, 4>::type x;
		x.setValue(s);
		return x;
	}
	return Libvirt::Domain::Xml::VDiskSource();
}

///////////////////////////////////////////////////////////////////////////////
// struct Network

template<int N>
Libvirt::Domain::Xml::VInterface Network<N>::operator()
	(const CVmGenericNetworkAdapter& network_)
{
	typename Libvirt::Details::Value::Grab<access_type>::type i = prepare(network_);

	i.setAlias(network_.getSystemName());
	QString m = network_.getMacAddress().replace(QRegExp("([^:]{2})(?!:|$)"), "\\1:");
	if (!m.isEmpty())
		i.setMac(m);

	access_type output;
	output.setValue(i);
	return output;
}

template<>
Libvirt::Domain::Xml::Interface605 Network<0>::prepare(const CVmGenericNetworkAdapter& network_)
{
	Libvirt::Domain::Xml::Interface605 output;
	output.setModel(QString("virtio"));
	output.setSource(network_.getHostInterfaceName());
	return output;
}

template<>
Libvirt::Domain::Xml::Interface613 Network<3>::prepare(const CVmGenericNetworkAdapter& network_)
{
	Libvirt::Domain::Xml::Interface613 output;
	Libvirt::Domain::Xml::Source7 s;
	s.setNetwork(network_.getVirtualNetworkID());
	output.setModel(QString("virtio"));
	output.setSource(s);
	return output;
}

template<>
Libvirt::Domain::Xml::Interface615 Network<4>::prepare(const CVmGenericNetworkAdapter& network_)
{
	Libvirt::Domain::Xml::Interface615 output;
	Libvirt::Domain::Xml::Source8 s;
	s.setDev(network_.getHostInterfaceName());
	output.setModel(QString("virtio"));
	output.setSource(s);
	return output;
}

///////////////////////////////////////////////////////////////////////////////
// struct List

Libvirt::Domain::Xml::Devices List::getResult() const
{
	Libvirt::Domain::Xml::Devices output;
	output.setEmulator(QString("/usr/libexec/qemu-kvm"));
	output.setChoice912List(m_devices);
	return output;
}

void List::add(const CVmHardDisk* disk_)
{
	if (NULL != disk_)
		add(Disk(*disk_, m_boot(*disk_)));
}

void List::add(const CVmParallelPort* port_)
{
	if (NULL == port_)
		return;

	Libvirt::Domain::Xml::Source15 a;
	a.setPath(port_->getUserFriendlyName());
	Libvirt::Domain::Xml::QemucdevSrcDef b;
	b.setSourceList(QList<Libvirt::Domain::Xml::Source15 >() << a);
	Libvirt::Domain::Xml::Qemucdev p;
	p.setQemucdevSrcDef(b);
	add<11>(p);
}

void List::add(const CVmSerialPort* port_)
{
	if (NULL == port_)
		return;

	if (PVE::SerialOutputFile != port_->getEmulatedType())
		return;

	Libvirt::Domain::Xml::Source15 a;
	a.setPath(port_->getUserFriendlyName());
	if (a.getPath().get().isEmpty())
		return;

	Libvirt::Domain::Xml::QemucdevSrcDef b;
	b.setSourceList(QList<Libvirt::Domain::Xml::Source15 >() << a);
	Libvirt::Domain::Xml::Qemucdev p;
	p.setType(Libvirt::Domain::Xml::EQemucdevSrcTypeChoiceFile);
	p.setQemucdevSrcDef(b);
	add<12>(p);
}

void List::add(const CVmOpticalDisk* cdrom_)
{
	if (NULL != cdrom_ && cdrom_->getEmulatedType() != Flavor<CVmOpticalDisk>::real)
		add(Cdrom(*cdrom_, m_boot(*cdrom_)));
}

void List::add(const CVmSoundDevice* sound_)
{
	if (NULL != sound_)
	{
		Libvirt::Domain::Xml::Sound s;
		s.setAlias(sound_->getUserFriendlyName());
		add<6>(s);
	}
}

void List::add(const CVmFloppyDisk* floppy_)
{
	if (NULL != floppy_)
		add(Floppy(*floppy_));
}

void List::add(const CVmRemoteDisplay* vnc_)
{
	if (NULL == vnc_)
		return;

	Libvirt::Domain::Xml::Variant665 v;
	v.setPort(vnc_->getPortNumber());
	v.setListen(vnc_->getHostName());
	if (PRD_AUTO == vnc_->getMode())
		v.setAutoport(Libvirt::Domain::Xml::EAutoportYes);

	mpl::at_c<Libvirt::Domain::Xml::VChoice667::types, 0>::type y;
	y.setValue(v);
	Libvirt::Domain::Xml::Graphics674 g;
	g.setChoice667(Libvirt::Domain::Xml::VChoice667(y));
	QString p = vnc_->getPassword();
	if (!p.isEmpty())
		g.setPasswd(p);

	mpl::at_c<Libvirt::Domain::Xml::VGraphics::types, 1>::type z;
	z.setValue(g);
	add<8>(Libvirt::Domain::Xml::VGraphics(z));
}

void List::add(const CVmVideo* video_)
{
	if (NULL == video_)
		return;

	Libvirt::Domain::Xml::Model1 m;
	m.setVram(video_->getMemorySize() << 10);
	if (P3D_DISABLED != video_->getEnable3DAcceleration())
	{
		Libvirt::Domain::Xml::Acceleration a;
		a.setAccel3d(Libvirt::Domain::Xml::EAccel3dYes);
		m.setAcceleration(a);
	}
	Libvirt::Domain::Xml::Video v;
	v.setModel(m);
	add<9>(v);
}

void List::add(const CVmGenericNetworkAdapter* network_)
{
	if (NULL == network_)
		return;

	switch (network_->getEmulatedType())
	{
	case PNA_BRIDGED_ETHERNET:
		if (network_->getVirtualNetworkID().isEmpty())
			return add<4>(Network<0>()(*network_));
		else
			return add<4>(Network<3>()(*network_));

	case PNA_DIRECT_ASSIGN:
		add<4>(Network<4>()(*network_));
	default:
		return;
	}
}

} // namespace Devices

namespace Vm
{
///////////////////////////////////////////////////////////////////////////////
// struct Reverse

Reverse::Reverse(const CVmConfiguration& input_): m_input(input_)
{
	QString x;
	CVmHardware* h;
	CVmIdentification* i = m_input.getVmIdentification();
	if (NULL == i)
		goto bad;

	x = QFileInfo(i->getHomePath()).absolutePath();
	if (x.isEmpty())
		goto bad;

	h = m_input.getVmHardwareList();
	if (NULL == h)
		goto bad;

	h->RevertDevicesPathToAbsolute(x);
	return;
bad:
	m_input.setVmType(PVT_CT);
}

PRL_RESULT Reverse::setBlank()
{
	if (PVT_VM != m_input.getVmType())
		return PRL_ERR_BAD_VM_CONFIG_FILE_SPECIFIED;

	m_result.reset(new Libvirt::Domain::Xml::Domain());
	m_result->setType(Libvirt::Domain::Xml::ETypeKvm);
	m_result->setOs(mpl::at_c<Libvirt::Domain::Xml::VOs::types, 1>::type());
	return PRL_ERR_SUCCESS;
}

PRL_RESULT Reverse::setIdentification()
{
	if (m_result.isNull())
		return PRL_ERR_UNINITIALIZED;

	CVmIdentification* i = m_input.getVmIdentification();
	if (NULL == i)
		return PRL_ERR_BAD_VM_CONFIG_FILE_SPECIFIED;
		
	mpl::at_c<Libvirt::Domain::Xml::VUUID::types, 1>::type u;
	u.setValue(::Uuid(i->getVmUuid()).toStringWithoutBrackets());
	Libvirt::Domain::Xml::Ids x;
	x.setUuid(Libvirt::Domain::Xml::VUUID(u));
	x.setName(i->getVmName());
	m_result->setIds(x);
	return PRL_ERR_SUCCESS;
}

PRL_RESULT Reverse::setSettings()
{
	if (m_result.isNull())
		return PRL_ERR_UNINITIALIZED;

	CVmSettings* s = m_input.getVmSettings();
	if (NULL == s)
		return PRL_ERR_BAD_VM_CONFIG_FILE_SPECIFIED;

	CVmCommonOptions* o = s->getVmCommonOptions();
	if (NULL == s)
		return PRL_ERR_BAD_VM_CONFIG_FILE_SPECIFIED;

	QString d = o->getVmDescription();
	if (!d.isEmpty())
		m_result->setDescription(d);

	return PRL_ERR_SUCCESS;
}

PRL_RESULT Reverse::setDevices()
{
	if (m_result.isNull())
		return PRL_ERR_UNINITIALIZED;

	CVmHardware* h = m_input.getVmHardwareList();
	if (NULL == h)
		return PRL_ERR_BAD_VM_CONFIG_FILE_SPECIFIED;

	Device::List b(Boot::Reverse(m_input.getVmSettings()->
		getVmStartupOptions()->getBootDeviceList()));
	b.add(m_input.getVmSettings()->getVmRemoteDisplay());
	foreach (const CVmVideo* d, h->m_lstVideo)
	{
		b.add(d);
	}
	foreach (const CVmFloppyDisk* d, h->m_lstFloppyDisks)
	{
		b.add(d);
	}
	foreach (const CVmOpticalDisk* d, h->m_lstOpticalDisks)
	{
		b.add(d);
	}
	foreach (const CVmHardDisk* d, h->m_lstHardDisks)
	{
		b.add(d);
	}
	foreach (const CVmSerialPort* d, h->m_lstSerialPorts)
	{
		b.add(d);
	}
//	foreach (const CVmParallelPort* d, h->m_lstParallelPorts)
//	{
//		b.add(d);
//	}
	foreach (const CVmGenericNetworkAdapter* d, h->m_lstNetworkAdapters)
	{
		b.add(d);
	}
	foreach (const CVmSoundDevice* d, h->m_lstSoundDevices)
	{
		b.add(d);
	}
	m_result->setDevices(b.getResult());
	return PRL_ERR_SUCCESS;
}

PRL_RESULT Reverse::setResources()
{
	if (m_result.isNull())
		return PRL_ERR_UNINITIALIZED;

	CVmHardware* h = m_input.getVmHardwareList();
	if (NULL == h)
		return PRL_ERR_BAD_VM_CONFIG_FILE_SPECIFIED;

	Resources u(*h);
	Libvirt::Domain::Xml::Memory m;
	if (u.getMemory(m))
		m_result->setMemory(m);

	Libvirt::Domain::Xml::Vcpu c;
	if (u.getCpu(c))
		m_result->setVcpu(c);

	Libvirt::Domain::Xml::Clock t;
	if (u.getClock(t))
		m_result->setClock(t);

	return PRL_ERR_SUCCESS;
}

QString Reverse::getResult()
{
	if (m_result.isNull())
		return QString();

	QDomDocument x;
	m_result->save(x);
	m_result.reset();
	return x.toString();
}

} // namespace Vm

namespace Network
{
namespace Address
{
namespace
{
int cast(const quint8* start_, const quint8* end_)
{
	int output = 0;
	while (start_ < end_)
	{
		switch (*start_)
		{
		case 255:
			output += 8;
			++start_;
			continue;
		default:
			return 0;
		case 254:
			++output;
		case 252:
			++output;
		case 248:
			++output;
		case 240:
			++output;
		case 224:
			++output;
		case 192:
			++output;
		case 128:
			++output;
		case 0:
			break;
		}
		break;
	}
	return output;
}

} // namespace

///////////////////////////////////////////////////////////////////////////////
// struct IPv4

QHostAddress IPv4::patchEnd(const QHostAddress& start_, const QHostAddress& end_)
{
	quint32 a = start_.toIPv4Address();
	quint32 b = end_.toIPv4Address();
	return QHostAddress(a + qMin(0xffffU, b - a));
}

int IPv4::getMask(const QHostAddress& mask_)
{
	quint32 x = qToBigEndian(mask_.toIPv4Address());
	return cast((quint8* )&x, 4 + (quint8* )&x);
}

const char* IPv4::getFamily()
{
	return "ipv4";
}

///////////////////////////////////////////////////////////////////////////////
// struct IPv6

QHostAddress IPv6::patchEnd(const QHostAddress& start_, const QHostAddress& end_)
{
	Q_IPV6ADDR a = start_.toIPv6Address();
	Q_IPV6ADDR b = end_.toIPv6Address();
	if (std::equal(&a.c[0], &a.c[14], &b.c[0]))
		return end_;

	a.c[14] = 0xff;
	a.c[15] = 0xff;
	return QHostAddress(a);
}

int IPv6::getMask(const QHostAddress& mask_)
{
	Q_IPV6ADDR x = mask_.toIPv6Address();
	return cast(x.c, x.c + 16);
}

const char* IPv6::getFamily()
{
	return "ipv6";
}

} // namespace Address

///////////////////////////////////////////////////////////////////////////////
// struct Reverse

Reverse::Reverse(const CVirtualNetwork& input_): m_input(input_)
{
}

PRL_RESULT Reverse::setUuid()
{
	mpl::at_c<Libvirt::Network::Xml::VUUID::types, 1>::type u;
	u.setValue(::Uuid(m_input.getUuid()).toStringWithoutBrackets());
	m_result.setUuid(Libvirt::Network::Xml::VUUID(u));
	return PRL_ERR_SUCCESS;
}

PRL_RESULT Reverse::setName()
{
	m_result.setName(m_input.getNetworkID());
	return PRL_ERR_SUCCESS;
}

PRL_RESULT Reverse::setType()
{
	if (PVN_BRIDGED_ETHERNET == m_input.getNetworkType())
	{
		Libvirt::Network::Xml::Forward f;
		f.setMode(Libvirt::Network::Xml::EModeBridge);
		m_result.setForward(f);
	}
	return PRL_ERR_SUCCESS;
}

PRL_RESULT Reverse::setBridge()
{
	CVZVirtualNetwork* z = m_input.getVZVirtualNetwork();
	if (NULL != z)
	{
		Libvirt::Network::Xml::Bridge b;
		b.setName(z->getBridgeName());
		m_result.setBridge(b);
	}
	else
	{
		Libvirt::Network::Xml::Bridge b;
		b.setStp(Libvirt::Network::Xml::EStpOff);
		m_result.setBridge(b);
		if (!m_input.getBoundCardMac().isEmpty())
			m_result.setMac(m_input.getBoundCardMac());
	}
	return PRL_ERR_SUCCESS;
}

PRL_RESULT Reverse::setVlan()
{
	unsigned short x = m_input.getVLANTag();
	if (Libvirt::Validatable<Libvirt::Network::Xml::PId>::validate(x))
	{
		Libvirt::Network::Xml::Tag t;
		t.setId(x);
		m_result.setVlan(QList<Libvirt::Network::Xml::Tag>() << t);
	}
	return PRL_ERR_SUCCESS;
}

namespace
{
template<class T>
Libvirt::Network::Xml::Ip craft(const CDHCPServer& src_,
			const QHostAddress& host_, const QHostAddress& mask_)
{
	typename mpl::at_c<Libvirt::Network::Xml::VIpAddr::types, T::index>::type a, e;
	a.setValue(src_.getIPScopeStart().toString());
	e.setValue(T::patchEnd(src_.getIPScopeStart(), src_.getIPScopeEnd()).toString());
	Libvirt::Network::Xml::Range r;
	r.setStart(a);
	r.setEnd(e);
	Libvirt::Network::Xml::Dhcp h;
	h.setRangeList(QList<Libvirt::Network::Xml::Range>() << r);
	Libvirt::Network::Xml::Ip output;
	output.setFamily(QString(T::getFamily()));
	output.setDhcp(h);
	a.setValue(host_.toString());
	output.setAddress(Libvirt::Network::Xml::VIpAddr(a));
	typename mpl::at_c<Libvirt::Network::Xml::VIpPrefix::types, T::index>::type p;
	p.setValue(T::getMask(mask_));
	mpl::at_c<Libvirt::Network::Xml::VChoice1143::types, 1>::type m;
	m.setValue(p);
	output.setChoice1143(Libvirt::Network::Xml::VChoice1143(m));

	return output;
}

} // namespace

PRL_RESULT Reverse::setHostOnly()
{
	CHostOnlyNetwork* n = m_input.getHostOnlyNetwork();
	if (NULL == n)
		return PRL_ERR_SUCCESS;

	QList<Libvirt::Network::Xml::Ip> x;
	CDHCPServer* v4 = n->getDHCPServer();
	if (NULL != v4 && v4->isEnabled())
		x << craft<Address::IPv4>(*v4, n->getHostIPAddress(), n->getIPNetMask());

/*
	CDHCPServer* v6 = n->getDHCPv6ServerOrig();
	if (NULL != v6 && v6->isEnabled())
		x << craft<Address::IPv6>(*v6, n->getHostIP6Address(), n->getIP6NetMask());
*/
	if (!x.isEmpty())
		m_result.setIpList(x);

	return PRL_ERR_SUCCESS;
}

QString Reverse::getResult() const
{
	QDomDocument x;
	m_result.save(x);
	return x.toString();
}

} // namespace Network

namespace Interface
{
namespace Bridge
{
///////////////////////////////////////////////////////////////////////////////
// struct Reverse

PRL_RESULT Reverse::setMaster()
{
	Libvirt::Iface::Xml::BasicEthernetContent e;
	e.setMac(m_master.getMacAddress());
	e.setName(m_master.getDeviceName());
	mpl::at_c<Libvirt::Iface::Xml::VChoice1208::types, 0>::type v;
	v.setValue(e);
	Libvirt::Iface::Xml::Bridge b = m_result.getBridge();
	b.setChoice1208List(QList<Libvirt::Iface::Xml::VChoice1208>() << v);
	m_result.setBridge(b);
	return PRL_ERR_SUCCESS;
}

PRL_RESULT Reverse::setBridge()
{
	Libvirt::Iface::Xml::Bridge b = m_result.getBridge();
	b.setDelay(2.0);
	b.setStp(Libvirt::Iface::Xml::EOnOrOffOff);
	m_result.setBridge(b);
	Libvirt::Iface::Xml::InterfaceAddressing1238 h;
	if (!m_master.isConfigureWithDhcp())
	{
		if (!m_master.isConfigureWithDhcpIPv6())
			return PRL_ERR_SUCCESS;
			
		Libvirt::Iface::Xml::Protocol p;
		p.setDhcp(Libvirt::Iface::Xml::Dhcp());
		h.setProtocol2(p);
	}
	mpl::at_c<Libvirt::Iface::Xml::VChoice1244::types, 0>::type a;
	a.setValue(Libvirt::Iface::Xml::Dhcp());
	h.setProtocol(Libvirt::Iface::Xml::VChoice1244(a));
	mpl::at_c<Libvirt::Iface::Xml::VInterfaceAddressing::types, 0>::type v;
	v.setValue(h);
	m_result.setInterfaceAddressing(v);
	return PRL_ERR_SUCCESS;
}

PRL_RESULT Reverse::setInterface()
{
	m_result.setName(m_name);
	return PRL_ERR_SUCCESS;
}

QString Reverse::getResult() const
{
	QDomDocument x;
	m_result.save(x);
	return x.toString();
}

} // namespace Bridge
} // namespace Interface

namespace Boot
{
///////////////////////////////////////////////////////////////////////////////
// struct Reverse

Reverse::Reverse(const QList<device_type* >& list_)
{
	unsigned o = 0;
	std::list<device_type* > x = list_.toStdList();
	x.sort(boost::bind(&device_type::getBootingNumber, _1) <
		boost::bind(&device_type::getBootingNumber, _2));
	foreach(device_type* d, x)
	{
		if (!d->isInUse())
			continue;

		key_type k = qMakePair(d->getType(), d->getIndex());
		m_map[k] = ++o;
	}
}

Reverse::order_type Reverse::operator()(const CVmDevice& device_) const
{
	key_type k = qMakePair(device_.getDeviceType(), device_.getIndex());
	map_type::const_iterator p = m_map.find(k);
	if (m_map.end() == p)
		return order_type();

	return p.value();
}

} // namespace Boot
} // namespace Transponster

