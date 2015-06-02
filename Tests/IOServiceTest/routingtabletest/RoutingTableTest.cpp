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


#include <QCoreApplication>
#include <QHash>
#include <QMutex>
#include <QByteArray>
#include <QBuffer>
#include <QDataStream>
#include <QtTest>

#include "IORoutingTable.h"

using namespace IOService;

/*****************************************************************************/

class RoutingTableTest : public QObject
{
    Q_OBJECT

private slots:
    void doAccepting ();
};

namespace PackageType {
enum PackageType {
    FirstType   = 1,
    SecondType  = 2,
    ThirdType   = 3,

    RangeBegin  = 4,
    RangeType   = 6,
    RangeEnd    = 10,

    SomeType    = 100
};
}

IORoutingTable::RouteName First_RouteName = (IORoutingTable::RouteName)1;
IORoutingTable::RouteName Second_RouteName = (IORoutingTable::RouteName)2;
IORoutingTable::RouteName Third_RouteName = (IORoutingTable::RouteName)3;

/*****************************************************************************/

void RoutingTableTest::doAccepting ()
{
    IORoutingTable* table = 0;

    // default    First  -- Optional
    // FirstType  First  -- Optional
    // SecondType Second -- Required
    // ThirdType  Third  -- Optional
    // range      Second -- Required

    IORoutingTable tableSrv( First_RouteName, IORoutingTable::OptionalRoute );
    tableSrv.addRoute( PackageType::FirstType, First_RouteName,
                       IORoutingTable::OptionalRoute );
    tableSrv.addRoute( PackageType::SecondType, Second_RouteName,
                       IORoutingTable::RequiredRoute );
    tableSrv.addRoute( PackageType::ThirdType, Third_RouteName,
                       IORoutingTable::OptionalRoute );
    tableSrv.addRoute( PackageType::RangeBegin,
                       PackageType::RangeEnd,
                       Second_RouteName,
                       IORoutingTable::RequiredRoute );
    table = &tableSrv;
    // default     First
    QVERIFY( table->findRoute(PackageType::SomeType) == First_RouteName );
    // FirstType   First
    QVERIFY( table->findRoute(PackageType::FirstType) == First_RouteName );
    // SecondType  Second
    QVERIFY( table->findRoute(PackageType::SecondType) == Second_RouteName );
    // ThirdType   Third
    QVERIFY( table->findRoute(PackageType::ThirdType) == Third_RouteName );
    // range       Second
    QVERIFY( table->findRoute(PackageType::RangeType) == Second_RouteName );


    // default    First  -- Required (First  -- Required)
    // FirstType  First  -- Optional (First  -- Optional)
    // SecondType Second -- Required (Second -- Required)
    // ThirdType  Second -- Required (Second -- Required)
    // range      First  -- Optional (Second -- Required)

    IORoutingTable tableCli1( First_RouteName, IORoutingTable::RequiredRoute );
    tableCli1.addRoute( PackageType::FirstType, First_RouteName,
                    IORoutingTable::OptionalRoute );
    tableCli1.addRoute( PackageType::SecondType, Second_RouteName,
                    IORoutingTable::RequiredRoute );
    tableCli1.addRoute( PackageType::ThirdType, Second_RouteName,
                    IORoutingTable::RequiredRoute );
    tableCli1.addRoute( PackageType::RangeBegin,
                        PackageType::RangeEnd,
                        First_RouteName,
                    IORoutingTable::OptionalRoute );
    table = &tableCli1;
    // default     First
    QVERIFY( table->findRoute(PackageType::SomeType) == First_RouteName );
    // FirstType   First
    QVERIFY( table->findRoute(PackageType::FirstType) == First_RouteName );
    // SecondType  Second
    QVERIFY( table->findRoute(PackageType::SecondType) == Second_RouteName );
    // ThirdType   Second
    QVERIFY( table->findRoute(PackageType::ThirdType) == Second_RouteName );
    // range       First
    QVERIFY( table->findRoute(PackageType::RangeType) == First_RouteName );




    // default    Second -- Optional (Second -- Optional)
    // FirstType  Third  -- Required (Third  -- Required)
    // SecondType Second -- Optional (Second -- Required)
    // ThirdType  Third  -- Optional (Third  -- Optional)
    // range      Second -- Optional (Second -- Required)

    IORoutingTable tableCli2( Second_RouteName, IORoutingTable::OptionalRoute );
    tableCli2.addRoute( PackageType::FirstType, Third_RouteName,
                        IORoutingTable::RequiredRoute );
    tableCli2.addRoute( PackageType::SecondType, Second_RouteName,
                        IORoutingTable::OptionalRoute );
    tableCli2.addRoute( PackageType::ThirdType, Third_RouteName,
                        IORoutingTable::OptionalRoute );
    tableCli2.addRoute( PackageType::RangeBegin,
                        PackageType::RangeEnd,
                        Second_RouteName,
                        IORoutingTable::OptionalRoute );
    table = &tableCli2;
    // default     Second
    QVERIFY( table->findRoute(PackageType::SomeType) == Second_RouteName );
    // FirstType   Third
    QVERIFY( table->findRoute(PackageType::FirstType) == Third_RouteName );
    // SecondType  Second
    QVERIFY( table->findRoute(PackageType::SecondType) == Second_RouteName );
    // ThirdType   Third
    QVERIFY( table->findRoute(PackageType::ThirdType) == Third_RouteName );
    // range       Second
    QVERIFY( table->findRoute(PackageType::RangeType) == Second_RouteName );



    // default    Third  -- Required (Third  -- Required)
    // FirstType  Third  -- Required (Third  -- Required)
    // SecondType Second -- Required (Second -- Required)
    // ThirdType  First  -- Required (First  -- Required)
    // range      Second -- Required (Second -- Required)

    IORoutingTable tableCli3( Third_RouteName, IORoutingTable::RequiredRoute );
    tableCli3.addRoute( PackageType::FirstType, Third_RouteName,
                        IORoutingTable::RequiredRoute );
    tableCli3.addRoute( PackageType::SecondType, Second_RouteName,
                        IORoutingTable::RequiredRoute );
    tableCli3.addRoute( PackageType::ThirdType, First_RouteName,
                        IORoutingTable::RequiredRoute );
    tableCli3.addRoute( PackageType::RangeBegin,
                        PackageType::RangeEnd,
                        Second_RouteName,
                        IORoutingTable::RequiredRoute );
    table = &tableCli3;
    // default     Third
    QVERIFY( table->findRoute(PackageType::SomeType) == Third_RouteName );
    // FirstType   Third
    QVERIFY( table->findRoute(PackageType::FirstType) == Third_RouteName );
    // SecondType  Second
    QVERIFY( table->findRoute(PackageType::SecondType) == Second_RouteName );
    // ThirdType   First
    QVERIFY( table->findRoute(PackageType::ThirdType) == First_RouteName );
    // range      Second
    QVERIFY( table->findRoute(PackageType::RangeType) == Second_RouteName );


    // default    First  -- Required X (can't accept)

    IORoutingTable tableCli4( First_RouteName, IORoutingTable::RequiredRoute );
    table = &tableCli4;
    // default     First
    QVERIFY( table->findRoute(PackageType::SomeType) == First_RouteName );
    // FirstType   First
    QVERIFY( table->findRoute(PackageType::FirstType) == First_RouteName );
    // SecondType  First
    QVERIFY( table->findRoute(PackageType::SecondType) == First_RouteName );
    // ThirdType   First
    QVERIFY( table->findRoute(PackageType::ThirdType) == First_RouteName );
    // range       First
    QVERIFY( table->findRoute(PackageType::RangeType) == First_RouteName );




    // default    First  -- Optional (First  -- Optional)
    // res:
    // (FirstType                     First  -- Optional)
    // (SecondType                    Second -- Required)
    // (ThirdType                     First  -- Optional)
    // (range                         Second -- Required)

    IORoutingTable tableCli5( First_RouteName, IORoutingTable::OptionalRoute );
    table = &tableCli5;
    // default     First
    QVERIFY( table->findRoute(PackageType::SomeType) == First_RouteName );
    // FirstType   First
    QVERIFY( table->findRoute(PackageType::FirstType) == First_RouteName );
    // SecondType  First
    QVERIFY( table->findRoute(PackageType::SecondType) == First_RouteName );
    // ThirdType   First
    QVERIFY( table->findRoute(PackageType::ThirdType) == First_RouteName );
    // range       First
    QVERIFY( table->findRoute(PackageType::RangeType) == First_RouteName );



    // default    Second  -- Optional (Second  -- Optional)
    // res:
    // (FirstType                      Second  -- Optional)
    // (SecondType                     Second  -- Required)
    // (ThirdType                      Second  -- Optional)
    // (range                          Second  -- Required)

    IORoutingTable tableCli6( Second_RouteName, IORoutingTable::OptionalRoute );
    table = &tableCli6;
    // default     Second
    QVERIFY( table->findRoute(PackageType::SomeType) == Second_RouteName );
    // FirstType   Second
    QVERIFY( table->findRoute(PackageType::FirstType) == Second_RouteName );
    // SecondType  Second
    QVERIFY( table->findRoute(PackageType::SecondType) == Second_RouteName );
    // ThirdType   Second
    QVERIFY( table->findRoute(PackageType::ThirdType) == Second_RouteName );
    // range       First
    QVERIFY( table->findRoute(PackageType::RangeType) == Second_RouteName );


    // Try to accept

    IORoutingTable out1;
    bool res = tableSrv.accept( tableCli1, out1 );
    QVERIFY( res );
    table = &out1;
    // default     First
    QVERIFY( table->findRoute(PackageType::SomeType) == First_RouteName );
    // FirstType   First
    QVERIFY( table->findRoute(PackageType::FirstType) == First_RouteName );
    // SecondType  Second
    QVERIFY( table->findRoute(PackageType::SecondType) == Second_RouteName );
    // ThirdType   Second
    QVERIFY( table->findRoute(PackageType::ThirdType) == Second_RouteName );
    // range       Second
    QVERIFY( table->findRoute(PackageType::RangeType) == Second_RouteName );


    IORoutingTable out2;
    res = tableSrv.accept( tableCli2, out2 );
    QVERIFY( res );
    table = &out2;
    // default     Second
    QVERIFY( table->findRoute(PackageType::SomeType) == Second_RouteName );
    // FirstType   Third
    QVERIFY( table->findRoute(PackageType::FirstType) == Third_RouteName );
    // SecondType  Second
    QVERIFY( table->findRoute(PackageType::SecondType) == Second_RouteName );
    // ThirdType   Third
    QVERIFY( table->findRoute(PackageType::ThirdType) == Third_RouteName );
    // range       Second
    QVERIFY( table->findRoute(PackageType::RangeType) == Second_RouteName );


    IORoutingTable out3;
    res = tableSrv.accept( tableCli3, out3 );
    QVERIFY( res );
    table = &out3;
    // default     Third
    QVERIFY( table->findRoute(PackageType::SomeType) == Third_RouteName );
    // FirstType   Third
    QVERIFY( table->findRoute(PackageType::FirstType) == Third_RouteName );
    // SecondType  Second
    QVERIFY( table->findRoute(PackageType::SecondType) == Second_RouteName );
    // ThirdType   First
    QVERIFY( table->findRoute(PackageType::ThirdType) == First_RouteName );
    // range       Second
    QVERIFY( table->findRoute(PackageType::RangeType) == Second_RouteName );



    IORoutingTable out4;
    res = tableSrv.accept( tableCli4, out4 );
    QVERIFY( ! res );


    IORoutingTable out5;
    res = tableSrv.accept( tableCli5, out5 );
    QVERIFY( res );

    IORoutingTable out6;
    res = tableSrv.accept( tableCli6, out6 );
    QVERIFY( res );
    table = &out6;
    // default     Second
    QVERIFY( table->findRoute(PackageType::SomeType) == Second_RouteName );
    // FirstType   Second
    QVERIFY( table->findRoute(PackageType::FirstType) == Second_RouteName );
    // SecondType  Second
    QVERIFY( table->findRoute(PackageType::SecondType) == Second_RouteName );
    // ThirdType   Second
    QVERIFY( table->findRoute(PackageType::ThirdType) == Second_RouteName );
    // range       Second
    QVERIFY( table->findRoute(PackageType::RangeType) == Second_RouteName );

}

/*****************************************************************************/

int main ( int argc, char *argv[] )
{
    QCoreApplication a(argc, argv);
    RoutingTableTest test;
    return QTest::qExec(&test, argc, argv);
}

/*****************************************************************************/
#include "RoutingTableTest.moc"
