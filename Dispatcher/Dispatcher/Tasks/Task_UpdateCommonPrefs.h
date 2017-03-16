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

class SimpleLockFlag;
class CDispCommonPreferences;
class CDispMemoryPreferences;
class CDispCpuPreferences;

class Task_UpdateCommonPrefs : public  CDspTaskHelper
{
   Q_OBJECT
public:
	Task_UpdateCommonPrefs (
		SmartPtr<CDspClient>&,
		const SmartPtr<IOPackage>&,
		const QString& sCommonPrefs
		);
	virtual ~Task_UpdateCommonPrefs();


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
	void fixReadOnlyInUsbPrefs( CDispCommonPreferences * pOldCommonPrefs );
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
	PRL_RESULT updateCpuFeaturesMask(
		const CDispCpuPreferences &oldMask, const CDispCpuPreferences &newMask);

private:
	static QMutex	s_commonPrefsMutex;
	bool			m_commonPrefsMutexLocked;

	SmartPtr<CDispCommonPreferences>	m_pNewCommonPrefs;
	SmartPtr<CDispCommonPreferences>	m_pOldCommonPrefs;
};


#endif //__Task_UpdateCommonPrefs_H_
