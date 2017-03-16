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
///	Task_CloneVm.h
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

#ifndef __Task_CloneVm_H_
#define __Task_CloneVm_H_

#include "CDspTaskHelper.h"
#include "CDspRegistry.h"
#include <prlxmlmodel/VmConfig/CVmConfiguration.h>
#include "Tasks/Mixin_CreateVmSupport.h"
#include <prlsdk/PrlDisk.h>
#include <QWaitCondition>

namespace Clone
{
namespace Source
{
struct Total;
} // namespace Source
} // namespace Clone

class Task_CloneVm: public  CDspTaskHelper
{
   Q_OBJECT
public:
	Task_CloneVm(Registry::Public& registry_,
		SmartPtr<CDspClient>&,
		const SmartPtr<IOPackage>&,
		SmartPtr<CVmConfiguration> pVmConfig,
		const QString& strVmNewName,
		const QString& sVmNewUuid,
		const QString & strVmNewPathName,
		unsigned int nFlags);

	~Task_CloneVm();

	virtual QString getVmUuid();

	/**
	 * Overridden method.
	 * Returns result code of cancel operation for this task
	 */
	virtual PRL_RESULT getCancelResult();

	/**
	 * This method uses at clone HDD state actions in order
	 * to setup thread safely operation result code
	 * @param setting operation result code
	 */
	const QString& getNewVmName() const
	{
		return m_newVmName;
	}
	const CVmIdent& getNewVmIdent() const
	{
		return m_newVmIdent;
	}
	const SmartPtr<CVmConfiguration>& getVmConfig() const
	{
		return m_pOldVmConfig;
	}
	bool isCreateTemplate() const
	{
		return m_bCreateTemplate;
	}
	QString getNewVmHome() const;
	PRL_RESULT track(CDspTaskHelper* task_);
	Registry::Public& getRegistry()
	{
		return m_registry;
	}

public:
	/**
	 * HDD clone state processing callback
	 * @param callback type - never used - don't know what is it actually
	 * @param iProgress - operation execution result (it can be either
	 * negative - in this case it's some of errors codes from PRL_RESULT or it
	 * can be equal to PRL_STATE_PROGRESS_COMPLETED - it means that operation
	 * completed successfully)
	 * @param pointer to the user data (in our case it's pointer to
	 * parent Task_CloneVm instance)
	 * @return sign whether operation should be interrupted (PRL_FALSE) or
	 * continued (PRL_TRUE)
	 */
	static PRL_BOOL CloneStateCallback(PRL_STATES_CALLBACK_TYPE iCallbackType, PRL_INT32 iProgress, PRL_VOID_PTR pParameter);

	/**
	* reset unique network settings for adapters in config
	*/
	static void ResetNetSettings( SmartPtr<CVmConfiguration> pVmConfig );

protected:
	virtual PRL_RESULT prepareTask();
	virtual void finalizeTask();

	virtual PRL_RESULT run_body();

	virtual void cancelOperation(SmartPtr<CDspClient> pUserSession, const SmartPtr<IOPackage> &p);

private:
	void setTaskParameters( const QString& strVmUuid,
		const QString& strVmNewName,
		const QString & strNewVmRootDir,
		bool bCreateTemplate );

	/**
	* form and send response on clone request
	*/
	void SendCloneResponse();

	/**
	* reimpl
	*/
	virtual bool providedAdditionState(){return true;}

	template<class T>
	PRL_RESULT do_(T tag_, Clone::Source::Total& source_);
private:
	Registry::Public& m_registry;
	SmartPtr<CVmConfiguration> m_pOldVmConfig;

	QString	m_newVmUuid;
	QString  m_newVmName;
	QString  m_newVmXmlPath;

	// list of pathes for delete on cancel
	QStringList m_cCloneUndoList;
	QMutex m_mtxWaitExternalTask;
	// External task thread (change SID)
	CDspTaskHelper *m_externalTask;

	bool	m_flgLockRegistred;
	CVmDirectory::TemporaryCatalogueItem*  m_pVmInfo;

	/** Sign whether VM template must be created */
	bool m_bCreateTemplate;
	/** Sign whether SID should be changed */
	bool m_bChangeSID;
	/** Sign whether linked clone should be created */
	bool m_bLinkedClone;
	/** Sign whether Boot Camp partitions must be imported */
	bool m_bImportBootCamp;
	CVmIdent m_newVmIdent;
};



#endif //__Task_CloneVm_H_
