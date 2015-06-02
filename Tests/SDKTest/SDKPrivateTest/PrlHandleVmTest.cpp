/////////////////////////////////////////////////////////////////////////////
///
///	@file PrlHandleVmTest.cpp
///
///	This file is the part of parallels public SDK library private tests suite.
///	Tests fixture class for testing VM handle.
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
#include "PrlHandleVmTest.h"

#include "SDK/Handles/Vm/PrlHandleVm.h"
#include "SDK/Handles/Disp/PrlHandleServer.h"
#include "SDK/Wrappers/SdkWrap/SdkHandleWrap.h"
#include "Tests/CommonTestsUtils.h"

#include <QFile>
#include <QTextStream>
#include <QDomDocument>

PrlHandleVmTest::PrlHandleVmTest()
{}

void PrlHandleVmTest::cleanup()
{
	m_Handle.reset();
}

#define CLEANUP_VM_EVENT_UNIQUE_FIELDS_BEFORE_COMPARE( _vmConfig ) \
{ \
	CVmEvent* pVmEvent = _vmConfig.getVmSettings() \
		->getVmRuntimeOptions()->getInternalVmInfo()->getParallelsEvent();\
	pVmEvent->setInitRequestId(); \
	pVmEvent->setEventIssuerId(); \
	_vmConfig.getVmIdentification()->setVmUptimeStartDateTime(); \
	_vmConfig.getVmIdentification()->setVmUptimeInSeconds(); \
}

void PrlHandleVmTest::testfromString()
{
	PrlHandleServer *pServer = new PrlHandleServer;
	SdkHandleWrap hServer(pServer->GetHandle());
	PrlHandleVm *pVm = new PrlHandleVm(PrlHandleServerPtr(pServer));
	m_Handle.reset(pVm->GetHandle());
	QFile _file("./TestDspCmdDirValidateVmConfig_vm_config.xml");
	QVERIFY(_file.open(QIODevice::ReadOnly));
	QTextStream _stream(&_file);
	QString _config = _stream.readAll();
	QVERIFY(PrlVm_FromString(m_Handle, _config.toUtf8().data()) == PRL_ERR_SUCCESS);
	CVmConfiguration _vm_conf(_config);
	QDomDocument _d1, _d2;

	CLEANUP_VM_EVENT_UNIQUE_FIELDS_BEFORE_COMPARE( _vm_conf );
	CLEANUP_VM_EVENT_UNIQUE_FIELDS_BEFORE_COMPARE( pVm->GetVmConfig() );

	_d1.setContent(_vm_conf.toString(), false);
	_d2.setContent(pVm->GetVmConfig().toString(), false);
	if (_d1.toString() != _d2.toString())
	{
		PUT_RAW_MESSAGE( "======CONFIG1:\n" );
		PUT_RAW_MESSAGE(  _d1.toString().toUtf8().constData() );
		PUT_RAW_MESSAGE( "\n======CONFIG2:\n" );
		PUT_RAW_MESSAGE(  _d2.toString().toUtf8().constData() );
		PUT_RAW_MESSAGE( "\n" );
	}
	QVERIFY(_d1.toString() == _d2.toString());
}
