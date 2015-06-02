/////////////////////////////////////////////////////////////////////////////
///
///	@file JobsManipulationsTest.h
///
///	This file is the part of parallels public SDK library tests suite.
///	Tests fixture class for testing jobs manipulations SDK API.
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
#ifndef JobsManipulationsTest_H
#define JobsManipulationsTest_H

#include <QtTest/QtTest>

#include "SDK/Wrappers/SdkWrap/SdkHandleWrap.h"

class JobsManipulationsTest : public QObject
{

Q_OBJECT

public:
	JobsManipulationsTest();

private slots:
	void cleanup();
	void testJobWaitOnInvalidJobHandle();
	void testJobWaitOnWrongJobHandle();
	void testJobWaitOnNonJobHandle();
	void testJobGetRetCodeOnInvalidJobHandle();
	void testJobGetRetCodeOnWrongJobHandle();
	void testJobGetRetCodeOnNonJobHandle();
	void testEnumerateEventParams();
	void testEventParamGetName();
	void testEventParamGetNameNotEnoughBufSize();
	void testEventParamGetNameNullBufSize();
	void testEventParamToString();
	void testEventParamToStringNotEnoughBufSize();
	void testEventParamToStringNullBufSize();
	void testEventGetErrCode();
	void testEventGetErrString();
	void testEventIsAnswerRequired();
	void testEventCanBeIgnored();
	void testEventGetIssuerType();
	void testEventGetIssuerId();
	void testEventParamToUint32();
	void testEventParamToInt32();
	void testEventParamToUint64();
	void testEventParamToUint64OnWrongParams();
	void testEventParamToInt64();
	void testEventParamToInt64OnWrongParams();
	void testEventParamToBoolean();
	void testEventParamToHandleForIncorrectParam();
	void testEventParamToHandleForVmConfigParam();
	void testEventParamToHandleForCommonPrefsParam();
	void testEventParamToHandleForFloppy();
	void testEventParamToHandleForHardDisk();
	void testEventParamToHandleForOpticalDisk();
	void testEventParamToHandleForSerialPort();
	void testEventParamToHandleForParallelPort();
	void testEventParamToHandleForSound();
	void testEventParamToHandleForNetAdapter();
	void testEventParamToHandleForUsbDevice();
	void testEventParamToHandleForGenericPciDevice();
	void testEventParamToHandleForGenericScsiDevice();
	void testEventParamToHandleForDisplayDevice();
	void testJobIsRequestWasSentOnNonSentRequest();
	void testJobIsRequestWasSentOnSentRequest();
	void testJobIsRequestWasSentOnWrongParams();
	void testJobGetPackageIdOnWrongParams();
	void testJobGetPackageId();

private:
	SdkHandleWrap m_JobHandle;
};

#endif
