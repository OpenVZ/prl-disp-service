////////////////////////////////////////////////////////////////////////////////
///
/// @file SavedStateTree.h
///
/// This class implements a tree of checkpoints
///
/// @author MaximKi
/// @owner SergeyM
///
/// Copyright (c) 2005-2015 Parallels IP Holdings GmbH
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
/// Our contact details: Parallels IP Holdings GmbH, Vordergasse 59, 8200
/// Schaffhausen, Switzerland.
///
////////////////////////////////////////////////////////////////////////////////

#ifndef SAVEDSTATETREE_H
#define SAVEDSTATETREE_H


#include <QObject>
#include <QDomDocument>
#include <QString>
#include <QList>
#include "SavedState.h"
#include "SavedStatesModel.h"

/**
 * Description of the main part of the saved states struct.
 * Class describe properties of snapshot tree.
 *
 * @author maximki
 */
class CSavedStateTree : public CSavedState
{
public:

	/// Standard class constructor
	CSavedStateTree();

	/// Standard class destructor
	~CSavedStateTree();

	/**
	 * Get root element
	 *
	 * @author maximki
	 * @return reference to root element
	 */
	CSavedStateTree * GetRoot();

	/**
	 * Get parent element for current node
	 *
	 * @author maximki
	 * @return reference to parent element
	 */
	CSavedStateTree * GetParent();

	/**
	 * Set parent element for current node
	 *
	 * @author maximki
	 * @param reference to parent element
	 * @return nothing
	 */
	void SetParent(CSavedStateTree *parent);

	/**
	 * Get next sibling element
	 *
	 * @author maximki
	 * @return reference to sibling element
	 */
	CSavedStateTree * GetNextSibling();

	/**
	 * Get previous sibling element
	 *
	 * @author maximki
	 * @return reference to sibling element
	 */
	CSavedStateTree * GetPrevSibling();

	/**
	 * Get child for current element
	 *
	 * @author maximki
	 * @param zero-based child index
	 * @return reference to child element
	 */
	CSavedStateTree * GetChild(const int index);

	/**
	 * Get list of children for current element
	 *
	 * @author maximki
	 * @return list of children elements
	 */
	QList <CSavedStateTree *> *GetChilds();

	/**
	 * Add child element to current element
	 *
	 * @author maximki
	 * @param reference to child element
	 * @return indicator of operation success
	 */
	bool AddChild(CSavedStateTree *child);

	/**
	 * Find saved state element in tree by it's guid
	 *
	 * @author maximki
	 * @param guid of required saved state
	 * @return reference to found element
	 */
	CSavedStateTree * FindByUuid(QString sGuid);

	/**
	 * Find and delete current node from saved states tree
	 *
	 * @author maximki
	 * @return nothing
	 */
	void ClearCurrentNodes();

	/**
	 * Set/unset this element as a current node
	 *
	 * @author maximki
	 * @param true/false; make this node as a current snapshot or not
	 * @return nothing
	 */
	void SetCurrent(bool Current);

	/**
	 * Is this record describe "Head" node?
	 *
	 * @author maximki
	 */
	bool IsHead() { return GetParent() == NULL ? true : false; }

	/**
	 * Get count of children elements
	 *
	 * @author maximki
	 * @return count of children
	 */
	int GetChildCount();

	/**
	 * Find current node in the tree
	 *
	 * @author maximki
	 * @param reference to start element of search
	 * @return reference to found element
	 */
	CSavedStateTree * FindCurrentNode(CSavedStateTree * node);

	/**
	 * Find you are here node in the tree
	 *
	 * @author maximki
	 * @param reference to start element of search
	 * @return reference to found element
	 */
	CSavedStateTree * FindYouAreHereNode(CSavedStateTree * node);

	/**
	 * Get XML representation of the object
	 *
	 * @author maximki
	 * @param reference to QDomDocument element
	 * @param serialize option
	 * @return DOM XML struct
	 */
	QDomElement GetXml(QDomDocument* parent_doc, bool all_save = false);

	/**
	 * Read XML and setup internal model according to it
	 *
	 * @author maximki
	 * @param reference to first element of XML model
	 * @return result of XML parsing
	 */
	SnapshotParser::SnapshotReturnCode ReadXml(QDomElement* first_element);

private:

	/// Reference to parent element
	CSavedStateTree *m_SSTparent;

	/// List of clildren elements
	QList <CSavedStateTree *> m_childNodes;

protected:

	/**
	 * Delete all children elements
	 *
	 * @author maximki
	 * @param saved states tree parent element
	 * @return nothing
	 */
	void ClearTreeChild(CSavedStateTree *node);

	/**
	 * Find saved state element in tree by it's guid
	 * Recursive function.
	 * Offered by FindByUuid()
	 *
	 * @author maximki
	 * @param reference to saved states tree element
	 * @param guid of required saved state
	 * @return
	 */
	CSavedStateTree * RecursiveFindByUuid(CSavedStateTree *node, QString sGuid);

	/**
	 * Create class element
	 * Work with XML
	 *
	 * @author maximki
	 * @param reference to QDomDocument element
	 * @return QDomElement element
	 */
	QDomElement CreateClassElement(QDomDocument* parent_doc);

	/**
	 * Append class-specific properties
	 * Work with XML
	 *
	 * @author maximki
	 * @param reference to QDomDocument element
	 * @param reference to parent QDomElement element
	 * @param reference to current saved states tree element
	 * @return nothing
	 */
	void AppendClassProperties(QDomDocument* parent_doc,
							   QDomElement* parent_element,
							   CSavedStateTree *current_node,
							   bool all_save = false);

	/**
	 * Parse class-specific properties
	 * Work with XML
	 *
	 * @author maximki
	 * @param reference to QDomElement
	 * @return result code of operation
	 */
	SnapshotParser::SnapshotReturnCode ParseClassProperties(QDomElement* first_element);

	/**
	 * Recursive parse class-specific properties
	 * Work with XML
	 *
	 * @author maximki
	 * @param reference to QDomElement
	 * @param reference to saved states tree element
	 * @return result code of operation
	 */
	SnapshotParser::SnapshotReturnCode RecursiveParseClassProperties(QDomElement* parent_element, CSavedStateTree *SSTcurrent);

	/**
	 * Clean up class-specific properties
	 *
	 * @author maximki
	 * @return nothing
	 */
	void CleanupClassProperties();

	/**
	 * Find and delete current node from saved states tree
	 * Recursive function.
	 * Offered by ClearCurrentNodes()
	 *
	 * @author maximki
	 * @param element of saved states tree
	 * @return nothing
	 */
	void RecursiveClearCurrentNodes(CSavedStateTree *node);
};

#endif //SAVEDSTATETREE_H
