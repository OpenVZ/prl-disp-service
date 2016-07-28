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

#ifndef __CDSP_VM_STATE_MACHINE_P_H__
#define __CDSP_VM_STATE_MACHINE_P_H__

#include <typeinfo>
#include <boost/msm/back/tools.hpp>
#include <boost/msm/front/functor_row.hpp>
#include <boost/msm/back/state_machine.hpp>
#include <boost/msm/front/state_machine_def.hpp>

namespace Vm
{
namespace State
{
namespace Details
{
namespace msmf = boost::msm::front;

QString demangle(const char* name_);

///////////////////////////////////////////////////////////////////////////////
// struct Trace

template<class T>
struct Trace: boost::msm::front::state<>
{
	template <typename Event, typename FSM>
	void on_entry(const Event&, FSM&)
	{
		WRITE_TRACE(DBG_FATAL, "enter %s state\nevent %s\n",
			qPrintable(demangle()), qPrintable(Trace<Event>::demangle()));
	}
	template <typename Event, typename FSM>
	void on_exit(const Event&, FSM&)
	{
		WRITE_TRACE(DBG_FATAL, "leave %s state\nevent %s\n",
			qPrintable(demangle()), qPrintable(Trace<Event>::demangle()));
	}
	static QString demangle()
	{
		return Details::demangle(typeid(T).name());
	}
};

///////////////////////////////////////////////////////////////////////////////
// struct Frontend

template<class T>
struct Frontend: msmf::state_machine_def<T>
{
	template <typename Event, typename FSM>
	void on_entry(const Event&, FSM&)
	{
		WRITE_TRACE(DBG_FATAL, "enter %s state\nevent %s\n",
			qPrintable(Trace<T>::demangle()), qPrintable(Trace<Event>::demangle()));
	}
	template <typename Event, typename FSM>
	void on_exit(const Event&, FSM&)
	{
		WRITE_TRACE(DBG_FATAL, "leave %s state\nevent %s\n",
			qPrintable(Trace<T>::demangle()), qPrintable(Trace<Event>::demangle()));
	}
	template <class FSM, class Event>
	void no_transition(const Event& , FSM&, int state_)
	{
		typedef typename boost::msm::back::recursive_get_transition_table<FSM>::type recursive_stt;
		typedef typename boost::msm::back::generate_state_set<recursive_stt>::type all_states;
		std::string n;
		boost::mpl::for_each<all_states,boost::msm::wrap<boost::mpl::_1> >
			(boost::msm::back::get_state_name<recursive_stt>(n, state_));

		WRITE_TRACE(DBG_FATAL, "no transition from state %s on event %s\n",
			qPrintable(demangle(n.c_str())),
			qPrintable(Trace<Event>::demangle()));
	}
	template <class FSM,class Event>
	void exception_caught (Event const&,FSM&, std::exception& exception_)
	{
		WRITE_TRACE(DBG_FATAL, "exception %s, fsm %s, event %s\n", exception_.what(),
			qPrintable(Trace<FSM>::demangle()),
			qPrintable(Trace<Event>::demangle()));
	}
};

} // namespace Details
} // namespace State
} // namespace Vm

#endif // __CDSP_VM_STATE_MACHINE_P_H__
