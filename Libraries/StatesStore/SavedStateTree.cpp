////////////////////////////////////////////////////////////////////////////////
///
/// @file SavedStateTree.cpp
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

#include <QDomCDATASection>
#include "SavedStateTree.h"

////////////////////////////////////////////////////////////////////////////////

CSavedStateTree::CSavedStateTree() : CSavedState()
{
	m_SSTparent = NULL;
	m_childNodes.clear();
}

CSavedStateTree::~CSavedStateTree()
{
}


/**
 * Get root element
 *
 * @author maximki
 * @return reference to root element
 */
CSavedStateTree * CSavedStateTree::GetRoot()
{
	CSavedStateTree *pSSTtmp = this;
	while (pSSTtmp->m_SSTparent)
		pSSTtmp = pSSTtmp->GetParent();

	return pSSTtmp;
} // CSavedStateTree::GetRoot()


/**
 * Get parent element for current node
 *
 * @author maximki
 * @return reference to parent element
 */
CSavedStateTree * CSavedStateTree::GetParent()
{
	return m_SSTparent;
} // CSavedStateTree::GetParent()


/**
 * Set parent element for current node
 *
 * @author maximki
 * @param reference to parent element
 * @return nothing
 */
void CSavedStateTree::SetParent(CSavedStateTree *parent)
{
	m_SSTparent = parent;
} // CSavedStateTree::SetParent()


/**
 * Get next sibling element
 *
 * @author maximki
 * @return reference to sibling element
 */
CSavedStateTree * CSavedStateTree::GetNextSibling()
{
	// If node have not parent it don't have sibling elements
	if (!GetParent())
		return NULL;

	int i;
	// Find this node in the list of it's own parent children
	for (i = 0; i < GetParent()->GetChildCount() && GetParent()->GetChild(i) != this; i++)
            {}

	if (GetParent()->GetChild(i) == this)
		// Return reference to sibling element
		return GetParent()->GetChild(i+1);
	else
		return NULL;
} // CSavedStateTree::GetNextSibling()


/**
 * Get previous sibling element
 *
 * @author maximki
 * @return reference to sibling element
 */
CSavedStateTree * CSavedStateTree::GetPrevSibling()
{
	// If node have not parent it don't have sibling elements
	if (!GetParent())
		return NULL;

	int i;
	// Find this node in the list of it's own parent children
	for (i = 0; i < GetParent()->GetChildCount() && GetParent()->GetChild(i) != this; i++)
            {}

	if (GetParent()->GetChild(i) == this)
		// Return reference to sibling element
		return GetParent()->GetChild(i-1);
	else
		return NULL;
} // CSavedStateTree::GetPrevSibling()


/**
 * Get child for current element
 *
 * @author maximki
 * @param zero-based child index
 * @return reference to child element
 */
CSavedStateTree * CSavedStateTree::GetChild(const int index)
{
	if ( (index < 0) || (index > GetChildCount() - 1) )
		return NULL;

	return m_childNodes[index];
} // CSavedStateTree::GetChild()


/**
 * Get list of children for current element
 *
 * @author maximki
 * @return list of children elements
 */
QList <CSavedStateTree *> *CSavedStateTree::GetChilds()
{
	return &m_childNodes;
} // CSavedStateTree::GetChilds()


/**
 * Add child element to current element
 *
 * @author maximki
 * @param reference to child element
 * @return indicator of operation success
 */
bool CSavedStateTree::AddChild(CSavedStateTree *child)
{
	int iInsertPos;

	// this node must not be a 'CurrentState' node
	if (!child || IsCurrentState())
		return false;

	// Calculation position for inserting new element
	if (IsCurrent() && !child->IsCurrentState())
		iInsertPos = GetChildCount() - 1;
	else
		iInsertPos = GetChildCount();

	// Insert new element
	m_childNodes.insert(iInsertPos, child);
	child->SetParent(this);

	return true;
} // CSavedStateTree::AddChild()


/**
 * Find saved state element in tree by it's guid
 *
 * @author maximki
 * @param guid of required saved state
 * @return reference to found element
 */
CSavedStateTree * CSavedStateTree::FindByUuid(QString sGuid)
{
	CSavedStateTree *result = NULL;

	// Recursive scan of saved states tree
	result = RecursiveFindByUuid(GetRoot(), sGuid);

	return result;
} // CSavedStateTree::FindByUuid()


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
CSavedStateTree * CSavedStateTree::RecursiveFindByUuid(CSavedStateTree *node, QString sGuid)
{
	// Node must exist
	if (!node)
		return NULL;

	// Analyse current node
	if (node->GetGuid() == sGuid)
		return node;

	CSavedStateTree *result = NULL;
	for (int i = 0; i < node->GetChildCount(); i++)
	{
		// Recursive scan of current element child
		result = RecursiveFindByUuid(node->GetChild(i), sGuid);
		if ( result )
			return result;
	}

	return result;
} // SavedStateTree::RecursiveSearchByUuid()


/**
 * Find and delete current node from saved states tree
 *
 * @author maximki
 * @return nothing
 */
void CSavedStateTree::ClearCurrentNodes()
{
	CSavedStateTree *SSTroot;
	SSTroot = GetRoot();
	RecursiveClearCurrentNodes(SSTroot);
}


/**
 * Find and delete current node from saved states tree
 * Recursive function.
 * Offered by ClearCurrentNodes()
 *
 * @author maximki
 * @param element of saved states tree
 * @return nothing
 */
void CSavedStateTree::RecursiveClearCurrentNodes(CSavedStateTree *node)
{
	// Node must exist
	if (!node)
		return;

	node->SetCurrent(false);

	for (int i = 0; i < node->m_childNodes.count(); i++)
	{
		// Recursive scan of current element child
		RecursiveClearCurrentNodes(node->GetChild(i));
	}
}


/**
 * Set/unset this element as a current node
 *
 * @author maximki
 * @param true/false; make this node as a current snapshot or not
 * @return nothing
 */
void CSavedStateTree::SetCurrent(bool Current)
{
	if ( IsCurrentState() )
		return;

	// Set this node as 'current'
	if (Current)
	{
		// Head element cannot be a 'current' element
		// Do nothing
		//// Commented. For details see ##Q## in CSavedStateStore.cpp
////	if (IsHead())
////		return;

		// Delete all 'current' nodes
		ClearCurrentNodes();

		// Create new 'You are here' node
		CSavedStateTree *CurrentStateNode = new CSavedStateTree;
		CurrentStateNode->SetCurrentState(true);
		CSavedState::SetCurrent(true);

		// Append 'You are here' node to this node
		AddChild(CurrentStateNode);
	}
	else
	{
		// Set this node as 'current'
		if (GetChildCount() > 0)
		{
			if ( GetChild( GetChildCount() - 1 )->IsCurrentState() )
			{
				// Deleting 'You are here' node
				CSavedStateTree *CurrentState = GetChild( GetChildCount() - 1 );
				GetChilds()->removeLast();
				delete CurrentState;
			}
		}
		CSavedState::SetCurrent(false);
	}
} // CSavedStateTree::SetCurrent()


/**
 * Get count of children elements
 *
 * @author maximki
 * @return count of children
 */
int CSavedStateTree::GetChildCount()
{
	return m_childNodes.count();
} // CSavedStateTree::GetChildCount()


/**
 * Find current node in the tree
 *
 * @author maximki
 * @param reference to start element of search
 * @return reference to found element
 */
CSavedStateTree * CSavedStateTree::FindCurrentNode(CSavedStateTree * node)
{
	if (!node)
		return NULL;

	// Analyse current element
	if (node->IsCurrent())
		return node;

	CSavedStateTree *result = NULL;
	for (int i = 0; i < node->m_childNodes.count(); i++)
	{
		// Recursive scan of current element child
		result = FindCurrentNode(node->GetChild(i));
		if ( result )
			return result;
	}

	return result;
}


/**
 * Get XML representation of the object
 *
 * @author maximki
 * @param reference to QDomDocument element
 * @return DOM XML struct
 */
QDomElement CSavedStateTree::GetXml(QDomDocument* parent_doc, bool all_save)
{
	// Create basic class element
	QDomElement xmlClassElement = CreateClassElement( parent_doc );

	// Append class-specific properties
	AppendClassProperties( parent_doc, &xmlClassElement, this, all_save);

	return xmlClassElement;

} // CSavedStateTree::getXml()


/**
 * Read XML and setup internal model according to it
 *
 * @author maximki
 * @param reference to first element of XML model
 * @return result of XML parsing
 */
SnapshotParser::SnapshotReturnCode CSavedStateTree::ReadXml(QDomElement* first_element)
{
	int i_rc;

	if( !first_element )
		return SnapshotParser::BadSnapshotTree;

	// Cleanup class properties
	CleanupClassProperties();

	// Parse class-specific properties
	i_rc = ParseClassProperties( first_element );
	if( !IS_OPERATION_SUCCEEDED( i_rc ) )
		return (SnapshotParser::SnapshotReturnCode)i_rc;

	return SnapshotParser::RcSuccess;

} // CSavedStateTree::readXml()


/**
 * Delete all children elements
 *
 * @author maximki
 * @param saved states tree parent element
 * @return nothing
 */
void CSavedStateTree::ClearTreeChild(CSavedStateTree *node)
{
	if ( (!node) || (node->GetChildCount() == 0) )
		return;

	// Recursive scan of current element child
	for (int i = 0; i < node->GetChildCount(); i++)
	{
		ClearTreeChild(node->GetChild(i));
		delete node->GetChild(i);
	}

	// As all children currently ware deleted we can clear node's children list
	node->GetChilds()->clear();

} // CSavedStateTree::ClearTreeChild()


/**
 * Create class element
 * Work with XML
 *
 * @author maximki
 * @param reference to QDomDocument element
 * @return QDomElement element
 */
QDomElement CSavedStateTree::CreateClassElement(QDomDocument* parent_doc)
{
	QDomElement xmlClassElement = parent_doc->createElement( XML_SS_CONFIG_EL_ITEM );

	return xmlClassElement;

} // CSavedStateTree::createClassElement()


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
void CSavedStateTree::AppendClassProperties(QDomDocument* parent_doc,
											QDomElement* parent_element,
											CSavedStateTree *current_node,
											bool bRuntimeSave )
{
	if (!current_node)
		return;

	// Do not add "You are here" node to xml model
	if ( current_node->IsCurrentState() )
		return;

	// Set attribute "snapshot guid"
	parent_element->setAttribute( XML_SS_CONFIG_AT_GUID, current_node->GetGuid() );

	// Set "current" attribute if necessary
	if ( current_node->IsCurrent() )
		parent_element->setAttribute( XML_SS_CONFIG_AT_CURRENT, XML_SS_CONFIG_VAL_IS_CURRENT );

	// Set attribute "vm state"
	switch (current_node->GetVmState())
	{
	case PVE::SnapshotedVmRunning :
		parent_element->setAttribute( XML_SS_CONFIG_AT_VMSTATE, XML_SS_CONFIG_VAL_VMSTATE_ON );
		break;
	case PVE::SnapshotedVmPaused :
		parent_element->setAttribute( XML_SS_CONFIG_AT_VMSTATE, XML_SS_CONFIG_VAL_VMSTATE_PAUSE );
		break;
	case PVE::SnapshotedVmSuspended :
		parent_element->setAttribute( XML_SS_CONFIG_AT_VMSTATE, XML_SS_CONFIG_VAL_VMSTATE_SUSPEND );
		break;
	default:
		parent_element->setAttribute( XML_SS_CONFIG_AT_VMSTATE, XML_SS_CONFIG_VAL_VMSTATE_OFF );
		break;
	}

	// append class-specific nodes

	// Create snap-shot name node
	QDomElement xmlNode_SnapShotName = parent_doc->createElement( XML_SS_CONFIG_EL_NAME );
	QDomText xmlNode_SnapShotNameText = parent_doc->createTextNode( current_node->GetName() );
	xmlNode_SnapShotName.appendChild(xmlNode_SnapShotNameText);
	parent_element->appendChild( xmlNode_SnapShotName );

	// Create snap-shot creating time node
	QDomElement xmlNode_SnapShotCreateTime = parent_doc->createElement( XML_SS_CONFIG_EL_CREATETIME );
	QDomText xmlNode_SnapShotCreateTimeText = parent_doc->createTextNode( current_node->GetCreateTime() );
	xmlNode_SnapShotCreateTime.appendChild(xmlNode_SnapShotCreateTimeText);
	parent_element->appendChild( xmlNode_SnapShotCreateTime );

	// Create snap-shot creator name node
	QDomElement xmlNode_SnapShotCreator = parent_doc->createElement( XML_SS_CONFIG_EL_CREATOR );
	QDomText xmlNode_SnapShotCreatorText = parent_doc->createTextNode( current_node->GetCreator() );
	xmlNode_SnapShotCreator.appendChild(xmlNode_SnapShotCreatorText);
	parent_element->appendChild( xmlNode_SnapShotCreator );

	// Create snap-shot screenshot file name node
	QDomElement xmlNode_SnapShotScreenShot = parent_doc->createElement( XML_SS_CONFIG_EL_SCREENSHOT );
	QDomText xmlNode_SnapShotScreenShotText = parent_doc->createTextNode( current_node->GetScreenShot() );
	xmlNode_SnapShotScreenShot.appendChild(xmlNode_SnapShotScreenShotText);
	parent_element->appendChild( xmlNode_SnapShotScreenShot );

	// Create snap-shot description node
	QDomElement xmlNode_SnapShotDescription = parent_doc->createElement( XML_SS_CONFIG_EL_DESCRIPTION );
	QDomCDATASection xmlNode_SnapShotDescriptionText = parent_doc->createCDATASection( current_node->GetDescription() );
	xmlNode_SnapShotDescription.appendChild(xmlNode_SnapShotDescriptionText);
	parent_element->appendChild( xmlNode_SnapShotDescription );

	if (bRuntimeSave)
	{
		QDomElement nodeRuntime = parent_doc->createElement( XML_SS_CONFIG_EL_RUNTIME );
		parent_element->appendChild( nodeRuntime );

#define ADD_TEXT_NODE( _parent_doc, _name, _val ) \
		{ \
			QDomElement node = _parent_doc->createElement( _name ); \
			QDomText text = _parent_doc->createTextNode( QString("%1").arg( _val ) ); \
			node.appendChild(text); \
			nodeRuntime.appendChild( node ); \
		}

		ADD_TEXT_NODE( parent_doc, XML_SS_CONFIG_EL_SIZE, current_node->GetRuntime().nSize );
		ADD_TEXT_NODE( parent_doc, XML_SS_CONFIG_EL_OS_VERSION, current_node->GetRuntime().nOsVersion );
// VirtualDisk commented out by request from CP team
//		ADD_TEXT_NODE( parent_doc, XML_SS_CONFIG_EL_UNFINISHED_OP, current_node->GetRuntime().nUnfinishedOpType );

#undef ADD_TEXT_NODE
	}

	for (int i = 0; i < current_node->GetChildCount(); i++)
	{
		QDomElement xmlNode_SnapShotItem = CreateClassElement(parent_doc);
		AppendClassProperties( parent_doc, &xmlNode_SnapShotItem, current_node->GetChild(i), bRuntimeSave );
		if (xmlNode_SnapShotItem.hasChildNodes())
			parent_element->appendChild( xmlNode_SnapShotItem );
	}
} // CSavedStateTree::AppendClassProperties()


/**
 * Parse class-specific properties
 * Work with XML
 *
 * @author maximki
 * @param reference to QDomElement
 * @return result code of operation
 */
SnapshotParser::SnapshotReturnCode CSavedStateTree::ParseClassProperties(QDomElement* first_element)
{
	int i_rc;

	// check parent element name
	if (first_element->tagName() != XML_SS_CONFIG_EL_ITEM)
		return SnapshotParser::BadSnapshotTree;

	i_rc = RecursiveParseClassProperties(first_element, this);
	if( !IS_OPERATION_SUCCEEDED( i_rc ) )
		return (SnapshotParser::SnapshotReturnCode)i_rc;

	return SnapshotParser::RcSuccess;
} // CSavedStateTree::ParseClassProperties()


/**
 * Recursive parse class-specific properties
 * Work with XML
 *
 * @author maximki
 * @param reference to QDomElement
 * @param reference to saved states tree element
 * @return result code of operation
 */
SnapshotParser::SnapshotReturnCode CSavedStateTree::RecursiveParseClassProperties(QDomElement* parent_element, CSavedStateTree *SSTcurrent)
{
	int i_rc;

	QString sAttrValue;

	if ( parent_element->tagName() != XML_SS_CONFIG_EL_ITEM )
		return SnapshotParser::BadSnapshotTree;

	// Get saved state guid
	sAttrValue = parent_element->attribute(XML_SS_CONFIG_AT_GUID);
	if ( sAttrValue.isEmpty() && !SSTcurrent->IsHead() )
		return SnapshotParser::BadSnapshotGuid;

	SSTcurrent->SetGuid(sAttrValue);

	// Find out is this snapshot 'current'
	sAttrValue = parent_element->attribute(XML_SS_CONFIG_AT_CURRENT);
	if (sAttrValue.isEmpty())
	{
		SSTcurrent->SetCurrent(false);
	}
	else
	{
		sAttrValue.toLower() == XML_SS_CONFIG_VAL_IS_CURRENT ? SSTcurrent->SetCurrent(true) : SSTcurrent->SetCurrent(false);
	}

	// Get vm state
	sAttrValue = parent_element->attribute(XML_SS_CONFIG_AT_VMSTATE);
	if (sAttrValue.toLower() == XML_SS_CONFIG_VAL_VMSTATE_ON)
	{
		SSTcurrent->SetVmState(PVE::SnapshotedVmRunning);
	}
	else if (sAttrValue.toLower() == XML_SS_CONFIG_VAL_VMSTATE_PAUSE)
	{
		SSTcurrent->SetVmState(PVE::SnapshotedVmPaused);
	}
	else if (sAttrValue.toLower() == XML_SS_CONFIG_VAL_VMSTATE_SUSPEND)
	{
		SSTcurrent->SetVmState(PVE::SnapshotedVmSuspended);
	}
	else
	{
		SSTcurrent->SetVmState(PVE::SnapshotedVmPoweredOff);
	}

	QDomElement elem;

	// Get saved state name
	elem = parent_element->firstChildElement();

	if (elem.isNull() || (elem.tagName() != XML_SS_CONFIG_EL_NAME))
	{
		return SnapshotParser::BadSnapshotName;
	}
	SSTcurrent->SetName(elem.text());

	// Get saved state create time
	elem = elem.nextSibling().toElement();
	if (elem.isNull() || (elem.tagName() != XML_SS_CONFIG_EL_CREATETIME))
	{
		return SnapshotParser::BadSnapshotCreateTime;
	}
	SSTcurrent->SetCreateTime(elem.text());

	// Get saved state creator
	elem = elem.nextSibling().toElement();
	if (elem.isNull() || (elem.tagName() != XML_SS_CONFIG_EL_CREATOR))
	{
		return SnapshotParser::BadSnapshotCreator;
	}
	SSTcurrent->SetCreator(elem.text());

	// Get saved state screenshot file name
	elem = elem.nextSibling().toElement();
	if (elem.isNull() || (elem.tagName() != XML_SS_CONFIG_EL_SCREENSHOT))
	{
		return SnapshotParser::BadSnapshotScreenshot;
	}
	SSTcurrent->SetScreenShot(elem.text());

	// Get saved state description
	elem = elem.nextSibling().toElement();
	if (elem.isNull() || (elem.tagName() != XML_SS_CONFIG_EL_DESCRIPTION))
	{
		return SnapshotParser::BadSnapshotDescription;
	}
	QDomCDATASection xmlCData = elem.firstChild().toCDATASection();
	if( xmlCData.isNull() )
		SSTcurrent->SetDescription( "" );
	else
		SSTcurrent->SetDescription( xmlCData.nodeValue() );

	// Try to load runtime info
	// NOTE: Here should be last entries
	elem = elem.nextSibling().toElement();
	if (!elem.isNull() && elem.tagName() == XML_SS_CONFIG_EL_RUNTIME)
	{
		CSavedState::Runtime rt = SSTcurrent->GetRuntime();

		QDomElement e = elem.firstChildElement();
		for( ; ! e.isNull(); e = e.nextSibling().toElement() )
		{
			if (e.tagName() == XML_SS_CONFIG_EL_SIZE)
				rt.nSize = e.text().toULongLong();
			else if (e.tagName() == XML_SS_CONFIG_EL_OS_VERSION)
				rt.nOsVersion = e.text().toUInt();
// VirtualDisk commented out by request from CP team
//			else if (e.tagName() == XML_SS_CONFIG_EL_UNFINISHED_OP )
//				rt.nUnfinishedOpType = (PROCESS_TYPE) e.text().toInt();
		}
		SSTcurrent->SetRuntime( rt );
		elem = elem.nextSibling().toElement();
	}

	// Get saved state children
	QList<CSavedStateTree *> childs;
	CSavedStateTree *child;
    while(!elem.isNull())
	{
		if ( elem.tagName() == XML_SS_CONFIG_EL_ITEM )
		{
			child = new CSavedStateTree;
			child->SetParent(SSTcurrent);
			SSTcurrent->AddChild(child);

			i_rc = RecursiveParseClassProperties(&elem, child);
 			if( !IS_OPERATION_SUCCEEDED( i_rc ) )
				return (SnapshotParser::SnapshotReturnCode)i_rc;
		}
		elem = elem.nextSibling().toElement();
	}

	return SnapshotParser::RcSuccess;
} // CSavedStateTree::RecursiveParseClassProperties()


/**
 * Clean up class-specific properties
 *
 * @author maximki
 * @return nothing
 */
void CSavedStateTree::CleanupClassProperties()
{
	ClearTreeChild(this);

	CleanupStateProperties();

} // CSavedStateTree::cleanupClassProperties()

CSavedStateTree * CSavedStateTree::FindYouAreHereNode(CSavedStateTree * node)
{
	if (!node)
		return NULL;

	// Analyse current element
	if (node->IsCurrentState())
		return node;

	CSavedStateTree *result = NULL;
	for (int i = 0; i < node->m_childNodes.count(); i++)
	{
		// Recursive scan of current element child
		result = FindYouAreHereNode(node->GetChild(i));
		if ( result )
			return result;
	}

	return result;
}
