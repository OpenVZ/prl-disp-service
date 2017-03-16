/*
 * Copyright (c) 2015-2017, Parallels International GmbH
 *
 * This file is part of Virtuozzo Core. Virtuozzo Core is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License as published by the Free Software Foundation;
 * either version 2 of the License, or (at your option) any later
 * version.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 * Our contact details: Parallels International GmbH, Vordergasse 59, 8200
 * Schaffhausen, Switzerland.
 */

#include "CDspVmDirManager.h"

#include "CDspService.h"
#include "Libraries/PrlCommonUtils/CFileHelper.h"

#include <prlcommon/Interfaces/ParallelsQt.h>
#include <prlcommon/Logging/Logging.h>
#include <prlcommon/PrlCommonUtilsBase/SysError.h>
#include <prlxmlmodel/VmConfig/CVmConfiguration.h>

#include <prlcommon/Std/PrlAssert.h>
#include "CDspVzHelper.h"


CDspVmDirManager::CDspVmDirManager()
:m_mutex( QMutex::Recursive )
{
}

CDspVmDirManager::~CDspVmDirManager()
{
}

void CDspVmDirManager::enableCrashSafeMech()
{
	if( ! getVmDirCatalogue()->enableCrashSafeSaving() )
		WRITE_TRACE(DBG_FATAL, "Unable to enableCrashSafeSaving mech for vm dir catalogue" );
}

QList<CVmIdent> CDspVmDirManager::getAllVmIdList()
{
	QList<CVmIdent> lstIdents;
	Vm::Directory::Dao::Locked d(*this);
	foreach (const Vm::Directory::Item::List::value_type& i, d.getItemList())
	{
		lstIdents += MakeVmIdent(i.second->getVmUuid(), i.first);
	}
	return lstIdents;
}

CDspLockedPointer<CVmDirectories>
CDspVmDirManager::getVmDirCatalogue()
{
	return CDspLockedPointer<CVmDirectories>( &m_mutex, &m_vmDirCatalogue );
}

CDspLockedPointer<CVmDirectory>
CDspVmDirManager::getVmDirectory( const QString& dirUuid )
{
	// lock before start search
	CDspLockedPointer<CVmDirectories>
		vmDirCatalogue = getVmDirCatalogue();

	CVmDirectory* pVmDir = vmDirCatalogue->getVmDirectoryByUuid( dirUuid );
	if ( !pVmDir )
		WRITE_TRACE(DBG_FATAL, "can't found VmDirectory by uuid=%s", QSTR2UTF8( dirUuid ) );

	return  CDspLockedPointer<CVmDirectory>( &m_mutex, pVmDir );
}

CDspLockedPointer<CVmDirectoryItem>
CDspVmDirManager::getVmDirItemByUuid(	const QString& dirUuid,
									 const QString& vmUuid)
{
	//LOCK before use
	CDspLockedPointer<CVmDirectory>
		pVmDir = getVmDirectory( dirUuid );

	if ( !pVmDir )
	{
		WRITE_TRACE(DBG_FATAL, "Can't found vmdir by uuid '%s'", QSTR2UTF8( dirUuid ) );
		return CDspLockedPointer<CVmDirectoryItem>( &m_mutex, 0);
	}

	CVmDirectoryItem* pItem = NULL;
	QListIterator<CVmDirectoryItem*> it( pVmDir->m_lstVmDirectoryItems );
	while ( it.hasNext() && !pItem )
	{
		pItem = it.next();
		if ( pItem->getVmUuid() != vmUuid )
			pItem = NULL;
	}

	return CDspLockedPointer<CVmDirectoryItem>( &m_mutex, pItem );
}

CDspLockedPointer<CVmDirectoryItem>
CDspVmDirManager::getVmDirItemByHome(
									 const QString& dirUuid,
									 const QString& vmHome
									 )
{
	//LOCK before use
	CDspLockedPointer<CVmDirectory>
		pVmDir = getVmDirectory( dirUuid );

	if ( !pVmDir )
	{
		WRITE_TRACE(DBG_FATAL, "Can't found vmdir by uuid '%s'", QSTR2UTF8( dirUuid ) );
		return CDspLockedPointer<CVmDirectoryItem>( &m_mutex, 0);
	}

	CVmDirectoryItem* pItem = NULL;
	QListIterator<CVmDirectoryItem*> it( pVmDir->m_lstVmDirectoryItems );
	while ( it.hasNext() && !pItem )
	{
		pItem = it.next();
		if ( !CFileHelper::IsPathsEqual(pItem->getVmHome(), vmHome ) )
			pItem = NULL;
	}

	return CDspLockedPointer<CVmDirectoryItem>( &m_mutex, pItem );
}

bool CDspVmDirManager::checkWhetherVmAlreadyPresents( const QString& vmHome )
{
	Vm::Directory::Dao::Locked d(*this);
	foreach (const Vm::Directory::Item::List::value_type& i, d.getItemList())
	{
		if( CFileHelper::IsPathsEqual( i.second->getVmHome(), vmHome ) )
			return true;
	}
	return (false);
}

CDspLockedPointer<CVmDirectoryItem>
CDspVmDirManager::getVmDirItemByName(
									 const QString& dirUuid,
									 const QString& vmName
									 )
{
	//LOCK before use
	CDspLockedPointer<CVmDirectory>
		pVmDir = getVmDirectory( dirUuid );

	if ( !pVmDir )
	{
		WRITE_TRACE(DBG_FATAL, "Can't found vmdir by uuid '%s'", QSTR2UTF8( dirUuid ) );
		return CDspLockedPointer<CVmDirectoryItem>( &m_mutex, 0);
	}

	CVmDirectoryItem* pItem = NULL;
	QListIterator<CVmDirectoryItem*> it( pVmDir->m_lstVmDirectoryItems );
	while ( it.hasNext() && !pItem )
	{
		pItem = it.next();
		if ( pItem->getVmName() == vmName )
			break;
		pItem = NULL;
	}

	return CDspLockedPointer<CVmDirectoryItem>( &m_mutex, pItem );
}

QString CDspVmDirManager::getVmNameByUuid( const CVmIdent& vmIdent )
{
	CDspLockedPointer<CVmDirectoryItem>
		pVmDirItem = CDspService::instance()->getVmDirManager().getVmDirItemByUuid(vmIdent);
	if( pVmDirItem )
		return pVmDirItem->getVmName();
	return QString();
}

QString CDspVmDirManager::getVmHomeByUuid( const CVmIdent& vmIdent )
{
	CDspLockedPointer<CVmDirectoryItem>
		pVmDirItem = CDspService::instance()->getVmDirManager().getVmDirItemByUuid(vmIdent);
	if( pVmDirItem )
		return pVmDirItem->getVmHome();
	return QString();
}


PRL_RESULT CDspVmDirManager::addNewVmDirectory( CVmDirectory* pVmDir )
{
	if ( ! pVmDir )
		return PRL_ERR_INVALID_PARAM;

	//LOCK before use
	CDspLockedPointer<CVmDirectories> pCatalogue = getVmDirCatalogue();

	if ( pCatalogue->getVmDirectoryByUuid( pVmDir->getUuid() ) )
	{
		WRITE_TRACE(DBG_FATAL, "PRL_ERR_VMDIR_ALREADY_EXIST [%s]", QSTR2UTF8( pVmDir->getUuid() ) );
		return PRL_ERR_VM_DIR_CONFIG_ALREADY_EXISTS;
	}

	pCatalogue->addVmDirectory( pVmDir );

	return saveVmDirCatalogue();
}

PRL_RESULT CDspVmDirManager::addVmDirItem( const QString& dirUuid, CVmDirectoryItem* pVmDirItem )
{
	if ( ! pVmDirItem )
		return PRL_ERR_INVALID_PARAM;

	CDspLockedPointer<CVmDirectory>
		pVmDirectory = getVmDirectory( dirUuid );

	if ( ! pVmDirectory )
		return PRL_ERR_VM_DIRECTORY_NOT_EXIST;


	// WE MUST CHECK VM UUID for valid work with deleteVmDirItem() method.
	//  (allowed only original vm_uuid in vm directory. )
	//
	CDspLockedPointer<CVmDirectoryItem>
		pExistsItem = CDspVmDirManager::getVmDirItemByUuid( dirUuid, pVmDirItem->getVmUuid() );

	if ( pExistsItem )
		return PRL_ERR_ENTRY_ALREADY_EXISTS;

	pVmDirectory->addVmDirectoryItem( pVmDirItem );

	PRL_RESULT res = saveVmDirCatalogue();

	if ( ! PRL_SUCCEEDED( res ) )
	{
		// rollback
		QMutableListIterator<CVmDirectoryItem*> it( pVmDirectory->m_lstVmDirectoryItems );
		while ( it.hasNext() )
		{
			CVmDirectoryItem* pItem = it.next();
			if ( pItem != pVmDirItem )
				continue;

			it.remove();
			break;
		}
	}
	else
	{
		
#if _CT_
		if (pVmDirItem->getVmType() == PVT_CT)
			CVzHelper::update_ctid_map(pVmDirItem->getVmUuid(), pVmDirItem->getCtId());
#endif
		if (pVmDirItem->getVmType() == PVT_VM)
		{
			CDspService::instance()->getVmConfigWatcher().registerVmToWatch( pVmDirItem->getVmHome(),
											pVmDirectory->getUuid(),
											pVmDirItem->getVmUuid());
			// to prevent conflicts
			CDspService::instance()->getVmConfigManager().removeFromCache( pVmDirItem->getVmHome() );
		}
	}

	return res;
}

PRL_RESULT CDspVmDirManager::deleteVmDirItem( const QString& dirUuid, const QString& vmUuid )
{
	CDspLockedPointer<CVmDirectory>
		pVmDir = getVmDirectory( dirUuid );

	if ( ! pVmDir )
		return PRL_ERR_VM_DIRECTORY_NOT_EXIST;

	CVmDirectoryItem* pItem = NULL;
	QMutableListIterator<CVmDirectoryItem*> it( pVmDir->m_lstVmDirectoryItems );
	while ( it.hasNext() && !pItem )
	{
		pItem = it.next();
		if ( pItem->getVmUuid() != vmUuid )
		{
			pItem = NULL;
			continue;
		}

		it.remove();
		break;
	}

	if( ! pItem )
		return PRL_ERR_ENTRY_DOES_NOT_EXIST;

	PRL_RESULT res = saveVmDirCatalogue();

	if ( ! PRL_SUCCEEDED( res ) )
	{
		// rollback
		pVmDir->addVmDirectoryItem( pItem );
	}
	else
	{
#if _CT_
		if (pItem->getVmType() == PVT_CT)
		{
			CVzHelper::update_ctid_map(pItem->getVmUuid(), QString());
			CDspService::instance()->getVzHelper()->getConfigCache().
				remove(pItem->getVmHome());
		}
#endif

		if (pItem->getVmType() == PVT_VM)
		{
			CDspService::instance()->getVmConfigWatcher().unregisterVmToWatch( pItem->getVmHome() );
			CDspService::instance()->getVmConfigManager().removeFromCache( pItem->getVmHome() );
		}
		if( pItem )
			delete pItem;
	}


	return res;
}

PRL_RESULT	CDspVmDirManager::saveVmDirCatalogue()
{
	CDspLockedPointer<CVmDirectories> pCatalogue = getVmDirCatalogue();
	QFile f(m_vmDirCatalogueFile);
	PRL_RESULT save_rc = pCatalogue->saveToFile(&f);
	if( PRL_FAILED( save_rc ) )
	{
		WRITE_TRACE(DBG_FATAL, "Error %s on save VM catalogue file. Reason: %ld: %s. path = '%s'"
			, PRL_RESULT_TO_STRING(save_rc)
			, Prl::GetLastError()
			, QSTR2UTF8( Prl::GetLastErrorAsString() )
			, QSTR2UTF8( m_vmDirCatalogueFile )
			);
		return PRL_ERR_SAVE_VM_CATALOG;
	}

	return PRL_ERR_SUCCESS;
}

PRL_RESULT CDspVmDirManager::checkAndLockNotExistsExclusiveVmParameters(
	const QString& dirUuid,
	CVmDirectory::TemporaryCatalogueItem* lockItem,
	QList<PRL_RESULT>* pErrorList
	)
{
	PRL_RESULT res = PRL_ERR_SUCCESS;
	QList<PRL_RESULT> errorList;

	if(pErrorList)
		pErrorList->clear();

	try
	{
		PRL_ASSERT( lockItem );
		if ( ! lockItem )
		{
			errorList << PRL_ERR_INVALID_ARG;
			throw errorList;
		}

		//LOCK
		CDspLockedPointer<CVmDirectory> pVmDir = getVmDirectory( dirUuid );

		if ( !pVmDir )
		{
			errorList << PRL_ERR_VM_DIRECTORY_NOT_EXIST;
			throw errorList;
		}

		//////////////////////////////////////////////////////////////////////////
		// search in pVmDir for uuid and home
		//////////////////////////////////////////////////////////////////////////
		QListIterator<CVmDirectoryItem*> it( pVmDir->m_lstVmDirectoryItems );
		while ( it.hasNext() )
		{
			CVmDirectoryItem* pItem = it.next();
			if( !lockItem->vmUuid.isEmpty() && lockItem->vmUuid == pItem->getVmUuid() )
				errorList << PRL_ERR_VM_ALREADY_REGISTERED_VM_UUID;

			if ( !lockItem->vmName.isEmpty() && pItem->getVmName() == lockItem->vmName )
				errorList << PRL_ERR_VM_ALREADY_REGISTERED_VM_NAME;

			if( !lockItem->vmXmlPath.isEmpty() && !pItem->getVmHome().isEmpty()
				&& CFileHelper::IsPathsEqual( pItem->getVmHome(), lockItem->vmXmlPath )
				)
				errorList << PRL_ERR_VM_ALREADY_REGISTERED_VM_PATH;

			if( errorList.size()
				&& lockItem->vmUuid == pItem->getVmUuid()
				&& CFileHelper::IsPathsEqual( pItem->getVmHome(), lockItem->vmXmlPath )
				&& pItem->getVmName() == lockItem->vmName
				)
			{
				errorList.clear();
				errorList << PRL_ERR_VM_ALREADY_REGISTERED;
				throw errorList;
			}

			// continue only if need collect all errors (pErrorList!=NULL)
			if( !pErrorList && errorList.size()>0 )
				throw errorList;
		}

		if( errorList.size()>0 )
			throw errorList;

		//////////////////////////////////////////////////////////////////////////
		// search in TemporaryCatalogue
		//////////////////////////////////////////////////////////////////////////
		QListIterator<CVmDirectory::TemporaryCatalogueItem> tmpIt( *pVmDir->getTemporaryCatalogue() );
		while ( tmpIt.hasNext() && PRL_SUCCEEDED( res ) )
		{
			const CVmDirectory::TemporaryCatalogueItem&
				item = tmpIt.next();
			if (
				( !lockItem->vmUuid.isEmpty() && lockItem->vmUuid == item.vmUuid )
				||
				( !lockItem->vmXmlPath.isEmpty()
				&& CFileHelper::IsPathsEqual(lockItem->vmXmlPath, item.vmXmlPath )
				)
				||
				( !lockItem->vmName.isEmpty() && lockItem->vmName == item.vmName)
				)
			{
				WRITE_TRACE(DBG_FATAL, "Locked in TemporaryCatalogue uuid=%s path=%s name=%s",
						QSTR2UTF8(item.vmUuid),  QSTR2UTF8(item.vmXmlPath), QSTR2UTF8(item.vmName));
				errorList << PRL_ERR_VM_ALREADY_REGISTERED_UNIQUE_PARAMS;
				throw errorList;
			}
		}

		//////////////////////////////////////////////////////////////////////////
		// store to TemporaryCatalogue
		//////////////////////////////////////////////////////////////////////////
		pVmDir->getTemporaryCatalogue()->push_back( *lockItem );
		return PRL_ERR_SUCCESS;

	}catch( QList<PRL_RESULT>& errList)
	{
		PRL_ASSERT( errList.size() > 0 );
		WRITE_TRACE(DBG_FATAL, "Unable to lock exclusive parameters for VM '%s' '%s' '%s'",
			QSTR2UTF8(lockItem->vmUuid),
			QSTR2UTF8(lockItem->vmName),
			QSTR2UTF8(lockItem->vmXmlPath)
		);
		for(int i=0; i< errList.size(); i++)
		{
			WRITE_TRACE( DBG_FATAL, "%s: %#X '%s' ",
				(0==i) ? "with errors:":"",
				errList[i], PRL_RESULT_TO_STRING(errList[i])
				);
		}
		if(pErrorList)
			*pErrorList = errList;
		return errList.first();
	}

}

PRL_RESULT CDspVmDirManager::checkAndLockNotExistsExclusiveVmParameters(
	const QStringList &lstDirUuid,
	CVmDirectory::TemporaryCatalogueItem* lockItem,
	QList<PRL_RESULT>* pErrorList)
{
	Vm::Directory::Dao::Locked x(*this);
	foreach (const CVmDirectory& d, x.getList())
	{
		if (!lstDirUuid.empty() && !lstDirUuid.contains(d.getUuid()))
			continue;

		PRL_RESULT res = checkAndLockNotExistsExclusiveVmParameters(
					d.getUuid(), lockItem, pErrorList);
		if (PRL_FAILED(res))
		{
			// rollback the locks
			unlockExclusiveVmParameters(lockItem);
			return res;
		}
	}
	return PRL_ERR_SUCCESS;
}

PRL_RESULT CDspVmDirManager::lockExistingExclusiveVmParameters(
	const QString& dirUuid, CVmDirectory::TemporaryCatalogueItem* lockItem)
{
	PRL_ASSERT( lockItem );
	if (!lockItem)
		return PRL_ERR_INVALID_ARG;

	CDspLockedPointer<CVmDirectory> pVmDir = getVmDirectory(dirUuid);
	if (!pVmDir)
		return PRL_ERR_VM_DIRECTORY_NOT_EXIST;

	QListIterator<CVmDirectoryItem*> it( pVmDir->m_lstVmDirectoryItems );
	bool bFound = false;
	while (it.hasNext() && !bFound)
	{
		CVmDirectoryItem* pItem = it.next();
		if (lockItem->vmUuid == pItem->getVmUuid())
		{
			bFound = true;
			break;
		}
	}

	if (!bFound)
		return PRL_ERR_VM_UUID_NOT_FOUND;

	QListIterator<CVmDirectory::TemporaryCatalogueItem> tmpIt(*pVmDir->getTemporaryCatalogue());
	while (tmpIt.hasNext())
	{
		const CVmDirectory::TemporaryCatalogueItem& item = tmpIt.next();
		if (!lockItem->vmUuid.isEmpty() && lockItem->vmUuid == item.vmUuid)
			return PRL_ERR_VM_ALREADY_REGISTERED_UNIQUE_PARAMS;
	}

	pVmDir->getTemporaryCatalogue()->push_back(*lockItem);
	return PRL_ERR_SUCCESS;
}

PRL_RESULT CDspVmDirManager::lockExclusiveVmParameters(
	const QString& dirUuid, CVmDirectory::TemporaryCatalogueItem* lockItem)
{
	PRL_ASSERT( lockItem );
	if (!lockItem)
		return PRL_ERR_INVALID_ARG;

	CDspLockedPointer<CVmDirectory> pVmDir = getVmDirectory(dirUuid);
	if (!pVmDir)
		return PRL_ERR_VM_DIRECTORY_NOT_EXIST;

	QListIterator<CVmDirectory::TemporaryCatalogueItem> tmpIt(*pVmDir->getTemporaryCatalogue());
	while (tmpIt.hasNext())
	{
		const CVmDirectory::TemporaryCatalogueItem& item = tmpIt.next();
		if (!lockItem->vmUuid.isEmpty() && lockItem->vmUuid == item.vmUuid)
			return PRL_ERR_VM_ALREADY_REGISTERED_UNIQUE_PARAMS;
	}

	pVmDir->getTemporaryCatalogue()->push_back(*lockItem);
	return PRL_ERR_SUCCESS;
}

void CDspVmDirManager::unlockExclusiveVmParameters( const QString& dirUuid,
	CVmDirectory::TemporaryCatalogueItem* lockItem)
{
	PRL_ASSERT( lockItem );
	if ( ! lockItem )
		return;

	//LOCK
	CDspLockedPointer<CVmDirectory> pVmDir = getVmDirectory( dirUuid );

	PRL_ASSERT( pVmDir );
	if ( !pVmDir )
		return;

	QMutableListIterator<CVmDirectory::TemporaryCatalogueItem>
		tmpIt( *pVmDir->getTemporaryCatalogue() );
	while ( tmpIt.hasNext() )
	{
		const CVmDirectory::TemporaryCatalogueItem&
			item = tmpIt.next();

		if ( item.m_sItemId != lockItem->m_sItemId )
			continue;

		tmpIt.remove();
		break;
	}
}

void CDspVmDirManager::unlockExclusiveVmParameters(
	CVmDirectory::TemporaryCatalogueItem* lockItem)
{
	CDspLockedPointer<CVmDirectories> pCatalogue = getVmDirCatalogue();
	foreach(CVmDirectory* pDir, *pCatalogue->getVmDirectoriesList())
	{
		unlockExclusiveVmParameters(pDir->getUuid(), lockItem);
	}
}

PRL_RESULT CDspVmDirManager::updateVmDirItem ( const CDspLockedPointer<CVmDirectoryItem>& pVmDirItem )
{
	if ( ! pVmDirItem )
		return PRL_ERR_INVALID_ARG;

	return saveVmDirCatalogue();
//	CDspService::instance()->getVmConfigWatcher().update();
}

CDspVmDirManager::VmDirItemsHash
CDspVmDirManager::findVmDirItemsInCatalogue( const QString& vmUuid, const QString& vmPath )
{
	VmDirItemsHash  hash;
	Vm::Directory::Dao::Locked d(*this);
	foreach (const Vm::Directory::Item::List::value_type& i, d.getItemList())
	{
		if(vmUuid == i.second->getVmUuid()
			&& CFileHelper::IsPathsEqual( vmPath, i.second->getVmHome())
		)
		{
			hash.insert(i.first, CDspLockedPointer<CVmDirectoryItem>(&m_mutex, i.second));
		}//if
	}

	return hash;
}

QList<PRL_ALLOWED_VM_COMMAND>
CDspVmDirManager::getCommonConfirmatedOperations()
{
	return getVmDirCatalogue()->getCommonLockedOperations()->getLockedOperations();
}

#define VZ_DIRECTORY_UUID   "{03bbd245-ddf6-42fe-9812-46ac816349f5}"
QString CDspVmDirManager::getVzDirectoryUuid()
{
	return VZ_DIRECTORY_UUID;
}

QString CDspVmDirManager::getTemplatesDirectoryUuid()
{
	return "{a610e7fa-23d7-48cd-9086-b9c82021a885}";
}

/*
 * Register Containers in the fake VzDirectory
 */

PRL_RESULT CDspVmDirManager::initVzDirCatalogue()
{
#ifdef _CT_
	PRL_RESULT result = PRL_ERR_SUCCESS;

#define VZ_DIR_PATH CDspService::instance()->getVzHelper()->getVzlibHelper().getVzPrivateDir()

	CVmDirectory* pVzDir = new CVmDirectory(
				getVzDirectoryUuid(),
				QString(VZ_DIR_PATH),
				"Virtuozzo Container"
				);

	WRITE_TRACE(DBG_FATAL, "initVzDirCatalogue");
	result = CDspService::instance()->getVzHelper()->fillVzDirectory(pVzDir);
	if (PRL_FAILED(result))
	{
		delete pVzDir;
		return result;
	}

	// Lock the Catalogue
	CDspLockedPointer<CVmDirectories> pCatalogue = getVmDirCatalogue();

	for (int i = 0; i < pCatalogue->m_lstVmDirectory.count(); i++)
	{
		CVmDirectory *pDir = pCatalogue->m_lstVmDirectory[i];
		if (pDir->getUuid() == getVzDirectoryUuid())
		{
			delete pDir;
			pCatalogue->m_lstVmDirectory.removeAt(i);
			break;
		}
	}

	pCatalogue->addVmDirectory( pVzDir );

	foreach(CVmDirectoryItem *pItem, pVzDir->m_lstVmDirectoryItems)
		CVzHelper::update_ctid_map(pItem->getVmUuid(), pItem->getCtId());

	result = saveVmDirCatalogue();

	return result;
#else
	return PRL_ERR_SUCCESS;
#endif
}

PRL_RESULT CDspVmDirManager::initTemplatesDirCatalogue()
{
	WRITE_TRACE(DBG_FATAL, "initTemplatesDirCatalogue");

	CDspLockedPointer<CVmDirectories> catalogue(getVmDirCatalogue());
	QList<CVmDirectory *>::iterator last(catalogue->m_lstVmDirectory.end());
	QList<CVmDirectory *>::iterator it(std::find_if(
				catalogue->m_lstVmDirectory.begin(), last,
				boost::bind(&CVmDirectory::getUuid, _1) == getTemplatesDirectoryUuid()));
	if (it == last)
	{
		CVmDirectory* templatesDir(new CVmDirectory(
			getTemplatesDirectoryUuid(),
			ParallelsDirs::getCommonDefaultVmCatalogue(),
			"Virtuozzo VM Templates"));
		catalogue->addVmDirectory(templatesDir);
		return saveVmDirCatalogue();
	}

	return PRL_ERR_SUCCESS;
}

CDspLockedPointer<CVmDirectory>
CDspVmDirManager::getVzDirectory()
{
	return getVmDirectory(getVzDirectoryUuid());
}

CDspLockedPointer<CVmDirectory>
CDspVmDirManager::getTemplatesDirectory()
{
	return getVmDirectory(getTemplatesDirectoryUuid());
}

bool CDspVmDirManager::getVmTypeByUuid(const QString &sVmUuid, PRL_VM_TYPE &nType)
{ 
	QString sDirUuid = CDspService::instance()->getVmDirManager().getVzDirectoryUuid();
	CDspLockedPointer<CVmDirectoryItem> pDirItem = CDspService::instance()->
			getVmDirHelper().getVmDirectoryItemByUuid(sDirUuid, sVmUuid);
	if (!pDirItem.isValid())
		return false;

	nType = pDirItem->getVmType();

	return true;
}

boost::optional<PRL_VM_TYPE> CDspVmDirManager::getVmTypeByIdent(const CVmIdent& ident_) const
{
	CDspLockedPointer<CVmDirectoryItem> pDirItem = CDspService::instance()->
			getVmDirHelper().getVmDirectoryItemByUuid(ident_.second, ident_.first);
	if (!pDirItem.isValid())
		return boost::none;

	return pDirItem->getVmType();
}

PRL_RESULT CDspVmDirManager::setCatalogueFileName(const QString& value_)
{
	QMutexLocker g(&m_mutex);
	if (CFileHelper::IsPathsEqual(m_vmDirCatalogueFile, value_))
		return PRL_ERR_SUCCESS;

	QFile f(value_);
	PRL_RESULT e = 	m_vmDirCatalogue.loadFromFile(&f);
	if (PRL_FAILED(e))
	{
		WRITE_TRACE(DBG_FATAL, "Can't load the catalog from file name %s: %d",
			QSTR2UTF8(value_), e);
		return e;
	}
	m_vmDirCatalogueFile = value_;
	return PRL_ERR_SUCCESS;
}

namespace Vm
{
namespace Directory
{
QString getFolder(const QString& uuid_)
{
	CDspLockedPointer<CVmDirectory> x = CDspService::instance()
				->getVmDirManager().getVmDirectory(uuid_);
	return x.isValid() ? x->getDefaultVmFolder() : QString();
}

namespace Dao
{
///////////////////////////////////////////////////////////////////////////////
// struct Locked

Locked::Locked(): m_service(CDspService::instance()->getVmDirManager().getVmDirCatalogue())
{
}

Locked::Locked(CDspVmDirManager& manager_): m_service(manager_.getVmDirCatalogue())
{
}

///////////////////////////////////////////////////////////////////////////////
// struct Free

Free::Free(): m_service(CDspService::instance()->getVmDirManager().getVmDirCatalogue().getPtr())
{
}

} // namespace Dao

namespace Item
{
///////////////////////////////////////////////////////////////////////////////
// struct Iterator

Iterator::reference Iterator::dereference() const
{
	m_value.first = m_outerP->getUuid();
	m_value.second = *m_innerP;
	return m_value;
}

void Iterator::increment()
{
	if (m_outerEnd == m_outerP)
		return;
	if (m_innerEnd == ++m_innerP)
	{
		++m_outerP;
		setup();
	}
}
void Iterator::setup()
{
	for (; m_outerP != m_outerEnd; ++m_outerP)
	{
		if (!m_outerP->m_lstVmDirectoryItems.isEmpty())
		{
			m_innerP = m_outerP->m_lstVmDirectoryItems.constBegin();
			m_innerEnd = m_outerP->m_lstVmDirectoryItems.constEnd();
			return;
		}
	}
	m_innerP = m_innerEnd = innerIterator_type();
}

} // namespace Item
} // namespace Directory
} // namespace Vm

