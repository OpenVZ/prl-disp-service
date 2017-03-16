/////////////////////////////////////////////////////////////////////////////
///
///	@file PrlApiBasicsTest.cpp
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

#include "PrlApiBasicsTest.h"
#include "SDK/Wrappers/SdkWrap/SdkHandleWrap.h"
#include "Tests/CommonTestsUtils.h"
#include <prlcommon/Logging/Logging.h>
#include <prlcommon/Messaging/CVmEvent.h>

void PrlApiBasicsTest::testCreateStringsList()
{
	SdkHandleWrap hStringsList;
	CHECK_RET_CODE_EXP(PrlApi_CreateStringsList(hStringsList.GetHandlePtr()))
	CHECK_HANDLE_TYPE(hStringsList, PHT_STRINGS_LIST)
}

void PrlApiBasicsTest::testCreateStringsListOnNullBufferPointer()
{
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlApi_CreateStringsList(0), PRL_ERR_INVALID_ARG)
}

#define CHECK_LIST_SIZE(list_handle, expected_size)\
	{\
		PRL_UINT32 nActualSize;\
		CHECK_RET_CODE_EXP(PrlStrList_GetItemsCount(list_handle, &nActualSize))\
		QVERIFY(nActualSize == expected_size);\
	}

#define CHECK_ITEM_PRESENTS(list_handle, item_value)\
	{\
		PRL_UINT32 nListSize;\
		CHECK_RET_CODE_EXP(PrlStrList_GetItemsCount(list_handle, &nListSize))\
		bool bFound = false;\
		for (PRL_UINT32 i = 0; i < nListSize; ++i)\
		{\
			PRL_CHAR sBuf[STR_BUF_LENGTH];\
			PRL_UINT32 nBufSize = sizeof(sBuf);\
			CHECK_RET_CODE_EXP(PrlStrList_GetItem(list_handle, i, sBuf, &nBufSize))\
			if (UTF8_2QSTR(sBuf) == item_value)\
			{\
				bFound = true;\
				break;\
			}\
		}\
		if (!bFound)\
			QFAIL(QString("Expected string item '%1' not presents at the list").arg(item_value).toUtf8().constData());\
	}

#define CHECK_ITEM_NOT_PRESENTS(list_handle, item_value)\
	{\
		PRL_UINT32 nListSize;\
		CHECK_RET_CODE_EXP(PrlStrList_GetItemsCount(list_handle, &nListSize))\
		for (PRL_UINT32 i = 0; i < nListSize; ++i)\
		{\
			PRL_CHAR sBuf[STR_BUF_LENGTH];\
			PRL_UINT32 nBufSize = sizeof(sBuf);\
			CHECK_RET_CODE_EXP(PrlStrList_GetItem(list_handle, i, sBuf, &nBufSize))\
			if (UTF8_2QSTR(sBuf) == item_value)\
				QFAIL(QString("Unexpected string item '%1' presents at the list").arg(item_value).toUtf8().constData());\
		}\
	}

void PrlApiBasicsTest::testAddItemToStringsList()
{
	QString sTestData = "some test data";
	SdkHandleWrap hStringsList;
	CHECK_RET_CODE_EXP(PrlApi_CreateStringsList(hStringsList.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlStrList_AddItem(hStringsList, sTestData.toUtf8().constData()))
	CHECK_LIST_SIZE(hStringsList, 1)
	CHECK_ITEM_PRESENTS(hStringsList, sTestData)
}

void PrlApiBasicsTest::testAddItemToStringsListOnNullListHandle()
{
	QString sTestData = "some test data";
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlStrList_AddItem(PRL_INVALID_HANDLE, sTestData.toUtf8().constData()), PRL_ERR_INVALID_ARG)
}

void PrlApiBasicsTest::testAddItemToStringsListOnWrongListHandle()
{
	QString sTestData = "some test data";
	SdkHandleWrap hHandlesList;
	CHECK_RET_CODE_EXP(PrlApi_CreateHandlesList(hHandlesList.GetHandlePtr()))
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlStrList_AddItem(hHandlesList, sTestData.toUtf8().constData()), PRL_ERR_INVALID_ARG)
}

void PrlApiBasicsTest::testAddItemToStringsListOnNullStringElement()
{
	SdkHandleWrap hStringsList;
	CHECK_RET_CODE_EXP(PrlApi_CreateStringsList(hStringsList.GetHandlePtr()))
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlStrList_AddItem(hStringsList, 0), PRL_ERR_INVALID_ARG)
}

void PrlApiBasicsTest::testGetStringsListItemsCount()
{
	QString sTestData = "some test data";
	SdkHandleWrap hStringsList;
	CHECK_RET_CODE_EXP(PrlApi_CreateStringsList(hStringsList.GetHandlePtr()))
	CHECK_LIST_SIZE(hStringsList, 0)
	CHECK_RET_CODE_EXP(PrlStrList_AddItem(hStringsList, sTestData.toUtf8().constData()))
	CHECK_LIST_SIZE(hStringsList, 1)
}

void PrlApiBasicsTest::testGetStringsListItemsCountOnNullListHandle()
{
	PRL_UINT32 nListSize;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlStrList_GetItemsCount(PRL_INVALID_HANDLE, &nListSize), PRL_ERR_INVALID_ARG)
}

void PrlApiBasicsTest::testGetStringsListItemsCountOnWrongListHandle()
{
	PRL_UINT32 nListSize;
	SdkHandleWrap hHandlesList;
	CHECK_RET_CODE_EXP(PrlApi_CreateHandlesList(hHandlesList.GetHandlePtr()))
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlStrList_GetItemsCount(hHandlesList, &nListSize), PRL_ERR_INVALID_ARG)
}

void PrlApiBasicsTest::testGetStringsListItemsCountOnNullResultBuffer()
{
	SdkHandleWrap hStringsList;
	CHECK_RET_CODE_EXP(PrlApi_CreateStringsList(hStringsList.GetHandlePtr()))
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlStrList_GetItemsCount(hStringsList, 0), PRL_ERR_INVALID_ARG)
}

void PrlApiBasicsTest::testRemoveItemFromStringsList()
{
	QString sItem1 = "item1";
	QString sItem2 = "item2";
	QString sItem3 = "item3";
	QString sItem4 = "item4";
	SdkHandleWrap hStringsList;
	CHECK_RET_CODE_EXP(PrlApi_CreateStringsList(hStringsList.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlStrList_AddItem(hStringsList, sItem1.toUtf8().constData()))
	CHECK_RET_CODE_EXP(PrlStrList_AddItem(hStringsList, sItem2.toUtf8().constData()))
	CHECK_RET_CODE_EXP(PrlStrList_AddItem(hStringsList, sItem3.toUtf8().constData()))
	CHECK_RET_CODE_EXP(PrlStrList_AddItem(hStringsList, sItem4.toUtf8().constData()))
	CHECK_LIST_SIZE(hStringsList, 4)
	CHECK_RET_CODE_EXP(PrlStrList_RemoveItem(hStringsList, 1))
	CHECK_LIST_SIZE(hStringsList, 3)
	CHECK_ITEM_NOT_PRESENTS(hStringsList, sItem2)
	CHECK_ITEM_PRESENTS(hStringsList, sItem1)
	CHECK_ITEM_PRESENTS(hStringsList, sItem3)
	CHECK_ITEM_PRESENTS(hStringsList, sItem4)
	CHECK_RET_CODE_EXP(PrlStrList_RemoveItem(hStringsList, 2))
	CHECK_LIST_SIZE(hStringsList, 2)
	CHECK_ITEM_NOT_PRESENTS(hStringsList, sItem4)
	CHECK_ITEM_PRESENTS(hStringsList, sItem1)
	CHECK_ITEM_PRESENTS(hStringsList, sItem3)
	CHECK_RET_CODE_EXP(PrlStrList_RemoveItem(hStringsList, 0))
	CHECK_LIST_SIZE(hStringsList, 1)
	CHECK_ITEM_NOT_PRESENTS(hStringsList, sItem1)
	CHECK_ITEM_PRESENTS(hStringsList, sItem3)
	CHECK_RET_CODE_EXP(PrlStrList_RemoveItem(hStringsList, 0))
	CHECK_LIST_SIZE(hStringsList, 0)
}

void PrlApiBasicsTest::testRemoveItemFromStringsListOnNullListHandle()
{
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlStrList_RemoveItem(PRL_INVALID_HANDLE, 0), PRL_ERR_INVALID_ARG)
}

void PrlApiBasicsTest::testRemoveItemFromStringsListOnWrongListHandle()
{
	SdkHandleWrap hHandlesList;
	CHECK_RET_CODE_EXP(PrlApi_CreateHandlesList(hHandlesList.GetHandlePtr()))
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlStrList_RemoveItem(hHandlesList, 0), PRL_ERR_INVALID_ARG)
}

void PrlApiBasicsTest::testRemoveItemFromStringsListOnOutOfRangeIndex()
{
	SdkHandleWrap hStringsList;
	CHECK_RET_CODE_EXP(PrlApi_CreateStringsList(hStringsList.GetHandlePtr()))
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlStrList_RemoveItem(hStringsList, 0), PRL_ERR_INVALID_ARG)
}

void PrlApiBasicsTest::testRemoveItemFromStringsListOnOutOfRangeIndex2()
{
	SdkHandleWrap hStringsList;
	CHECK_RET_CODE_EXP(PrlApi_CreateStringsList(hStringsList.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlStrList_AddItem(hStringsList, "some data"))
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlStrList_RemoveItem(hStringsList, 1), PRL_ERR_INVALID_ARG)
}

void PrlApiBasicsTest::testGetItemFromStringsList()
{
	QString sTestData = "some test data";
	SdkHandleWrap hStringsList;
	CHECK_RET_CODE_EXP(PrlApi_CreateStringsList(hStringsList.GetHandlePtr()))
	CHECK_LIST_SIZE(hStringsList, 0)
	CHECK_RET_CODE_EXP(PrlStrList_AddItem(hStringsList, sTestData.toUtf8().constData()))
	CHECK_LIST_SIZE(hStringsList, 1)
	QByteArray sValueBuffer;
	PRL_UINT32 nValueBufSize = 0;
	CHECK_RET_CODE_EXP(PrlStrList_GetItem(hStringsList, 0, 0, &nValueBufSize))
	QVERIFY(nValueBufSize != 0);
	sValueBuffer.resize(nValueBufSize);
	CHECK_RET_CODE_EXP(PrlStrList_GetItem(hStringsList, 0, sValueBuffer.data(), &nValueBufSize))
	QCOMPARE(UTF8_2QSTR(sValueBuffer), sTestData);
}

void PrlApiBasicsTest::testGetItemFromStringsListOnNullListHandle()
{
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlStrList_GetItem(PRL_INVALID_HANDLE, 0, 0, 0), PRL_ERR_INVALID_ARG)
}

void PrlApiBasicsTest::testGetItemFromStringsListOnWrongListHandle()
{
	SdkHandleWrap hHandlesList;
	CHECK_RET_CODE_EXP(PrlApi_CreateHandlesList(hHandlesList.GetHandlePtr()))
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlStrList_GetItem(hHandlesList, 0, 0, 0), PRL_ERR_INVALID_ARG)
}

void PrlApiBasicsTest::testGetItemFromStringsListOnNullBufferSizeVariable()
{
	SdkHandleWrap hStringsList;
	CHECK_RET_CODE_EXP(PrlApi_CreateStringsList(hStringsList.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlStrList_AddItem(hStringsList, "some data"))
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlStrList_GetItem(hStringsList, 0, 0, 0), PRL_ERR_INVALID_ARG)
}

void PrlApiBasicsTest::testGetItemFromStringsListOnOutOfRangeIndex()
{
	SdkHandleWrap hStringsList;
	CHECK_RET_CODE_EXP(PrlApi_CreateStringsList(hStringsList.GetHandlePtr()))
	QByteArray sValueBuffer;
	PRL_UINT32 nValueBufSize = 0;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlStrList_GetItem(hStringsList, 0, 0, &nValueBufSize), PRL_ERR_INVALID_ARG)
}

void PrlApiBasicsTest::testGetItemFromStringsListOnOutOfRangeIndex2()
{
	SdkHandleWrap hStringsList;
	CHECK_RET_CODE_EXP(PrlApi_CreateStringsList(hStringsList.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlStrList_AddItem(hStringsList, "some data"))
	QByteArray sValueBuffer;
	PRL_UINT32 nValueBufSize = 0;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlStrList_GetItem(hStringsList, 1, 0, &nValueBufSize), PRL_ERR_INVALID_ARG)
}

#define CHECK_MESSAGE_TYPE(error_code, message_type)\
	{\
		PRL_MESSAGE_TYPE_ENUM _msg_type;\
		CHECK_RET_CODE_EXP(PrlApi_GetMessageType(error_code, &_msg_type));\
		QVERIFY(_msg_type == message_type);\
	}

void PrlApiBasicsTest::testGetMessageType()
{
	CHECK_MESSAGE_TYPE(PET_ANSWER_YES, PMT_ANSWER)
	CHECK_MESSAGE_TYPE(PRL_ERR_OUT_OF_MEMORY, PMT_CRITICAL)
	CHECK_MESSAGE_TYPE(PET_QUESTION_DO_YOU_WANT_TO_OVERWRITE_FILE, PMT_QUESTION)
}

void PrlApiBasicsTest::testMsgCanBeIgnored()
{
	PRL_BOOL bCanBeIgnored = PRL_TRUE;
	CHECK_RET_CODE_EXP(PrlApi_MsgCanBeIgnored(PRL_ERR_OUT_OF_MEMORY, &bCanBeIgnored))
	QVERIFY(bCanBeIgnored == PRL_FALSE);
}

#define CHECK_HANDLES_LIST_SIZE(list_handle, expected_size)\
	{\
		PRL_UINT32 nActualSize;\
		CHECK_RET_CODE_EXP(PrlHndlList_GetItemsCount(list_handle, &nActualSize))\
		QVERIFY(nActualSize == expected_size);\
	}

#define CHECK_HANDLE_ITEM_PRESENTS(list_handle, item_value, handle_type)\
	{\
		PRL_UINT32 nListSize;\
		CHECK_RET_CODE_EXP(PrlHndlList_GetItemsCount(list_handle, &nListSize))\
		bool bFound = false;\
		for (PRL_UINT32 i = 0; i < nListSize; ++i)\
		{\
			SdkHandleWrap hItem;\
			CHECK_RET_CODE_EXP(PrlHndlList_GetItem(list_handle, i, hItem.GetHandlePtr()))\
			if (hItem.GetHandle() == item_value.GetHandle())\
			{\
				PRL_HANDLE_TYPE _type;\
				CHECK_RET_CODE_EXP(PrlHandle_GetType(hItem, &_type))\
				if (_type == handle_type)\
				{\
					bFound = true;\
					break;\
				}\
			}\
		}\
		if (!bFound)\
			QFAIL(QString("Expected handle item with type %1 not presents at the list").arg(PRL_HANDLE_TYPE_TO_STRING(handle_type)).toUtf8().constData());\
	}

#define CHECK_HANDLE_ITEM_NOT_PRESENTS(list_handle, item_value, handle_type)\
	{\
		PRL_UINT32 nListSize;\
		CHECK_RET_CODE_EXP(PrlHndlList_GetItemsCount(list_handle, &nListSize))\
		for (PRL_UINT32 i = 0; i < nListSize; ++i)\
		{\
			SdkHandleWrap hItem;\
			CHECK_RET_CODE_EXP(PrlHndlList_GetItem(list_handle, i, hItem.GetHandlePtr()))\
			if (hItem.GetHandle() == item_value.GetHandle())\
				QFAIL(QString("Unexpected handle item with type '%1' presents at the list").arg(PRL_HANDLE_TYPE_TO_STRING(handle_type)).toUtf8().constData());\
		}\
	}

void PrlApiBasicsTest::testCreateHandlesList()
{
	SdkHandleWrap hHandlesList;
	CHECK_RET_CODE_EXP(PrlApi_CreateHandlesList(hHandlesList.GetHandlePtr()))
	CHECK_HANDLE_TYPE(hHandlesList, PHT_HANDLES_LIST)
}

void PrlApiBasicsTest::testCreateHandlesListOnNullBufferPointer()
{
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlApi_CreateHandlesList(0), PRL_ERR_INVALID_ARG)
}

void PrlApiBasicsTest::testAddItemToHandlesList()
{
	SdkHandleWrap hServer;
	CHECK_RET_CODE_EXP(PrlSrv_Create(hServer.GetHandlePtr()))
	SdkHandleWrap hHandlesList;
	CHECK_RET_CODE_EXP(PrlApi_CreateHandlesList(hHandlesList.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlHndlList_AddItem(hHandlesList, hServer.GetHandle()))
	CHECK_HANDLES_LIST_SIZE(hHandlesList, 1)
	CHECK_HANDLE_ITEM_PRESENTS(hHandlesList, hServer, PHT_SERVER)
}

void PrlApiBasicsTest::testAddItemToHandlesListOnNullListHandle()
{
	PRL_HANDLE handle = PRL_INVALID_HANDLE;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlHndlList_AddItem(PRL_INVALID_HANDLE, handle), PRL_ERR_INVALID_ARG)
}

void PrlApiBasicsTest::testAddItemToHandlesListOnWrongListHandle()
{
	SdkHandleWrap hServer;
	CHECK_RET_CODE_EXP(PrlSrv_Create(hServer.GetHandlePtr()))
	SdkHandleWrap hStringsList;
	CHECK_RET_CODE_EXP(PrlApi_CreateStringsList(hStringsList.GetHandlePtr()))
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlHndlList_AddItem(hStringsList, hServer), PRL_ERR_INVALID_ARG)
}

void PrlApiBasicsTest::testAddItemToHandlesListOnInvalidItemHandle()
{
	PRL_HANDLE handle = PRL_INVALID_HANDLE;
	SdkHandleWrap hHandlesList;
	CHECK_RET_CODE_EXP(PrlApi_CreateHandlesList(hHandlesList.GetHandlePtr()))
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlHndlList_AddItem(hHandlesList, handle), PRL_ERR_INVALID_ARG)
}

void PrlApiBasicsTest::testAddItemToHandlesListOnRemovedItemHandle()
{
	PRL_HANDLE handle = PRL_INVALID_HANDLE;
	{
		SdkHandleWrap hServer;
		CHECK_RET_CODE_EXP(PrlSrv_Create(hServer.GetHandlePtr()))
		handle = hServer.GetHandle();
	}
	SdkHandleWrap hHandlesList;
	CHECK_RET_CODE_EXP(PrlApi_CreateHandlesList(hHandlesList.GetHandlePtr()))
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlHndlList_AddItem(hHandlesList, handle), PRL_ERR_INVALID_ARG)
}

void PrlApiBasicsTest::testGetHandlesListItemsCount()
{
	SdkHandleWrap hServer;
	CHECK_RET_CODE_EXP(PrlSrv_Create(hServer.GetHandlePtr()))
	SdkHandleWrap hHandlesList;
	CHECK_RET_CODE_EXP(PrlApi_CreateHandlesList(hHandlesList.GetHandlePtr()))
	CHECK_HANDLES_LIST_SIZE(hHandlesList, 0)
	CHECK_RET_CODE_EXP(PrlHndlList_AddItem(hHandlesList, hServer))
	CHECK_HANDLES_LIST_SIZE(hHandlesList, 1)
}

void PrlApiBasicsTest::testGetHandlesListItemsCountOnNullListHandle()
{
	PRL_UINT32 nListSize;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlHndlList_GetItemsCount(PRL_INVALID_HANDLE, &nListSize), PRL_ERR_INVALID_ARG)
}

void PrlApiBasicsTest::testGetHandlesListItemsCountOnWrongListHandle()
{
	PRL_UINT32 nListSize;
	SdkHandleWrap hStringsList;
	CHECK_RET_CODE_EXP(PrlApi_CreateStringsList(hStringsList.GetHandlePtr()))
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlHndlList_GetItemsCount(hStringsList, &nListSize), PRL_ERR_INVALID_ARG)
}

void PrlApiBasicsTest::testGetHandlesListItemsCountOnNullResultBuffer()
{
	SdkHandleWrap hHandlesList;
	CHECK_RET_CODE_EXP(PrlApi_CreateHandlesList(hHandlesList.GetHandlePtr()))
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlHndlList_GetItemsCount(hHandlesList, 0), PRL_ERR_INVALID_ARG)
}

void PrlApiBasicsTest::testRemoveItemFromHandlesList()
{
	SdkHandleWrap hItem1;
	CHECK_RET_CODE_EXP(PrlSrv_Create(hItem1.GetHandlePtr()))
	SdkHandleWrap hItem2;
	CHECK_RET_CODE_EXP(PrlApi_CreateStringsList(hItem2.GetHandlePtr()))
	SdkHandleWrap hItem3;
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(hItem1, hItem3.GetHandlePtr()))
	SdkHandleWrap hItem4;
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(hItem3, PDE_SOUND_DEVICE, hItem4.GetHandlePtr()))
	SdkHandleWrap hHandlesList;
	CHECK_RET_CODE_EXP(PrlApi_CreateHandlesList(hHandlesList.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlHndlList_AddItem(hHandlesList, hItem1.GetHandle()))
	CHECK_RET_CODE_EXP(PrlHndlList_AddItem(hHandlesList, hItem2.GetHandle()))
	CHECK_RET_CODE_EXP(PrlHndlList_AddItem(hHandlesList, hItem3.GetHandle()))
	CHECK_RET_CODE_EXP(PrlHndlList_AddItem(hHandlesList, hItem4.GetHandle()))
	CHECK_HANDLES_LIST_SIZE(hHandlesList, 4)
	CHECK_RET_CODE_EXP(PrlHndlList_RemoveItem(hHandlesList, 1))
	CHECK_HANDLES_LIST_SIZE(hHandlesList, 3)
	CHECK_HANDLE_ITEM_NOT_PRESENTS(hHandlesList, hItem2, PHT_VIRTUAL_MACHINE)
	CHECK_HANDLE_ITEM_PRESENTS(hHandlesList, hItem1, PHT_SERVER)
	CHECK_HANDLE_ITEM_PRESENTS(hHandlesList, hItem3, PHT_VIRTUAL_MACHINE)
	CHECK_HANDLE_ITEM_PRESENTS(hHandlesList, hItem4, PHT_VIRTUAL_DEV_SOUND)
	CHECK_RET_CODE_EXP(PrlHndlList_RemoveItem(hHandlesList, 2))
	CHECK_HANDLES_LIST_SIZE(hHandlesList, 2)
	CHECK_HANDLE_ITEM_NOT_PRESENTS(hHandlesList, hItem4, PHT_VIRTUAL_DEV_SOUND)
	CHECK_HANDLE_ITEM_PRESENTS(hHandlesList, hItem1, PHT_SERVER)
	CHECK_HANDLE_ITEM_PRESENTS(hHandlesList, hItem3, PHT_VIRTUAL_MACHINE)
	CHECK_RET_CODE_EXP(PrlHndlList_RemoveItem(hHandlesList, 0))
	CHECK_HANDLES_LIST_SIZE(hHandlesList, 1)
	CHECK_HANDLE_ITEM_NOT_PRESENTS(hHandlesList, hItem1, PHT_SERVER)
	CHECK_HANDLE_ITEM_PRESENTS(hHandlesList, hItem3, PHT_VIRTUAL_MACHINE)
	CHECK_RET_CODE_EXP(PrlHndlList_RemoveItem(hHandlesList, 0))
	CHECK_HANDLES_LIST_SIZE(hHandlesList, 0)
}

void PrlApiBasicsTest::testRemoveItemFromHandlesListOnNullListHandle()
{
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlHndlList_RemoveItem(PRL_INVALID_HANDLE, 0), PRL_ERR_INVALID_ARG)
}

void PrlApiBasicsTest::testRemoveItemFromHandlesListOnWrongListHandle()
{
	SdkHandleWrap hStringsList;
	CHECK_RET_CODE_EXP(PrlApi_CreateStringsList(hStringsList.GetHandlePtr()))
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlHndlList_RemoveItem(hStringsList, 0), PRL_ERR_INVALID_ARG)
}

void PrlApiBasicsTest::testRemoveItemFromHandlesListOnOutOfRangeIndex()
{
	SdkHandleWrap hHandlesList;
	CHECK_RET_CODE_EXP(PrlApi_CreateHandlesList(hHandlesList.GetHandlePtr()))
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlHndlList_RemoveItem(hHandlesList, 0), PRL_ERR_INVALID_ARG)
}

void PrlApiBasicsTest::testRemoveItemFromHandlesListOnOutOfRangeIndex2()
{
	SdkHandleWrap hHandlesList;
	CHECK_RET_CODE_EXP(PrlApi_CreateHandlesList(hHandlesList.GetHandlePtr()))
	SdkHandleWrap hServer;
	CHECK_RET_CODE_EXP(PrlSrv_Create(hServer.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlHndlList_AddItem(hHandlesList, hServer))
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlHndlList_RemoveItem(hHandlesList, 1), PRL_ERR_INVALID_ARG)
}

void PrlApiBasicsTest::testGetItemFromHandlesList()
{
	SdkHandleWrap hServer;
	CHECK_RET_CODE_EXP(PrlSrv_Create(hServer.GetHandlePtr()))
	SdkHandleWrap hHandlesList;
	CHECK_RET_CODE_EXP(PrlApi_CreateHandlesList(hHandlesList.GetHandlePtr()))
	CHECK_HANDLES_LIST_SIZE(hHandlesList, 0)
	CHECK_RET_CODE_EXP(PrlHndlList_AddItem(hHandlesList, hServer.GetHandle()))
	CHECK_HANDLES_LIST_SIZE(hHandlesList, 1)
	SdkHandleWrap hActualValue;
	CHECK_RET_CODE_EXP(PrlHndlList_GetItem(hHandlesList, 0, hActualValue.GetHandlePtr()))
	QVERIFY(hActualValue.GetHandle() == hServer.GetHandle());
}

void PrlApiBasicsTest::testGetItemFromHandlesListOnNullListHandle()
{
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlHndlList_GetItem(PRL_INVALID_HANDLE, 0, 0), PRL_ERR_INVALID_ARG)
}

void PrlApiBasicsTest::testGetItemFromHandlesListOnWrongListHandle()
{
	SdkHandleWrap hStringsList;
	CHECK_RET_CODE_EXP(PrlApi_CreateStringsList(hStringsList.GetHandlePtr()))
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlHndlList_GetItem(hStringsList, 0, 0), PRL_ERR_INVALID_ARG)
}

void PrlApiBasicsTest::testGetItemFromHandlesListOnNullResultBuffer()
{
	SdkHandleWrap hHandlesList;
	CHECK_RET_CODE_EXP(PrlApi_CreateHandlesList(hHandlesList.GetHandlePtr()))
	SdkHandleWrap hServer;
	CHECK_RET_CODE_EXP(PrlSrv_Create(hServer.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlHndlList_AddItem(hHandlesList, hServer))
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlHndlList_GetItem(hHandlesList, 0, 0), PRL_ERR_INVALID_ARG)
}

void PrlApiBasicsTest::testGetItemFromHandlesListOnOutOfRangeIndex()
{
	SdkHandleWrap hHandlesList;
	CHECK_RET_CODE_EXP(PrlApi_CreateHandlesList(hHandlesList.GetHandlePtr()))
	SdkHandleWrap hActualValue;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlHndlList_GetItem(hHandlesList, 0, hActualValue.GetHandlePtr()),\
										PRL_ERR_INVALID_ARG)
}

void PrlApiBasicsTest::testGetItemFromHandlesListOnOutOfRangeIndex2()
{
	SdkHandleWrap hHandlesList;
	CHECK_RET_CODE_EXP(PrlApi_CreateHandlesList(hHandlesList.GetHandlePtr()))
	SdkHandleWrap hServer;
	CHECK_RET_CODE_EXP(PrlSrv_Create(hServer.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlHndlList_AddItem(hHandlesList, hServer))
	SdkHandleWrap hActualValue;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlHndlList_GetItem(hHandlesList, 1, hActualValue.GetHandlePtr()),\
										PRL_ERR_INVALID_ARG)
}

void PrlApiBasicsTest::testApiGetVersion()
{
	PRL_UINT32 nVersion = 0;
	CHECK_RET_CODE_EXP(PrlApi_GetVersion(&nVersion))
	QCOMPARE(quint32(nVersion), quint32(PARALLELS_API_VER));
}

void PrlApiBasicsTest::testApiGetVersionOnNullPointer()
{
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlApi_GetVersion(0), PRL_ERR_INVALID_ARG)
}

void PrlApiBasicsTest::testApiGetAppMode()
{
	PRL_APPLICATION_MODE nAppMode = PAM_SERVER;
	CHECK_RET_CODE_EXP(PrlApi_GetAppMode(&nAppMode))
	QCOMPARE(quint32(nAppMode), quint32(PAM_SERVER));
}

void PrlApiBasicsTest::testApiGetAppModeOnNullPointer()
{
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlApi_GetAppMode(0), PRL_ERR_INVALID_ARG)
}

void PrlApiBasicsTest::testGetErrDescriptionForSpecificErrorCode()
{
	SdkHandleWrap hServer;
	CHECK_RET_CODE_EXP(PrlSrv_Create(hServer.GetHandlePtr()))
	SdkHandleWrap hVm;
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(hServer, hVm.GetHandlePtr()))
	SdkHandleWrap hEvent;
	CHECK_RET_CODE_EXP(PrlVm_CreateAnswerEvent(hVm, hEvent.GetHandlePtr(), PET_ANSWER_OK))
	CVmEvent _event(
		PET_DSP_EVT_VM_MESSAGE,
		Uuid::createUuid().toString(),
		PIE_VIRTUAL_MACHINE,
		PRL_WARN_IMAGE_IS_STOLEN,
		PVE::EventRespNotRequired
	);
	_event.addEventParameter(new CVmEventParameter(PVE::String, "floppy0", EVT_PARAM_MESSAGE_PARAM_0));
	CHECK_RET_CODE_EXP(PrlEvent_FromString(hEvent, _event.toString().toUtf8().constData()))

	PRL_CHAR sBuf[1024];
	PRL_UINT32 nBufSize = sizeof(sBuf);
	CHECK_RET_CODE_EXP(PrlEvent_GetErrString(hEvent, PRL_FALSE, PRL_FALSE, sBuf, &nBufSize))
}

void PrlApiBasicsTest::testGetErrDescription_ForSingularPluralParams()
{
	SdkHandleWrap hServer;
	CHECK_RET_CODE_EXP(PrlSrv_Create(hServer.GetHandlePtr()));
	SdkHandleWrap hVm;
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(hServer, hVm.GetHandlePtr()));
	SdkHandleWrap hEvent;
	CHECK_RET_CODE_EXP(PrlVm_CreateEvent(hVm, hEvent.GetHandlePtr()));
	CVmEvent _event(
		PET_DSP_EVT_VM_MESSAGE,
		Uuid::createUuid().toString(),
		PIE_VIRTUAL_MACHINE,
		PRL_ERR_TEST_TEXT_MESSAGES_PS,
		PVE::EventRespNotRequired
	);
	/* Test message should have next fields:
		Sources/SDK/Handles/ingsStorage.cpp
	REGISTER_ERROR_STRING2(
		PRL_ERR_TEST_TEXT_MESSAGES_PS,
		// BRIEF /
		tr("BRIEF DEFAULT %1."),
		// brief singular
		1, tr("BRIEF SINGULAR %1."),
		// LONG /
		tr("LONG DEFAULT %2."),
		// LONG singular
		2, tr("LONG SINGULAR %2."),
		false,
		PMT_CRITICAL );

REGISTER_ERROR_STRING(
		PRL_ERR_TEST_TEXT_MESSAGES,
		tr("BRIEF DEFAULT %1."),
		tr("LONG DEFAULT %2."),
		false,
		PMT_CRITICAL );

	*/
#define SP_TEST( _bIsBrief, _sValNumber, _PARAM, _sExpectedMessage ) \
{	\
	PRL_CHAR sBuf[1024]; \
	PRL_UINT32 nBufSize = sizeof(sBuf); \
	\
	_event.m_lstEventParameters.clear(); \
	\
	_event.addEventParameter(new CVmEventParameter(PVE::UnsignedInt, _sValNumber, _PARAM )); \
	CHECK_RET_CODE_EXP(PrlEvent_FromString(hEvent, _event.toString().toUtf8().constData())); \
	CHECK_RET_CODE_EXP(PrlEvent_GetErrString(hEvent, _bIsBrief, PRL_FALSE, sBuf, &nBufSize)); \
	QCOMPARE( QString(sBuf), _sExpectedMessage ); \
}
	// test param 0/1 (EVT_PARAM_MESSAGE_PARAM_xxx)
	// test default
	// test plural
	// test singular
	// brief / long

	const char s5[]="5"; // PLURAL
	const char s1[]="1"; // SINGULAR

	QString BRIEF_DEFAULT_x1 = QString( "BRIEF DEFAULT %1." );
	QString BRIEF_SINGULAR_x1 = QString( "BRIEF SINGULAR %1." );

	QString LONG_DEFAULT_x2 = QString( "LONG DEFAULT %2." );
	QString LONG_SINGULAR_x2 = QString( "LONG SINGULAR %2." );

	// test new format
	_event.setEventCode(PRL_ERR_TEST_TEXT_MESSAGES_PS);
	// brief test
	SP_TEST( PRL_TRUE, s5, EVT_PARAM_MESSAGE_PARAM_0, BRIEF_DEFAULT_x1.arg(s5) );
	SP_TEST( PRL_TRUE, s1, EVT_PARAM_MESSAGE_PARAM_0, BRIEF_SINGULAR_x1.arg(s1) );
	SP_TEST( PRL_TRUE, s1, EVT_PARAM_MESSAGE_PARAM_1, BRIEF_DEFAULT_x1 );

	// long test
	SP_TEST( PRL_FALSE, s5, EVT_PARAM_MESSAGE_PARAM_1, LONG_DEFAULT_x2.arg(s5) );
	SP_TEST( PRL_FALSE, s1, EVT_PARAM_MESSAGE_PARAM_1, LONG_SINGULAR_x2.arg(s1) );
	SP_TEST( PRL_FALSE, s1, EVT_PARAM_MESSAGE_PARAM_2, LONG_DEFAULT_x2 );

	// test old format
	_event.setEventCode(PRL_ERR_TEST_TEXT_MESSAGES);
	SP_TEST( PRL_TRUE, s5, EVT_PARAM_MESSAGE_PARAM_0, BRIEF_DEFAULT_x1.arg(s5) );
	SP_TEST( PRL_TRUE, s1, EVT_PARAM_MESSAGE_PARAM_0, BRIEF_DEFAULT_x1.arg(s1) );
	SP_TEST( PRL_TRUE, s1, EVT_PARAM_MESSAGE_PARAM_1, BRIEF_DEFAULT_x1 );

	SP_TEST( PRL_FALSE, s5, EVT_PARAM_MESSAGE_PARAM_1, LONG_DEFAULT_x2.arg(s5) );
	SP_TEST( PRL_FALSE, s1, EVT_PARAM_MESSAGE_PARAM_1, LONG_DEFAULT_x2.arg(s1) );
	SP_TEST( PRL_FALSE, s1, EVT_PARAM_MESSAGE_PARAM_2, LONG_DEFAULT_x2 );

#undef SP_TEST
}

#define CHECK_OPAQUE_TYPE_LIST_INSTANTIATION(type)\
	{\
		SdkHandleWrap hOpTypeList;\
		CHECK_RET_CODE_EXP(PrlApi_CreateOpTypeList(hOpTypeList.GetHandlePtr(), sizeof(type)))\
		CHECK_HANDLE_TYPE(hOpTypeList, PHT_OPAQUE_TYPE_LIST)\
		PRL_SIZE nTypeSize = 0;\
		CHECK_RET_CODE_EXP(PrlOpTypeList_GetTypeSize(hOpTypeList, &nTypeSize))\
		QCOMPARE(quint32(nTypeSize), quint32(sizeof(type)));\
	}

void PrlApiBasicsTest::testCreateOpTypeList()
{
	CHECK_OPAQUE_TYPE_LIST_INSTANTIATION(PRL_CHAR)
	CHECK_OPAQUE_TYPE_LIST_INSTANTIATION(PRL_UINT8)
	CHECK_OPAQUE_TYPE_LIST_INSTANTIATION(PRL_INT8)
	CHECK_OPAQUE_TYPE_LIST_INSTANTIATION(PRL_UINT16)
	CHECK_OPAQUE_TYPE_LIST_INSTANTIATION(PRL_INT16)
	CHECK_OPAQUE_TYPE_LIST_INSTANTIATION(PRL_UINT32)
	CHECK_OPAQUE_TYPE_LIST_INSTANTIATION(PRL_INT32)
	CHECK_OPAQUE_TYPE_LIST_INSTANTIATION(PRL_UINT64)
	CHECK_OPAQUE_TYPE_LIST_INSTANTIATION(PRL_INT64)
	CHECK_OPAQUE_TYPE_LIST_INSTANTIATION(PRL_SIZE)
	CHECK_OPAQUE_TYPE_LIST_INSTANTIATION(PRL_BOOL)
}

void PrlApiBasicsTest::testCreateOpTypeListOnNullBufferPointer()
{
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlApi_CreateOpTypeList(0, sizeof(PRL_UINT32)), PRL_ERR_INVALID_ARG)
}

void PrlApiBasicsTest::testCreateOpTypeListOnWrongDataSize()
{
	SdkHandleWrap hOpTypeList;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlApi_CreateOpTypeList(hOpTypeList.GetHandlePtr(), 0), PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlApi_CreateOpTypeList(hOpTypeList.GetHandlePtr(), 3), PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlApi_CreateOpTypeList(hOpTypeList.GetHandlePtr(), 3878989), PRL_ERR_INVALID_ARG)
}

#define CHECK_LIST_SIZE_OP_TYPE(list_handle, expected_size)\
	{\
		PRL_UINT32 nActualSize;\
		CHECK_RET_CODE_EXP(PrlOpTypeList_GetItemsCount(list_handle, &nActualSize))\
		QVERIFY(nActualSize == expected_size);\
	}

#define CHECK_ITEM_PRESENTS_OP_TYPE(type, list_handle, item_value)\
	{\
		PRL_UINT32 nListSize;\
		CHECK_RET_CODE_EXP(PrlOpTypeList_GetItemsCount(list_handle, &nListSize))\
		bool bFound = false;\
		for (PRL_UINT32 i = 0; i < nListSize; ++i)\
		{\
			type nItem = 0;\
			CHECK_RET_CODE_EXP(PrlOpTypeList_GetItem(list_handle, i, &nItem))\
			if (nItem == item_value)\
			{\
				bFound = true;\
				break;\
			}\
		}\
		if (!bFound)\
			QFAIL(QString("Expected uint32 item '%1' not presents at the list").arg(item_value).toUtf8().constData());\
	}

#define CHECK_ITEM_NOT_PRESENTS_OP_TYPE(type, list_handle, item_value)\
	{\
		PRL_UINT32 nListSize;\
		CHECK_RET_CODE_EXP(PrlOpTypeList_GetItemsCount(list_handle, &nListSize))\
		for (PRL_UINT32 i = 0; i < nListSize; ++i)\
		{\
			type nItem = 0;\
			CHECK_RET_CODE_EXP(PrlOpTypeList_GetItem(list_handle, i, &nItem))\
			if (nItem == item_value)\
				QFAIL(QString("Unexpected uint32 item '%1' presents at the list").arg(item_value).toUtf8().constData());\
		}\
	}

void PrlApiBasicsTest::testAddItemToOpTypeList()
{
	PRL_UINT32 nTestData = 456476436;
	SdkHandleWrap hOpTypeList;
	CHECK_RET_CODE_EXP(PrlApi_CreateOpTypeList(hOpTypeList.GetHandlePtr(), sizeof(PRL_UINT32)))
	CHECK_RET_CODE_EXP(PrlOpTypeList_AddItem(hOpTypeList, (PRL_CONST_VOID_PTR)&nTestData))
	CHECK_LIST_SIZE_OP_TYPE(hOpTypeList, 1)
	CHECK_ITEM_PRESENTS_OP_TYPE(PRL_UINT32, hOpTypeList, nTestData)
}

void PrlApiBasicsTest::testAddItemToOpTypeListOnNullListHandle()
{
	PRL_UINT32 nTestData = 0;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlOpTypeList_AddItem(PRL_INVALID_HANDLE, (PRL_CONST_VOID_PTR)&nTestData), PRL_ERR_INVALID_ARG)
}

void PrlApiBasicsTest::testAddItemToOpTypeListOnWrongListHandle()
{
	PRL_UINT32 nTestData = 0;
	SdkHandleWrap hHandlesList;
	CHECK_RET_CODE_EXP(PrlApi_CreateHandlesList(hHandlesList.GetHandlePtr()))
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlOpTypeList_AddItem(hHandlesList, (PRL_CONST_VOID_PTR)&nTestData), PRL_ERR_INVALID_ARG)
}

void PrlApiBasicsTest::testAddItemToOpTypeListOnNullPointer()
{
	SdkHandleWrap hOpTypeList;
	CHECK_RET_CODE_EXP(PrlApi_CreateOpTypeList(hOpTypeList.GetHandlePtr(), sizeof(PRL_UINT32)))
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlOpTypeList_AddItem(hOpTypeList, 0), PRL_ERR_INVALID_ARG)
}

void PrlApiBasicsTest::testGetOpTypeListItemsCount()
{
	PRL_UINT32 nTestData = 0;
	SdkHandleWrap hOpTypeList;
	CHECK_RET_CODE_EXP(PrlApi_CreateOpTypeList(hOpTypeList.GetHandlePtr(), sizeof(PRL_UINT32)))
	CHECK_LIST_SIZE_OP_TYPE(hOpTypeList, 0)
	CHECK_RET_CODE_EXP(PrlOpTypeList_AddItem(hOpTypeList, (PRL_CONST_VOID_PTR)&nTestData))
	CHECK_LIST_SIZE_OP_TYPE(hOpTypeList, 1)
}

void PrlApiBasicsTest::testGetOpTypeListItemsCountOnNullListHandle()
{
	PRL_UINT32 nListSize;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlOpTypeList_GetItemsCount(PRL_INVALID_HANDLE, &nListSize), PRL_ERR_INVALID_ARG)
}

void PrlApiBasicsTest::testGetOpTypeListItemsCountOnWrongListHandle()
{
	PRL_UINT32 nListSize;
	SdkHandleWrap hHandlesList;
	CHECK_RET_CODE_EXP(PrlApi_CreateHandlesList(hHandlesList.GetHandlePtr()))
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlOpTypeList_GetItemsCount(hHandlesList, &nListSize), PRL_ERR_INVALID_ARG)
}

void PrlApiBasicsTest::testGetOpTypeListItemsCountOnNullResultBuffer()
{
	SdkHandleWrap hOpTypeList;
	CHECK_RET_CODE_EXP(PrlApi_CreateOpTypeList(hOpTypeList.GetHandlePtr(), sizeof(PRL_UINT32)))
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlOpTypeList_GetItemsCount(hOpTypeList, 0), PRL_ERR_INVALID_ARG)
}

void PrlApiBasicsTest::testRemoveItemFromOpTypeList()
{
	PRL_UINT32 nItem1 = 0;
	PRL_UINT32 nItem2 = 1;
	PRL_UINT32 nItem3 = 2;
	PRL_UINT32 nItem4 = 3;
	SdkHandleWrap hOpTypeList;
	CHECK_RET_CODE_EXP(PrlApi_CreateOpTypeList(hOpTypeList.GetHandlePtr(), sizeof(PRL_UINT32)))
	CHECK_RET_CODE_EXP(PrlOpTypeList_AddItem(hOpTypeList, (PRL_CONST_VOID_PTR)&nItem1))
	CHECK_RET_CODE_EXP(PrlOpTypeList_AddItem(hOpTypeList, (PRL_CONST_VOID_PTR)&nItem2))
	CHECK_RET_CODE_EXP(PrlOpTypeList_AddItem(hOpTypeList, (PRL_CONST_VOID_PTR)&nItem3))
	CHECK_RET_CODE_EXP(PrlOpTypeList_AddItem(hOpTypeList, (PRL_CONST_VOID_PTR)&nItem4))
	CHECK_LIST_SIZE_OP_TYPE(hOpTypeList, 4)
	CHECK_RET_CODE_EXP(PrlOpTypeList_RemoveItem(hOpTypeList, 1))
	CHECK_LIST_SIZE_OP_TYPE(hOpTypeList, 3)
	CHECK_ITEM_NOT_PRESENTS_OP_TYPE(PRL_UINT32, hOpTypeList, nItem2)
	CHECK_ITEM_PRESENTS_OP_TYPE(PRL_UINT32, hOpTypeList, nItem1)
	CHECK_ITEM_PRESENTS_OP_TYPE(PRL_UINT32, hOpTypeList, nItem3)
	CHECK_ITEM_PRESENTS_OP_TYPE(PRL_UINT32, hOpTypeList, nItem4)
	CHECK_RET_CODE_EXP(PrlOpTypeList_RemoveItem(hOpTypeList, 2))
	CHECK_LIST_SIZE_OP_TYPE(hOpTypeList, 2)
	CHECK_ITEM_NOT_PRESENTS_OP_TYPE(PRL_UINT32, hOpTypeList, nItem4)
	CHECK_ITEM_PRESENTS_OP_TYPE(PRL_UINT32, hOpTypeList, nItem1)
	CHECK_ITEM_PRESENTS_OP_TYPE(PRL_UINT32, hOpTypeList, nItem3)
	CHECK_RET_CODE_EXP(PrlOpTypeList_RemoveItem(hOpTypeList, 0))
	CHECK_LIST_SIZE_OP_TYPE(hOpTypeList, 1)
	CHECK_ITEM_NOT_PRESENTS_OP_TYPE(PRL_UINT32, hOpTypeList, nItem1)
	CHECK_ITEM_PRESENTS_OP_TYPE(PRL_UINT32, hOpTypeList, nItem3)
	CHECK_RET_CODE_EXP(PrlOpTypeList_RemoveItem(hOpTypeList, 0))
	CHECK_LIST_SIZE_OP_TYPE(hOpTypeList, 0)
}

void PrlApiBasicsTest::testRemoveItemFromOpTypeListOnNullListHandle()
{
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlOpTypeList_RemoveItem(PRL_INVALID_HANDLE, 0), PRL_ERR_INVALID_ARG)
}

void PrlApiBasicsTest::testRemoveItemFromOpTypeListOnWrongListHandle()
{
	SdkHandleWrap hHandlesList;
	CHECK_RET_CODE_EXP(PrlApi_CreateHandlesList(hHandlesList.GetHandlePtr()))
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlOpTypeList_RemoveItem(hHandlesList, 0), PRL_ERR_INVALID_ARG)
}

void PrlApiBasicsTest::testRemoveItemFromOpTypeListOnOutOfRangeIndex()
{
	SdkHandleWrap hOpTypeList;
	CHECK_RET_CODE_EXP(PrlApi_CreateOpTypeList(hOpTypeList.GetHandlePtr(), sizeof(PRL_UINT32)))
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlOpTypeList_RemoveItem(hOpTypeList, 0), PRL_ERR_INVALID_ARG)
}

void PrlApiBasicsTest::testRemoveItemFromOpTypeListOnOutOfRangeIndex2()
{
	PRL_UINT32 nTestData = 0;
	SdkHandleWrap hOpTypeList;
	CHECK_RET_CODE_EXP(PrlApi_CreateOpTypeList(hOpTypeList.GetHandlePtr(), sizeof(PRL_UINT32)))
	CHECK_RET_CODE_EXP(PrlOpTypeList_AddItem(hOpTypeList, (PRL_CONST_VOID_PTR)&nTestData))
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlOpTypeList_RemoveItem(hOpTypeList, 1), PRL_ERR_INVALID_ARG)
}

void PrlApiBasicsTest::testGetItemFromOpTypeList()
{
	PRL_UINT32 nTestData = 454776878;
	SdkHandleWrap hOpTypeList;
	CHECK_RET_CODE_EXP(PrlApi_CreateOpTypeList(hOpTypeList.GetHandlePtr(), sizeof(PRL_UINT32)))
	CHECK_LIST_SIZE_OP_TYPE(hOpTypeList, 0)
	CHECK_RET_CODE_EXP(PrlOpTypeList_AddItem(hOpTypeList, (PRL_CONST_VOID_PTR)&nTestData))
	CHECK_LIST_SIZE_OP_TYPE(hOpTypeList, 1)
	PRL_UINT32 nActualItem = 0;
	CHECK_RET_CODE_EXP(PrlOpTypeList_GetItem(hOpTypeList, 0, &nActualItem))
	QCOMPARE(quint32(nActualItem), quint32(nTestData));
}

void PrlApiBasicsTest::testGetItemFromOpTypeListOnNullListHandle()
{
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlOpTypeList_GetItem(PRL_INVALID_HANDLE, 0, 0), PRL_ERR_INVALID_ARG)
}

void PrlApiBasicsTest::testGetItemFromOpTypeListOnWrongListHandle()
{
	PRL_UINT32 nItem = 0;
	SdkHandleWrap hHandlesList;
	CHECK_RET_CODE_EXP(PrlApi_CreateHandlesList(hHandlesList.GetHandlePtr()))
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlOpTypeList_GetItem(hHandlesList, 0, &nItem), PRL_ERR_INVALID_ARG)
}

void PrlApiBasicsTest::testGetItemFromOpTypeListOnNullBufferPointer()
{
	PRL_UINT32 nTestData = 0;
	SdkHandleWrap hOpTypeList;
	CHECK_RET_CODE_EXP(PrlApi_CreateOpTypeList(hOpTypeList.GetHandlePtr(), sizeof(PRL_UINT32)))
	CHECK_RET_CODE_EXP(PrlOpTypeList_AddItem(hOpTypeList, (PRL_CONST_VOID_PTR)&nTestData))
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlOpTypeList_GetItem(hOpTypeList, 0, 0), PRL_ERR_INVALID_ARG)
}

void PrlApiBasicsTest::testGetItemFromOpTypeListOnOutOfRangeIndex()
{
	SdkHandleWrap hOpTypeList;
	CHECK_RET_CODE_EXP(PrlApi_CreateOpTypeList(hOpTypeList.GetHandlePtr(), sizeof(PRL_UINT32)))
	PRL_UINT32 nItem = 0;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlOpTypeList_GetItem(hOpTypeList, 0, &nItem), PRL_ERR_INVALID_ARG)
}

void PrlApiBasicsTest::testGetItemFromOpTypeListOnOutOfRangeIndex2()
{
	PRL_UINT32 nTestData = 0;
	SdkHandleWrap hOpTypeList;
	CHECK_RET_CODE_EXP(PrlApi_CreateOpTypeList(hOpTypeList.GetHandlePtr(), sizeof(PRL_UINT32)))
	CHECK_RET_CODE_EXP(PrlOpTypeList_AddItem(hOpTypeList, (PRL_CONST_VOID_PTR)&nTestData))
	PRL_UINT32 nItem = 0;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlOpTypeList_GetItem(hOpTypeList, 1, &nItem), PRL_ERR_INVALID_ARG)
}

#define CHECK_OP_TYPE_LIST_ON_BOUND_VALUES(type, min_value, max_value, middle_value1, middle_value2)\
	{\
		type nMaxValue=max_value, nMinValue=min_value, nMiddleValue1=middle_value1, nMiddleValue2=middle_value2;\
		SdkHandleWrap hOpTypeList;\
		CHECK_RET_CODE_EXP(PrlApi_CreateOpTypeList(hOpTypeList.GetHandlePtr(), sizeof(type)))\
		CHECK_RET_CODE_EXP(PrlOpTypeList_AddItem(hOpTypeList, (PRL_CONST_VOID_PTR)&nMinValue))\
		CHECK_RET_CODE_EXP(PrlOpTypeList_AddItem(hOpTypeList, (PRL_CONST_VOID_PTR)&nMiddleValue1))\
		CHECK_RET_CODE_EXP(PrlOpTypeList_AddItem(hOpTypeList, (PRL_CONST_VOID_PTR)&nMiddleValue2))\
		CHECK_RET_CODE_EXP(PrlOpTypeList_AddItem(hOpTypeList, (PRL_CONST_VOID_PTR)&nMaxValue))\
		CHECK_ITEM_PRESENTS_OP_TYPE(type, hOpTypeList, nMinValue)\
		CHECK_ITEM_PRESENTS_OP_TYPE(type, hOpTypeList, nMiddleValue1)\
		CHECK_ITEM_PRESENTS_OP_TYPE(type, hOpTypeList, nMiddleValue2)\
		CHECK_ITEM_PRESENTS_OP_TYPE(type, hOpTypeList, nMaxValue)\
	}

#ifndef _WIN_
#       ifndef ULLONG_MAX
#          define ULLONG_MAX  0xffffffffffffffffLL
#       endif
#       ifndef LLONG_MAX
#          define LLONG_MAX   0x7fffffffffffffffLL
#       endif
#       ifndef LLONG_MIN
#          define LLONG_MIN   0x8000000000000000LL
#       endif
#endif

void PrlApiBasicsTest::testOpTypeListOnBoundValues()
{
	CHECK_OP_TYPE_LIST_ON_BOUND_VALUES(PRL_CHAR, CHAR_MIN, CHAR_MAX, -1, 1)
	CHECK_OP_TYPE_LIST_ON_BOUND_VALUES(PRL_UINT8, 0, UCHAR_MAX, UCHAR_MAX/2-1, UCHAR_MAX/2+1)
	CHECK_OP_TYPE_LIST_ON_BOUND_VALUES(PRL_INT8, SCHAR_MIN, SCHAR_MAX, -1, 1)
	CHECK_OP_TYPE_LIST_ON_BOUND_VALUES(PRL_UINT16, 0, USHRT_MAX, USHRT_MAX/2-1, USHRT_MAX/2+1)
	CHECK_OP_TYPE_LIST_ON_BOUND_VALUES(PRL_INT16, SHRT_MIN, SHRT_MAX, -1, 1)
	CHECK_OP_TYPE_LIST_ON_BOUND_VALUES(PRL_UINT32, 0, UINT_MAX, UINT_MAX/2-1, UINT_MAX/2+1)
	CHECK_OP_TYPE_LIST_ON_BOUND_VALUES(PRL_INT32, INT_MIN, INT_MAX, -1, 1)
	CHECK_OP_TYPE_LIST_ON_BOUND_VALUES(PRL_UINT64, 0ull, ULLONG_MAX, ULLONG_MAX/2ull-1ull, ULLONG_MAX/2ull+1ull)
	CHECK_OP_TYPE_LIST_ON_BOUND_VALUES(PRL_INT64, LLONG_MIN, LLONG_MAX, -1ll, 1ll)
}

void PrlApiBasicsTest::testOpTypeListGetTypeSize()
{
	SdkHandleWrap hOpTypeList;
	CHECK_RET_CODE_EXP(PrlApi_CreateOpTypeList(hOpTypeList.GetHandlePtr(), sizeof(PRL_UINT32)))
	PRL_SIZE nTypeSize = 0;
	CHECK_RET_CODE_EXP(PrlOpTypeList_GetTypeSize(hOpTypeList, &nTypeSize))
	QCOMPARE(quint32(nTypeSize), quint32(sizeof(PRL_UINT32)));
}

void PrlApiBasicsTest::testOpTypeListGetTypeSizeOnNullListHandle()
{
	PRL_SIZE nTypeSize = 0;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlOpTypeList_GetTypeSize(PRL_INVALID_HANDLE, &nTypeSize), PRL_ERR_INVALID_ARG)
}

void PrlApiBasicsTest::testOpTypeListGetTypeSizeOnWrongListHandle()
{
	PRL_SIZE nTypeSize = 0;
	SdkHandleWrap hHandlesList;
	CHECK_RET_CODE_EXP(PrlApi_CreateHandlesList(hHandlesList.GetHandlePtr()))
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlOpTypeList_GetTypeSize(hHandlesList, &nTypeSize), PRL_ERR_INVALID_ARG)
}

void PrlApiBasicsTest::testOpTypeListGetTypeSizeOnNullPointer()
{
	SdkHandleWrap hOpTypeList;
	CHECK_RET_CODE_EXP(PrlApi_CreateOpTypeList(hOpTypeList.GetHandlePtr(), sizeof(PRL_UINT32)))
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlOpTypeList_GetTypeSize(hOpTypeList, 0), PRL_ERR_INVALID_ARG)
}

#define CHECK_GUEST_TYPE(guest_type, presence_sign)\
	{\
		SdkHandleWrap hGuestsTypesList;\
		CHECK_RET_CODE_EXP(PrlApi_GetSupportedOsesTypes(PHO_UNKNOWN, hGuestsTypesList.GetHandlePtr()))\
		PRL_SIZE nTypeSize = 0;\
		CHECK_RET_CODE_EXP(PrlOpTypeList_GetTypeSize(hGuestsTypesList, &nTypeSize))\
		QCOMPARE(quint32(nTypeSize), quint32(sizeof(PRL_UINT8)));\
		PRL_UINT32 nItemsCount = 0;\
		CHECK_RET_CODE_EXP(PrlOpTypeList_GetItemsCount(hGuestsTypesList, &nItemsCount))\
		bool bFound = false;\
		for (PRL_UINT32 i = 0; i < nItemsCount; ++i)\
		{\
			PRL_UINT8 nOsType = 0;\
			CHECK_RET_CODE_EXP(PrlOpTypeList_GetItem(hGuestsTypesList, i, &nOsType))\
			if (guest_type == nOsType)\
			{\
				bFound = true;\
				break;\
			}\
		}\
		QVERIFY(bFound == presence_sign);\
	}

#define CHECK_GUEST_TYPE_PRESENTS(guest_type)\
	CHECK_GUEST_TYPE(guest_type, true)

#define CHECK_GUEST_TYPE_NOT_PRESENTS(guest_type)\
	CHECK_GUEST_TYPE(guest_type, false)

void PrlApiBasicsTest::testGetSupportedOsesTypes()
{
	//Seems Windows guest type will be always supported
	CHECK_GUEST_TYPE_PRESENTS(PVS_GUEST_TYPE_WINDOWS)
}

void PrlApiBasicsTest::testGetSupportedOsesTypesOnWrongParams()
{
	SdkHandleWrap hOsesTypesList;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlApi_GetSupportedOsesTypes(PHO_UNKNOWN, 0), PRL_ERR_INVALID_ARG)
}

#define CHECK_GUEST_VER(guest_type, guest_ver, presence_sign)\
	{\
		SdkHandleWrap hGuestsList;\
		CHECK_RET_CODE_EXP(PrlApi_GetSupportedOsesVersions(PHO_UNKNOWN, guest_type, hGuestsList.GetHandlePtr()))\
		PRL_SIZE nTypeSize = 0;\
		CHECK_RET_CODE_EXP(PrlOpTypeList_GetTypeSize(hGuestsList, &nTypeSize))\
		QCOMPARE(quint32(nTypeSize), quint32(sizeof(PRL_UINT16)));\
		PRL_UINT32 nItemsCount = 0;\
		CHECK_RET_CODE_EXP(PrlOpTypeList_GetItemsCount(hGuestsList, &nItemsCount))\
		bool bFound = false;\
		for (PRL_UINT32 i = 0; i < nItemsCount; ++i)\
		{\
			PRL_UINT16 nOsVer = 0;\
			CHECK_RET_CODE_EXP(PrlOpTypeList_GetItem(hGuestsList, i, &nOsVer))\
			if (guest_ver == nOsVer)\
			{\
				bFound = true;\
				break;\
			}\
		}\
		QVERIFY(bFound == presence_sign);\
	}

#define CHECK_GUEST_VER_PRESENTS(guest_type, guest_ver)\
	CHECK_GUEST_VER(guest_type, guest_ver, true)

#define CHECK_GUEST_VER_NOT_PRESENTS(guest_type, guest_ver)\
	CHECK_GUEST_VER(guest_type, guest_ver, false)

void PrlApiBasicsTest::testGetSupportedOsesVersions()
{
	//I guess Windows XP will be supported quite a long time
	CHECK_GUEST_VER_PRESENTS(PVS_GUEST_TYPE_WINDOWS, PVS_GUEST_VER_WIN_XP)
}

void PrlApiBasicsTest::testGetSupportedOsesVersionsOnWrongParams()
{
	SdkHandleWrap hGuestsList;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlApi_GetSupportedOsesVersions(PHO_UNKNOWN, PVS_GUEST_TYPE_WINDOWS, 0), PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlApi_GetSupportedOsesVersions(PHO_UNKNOWN, 0, hGuestsList.GetHandlePtr()), PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlApi_GetSupportedOsesVersions(PHO_UNKNOWN, PVS_GUEST_TYPE_OTHER-1, hGuestsList.GetHandlePtr()), PRL_ERR_INVALID_ARG)
}

#define CHECK_DEFAULT_OS_VERSION(guest_type, guest_ver)\
	{\
		PRL_UINT16 nOsVer = 0;\
		CHECK_RET_CODE_EXP(PrlApi_GetDefaultOsVersion(guest_type, &nOsVer))\
		QCOMPARE(quint32(nOsVer), quint32(guest_ver));\
	}

void PrlApiBasicsTest::testGetDefaultOsVersion()
{
	CHECK_DEFAULT_OS_VERSION( PVS_GUEST_TYPE_FREEBSD, PVS_GUEST_VER_BSD_7X )
	CHECK_DEFAULT_OS_VERSION( PVS_GUEST_TYPE_OS2, PVS_GUEST_VER_OS2_WARP45 )
	CHECK_DEFAULT_OS_VERSION( PVS_GUEST_TYPE_NETWARE, PVS_GUEST_VER_NET_5X )
	CHECK_DEFAULT_OS_VERSION( PVS_GUEST_TYPE_SOLARIS, PVS_GUEST_VER_SOL_10 )
	CHECK_DEFAULT_OS_VERSION( PVS_GUEST_TYPE_MSDOS, PVS_GUEST_VER_DOS_MS622 )
	CHECK_DEFAULT_OS_VERSION( PVS_GUEST_TYPE_ANDROID, PVS_GUEST_VER_ANDROID_4_0 )
	CHECK_DEFAULT_OS_VERSION( PVS_GUEST_TYPE_OTHER, PVS_GUEST_VER_OTH_OPENSTEP )
}

void PrlApiBasicsTest::testGetDefaultOsVersionOnWrongParams()
{
	PRL_UINT16 nOsVer = 0;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlApi_GetDefaultOsVersion(0, &nOsVer), PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlApi_GetDefaultOsVersion(PVS_GUEST_TYPE_OTHER-1, &nOsVer), PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlApi_GetDefaultOsVersion(PVS_GUEST_TYPE_WINDOWS, 0), PRL_ERR_INVALID_ARG)
}

