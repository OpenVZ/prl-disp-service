/////////////////////////////////////////////////////////////////////////////
///
///	@file IOTCPControlBlockStatTest.cpp
///
///	This file is the part of Parallels IO service tests suite.
///	Tests fixture class for testing TCP control block statistics parsing mech.
///
///	@author sandro
///
/// Copyright (c) 1999-2015 Parallels IP Holdings GmbH
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

#include "IOTCPControlBlockStatTest.h"

#include "IOTCPControlBlockStat.h"

#include <QFile>

using namespace IOService;

void IOTCPControlBlockStatTest::testTCPCBForMac()
{
#ifndef _MAC_
	QSKIP("Doesn't make sense on non Mac platform", SkipAll);
#endif
	QFile _file("./tcpcontrolblockstattest_mac.dat");
	QVERIFY(_file.open(QIODevice::ReadOnly));
	QByteArray _buf = _file.readAll();
	IOTCPControlBlockStat _stat = IOTCPControlBlockStat::getStatForLocalPort(56273, _buf);
	QCOMPARE((quint32)1600/32, _stat.m_rtt);
	QCOMPARE((quint32)9600, _stat.m_sndwnd);
}

int main ( int argc, char *argv[] )
{
    QCoreApplication a(argc, argv);
    IOTCPControlBlockStatTest test;
    return QTest::qExec(&test, argc, argv);
}

