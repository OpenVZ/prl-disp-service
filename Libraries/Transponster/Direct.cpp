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

#include "Direct.h"
#include "Direct_p.h"
#include "Reverse_p.h"
#include <prlsdk/PrlOses.h>
#include <prlcommon/HostUtils/HostUtils.h>

namespace Transponster
{
namespace Visitor
{
namespace Source
{
///////////////////////////////////////////////////////////////////////////////
// struct Unit

bool Unit<CVmHardDisk>::operator()(const mpl::at_c<Libvirt::Domain::Xml::VDiskSource::types, 3>::type& source_) const
{
	const Libvirt::Domain::Xml::Source3* v = source_.getValue().get_ptr();
	if (NULL == v)
		return false;

	getDevice().setSystemName(v->getVolume());
	getDevice().setEmulatedType(PVE::BootCampHardDisk);
	return true;
}

} // namespace Source

///////////////////////////////////////////////////////////////////////////////
// struct Floppy

PRL_RESULT Floppy::operator()(const Libvirt::Domain::Xml::Disk& disk_)
{
	if (!Clustered<CVmFloppyDisk>::operator()(disk_)) {
		// source field can be empty or absent for
		// disconnected floppies so we force emulated
		// type to disk image type
		getDevice().setEmulatedType(PVE::FloppyDiskImage);
	}

	CVmFloppyDisk* d = getResult();
	if (NULL == d)
		return PRL_ERR_UNEXPECTED;

	d->setItemId(m_hardware->m_lstFloppyDisks.size());
	d->setIndex(m_hardware->m_lstFloppyDisks.size());
	m_hardware->addFloppyDisk(d);
	if (disk_.getBoot())
	{
		m_clip->getBootSlot(disk_.getBoot().get())
			.set(d->getDeviceType(), d->getIndex());
	}
	d->setTargetDeviceName(disk_.getTarget().getDev());
	boost::optional<QString> alias(disk_.getAlias());
	if (alias)
		d->setAlias(*alias);
	return PRL_ERR_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
// struct Iotune

void Iotune::operator()(const mpl::at_c<Libvirt::Domain::Xml::VChoice1061::types, 0>::type& iopsLimit_) const
{
	m_disk->setIopsLimit(iopsLimit_.getValue());
}

void Iotune::operator()(const mpl::at_c<Libvirt::Domain::Xml::VChoice1057::types, 0>::type& ioLimit_) const
{
	m_disk->setIoLimit(new CVmIoLimit(PRL_IOLIMIT_BS, ioLimit_.getValue()));
}

///////////////////////////////////////////////////////////////////////////////
// struct Disk

PRL_RESULT Disk::operator()(const Libvirt::Domain::Xml::Disk& disk_)
{
	if (!Clustered<CVmHardDisk>::operator()(disk_))
		return PRL_ERR_UNIMPLEMENTED;

	CVmHardDisk* d = getResult();
	if (NULL == d)
		return PRL_ERR_UNEXPECTED;

	d->setItemId(m_hardware->m_lstHardDisks.size());
	d->setIndex(m_hardware->m_lstHardDisks.size());
	QString dev = disk_.getTarget().getDev();
	d->setStackIndex(Parallels::fromBase26(dev.remove(0, 2)));
	boost::optional<PRL_CLUSTERED_DEVICE_SUBTYPE> m = m_clip->getControllerModel(disk_);
	if (m)
		d->setSubType(m.get());
	m_hardware->addHardDisk(d);
	if (disk_.getBoot())
	{
		m_clip->getBootSlot(disk_.getBoot().get())
			.set(d->getDeviceType(), d->getIndex());
	}
	boost::optional<Libvirt::Domain::Xml::Iotune> t = disk_.getIotune();
	if (t)
	{
		Iotune v(d);
		boost::apply_visitor(v, (*t).getChoice1057());
		boost::apply_visitor(v, (*t).getChoice1061());
	}
	d->setTargetDeviceName(disk_.getTarget().getDev());
	if (disk_.getSerial())
		d->setSerialNumber(disk_.getSerial().get());
	boost::optional<QString> alias(disk_.getAlias());
	if (alias)
		d->setAlias(*alias);
	d->setPassthrough(1 == disk_.getDisk().which());
	return PRL_ERR_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
// struct Cdrom

PRL_RESULT Cdrom::operator()(const Libvirt::Domain::Xml::Disk& disk_)
{
	if (!Clustered<CVmOpticalDisk>::operator()(disk_))
		getDevice().setEmulatedType(PVE::CdRomImage);

	getDevice().setPassthrough(disk_.getTransient() ?
		PVE::PassthroughEnabled : PVE::PassthroughDisabled);

	CVmOpticalDisk* d = getResult();
	if (NULL == d)
		return PRL_ERR_UNEXPECTED;

	d->setItemId(m_hardware->m_lstOpticalDisks.size());
	d->setIndex(m_hardware->m_lstOpticalDisks.size());
	QString dev = disk_.getTarget().getDev();
	d->setStackIndex(Parallels::fromBase26(dev.remove(0, 2)));
	boost::optional<PRL_CLUSTERED_DEVICE_SUBTYPE> m = m_clip->getControllerModel(disk_);
	if (m)
		d->setSubType(m.get());
	m_hardware->addOpticalDisk(d);
	if (disk_.getBoot())
	{
		m_clip->getBootSlot(disk_.getBoot().get())
			.set(d->getDeviceType(), d->getIndex());
	}
	d->setTargetDeviceName(disk_.getTarget().getDev());
	boost::optional<QString> alias(disk_.getAlias());
	if (alias)
		d->setAlias(*alias);
	return PRL_ERR_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
// struct Graphics

PRL_RESULT Graphics::operator()(const mpl::at_c<Libvirt::Domain::Xml::VGraphics::types, 1>::type& vnc_) const
{
	if (0 != vnc_.getValue().getChoice690().which())
		return PRL_ERR_UNIMPLEMENTED;
		
	QScopedPointer<CVmRemoteDisplay> v(new CVmRemoteDisplay());
	if (vnc_.getValue().getPasswd())
	{
		v->setPassword(vnc_.getValue().getPasswd().get());
	}
	const mpl::at_c<Libvirt::Domain::Xml::VChoice690::types, 0>::type* s =
		boost::get<mpl::at_c<Libvirt::Domain::Xml::VChoice690::types, 0>::type>
			(&vnc_.getValue().getChoice690());
	if (s->getValue().getListen())
	{
		v->setHostName(s->getValue().getListen().get());
	}
	if (s->getValue().getAutoport())
	{
		v->setMode(Libvirt::Domain::Xml::EVirYesNoYes == s->getValue().getAutoport().get() ?
			PRD_AUTO : PRD_MANUAL);
	}
	if (s->getValue().getPort() && s->getValue().getPort().get() > 0)
	{
		v->setPortNumber(s->getValue().getPort().get());
	}
	else
		v->setPortNumber(0);

	if (s->getValue().getWebsocket() && s->getValue().getWebsocket().get() > 0)
	{
		v->setWebSocketPortNumber(s->getValue().getWebsocket().get());
	}
	else
		v->setWebSocketPortNumber(0);

	m_vm->getVmSettings()->setVmRemoteDisplay(v.take());
	return PRL_ERR_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
// struct Ips

QList<QString> Ips::operator()(const QList<Libvirt::Domain::Xml::Ip>& ips_)
{
	QList<QString> ips;
	foreach (const Libvirt::Domain::Xml::Ip& ip, ips_)
	{
		QString a = boost::apply_visitor(Visitor::Address::String::Conductor(), ip.getAddress());
		QString res = QString("%1/%2").arg(a), m;

		Libvirt::Domain::Xml::VIpPrefix p;
		if (QHostAddress(a).protocol() == QAbstractSocket::IPv4Protocol)
		{
			if (ip.getPrefix())
				p = ip.getPrefix().get();
			else
			{
				mpl::at_c<Libvirt::Domain::Xml::VIpPrefix::types, 0>::type d;
				d.setValue(24);
				p = d;
			}
			m = boost::apply_visitor(Visitor::Address::String::Ipv4Mask(), p);
		}
		else
		{
			if (ip.getPrefix())
				p = ip.getPrefix().get();
			else
			{
				mpl::at_c<Libvirt::Domain::Xml::VIpPrefix::types, 1>::type d;
				d.setValue(64);
				p = d;
			}
			m = boost::apply_visitor(Visitor::Address::String::Conductor(), p);
		}
		ips << res.arg(m);
	}
	return ips;
}

///////////////////////////////////////////////////////////////////////////////
// struct Network

PRL_VM_NET_ADAPTER_TYPE Network::parseAdapterType(const QString& type)
{
	if (type == "rtl8139")
		return PNT_RTL;
	else if (type == "e1000")
		return PNT_E1000;
	else if (type == "virtio")
		return PNT_VIRTIO;
	return PNT_UNDEFINED;
}

CNetPktFilter* Network::buildFilter(
		const boost::optional<Libvirt::Domain::Xml::FilterrefNodeAttributes>& filterref)
{
	QString filter = filterref ? filterref->getFilter() : QString();
	CNetPktFilter* f = new CNetPktFilter();
	f->setPreventIpSpoof(filter.contains("no-ip-spoofing"));
	f->setPreventMacSpoof(filter.contains("no-mac-spoofing"));
	f->setPreventPromisc(filter.contains("no-promisc"));
	return f;
}

PRL_RESULT Network::operator()(const mpl::at_c<Libvirt::Domain::Xml::VInterface::types, 0>::type& bridge_) const
{
	QScopedPointer<CVmGenericNetworkAdapter> a(new CVmGenericNetworkAdapter());
	a->setItemId(m_hardware->m_lstNetworkAdapters.size());
	a->setIndex(m_hardware->m_lstNetworkAdapters.size());
	a->setConnected();
	a->setEnabled(PVE::DeviceEnabled);
	a->setEmulatedType(PNA_BRIDGE);
	if (bridge_.getValue().getModel())
	{
		a->setAdapterType(parseAdapterType(bridge_.getValue().getModel().get()));
	}
	if (bridge_.getValue().getSource() && bridge_.getValue().getSource().get().getBridge())
	{
		const QString& n = bridge_.getValue().getSource().get().getBridge().get();
		if (n == QString("host-routed"))
			a->setEmulatedType(PNA_ROUTED);
		a->setVirtualNetworkID(n);
		a->setSystemName(n);
	}
	if (bridge_.getValue().getMac())
	{
		a->setMacAddress(HostUtils::parseMacAddress(bridge_.getValue().getMac().get()));
	}
	if (bridge_.getValue().getTarget())
	{
		a->setHostInterfaceName(bridge_.getValue().getTarget().get());
	}
	a->setPktFilter(buildFilter(bridge_.getValue().getFilterref()));

	a->setNetAddresses(Ips()(bridge_.getValue().getIpList()));

	if (bridge_.getValue().getBoot())
	{
		m_clip->getBootSlot(bridge_.getValue().getBoot().get())
			.set(a->getDeviceType(), a->getIndex());
	}
	if (bridge_.getValue().getLink()
			&& bridge_.getValue().getLink().get() == Libvirt::Domain::Xml::EStateDown)
		a->setConnected(PVE::DeviceDisconnected);

	m_hardware->addNetworkAdapter(a.take());
	return PRL_ERR_SUCCESS;
}

PRL_RESULT Network::operator()(const mpl::at_c<Libvirt::Domain::Xml::VInterface::types, 3>::type& network_) const
{
	QScopedPointer<CVmGenericNetworkAdapter> a(new CVmGenericNetworkAdapter());
	a->setItemId(m_hardware->m_lstNetworkAdapters.size());
	a->setIndex(m_hardware->m_lstNetworkAdapters.size());
	a->setConnected();
	a->setEnabled(PVE::DeviceEnabled);
	a->setEmulatedType(PNA_BRIDGED_NETWORK);
	if (network_.getValue().getModel())
	{
		a->setAdapterType(parseAdapterType(network_.getValue().getModel().get()));
	}
	a->setVirtualNetworkID(network_.getValue().getSource().getNetwork());
	if (network_.getValue().getMac())
	{
		a->setMacAddress(HostUtils::parseMacAddress(network_.getValue().getMac().get()));
	}
	if (network_.getValue().getTarget())
	{
		a->setHostInterfaceName(network_.getValue().getTarget().get());
	}
	a->setPktFilter(buildFilter(network_.getValue().getFilterref()));

	a->setNetAddresses(Ips()(network_.getValue().getIpList()));

	if (network_.getValue().getBoot())
	{
		m_clip->getBootSlot(network_.getValue().getBoot().get())
			.set(a->getDeviceType(), a->getIndex());
	}
	if (network_.getValue().getLink()
			&& network_.getValue().getLink().get() == Libvirt::Domain::Xml::EStateDown)
		a->setConnected(PVE::DeviceDisconnected);

	m_hardware->addNetworkAdapter(a.take());
	return PRL_ERR_SUCCESS;
}

PRL_RESULT Network::operator()(const mpl::at_c<Libvirt::Domain::Xml::VInterface::types, 4>::type& direct_) const
{
	QScopedPointer<CVmGenericNetworkAdapter> a(new CVmGenericNetworkAdapter());
	a->setItemId(m_hardware->m_lstNetworkAdapters.size());
	a->setIndex(m_hardware->m_lstNetworkAdapters.size());
	a->setConnected();
	a->setEnabled(PVE::DeviceEnabled);
	a->setEmulatedType(PNA_DIRECT_ASSIGN);
	if (direct_.getValue().getModel())
	{
		a->setAdapterType(parseAdapterType(direct_.getValue().getModel().get()));
	}
	a->setSystemName(direct_.getValue().getSource().getDev());
	if (direct_.getValue().getMac())
	{
		a->setMacAddress(HostUtils::parseMacAddress(direct_.getValue().getMac().get()));
	}
	if (direct_.getValue().getTarget())
	{
		a->setHostInterfaceName(direct_.getValue().getTarget().get());
	}
	a->setPktFilter(buildFilter(direct_.getValue().getFilterref()));

	a->setNetAddresses(Ips()(direct_.getValue().getIpList()));

	if (direct_.getValue().getBoot())
	{
		m_clip->getBootSlot(direct_.getValue().getBoot().get())
			.set(a->getDeviceType(), a->getIndex());
	}
	if (direct_.getValue().getLink()
			&& direct_.getValue().getLink().get() == Libvirt::Domain::Xml::EStateDown)
		a->setConnected(PVE::DeviceDisconnected);

	m_hardware->addNetworkAdapter(a.take());
	return PRL_ERR_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
// struct DiskForm

PRL_RESULT DiskForm::operator()(const mpl::at_c<Libvirt::Domain::Xml::VDisk::types, 0>::type& disk_) const
{
	switch (disk_.getValue())
	{
	case Libvirt::Domain::Xml::EDeviceDisk:
		return Disk(m_hardware, *m_clip)(m_disk);
	case Libvirt::Domain::Xml::EDeviceCdrom:
		return Cdrom(m_hardware, *m_clip)(m_disk);
	case Libvirt::Domain::Xml::EDeviceFloppy:
		return Floppy(m_hardware, *m_clip)(m_disk);
	}
	return PRL_ERR_UNIMPLEMENTED;
}

PRL_RESULT DiskForm::operator()(const mpl::at_c<Libvirt::Domain::Xml::VDisk::types, 1>::type& lun_) const
{
	if (Libvirt::Domain::Xml::EDevice1Lun == lun_.getValue().getDevice())
		return Disk(m_hardware, *m_clip)(m_disk);

	return PRL_ERR_UNIMPLEMENTED;
}

///////////////////////////////////////////////////////////////////////////////
// struct Device

PRL_RESULT Device::operator()(const mpl::at_c<Libvirt::Domain::Xml::VChoice941::types, 4>::type& interface_) const
{
	CVmHardware* h = m_vm->getVmHardwareList();
	if (NULL == h)
		return PRL_ERR_UNEXPECTED;

	return boost::apply_visitor(Network(*h, *m_clip), interface_.getValue());
}

PRL_RESULT Device::operator()(const mpl::at_c<Libvirt::Domain::Xml::VChoice941::types, 5>::type& input_) const
{
	CVmHardware* h = m_vm->getVmHardwareList();
	if (NULL == h)
		return PRL_ERR_UNEXPECTED;

	if (!input_.getValue().getBus())
		return PRL_ERR_SUCCESS;

	switch (input_.getValue().getBus().get())
	{
	case Libvirt::Domain::Xml::EBus1Usb:
	{
		const QList<CVmUsbDevice*>& u = h->m_lstUsbDevices;
		if (std::find_if(u.begin(), u.end(),
			boost::bind(&CVmUsbDevice::getUsbType, _1) == PUDT_OTHER)
			== u.end())
		{
			CVmUsbDevice* d = new(CVmUsbDevice);
			d->setEnabled(PVE::DeviceEnabled);
			d->setUsbType(PUDT_OTHER);
			h->addUsbDevice(d);
		}
	}
	default:
		break;
	}
	return PRL_ERR_SUCCESS;
}

PRL_RESULT Device::operator()(const mpl::at_c<Libvirt::Domain::Xml::VChoice941::types, 6>::type& sound_) const
{
	CVmHardware* h = m_vm->getVmHardwareList();
	if (NULL == h)
		return PRL_ERR_UNEXPECTED;

	CVmSoundDevice* d = new CVmSoundDevice();
	d->setIndex(h->m_lstSoundDevices.size());
	d->setItemId(h->m_lstSoundDevices.size());
	if (sound_.getValue().getAlias())
		d->setUserFriendlyName(sound_.getValue().getAlias().get());

	h->addSoundDevice(d);
	return PRL_ERR_SUCCESS;
}

PRL_RESULT Device::operator()(const mpl::at_c<Libvirt::Domain::Xml::VChoice941::types, 11>::type& parallel_) const
{
	CVmHardware* h = m_vm->getVmHardwareList();
	if (NULL == h)
		return PRL_ERR_UNEXPECTED;

	CVmParallelPort* p = new CVmParallelPort();
	p->setIndex(h->m_lstParallelPorts.size());
	p->setItemId(h->m_lstParallelPorts.size());
	if (parallel_.getValue().getAlias())
	{
		p->setUserFriendlyName(parallel_.getValue().getAlias().get());
		p->setSystemName(p->getUserFriendlyName());
	}

	h->addParallelPort(p);
	return PRL_ERR_SUCCESS;
}

PRL_RESULT Device::operator()(const mpl::at_c<Libvirt::Domain::Xml::VChoice941::types, 12>::type& serial_) const
{
	CVmHardware* h = m_vm->getVmHardwareList();
	if (NULL == h)
		return PRL_ERR_UNEXPECTED;

	if (serial_.getValue().getQemucdevSrcDef().getSourceList().isEmpty())
		return PRL_ERR_SUCCESS;

	Libvirt::Domain::Xml::Source15 s = serial_.getValue().getQemucdevSrcDef().getSourceList().front();

	QString n;
	PVE::SerialPortEmulatedType t;
	switch (serial_.getValue().getType())
	{
	case Libvirt::Domain::Xml::EQemucdevSrcTypeChoiceFile:
		t = PVE::SerialOutputFile;
		if (!s.getPath())
			return PRL_ERR_SUCCESS;
		n = s.getPath().get();
		break;
	case Libvirt::Domain::Xml::EQemucdevSrcTypeChoiceUnix:
		t = PVE::SerialSocket;
		if (!s.getPath())
			return PRL_ERR_SUCCESS;
		n = s.getPath().get();
		break;
	case Libvirt::Domain::Xml::EQemucdevSrcTypeChoiceDev:
		t = PVE::RealSerialPort;
		if (!s.getPath())
			return PRL_ERR_SUCCESS;
		n = s.getPath().get();
		break;
	case Libvirt::Domain::Xml::EQemucdevSrcTypeChoiceUdp:
		t = PVE::SerialUDP;
		if (!s.getHost() || !s.getService())
			return PRL_ERR_SUCCESS;
		n = QString("%1:%2").arg(s.getHost().get(), s.getService().get());
		break;
	case Libvirt::Domain::Xml::EQemucdevSrcTypeChoiceTcp:
		t = PVE::SerialTCP;
		if (!s.getHost() || !s.getService())
			return PRL_ERR_SUCCESS;
		n = QString("%1:%2").arg(s.getHost().get(), s.getService().get());
		break;
	default:
		return PRL_ERR_SUCCESS;
	}

	CVmSerialPort* p = new CVmSerialPort();
	p->setIndex(h->m_lstSerialPorts.size());
	p->setItemId(h->m_lstSerialPorts.size());
	p->setEnabled(true);
	p->setEmulatedType(t);
	p->setUserFriendlyName(n);
	p->setSystemName(p->getUserFriendlyName());

	if (s.getMode())
	{
		p->setSocketMode(s.getMode().get() == QString("connect") ?
			PSP_SERIAL_SOCKET_CLIENT : PSP_SERIAL_SOCKET_SERVER);
	}

	h->addSerialPort(p);
	return PRL_ERR_SUCCESS;
}

PRL_RESULT Device::operator()(const mpl::at_c<Libvirt::Domain::Xml::VChoice941::types, 0>::type& disk_) const
{
	CVmHardware* h(m_vm->getVmHardwareList());
	if (!h)
		return PRL_ERR_UNEXPECTED;
	return boost::apply_visitor(DiskForm(*h, *m_clip, disk_.getValue()), disk_.getValue().getDisk());
}

PRL_RESULT Device::operator()(const mpl::at_c<Libvirt::Domain::Xml::VChoice941::types, 9>::type& video_) const
{
	CVmHardware* h = m_vm->getVmHardwareList();
	if (NULL == h)
		return PRL_ERR_UNEXPECTED;

	CVmVideo* v = new CVmVideo();
	v->setEnable3DAcceleration(P3D_DISABLED);
	if (video_.getValue().getModel())
	{
		const Libvirt::Domain::Xml::Model1 &model = video_.getValue().getModel().get();
		if (model.getVram())
		{
			v->setMemorySize(model.getVram().get() >> 10);
		}
		if (model.getAcceleration() && model.getAcceleration().get().getAccel3d() &&
			model.getAcceleration().get().getAccel3d().get() == Libvirt::Domain::Xml::EVirYesNoYes)
		{
			v->setEnable3DAcceleration(P3D_ENABLED_HIGHEST);
		}
	}
	v->setEnabled(true);
	h->setVideo(v);
	return PRL_ERR_SUCCESS;
}

PRL_RESULT Device::operator()(const mpl::at_c<Libvirt::Domain::Xml::VChoice941::types, 1>::type& controller_) const
{
	if (controller_.getValue().getChoice590().which() != 2)
		return PRL_ERR_SUCCESS;
	if (m_vm->getVmSettings() == NULL)
		return PRL_ERR_UNEXPECTED;
	CVmUsbController* u = m_vm->getVmSettings()->getUsbController();
	if (u == NULL)
		return PRL_ERR_UNEXPECTED;
	CVmHardware* h = m_vm->getVmHardwareList();
	if (NULL == h)
		return PRL_ERR_UNEXPECTED;
	boost::apply_visitor(Visitor::Controller::Usb(*u, *h),
		controller_.getValue().getChoice590());
	return PRL_ERR_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
// struct Clock

void Clock::operator()(const mpl::at_c<Libvirt::Domain::Xml::VClock::types, 0>::type& fixed_) const
{
	if (!fixed_.getValue().getAdjustment())
		return;

	const Libvirt::Domain::Xml::VAdjustment& a = fixed_.getValue().getAdjustment().get();
	m_clock->setTimeShift(boost::get<mpl::at_c<Libvirt::Domain::Xml::VAdjustment::types, 1>::type>
		(a).getValue().toLongLong());
}

void Clock::operator()(const mpl::at_c<Libvirt::Domain::Xml::VClock::types, 2>::type& variable_) const
{
	if (!variable_.getValue().getAdjustment())
		return;

	m_clock->setTimeShift(variable_.getValue().getAdjustment().get().toLongLong());
}

///////////////////////////////////////////////////////////////////////////////
// struct Adjustment

void Adjustment::operator()(const mpl::at_c<Libvirt::Domain::Xml::VClock::types, 0>::type& fixed_) const
{
	mpl::at_c<Libvirt::Domain::Xml::VAdjustment::types, 1>::type value;
	value.setValue(QString::number(m_offset));
	Libvirt::Domain::Xml::Clock377 c = fixed_.getValue();
	c.setOffset(Libvirt::Domain::Xml::EOffsetUtc);
	c.setAdjustment(Libvirt::Domain::Xml::VAdjustment(value));
	mpl::at_c<Libvirt::Domain::Xml::VClock::types, 0>::type v;
	v.setValue(c);
	m_result->setClock(v);
}

void Adjustment::operator()(const mpl::at_c<Libvirt::Domain::Xml::VClock::types, 2>::type& variable_) const
{
	Libvirt::Domain::Xml::Clock383 c = variable_.getValue();
	c.setBasis(Libvirt::Domain::Xml::EBasisUtc);
	c.setAdjustment(QString::number(m_offset));
	mpl::at_c<Libvirt::Domain::Xml::VClock::types, 2>::type v;
	v.setValue(c);
	m_result->setClock(v);
}

namespace Controller
{

///////////////////////////////////////////////////////////////////////////////
// struct Usb

void Usb::operator()(const mpl::at_c<Libvirt::Domain::Xml::VChoice590::types, 2>::type& usb_) const
{
	boost::optional<Libvirt::Domain::Xml::EModel1> m = usb_.getValue().getModel();
	if (!m)
	{
		// libvirt uses piix3-uhci for the default USB controller model.
		m = Libvirt::Domain::Xml::EModel1Piix3Uhci;
	}
	switch (m.get())
	{
	case Libvirt::Domain::Xml::EModel1Piix3Uhci:
		m_settings->setUhcEnabled(true);
		break;
	case Libvirt::Domain::Xml::EModel1Ehci:
		m_settings->setEhcEnabled(true);
		break;
	case Libvirt::Domain::Xml::EModel1NecXhci:
		m_settings->setXhcEnabled(true);
		break;
	default:
		return;
	}
}

} // namespace Controller

namespace Fixup
{
namespace
{
///////////////////////////////////////////////////////////////////////////////
// struct Traits

template <typename T>
struct Traits
{
	static bool examine(const T* item_, uint index_, PRL_MASS_STORAGE_INTERFACE_TYPE type_)
	{
		return check(item_, index_, type_);
	}
	static bool check(const T* item_, uint index_, PRL_MASS_STORAGE_INTERFACE_TYPE type_)
	{
		return item_->getEnabled() == PVE::DeviceEnabled &&
			item_->getStackIndex() == index_ &&
			item_->getInterfaceType() == type_;
	}
};

template<>
bool Traits<CVmHardDisk>::examine(const CVmHardDisk* item_, uint index_, PRL_MASS_STORAGE_INTERFACE_TYPE type_)
{
	return item_->getConnected() == PVE::DeviceConnected && check(item_, index_, type_);
}

} // namespace

///////////////////////////////////////////////////////////////////////////////
// struct DiskForm

template <typename T>
void DiskForm::setDiskSource(const QList<T*> list_, uint index_) const
{
	Clustered<CVmHardDisk> c;
	if (!c(m_disk))
		return;

	typename QList<T*>::const_iterator item = std::find_if(list_.constBegin(), list_.constEnd(),
		boost::bind(&Traits<T>::examine, _1, index_, c.getResult()->getInterfaceType()));

	if (item == list_.constEnd())
		return;

	Transponster::Device::Clustered::Builder::Ordinary<T> b(*item);
	b.setSource();
	m_disk.setDiskSource(static_cast<const Transponster::Device::Clustered::Builder::Ordinary<T>& >(b)
		.getResult().getDiskSource());
}

PRL_RESULT DiskForm::operator()(const mpl::at_c<Libvirt::Domain::Xml::VDisk::types, 0>::type& disk_) const
{
	const Libvirt::Domain::Xml::EDevice e = disk_.getValue();
	QString dev = m_disk.getTarget().getDev();
	if (dev.isEmpty())
		return PRL_ERR_FAILURE;

	uint i = Parallels::fromBase26(dev.remove(0, 2));

	switch (e)
	{
	case Libvirt::Domain::Xml::EDeviceDisk:
		setDiskSource(m_hardware->m_lstHardDisks, i);
		break;
	case Libvirt::Domain::Xml::EDeviceCdrom:
		setDiskSource(m_hardware->m_lstOpticalDisks, i);
		break;
	case Libvirt::Domain::Xml::EDeviceFloppy:
		setDiskSource(m_hardware->m_lstFloppyDisks, i);
		break;
	}
	mpl::at_c<Libvirt::Domain::Xml::VChoice941::types, 0>::type y;
	y.setValue(m_disk);
	*m_list << Libvirt::Domain::Xml::VChoice941(y);
	return PRL_ERR_SUCCESS;
}

PRL_RESULT DiskForm::operator()(const mpl::at_c<Libvirt::Domain::Xml::VDisk::types, 1>::type& lun_) const
{
	if (Libvirt::Domain::Xml::EDevice1Lun != lun_.getValue().getDevice())
		return PRL_ERR_UNEXPECTED;

	return PRL_ERR_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
// struct Device

PRL_RESULT Device::operator()(const mpl::at_c<Libvirt::Domain::Xml::VChoice941::types, 12>::type&)
{
	//drop all serial devices
	return PRL_ERR_SUCCESS;
}

PRL_RESULT Device::operator()(const mpl::at_c<Libvirt::Domain::Xml::VChoice941::types, 0>::type& disk_)
{
	return boost::apply_visitor(DiskForm(&m_hardware, m_list, disk_.getValue()), disk_.getValue().getDisk());
}

} // namespace Fixup

namespace Numatune
{

///////////////////////////////////////////////////////////////////////////////
// struct Memory

QString Memory::operator()(const mpl::at_c<Libvirt::Domain::Xml::VMemory::types, 0>::type& mask_) const
{
	if (mask_.getValue())
		return mask_.getValue().get();

	return "";
}

} // namespace Numatune

///////////////////////////////////////////////////////////////////////////////
// struct Timer

void Timer::operator()(const mpl::at_c<Libvirt::Domain::Xml::VTimer::types, 0>::type& ) const
{
	mpl::at_c<Libvirt::Domain::Xml::VTickpolicy::types, 0>::type p;
	p.setValue(Libvirt::Domain::Xml::ETickpolicyDiscard);

	Libvirt::Domain::Xml::Timer402 tt;
	tt.setTickpolicy(Libvirt::Domain::Xml::VTickpolicy(p));
	tt.setName(Libvirt::Domain::Xml::EName1Pit);

	mpl::at_c<Libvirt::Domain::Xml::VTimer::types, 2>::type v;
	v.setValue(tt);

	Libvirt::Domain::Xml::Timer t;
	t.setTimer(Libvirt::Domain::Xml::VTimer(v));
	m_timers.append(t);
}

void Timer::operator()(const mpl::at_c<Libvirt::Domain::Xml::VTimer::types, 3>::type& ) const
{
	quint32 os = m_config.getOsVersion();

	mpl::at_c<Libvirt::Domain::Xml::VTimer::types, 3>::type v;
	if (IS_WINDOWS(os) && os >= PVS_GUEST_VER_WIN_2008)
		v.setValue(Libvirt::Domain::Xml::EName2Hypervclock);
	else
		v.setValue(Libvirt::Domain::Xml::EName2Kvmclock);

	Libvirt::Domain::Xml::Timer t;
	t.setTimer(Libvirt::Domain::Xml::VTimer(v));
	t.setPresent(Libvirt::Domain::Xml::EVirYesNoYes);
	m_timers.append(t);
}

} // namespace Visitor

namespace Vm
{
namespace Direct
{
///////////////////////////////////////////////////////////////////////////////
// struct Cpu

Cpu::Cpu(const Libvirt::Domain::Xml::Domain& vm_, CVmCpu* prototype_,
	const VtInfo& vt_): m_result(new CVmCpu(prototype_)),
	m_vcpu(vm_.getVcpu()), m_tune(vm_.getCputune()),
	m_numa(vm_.getNumatune())
{
	QemuKvm* k = vt_.getQemuKvm();
	if (NULL != k  && k->getVCpuInfo() != NULL)
		m_period = k->getVCpuInfo()->getDefaultPeriod();
}

PRL_RESULT Cpu::setMask()
{
	if (m_result.isNull())
		return PRL_ERR_UNINITIALIZED;

	if (!m_vcpu)
		return PRL_ERR_NO_DATA;

	if (m_vcpu->getCpuset())
		m_result->setCpuMask(m_vcpu->getCpuset().get());

	return PRL_ERR_SUCCESS;
}

PRL_RESULT Cpu::setNode()
{
	if (m_result.isNull())
		return PRL_ERR_UNINITIALIZED;

	if (!m_numa)
		return PRL_ERR_SUCCESS;

	boost::optional<Libvirt::Domain::Xml::Memory1> m = m_numa->getMemory();
	if (!m && !m->getMode())
		return PRL_ERR_SUCCESS;

	if (m->getMode().get() == Libvirt::Domain::Xml::EMode3Strict)
	{
		m_result->setNodeMask(boost::apply_visitor(
			Visitor::Numatune::Memory(), m->getMemory()));
	}

	return PRL_ERR_SUCCESS;
}

PRL_RESULT Cpu::setUnits()
{
	if (m_result.isNull())
		return PRL_ERR_UNINITIALIZED;

	if (m_tune && m_tune->getShares())
		m_result->setCpuUnits(m_tune->getShares().get() * 1000 / 1024);

	return PRL_ERR_SUCCESS;
}

PRL_RESULT Cpu::setLimit()
{
	if (m_result.isNull())
		return PRL_ERR_UNINITIALIZED;

	if (!m_tune || !m_tune->getQuota())
		return PRL_ERR_SUCCESS;

	qint32 p = 0, q = m_tune->getQuota().get();
	if (m_tune->getPeriod())
		p = m_tune->getPeriod().get();
	else if (m_period)
		return PRL_ERR_SUCCESS;
	else
		p = m_period.get();

	m_result->setCpuLimit(q == -1 ? 0 : q * 100 / p);
	return PRL_ERR_SUCCESS;
}

PRL_RESULT Cpu::setNumber()
{
	if (m_result.isNull())
		return PRL_ERR_UNINITIALIZED;

	if (!m_vcpu)
		return PRL_ERR_NO_DATA;

	m_result->setNumber(m_vcpu->getOwnValue());
	return PRL_ERR_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
// struct Vm

Vm::Vm(char* xml_)
{
	shape(xml_, m_input);
}

PRL_RESULT Vm::setBlank()
{
	if (m_input.isNull())
		return PRL_ERR_READ_XML_CONTENT;
 
	if (Libvirt::Domain::Xml::ETypeKvm != m_input->getType())
		return PRL_ERR_BAD_VM_CONFIG_FILE_SPECIFIED;

	m_result.reset(new CVmConfiguration());
	m_result->setVmType(PVT_VM);
	m_result->setVmHardwareList(new CVmHardware());
	return PRL_ERR_SUCCESS;
}

PRL_RESULT Vm::setIdentification()
{
	if (m_result.isNull())
		return PRL_ERR_UNINITIALIZED;

	CVmIdentification* i = new CVmIdentification();
        i->setVmName(m_input->getIds().getName());
	if (m_input->getIds().getUuid())
	{
		Visitor::Uuid<Libvirt::Domain::Xml::VUUID> v
			(boost::bind(&CVmIdentification::setVmUuid, &*i, _1));
		boost::apply_visitor(v, m_input->getIds().getUuid().get());
	}
	m_result->setVmIdentification(i);
	return PRL_ERR_SUCCESS;
}

PRL_RESULT Vm::setSettings()
{
	if (m_result.isNull())
		return PRL_ERR_UNINITIALIZED;

	CVmSettings* s = new CVmSettings();
	CVmCommonOptions* o = new CVmCommonOptions();
	s->setVmCommonOptions(o);
	if (m_input->getDescription())
		o->setVmDescription(m_input->getDescription().get());

	CVmRunTimeOptions* r(new CVmRunTimeOptions());
	s->setVmRuntimeOptions(r);
	if (m_input->getBlkiotune())
	{
		const Libvirt::Domain::Xml::Blkiotune& b(m_input->getBlkiotune().get());
		if (b.getWeight())
			r->setIoPriority(HostUtils::convertWeightToIoprio(b.getWeight().get()));
	}

	//EFI boot support
	Libvirt::Domain::Xml::VOs v = m_input->getOs();
	CVmStartupOptions *so = s->getVmStartupOptions();
	boost::apply_visitor(Visitor::Startup::Bootmenu(*so), v);
	CVmStartupBios* b = so->getBios();
	boost::apply_visitor(Visitor::Startup::Bios(*b), v);

	CVmUsbController* u(new CVmUsbController());
	u->setUhcEnabled(false);
	u->setEhcEnabled(false);
	u->setXhcEnabled(false);
	s->setUsbController(u);

	m_result->setVmSettings(s);
	return PRL_ERR_SUCCESS;
}

PRL_RESULT Vm::setDevices()
{
	if (m_result.isNull())
		return PRL_ERR_UNINITIALIZED;

	const Libvirt::Domain::Xml::Devices* d = m_input->getDevices().get_ptr();
	if (NULL != d)
	{
		Visitor::Controller::Collect::output_type x;
		foreach (const Libvirt::Domain::Xml::VChoice941& v, d->getChoice941List())
		{
			boost::apply_visitor(Visitor::Controller::Collect(x), v);
		}
		Clip c(*m_result->getVmSettings()->getVmStartupOptions(), x);
		foreach (const Libvirt::Domain::Xml::VChoice941& v, d->getChoice941List())
		{
			boost::apply_visitor(Visitor::Device(*m_result, c), v);
		}
	}
	return PRL_ERR_SUCCESS;
}

PRL_RESULT Vm::setResources(const VtInfo& vt_)
{
	if (m_result.isNull())
		return PRL_ERR_UNINITIALIZED;

	Resources r(*m_result);
	r.setMemory(m_input->getMemory());
	if (m_input->getCurrentMemory())
		r.setCurrentMemory(m_input->getCurrentMemory().get());

	if (m_input->getMaxMemory())
		r.setMaxMemory(m_input->getMaxMemory().get());
	if (m_input->getVcpu())
		r.setCpu(*m_input, vt_);

	if (m_input->getCpu())
		r.setCpu(m_input->getCpu().get());
	else
		r.setCpu(Libvirt::Domain::Xml::Cpu());

	if (m_input->getClock())
		r.setClock(m_input->getClock().get());
	if (m_input->getSysinfo())
		r.setChipset(m_input->getSysinfo().get());

	return PRL_ERR_SUCCESS;
}

} // namespace Direct
} // namespace Vm

namespace Network
{
///////////////////////////////////////////////////////////////////////////////
// struct Direct

Direct::Direct(char* xml_, bool enabled_)
{
	m_result.setEnabled(enabled_);
	shape(xml_, m_input);
}

PRL_RESULT Direct::setUuid()
{
	if (m_input.isNull())
		return PRL_ERR_READ_XML_CONTENT;
 
	if (m_input->getUuid())
	{
		Visitor::Uuid<Libvirt::Network::Xml::VUUID> v
			(boost::bind(&CVirtualNetwork::setUuid, &m_result, _1));
		boost::apply_visitor(v, m_input->getUuid().get());
	}
	return PRL_ERR_SUCCESS;
}

PRL_RESULT Direct::setName()
{
	if (m_input.isNull())
		return PRL_ERR_READ_XML_CONTENT;
 
	m_result.setNetworkID(m_input->getName());
	return PRL_ERR_SUCCESS;
}

PRL_RESULT Direct::setType()
{
	if (m_input.isNull())
		return PRL_ERR_READ_XML_CONTENT;

	if (!m_input->getForward()) 
		m_result.setNetworkType(PVN_HOST_ONLY);
	else if (m_input->getForward().get().getMode() && 
		m_input->getForward().get().getMode().get() == Libvirt::Network::Xml::EModeBridge)
		m_result.setNetworkType(PVN_BRIDGED_ETHERNET);
	else
		return PRL_ERR_UNIMPLEMENTED;
		
	return PRL_ERR_SUCCESS;
}

PRL_RESULT Direct::setBridge()
{
	if (m_input.isNull())
		return PRL_ERR_READ_XML_CONTENT;
 
	if (m_input->getBridge())
	{
                CVZVirtualNetwork* z = new CVZVirtualNetwork();
                z->setBridgeName(m_input->getBridge().get().getName().get());
		m_result.setVZVirtualNetwork(z);
	}
	else if (m_input->getMac())
		m_result.setBoundCardMac(m_input->getMac().get());

	return PRL_ERR_SUCCESS;
}

PRL_RESULT Direct::setVlan()
{
	if (m_input.isNull())
		return PRL_ERR_READ_XML_CONTENT;
 
	if (m_input->getVlan() && !m_input->getVlan().get().isEmpty())
		m_result.setVLANTag(m_input->getVlan().get().front().getId());

	return PRL_ERR_SUCCESS;
}

PRL_RESULT Direct::setHostOnly()
{
	if (m_input.isNull())
		return PRL_ERR_READ_XML_CONTENT;

	CHostOnlyNetwork* h = m_result.getHostOnlyNetwork();
	if (NULL == h)
		return PRL_ERR_UNEXPECTED;

	foreach (const Libvirt::Network::Xml::Ip& p, m_input->getIpList())
	{
		boost::optional<Libvirt::Network::Xml::VIpAddr> a = p.getAddress();
		if (a)
			boost::apply_visitor(Visitor::Address::Ip(*h), a.get());

		boost::optional<QString> f = p.getFamily();
		boost::optional<Libvirt::Network::Xml::VChoice1186> m = p.getChoice1186();
		if (f && m)
			boost::apply_visitor(Visitor::Address::Mask(*h, *f), m.get());

		boost::optional<Libvirt::Network::Xml::Dhcp> d = p.getDhcp();
		if (d)
		{
			QList<Libvirt::Network::Xml::Range> l = d->getRangeList();

			// If no DHCP range is specified then DHCP is disabled
			if (l.isEmpty())
				continue;

			if (f && QString("ipv6") == f.get())
				h->getDHCPv6Server()->setEnabled(true);
			else
				h->getDHCPServer()->setEnabled(true);

			// Virtuozzo supports only one DHCP range per network
			boost::apply_visitor(Visitor::Dhcp::Start(*h), l[0].getStart());
			boost::apply_visitor(Visitor::Dhcp::End(*h), l[0].getEnd());
		}
	}
	return PRL_ERR_SUCCESS;
}

} // namespace Network

namespace Interface
{
namespace Physical
{
///////////////////////////////////////////////////////////////////////////////
// struct Direct

Direct::Direct(char* xml_, bool enabled_)
{
	m_result.setEnabled(enabled_);
	shape(xml_, m_input);
}

PRL_RESULT Direct::operator()()
{
	if (m_input.isNull())
		return PRL_ERR_READ_XML_CONTENT;

	m_result.setDeviceName(m_input->getName());
	if (m_input->getMac())
		m_result.setMacAddress(m_input->getMac().get().toUpper().remove(QString(":")));

	boost::apply_visitor(Visitor::Addressing(m_result),
		m_input->getInterfaceAddressing());

	if (m_result.isConfigureWithDhcp())
		m_result.setConfigureWithDhcpIPv6(true);

	return PRL_ERR_SUCCESS;
}

} // namespace Physical

namespace Vlan
{
///////////////////////////////////////////////////////////////////////////////
// struct Direct

Direct::Direct(char* xml_, bool enabled_)
{
	m_result.setEnabled(enabled_);
	shape(xml_, m_input);
}

PRL_RESULT Direct::operator()()
{
	if (m_input.isNull())
		return PRL_ERR_READ_XML_CONTENT;

	Libvirt::Iface::Xml::VlanInterfaceCommon c = m_input->getVlanInterfaceCommon();
	if (c.getName())
		m_result.setDeviceName(c.getName().get());

	Libvirt::Iface::Xml::Vlan v = m_input->getVlan();
	m_result.setVLANTag(v.getTag());

	boost::apply_visitor(Visitor::Addressing(m_result),
		m_input->getInterfaceAddressing());

	if (m_result.isConfigureWithDhcp())
		m_result.setConfigureWithDhcpIPv6(true);

	return PRL_ERR_SUCCESS;
}

} // namespace Vlan

namespace Bridge
{
///////////////////////////////////////////////////////////////////////////////
// struct Direct

Direct::Direct(char* xml_, bool enabled_): m_stp(), m_enabled(enabled_), m_delay()
{
	shape(xml_, m_input);
}

PRL_RESULT Direct::setMaster()
{
	if (m_input.isNull())
		return PRL_ERR_READ_XML_CONTENT;
 
	foreach (const Libvirt::Iface::Xml::VChoice1250& a,
		m_input->getBridge().getChoice1250List())
	{
		if (boost::apply_visitor(Visitor::Bridge::Master(m_master), a))
			return PRL_ERR_SUCCESS;
	}
	return PRL_ERR_FILE_NOT_FOUND;
}

PRL_RESULT Direct::setBridge()
{
	if (m_input.isNull())
		return PRL_ERR_READ_XML_CONTENT;

	if (m_input->getBridge().getStp())
	{
		m_stp = Libvirt::Iface::Xml::EVirOnOffOn ==
			m_input->getBridge().getStp().get();
	}
	if (m_input->getBridge().getDelay())
		m_delay = m_input->getBridge().getDelay().get();

	return PRL_ERR_SUCCESS;
}

PRL_RESULT Direct::setInterface()
{
	if (m_input.isNull())
		return PRL_ERR_READ_XML_CONTENT;
 
	m_name = m_input->getName();
	return PRL_ERR_SUCCESS;
}

} // namespace Bridge
} // namespace Interface

namespace Snapshot
{
///////////////////////////////////////////////////////////////////////////////
// struct Direct

Direct::Direct(char* xml_)
{
	shape(xml_, m_input);
}

PRL_RESULT Direct::setInstructions()
{
	return PRL_ERR_SUCCESS;
}

PRL_RESULT Direct::setIdentity()
{
	if (m_input.isNull())
		return PRL_ERR_READ_XML_CONTENT;

	m_result.SetGuid(m_input->getName().get());
	m_result.SetName(m_input->getName().get());
	if (m_input->getDescription())
		m_result.SetDescription(m_input->getDescription().get());
	if (m_input->getCreationTime())
	{
		QDateTime d = QDateTime::fromTime_t(m_input->getCreationTime().get().toUInt());
		m_result.SetCreateTime(d.toString("yyyy-MM-dd hh:mm:ss"));
	}
	if (m_input->getState())
	{
		switch (m_input->getState().get())
		{
		case Libvirt::Snapshot::Xml::EStateRunning:
			m_result.SetVmState(PVE::SnapshotedVmRunning);
			break;
		case Libvirt::Snapshot::Xml::EStatePaused:
			m_result.SetVmState(PVE::SnapshotedVmPaused);
			break;
		default:
			m_result.SetVmState(PVE::SnapshotedVmPoweredOff);
			break;
		}
	}
	if (m_input->getActive())
		m_result.SetCurrent(m_input->getActive().get() == Libvirt::Snapshot::Xml::EActive1);

	return PRL_ERR_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
// struct Vm

Vm::Vm(char* xml_)
{
	QScopedPointer<Libvirt::Snapshot::Xml::Domainsnapshot> snapshot;

	shape(xml_, snapshot);

	if (snapshot.isNull())
		return;

	if (!snapshot->getChoice1726())
		return;

	if (1 == snapshot->getChoice1726()->which())
	{
		const Libvirt::Domain::Xml::Domain& d =
			boost::get<mpl::at_c<Libvirt::Snapshot::Xml::VChoice1726::types, 1>::type>
				(snapshot->getChoice1726().get()).getValue();
		m_input.reset(new Libvirt::Domain::Xml::Domain(d));
	}
}

} // namespace Snapshot

///////////////////////////////////////////////////////////////////////////////
// struct Clip

Boot::Slot Clip::getBootSlot(Libvirt::Domain::Xml::PPositiveInteger::value_type order_)
{
	CVmStartupOptions::CVmBootDevice* d = new CVmStartupOptions::CVmBootDevice();
	d->sequenceNumber = order_;
	d->inUseStatus = true;

	QList<CVmStartupOptions::CVmBootDevice*>::iterator it =
		std::lower_bound(m_bootList->begin(), m_bootList->end(), d,
			boost::bind(&CVmStartupOptions::CVmBootDevice::getBootingNumber, _1) < order_);
	m_bootList->insert(it, d);

	return Boot::Slot(*d);
}

boost::optional<PRL_CLUSTERED_DEVICE_SUBTYPE> Clip::getControllerModel(const Libvirt::Domain::Xml::Disk& disk_) const
{
	boost::optional<PRL_CLUSTERED_DEVICE_SUBTYPE> m;
	if (!disk_.getAddress() || !disk_.getTarget().getBus())
		return m;
	if (disk_.getTarget().getBus().get() != Libvirt::Domain::Xml::EBusScsi)
		return m;
	const Libvirt::Domain::Xml::Driveaddress& a = boost::get<mpl::at_c<Libvirt::Domain::Xml::VAddress::types, 1>::type>
		(disk_.getAddress().get()).getValue();
	if (!a.getController())
		return m;
	foreach(const Libvirt::Domain::Xml::Controller& c, *m_controllerList)
	{
		if (a.getController().get() != QString::number(c.getIndex()))
			continue;
		boost::apply_visitor(Visitor::Controller::Scsi(m), c.getChoice590());
		if (m)
			break;
	}
	return m;
}

namespace Capabilities
{

///////////////////////////////////////////////////////////////////////////////
// struct Direct

Direct::Direct(char* xml_)
{
	shape(xml_, m_input);
}

QList<QString> Direct::getCpuFeatures() const
{
	if (!m_input->getHost().getCpu().getCpuspec())
		return QList<QString>();
	return m_input->getHost().getCpu().getCpuspec()->getFeatureList();
}

QString Direct::getCpuModel() const
{
	if (!m_input->getHost().getCpu().getCpuspec())
		return QString();
	return m_input->getHost().getCpu().getCpuspec()->getModel();
}

} // namespace Capabilities

} // namespace Transponster
