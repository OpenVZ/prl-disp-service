/////////////////////////////////////////////////////////////////////////////
///
/// Copyright (C) 2006-2015 Parallels IP Holdings GmbH
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
/// @file
///		UuidTest.cpp
///
/// @author
///		romanp
///
/// @brief
///		Uuid test cases.
///
/////////////////////////////////////////////////////////////////////////////

#include <QByteArray>
#include <QBuffer>
#include <QDataStream>

#include "UuidTest.h"

/****************************************************************************/

void UuidTest::creatingUuids ()
{
    QUuid qtUuid;
    Uuid prlUuid;

    // Should be null
    QVERIFY( qtUuid.isNull() );
    QVERIFY( prlUuid.isNull() );

    QString nullUuid("{00000000-0000-0000-0000-000000000000}");
    QString validUuid("{11111111-1111-1111-1111-111111111111}");
    QString invalidUuid("blablablabla-blablabal-blablab-lbalb");

    qtUuid = QUuid(nullUuid);
    prlUuid = Uuid(nullUuid);

    // Should be null
    QVERIFY( qtUuid.isNull() );
    QVERIFY( prlUuid.isNull() );

    qtUuid = QUuid(validUuid);
    prlUuid = Uuid(validUuid);

    // Should be valid
    QVERIFY( ! qtUuid.isNull() );
    QVERIFY( ! prlUuid.isNull() );

    qtUuid = QUuid(invalidUuid);
    prlUuid = Uuid(invalidUuid);

    // Should be null again
    QVERIFY( qtUuid.isNull() );
    QVERIFY( prlUuid.isNull() );

    prlUuid = Uuid::createUuid();
    qtUuid = QUuid::createUuid();

    // Should be valid
    QVERIFY( ! qtUuid.isNull() );
    QVERIFY( ! prlUuid.isNull() );

    // Swap values
    prlUuid = Uuid(qtUuid.toString());
    qtUuid = QUuid(prlUuid.toString());

    // Should be valid
    QVERIFY( ! qtUuid.isNull() );
    QVERIFY( ! prlUuid.isNull() );

    prlUuid = Uuid(validUuid.toLatin1().data());
    qtUuid = QUuid(validUuid.toLatin1().data());

    // Should be valid
    QVERIFY( ! qtUuid.isNull() );
    QVERIFY( ! prlUuid.isNull() );

    const char *nullPtr = 0;

    prlUuid = Uuid(nullPtr);
    qtUuid = QUuid(nullPtr);

    // Should be null
    QVERIFY( qtUuid.isNull() );
    QVERIFY( prlUuid.isNull() );

    Uuid_t uuidT;
    Uuid::createUuid( uuidT );
    prlUuid = Uuid::toUuid( uuidT );

    // Should be valid
    QVERIFY( ! prlUuid.isNull() );

    qtUuid = QUuid( prlUuid.toString() );
    QString prlUuidStr = prlUuid.toString();
    QString qtUuidStr = qtUuid.toString();

    // Should be equal
    QVERIFY( prlUuidStr == qtUuidStr );

    // Generate uuids and check uniqueness
    const int UuidsNum = 100000;
    QHash<QString, int> uuidHash;
    for ( int i = 0; i < UuidsNum; ++i ) {
        Uuid uuid = Uuid::createUuid();
        QString uuidStr = uuid.toString();
        QVERIFY( ! uuidHash.contains(uuid.toString()) );
        uuidHash[uuidStr] = 0;
    }
    QVERIFY( uuidHash.size() == UuidsNum );
}

void UuidTest::comparingUuids ()
{
    Uuid prlU1 = Uuid::createUuid();
    Uuid prlU2 = Uuid::createUuid();
    QUuid qtU1 = QUuid(prlU1.toString());
    QUuid qtU2 = QUuid(prlU2.toString());

    // Should be valid
    QVERIFY( ! prlU1.isNull() );
    QVERIFY( ! prlU2.isNull() );
    QVERIFY( ! qtU1.isNull() );
    QVERIFY( ! qtU2.isNull() );

    // Not unique
    QVERIFY( ! (qtU1 == qtU2) );
    QVERIFY( qtU1 != qtU2 );
    QVERIFY( ! (prlU1 == prlU2) );
    QVERIFY( prlU1 != prlU2 );

    bool firGreater = (qtU1 > qtU2);
    bool firSmaller = (qtU1 < qtU2);

    // Check operator >
    if ( firGreater )
        QVERIFY( prlU1 > prlU2 );
    else
        QVERIFY( prlU2 > prlU1 );

    // Check operator <
    if ( firSmaller )
        QVERIFY( prlU1 < prlU2 );
    else
        QVERIFY( prlU2 < prlU1 );


    // Check on unique uuids
    prlU1 = prlU2;
    qtU1 = qtU2;

    QVERIFY( qtU1 == qtU2 );
    QVERIFY( ! (qtU1 != qtU2) );
    QVERIFY( ! (qtU1 < qtU2) );
    QVERIFY( ! (qtU1 > qtU2) );

    QVERIFY( prlU1 == prlU2 );
    QVERIFY( ! (prlU1 != prlU2) );
    QVERIFY( ! (prlU1 < prlU2) );
    QVERIFY( ! (prlU1 > prlU2) );
}

void UuidTest::castingUuids ()
{
    Uuid prlUuid = Uuid::createUuid();
    QUuid qtUuid = QUuid(prlUuid.toString());

    QString prlStr = prlUuid.toString();
    QString qtStr = qtUuid.toString();

    QVERIFY( prlStr == qtStr );

    prlStr = prlUuid;
    qtStr = qtUuid;

    QVERIFY( prlStr == qtStr );

    Uuid_t prlUuidT;
    prlUuid.dump( prlUuidT );
    QString strFromUuidT = Uuid::toString( prlUuidT );
    Uuid uuidFromUuidT = Uuid::toUuid( prlUuidT );
    Uuid_t newPrlUuidT;
    uuidFromUuidT.dump( newPrlUuidT );

    QVERIFY( 0 == memcmp(prlUuidT, newPrlUuidT, sizeof(prlUuidT)) );
    QVERIFY( strFromUuidT == prlStr );
    QVERIFY( uuidFromUuidT == prlUuid );

    bool res = Uuid::dump( qtStr, prlUuidT );
    QVERIFY( res == true );

    strFromUuidT = Uuid::toString( prlUuidT );
    QVERIFY( strFromUuidT == prlStr );

    res = Uuid::dump( "invalid_uuid_blablabla", prlUuidT );
    QVERIFY( res == false );
}

void UuidTest::streamUuids ()
{
    Uuid prlUuid = Uuid::createUuid();
    QUuid qtUuid = QUuid(prlUuid.toString());

    QByteArray ba1;
    QByteArray ba2;

    QBuffer buff1( &ba1 );
    QBuffer buff2( &ba2 );

    buff1.open( QIODevice::ReadWrite );
    buff2.open( QIODevice::ReadWrite );

    QDataStream stream1( &buff1 );
    QDataStream stream2( &buff2 );

    stream1 << prlUuid;
    stream2 << qtUuid;

    QVERIFY( ba1 == ba2 );
    ba1.clear();
    QVERIFY( ba1 != ba2 );

    ba2.clear();

    buff1.seek(0);
    buff2.seek(0);

    // Once more time
    stream1 << prlUuid;
    stream2 << qtUuid;

    buff1.seek(0);
    buff2.seek(0);

    Uuid prlUuid2;
    QUuid qtUuid2;

    stream1 >> prlUuid2;
    stream2 >> qtUuid2;

    QVERIFY( prlUuid == prlUuid2 );
    QVERIFY( qtUuid == qtUuid2 );
    QVERIFY( qtUuid.toString() == prlUuid.toString() );
}

void UuidTest::obfuscateUuid()
{
	QString uuid = Uuid::createUuid().toString();
	QString obfuscatedUuid = Uuid::obfuscateUuid( uuid );
	QVERIFY( !obfuscatedUuid.isEmpty() );

	//Apply some transformations to UUID string representation
	uuid.remove( '{' );
	uuid.remove( '}' );

	QString upperCase = uuid.toUpper();
	QString lowerCase = uuid.toLower();

	QCOMPARE( obfuscatedUuid, Uuid::obfuscateUuid( upperCase ) );
	QCOMPARE( obfuscatedUuid, Uuid::obfuscateUuid( lowerCase ) );
}

void UuidTest::obfuscateUuidForWrongUuid()
{
	QVERIFY( !Uuid::obfuscateUuid( Uuid::createUuid().toString() ).isEmpty() );
	QVERIFY( Uuid::obfuscateUuid( "" ).isEmpty() );
	QVERIFY( Uuid::obfuscateUuid( "non UUID of same length as expected" ).isEmpty() );
}

void UuidTest::useUuidFormatWithoutDashes()
{
	QCOMPARE(Uuid("271449fb6ab94442966b6b00717974ad").toString(), Uuid("271449fb-6ab9-4442-966b-6b00717974ad").toString());
	QCOMPARE(Uuid("f94e3d8134ed494e8689233cdf8c5dc7").toString(), Uuid("f94e3d81-34ed-494e-8689-233cdf8c5dc7").toString());
	QCOMPARE(Uuid("863cd36b106c4bbc831fc3c33d3f8c7c").toString(), Uuid("863cd36b-106c-4bbc-831f-c3c33d3f8c7c").toString());
}

/****************************************************************************/

QTEST_MAIN(UuidTest)
