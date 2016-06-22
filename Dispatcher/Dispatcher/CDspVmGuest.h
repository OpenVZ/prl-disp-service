///////////////////////////////////////////////////////////////////////////////
///
/// @file CDspVmGuest.h
///
/// Class for VM activity running inside guest agent
///
/// @author yur
///
/// Copyright (c) 2015-2016 Parallels IP Holdings GmbH
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

#ifndef __CVMGUEST_H__
#define __CVMGUEST_H__

#include <QObject>
#include <QString>

#include <prlsdk/PrlEnums.h>
#include <prlxmlmodel/VmConfig/CVmConfiguration.h>

#define BOOST_THREAD_PROVIDES_FUTURE
#include <boost/thread.hpp>
#include <boost/thread/future.hpp>

#include "CDspVmConfigManager.h"

#ifdef _LIBVIRT_
#include "CDspLibvirt.h"
#include "CDspLibvirtExec.h"
#endif // _LIBVIRT_

namespace Vm
{

class Guest: public QObject
{
	Q_OBJECT

public:
	Guest(const QString& uuid_, const Config::Edit::Atomic& editor_, bool f_) :
		m_uuid(uuid_), m_editor(editor_), m_fromKnownState(f_)
	{
	}

	boost::future<std::pair<PRL_VM_TOOLS_STATE, QString> > getFuture()
	{
		return m_state.get_future();
	}

	static void setToolsVersion(CVmConfiguration& c_, const QString& v_);
	static void configureNetwork(CVmConfiguration& c_, const QString& uuid_);
	static Libvirt::Result runProgram(
		Libvirt::Instrument::Agent::Vm::Guest guest_,
		const QString& uuid_,
		const Libvirt::Instrument::Agent::Vm::Exec::Request&);

protected:
	virtual void timerEvent(QTimerEvent*);

private:
	const QString m_uuid;
	Config::Edit::Atomic m_editor;
	bool m_fromKnownState;

	boost::promise<std::pair<PRL_VM_TOOLS_STATE, QString> > m_state;
};

}

#endif // __DSPVMGUESTPERSONALITY_H__
