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
#include <Libraries/HostUtils/HostUtils.h>

namespace Transponster
{
namespace Visitor
{
namespace Source
{
///////////////////////////////////////////////////////////////////////////////
// struct Unit

bool Unit<CVmHardDisk>::operator()(const mpl::at_c<Libvirt::Domain::Xml::VDiskSource::types, 4>::type& source_) const
{
	const Libvirt::Domain::Xml::Source4* v = source_.getValue().get_ptr();
	if (NULL == v)
		return false;

	getDevice().setSystemName(v->getVolume());
	getDevice().setEmulatedType(PVE::BootCampHardDisk);
	return true;
}

} // namespace Source

///////////////////////////////////////////////////////////////////////////////
// struct Cpu

void Cpu::operator()(const mpl::at_c<Libvirt::Domain::Xml::VCpu::types, 2>::type& cpu_) const
{
	foreach (const Libvirt::Domain::Xml::Feature& f,
		cpu_.getValue().getFeatureList())
	{
		if (f.getName().compare(QString("vmx"), Qt::CaseInsensitive))
		{
			if (f.getPolicy() == Libvirt::Domain::Xml::EPolicyDisable)
				m_hardware->getCpu()->setVirtualizedHV(false);
			break;
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
// struct Floppy

PRL_RESULT Floppy::operator()(const Libvirt::Domain::Xml::Disk& disk_)
{
	if (!Clustered<CVmFloppyDisk>::operator()(disk_))
		return PRL_ERR_UNIMPLEMENTED;

	CVmFloppyDisk* d = getResult();
	if (NULL == d)
		return PRL_ERR_UNEXPECTED;

	d->setItemId(m_hardware->m_lstFloppyDisks.size());
	d->setIndex(m_hardware->m_lstFloppyDisks.size());
	m_hardware->addFloppyDisk(d);
	return PRL_ERR_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
// struct Iotune

void Iotune::operator()(const mpl::at_c<Libvirt::Domain::Xml::VChoice1042::types, 0>::type& iopsLimit_) const
{
	m_disk->setIopsLimit(iopsLimit_.getValue());
}

void Iotune::operator()(const mpl::at_c<Libvirt::Domain::Xml::VChoice1038::types, 0>::type& ioLimit_) const
{
	m_disk->setIoLimit(new CVmIoLimit(PRL_IOLIMIT_BS, ioLimit_.getValue()));
}

///////////////////////////////////////////////////////////////////////////////
// struct Disk

PRL_RESULT Disk::operator()(const Libvirt::Domain::Xml::Disk& disk_)
{
	if (!Clustered<CVmHardDisk>::operator()(disk_))
		return PRL_ERR_UNIMPLEMENTED;

	getDevice().setEncrypted(disk_.getEncryption());
	CVmHardDisk* d = getResult();
	if (NULL == d)
		return PRL_ERR_UNEXPECTED;

	d->setItemId(m_hardware->m_lstHardDisks.size());
	d->setIndex(m_hardware->m_lstHardDisks.size());
	d->setStackIndex(m_clip->getBusSlot(d->getInterfaceType()));
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
		boost::apply_visitor(v, (*t).getChoice1042());
		boost::apply_visitor(v, (*t).getChoice1038());
	}
	d->setTargetDeviceName(disk_.getTarget().getDev());
	return PRL_ERR_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
// struct Cdrom

PRL_RESULT Cdrom::operator()(const Libvirt::Domain::Xml::Disk& disk_)
{
	Clustered<CVmOpticalDisk>::operator()(disk_);
	getDevice().setPassthrough(disk_.getTransient() ?
		PVE::PassthroughEnabled : PVE::PassthroughDisabled);

	CVmOpticalDisk* d = getResult();
	if (NULL == d)
		return PRL_ERR_UNEXPECTED;

	d->setItemId(m_hardware->m_lstOpticalDisks.size());
	d->setIndex(m_hardware->m_lstOpticalDisks.size());
	d->setStackIndex(m_clip->getBusSlot(d->getInterfaceType()));
	boost::optional<PRL_CLUSTERED_DEVICE_SUBTYPE> m = m_clip->getControllerModel(disk_);
	if (m)
		d->setSubType(m.get());
	m_hardware->addOpticalDisk(d);
	if (disk_.getBoot())
	{
		m_clip->getBootSlot(disk_.getBoot().get())
			.set(d->getDeviceType(), d->getIndex());
	}
	return PRL_ERR_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
// struct Graphics

PRL_RESULT Graphics::operator()(const mpl::at_c<Libvirt::Domain::Xml::VGraphics::types, 1>::type& vnc_) const
{
	if (0 != vnc_.getValue().getChoice683().which())
		return PRL_ERR_UNIMPLEMENTED;
		
	QScopedPointer<CVmRemoteDisplay> v(new CVmRemoteDisplay());
	if (vnc_.getValue().getPasswd())
	{
		v->setPassword(vnc_.getValue().getPasswd().get());
	}
	const mpl::at_c<Libvirt::Domain::Xml::VChoice683::types, 0>::type* s =
		boost::get<mpl::at_c<Libvirt::Domain::Xml::VChoice683::types, 0>::type>
			(&vnc_.getValue().getChoice683());
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

	m_vm->getVmSettings()->setVmRemoteDisplay(v.take());
	return PRL_ERR_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
// struct Network

PRL_VM_NET_ADAPTER_TYPE Network::parseAdapterType(const QString& type)
{
	if (type == "ne2k_pci")
		return PNT_RTL;
	else if (type == "e1000")
		return PNT_E1000;
	else if (type == "virtio")
		return PNT_VIRTIO;
	return PNT_UNDEFINED;
}

PRL_RESULT Network::operator()(const mpl::at_c<Libvirt::Domain::Xml::VInterface::types, 0>::type& bridge_) const
{
	QScopedPointer<CVmGenericNetworkAdapter> a(new CVmGenericNetworkAdapter());
	a->setItemId(m_hardware->m_lstNetworkAdapters.size());
	a->setIndex(m_hardware->m_lstNetworkAdapters.size());
	a->setConnected();
	a->setEnabled(PVE::DeviceEnabled);
	a->setEmulatedType(PNA_BRIDGED_ETHERNET);
	if (bridge_.getValue().getModel())
	{
		a->setAdapterType(parseAdapterType(bridge_.getValue().getModel().get()));
	}
	if (bridge_.getValue().getSource())
	{
		a->setSystemName(bridge_.getValue().getSource().get());
		a->setBoundAdapterName(bridge_.getValue().getSource().get());
	}
	if (bridge_.getValue().getMac())
	{
		a->setMacAddress(HostUtils::parseMacAddress(bridge_.getValue().getMac().get()));
	}
	if (bridge_.getValue().getTarget())
	{
		a->setHostInterfaceName(bridge_.getValue().getTarget().get());
	}
	if (bridge_.getValue().getFilterref())
	{
		CNetPktFilter* f = new CNetPktFilter();
		a->setPktFilter(f);
	}
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
	a->setEmulatedType(PNA_BRIDGED_ETHERNET);
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
	m_hardware->addNetworkAdapter(a.take());
	return PRL_ERR_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
// struct Device

PRL_RESULT Device::operator()(const mpl::at_c<Libvirt::Domain::Xml::VChoice928::types, 4>::type& interface_) const
{
	CVmHardware* h = m_vm->getVmHardwareList();
	if (NULL == h)
		return PRL_ERR_UNEXPECTED;

	return boost::apply_visitor(Network(*h), interface_.getValue());
}

PRL_RESULT Device::operator()(const mpl::at_c<Libvirt::Domain::Xml::VChoice928::types, 6>::type& sound_) const
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

PRL_RESULT Device::operator()(const mpl::at_c<Libvirt::Domain::Xml::VChoice928::types, 11>::type& parallel_) const
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

PRL_RESULT Device::operator()(const mpl::at_c<Libvirt::Domain::Xml::VChoice928::types, 12>::type& serial_) const
{
	CVmHardware* h = m_vm->getVmHardwareList();
	if (NULL == h)
		return PRL_ERR_UNEXPECTED;

	if (Libvirt::Domain::Xml::EQemucdevSrcTypeChoiceFile != serial_.getValue().getType())
		return PRL_ERR_SUCCESS;

	if (serial_.getValue().getQemucdevSrcDef().getSourceList().isEmpty())
		return PRL_ERR_SUCCESS;

	if (!serial_.getValue().getQemucdevSrcDef().getSourceList().front().getPath())
		return PRL_ERR_SUCCESS;

	CVmSerialPort* p = new CVmSerialPort();
	p->setIndex(h->m_lstSerialPorts.size());
	p->setItemId(h->m_lstSerialPorts.size());
	p->setEnabled(true);
	p->setEmulatedType(PVE::SerialOutputFile);
	p->setUserFriendlyName(serial_.getValue().getQemucdevSrcDef()
		.getSourceList().front().getPath().get());
	p->setSystemName(p->getUserFriendlyName());
	h->addSerialPort(p);
	return PRL_ERR_SUCCESS;
}

PRL_RESULT Device::operator()(const mpl::at_c<Libvirt::Domain::Xml::VChoice928::types, 0>::type& disk_) const
{
	if (0 != disk_.getValue().getDisk().which())
		return PRL_ERR_SUCCESS;

	const Libvirt::Domain::Xml::EDevice* e = boost::get<mpl::at_c<Libvirt::Domain::Xml::VDisk::types, 0>::type>
		(disk_.getValue().getDisk()).getValue().get_ptr();
	if (NULL != e)
	{
		CVmHardware* h = m_vm->getVmHardwareList();
		if (NULL == h)
			return PRL_ERR_UNEXPECTED;

		switch (*e)
		{
		case Libvirt::Domain::Xml::EDeviceDisk:
			return Disk(*h, *m_clip)(disk_.getValue());
		case Libvirt::Domain::Xml::EDeviceCdrom:
			return Cdrom(*h, *m_clip)(disk_.getValue());
		case Libvirt::Domain::Xml::EDeviceFloppy:
			return Floppy(*h)(disk_.getValue());
		}
	}
	return PRL_ERR_UNIMPLEMENTED;
}

PRL_RESULT Device::operator()(const mpl::at_c<Libvirt::Domain::Xml::VChoice928::types, 9>::type& video_) const
{
	CVmHardware* h = m_vm->getVmHardwareList();
	if (NULL == h)
		return PRL_ERR_UNEXPECTED;

	CVmVideo* v = new CVmVideo();
	if (video_.getValue().getModel())
	{
		if (video_.getValue().getModel().get().getVram())
		{
			v->setMemorySize(video_.getValue().getModel()
				.get().getVram().get() >> 10);
		}
		if (video_.getValue().getModel().get().getAcceleration())
		{
			const Libvirt::Domain::Xml::EVirYesNo* a3 = video_.getValue().getModel()
				.get().getAcceleration().get().getAccel3d().get_ptr();
			v->setEnable3DAcceleration(NULL != a3 && *a3 == Libvirt::Domain::Xml::EVirYesNoYes ?
				P3D_DISABLED : P3D_ENABLED_HIGHEST);
		}
	}
	v->setEnabled(true);
	h->setVideo(v);
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

} // namespace Visitor

namespace
{
template<class T>
void shape(char* xml_, QScopedPointer<T>& dst_)
{
	if (NULL == xml_)
		return;

	QByteArray b(xml_);
	free(xml_);
	QDomDocument x;
	if (!x.setContent(b, true))
		return;

	QScopedPointer<T> g(new T());
	if (g->load(x.documentElement()))
		dst_.reset(g.take());
}

} // namespace

namespace Vm
{
///////////////////////////////////////////////////////////////////////////////
// struct Direct

Direct::Direct(char* xml_)
{
	shape(xml_, m_input);
}

PRL_RESULT Direct::setBlank()
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

PRL_RESULT Direct::setIdentification()
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

PRL_RESULT Direct::setSettings()
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
	const Libvirt::Domain::Xml::Loader* l
		= boost::get<mpl::at_c<Libvirt::Domain::Xml::VOs::types, 1>::type>
			(m_input->getOs()).getValue().getLoader().get_ptr();
	if (l && l->getType() &&
		l->getType().get() == Libvirt::Domain::Xml::EType2Pflash)
	{
		s->getVmStartupOptions()
			->getBios()
			->setEfiEnabled(true);
	}

	m_result->setVmSettings(s);
	return PRL_ERR_SUCCESS;
}

PRL_RESULT Direct::setDevices()
{
	if (m_result.isNull())
		return PRL_ERR_UNINITIALIZED;

	const Libvirt::Domain::Xml::Devices* d = m_input->getDevices().get_ptr();
	if (NULL != d)
	{
		Visitor::Controller::Collect::output_type x;
		foreach (const Libvirt::Domain::Xml::VChoice928& v, d->getChoice928List())
		{
			boost::apply_visitor(Visitor::Controller::Collect(x), v);
		}
		Clip c(*m_result->getVmSettings()->getVmStartupOptions(), x);
		foreach (const Libvirt::Domain::Xml::VChoice928& v, d->getChoice928List())
		{
			boost::apply_visitor(Visitor::Device(*m_result, c), v);
		}
	}
	return PRL_ERR_SUCCESS;
}

PRL_RESULT Direct::setResources()
{
	if (m_result.isNull())
		return PRL_ERR_UNINITIALIZED;

	Resources r(*m_result);
	r.setMemory(m_input->getMemory());
	if (m_input->getCpu())
		r.setVCpu(m_input->getCpu().get());
	if (m_input->getVcpu())
		r.setCpu(m_input->getVcpu().get());
	if (m_input->getClock())
		r.setClock(m_input->getClock().get());
	if (m_input->getSysinfo())
		r.setChipset(m_input->getSysinfo().get());

	return PRL_ERR_SUCCESS;
}

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
 
	typedef mpl::at_c<Libvirt::Iface::Xml::VChoice1228::types, 0>::type eth_type;
	foreach (const Libvirt::Iface::Xml::VChoice1228& a,
		m_input->getBridge().getChoice1228List())
	{
		if (0 != a.which())
			continue;

		m_master.setDeviceName(boost::get<eth_type>(a).getValue().getName());
		if (boost::get<eth_type>(a).getValue().getMac())
			m_master.setMacAddress(boost::get<eth_type>(a).getValue().getMac().get().toUpper().remove(QString(":")));

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
		m_result.SetCreateTime(m_input->getCreationTime().get());
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
		boost::apply_visitor(Visitor::Controller::Scsi(m), c.getChoice585());
		if (m)
			break;
	}
	return m;
}

} // namespace Transponster
