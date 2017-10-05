///////////////////////////////////////////////////////////////////////////////
///
/// @file Task_EditVm_p.h
///
/// Edit VM configuration
///
/// @author shrike
///
/// Copyright (c) 2005-2017, Parallels International GmbH
///
/// This file is part of Virtuozzo Core. Virtuozzo Core is free
/// software; you can redistribute it and/or modify it under the terms
/// of the GNU General Public License as published by the Free Software
/// Foundation; either version 2 of the License, or (at your option) any
/// later version.
/// 
/// This program is distributed in the hope that it will be useful,
/// but WITHOUT ANY WARRANTY; without even the implied warranty of
/// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
/// GNU General Public License for more details.
/// 
/// You should have received a copy of the GNU General Public License
/// along with this program; if not, write to the Free Software
/// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
/// 02110-1301, USA.
///
/// Our contact details: Parallels International GmbH, Vordergasse 59, 8200
/// Schaffhausen, Switzerland.
///
/////////////////////////////////////////////////////////////////////////////////

#ifndef __TASK_EDITVM_P_H__
#define __TASK_EDITVM_P_H__

#include "CDspLibvirt.h"
#include <boost/bind.hpp>
#include "CDspTaskHelper.h"
#include "CDspVmManager_p.h"
#include "CVcmmdInterface.h"
#include <boost/mpl/vector.hpp>
#include <boost/mpl/for_each.hpp>
#include <boost/function.hpp>
#include "CDspVmGuestPersonality.h"
#include <prlxmlmodel/VmConfig/CVmConfiguration.h>
#include <prlxmlmodel/ParallelsObjects/CXmlModelHelper.h>

class Task_EditVm;
namespace Edit
{
namespace Vm
{
///////////////////////////////////////////////////////////////////////////////
// struct Action

struct Action
{
	virtual ~Action();

	virtual bool execute(CDspTaskFailure& feedback_) = 0;
	void setNext(Action* value_)
	{
		m_next.reset(value_);
	}
	Action& getTail();

private:
	QScopedPointer<Action> m_next;
};

///////////////////////////////////////////////////////////////////////////////
// struct Flop

struct Flop: Action
{
	explicit Flop(PRL_RESULT code_): m_code(code_)
	{
	}

	bool execute(CDspTaskFailure& feedback_)
	{
		return PRL_SUCCEEDED(feedback_(m_code));
	}

private:
	PRL_RESULT m_code;
};

///////////////////////////////////////////////////////////////////////////////
// struct Domain

template<class T, class U>
struct Domain: Action
{
	Domain(const U& agent_, const T& decorated_): m_agent(agent_), m_decorated(decorated_)
	{
	}

	bool execute(CDspTaskFailure& feedback_)
	{
		Libvirt::Result e = m_decorated(m_agent);
		if (e.isFailed())
		{
			feedback_(e.error().convertToEvent());
			return false;
		}
		return Action::execute(feedback_);
	}

private:
	U m_agent;
	T m_decorated;
};

///////////////////////////////////////////////////////////////////////////////
// struct Reconnect

struct Reconnect: Action
{
	Reconnect(const CVmGenericNetworkAdapter& adapter_)
	: m_adapter(adapter_.getHostInterfaceName()), m_network(adapter_.getVirtualNetworkID())
	{
	}

	bool execute(CDspTaskFailure& feedback_);

private:
	QString m_adapter;
	QString m_network;
};

///////////////////////////////////////////////////////////////////////////////
// struct VcmmdAction

struct VcmmdAction: Action
{
	VcmmdAction(const Vcmmd::Api& api_, const Vcmmd::Config::Vm::Model& patch_):
			m_api(api_), m_patch(patch_)
	{
	}

	bool execute(CDspTaskFailure& feedback_);

private:
	Vcmmd::Api m_api;
	Vcmmd::Config::Vm::Model m_patch;
};

///////////////////////////////////////////////////////////////////////////////
// struct Request

struct Request
{
	typedef SmartPtr<CVmConfiguration> config_type;

	Request(Task_EditVm& task_, const config_type& start_,
		const config_type& final_);

	const CVmIdent& getObject() const
	{
		return m_object;
	}
	const CVmConfiguration& getStart() const
	{
		return *m_start;
	}
	const CVmConfiguration& getFinal() const
	{
		return *m_final;
	}

	Task_EditVm& getTask() const
	{
		return *m_task;
	}

private:
	CVmIdent m_object;
	Task_EditVm* m_task;
	const config_type m_start;
	const config_type m_final;
};

///////////////////////////////////////////////////////////////////////////////
// struct Forge

struct Forge
{
	explicit Forge(const QString& vm_): m_vm(vm_)
	{
	}
	explicit Forge(const Request& request_): m_vm(request_.getObject().first)
	{
	}

	template<class T>
	Action* craft(const T& decorated_) const
	{
		return new Domain<T, Libvirt::Instrument::Agent::Vm::Unit>
			(getUnit(), decorated_);
	}
	template<class T>
	Action* craftState(const T& decorated_) const
	{
		return new Domain<T, Libvirt::Instrument::Agent::Vm::Limb::State>
			(getUnit().getState(), decorated_);
	}
	template<class T>
	Action* craftGuest(const T& decorated_) const
	{
		return new Domain<T, Libvirt::Instrument::Agent::Vm::Guest>
			(getUnit().getGuest(), decorated_);
	}
	template<class T>
	Action* craftRuntime(const T& decorated_) const
	{
		return new Domain<T, Libvirt::Instrument::Agent::Vm::Editor>
			(getUnit().getRuntime(), decorated_);
	}

private:
	Libvirt::Instrument::Agent::Vm::Unit getUnit() const
	{
		return Libvirt::Kit.vms().at(m_vm);
	}

	const QString m_vm;
};

///////////////////////////////////////////////////////////////////////////////
// struct Visitor

struct Visitor
{
	explicit Visitor(const Request& input_): m_input(&input_)
	{
	}

	template<class T>
	void operator()(T factory_)
	{
		Action* a = factory_(*m_input);
		if (NULL == a)
			return;

		a->getTail().setNext(m_result.take());
		m_result.reset(a);
	}

	Action* getResult()
	{
		return m_result.take();
	}

private:
	const Request* m_input;
	QScopedPointer<Action> m_result;
};

///////////////////////////////////////////////////////////////////////////////
// struct Factory

template<class T>
struct Factory
{
	Action* prime(const Request& input_) const
	{
		Visitor v(input_);
		boost::mpl::for_each<T>(boost::ref(v));
		return v.getResult();
	}
};

///////////////////////////////////////////////////////////////////////////////
// struct Gear

template<class T, class U>
struct Gear: protected Factory<U>
{
	explicit Gear(Task_EditVm& task_): m_task(&task_)
	{
	}

	void operator()(const Request::config_type& old_, const Request::config_type& new_)
	{
		Request r(*m_task, old_, new_);
		QScopedPointer<Edit::Vm::Action> a(static_cast<T& >(*this).prime(r));
		if (!a.isNull())
		{
			CDspTaskFailure f(*m_task);
			a->execute(f);
		}
	}

private:
	Task_EditVm* m_task;
};

namespace Config
{
///////////////////////////////////////////////////////////////////////////////
// struct Magic

struct Magic
{
	typedef Libvirt::Result result_type;
	typedef Command::Vm::Fork::Config::Detector detector_type;
	typedef Libvirt::Instrument::Agent::Vm::Unit agent_type;

	static detector_type* craftDetector(agent_type agent_);
};

///////////////////////////////////////////////////////////////////////////////
// struct Define

struct Define: Magic
{
	typedef QPair<agent_type, const CVmConfiguration* > load_type;

	static detector_type* craftDetector(const load_type& load_)
	{
		return Magic::craftDetector(load_.first);
	}
};

///////////////////////////////////////////////////////////////////////////////
// struct Novel

struct Novel: Define
{
	result_type operator()(load_type load_);
};

///////////////////////////////////////////////////////////////////////////////
// struct Update

struct Update: Define
{
	result_type operator()(load_type load_);
};

///////////////////////////////////////////////////////////////////////////////
// struct Event

struct Event: Magic
{
	typedef agent_type load_type;

	result_type operator()(load_type load_);
};

///////////////////////////////////////////////////////////////////////////////
// struct Facade

template<class T, class Enabled = void>
struct Facade
{
	typedef Magic::result_type result_type;

	explicit Facade(const CVmConfiguration& config_): m_config(&config_)
	{
	}

	result_type operator()(Magic::agent_type agent_)
	{
		return Command::Vm::Gear<Command::Tag::Config<T> >
			::run(qMakePair(agent_, m_config));
	}

private:
	const CVmConfiguration* m_config;
};

template<class T>
struct Facade<T, typename boost::enable_if<boost::is_same<typename T::load_type, Magic::agent_type> >::type>
{
	typedef Magic::result_type result_type;

	result_type operator()(Magic::agent_type agent_)
	{
		return Command::Vm::Gear<Command::Tag::Config<T> >
			::run(agent_);
	}
};

} // namespace Config

///////////////////////////////////////////////////////////////////////////////
// struct Transfer

struct Transfer: Action
{
	Transfer(const CVmIdent& object_, const QString& target_):
		m_target(target_), m_object(object_)
	{
	}

	bool execute(CDspTaskFailure& feedback_);

private:
	QString m_target;
	CVmIdent m_object;
};

///////////////////////////////////////////////////////////////////////////////
// struct Patch

struct Patch: Action
{
	explicit Patch(const Request& input_);

	bool execute(CDspTaskFailure& feedback_);

private:
	QString m_home;
	QString m_name;
	QString m_editor;
	CVmIdent m_ident;
};

///////////////////////////////////////////////////////////////////////////////
// struct Apply

struct Apply
{
	Action* operator()(const Request& input_) const;
};

namespace Create
{
///////////////////////////////////////////////////////////////////////////////
// struct Nvram

struct Nvram
{
	Vm::Action* operator()(const Request& input_) const;
};

///////////////////////////////////////////////////////////////////////////////
// struct Action

template <class T>
struct Action: Vm::Action
{
	Action(const T& data_, const CVmConfiguration& config_)
	: m_data(data_), m_path(QFileInfo(config_.getVmIdentification()->getHomePath()).absolutePath())
	{
	}

	bool execute(CDspTaskFailure& feedback_);
	PRL_RESULT execute();

private:
	T m_data;
	QString m_path;
};

} // namespace Create

typedef boost::mpl::vector<Create::Nvram, Apply> probeList_type;
typedef Gear<Factory<probeList_type>, probeList_type> driver_type;

namespace Runtime
{
///////////////////////////////////////////////////////////////////////////////
// struct NotApplied

struct NotApplied: Action
{
	explicit NotApplied(const Request& input_): m_vmUuid(input_.getObject().first),
			m_session(input_.getTask().getClient())
	{
	}

	bool execute(CDspTaskFailure& feedback_);

private:
	QString m_vmUuid;
	SmartPtr<CDspClient> m_session;
};

///////////////////////////////////////////////////////////////////////////////
// struct ChangeableMedia

template<class T>
struct ChangeableMedia
{
	Action* operator()(const Request& input_) const;

private:
	static QList<T* > getList(const CVmHardware* hardware_);
};

///////////////////////////////////////////////////////////////////////////////
// struct Adapter

struct Adapter
{
	Action* operator()(const Request& input_) const;
};

///////////////////////////////////////////////////////////////////////////////
// struct Vcmmd

struct Vcmmd
{
	Action* operator()(const Request& input_) const;
};

///////////////////////////////////////////////////////////////////////////////
// struct Factory

struct Disk
{
	Action* operator()(const Request& input_) const;

private:
	bool isDiskIoUntunable(const CVmHardDisk* disk_) const;
};

///////////////////////////////////////////////////////////////////////////////
// struct Blkiotune

struct Blkiotune
{
	Vm::Action* operator()(const Request& input_) const;
};

namespace Network
{

///////////////////////////////////////////////////////////////////////////////
// struct Factory

struct Factory
{
	Vm::Action* operator()(const Request& input_) const;
};

} // namespace Network

namespace Cpu
{

namespace Limit
{

typedef boost::function<Libvirt::Result (Libvirt::Instrument::Agent::Vm::Editor, quint64, quint64)> setter_type;

///////////////////////////////////////////////////////////////////////////////
// struct Percents

struct Percents
{
	explicit Percents(quint32 value_, setter_type setter_)
		: m_value(value_), m_setter(setter_)
	{
	}

	Libvirt::Result operator()(const Libvirt::Instrument::Agent::Vm::Editor& agent_) const;

private:
	quint32 m_value;
	setter_type m_setter;
};

///////////////////////////////////////////////////////////////////////////////
// struct Mhz

struct Mhz
{
	explicit Mhz(quint32 value_, setter_type setter_)
		: m_value(value_), m_setter(setter_)
	{
	}

	Libvirt::Result operator()(const Libvirt::Instrument::Agent::Vm::Editor& agent_) const;

private:
	quint32 m_value;
	setter_type m_setter;
};

///////////////////////////////////////////////////////////////////////////////
// struct Any

struct Any
{
	Any(const CVmCpu& cpu, quint32 type_): m_cpu(cpu), m_type(type_)
	{
	}

	Libvirt::Result operator()(const Libvirt::Instrument::Agent::Vm::Editor& agent_) const;

private:
	CVmCpu m_cpu;
	quint32 m_type;
};

} // namespace Limit

///////////////////////////////////////////////////////////////////////////////
// struct Factory

struct Factory
{
	Action* operator()(const Request& input_) const;

private:
	Action* craftLimit(const Request& input_) const;
};

} // namespace Cpu

namespace Hotplug
{
namespace Traits
{
///////////////////////////////////////////////////////////////////////////////
// struct Generic

template<class T>
struct Generic
{
	typedef T device_type;
	typedef QList<device_type* > haystack_type;

	static bool canPlug(const device_type& novel_);
	static bool canPlug(const device_type& original_, const device_type& update_);
	static device_type* match(device_type* needle_, const haystack_type& haystack_);
};

///////////////////////////////////////////////////////////////////////////////
// struct Specific forward declaration

template<class T>
struct Specific;

///////////////////////////////////////////////////////////////////////////////
// struct Specific<CVmHardDisk>

template<>
struct Specific<CVmHardDisk>: Generic<CVmHardDisk>
{
        static CVmHardDisk* match(CVmHardDisk* needle_, const haystack_type& haystack_);
        static haystack_type point(const CVmHardware* hardware_)
	{
		return hardware_->m_lstHardDisks;
	}
};

///////////////////////////////////////////////////////////////////////////////
// struct Specific<CVmSerialPort>

template<>
struct Specific<CVmSerialPort>: Generic<CVmSerialPort>
{
	static haystack_type point(const CVmHardware* hardware_)
	{
		return hardware_->m_lstSerialPorts;
	}
};

///////////////////////////////////////////////////////////////////////////////
// struct Specific<CVmGenericNetworkAdapter>

template<>
struct Specific<CVmGenericNetworkAdapter>
{
	typedef Generic<CVmGenericNetworkAdapter> generic_type;
	typedef generic_type::device_type device_type;
	typedef generic_type::haystack_type haystack_type;

	static haystack_type point(const CVmHardware* hardware_)
	{
		return hardware_->m_lstNetworkAdapters;
	}
	static device_type* match(device_type* needle_, const haystack_type& haystack_)
	{
		return generic_type::match(needle_, haystack_);
	}
	static bool canPlug(const device_type& novel_);
	static bool canPlug(const device_type& original_, const device_type& update_);
	static bool canUpdate(const device_type& original_, const device_type& update_);
};

} // namespace Traits

///////////////////////////////////////////////////////////////////////////////
// struct Generic

template<class T>
struct Generic
{
	typedef Traits::Specific<T> traits_type;
	typedef typename traits_type::device_type device_type;
	typedef typename traits_type::haystack_type haystack_type;

	Action* operator()(const Request& input_) const;
};

///////////////////////////////////////////////////////////////////////////////
// struct Factory

template<class T>
struct Factory: Generic<T>
{
};

template<>
struct Factory<CVmGenericNetworkAdapter>: Generic<CVmGenericNetworkAdapter>
{
	typedef Generic<CVmGenericNetworkAdapter> generic_type;

	Action* operator()(const Request& input_) const;
};

} // namespace Hotplug

///////////////////////////////////////////////////////////////////////////////
// struct Driver

typedef boost::mpl::vector<ChangeableMedia<CVmOpticalDisk>, ChangeableMedia<CVmFloppyDisk>,
		Adapter, Hotplug::Factory<CVmSerialPort>, Hotplug::Factory<CVmHardDisk>,
		Hotplug::Factory<CVmGenericNetworkAdapter>, Disk, Blkiotune,
		Network::Factory, Vcmmd, Cpu::Factory> probeList_type;

struct Driver: Gear<Driver, probeList_type>
{
	explicit Driver(Task_EditVm& task_): Gear<Driver, probeList_type>(task_)
	{
	}

	Action* prime(const Request& input_) const;
};

} // namespace Runtime
} // namespace Vm
} // namespace Edit

#endif	// __TASK_EDITVM_P_H__
