/*
 * Copyright (c) 2015 Parallels IP Holdings GmbH
 *
 * This file is part of Virtuozzo Core. Virtuozzo Core is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License as published by the Free Software Foundation;
 * either version 2 of the License, or (at your option) any later
 * version.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 * Our contact details: Parallels IP Holdings GmbH, Vordergasse 59, 8200
 * Schaffhausen, Switzerland.
 */


#include <QtTest>

#include "IOProtocol.h"

using namespace IOService;

/*****************************************************************************/

class IOProtocolTest : public QObject
{
    Q_OBJECT

private slots:
    void createInstances ();
    void readWriteToStream ();
    void readWriteToBuffer ();
    void checksumCheck ();
};

/*****************************************************************************/

void IOProtocolTest::createInstances ()
{
    for ( uint i = 0; i <= 1000; ++i ) {
        SmartPtr<IOPackage> pkg1 =
            IOPackage::createInstance( 0, i );
        QVERIFY( pkg1.isValid() );
        QVERIFY( pkg1->header.buffersNumber == i );

        // Create pkg uuid
        Uuid::createUuid( pkg1->header.uuid );

        for ( uint buffNum = 0; buffNum < i; ++buffNum ) {
            QString uuid = Uuid::createUuid().toString();
            pkg1->fillBuffer( buffNum, IOPackage::RawEncoding,
                              uuid.toAscii().data(), uuid.length() + 1 );
        }

        SmartPtr<IOPackage> pkg2 = IOPackage::duplicateInstance(pkg1);
        QVERIFY( pkg2.isValid() );
        QVERIFY( pkg2->header.buffersNumber == i );

        for ( uint buffNum = 0; buffNum < i; ++buffNum ) {
            SmartPtr<char> buff1, buff2;
            quint32 size1, size2;
            IOPackage::EncodingType enc1, enc2;
            pkg1->getBuffer(buffNum, enc1, buff1, size1);
            pkg2->getBuffer(buffNum, enc2, buff2, size2);

            QVERIFY( enc1 == enc2 );
            QVERIFY( size1 == size2 );
            QVERIFY( buff1.getImpl() == buff2.getImpl() );
        }

        // Deep copy
        pkg2 = IOPackage::duplicateInstance(pkg1, true);
        QVERIFY( pkg2->header.buffersNumber == i );

        for ( uint buffNum = 0; buffNum < i; ++buffNum ) {
            SmartPtr<char> buff1, buff2;
            quint32 size1, size2;
            IOPackage::EncodingType enc1, enc2;
            pkg1->getBuffer(buffNum, enc1, buff1, size1);
            pkg2->getBuffer(buffNum, enc2, buff2, size2);

            QVERIFY( enc1 == enc2 );
            QVERIFY( size1 == size2 );
            QVERIFY( buff1.getImpl() != buff2.getImpl() );
            QVERIFY( 0 == memcmp(buff1.getImpl(), buff2.getImpl(), size1) );
        }

        // Recreate
        pkg1 = IOPackage::createInstance( 0, 0 );
        Uuid::createUuid( pkg1->header.uuid );
        Uuid::createUuid( pkg1->header.senderUuid );
        pkg2 = IOPackage::createInstance( 0, 0 );
        Uuid::createUuid( pkg2->header.uuid );
        Uuid::createUuid( pkg2->header.senderUuid );
        QVERIFY( Uuid::toUuid(pkg1->header.parentUuid).isNull() );
        QVERIFY( Uuid::toUuid(pkg2->header.parentUuid).isNull() );
        QVERIFY( Uuid::toUuid(pkg1->header.receiverUuid).isNull() );
        QVERIFY( Uuid::toUuid(pkg2->header.receiverUuid).isNull() );
        QVERIFY( Uuid::toUuid(pkg1->header.uuid) !=
                 Uuid::toUuid(pkg2->header.uuid) );
        QVERIFY( Uuid::toUuid(pkg1->header.senderUuid) !=
                 Uuid::toUuid(pkg2->header.senderUuid) );


        // Create response
        QVERIFY( pkg1->isResponsePackage() == false );
        QVERIFY( pkg2->isResponsePackage() == false );
        pkg2->makeBroadcastResponse( pkg1 );

        QVERIFY( pkg1->isResponsePackage() == false );
        QVERIFY( pkg2->isResponsePackage() == true );
        QVERIFY( Uuid::toUuid(pkg2->header.parentUuid) ==
                 Uuid::toUuid(pkg1->header.uuid) );
        QVERIFY( Uuid::toUuid(pkg1->header.receiverUuid).isNull() );
        QVERIFY( Uuid::toUuid(pkg2->header.receiverUuid).isNull() );

        pkg1->makeDirectResponse( pkg2 );
        QVERIFY( pkg1->isResponsePackage() == true );
        QVERIFY( pkg2->isResponsePackage() == true );
        QVERIFY( Uuid::toUuid(pkg2->header.parentUuid) ==
                 Uuid::toUuid(pkg1->header.uuid) );
        QVERIFY( Uuid::toUuid(pkg1->header.parentUuid) ==
                 Uuid::toUuid(pkg2->header.uuid) );
        QVERIFY( ! Uuid::toUuid(pkg1->header.receiverUuid).isNull() );
        QVERIFY( Uuid::toUuid(pkg2->header.receiverUuid).isNull() );
        QVERIFY( Uuid::toUuid(pkg1->header.receiverUuid) ==
                 Uuid::toUuid(pkg2->header.senderUuid) );

    }
}

void IOProtocolTest::readWriteToStream ()
{
    for ( uint i = 0; i <= 1000; ++i ) {
        SmartPtr<IOPackage> pkg1 =
            IOPackage::createInstance( 0, i );
        QVERIFY( pkg1->header.buffersNumber == i );

        // Create pkg uuid
        Uuid::createUuid( pkg1->header.uuid );

        quint32 size = 0;

        for ( uint buffNum = 0; buffNum < i; ++buffNum ) {
            QString uuid = Uuid::createUuid().toString();
            pkg1->fillBuffer( buffNum, IOPackage::RawEncoding,
                              uuid.toAscii().data(), uuid.length() + 1 );
            size += (uuid.length() + 1);
        }
        QVERIFY( pkg1->header.buffersNumber == i );

        QVERIFY( size == pkg1->buffersSize() );

        QByteArray ba1, ba2;
        QBuffer buffer1( &ba1 );
        QBuffer buffer2( &ba2 );
        buffer1.open( QIODevice::ReadWrite );
        buffer2.open( QIODevice::ReadWrite );
        QDataStream in1( &buffer1 );
        QDataStream in2( &buffer2 );
        in1.setVersion(QDataStream::Qt_4_0);
        in2.setVersion(QDataStream::Qt_4_0);

        pkg1->writeToStream(in1);
        buffer1.seek(0);

        SmartPtr<IOPackage> pkg2 = IOPackage::createInstance(in1);
        QVERIFY( pkg2->header.buffersNumber == i );
        QVERIFY( size == pkg2->buffersSize() );
        buffer1.seek(0);

        pkg2->writeToStream(in2);
        buffer2.seek(0);

        QVERIFY( ba1.size() == ba2.size() );
        QVERIFY( 0 == memcmp(ba1.data(), ba2.data(), ba1.size()) );

        for ( uint buffNum = 0; buffNum < i; ++buffNum ) {
            SmartPtr<char> buff1, buff2;
            quint32 size1, size2;
            IOPackage::EncodingType enc1, enc2;
            pkg1->getBuffer(buffNum, enc1, buff1, size1);
            pkg2->getBuffer(buffNum, enc2, buff2, size2);

            QVERIFY( 0 == memcmp(&pkg1->header, &pkg2->header,
                                 sizeof(IOPackage::PODHeader)) );

            QVERIFY( enc1 == enc2 );
            QVERIFY( size1 == size2 );
            QVERIFY( 0 == memcmp(buff1.getImpl(), buff2.getImpl(), size1) );
        }

        // Create response
        QVERIFY( pkg1->isResponsePackage() == false );
        QVERIFY( pkg2->isResponsePackage() == false );

        pkg2 = IOPackage::createInstance( in1, pkg1 );
        buffer1.seek(0);
        QVERIFY( pkg1->isResponsePackage() == false );
        QVERIFY( pkg2->isResponsePackage() == true );

        QVERIFY( ba1.size() == ba2.size() );
        QVERIFY( 0 == memcmp(ba1.data(), ba2.data(), ba1.size()) );

        for ( uint buffNum = 0; buffNum < i; ++buffNum ) {
            SmartPtr<char> buff1, buff2;
            quint32 size1, size2;
            IOPackage::EncodingType enc1, enc2;
            pkg1->getBuffer(buffNum, enc1, buff1, size1);
            pkg2->getBuffer(buffNum, enc2, buff2, size2);

            QVERIFY( 0 != memcmp(&pkg1->header, &pkg2->header,
                                 sizeof(IOPackage::PODHeader)) );

            QVERIFY( 0 == memcmp(pkg1->header.uuid, pkg2->header.parentUuid,
                                 sizeof(Uuid_t)) );


            QVERIFY( enc1 == enc2 );
            QVERIFY( size1 == size2 );
            QVERIFY( 0 == memcmp(buff1.getImpl(), buff2.getImpl(), size1) );
        }
    }
}

void IOProtocolTest::readWriteToBuffer ()
{
    for ( uint i = 0; i <= 1000; ++i ) {
        SmartPtr<IOPackage> pkg1 =
            IOPackage::createInstance( 0, i );
        QVERIFY( pkg1->header.buffersNumber == i );

        // Create pkg uuid
        Uuid::createUuid( pkg1->header.uuid );

        quint32 size = 0;

        for ( uint buffNum = 0; buffNum < i; ++buffNum ) {
            QString uuid = Uuid::createUuid().toString();
            pkg1->fillBuffer( buffNum, IOPackage::RawEncoding,
                              uuid.toAscii().data(), uuid.length() + 1 );
            size += (uuid.length() + 1);
        }
        QVERIFY( pkg1->header.buffersNumber == i );

        QVERIFY( size == pkg1->buffersSize() );

        SmartPtr<char> b1;
        SmartPtr<char> b2;
        quint32 s1;
        quint32 s2;

        b1 = pkg1->toBuffer(s1);

        SmartPtr<IOPackage> pkg2 = IOPackage::createInstance(b1, s1);
        QVERIFY( pkg2->header.buffersNumber == i );
        QVERIFY( size == pkg2->buffersSize() );

        b2 = pkg2->toBuffer(s2);

        QVERIFY( s1 == s2 );
        QVERIFY( 0 == memcmp(b1.getImpl(), b2.getImpl(), s1) );

        for ( uint buffNum = 0; buffNum < i; ++buffNum ) {
            SmartPtr<char> buff1, buff2;
            quint32 size1, size2;
            IOPackage::EncodingType enc1, enc2;
            pkg1->getBuffer(buffNum, enc1, buff1, size1);
            pkg2->getBuffer(buffNum, enc2, buff2, size2);

            QVERIFY( 0 == memcmp(&pkg1->header, &pkg2->header,
                                 sizeof(IOPackage::PODHeader)) );

            QVERIFY( enc1 == enc2 );
            QVERIFY( size1 == size2 );
            QVERIFY( 0 == memcmp(buff1.getImpl(), buff2.getImpl(), size1) );
        }

        // Create response
        QVERIFY( pkg1->isResponsePackage() == false );
        QVERIFY( pkg2->isResponsePackage() == false );

        pkg2 = IOPackage::createInstance( b1, s1, pkg1 );
        QVERIFY( pkg1->isResponsePackage() == false );
        QVERIFY( pkg2->isResponsePackage() == true );

        for ( uint buffNum = 0; buffNum < i; ++buffNum ) {
            SmartPtr<char> buff1, buff2;
            quint32 size1, size2;
            IOPackage::EncodingType enc1, enc2;
            pkg1->getBuffer(buffNum, enc1, buff1, size1);
            pkg2->getBuffer(buffNum, enc2, buff2, size2);

            QVERIFY( 0 != memcmp(&pkg1->header, &pkg2->header,
                                 sizeof(IOPackage::PODHeader)) );

            QVERIFY( 0 == memcmp(pkg1->header.uuid, pkg2->header.parentUuid,
                                 sizeof(Uuid_t)) );


            QVERIFY( enc1 == enc2 );
            QVERIFY( size1 == size2 );
            QVERIFY( 0 == memcmp(buff1.getImpl(), buff2.getImpl(), size1) );
        }
    }
}

void IOProtocolTest::checksumCheck ()
{
    const quint32 PkgType = 123;
    const quint32 BuffsNum = 5;

    SmartPtr<IOPackage> pkg1 =
        IOPackage::createInstance( PkgType, BuffsNum );

    // Create pkg uuid
    Uuid::createUuid( pkg1->header.uuid );
    // Create pkg parentUuid
    Uuid::createUuid( pkg1->header.parentUuid );
    // Create pkg senderUuid
    Uuid::createUuid( pkg1->header.senderUuid );
    // Create pkg receiverUuid
    Uuid::createUuid( pkg1->header.receiverUuid );

    quint16 crc16_1 = pkg1->headerChecksumCRC16();
    quint16 crc16_2 = qChecksum( reinterpret_cast<const char*>(&pkg1->header),
                                 sizeof(IOPackage::PODHeader) - sizeof(quint16) );
    QVERIFY( crc16_1 == crc16_2 );

    // Change CRC16. This should NOT affect
    pkg1->header.crc16 = crc16_1;
    crc16_2 = qChecksum( reinterpret_cast<const char*>(&pkg1->header),
                         sizeof(IOPackage::PODHeader) - sizeof(quint16) );
    QVERIFY( crc16_1 == crc16_2 );

    // Change other members. This SHOULD affect
    quint32 buffsNum = pkg1->header.buffersNumber;
    pkg1->header.buffersNumber = BuffsNum + 1;
    crc16_2 = qChecksum( reinterpret_cast<const char*>(&pkg1->header),
                         sizeof(IOPackage::PODHeader) - sizeof(quint16)  );

    QVERIFY( crc16_1 != crc16_2 );

    // Now CRC16 should be the same
    pkg1->header.buffersNumber = buffsNum;
    crc16_2 = qChecksum( reinterpret_cast<const char*>(&pkg1->header),
                         sizeof(IOPackage::PODHeader) - sizeof(quint16) );
    QVERIFY( crc16_1 == crc16_2 );
}

/*****************************************************************************/

int main ( int argc, char *argv[] )
{
    QCoreApplication a(argc, argv);
    IOProtocolTest test;
    return QTest::qExec(&test, argc, argv);
}

/*****************************************************************************/
#include "IOProtocolTest.moc"
