///////////////////////////////////////////////////////////////////////////////
///
/// @file CDspVmGuest.h
///
/// Class for VM activity running inside guest agent
///
/// @author yur
///
/// Copyright (c) 2015-2017, Parallels International GmbH
/// Copyright (c) 2017-2019 Virtuozzo International GmbH, All rights reserved.
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
/// Our contact details: Virtuozzo International GmbH, Vordergasse 59, 8200
/// Schaffhausen, Switzerland.
///
/////////////////////////////////////////////////////////////////////////////////

#ifndef __CVMGUEST_H__
#define __CVMGUEST_H__

#include <QObject>
#include <QString>

#include <prlsdk/PrlEnums.h>
#include <prlxmlmodel/VmConfig/CVmConfiguration.h>

#include <boost/thread.hpp>
#include <boost/thread/future.hpp>

#include "CDspVmConfigManager.h"
#include "CVmIdent.h"

#ifdef _LIBVIRT_
#include "CDspLibvirt.h"
#include "CDspLibvirtExec.h"
#endif // _LIBVIRT_

namespace Vm
{
namespace State
{
struct Frontend;

} // namespace State

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
	Watcher(const CVmIdent& ident_, QWeakPointer<QAtomicInt> incarnation_) :
		m_retries(), m_ident(ident_), m_incarnation(incarnation_), m_ourIncarnation(0)
	{
		WRITE_TRACE(DBG_INFO, "%s new watcher", qPrintable(ident_.first));
		QSharedPointer<QAtomicInt> ref = incarnation_.toStrongRef();
		if (ref) {
			WRITE_TRACE(DBG_INFO, "%s watcher incarnation %d", qPrintable(ident_.first), int(*ref));
			m_ourIncarnation = *ref;
		}
	}

	void setRetries(quint32 value_)
	{
		m_retries = value_;
	}
	quint32 getRetries() const
	{
		return m_retries;
	}
	void adopt(PRL_VM_TOOLS_STATE state_, const QString& version_ = QString());
	Q_INVOKABLE void respin();

signals:
	void guestToolsStarted(const QString v_);
	

protected:
	virtual void timerEvent(QTimerEvent*);

private:
	int m_retries;
	CVmIdent m_ident;
	QWeakPointer<QAtomicInt> m_incarnation;
	int m_ourIncarnation;
};

///////////////////////////////////////////////////////////////////////////////
// struct Spin

struct Spin: QRunnable
{
	Spin(const CVmIdent& ident_, Watcher& watcher_):
		m_ident(ident_), m_watcher(&watcher_)
	{
	}

	void run();

private:
	CVmIdent m_ident;
	Watcher* m_watcher;
};

///////////////////////////////////////////////////////////////////////////////
// struct Connector

struct Connector
{
	Connector(const QString& directory_, State::Frontend& frontend_,
		QWeakPointer<QAtomicInt> incarnation_);

	Connector& setNetwork(Actor* value_);
	Connector& setRetries(quint32 value_);
	void operator()();

private:
	quint32 m_retries;
	QString m_directory;
	State::Frontend* m_frontend;
	QScopedPointer<Actor> m_network;
	QWeakPointer<QAtomicInt> m_incarnation;
};

} //namespace Guest
} //namespace Vm

#endif // __DSPVMGUESTPERSONALITY_H__
