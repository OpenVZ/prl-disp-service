/*
 * Copyright (c) 2015-2017, Parallels International GmbH
 * Copyright (c) 2017-2019 Virtuozzo International GmbH. All rights reserved.
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

#include <fstream>
#include "Direct.h"
#include "Reverse_p.h"
#include "Direct_p.h"
#include <prlsdk/PrlOses.h>
#include <prlcommon/HostUtils/HostUtils.h>

namespace Transponster
{
namespace Direct
{
///////////////////////////////////////////////////////////////////////////////
// struct Text

Text::Text(char* xml_)
{
	if (NULL == xml_)
		WRITE_TRACE(DBG_FATAL, "Invalid argument");
	else
	{
		setValue(QByteArray(xml_));
		free(xml_);
	}
}

Text::Text(const QString& xml_)
{
	setValue(xml_.toUtf8());
}

} // namespace Direct

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
	m_clip->getBootSlot(disk_.getBoot()).set(d->getDeviceType(), d->getIndex());
	d->setTargetDeviceName(disk_.getTarget().getDev());
	boost::optional<QString> alias(disk_.getAlias());
	if (alias)
		d->setAlias(*alias);
	return PRL_ERR_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
// struct Iotune

void Iotune::operator()(const mpl::at_c<Libvirt::Domain::Xml::VChoice4774::types, 0>::type& iopsLimit_) const
{
	m_disk->setIopsLimit(iopsLimit_.getValue());
}

void Iotune::operator()(const mpl::at_c<Libvirt::Domain::Xml::VChoice4771::types, 0>::type& ioLimit_) const
{
	m_disk->setIoLimit(new CVmIoLimit(PRL_IOLIMIT_BS, ioLimit_.getValue()));
}

///////////////////////////////////////////////////////////////////////////////
// struct BackingChain

void BackingChain::operator()(const mpl::at_c<list_type, 4>::type& image_) const
{
	if (!image_.getValue() || !image_.getValue().get().getFile())
		return;

	const QString& f = image_.getValue().get().getFile().get();
	if (f.isEmpty())
		return;

	CVmHddPartition* p = new CVmHddPartition();
	p->setSystemName(f);
	m_disk->m_lstPartition.prepend(p);
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
	m_clip->getBootSlot(disk_.getBoot()).set(d->getDeviceType(), d->getIndex());
	boost::optional<Libvirt::Domain::Xml::Iotune> t = disk_.getIotune();
	if (t)
	{
		Iotune v(d);
		boost::apply_visitor(v, (*t).getChoice4771());
		boost::apply_visitor(v, (*t).getChoice4774());
	}
	d->setTargetDeviceName(disk_.getTarget().getDev());
	if (disk_.getSerial())
		d->setSerialNumber(disk_.getSerial().get());
	boost::optional<QString> alias(disk_.getAlias());
	if (alias)
		d->setAlias(*alias);
	d->setPassthrough(1 == disk_.getDisk().which());
	const Libvirt::Domain::Xml::VDiskBackingChain* c = &disk_.getDiskBackingChain();
	while (0 == c->which())
	{
		typedef mpl::at_c<Libvirt::Domain::Xml::VDiskBackingChain::types, 0>::type
			chain_type;
		const chain_type* w = boost::get<const chain_type>(c);
		boost::apply_visitor(BackingChain(*d), w->getValue().getDiskSource());
		c = &(w->getValue().getDiskBackingChain()->value);
	}
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
	m_clip->getBootSlot(disk_.getBoot()).set(d->getDeviceType(), d->getIndex());
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
	if (0 != vnc_.getValue().getChoice727().which())
		return PRL_ERR_UNIMPLEMENTED;

	QScopedPointer<CVmRemoteDisplay> v(new CVmRemoteDisplay());
	if (vnc_.getValue().getPasswd())
	{
		v->setPassword(vnc_.getValue().getPasswd().get());
	}
	const mpl::at_c<Libvirt::Domain::Xml::VChoice727::types, 0>::type* s =
		boost::get<mpl::at_c<Libvirt::Domain::Xml::VChoice727::types, 0>::type>
			(&vnc_.getValue().getChoice727());
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

namespace Network
{
///////////////////////////////////////////////////////////////////////////////
// struct Builder

Builder::Builder(int index_)
{
	m_result.setItemId(index_);
	m_result.setIndex(index_);
	m_result.setConnected();
	m_result.setEnabled(PVE::DeviceEnabled);
}

void Builder::setModel(const boost::optional<QString>& value_)
{
	if (!value_)
		return;

	if (value_.get() == "rtl8139")
		m_result.setAdapterType(PNT_RTL);
	else if (value_.get() == "e1000")
		m_result.setAdapterType(PNT_E1000);
	else if (value_.get() == "virtio")
		m_result.setAdapterType(PNT_VIRTIO);
	else if (value_.get() == "hv-net")
		m_result.setAdapterType(PNT_HYPERV);
	else
		m_result.setAdapterType(PNT_UNDEFINED);
}

void Builder::setMac(const boost::optional<QString>& value_)
{
	if (value_)
		m_result.setMacAddress(HostUtils::parseMacAddress(value_.get()));
}

void Builder::setTarget(const boost::optional<QString>& value_)
{
	if (value_)
		m_result.setHostInterfaceName(value_.get());
}

void Builder::setFilter(const boost::optional<Libvirt::Domain::Xml::FilterrefNodeAttributes>& value_)
{
	QString filter = value_ ? value_->getFilter() : QString();
	CNetPktFilter* f = new CNetPktFilter();
	f->setPreventIpSpoof(filter.contains("no-ip-spoofing"));
	f->setPreventMacSpoof(filter.contains("no-mac-spoofing"));
	f->setPreventPromisc(filter.contains("no-promisc"));
	m_result.setPktFilter(f);
}

void Builder::setIps(const QList<Libvirt::Domain::Xml::Ip>& value_)
{
	QList<QString> ips;
	foreach (const Libvirt::Domain::Xml::Ip& ip, value_)
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
	m_result.setNetAddresses(ips);
}

void Builder::setConnected(const boost::optional<Libvirt::Domain::Xml::EState >& value_)
{
	if (value_ && value_.get() == Libvirt::Domain::Xml::EStateDown)
		m_result.setConnected(PVE::DeviceDisconnected);
}

///////////////////////////////////////////////////////////////////////////////
// struct Unit

template<class T>
CVmGenericNetworkAdapter& Unit::prepare(const T& variant_) const
{
	Builder b(m_hardware->m_lstNetworkAdapters.size());
	b.setModel(variant_.getModel());
	b.setMac(variant_.getMac());
	b.setTarget(variant_.getTarget());
	b.setFilter(variant_.getFilterref());
	b.setIps(variant_.getIpList());
	b.setConnected(variant_.getLink());

	QScopedPointer<CVmGenericNetworkAdapter> a(new CVmGenericNetworkAdapter(b.getResult()));
	m_clip->getBootSlot(variant_.getBoot())
		.set(a->getDeviceType(), a->getIndex());

	m_hardware->addNetworkAdapter(a.take());

	return *m_hardware->m_lstNetworkAdapters.back();
}

PRL_RESULT Unit::operator()(const mpl::at_c<Libvirt::Domain::Xml::VInterface::types, 0>::type& bridge_) const
{
	CVmGenericNetworkAdapter& a = prepare(bridge_.getValue());
	a.setEmulatedType(PNA_BRIDGE);
	if (bridge_.getValue().getSource())
	{
		const QString& n = bridge_.getValue().getSource().get().getInterfaceBridgeAttributes().getBridge();
		if (n == QString("host-routed"))
			a.setEmulatedType(PNA_ROUTED);
		a.setVirtualNetworkID(n);
		a.setSystemName(n);
	}

	return PRL_ERR_SUCCESS;
}

PRL_RESULT Unit::operator()(const mpl::at_c<Libvirt::Domain::Xml::VInterface::types, 1>::type& ethernet_) const
{
	if (!ethernet_.getValue().getScript() ||
		ethernet_.getValue().getScript()->compare("/bin/true") != 0)
		return PRL_ERR_INVALID_ARG;

	CVmGenericNetworkAdapter& a = prepare(ethernet_.getValue());
	a.setEmulatedType(PNA_ROUTED);

	return PRL_ERR_SUCCESS;
}

PRL_RESULT Unit::operator()(const mpl::at_c<Libvirt::Domain::Xml::VInterface::types, 3>::type& network_) const
{
	CVmGenericNetworkAdapter& a = prepare(network_.getValue());
	a.setEmulatedType(PNA_BRIDGED_NETWORK);
	a.setVirtualNetworkID(network_.getValue().getSource().getInterfaceNetworkAttributes().getNetwork());

	return PRL_ERR_SUCCESS;
}

PRL_RESULT Unit::operator()(const mpl::at_c<Libvirt::Domain::Xml::VInterface::types, 4>::type& direct_) const
{
	CVmGenericNetworkAdapter& a = prepare(direct_.getValue());
	a.setEmulatedType(PNA_DIRECT_ASSIGN);
	a.setSystemName(direct_.getValue().getSource().getDev());

	return PRL_ERR_SUCCESS;
}

} // namespace Network

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

PRL_RESULT Device::operator()(const mpl::at_c<Libvirt::Domain::Xml::VChoice985::types, 4>::type& interface_) const
{
	CVmHardware* h = m_vm->getVmHardwareList();
	if (NULL == h)
		return PRL_ERR_UNEXPECTED;

	return boost::apply_visitor(Network::Unit(*h, *m_clip), interface_.getValue());
}

PRL_RESULT Device::operator()(const mpl::at_c<Libvirt::Domain::Xml::VChoice985::types, 5>::type& input_) const
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

PRL_RESULT Device::operator()(const mpl::at_c<Libvirt::Domain::Xml::VChoice985::types, 6>::type& sound_) const
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

PRL_RESULT Device::operator()(const mpl::at_c<Libvirt::Domain::Xml::VChoice985::types, 7>::type& pci_) const
{
	CVmHardware* h = m_vm->getVmHardwareList();
	if (NULL == h)
		return PRL_ERR_UNEXPECTED;

	QString a;
	if (pci_.getValue().getAlias())
		a = pci_.getValue().getAlias().get();

	boost::apply_visitor(Visitor::Nodedev(a, *h), pci_.getValue().getChoice917());
	return PRL_ERR_SUCCESS;
}

PRL_RESULT Device::operator()(const mpl::at_c<Libvirt::Domain::Xml::VChoice985::types, 11>::type& parallel_) const
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

PRL_RESULT Device::operator()(const mpl::at_c<Libvirt::Domain::Xml::VChoice985::types, 12>::type& serial_) const
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

PRL_RESULT Device::operator()(const mpl::at_c<Libvirt::Domain::Xml::VChoice985::types, 0>::type& disk_) const
{
	CVmHardware* h(m_vm->getVmHardwareList());
	if (!h)
		return PRL_ERR_UNEXPECTED;
	return boost::apply_visitor(DiskForm(*h, *m_clip, disk_.getValue()), disk_.getValue().getDisk());
}

PRL_RESULT Device::operator()(const mpl::at_c<Libvirt::Domain::Xml::VChoice985::types, 9>::type& video_) const
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

PRL_RESULT Device::operator()(const mpl::at_c<Libvirt::Domain::Xml::VChoice985::types, 1>::type& controller_) const
{
	if (controller_.getValue().getChoice625().which() != 2)
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
		controller_.getValue().getChoice625());
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
	Libvirt::Domain::Xml::Clock395 c = fixed_.getValue();
	c.setOffset(Libvirt::Domain::Xml::EOffsetUtc);
	c.setAdjustment(Libvirt::Domain::Xml::VAdjustment(value));
	mpl::at_c<Libvirt::Domain::Xml::VClock::types, 0>::type v;
	v.setValue(c);
	m_result->setClock(v);
}

void Adjustment::operator()(const mpl::at_c<Libvirt::Domain::Xml::VClock::types, 2>::type& variable_) const
{
	Libvirt::Domain::Xml::Clock401 c = variable_.getValue();
	c.setBasis(Libvirt::Domain::Xml::EBasisUtc);
	c.setAdjustment(QString::number(m_offset));
	mpl::at_c<Libvirt::Domain::Xml::VClock::types, 2>::type v;
	v.setValue(c);
	m_result->setClock(v);
}

///////////////////////////////////////////////////////////////////////////////
// struct Nodedev

Nodedev::result_type Nodedev::operator()
	(const mpl::at_c<Libvirt::Domain::Xml::VHostdevsubsys::types, 0>::type& alternative_) const
{
	const Libvirt::Domain::Xml::Pciaddress& a = alternative_.getValue().getSource().getAddress();
	QString x = QString("%1:%2:%3")
		.arg(QString::number(a.getBus().toUInt(NULL, 16), 16))
		.arg(QString::number(a.getSlot().toUInt(NULL, 16), 16))
		.arg(QString::number(a.getFunction().toUInt(NULL, 16), 16));
	CVmGenericPciDevice* d = new CVmGenericPciDevice();
	d->setIndex(m_sink->m_lstGenericPciDevices.size());
	d->setEnabled(PVE::DeviceEnabled);
	d->setAlias(m_alias);
	d->setSystemName(x);

	m_sink->addGenericPciDevice(d);
}

namespace Controller
{
///////////////////////////////////////////////////////////////////////////////
// struct Usb

void Usb::operator()(const mpl::at_c<Libvirt::Domain::Xml::VChoice625::types, 2>::type& usb_) const
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
	case Libvirt::Domain::Xml::EModel1QemuXhci:
		m_settings->setXhcEnabled(true);
		break;
	default:
		return;
	}
}

///////////////////////////////////////////////////////////////////////////////
// struct Pci

bool Pci::operator()(const mpl::at_c<Libvirt::Nodedev::Xml::VCapability::types, 1>::type& pci_) const
{
	const mpl::at_c<Libvirt::Nodedev::Xml::VCapability::types, 1>::type::value_type& v = 
		pci_.getValue();

	CHwGenericPciDevice d;
	d.setDeviceName(v.getProduct().getOwnValue());
	d.setDeviceId(QString("%1:%2:%3:%4.%5")
		.arg(QString().setNum(v.getBus(), 16))
		.arg(QString().setNum(v.getSlot(), 16))
		.arg(QString().setNum(v.getFunction(), 16))
		.arg(QString(v.getVendor().getId()).replace("0x", ""))
		.arg(QString(v.getProduct().getId()).replace("0x", "")));
	d.setType(PGD_PCI_OTHER);
	d.setPrimary(true);
	d.setSupported(true);
	*m_bin = d;

	return true;
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
	mpl::at_c<Libvirt::Domain::Xml::VChoice985::types, 0>::type y;
	y.setValue(m_disk);
	*m_list << Libvirt::Domain::Xml::VChoice985(y);
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

PRL_RESULT Device::operator()(const mpl::at_c<Libvirt::Domain::Xml::VChoice985::types, 12>::type&)
{
	//drop all serial devices
	return PRL_ERR_SUCCESS;
}

PRL_RESULT Device::operator()(const mpl::at_c<Libvirt::Domain::Xml::VChoice985::types, 0>::type& disk_)
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

	Libvirt::Domain::Xml::Timer420 tt;
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

namespace Export
{
///////////////////////////////////////////////////////////////////////////////
// struct Image

Image::result_type Image::operator()
	(const boost::mpl::at_c<xml::VChoice985::types, 0>::type& disk_) const
{
	if (!boost::apply_visitor(*this, disk_.getValue().getDisk()))
		return false;

	CVmHardDisk d;
	Source::Unit<CVmHardDisk> v;
	v.setDevice(d);
	if (!boost::apply_visitor(v, disk_.getValue().getDiskSource()))
		return false;

	(*m_map)[disk_.getValue().getTarget().getDev()] = d.getSystemName();
	return true;
}

Image::result_type Image::operator()
	(const boost::mpl::at_c<xml::VDisk::types, 0>::type& disk_) const
{
	return xml::EDeviceDisk == disk_.getValue();
}

} // namespace Export
} // namespace Visitor

namespace Chipset
{
typedef QPair<quint32, quint32> model_type;

///////////////////////////////////////////////////////////////////////////////
// struct Generic

template<class T>
Prl::Expected<model_type, PRL_RESULT>
	Generic<T>::deserialize(const QString& text_) const
{
	if (!text_.startsWith(T::s_PREFIX))
		return PRL_ERR_SYMBOL_NOT_FOUND;

	quint32 x = 0;
	std::istringstream s(text_.mid(T::s_PREFIX.length()).toStdString());
	s >> x;

	return model_type(T::TYPE, x);
}

template<class T>
QString Generic<T>::serialize(model_type::second_type version_) const
{
	return QString(T::s_PREFIX).append(QString::number(version_)).append(".0");
}

///////////////////////////////////////////////////////////////////////////////
// struct i440fx

const QString i440fx::s_PREFIX("pc-i440fx-vz7.");

Prl::Expected<model_type, PRL_RESULT>
	i440fx::deserialize(const QString& text_) const
{
	Prl::Expected<model_type, PRL_RESULT> x =
		Generic<i440fx>::deserialize(text_);
	if (x.isFailed())
		return x;

	return model_type(x.value().first, std::max(x.value().second, 6u) - 3);
}

QString i440fx::serialize(model_type::second_type version_) const
{
	return Generic<i440fx>::serialize(qMax(version_, 3u) + 3u);
}

///////////////////////////////////////////////////////////////////////////////
// struct Q35

const QString Q35::s_PREFIX("pc-q35-vz7.");

Prl::Expected<model_type, PRL_RESULT>
	Q35::deserialize(const QString& text_) const
{
	Prl::Expected<model_type, PRL_RESULT> x =
		Generic<Q35>::deserialize(text_);
	if (x.isFailed())
		return x;
	if (7 > x.value().second)
		return PRL_ERR_INVALID_ARG;

	return model_type(x.value().first, x.value().second - 6);

}

QString Q35::serialize(model_type::second_type version_) const
{
	return Generic<Q35>::serialize(version_ + 6);
}

///////////////////////////////////////////////////////////////////////////////
// struct Marshal

model_type Marshal::deserialize_(const QString& text_) const
{
	Prl::Expected<model_type, PRL_RESULT> x;
	x = i440fx().deserialize(text_);
	if (x.isSucceed())
		return x.value();
	x = Q35().deserialize(text_);
	if (x.isSucceed())
		return x.value();

	::Chipset y;
	return model_type(y.getType(), y.getVersion());
}

::Chipset Marshal::deserialize(const QString& text_) const
{
	model_type m = deserialize_(text_);
	::Chipset output;
	output.setType(m.first);
	output.setVersion(m.second);
	output.setAlias(text_);

	return output;
}

QString Marshal::serialize(const model_type& object_) const
{
	switch (object_.first)
	{
	case i440fx::TYPE: 
		return i440fx().serialize(object_.second);
	case Q35::TYPE: 
		return Q35().serialize(object_.second);
	default:
		return QString();
	}
}

QString Marshal::serialize(const ::Chipset& object_) const
{
	QString a = object_.getAlias();
	if (!a.isEmpty())
		return a;

	return serialize(model_type(object_.getType(), object_.getVersion()));
}

} // namespace Chipset

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
	m_result->setSockets(1);
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
		foreach (const Libvirt::Domain::Xml::VChoice985& v, d->getChoice985List())
		{
			boost::apply_visitor(Visitor::Controller::Collect(x), v);
		}
		Clip c(*m_result->getVmSettings()->getVmStartupOptions(), x);
		foreach (const Libvirt::Domain::Xml::VChoice985& v, d->getChoice985List())
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

	r.setChipset(m_input->getOs());

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
		boost::optional<Libvirt::Network::Xml::VChoice2401> m = p.getChoice2401();
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

Direct::Direct(bool enabled_)
{
	value_type v = getValue();
	v.setEnabled(enabled_);
	setValue(v);
}

PRL_RESULT Direct::operator()(const object_type& object_)
{
	value_type v = getValue();
	v.setDeviceName(object_.getName());
	if (object_.getMac())
		v.setMacAddress(object_.getMac().get().toUpper().remove(QString(":")));

	boost::apply_visitor(Visitor::Addressing(v), object_.getInterfaceAddressing());
	if (v.isConfigureWithDhcp())
		v.setConfigureWithDhcpIPv6(true);

	setValue(v);

	return PRL_ERR_SUCCESS;
}

} // namespace Physical

namespace Vlan
{
///////////////////////////////////////////////////////////////////////////////
// struct Direct

Direct::Direct(bool enabled_)
{
	value_type v = getValue();
	v.setEnabled(enabled_);
	setValue(v);
}

PRL_RESULT Direct::operator()(const object_type& object_)
{
	value_type v = getValue();
	Libvirt::Iface::Xml::VlanInterfaceCommon c = object_.getVlanInterfaceCommon();
	if (c.getName())
		v.setDeviceName(c.getName().get());

	Libvirt::Iface::Xml::Vlan x = object_.getVlan();
	v.setVLANTag(x.getTag());

	boost::apply_visitor(Visitor::Addressing(v), object_.getInterfaceAddressing());
	if (v.isConfigureWithDhcp())
		v.setConfigureWithDhcpIPv6(true);

	setValue(v);

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
 
	foreach (const Libvirt::Iface::Xml::VChoice2462& a,
		m_input->getBridge().getChoice2462List())
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

	if (!snapshot->getChoice4011())
		return;

	if (1 == snapshot->getChoice4011()->which())
	{
		const Libvirt::Domain::Xml::Domain& d =
			boost::get<mpl::at_c<Libvirt::Snapshot::Xml::VChoice4011::types, 1>::type>
				(snapshot->getChoice4011().get()).getValue();
		m_input.reset(new Libvirt::Domain::Xml::Domain(d));
	}
}

namespace Export
{
///////////////////////////////////////////////////////////////////////////////
// struct View

PRL_RESULT View::operator()(const object_type& object_)
{
	if (!(object_.getDevices() && object_.getDevices()->getXBlockexport()))
		return PRL_ERR_SUCCESS;

	namespace xml = Libvirt::Domain::Xml;
	const xml::Domainblockexport_& x = object_.getDevices()->getXBlockexport().get();

	QUrl t;
	t.setScheme(QLatin1String("nbd"));
	t.setHost(x.getAddress().getHost());
	if (x.getAddress().getPort())
		t.setPort(x.getAddress().getPort().get());

	Visitor::Export::map_type m;
	foreach (const xml::VChoice985& d, object_.getDevices()->getChoice985List())
	{
		boost::apply_visitor(Visitor::Export::Image(m), d);
	}
	// NB. both if's above cannot change the m_result. simply we don't perform
	// redundant construction & copy actions in the begining.
	value_type v;
	foreach (const xml::Disk1& d, x.getDiskList())
	{
		if (!d.getSnapshot())
			continue;

		value_type::value_type y;
		y.get<0>() = d.getSnapshot().get();
		y.get<1>() = boost::apply_visitor(Visitor::Export::Name(m), d.getName());

		QUrl u(t);
		if (d.getExportname())
			u.setPath(d.getExportname().get());

		y.get<2>() = u;
		v << y;
	}
	setValue(v);

	return PRL_ERR_SUCCESS;
}

} // namespace Export
} // namespace Snapshot

///////////////////////////////////////////////////////////////////////////////
// struct Clip

Boot::Slot Clip::getBootSlot(const order_type& order_)
{
	CVmStartupOptions::CVmBootDevice* d = new CVmStartupOptions::CVmBootDevice();
	if (order_)
	{
		typedef CVmStartupOptions::CVmBootDevice device_type;

		d->sequenceNumber = order_.get();
		d->inUseStatus = true;
		QList<device_type* >::iterator it =
			std::lower_bound(m_bootList->begin(), m_bootList->end(), d,
				boost::bind(&device_type::getBootingNumber, _1) < order_.get() &&
				boost::bind(&device_type::isInUse, _1));
		m_bootList->insert(it, d);
	}
	else
		m_bootList->push_back(d);

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
		boost::apply_visitor(Visitor::Controller::Scsi(m), c.getChoice625());
		if (m)
			break;
	}
	return m;
}

namespace Host
{
///////////////////////////////////////////////////////////////////////////////
// struct Pci

PRL_GENERIC_PCI_DEVICE_CLASS Pci::guess(const boost::optional<QString>& path_)
{
	if (!path_)
		return PGD_PCI_OTHER;

	std::ifstream f(qPrintable(QDir(path_.get()).absoluteFilePath("class")));
	if (!f)
		return PGD_PCI_OTHER;

	union
	{
		quint32 raw;
		struct
		{
			quint8 progIf;
			quint8 subclazz;
			quint8 clazz;
			quint8 zero;
		} parsed;
	} x;
	f >> std::hex >> x.raw;
	f.close();
	switch (x.parsed.clazz)
	{
	case 0x2:
		return PGD_PCI_NETWORK;
	case 0x3:
		return PGD_PCI_DISPLAY;
	case 0x4:
		switch (x.parsed.subclazz)
		{
		case 0x1:
		case 0x3:
			return PGD_PCI_SOUND;
		}
	default:
		return PGD_PCI_OTHER;
	}
}

PRL_RESULT Pci::operator()(const object_type& object_)
{
	foreach (const Libvirt::Nodedev::Xml::VCapability& c, object_.getCapabilityList())
	{
		Visitor::Controller::Pci::bin_type b;
		if (!boost::apply_visitor(Visitor::Controller::Pci(b), c))
			continue;

		PRL_GENERIC_PCI_DEVICE_CLASS z = guess(object_.getPath());
		if (PGD_PCI_DISPLAY != z)
		{
			b.setType(z);
			setValue(b);
		}
		break;
	}

	return PRL_ERR_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
// struct Capabilities

PRL_RESULT Capabilities::operator()(const object_type& object_)
{
	setValue(object_);
	return PRL_ERR_SUCCESS;
}

CpuFeatures* Capabilities::getCpuFeatures() const
{
	if (!isValid())
		return NULL;

	QList<QString> d, r;
	foreach (const Libvirt::Capability::Xml::Feature& f,
		getValue().getCpu()->getMode2().getAnonymous4935()->getFeatureList())
	{
		switch (f.getPolicy())
		{
		case Libvirt::Capability::Xml::EPolicyDisable:
			d << f.getName();
		case Libvirt::Capability::Xml::EPolicyForbid:
			break;
		default:
			r << f.getName();
		}
	}
	CpuFeatures* output = new CpuFeatures();
	output->setDisabled(d);
	output->setRequired(r);

	return output;
}

QString Capabilities::getCpuModel() const
{
	return getValue().getCpu()->getMode2().getAnonymous4935()
		->getModel().getOwnValue();
}

bool Capabilities::isValid() const
{
	return getValue().getCpu() &&
		getValue().getCpu()->getMode2().getSupported() == Libvirt::Capability::Xml::EVirYesNoYes &&
		getValue().getCpu()->getMode2().getAnonymous4935();
}

} // namespace Host
} // namespace Transponster
