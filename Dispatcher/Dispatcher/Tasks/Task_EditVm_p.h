///////////////////////////////////////////////////////////////////////////////
///
/// @file Task_EditVm_p.h
///
/// Edit VM configuration
///
/// @author shrike
///
/// Copyright (c) 2005-2015 Parallels IP Holdings GmbH
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
/// Our contact details: Parallels IP Holdings GmbH, Vordergasse 59, 8200
/// Schaffhausen, Switzerland.
///
/////////////////////////////////////////////////////////////////////////////////

#ifndef __TASK_EDITVM_P_H__
#define __TASK_EDITVM_P_H__

#include "CDspLibvirt.h"
#include <boost/bind.hpp>
#include "CDspTaskHelper.h"
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
	VcmmdAction(const Vcmmd::Api& vcmmd_, quint64 limit_, quint64 guarantee_):
		m_vcmmd(vcmmd_), m_limit(limit_), m_guarantee(guarantee_)
	{
	}

	bool execute(CDspTaskFailure& feedback_);

private:
	Vcmmd::Api m_vcmmd;
	quint64 m_limit;
	quint64 m_guarantee;
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

private:
	static Libvirt::Result define(Libvirt::Instrument::Agent::Vm::Unit agent_,
					const CVmConfiguration& config_);
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
// struct Memory

struct Memory
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

///////////////////////////////////////////////////////////////////////////////
// struct Hotplug

template<class T>
struct Hotplug
{
	Action* operator()(const Request& input_) const;

private:
	static QList<T* > getList(const CVmHardware* hardware_);
	static QList<T* > getDifference(const QList<T* >& first_,
					const QList<T* >& second_);
};

///////////////////////////////////////////////////////////////////////////////
// struct Driver

typedef boost::mpl::vector<ChangeableMedia<CVmOpticalDisk>, ChangeableMedia<CVmFloppyDisk>,
		Adapter, Hotplug<CVmSerialPort>, Hotplug<CVmHardDisk>, Disk,
		Blkiotune, Network::Factory, Memory, Cpu::Factory> probeList_type;

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
