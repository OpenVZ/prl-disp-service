/////////////////////////////////////////////////////////////////////////////
///
///	@file PrlApiBasicsTest.h
///
///	This file is the part of parallels public SDK library tests suite.
///	Tests fixture class for testing SDK API common basics elements.
///
///	@author sandro
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
/////////////////////////////////////////////////////////////////////////////
#ifndef PrlApiBasicsTest_H
#define PrlApiBasicsTest_H

#include <QtTest/QtTest>

class PrlApiBasicsTest : public QObject
{

Q_OBJECT

private slots:
	void testCreateStringsList();
	void testCreateStringsListOnNullBufferPointer();
	void testAddItemToStringsList();
	void testAddItemToStringsListOnNullListHandle();
	void testAddItemToStringsListOnWrongListHandle();
	void testAddItemToStringsListOnNullStringElement();
	void testGetStringsListItemsCount();
	void testGetStringsListItemsCountOnNullListHandle();
	void testGetStringsListItemsCountOnWrongListHandle();
	void testGetStringsListItemsCountOnNullResultBuffer();
	void testRemoveItemFromStringsList();
	void testRemoveItemFromStringsListOnNullListHandle();
	void testRemoveItemFromStringsListOnWrongListHandle();
	void testRemoveItemFromStringsListOnOutOfRangeIndex();
	void testRemoveItemFromStringsListOnOutOfRangeIndex2();
	void testGetItemFromStringsList();
	void testGetItemFromStringsListOnNullListHandle();
	void testGetItemFromStringsListOnWrongListHandle();
	void testGetItemFromStringsListOnNullBufferSizeVariable();
	void testGetItemFromStringsListOnOutOfRangeIndex();
	void testGetItemFromStringsListOnOutOfRangeIndex2();
	void testGetMessageType();
	void testMsgCanBeIgnored();
	void testCreateHandlesList();
	void testCreateHandlesListOnNullBufferPointer();
	void testAddItemToHandlesList();
	void testAddItemToHandlesListOnNullListHandle();
	void testAddItemToHandlesListOnWrongListHandle();
	void testAddItemToHandlesListOnInvalidItemHandle();
	void testAddItemToHandlesListOnRemovedItemHandle();
	void testGetHandlesListItemsCount();
	void testGetHandlesListItemsCountOnNullListHandle();
	void testGetHandlesListItemsCountOnWrongListHandle();
	void testGetHandlesListItemsCountOnNullResultBuffer();
	void testRemoveItemFromHandlesList();
	void testRemoveItemFromHandlesListOnNullListHandle();
	void testRemoveItemFromHandlesListOnWrongListHandle();
	void testRemoveItemFromHandlesListOnOutOfRangeIndex();
	void testRemoveItemFromHandlesListOnOutOfRangeIndex2();
	void testGetItemFromHandlesList();
	void testGetItemFromHandlesListOnNullListHandle();
	void testGetItemFromHandlesListOnWrongListHandle();
	void testGetItemFromHandlesListOnNullResultBuffer();
	void testGetItemFromHandlesListOnOutOfRangeIndex();
	void testGetItemFromHandlesListOnOutOfRangeIndex2();
	void testApiGetVersion();
	void testApiGetVersionOnNullPointer();
	void testApiGetAppMode();
	void testApiGetAppModeOnNullPointer();
	void testGetErrDescriptionForSpecificErrorCode();
	void testGetErrDescription_ForSingularPluralParams();
	void testCreateOpTypeList();
	void testCreateOpTypeListOnNullBufferPointer();
	void testCreateOpTypeListOnWrongDataSize();
	void testAddItemToOpTypeList();
	void testAddItemToOpTypeListOnNullListHandle();
	void testAddItemToOpTypeListOnWrongListHandle();
	void testAddItemToOpTypeListOnNullPointer();
	void testGetOpTypeListItemsCount();
	void testGetOpTypeListItemsCountOnNullListHandle();
	void testGetOpTypeListItemsCountOnWrongListHandle();
	void testGetOpTypeListItemsCountOnNullResultBuffer();
	void testRemoveItemFromOpTypeList();
	void testRemoveItemFromOpTypeListOnNullListHandle();
	void testRemoveItemFromOpTypeListOnWrongListHandle();
	void testRemoveItemFromOpTypeListOnOutOfRangeIndex();
	void testRemoveItemFromOpTypeListOnOutOfRangeIndex2();
	void testGetItemFromOpTypeList();
	void testGetItemFromOpTypeListOnNullListHandle();
	void testGetItemFromOpTypeListOnWrongListHandle();
	void testGetItemFromOpTypeListOnNullBufferPointer();
	void testGetItemFromOpTypeListOnOutOfRangeIndex();
	void testGetItemFromOpTypeListOnOutOfRangeIndex2();
	void testOpTypeListOnBoundValues();
	void testOpTypeListGetTypeSize();
	void testOpTypeListGetTypeSizeOnNullListHandle();
	void testOpTypeListGetTypeSizeOnWrongListHandle();
	void testOpTypeListGetTypeSizeOnNullPointer();
	void testGetSupportedOsesTypes();
	void testGetSupportedOsesTypesOnWrongParams();
	void testGetSupportedOsesVersions();
	void testGetSupportedOsesVersionsOnWrongParams();
	void testGetDefaultOsVersion();
	void testGetDefaultOsVersionOnWrongParams();
};

#endif

