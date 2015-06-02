/*
 * Copyright (c) 2015 Parallels IP Holdings GmbH
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
 * Our contact details: Parallels IP Holdings GmbH, Vordergasse 59, 8200
 * Schaffhausen, Switzerland.
 */

#ifndef H__CDspVmDirManager__H
#define H__CDspVmDirManager__H

#include "CDspSync.h"
#include "XmlModel/VmDirectory/CVmDirectories.h"
#include "CDspClient.h"
#include <prlsdk/PrlEnums.h>
#include <boost/iterator/iterator_facade.hpp>

class CDspVmDirManager;
namespace Vm
{
namespace Directory
{
QString getFolder(const QString& uuid_);

inline QString getFolder(const CDspClient& user_)
{
	return getFolder(user_.getVmDirectoryUuid());
}

inline QString getFolder(const CDispUserWorkspace& workspace_)
{
	return getFolder(workspace_.getVmDirectory());
}

///////////////////////////////////////////////////////////////////////////////
// struct Iterator

struct Iterator: boost::iterator_facade<Iterator, CVmDirectory,
			boost::forward_traversal_tag>
{
	typedef QList<CVmDirectory* >::const_iterator baseIterator_type;

        Iterator(baseIterator_type p_, baseIterator_type end_):
		m_p(p_), m_end(end_)
        {
        }

private:
	friend class boost::iterator_core_access;

	CVmDirectory& dereference() const
	{
		return **m_p;
	}
	bool equal(const Iterator& other_) const
	{
		return m_p == other_.m_p;
	}
	void increment()
	{
		if (m_end != m_p)
			++m_p;
	}

	baseIterator_type m_p;
	baseIterator_type m_end;
};

///////////////////////////////////////////////////////////////////////////////
// struct List

struct List
{
	typedef Iterator const_iterator;

	explicit List(const CVmDirectories& base_): m_base(&base_)
	{
	}

	const_iterator end() const
	{
		return Iterator(m_base->m_lstVmDirectory.constEnd(),
				m_base->m_lstVmDirectory.constEnd());
	}
	const_iterator begin() const
	{
		return Iterator(m_base->m_lstVmDirectory.constBegin(),
				m_base->m_lstVmDirectory.constEnd());
	}
private:
	const CVmDirectories* m_base;
};

namespace Item
{
///////////////////////////////////////////////////////////////////////////////
// struct Iterator

struct Iterator: boost::iterator_facade<Iterator, QPair<QString, CVmDirectoryItem* >,
			 boost::forward_traversal_tag>
{
        typedef Directory::Iterator outerIterator_type;
        typedef QList<CVmDirectoryItem* >::const_iterator innerIterator_type;

        Iterator(outerIterator_type outerP_, outerIterator_type outerEnd_):
                m_outerP(outerP_), m_outerEnd(outerEnd_)
        {
                setup();
        }

private:
	friend class boost::iterator_core_access;

	reference dereference() const;
	bool equal(const Iterator& other_) const
	{
		return m_innerP == other_.m_innerP && m_outerP == other_.m_outerP;
	}
	void increment();
	void setup();

	mutable value_type m_value;
	innerIterator_type m_innerP;
	innerIterator_type m_innerEnd;
	outerIterator_type m_outerP;
	outerIterator_type m_outerEnd;
};

///////////////////////////////////////////////////////////////////////////////
// struct List

struct List
{
	typedef Iterator const_iterator;
	typedef Iterator::value_type value_type;

	explicit List(const CVmDirectories& base_): m_base(base_)
	{
	}

	const_iterator end() const
	{
		return Iterator(m_base.end(), m_base.end());
	}
	const_iterator begin() const
	{
		return Iterator(m_base.begin(), m_base.end());
	}
private:
	Directory::List m_base;
};

} // namespace Item

namespace Dao
{
///////////////////////////////////////////////////////////////////////////////
// struct Impl

template<class T>
struct Impl
{
	List getList() const
	{
		const T& t = static_cast<const T& >(*this);
		return List(t.getService());
	}
	Item::List getItemList() const
	{
		const T& t = static_cast<const T& >(*this);
		return Item::List(t.getService());
	}
};

///////////////////////////////////////////////////////////////////////////////
// struct Locked

struct Locked: Impl<Locked>
{
	Locked();
	explicit Locked(CDspVmDirManager& manager_);

	const CVmDirectories& getService() const
	{
		return *m_service.getPtr();
	}
private:
	CDspLockedPointer<CVmDirectories> m_service;
};

///////////////////////////////////////////////////////////////////////////////
// struct Free

struct Free: Impl<Free>
{
	Free();

	const CVmDirectories& getService() const
	{
		return m_service;
	}
private:
	CVmDirectories m_service;
};

} // namespace Dao
} // namespace Directory
} // namespace Vm

///////////////////////////////////////////////////////////////////////////////
// class CDspVmDirManager

class CDspVmDirManager
{
	friend class CDspVmDirHelper;
	friend class CDspVmSnapshotStoreHelper;
	friend class CDspVm;

public:
	CDspVmDirManager();
	~CDspVmDirManager();

	/**
	* enable crash safe mech to save config on disk
	*/
	void enableCrashSafeMech();

	//////////////////////////////////////////////////////////////////////////
	//
	//   Vm Directory operations operations
	//
	//////////////////////////////////////////////////////////////////////////

	/** Returns all VM identifications */
	QList<CVmIdent> getAllVmIdList();

	/** Returns reference to vm dirs catalogue */
    CDspLockedPointer<CVmDirectories> getVmDirCatalogue();

    CDspLockedPointer<CVmDirectory> getVmDirectory( const QString& dirUuid );

	/**
	* method check parameters and locks it
	* @param vm directory uuid
	* @param struct with parameters to check and lock
	* @param [optional] pointer to list with error.
	*		If pErrorList == NULL method breaks search and return after first error.
	*		If pErrorList != NULL method continue search to end of that vm directory and add all errors to the list.
	* @return PRL_ERR_SUCCESS when all parameters not exist and not locked.
	* @return error when any parameter already exists
	*	( if pErrorList!=NULL return first member of pErrorList)
	**/
	PRL_RESULT checkAndLockNotExistsExclusiveVmParameters(
		const QString& dirUuid,
		CVmDirectory::TemporaryCatalogueItem* locksItem,
		QList<PRL_RESULT>* pErrorList = 0);
	/**
	* method check parameters in the VmDirCatalogue and locks it the lstDirUuid.
	* @param vm directory uuid list
	* @param struct with parameters to check and lock
	* @return PRL_ERR_SUCCESS when all parameters not exist and not locked.
	* @return error when any parameter already exists
	*	( if pErrorList!=NULL return first member of pErrorList)
	**/
	PRL_RESULT checkAndLockNotExistsExclusiveVmParameters(
		const QStringList &lstDirUuid,
		CVmDirectory::TemporaryCatalogueItem* locksItem,
		QList<PRL_RESULT>* pErrorList = 0);

	/**
	* method check parameters and locks it
	* return PRL_ERR_SUCCESS when all parameters exist and not locked.
	* return PRL_ERR_VM_ALREADY_REGISTERED_UNIQUE_PARAMS when operation on this VM is already in progress.
	* return PRL_ERR_INVALID_ARG when specified VM was not found
	* return PRL_ERR_VM_DIRECTORY_NOT_EXIST when dispatcher has bkonen directory configuration.
	**/
	PRL_RESULT lockExistingExclusiveVmParameters( const QString& dirUuid,
		CVmDirectory::TemporaryCatalogueItem* locksItem);

	PRL_RESULT lockExclusiveVmParameters( const QString& dirUuid,
		CVmDirectory::TemporaryCatalogueItem* locksItem);

	void unlockExclusiveVmParameters( const QString& dirUuid,
		CVmDirectory::TemporaryCatalogueItem* locksItem);
	void unlockExclusiveVmParameters(
		CVmDirectory::TemporaryCatalogueItem* locksItem);

	CDspLockedPointer<CVmDirectoryItem> getVmDirItemByUuid(
		const QString& dirUuid,
		const QString& vmUuid);

	CDspLockedPointer<CVmDirectoryItem> getVmDirItemByUuid(
		const CVmIdent& vmIdent )
	{
		return getVmDirItemByUuid( vmIdent.second, vmIdent.first );
	}

    CDspLockedPointer<CVmDirectoryItem> getVmDirItemByHome(
		const QString& dirUuid,
		const QString& vmHome);

    CDspLockedPointer<CVmDirectoryItem> getVmDirItemByName(
		const QString& dirUuid,
		const QString& vmName);

	// Helpers to get VmName and VmHome
	// NOTE: They locks vm directory inside ! (CDspLockedPointer<CVmDirectoryItem>)
	// returns "" if error.
	static QString getVmNameByUuid( const CVmIdent& vmIdent );
	static QString getVmHomeByUuid( const CVmIdent& vmIdent );

	/**
	 * Method that let to determine whether specified VM path was already registered some where in VM catalogue
	 * @param path to VM home dir
	 * @return sign whether specified VM home path already presents in VM catalogue
	 */
    bool checkWhetherVmAlreadyPresents( const QString& vmHome );

	/**
	* Method save VM directories catalogue
	* @param pCatalogue VM directories catalogue locked pointer
	* @return result - PRL_ERR_SUCCESS - saving was done, otherwise not
	*/
	PRL_RESULT saveVmDirCatalogue();

	/**
	* Method that returns list with commands which required confirmation notwithstanding VM confirmation settings.
	* @return result - list of commands
	*/
	QList<PRL_ALLOWED_VM_COMMAND> getCommonConfirmatedOperations();

public:
	typedef QHash< QString /* dirUuid */ , CDspLockedPointer<CVmDirectoryItem> >  VmDirItemsHash;
	typedef QHashIterator< QString /* dirUuid */ , CDspLockedPointer<CVmDirectoryItem> >  VmDirItemsHashIterator;

	/**
	* Method search in all vm directories VmItems with same vmUuid and vmPath
	* @param vmUuid
	* @param vmPath
	* @return hash with type VmDirItemsHash
	*/
	VmDirItemsHash findVmDirItemsInCatalogue( const QString& vmUuid, const QString& vmPath );
	PRL_RESULT updateVmDirItem ( const CDspLockedPointer<CVmDirectoryItem>& pVmDirItem );
	bool getVmTypeByUuid(const QString &sVmUuid, PRL_VM_TYPE &nType);

	PRL_RESULT initVzDirCatalogue();
	CDspLockedPointer<CVmDirectory> getVzDirectory();
	static QString getVzDirectoryUuid();

	PRL_RESULT setCatalogueFileName(const QString& value_);
protected:
	PRL_RESULT addNewVmDirectory( CVmDirectory* );

	PRL_RESULT addVmDirItem( const QString& dirUuid, CVmDirectoryItem* pVmDirItem );
	PRL_RESULT deleteVmDirItem( const QString& dirUuid, const QString& vmUuid);

private:
	QMutex	m_mutex;
	QString m_vmDirCatalogueFile;
	CVmDirectories m_vmDirCatalogue;

};
#endif //H__CDspVmDirManager__H
