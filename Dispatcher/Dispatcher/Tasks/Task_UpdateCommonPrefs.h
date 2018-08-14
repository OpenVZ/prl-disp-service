////////////////////////////////////////////////////////////////////////////////
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
/// @file
///	Task_UpdateCommonPrefs.h
///
/// @brief
///	Definition of the class CDspTaskHelper
///
/// @brief
///	This class implements long running tasks helper class
///
/// @author sergeyt
///	SergeyT@
///
////////////////////////////////////////////////////////////////////////////////

#ifndef __Task_UpdateCommonPrefs_H_
#define __Task_UpdateCommonPrefs_H_

#include "CDspTaskHelper.h"
#include "CDspInstrument.h"
#include <boost/phoenix/core/reference.hpp>

class SimpleLockFlag;
class CDispCommonPreferences;
class CDispMemoryPreferences;
class CDispCpuPreferences;
class CDispatcherConfig;

namespace Details
{
namespace Preference
{
typedef boost::phoenix::expression::reference<CDispCommonPreferences>::type reference_type;
typedef QPair<reference_type, reference_type> event_type;
typedef Instrument::Chain::Unit<event_type> chain_type;

///////////////////////////////////////////////////////////////////////////////
// struct Usb

struct Usb: chain_type
{
	Usb(const QString& directory_, const redo_type& redo_):
		chain_type(redo_), m_directory(directory_)
	{
	}

	result_type operator()(const request_type& request_);

private:
	QString m_directory;
};

///////////////////////////////////////////////////////////////////////////////
// struct Merge

struct Merge: std::unary_function<const event_type&, PRL_RESULT>
{
	Merge(const IOSender::Handle& client_, CDispatcherConfig& config_):
		m_client(client_), m_config(&config_)
	{
	}

	result_type operator()(argument_type request_);

private:
	IOSender::Handle m_client;
	CDispatcherConfig* m_config;
};

///////////////////////////////////////////////////////////////////////////////
// struct Envelope

struct Envelope: std::unary_function<CDispCommonPreferences&, PRL_RESULT>
{
	typedef chain_type::redo_type handler_type;

	Envelope(const handler_type& handler_, argument_type preferences_):
		m_handler(handler_), m_preferences(&preferences_)
	{
	}

	result_type operator()(argument_type preferences_)
	{
		return m_handler(qMakePair(boost::phoenix::ref(*m_preferences),
			boost::phoenix::ref(preferences_)));
	}

private:
	handler_type m_handler;
	CDispCommonPreferences* m_preferences;
};

} // namespace Preference
} // namespace Details

///////////////////////////////////////////////////////////////////////////////
// class Task_UpdateCommonPrefs

class Task_UpdateCommonPrefs : public  CDspTaskHelper
{
   Q_OBJECT
public:
	Task_UpdateCommonPrefs (
		SmartPtr<CDspClient>&,
		const SmartPtr<IOPackage>&,
		const QString& sCommonPrefs
		);


	virtual PRL_RESULT	prepareTask();
	virtual void			finalizeTask();

protected:
   virtual PRL_RESULT run_body();

private:
	PRL_RESULT saveCommonPrefs();

	void fixReadOnlyInCommonPrefs();
	void fixReadOnlyInWorkspace();
	void fixReadOnlyInRemoteDisplay();
	void fixReadOnlyInMemory();
	void fixReadOnlyInNetwork();
	void fixReadOnlyInPci();
	void fixReadOnlyInDebug();
	void fixReadOnlyListenAnyAddr();
	void fixReadOnlyInDispToDispPrefs();
	/**
	* Checks "AllowMultiplePMC" options change.
	* Affects firewall/iptables params if options were changed.
	*/
	void checkAndDisableFirewall();
	/**
	 * Store the task error code, log it, and return it unchanged.
	 * It allows do not use goto-style internal exceptions in run_body,
	 * and return error like this:
	 *		if (something_failed) return setErrorCode(PRL_ERR_XXX);
	 */
	PRL_RESULT setErrorCode(PRL_RESULT nResult);
	bool isHostIdChanged() const;
	PRL_RESULT updateHostId();
	PRL_RESULT checkHeadlessMode();

private:
	static QMutex	s_commonPrefsMutex;
	bool			m_commonPrefsMutexLocked;

	SmartPtr<CDispCommonPreferences>	m_pNewCommonPrefs;
	SmartPtr<CDispCommonPreferences>	m_pOldCommonPrefs;
};


#endif //__Task_UpdateCommonPrefs_H_
