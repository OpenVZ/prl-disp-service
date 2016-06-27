/*
 * Copyright (c) 2016 Parallels IP Holdings GmbH
 *
 * This file is part of Virtuozzo Core. Virtuozzo Core is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License as published by the Free Software Foundation;
 * either version 2 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 * Our contact details: Parallels IP Holdings GmbH, Vordergasse 59, 8200
 * Schaffhausen, Switzerland.
 */

#ifndef __CDSP_VM_STATE_MACHINE_H__
#define __CDSP_VM_STATE_MACHINE_H__

#include "CDspService.h"
#include "CDspVmStateSender.h"
#include "CVcmmdInterface.h"
#include "CDspVmNetworkHelper.h"
#include "CDspBackupDevice.h"
#include "Tasks/Task_BackgroundJob.h"
#include "Interfaces/Debug.h"
#include <Libraries/StatesUtils/StatesHelper.h>
#include <prlsdk/PrlEnums.h>
#include <prlcommon/Logging/Logging.h>
#include <typeinfo>
#include <boost/msm/back/tools.hpp>
#include <boost/signals2/signal.hpp>
#include <boost/msm/front/functor_row.hpp>
#include <boost/msm/back/state_machine.hpp>
#include <boost/msm/front/state_machine_def.hpp>
#include "CDspLibvirtExec.h"
#include "CDspVmGuest.h"

namespace Vm
{
namespace State
{
namespace msmf = boost::msm::front;

QString demangle(const char* name_);

///////////////////////////////////////////////////////////////////////////////
// struct Event

template <VIRTUAL_MACHINE_STATE N>
struct Event: boost::mpl::integral_c<VIRTUAL_MACHINE_STATE, N>
{
};

///////////////////////////////////////////////////////////////////////////////
// struct Switch

struct Switch
{
};

///////////////////////////////////////////////////////////////////////////////
// struct Running

// State flag that VM is active
struct Running
{
};

///////////////////////////////////////////////////////////////////////////////
// struct Frontend

struct Frontend: msmf::state_machine_def<Frontend>
{
	Frontend(const QString& uuid_, const SmartPtr<CDspClient>& user_,
			const QSharedPointer< ::Network::Routing>& routing_);

	template <VIRTUAL_MACHINE_STATE N>
	struct State: public msmf::state<>, boost::mpl::integral_c<VIRTUAL_MACHINE_STATE, N>
	{
		template <class Event, class Fsm>
		void on_entry(Event const&, Fsm& fsm_)
		{
			WRITE_TRACE(DBG_INFO, "VM '%s' changed state to %s", qPrintable(fsm_.m_name), PRL_VM_STATE_TO_STRING(N));
		}
	};

	typedef State<VMS_STOPPED> Stopped;
	typedef State<VMS_SUSPENDED> Suspended;
	typedef State<VMS_PAUSED> Paused;
	typedef State<VMS_MOUNTED> Mounted;
	typedef State<VMS_UNKNOWN> Unknown;

	// State
	struct Running: State<VMS_RUNNING>
	{
		typedef boost::mpl::vector< ::Vm::State::Running> flag_list;
	};

	// State
	struct Reverting: Stopped
	{
	};

	typedef Unknown initial_state;

	// Guard
	struct HasState
	{
		template<class Event, class FromState>
		bool operator()(const Event&, Frontend& fsm_, FromState&, Stopped&)
		{
			return !CStatesHelper(fsm_.getHome()).savFileExists();
		}
		template<class Event, class FromState>
		bool operator()(const Event&, Frontend& fsm_, FromState&, Suspended&)
		{
			return CStatesHelper(fsm_.getHome()).savFileExists();
		}
	};

	// Action
	struct Guarantee
	{
		template<class Event, class FromState>
		void operator()(const Event&, Frontend& fsm_, FromState&, Running&)
		{
			WRITE_TRACE(DBG_INFO, "enabling guarantees for VM '%s'", qPrintable(fsm_.m_name));
			Vcmmd::Api(fsm_.getUuid()).activate();
		}

		template<class Event, class FromState>
		void operator()(const Event&, Frontend& fsm_, FromState&, Paused&)
		{
			WRITE_TRACE(DBG_INFO, "disabling guarantees for VM '%s'", qPrintable(fsm_.m_name));
			Vcmmd::Api(fsm_.getUuid()).deactivate();
		}

		template<class Event, class FromState, class ToState>
		void operator()(const Event&, Frontend& fsm_, FromState&, ToState&)
		{
			WRITE_TRACE(DBG_INFO, "unregistering guarantees for VM '%s'", qPrintable(fsm_.m_name));
			Vcmmd::Api(fsm_.getUuid()).deinit();
		}
	};

	// Action
	struct Traffic
	{
		template<class Event, class FromState>
		void operator()(const Event&, Frontend& fsm_, FromState&, Running&)
		{
			boost::optional<CVmConfiguration> y = fsm_.getConfig();
			if (!y)
				return;

			WRITE_TRACE(DBG_INFO, "configuring network rates for VM '%s'", qPrintable(fsm_.m_name));
			Network::Traffic::Accounting x(fsm_.getUuid());
			foreach (CVmGenericNetworkAdapter *a, y->getVmHardwareList()->m_lstNetworkAdapters)
			{
				x(QSTR2UTF8(a->getHostInterfaceName()));
			}
			Task_NetworkShapingManagement::setNetworkRate(y.get());
		}
	};

	// Action
	struct Notification
	{
		template<class Event, class FromState, class ToState>
		void operator()(const Event&, Frontend& fsm_, FromState&, ToState&)
		{
			CDspLockedPointer<CDspVmStateSender> s = CDspService::instance()->getVmStateSender();
			if (s.isValid())
			{
				s->onVmStateChanged(FromState::value, ToState::value, fsm_.getUuid(),
					fsm_.m_user->getVmDirectoryUuid(), false);
			}
		}
	};

	// Action
	struct RoutesUp
	{
		template<class Event, class FromState, class ToState>
		void operator()(const Event&, Frontend& fsm_, FromState&, ToState&)
		{
			boost::optional<CVmConfiguration> y = fsm_.getConfig();
			if (!y)
				return;

			WRITE_TRACE(DBG_INFO, "configuring routes for VM '%s'", qPrintable(fsm_.m_name));
			fsm_.m_routing->up(y.get());
		}
	};

	// Action
	struct RoutesDown
	{
		template<class Event, class FromState, class ToState>
		void operator()(const Event&, Frontend& fsm_, FromState&, ToState&)
		{
			boost::optional<CVmConfiguration> y = fsm_.getConfig();
			if (!y)
				return;

			WRITE_TRACE(DBG_INFO, "disabling routes for VM '%s'", qPrintable(fsm_.m_name));
			fsm_.m_routing->down(y.get());
		}
	};

	// Action
	struct Cluster
	{
		template<class Event, class FromState, class ToState>
		void operator()(const Event&, Frontend& fsm_, FromState& from_, ToState& to_)
		{
			fsm_.getConfigEditor()(boost::bind
				(&configure<FromState, ToState>, _1, boost::ref(from_), boost::ref(to_)));
		}

		template<class FromState, class ToState>
		static void configure(CVmConfiguration& config_, FromState&, ToState&)
		{
			bool f = boost::is_same<FromState, Running>::value;
			bool t = boost::is_same<ToState, Running>::value;
			// ignore transitions where Running doesn't present on neither side
			if (!f && !t)
				return;
			CVmSettings *s = config_.getVmSettings();
			if (s)
			{
				ClusterOptions *o = s->getClusterOptions();
				if (o)
					o->setRunning(!f && t);
			}
		}
	};

	// Action
	struct Runtime
	{
		template<class Event, class FromState, class ToState>
		void operator()(const Event&, Frontend& fsm_, FromState& from_, ToState& to_)
		{
			Libvirt::Instrument::Agent::Vm::Unit v = Libvirt::Kit.vms().at(fsm_.getUuid());
			qint64 p = qMax<qint64>(fsm_.getService().getDispConfigGuard()
				.getDispWorkSpacePrefs()->getVmGuestCollectPeriod(), quint64(1));
			v.setMemoryStatsPeriod(p);

			CVmConfiguration runtime;
			if (v.getConfig(runtime, true).isFailed())
			{
				WRITE_TRACE(DBG_INFO, "failed to read runtime config from for VM '%s'", qPrintable(fsm_.m_name));
				return;
			}

			WRITE_TRACE(DBG_INFO, "updating config from runtime for VM '%s'", qPrintable(fsm_.m_name));
			boost::signals2::signal<void (CVmConfiguration& )> s;
			s.connect(boost::bind(&Vm::Config::Repairer<Vm::Config::revise_types>::type::do_,
				_1, boost::cref(runtime)));
			s.connect(boost::bind(&Cluster::configure<FromState, ToState>, _1,
				boost::ref(from_), boost::ref(to_)));
			fsm_.getConfigEditor()(s);
		}
	};

	// Action
	struct GuestTools
	{
		template<class Event, class FromState>
		void operator()(const Event&, Frontend& fsm_, FromState&, Running&)
		{
			WRITE_TRACE(DBG_INFO, "action guest tools on running for VM '%s'", qPrintable(fsm_.m_name));
			bool f = boost::is_same<FromState, Unknown>::value;

			::Vm::Guest::Actor *a = new ::Vm::Guest::Actor(fsm_.getConfigEditor());
			::Vm::Guest::Watcher *p = new ::Vm::Guest::Watcher(fsm_.getUuid());

			p->connect(p, SIGNAL(guestToolsStarted(const QString)),
				a, SLOT(setToolsVersionSlot(const QString)));
			p->connect(p, SIGNAL(destroyed()), a, SLOT(deleteLater()));
			if (!f) {
				p->connect(p, SIGNAL(guestToolsStarted(const QString)),
					a, SLOT(configureNetworkSlot(const QString)));
				p->connect(p, SIGNAL(destroyed()), a, SLOT(deleteLater()));
			}

			fsm_.m_toolsState = p->getFuture();

			p->startTimer(0);
		}
	};

	struct BackupDisable
	{
		template<class Event, class FromState, class ToState>
		void operator()(const Event&, Frontend& fsm_, FromState&, ToState&)
		{
			boost::optional<CVmConfiguration> c = fsm_.getConfig();
			if (!c)
				return;
			WRITE_TRACE(DBG_INFO, "disabling backups, attached to VM '%s'", qPrintable(fsm_.m_name));
			SmartPtr<CVmConfiguration> x(&c.get(), SmartPtrPolicy::DoNotReleasePointee);
			::Backup::Device::Service(x).disable();
		}
	};

	struct transition_table : boost::mpl::vector<
	//      +-----------+----------------------+-----------+--------+
	//        Start       Event                  Target      Action
	//      +-----------+----------------------+-----------+--------+
	//
	msmf::Row<Unknown,    Event<VMS_STOPPED>,    Stopped,
		msmf::ActionSequence_<boost::mpl::vector<Guarantee, Notification> >, HasState>,

	msmf::Row<Unknown,    Event<VMS_STOPPED>,    Suspended,
		msmf::ActionSequence_<boost::mpl::vector<Guarantee, Notification> >, HasState>,

	msmf::Row<Unknown,    Event<VMS_SUSPENDED>,  Suspended,
		msmf::ActionSequence_<boost::mpl::vector<Guarantee, Notification> > >,

	msmf::Row<Unknown,    Event<VMS_PAUSED>,     Paused,
		msmf::ActionSequence_<boost::mpl::vector<Guarantee, RoutesUp, Notification> > >,

	msmf::Row<Unknown,    Event<VMS_RUNNING>,    Running,
		msmf::ActionSequence_<boost::mpl::vector<Guarantee, Traffic, RoutesUp, Runtime, Notification, GuestTools> > >,

	//      +-----------+----------------------+-----------+--------+
	//        Start       Event                  Target      Action
	//      +-----------+----------------------+-----------+--------+
	msmf::Row<Stopped,    Event<VMS_RUNNING>,    Running,
		msmf::ActionSequence_<boost::mpl::vector<Guarantee, Traffic, RoutesUp, Runtime, Notification, GuestTools> > >,

	msmf::Row<Stopped,    Event<VMS_MOUNTED>,    Mounted,    Notification >,

	//      +-----------+----------------------+-----------+--------+
	//        Start       Event                  Target      Action
	//      +-----------+----------------------+-----------+--------+
	msmf::Row<Suspended,  Event<VMS_STOPPED>,    Stopped,    Notification >,

	msmf::Row<Suspended,  Event<VMS_PAUSED>,     Paused,
		msmf::ActionSequence_<boost::mpl::vector<Guarantee, RoutesUp, Notification> > >,

	msmf::Row<Suspended,  Event<VMS_RUNNING>,    Running,
		msmf::ActionSequence_<boost::mpl::vector<Guarantee, Traffic, RoutesUp, Runtime, Notification> > >,

	//      +-----------+----------------------+-----------+--------+
	//        Start       Event                  Target      Action
	//      +-----------+----------------------+-----------+--------+
	msmf::Row<Running,    Event<VMS_STOPPED>,    Stopped,
		msmf::ActionSequence_<boost::mpl::vector<Guarantee, RoutesDown, Cluster, BackupDisable, Notification> > >,

	msmf::Row<Running,    Event<VMS_SUSPENDED>,  Suspended,
		msmf::ActionSequence_<boost::mpl::vector<Guarantee, RoutesDown, Cluster, BackupDisable, Notification> > >,

	msmf::Row<Running,    Event<VMS_PAUSED>,     Paused,
		msmf::ActionSequence_<boost::mpl::vector<Guarantee, Cluster, Notification> > >,

	//      +-----------+----------------------+-----------+--------+
	//        Start       Event                  Target      Action
	//      +-----------+----------------------+-----------+--------+
	msmf::Row<Paused,     Event<VMS_STOPPED>,    Stopped,
		msmf::ActionSequence_<boost::mpl::vector<Guarantee, RoutesDown, BackupDisable, Notification> > >,

	msmf::Row<Paused,     Event<VMS_SUSPENDED>,  Suspended,
		msmf::ActionSequence_<boost::mpl::vector<Guarantee, RoutesDown, BackupDisable, Notification> > >,

	msmf::Row<Paused,     Event<VMS_RUNNING>,    Running,
		msmf::ActionSequence_<boost::mpl::vector<Guarantee, Traffic, Cluster, Notification> > >,

	//      +-----------+----------------------+-----------+--------+
	//        Start       Event                  Target      Action
	//      +-----------+----------------------+-----------+--------+
	msmf::Row<Mounted,    Event<VMS_STOPPED>,    Stopped,    Notification >,

	//      +-----------+----------------------+-----------+--------+
	//        Start       Event                  Target      Action
	//      +-----------+----------------------+-----------+--------+
	msmf::Row<Stopped,    Switch,                Reverting >,
	msmf::Row<Suspended,  Switch,                Reverting >,

	//      +-----------+----------------------+-----------+--------+
	//        Start       Event                  Target      Action
	//      +-----------+----------------------+-----------+--------+
	msmf::Row<Reverting,  Event<VMS_STOPPED>,    Stopped,
		msmf::ActionSequence_<boost::mpl::vector<Guarantee, Notification> >, HasState>,

	msmf::Row<Reverting,  Event<VMS_STOPPED>,    Suspended,
		msmf::ActionSequence_<boost::mpl::vector<Guarantee, Notification> >, HasState>,

	msmf::Row<Reverting,  Event<VMS_SUSPENDED>,  Suspended,
		msmf::ActionSequence_<boost::mpl::vector<Guarantee, Notification> > >,

	msmf::Row<Reverting,  Event<VMS_PAUSED>,     Paused,
		msmf::ActionSequence_<boost::mpl::vector<Guarantee, RoutesUp, Runtime, Notification> > >,

	msmf::Row<Reverting,  Event<VMS_RUNNING>,    Running,
		msmf::ActionSequence_<boost::mpl::vector<Guarantee, Traffic, RoutesUp, Runtime, Notification, GuestTools> > >
	//      +-----------+----------------------+-----------+--------+
	> {};

	template <class Event, class FSM>
	void no_transition(const Event&, FSM& fms_, int state_)
	{
		typedef typename boost::msm::back::recursive_get_transition_table<FSM>::type recursive_stt;
		typedef typename boost::msm::back::generate_state_set<recursive_stt>::type all_states;
		std::string n;
		boost::mpl::for_each<all_states,boost::msm::wrap<boost::mpl::placeholders::_1> >
			(boost::msm::back::get_state_name<recursive_stt>(n, state_));

		WRITE_TRACE(DBG_FATAL, "VM '%s': no transition from state '%s' on event 'Event<%s>'\n",
			qPrintable(fms_.m_name),
			qPrintable(demangle(n.c_str())),
			PRL_VM_STATE_TO_STRING(Event::value));
	}

	template <class FSM>
	void no_transition(const Switch&, FSM& fms_, int state_)
	{
		typedef typename boost::msm::back::recursive_get_transition_table<FSM>::type recursive_stt;
		typedef typename boost::msm::back::generate_state_set<recursive_stt>::type all_states;
		std::string n;
		boost::mpl::for_each<all_states,boost::msm::wrap<boost::mpl::placeholders::_1> >
			(boost::msm::back::get_state_name<recursive_stt>(n, state_));

		WRITE_TRACE(DBG_FATAL, "VM '%s': unable to prepare for snapshot-switch from state '%s'",
			qPrintable(fms_.m_name),
			qPrintable(demangle(n.c_str())));
	}

	const QString& getName() const
	{
		return m_name;
	}
	const QString& getUuid() const
	{
		return m_uuid;
	}
	const QString getHome() const;
	void setHome(const QString& path_)
	{
		m_home = QFileInfo(path_);
	}
	PRL_VM_TOOLS_STATE getToolsState()
	{
		if (m_toolsState.has_value())
			return m_toolsState.get();
		boost::optional<CVmConfiguration> c = getConfig();
		if (!c)
			return PTS_NOT_INSTALLED;
		QString v = c->getVmSettings()->getVmTools()->getAgentVersion();
		return v.isEmpty() ? PTS_NOT_INSTALLED : PTS_POSSIBLY_INSTALLED;
	}

	void setName(const QString& value_);
	boost::optional<CVmConfiguration> getConfig() const;
	Config::Edit::Atomic getConfigEditor() const;

protected:
	CDspClient& getUser() const
	{
		return *m_user;
	}
	CDspService& getService() const
	{
		return *m_service;
	}

private:
	QString m_uuid;
	CDspService* m_service;
	SmartPtr<CDspClient> m_user;
	QSharedPointer< ::Network::Routing> m_routing;
	QString m_name;
	boost::optional<QFileInfo> m_home;
	boost::future<PRL_VM_TOOLS_STATE> m_toolsState;
};

///////////////////////////////////////////////////////////////////////////////
// struct Machine

typedef boost::msm::back::state_machine<Frontend> Machine;

} // namespace State
} // namespace Vm

#endif // __CDSP_VM_STATE_MACHINE_H__
