/////////////////////////////////////////////////////////////////////////////
///
///	@file PrlQuestionsListTest.cpp
///
///	This file is the part of parallels public SDK library private tests suite.
///	Tests fixture class for testing internal SDK questions list object.
///
///	@author sandro
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
/////////////////////////////////////////////////////////////////////////////
#include "PrlQuestionsListTest.h"
#include "SDK/Handles/Core/PrlHandleVmEvent.h"

void PrlQuestionsListTest::cleanup()
{
	m_lstEventsHandles.clear();
}

void PrlQuestionsListTest::testRegisterQuestionObject()
{
	QString sIssuerId = Uuid::createUuid().toString();
	CVmEvent _evt(PET_DSP_EVT_VM_QUESTION, sIssuerId);
	PrlHandleVmEventPtr pExpectedHandle(new PrlHandleVmEvent(PrlHandleServerPtr(0), PET_DSP_EVT_VM_QUESTION, &_evt));
	m_lstEventsHandles.append(SdkHandleWrap(pExpectedHandle->GetHandle()));
	PrlQuestionsList _questions_list;
	_questions_list.RegisterQuestionObject(pExpectedHandle);
	QList<PrlHandleBasePtr> _lst = _questions_list.GetQuestionsList();
	QVERIFY(_lst.size() == 1);
	PrlHandleVmEventPtr pActualHandle =
				PrlHandleVmEventPtr(dynamic_cast<PrlHandleVmEvent *>(_lst.front().getHandle()));
	QVERIFY(pExpectedHandle->GetEventObject().getEventIssuerId() ==\
			pActualHandle->GetEventObject().getEventIssuerId());
	QVERIFY(pActualHandle->GetEventObject().getEventIssuerId() == sIssuerId);
}

void PrlQuestionsListTest::testGetQuestionsByIssuerId()
{
	QString sIssuerId1 = Uuid::createUuid().toString();
	QString sIssuerId2 = Uuid::createUuid().toString();

	PrlQuestionsList _questions_list;

	CVmEvent _evt1(PET_DSP_EVT_VM_QUESTION, sIssuerId1);
	_questions_list.RegisterQuestionObject(
		PrlHandleVmEventPtr(new PrlHandleVmEvent(PrlHandleServerPtr(0), PET_DSP_EVT_VM_QUESTION, &_evt1)));

	CVmEvent _evt2(PET_DSP_EVT_VM_QUESTION, sIssuerId2);
	_questions_list.RegisterQuestionObject(
		PrlHandleVmEventPtr(new PrlHandleVmEvent(PrlHandleServerPtr(0), PET_DSP_EVT_VM_QUESTION, &_evt2)));

	QList<PrlHandleBasePtr> _lst = _questions_list.GetQuestionsList();
	foreach(PrlHandleBasePtr _ptr, _lst)
		m_lstEventsHandles.append(SdkHandleWrap(_ptr->GetHandle()));
	QVERIFY(_lst.size() == 2);

	_lst = _questions_list.GetQuestionsByIssuerId(sIssuerId1);
	QVERIFY(_lst.size() == 1);

	_lst = _questions_list.GetQuestionsByIssuerId(sIssuerId2);
	QVERIFY(_lst.size() == 1);
}

void PrlQuestionsListTest::testUnregisterQuestionObject()
{
	QString sRequestId = Uuid::createUuid().toString();
	CVmEvent _evt(PET_DSP_EVT_VM_QUESTION, Uuid::createUuid().toString());
	_evt.setInitRequestId(sRequestId);
	PrlHandleVmEventPtr pHandle(new PrlHandleVmEvent(PrlHandleServerPtr(0), PET_DSP_EVT_VM_QUESTION, &_evt));
	m_lstEventsHandles.append(SdkHandleWrap(pHandle->GetHandle()));
	PrlQuestionsList _questions_list;
	_questions_list.RegisterQuestionObject(pHandle);
	QList<PrlHandleBasePtr> _lst = _questions_list.GetQuestionsList();
	QVERIFY(_lst.size() == 1);
	_questions_list.UnregisterQuestionObject(sRequestId);
	_lst = _questions_list.GetQuestionsList();
	QVERIFY(_lst.size() == 0);
}
