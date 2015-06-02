///////////////////////////////////////////////////////////////////////////////
///
/// @file Task_SearchLostConfigs.h
///
/// Implementation of task that order to search VM configurations at specified places those are not registered at
/// user VM directory.
///
/// @author sandro@
/// @owner sergeym@
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

#ifndef __Task_SearchLostConfigs_H_
#define __Task_SearchLostConfigs_H_

#include "CDspTaskHelper.h"
#include <QStringList>


typedef QHash<QString, QFileInfo> MapFoundVm;
/**
 * Implementation of task that searching non registered at user VM directory VMs
 */
class Task_SearchLostConfigs : public  CDspTaskHelper
{
Q_OBJECT

public:
	/**
	 * Class constructor.
	 * @param pointer to the user session object
	 * @param pointer to request package object
	 * @param list of dir entries where search have to be done (if list empty it's mean that search need to be done at all system local disks)
	 * @param sign return result in respons either by getResult()
	 * @param sign match directories with '*.pvm' suffix only
	 */
	Task_SearchLostConfigs( SmartPtr<CDspClient> &pUser,
			const SmartPtr<IOPackage> &pRequestPkg,
			const QStringList &lstSearchDirs,
			bool bResultInResponse = true,
			bool bPvmDirOnly = false,
			int nDepthLimit = -1);

	/**
	* Overridden method.
	* Returns result code of cancel operation for this task
	*/
	virtual PRL_RESULT getCancelResult();

	SmartPtr<MapFoundVm> getResult()
	{
		return m_lstFoundVms;
	}

protected:
	/**
	 * Returns result code which specifies whether all prestart conditions correct
	 */
	PRL_RESULT prepareTask();
	/**
	 * Overridden method of task body
	 */
	PRL_RESULT run_body();
	/**
	 * Makes all necessary actions (sending answer and etc.) on task comletion
	 */
	void finalizeTask();

private:
	/**
	 * List of dir entries where search must be done
	 */
	QStringList m_lstSearchDirs;
	/**
	 * List of pathes to found VMs
	 */
	SmartPtr<MapFoundVm> m_lstFoundVms;
	/**
	* List of pathes that will be skipped or searched in which firstly depends on value.
	*/
	QHash<QString, bool> m_lstSpecialPaths;

	/**
	* Sign return result in the response or by means of getResult()
	*/
	bool m_bResultInResponse;
	/**
	* Sign match directories with '*.pvm' suffix only
	*/
	bool m_bPvmDirOnly;
	/**
	* Limit processing directory tree by depth
	*/
	int m_nDepthLimit;
private:
	/**
	 * Processes specified search path
	 * @param path to searh directory
	 */
	void processSearchPath(const QString &sSearchPath);
};

#endif //__Task_SearchLostConfigs_H_
