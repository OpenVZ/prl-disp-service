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


#include <QApplication>

#include <QBuffer>
#include <QByteArray>
#include <QDataStream>
#include "Libraries/Logging/Logging.h"

#include "Client.h"

#ifdef _WIN_
 #include <windows.h>
 #define sleepMsecs Sleep
#else
 #include <unistd.h>
 #define sleepMsecs(usecs) usleep(usecs * 1000)
#endif

using namespace IOService;

/*****************************************************************************/

Client::Client () :
    m_client( 0 ),
    m_secLevel( PSL_LOW_SECURITY ),
    m_host( IOService::LoopbackAddr )
{
    setupUi(this);

    // Default init
    statusBar()->showMessage(tr("Disconnected"));
    sendButton->setDisabled( true );
    hostEdit->setDisabled( true );
    peerUuidLabel->setDisabled( true );
    proxyPeerUuidEdit->setDisabled( true );


    QObject::connect( connectButton, SIGNAL(clicked(bool)),
                      SLOT(onConnectPressed()) );

    QObject::connect( sendButton, SIGNAL(clicked(bool)),
                      SLOT(onSendPressed()) );

    QObject::connect( useProxyCheckBox, SIGNAL(stateChanged(int)),
                      SLOT(onUseProxyChecked(int)) );

    QObject::connect( securityLevelComboBox, SIGNAL(currentIndexChanged(int)),
                      SLOT(onSecurityLevelChanged(int)) );

    QObject::connect( hostComboBox, SIGNAL(currentIndexChanged(int)),
                      SLOT(onHostComboChanged(int)) );

    QObject::connect( &m_timer, SIGNAL(timeout()),
                      SLOT(onTimer()) );

    m_timer.start( 500 );
}

void Client::onTimer ()
{
    if ( m_client == 0 )
        return;

    IOSender::Statistics stat;
    bool res = m_client->connectionStatistics( stat );
    Q_ASSERT(res);
    Q_UNUSED(res);

    sentPackagesLabel->setText( QString("%1 pkg").arg(stat.sentPackages) );
    receivedPackagesLabel->setText( QString("%1 pkg").arg(stat.receivedPackages) );
    sentBytesLabel->setText( QString("%1 b").arg(stat.sentBytes) );
    readBytesLabel->setText( QString("%1 b").arg(stat.receivedBytes) );
}

void Client::onConnectPressed ()
{
    if ( m_client == 0  ) {
        if ( useProxyCheckBox->isChecked() )
            m_client = new IOClient(
                       IORoutingTableHelper::GetClientRoutingTable(m_secLevel),
                       IOSender::Client,
                       proxyPeerUuidEdit->text(),
                       (hostComboBox->currentIndex() == 0 ?
                          m_host : hostEdit->text()),
                       portSpinBox->value() );
        else
            m_client = new IOClient(
                       IORoutingTableHelper::GetClientRoutingTable(m_secLevel),
                       IOSender::Client,
                       (hostComboBox->currentIndex() == 0 ?
                          m_host : hostEdit->text()),
                       portSpinBox->value() );

        QObject::connect( m_client,
                   SIGNAL(onPackageReceived(const SmartPtr<IOPackage>)),
                   SLOT(receivePackage(const SmartPtr<IOPackage>)) );

        QObject::connect( m_client,
                      SIGNAL(onStateChanged(IOSender::State)),
                      SLOT(stateChanged(IOSender::State)) );

        statusBar()->showMessage(tr("Connecting ..."));

        const quint32 timeout = 10000;
        m_client->connectClient( timeout );
    }
    else {
        Q_ASSERT(m_client);
        statusBar()->showMessage(tr("Disconnecting ..."));

        m_client->disconnectClient();
    }
}


bool Client::sendTillResult ( const SmartPtr<IOPackage>& p,
                              qint64 nums,
                              quint64& sendLatency )
{
    quint64 localLatency = 0;

    for ( int i = 0; i < nums; ++i ) {
        QApplication::processEvents();

        quint64 msecs = IOService::msecsFromEpoch();
        IOSendJob::Handle job = m_client->sendPackage( p );
        IOSendJob::Result res = m_client->waitForSend( job );
        quint64 latency = IOService::msecsFromEpoch() - msecs;
        localLatency += latency;

        if ( res != IOSendJob::Success ) {
            WRITE_TRACE(DBG_FATAL, "Send to server failed!");
            return false;
        }
        res = m_client->getSendResult( job );
        Q_ASSERT( res != IOSendJob::SendQueueIsFull );

        if ( res == IOSendJob::Fail ) {
            LOG_MESSAGE(DBG_FATAL, "Send failed!");
            return false;
        }
    }

    LOG_MESSAGE(DBG_WARNING, "Send pkgs num=%lld of size=%d to server took: "
                "%lld msecs", nums, p->data[0].bufferSize, localLatency);

    sendLatency += localLatency;

    return true;
}

void Client::onSendPressed ()
{
    // Format:
    // some_data -- sends package with data
    // 1. some_data(NUM) -- sends NUM packages with data
    // 2. some_data(NUMb|m|g) -- sends package with NUM (mega, or giga) bytes
    // 3. some_data(NUMb|m|gXNUM) -- sends NUM packages of NUM (mega, or giga)
    //                                bytes
    // 4. some_data(NUMb|m|g%NUMb|m|g) -- sends packages of NUM (mega, or giga)
    //                                    bytes / NUM (mega, or giga) bytes

    QString str = textEdit->toPlainText();
    textEdit->clear();

    // (.*)\((\d+)\)$
    QRegExp format1( "(.*)\\((\\d+)\\)$",
                     Qt::CaseInsensitive );

    // (.*)\((\d+)(b|m|g{1})\)$
    QRegExp format2( "(.*)\\((\\d+)(b|m|g{1})\\)$",
                     Qt::CaseInsensitive );

    // (.*)\((\d+)(b|m|g{1})x(\d+)\)$
    QRegExp format3( "(.*)\\((\\d+)(b|m|g{1})x(\\d+)\\)$",
                     Qt::CaseInsensitive );

    // (.*)\((\d+)(b|m|g{1})%(\d+)(b|m|g{1})\)$
    QRegExp format4( "(.*)\\((\\d+)(b|m|g{1})%(\\d+)(b|m|g{1})\\)$",
                     Qt::CaseInsensitive );

    quint64 sendLatency = 0;

    if ( format1.indexIn(str) != -1 ) {
        QString data = format1.cap(1);
        qint64 num = format1.cap(2).toInt();

        SmartPtr<IOPackage> p =
            IOPackage::createInstance( 0, 1 );
        p->fillBuffer( 0, IOPackage::RawEncoding,
                       data.toAscii().data(), data.size() );

        sendTillResult( p, num, sendLatency );
    }
    else if ( format2.indexIn(str) != -1 ) {
        QString tmpData = format2.cap(1);
        qint64 num = format2.cap(2).toInt();
        QString type = format2.cap(3);

        // Megs
        if ( type.contains("m", Qt::CaseInsensitive) ) {
            num = 1024 * 1024 * num;
        }
        // Gigs
        else if ( type.contains("g", Qt::CaseInsensitive) ) {
            num = 1024 * 1024 * 1024 * num;
        }

        SmartPtr<IOPackage> p =
            IOPackage::createInstance( 0, 1 );

        QByteArray data( num, '0' );

        int filled = 0;
        int tmpDataSz = tmpData.size();
        QByteArray tmpBa = tmpData.toAscii();
        if ( num > 1024 ) {
            LOG_MESSAGE(DBG_FATAL, "Package size is big, buffer is set with "
                        " zeroes!");
        }
        else {
            while ( filled < num ) {
                if ( (filled + tmpDataSz) > num )
                    tmpDataSz = (filled + tmpDataSz) - num;
                data.replace( filled, tmpDataSz, tmpBa );
                filled += tmpDataSz;
            }
        }

        p->setBuffer( 0, IOPackage::RawEncoding,
                       SmartPtr<char>(data.data(),
                                       SmartPtrPolicy::DoNotReleasePointee),
                       num );

        sendTillResult( p, 1, sendLatency );
    }
    else if ( format3.indexIn(str) != -1 ) {
        QString tmpData = format3.cap(1);
        qint64 num1 = format3.cap(2).toInt();
        QString type = format3.cap(3);
        qint64 num2 = format3.cap(4).toInt();

        // Megs
        if ( type.contains("m", Qt::CaseInsensitive) ) {
            num1 = 1024 * 1024 * num1;
        }
        // Gigs
        else if ( type.contains("g", Qt::CaseInsensitive) ) {
            num1 = 1024 * 1024 * 1024 * num1;
        }

        SmartPtr<IOPackage> p =
            IOPackage::createInstance( 0, 1 );

        QByteArray data( num1, '0' );

        int filled = 0;
        int tmpDataSz = tmpData.size();
        QByteArray tmpBa = tmpData.toAscii();
        if ( num1 > 1024 ) {
            LOG_MESSAGE(DBG_FATAL, "Package size is big, buffer is set with "
                        " zeroes!");
        }
        else {
            while ( filled < num1 ) {
                if ( (filled + tmpDataSz) > num1 )
                    tmpDataSz = (filled + tmpDataSz) - num1;
                data.replace( filled, tmpDataSz, tmpBa );
                filled += tmpDataSz;
            }
        }

        p->setBuffer( 0, IOPackage::RawEncoding,
                       SmartPtr<char>(data.data(),
                                       SmartPtrPolicy::DoNotReleasePointee),
                       num1 );

        sendTillResult( p, num2, sendLatency );
    }
    else if ( format4.indexIn(str) != -1 ) {
        QString tmpData = format4.cap(1);
        qint64 num1 = format4.cap(2).toInt();
        QString type1 = format4.cap(3);
        qint64 num2 = format4.cap(4).toInt();
        QString type2 = format4.cap(5);

        // Megs
        if ( type1.contains("m", Qt::CaseInsensitive) ) {
            num1 = 1024 * 1024 * num1;
        }
        // Gigs
        else if ( type1.contains("g", Qt::CaseInsensitive) ) {
            num1 = 1024 * 1024 * 1024 * num1;
        }

        // Megs
        if ( type2.contains("m", Qt::CaseInsensitive) ) {
            num2 = 1024 * 1024 * num2;
        }
        // Gigs
        else if ( type2.contains("g", Qt::CaseInsensitive) ) {
            num2 = 1024 * 1024 * 1024 * num2;
        }

        SmartPtr<IOPackage> p =
            IOPackage::createInstance( 0, 1 );

        QByteArray data( num2, '0' );


        int filled = 0;
        int tmpDataSz = tmpData.size();
        QByteArray tmpBa = tmpData.toAscii();
        if ( num2 > 1024 ) {
            LOG_MESSAGE(DBG_FATAL, "Package size is big, buffer is set with "
                        " zeroes!");
        }
        else {
            while ( filled < num2 ) {
                if ( (filled + tmpDataSz) > num2 )
                    tmpDataSz = (filled + tmpDataSz) - num2;
                data.replace( filled, tmpDataSz, tmpBa );
                filled += tmpDataSz;
            }
        }

        p->setBuffer( 0, IOPackage::RawEncoding,
                       SmartPtr<char>(data.data(),
                                       SmartPtrPolicy::DoNotReleasePointee),
                       num2 );

        sendTillResult( p, num1 / num2, sendLatency );
    }
    else {
        SmartPtr<IOPackage> p =
            IOPackage::createInstance( 0, 1 );
        p->fillBuffer( 0, IOPackage::RawEncoding,
                       str.toAscii().data(), str.size() );

        sendTillResult( p, 1, sendLatency );
    }
}

void Client::onUseProxyChecked ( int )
{
    bool val = false;
    if ( useProxyCheckBox->isChecked() ) {
        val = false;
    }
    else {
        val = true;
    }

    peerUuidLabel->setDisabled( val );
    proxyPeerUuidEdit->setDisabled( val );

}

void Client::onSecurityLevelChanged ( int index )
{
    if ( index == 0 )
        m_secLevel = PSL_LOW_SECURITY;
    else if ( index == 1 )
        m_secLevel = PSL_NORMAL_SECURITY;
    else
        m_secLevel = PSL_HIGH_SECURITY;
}

void Client::onHostComboChanged ( int index )
{
    if ( index == 0 ) {
        m_host = IOService::LoopbackAddr;
        hostEdit->setDisabled( true );
    }
    else {
        hostEdit->setDisabled( false );
        m_host = hostEdit->text();
    }
}

void Client::receivePackage ( const SmartPtr<IOPackage> p )
{
    static quint64 pkgNum = 0;
    LOG_MESSAGE( DBG_INFO,"Package from server #%lld", ++pkgNum);

    if ( p->data[0].bufferSize <= 500 ) {
        QString str( QByteArray(p->buffers[0].getImpl(),
                                p->data[0].bufferSize) );
        textEditReceived->append( str );
    } else {
        QString str("PACKAGE #%1 IS TOO BIG (%2 size)");
        textEditReceived->append( str.arg(pkgNum).arg(p->data[0].bufferSize) );
    }
}

void Client::stateChanged ( IOSender::State st )
{
    if ( st == IOSender::Connected ) {
        connectButton->setText( "Disconnect" );
        statusBar()->showMessage(tr("Connected"));
        sendButton->setDisabled( false );
    }
    else if ( st == IOSender::Disconnected ) {
        connectButton->setText( "Connect" );
        statusBar()->showMessage(tr("Disconnected"));
        sendButton->setDisabled( true );
        delete m_client;
        m_client = 0;
    }
}

/*****************************************************************************/

int main ( int argc, char* argv[] )
{
    QApplication app( argc, argv );

    Client* cl = new Client;
    cl->show();

    return app.exec();
}

/*****************************************************************************/
