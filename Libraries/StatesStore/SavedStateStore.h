////////////////////////////////////////////////////////////////////////////////
///
/// @file SavedStateStore.h
///
/// This class implements a description of checkpoint
///
/// @author MaximKi
/// @owner SergeyM
///
/// Copyright (c) 2005-2017, Parallels International GmbH
///
/// This file is part of Virtuozzo Core Libraries. Virtuozzo Core
/// Libraries is free software; you can redistribute it and/or modify it
/// under the terms of the GNU Lesser General Public License as published
/// by the Free Software Foundation; either version 2.1 of the License, or
/// (at your option) any later version.
///
/// This library is distributed in the hope that it will be useful,
/// but WITHOUT ANY WARRANTY; without even the implied warranty of
/// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
/// Lesser General Public License for more details.
///
/// You should have received a copy of the GNU Lesser General Public
/// License along with this library.  If not, see
/// <http://www.gnu.org/licenses/> or write to Free Software Foundation,
/// 51 Franklin Street, Fifth Floor Boston, MA 02110, USA.
///
/// Our contact details: Parallels International GmbH, Vordergasse 59, 8200
/// Schaffhausen, Switzerland.
///
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "SavedStateTree.h"


/**
 * Description of the part that provide access to saved states struct.
 *
 * @author maximki
 */
class CSavedStateStore
{
public:

	/// Standard class constructor
	CSavedStateStore();

	/**
	 * Class constructor
	 *
	 * @author maximki
	 * @param xml file name
	 */
	CSavedStateStore(const QString sXmlFileName);

	/**
	 * Class destructor
	 */
	~CSavedStateStore();

	/**
	 * Function return tree of saved states
	 *
	 * @author maximki
	 * @return reference to saved states tree
	 */
	CSavedStateTree * GetSavedStateTree() { return m_SST; }

	/**
	 * Find currently active saved state
	 *
	 * @author maximki
	 * @return reference to current snapshot node
	 */
	CSavedStateTree * FindCurrentSnapshot();

	/**
	 * Create new snapshot node
	 *
	 * @author maximki
	 * @param description of new saved state element
	 * @return operation result
	 */
	SnapshotParser::SnapshotReturnCode CreateSnapshot(CSavedState state);

	/**
	 * Delete shapshot node
	 *
	 * @author maximki
	 * @param deleted element
	 * @return operation result
	 */
	SnapshotParser::SnapshotReturnCode DeleteNode(CSavedStateTree *node);

	/**
	 * Delete shapshot node
	 *
	 * @author maximki
	 * @param saved state guid
	 * @return operation result
	 */
	SnapshotParser::SnapshotReturnCode DeleteNode(QString sGuid);

	/**
	 * Delete shapshot branch
	 *
	 * @author maximki
	 * @param root of the deleted elements
	 * @return operation result
	 */
	SnapshotParser::SnapshotReturnCode DeleteBranch(CSavedStateTree *node);

	/**
	 * Delete shapshot branch
	 *
	 * @author maximki
	 * @param saved state guid
	 * @return operation result
	 */
	SnapshotParser::SnapshotReturnCode DeleteBranch(QString sGuid);

	/**
	 * Load shapshot tree from file
	 *
	 * @author maximki
	 * @param file name
	 * @return operation result
	 */
	SnapshotParser::SnapshotReturnCode Load(const QString sXmlFileName);

	/**
	 * Load shapshot tree from file
	 *
	 * @author maximki
	 * @param file name
	 * @return operation result
	 */
	SnapshotParser::SnapshotReturnCode Load(const QString & content, const QString & strSnapshotFolder);

	/**
	 * Save shapshot tree to file
	 *
	 * @author maximki
	 * @return operation result
	 */
	SnapshotParser::SnapshotReturnCode Save(bool all_save = false);
	SnapshotParser::SnapshotReturnCode Save(const QString& sXmlFileName, bool all_save = false);
	SnapshotParser::SnapshotReturnCode Save(QIODevice& ioDevice, bool all_save = false) const;

	/**
	 * Clear tree of saved states
	 *
	 * @author maximki
	 * @return nothing
	 */
	void ClearSavedStateTree();

	/**
	 *
	 */
	bool IsEmpty();

	QString GetLoadedFileName();

	QString GetSnapshotsLocation();

private:

	/// XML file name
	QString m_sXmlFileName;

	/// Value of attribute "xsi:noNamespaceSchemaLocation" of XML model
	QString m_sSchema;

	/// Tree of saved states
	CSavedStateTree *m_SST;

private:

	/**
	 * Delete all children of element
	 *
	 * @author maximki
	 * @param parent of the deleted elements
	 * @return nothing
	 */
	void ClearTreeChild(CSavedStateTree *node);

	QString m_sSnapshotFolder;
};
