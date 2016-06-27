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

namespace Guest
{

///////////////////////////////////////////////////////////////////////////////
// class Actor

class Actor: public QObject
{
	Q_OBJECT

public:
	explicit Actor(const Config::Edit::Atomic& editor_) : m_editor(editor_)
	{
	}

	static void setToolsVersion(CVmConfiguration& c_, const QString& v_);
	static void configureNetwork(CVmConfiguration& c_, QString& uuid,
			QScopedPointer<Libvirt::Instrument::Agent::Vm::Exec::Request>& r);
	static Libvirt::Result runProgram(
		Libvirt::Instrument::Agent::Vm::Guest guest_,
		const QString& uuid_,
		const Libvirt::Instrument::Agent::Vm::Exec::Request&);

public slots:
	void setToolsVersionSlot(const QString v_);
	void configureNetworkSlot(const QString v_);

private:
	Config::Edit::Atomic m_editor;
};

///////////////////////////////////////////////////////////////////////////////
// class Watcher

class Watcher: public QObject
{
	Q_OBJECT

public:
	explicit Watcher(const QString& uuid_) : m_uuid(uuid_)
	{
	}

	boost::future<PRL_VM_TOOLS_STATE> getFuture()
	{
		return m_state.get_future();
	}

signals:
	void guestToolsStarted(const QString v_);
	

protected:
	virtual void timerEvent(QTimerEvent*);

private:
	QString m_uuid;
	boost::promise<PRL_VM_TOOLS_STATE> m_state;
};

} //namespace Guest

} //namespace Vm

#endif // __DSPVMGUESTPERSONALITY_H__
