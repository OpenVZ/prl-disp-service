////////////////////////////////////////////////////////////////////////////////
///
/// @file SavedStateStore.cpp
///
/// This class implements a description of checkpoint
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

#include "SavedStateStore.h"
#include <QFile>
#include <QTextStream>
#include <QDateTime>
//. #include "AppUtils.h"


/**
 * Class constructor
 *
 * @author maximki
 */
CSavedStateStore::CSavedStateStore()
{
	m_sXmlFileName = "";
	m_SST = NULL;
} // CSavedStateStore::CSavedStateStore()


/**
 * Class constructor
 *
 * @author maximki
 * @param xml file name
 */
CSavedStateStore::CSavedStateStore(const QString sXmlFileName)
{
	m_sXmlFileName = "";
	m_SST = NULL;

	// Load shapshot tree from file
	Load(sXmlFileName);
} // CSavedStateStore::CSavedStateStore()

CSavedStateStore::~CSavedStateStore()
{
	if ( m_SST )
		ClearSavedStateTree();
}

/**
 * Find currently active saved state
 *
 * @author maximki
 * @return reference to current snapshot node
 */
CSavedStateTree * CSavedStateStore::FindCurrentSnapshot()
{
	return m_SST->FindCurrentNode(m_SST);
} // CSavedStateStore::FindCurrentSnapshot()


/**
 * Create new snapshot node
 *
 * @author maximki
 * @param description of new saved state element
 * @return operation result
 */
SnapshotParser::SnapshotReturnCode CSavedStateStore::CreateSnapshot(CSavedState state)
{
	// Create new node element
	CSavedStateTree *NewStateNode = new CSavedStateTree;

	// Fill property list
	NewStateNode->SetGuid(state.GetGuid());
	NewStateNode->SetName(state.GetName());
	NewStateNode->SetCreator(state.GetCreator());
	NewStateNode->SetCreateTime(state.GetCreateTime());
	NewStateNode->SetScreenShot(state.GetScreenShot());
	NewStateNode->SetDescription(state.GetDescription());
	NewStateNode->SetVmState(state.GetVmState());

	// Find parent snapshot
	CSavedStateTree *parent = FindCurrentSnapshot();

	if (parent)
	{
		// Add new snapshot record to store
		if (!parent->AddChild(NewStateNode))
		{
			delete NewStateNode;
			return SnapshotParser::CannotCreateSnapshot;
		}
	}
	else
	{
		// Create 'Head' element
		CSavedStateTree *MainRootNode = new CSavedStateTree;

		MainRootNode->AddChild(NewStateNode);

		// Create first element
		m_SST = MainRootNode;
	}

	// Set new snapshot as a current snapshot
	NewStateNode->SetCurrent(true);

	return SnapshotParser::RcSuccess;
} // CSavedStateStore::CreateSnapshot()


/**
 * Delete shapshot node
 *
 * @author maximki
 * @param deleted element
 * @return operation result
 */
SnapshotParser::SnapshotReturnCode CSavedStateStore::DeleteNode(CSavedStateTree *node)
{
	// If node does not exist or if node is head element we cannot delete it
	if (!node || !node->GetParent())
		return SnapshotParser::CannotDeleteSnapshot;

	int i;
	int iNodeCount = -1;
	int iNodeInsertPos;
	bool bIsNodeCurrent = node->IsCurrent();

	// Delete *node element from parent childlist
	for (i = 0; i < node->GetParent()->GetChildCount(); i++)
	{
		if ( node->GetParent()->GetChild(i) == node )
		{
			iNodeCount = i;
			node->GetParent()->GetChilds()->removeAt(iNodeCount);
			break;
		}
	}

	node->SetCurrent(false);

	// Reset parent element for children of deleted node
	iNodeInsertPos = iNodeCount > 0 ? iNodeCount : 0;
	for (i = 0; i < node->GetChildCount(); i++)
	{
		node->GetChild(i)->SetParent(node->GetParent());
		node->GetParent()->GetChilds()->insert(iNodeInsertPos++, node->GetChild(i));
	}
	if (bIsNodeCurrent)
	{
		if ( node->GetParent()->IsHead() && (node->GetParent()->GetChildCount() == 0) )
		{
			// There is no snapshots. Delete head element
			CSavedStateTree *tmpCur = node->GetParent();
			delete tmpCur;
			tmpCur = m_SST = NULL;
		}
		else
			node->GetParent()->SetCurrent(true);
/*
##Q##
Zakommentirovano, t.k. teper` pri udalenii potomka root'a, kotoriy bil "current",
"current'om" stanovitsiya sam root. Ran`she "current'om" stanovilsya drugoi potomok root'a.
Chtobi vernut` vse vzad nado udalit` 9 strok naverhu i raskomentirovat` etot kod.

		// If *node's parent element isn't head set it 'current'
		if (!node->GetParent()->IsHead())
			node->GetParent()->SetCurrent(true);
		else
		{
			// *node's parent element is head --> set next created(after *node) *node's child as 'current'
			CSavedStateTree *tmpCur = node->GetChild(0);
			for (i = 1; i < node->GetChildCount(); i++)
			{
				if ( QDateTime::fromString(node->GetChild(i)->GetCreateTime()) < QDateTime::fromString(tmpCur->GetCreateTime()) )
					tmpCur = node->GetChild(i);
			}
			// Deleted node has child that can be set 'current'
			if (tmpCur)
				tmpCur->SetCurrent(true);
			else
			{
				// If Head element have another child set it 'current'
				if (node->GetParent()->GetChildCount() > 0)
				{
					node->GetParent()->GetChild(0)->SetCurrent(true);
				}
				else
				{
					// There is no snapshots. Delete head element
					tmpCur = node->GetParent();
					delete tmpCur;
					tmpCur = m_SST = NULL;
				}
			}
		}
*/	}

	delete node;
	node = NULL;

	// If there is no snapshots and snapshot tree is only root element which is "current"
	// delete snapshot tree.
	if ( m_SST && (m_SST->GetChildCount() == 1) && m_SST->IsCurrent() )
	{
		m_SST->SetCurrent(false);
		delete m_SST;
		m_SST = NULL;
	}

	return SnapshotParser::RcSuccess;
} // CSavedStateStore::DeleteNode()


/**
 * Delete shapshot node
 *
 * @author maximki
 * @param saved state guid
 * @return operation result
 */
SnapshotParser::SnapshotReturnCode CSavedStateStore::DeleteNode(QString sGuid)
{
	CSavedStateTree *DeletingNode = m_SST->FindByUuid(sGuid);
	return DeleteNode(DeletingNode);
} // CSavedStateStore::DeleteNode()


/**
 * Delete shapshot branch
 *
 * @author maximki
 * @param root of the deleted elements
 * @return operation result
 */
SnapshotParser::SnapshotReturnCode CSavedStateStore::DeleteBranch(CSavedStateTree *node)
{
	if (!node)
		return SnapshotParser::CannotDeleteSnapshot;

	bool bContainsCurrentNode = node->FindCurrentNode(node) == NULL ? false : true;

	// Delete all children elements
	ClearTreeChild(node);

	if (node->GetParent())
	{
		// Delete *node element from it's parent childlist
		for (int i = 0; i < node->GetParent()->GetChildCount(); i++)
		{
			if ( node->GetParent()->GetChild(i) == node )
			{
				node->GetParent()->GetChilds()->removeAt(i);
				break;
			}
		}

		// Set parent node as 'current' node if one of this node's children was 'current'
		if (bContainsCurrentNode)
		{
			if ( node->GetParent()->IsHead() && (node->GetParent()->GetChildCount() == 0) )
			{
				// There is no snapshots. Delete head element
				CSavedStateTree *tmpCur = node->GetParent();
				delete tmpCur;
				tmpCur = m_SST = NULL;
			}
			else
				node->GetParent()->SetCurrent(true);
/*
##Q##
Zakommentirovano, t.k. teper` pri udalenii potomka root'a, kotoriy bil "current",
"current'om" stanovitsiya sam root. Ran`she "current'om" stanovilsya drugoi potomok root'a.
Chtobi vernut` vse vzad nado udalit` 9 strok naverhu i raskomentirovat` etot kod.

			// If *node's parent element isn't 'head' set it 'current'
			if (node->GetParent()->GetParent())
				node->GetParent()->SetCurrent(true);
			else
			{
				// *node's parent element is head --> set next created(after *node) *node's child as 'current'
				CSavedStateTree *tmpCur = node->GetChild(0);
				for (int i = 1; i < node->GetChildCount(); i++)
				{
					if ( node->GetChild(i)->GetCreateTime() < tmpCur->GetCreateTime() )
						tmpCur = node->GetChild(i);
				}

				// Deleted node has child that can be set 'current'
				if (tmpCur)
					tmpCur->SetCurrent(true);
				else
				{
					// If Head element have another child set it 'current'
					if (node->GetParent()->GetChildCount() > 0)
					{
						node->GetParent()->GetChild(0)->SetCurrent(true);
					}
					else
					{
						// There is no snapshots. Delete head element
						tmpCur = node->GetParent();
						delete tmpCur;
						tmpCur = m_SST = NULL;
					}
				}
			}
*/		}
	}

	// If node is a root element we clear reference to root element
	if (node == m_SST)
	{
		m_SST = NULL;
	}

	// Delete node
	delete node;
	node = NULL;

	// If there is no snapshots and snapshot tree is only root element which is "current"
	// delete snapshot tree.
	if ( m_SST && (m_SST->GetChildCount() == 1) && m_SST->IsCurrent() )
	{
		m_SST->SetCurrent(false);
		delete m_SST;
		m_SST = NULL;
	}

	return SnapshotParser::RcSuccess;
} // CSavedStateStore::DeleteBranch()


/**
 * Delete shapshot branch
 *
 * @author maximki
 * @param saved state guid
 * @return operation result
 */
SnapshotParser::SnapshotReturnCode CSavedStateStore::DeleteBranch(QString sGuid)
{
	CSavedStateTree *DeletingBranchRoot = m_SST->FindByUuid(sGuid);
	return DeleteBranch(DeletingBranchRoot);
} // CSavedStateStore::DeleteBranch()


/**
 * Load shapshot tree from file
 *
 * @author maximki
 * @param file name
 * @return operation result
 */
SnapshotParser::SnapshotReturnCode CSavedStateStore::Load(const QString sXmlFileName)
{
	DeleteBranch(m_SST);

	m_sXmlFileName = sXmlFileName;
	QDomDocument doc;
    QFile file(sXmlFileName);

    if (!file.open(QIODevice::ReadOnly))
        return SnapshotParser::BadFileName;

	// Set content of xml model
	if (!doc.setContent(&file))
	{
        file.close();
        return SnapshotParser::BadFileName;
    }
    file.close();

    QDomElement docElem = doc.documentElement();

	// Root element have to have special name
	if (docElem.tagName() != XML_SS_CONFIG_EL_ROOT)
		return SnapshotParser::BadSnapshotTree;

	int iRs = SnapshotParser::RcSuccess;

	// Get first child element. This is root element in snapshot tree
	QDomNode docNodeItem = docElem.firstChild();
	if (!docNodeItem.isNull()) {
		if (docNodeItem.isElement()) {
			QDomElement docElemItem = docNodeItem.toElement();
			m_SST = new CSavedStateTree;
			iRs = m_SST->ReadXml(&docElemItem);
		}
		else
		{
			iRs = SnapshotParser::BadSnapshotTree;
		}
	}
	else
	{
		iRs = SnapshotParser::EmptySnapshotTree;
	}

	// Return result of xml parsing
	return (SnapshotParser::SnapshotReturnCode)iRs;
} // CSavedStateStore::Load()

/**
 * Load shapshot tree from file
 *
 * @author maximki
 * @param file name
 * @return operation result
 */
SnapshotParser::SnapshotReturnCode CSavedStateStore::Load(const QString & content,
	const QString & strSnapshotFolder)
{
	DeleteBranch(m_SST);

	m_sSnapshotFolder = strSnapshotFolder;
	QDomDocument doc;

	// Set content of xml model
	if (!doc.setContent(content))
	{
        	return SnapshotParser::BadFileName;
    	}

	QDomElement docElem = doc.documentElement();

	// Root element have to have special name
	if (docElem.tagName() != XML_SS_CONFIG_EL_ROOT)
		return SnapshotParser::BadSnapshotTree;

	int iRs = SnapshotParser::RcSuccess;

	// Get first child element. This is root element in snapshot tree
	QDomNode docNodeItem = docElem.firstChild();
	if (!docNodeItem.isNull()) {
		if (docNodeItem.isElement()) {
			QDomElement docElemItem = docNodeItem.toElement();
			m_SST = new CSavedStateTree;
			iRs = m_SST->ReadXml(&docElemItem);
		}
		else
		{
			iRs = SnapshotParser::BadSnapshotTree;
		}
	}
	else
	{
		iRs = SnapshotParser::EmptySnapshotTree;
	}

	// Return result of xml parsing
	return (SnapshotParser::SnapshotReturnCode)iRs;
}

/**
 * Save shapshot tree to file
 *
 * @author maximki
 * @return operation result
 */
SnapshotParser::SnapshotReturnCode CSavedStateStore::Save(bool all_save)
{
	return Save( m_sXmlFileName, all_save );
} // CSavedStateStore::Save()


SnapshotParser::SnapshotReturnCode CSavedStateStore::Save(const QString& sXmlFileName, bool all_save)
{
	// Try to open file for writing
	QFile fileWrite(sXmlFileName);
	if (!fileWrite.open(QIODevice::WriteOnly))
		return SnapshotParser::BadFileName;

	return Save( fileWrite, all_save );
} // CSavedStateStore::Save()

/**
 * Save shapshot tree to file
 *
 * @author maximki
 * @param file name
 * @return operation result
 */
SnapshotParser::SnapshotReturnCode CSavedStateStore::Save(QIODevice& ioDevice, bool all_save)
{
	QDomDocument xmlDummyParentDoc;

	// Create root element
	QDomElement xmlDummyParentElement = xmlDummyParentDoc.createElement( XML_SS_CONFIG_EL_ROOT );

	xmlDummyParentDoc.appendChild( xmlDummyParentElement );

	// Write XML to file
	try
	{
		QTextStream out( &ioDevice );
		out << XML_SS_CONFIG_HEADER << endl;
		QDomElement xmlSnapshots;
		xmlSnapshots = m_SST->GetXml( &xmlDummyParentDoc, all_save );
		// Add saved states tree if it isn't clear
		if (xmlSnapshots.hasChildNodes())
			xmlDummyParentElement.appendChild( xmlSnapshots );

		xmlDummyParentDoc.save( out, 4 );
		out.flush();
	}
	catch (...)
	{
		return SnapshotParser::CannotWriteFile;
	}

	//. SetFreeShareOnFile(sXmlFileName);
	return SnapshotParser::RcSuccess;
} // CSavedStateStore::Save()


/**
 * Clear tree of saved states
 *
 * @author maximki
 * @return nothing
 */
void CSavedStateStore::ClearSavedStateTree()
{
	// Get root element
	CSavedStateTree *node = m_SST->GetRoot();
	m_SST = NULL;

	// Delete children elements
	ClearTreeChild(node);

	// Delete root elements
	delete node;
} // CSavedStateStore::ClearSavedStateTree()


bool CSavedStateStore::IsEmpty()
{
	return m_SST == NULL ? true : false;
}


/**
 * Delete all children of element
 *
 * @author maximki
 * @param parent of the deleted elements
 * @return nothing
 */
void CSavedStateStore::ClearTreeChild(CSavedStateTree *node)
{
	if ( (!node) && (node->GetChildCount() == 0) )
		return;

	for (int i = 0; i < node->GetChildCount(); i++)
	{
		// Recursive call
		CSavedStateTree *pChild = node->GetChild(i);
		ClearTreeChild( pChild );
		delete pChild;
	}

	node->GetChilds()->clear();

} // CSavedStateStore::ClearTreeChild()

QString CSavedStateStore::GetLoadedFileName()
{
	return m_sXmlFileName;
}

QString CSavedStateStore::GetSnapshotsLocation()
{
	return m_sSnapshotFolder;
}
