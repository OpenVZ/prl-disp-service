/////////////////////////////////////////////////////////////////////////////
///
/// Copyright (c) 1999-2017, Parallels International GmbH
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
/// @file
///		CXmlModelHelperTest.cpp
///
/// @author
///		sandro
///
/// @brief
///		Tests fixture class for testing XML model help primitives.
///
/// @brief
///		None.
///
/////////////////////////////////////////////////////////////////////////////

#include <prlxmlmodel/VmConfig/CVmHardDisk.h>
#include <prlxmlmodel/VmConfig/CVmHardware.h>
#include "CXmlModelHelperTest.h"
#include <prlxmlmodel/ParallelsObjects/CXmlModelHelper.h>
#include <prlcommon/PrlUuid/Uuid.h>
#include <prlcommon/Std/SmartPtr.h>
#include "Tests/CommonTestsUtils.h"

#define FILL_HDD_PROPS(hdd)\
	QString sDeviceName = Uuid::createUuid().toString();\
	hdd.setSystemName( sDeviceName );\
	hdd.setUserFriendlyName( sDeviceName );\
	hdd.setInterfaceType( PMS_SATA_DEVICE );\
	hdd.setConnected( true );\
	hdd.setEnabled( true );\
	hdd.setEmulatedType( PDT_USE_IMAGE_FILE );\
	hdd.setIndex( nIndex++ );\
	hdd.setStackIndex( nStackIndex++ );\
	hdd.setPassthrough( PVE::PassthroughDisabled );

#define ADD_HARD_DISK(pVmHardware, sHdd)\
	{\
		CVmHardDisk *pHdd = new CVmHardDisk;\
		CHECK_RET_CODE_EXP(pHdd->fromString( sHdd ))\
		pVmHardware->m_lstHardDisks.append( pHdd );\
	}

#define PREPARE_HDDS_ELEMENTS(num_of_hdds)\
	SmartPtr<CVmHardware> pVmHardware1( new CVmHardware ), pVmHardware2( new CVmHardware );\
	quint32 nIndex = 0, nStackIndex = 0;\
	for (int i = 0; i < num_of_hdds; ++i)\
	{\
		CVmHardDisk _hdd;\
		FILL_HDD_PROPS(_hdd)\
		QString sHdd = _hdd.toString();\
		ADD_HARD_DISK(pVmHardware1, sHdd)\
		ADD_HARD_DISK(pVmHardware2, sHdd)\
	}\
	pVmHardware1->fromString( pVmHardware2->toString() );/* Initialize XML model items IDs */\
	pVmHardware2->fromString( pVmHardware2->toString() );

#define lstHdds1 pVmHardware1->m_lstHardDisks
#define lstHdds2 pVmHardware2->m_lstHardDisks

void CXmlModelHelperTest::testIsEqualForHdd()
{
	PREPARE_HDDS_ELEMENTS(1)
	QVERIFY(CXmlModelHelper::IsEqual( lstHdds1[ 0 ], lstHdds2[ 0 ] ));
}

void CXmlModelHelperTest::testIsEqualForHddSystemNameChanged()
{
	PREPARE_HDDS_ELEMENTS(1)
	lstHdds1[ 0 ]->setSystemName( Uuid::createUuid().toString() );
	QVERIFY(! CXmlModelHelper::IsEqual( lstHdds1[ 0 ], lstHdds2[ 0 ] ));
}

void CXmlModelHelperTest::testIsEqualForHddFriendlyNameChanged()
{
	PREPARE_HDDS_ELEMENTS(1)
	lstHdds1[ 0 ]->setUserFriendlyName( Uuid::createUuid().toString() );
	QVERIFY(! CXmlModelHelper::IsEqual( lstHdds1[ 0 ], lstHdds2[ 0 ] ));
}

void CXmlModelHelperTest::testIsEqualForHddIfaceChanged()
{
	PREPARE_HDDS_ELEMENTS(1)
	lstHdds1[ 0 ]->setInterfaceType( PMS_SCSI_DEVICE );
	QVERIFY(! CXmlModelHelper::IsEqual( lstHdds1[ 0 ], lstHdds2[ 0 ] ));
}

void CXmlModelHelperTest::testIsEqualForHddEmulatedTypeChanged()
{
	PREPARE_HDDS_ELEMENTS(1)
	lstHdds1[ 0 ]->setEmulatedType( PDT_USE_REAL_DEVICE );
	QVERIFY(! CXmlModelHelper::IsEqual( lstHdds1[ 0 ], lstHdds2[ 0 ] ));
}

void CXmlModelHelperTest::testIsEqualForHddEnabledSignChanged()
{
	PREPARE_HDDS_ELEMENTS(1)
	lstHdds1[ 0 ]->setEnabled( ! lstHdds1[ 0 ]->getEnabled() );
	QVERIFY(! CXmlModelHelper::IsEqual( lstHdds1[ 0 ], lstHdds2[ 0 ] ));
}

void CXmlModelHelperTest::testIsEqualForHddConnectedSignChanged()
{
	PREPARE_HDDS_ELEMENTS(1)
	lstHdds1[ 0 ]->setConnected( ! lstHdds1[ 0 ]->getConnected() );
	QVERIFY(! CXmlModelHelper::IsEqual( lstHdds1[ 0 ], lstHdds2[ 0 ] ));
}

void CXmlModelHelperTest::testIsEqualForHddIndexChanged()
{
	PREPARE_HDDS_ELEMENTS(1)
	lstHdds1[ 0 ]->setIndex( lstHdds1[ 0 ]->getIndex() + 1 );
	QVERIFY(! CXmlModelHelper::IsEqual( lstHdds1[ 0 ], lstHdds2[ 0 ] ));
}

void CXmlModelHelperTest::testIsEqualForHddStackIndexChanged()
{
	PREPARE_HDDS_ELEMENTS(1)
	lstHdds1[ 0 ]->setStackIndex( lstHdds1[ 0 ]->getStackIndex() + 1 );
	QVERIFY(! CXmlModelHelper::IsEqual( lstHdds1[ 0 ], lstHdds2[ 0 ] ));
}

void CXmlModelHelperTest::testIsEqualForHddPassthroughSignChanged()
{
	PREPARE_HDDS_ELEMENTS(1)
	lstHdds1[ 0 ]->setPassthrough( PVE::PassthroughEnabled );
	QVERIFY(! CXmlModelHelper::IsEqual( lstHdds1[ 0 ], lstHdds2[ 0 ] ));
}

void CXmlModelHelperTest::testJustConnectedPropWasChangedForHdd()
{
	PREPARE_HDDS_ELEMENTS(1)
	lstHdds1[ 0 ]->setConnected( ! lstHdds1[ 0 ]->getConnected() );
	QVERIFY(CXmlModelHelper::JustConnectedPropWasChanged( lstHdds1[ 0 ], lstHdds2[ 0 ]));
}

void CXmlModelHelperTest::testJustConnectedPropWasChangedForEqualHdds()
{
	PREPARE_HDDS_ELEMENTS(1)
	QVERIFY(! CXmlModelHelper::JustConnectedPropWasChanged( lstHdds1[ 0 ], lstHdds2[ 0 ]));
}

void CXmlModelHelperTest::testJustConnectedPropWasChangedForHddSystemNameChanged()
{
	PREPARE_HDDS_ELEMENTS(1)
	lstHdds1[ 0 ]->setConnected( ! lstHdds1[ 0 ]->getConnected() );
	lstHdds1[ 0 ]->setSystemName( Uuid::createUuid().toString() );
	QVERIFY(! CXmlModelHelper::JustConnectedPropWasChanged( lstHdds1[ 0 ], lstHdds2[ 0 ]));
}

void CXmlModelHelperTest::testJustConnectedPropWasChangedForHddFriendlyNameChanged()
{
	PREPARE_HDDS_ELEMENTS(1)
	lstHdds1[ 0 ]->setConnected( ! lstHdds1[ 0 ]->getConnected() );
	lstHdds1[ 0 ]->setUserFriendlyName( Uuid::createUuid().toString() );
	QVERIFY(! CXmlModelHelper::JustConnectedPropWasChanged( lstHdds1[ 0 ], lstHdds2[ 0 ]));
}

void CXmlModelHelperTest::testJustConnectedPropWasChangedForHddInterfaceTypeChanged()
{
	PREPARE_HDDS_ELEMENTS(1)
	lstHdds1[ 0 ]->setConnected( ! lstHdds1[ 0 ]->getConnected() );
	lstHdds1[ 0 ]->setInterfaceType( PMS_SCSI_DEVICE );
	QVERIFY(! CXmlModelHelper::JustConnectedPropWasChanged( lstHdds1[ 0 ], lstHdds2[ 0 ]));
}

void CXmlModelHelperTest::testJustConnectedPropWasChangedForHddEmulatedTypeChanged()
{
	PREPARE_HDDS_ELEMENTS(1)
	lstHdds1[ 0 ]->setConnected( ! lstHdds1[ 0 ]->getConnected() );
	lstHdds1[ 0 ]->setEmulatedType( PDT_USE_REAL_DEVICE );
	QVERIFY(! CXmlModelHelper::JustConnectedPropWasChanged( lstHdds1[ 0 ], lstHdds2[ 0 ]));
}

void CXmlModelHelperTest::testJustConnectedPropWasChangedForHddEnabledSignChanged()
{
	PREPARE_HDDS_ELEMENTS(1)
	lstHdds1[ 0 ]->setConnected( ! lstHdds1[ 0 ]->getConnected() );
	lstHdds1[ 0 ]->setEnabled( ! lstHdds1[ 0 ]->getEnabled() );
	QVERIFY(! CXmlModelHelper::JustConnectedPropWasChanged( lstHdds1[ 0 ], lstHdds2[ 0 ]));
}

void CXmlModelHelperTest::testJustConnectedPropWasChangedForHddIndexChanged()
{
	PREPARE_HDDS_ELEMENTS(1)
	lstHdds1[ 0 ]->setConnected( ! lstHdds1[ 0 ]->getConnected() );
	lstHdds1[ 0 ]->setIndex( lstHdds1[ 0 ]->getIndex() + 1 );
	QVERIFY(! CXmlModelHelper::JustConnectedPropWasChanged( lstHdds1[ 0 ], lstHdds2[ 0 ]));
}

void CXmlModelHelperTest::testJustConnectedPropWasChangedForHddStackIndexChanged()
{
	PREPARE_HDDS_ELEMENTS(1)
	lstHdds1[ 0 ]->setConnected( ! lstHdds1[ 0 ]->getConnected() );
	lstHdds1[ 0 ]->setStackIndex( lstHdds1[ 0 ]->getStackIndex() + 1 );
	QVERIFY(! CXmlModelHelper::JustConnectedPropWasChanged( lstHdds1[ 0 ], lstHdds2[ 0 ]));
}

void CXmlModelHelperTest::testJustConnectedPropWasChangedForHddPassthroughSignChanged()
{
	PREPARE_HDDS_ELEMENTS(1)
	lstHdds1[ 0 ]->setConnected( ! lstHdds1[ 0 ]->getConnected() );
	lstHdds1[ 0 ]->setPassthrough( PVE::PassthroughEnabled );
	QVERIFY(! CXmlModelHelper::JustConnectedPropWasChanged( lstHdds1[ 0 ], lstHdds2[ 0 ]));
}

void CXmlModelHelperTest::testIsElemInListWhenElemPresent()
{
	PREPARE_HDDS_ELEMENTS(5)
	for( int i = 0; i < lstHdds1.size(); ++i )
	{
		CVmHardDisk *pElem = CXmlModelHelper::IsElemInList( lstHdds1[ i ], lstHdds2 );
		QVERIFY(pElem);
		QVERIFY(*pElem == *lstHdds1[ i ]);
	}
}

void CXmlModelHelperTest::testIsElemInListConnectedSignChanged()
{
	PREPARE_HDDS_ELEMENTS(5)
	foreach(CVmHardDisk *pHdd, lstHdds1)
		pHdd->setConnected( PVE::DeviceDisconnected );
	foreach(CVmHardDisk *pHdd, lstHdds1)
		QVERIFY(CXmlModelHelper::IsElemInList( pHdd, lstHdds2 ));
}

void CXmlModelHelperTest::testIsElemInListWhenElemAbsent()
{
	PREPARE_HDDS_ELEMENTS(5)
	for( int i = 0; i < lstHdds1.size(); ++i )
	{
		QList<CVmHardDisk *>::iterator _it = lstHdds2.begin();
		for ( ; _it != lstHdds2.end(); ++_it )
		{
			if ( lstHdds1[ i ]->getItemId() == (*_it)->getItemId() )
			{
				delete *_it;
				lstHdds2.erase( _it );
				break;
			}
		}
		QVERIFY(! CXmlModelHelper::IsElemInList( lstHdds1[ i ], lstHdds2 ));
	}
}

void CXmlModelHelperTest::testIsElemInListWhenAllElemsAbsent()
{
	PREPARE_HDDS_ELEMENTS(5)
	QList<CVmHardDisk *> _lst;
	for( int i = 0; i < lstHdds1.size(); ++i )
		QVERIFY(! CXmlModelHelper::IsElemInList( lstHdds1[ i ], _lst ));
}

