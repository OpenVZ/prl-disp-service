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


#ifndef CLIENT_H
#define CLIENT_H

#include <QTimer>
#include "IOClient.h"
#include "IORoutingTableHelper.h"

#include "ui_Client.h"

using namespace IOService;

class Client : public QMainWindow,
               public Ui::MainWindow
{
    Q_OBJECT
public:
    Client ();

public slots:
    void receivePackage ( const SmartPtr<IOPackage> );
    void stateChanged ( IOSender::State );

    void onConnectPressed ();
    void onSendPressed ();
    void onUseProxyChecked ( int );
    void onSecurityLevelChanged ( int );
    void onHostComboChanged ( int );
    void onTimer ();

private:
    bool sendTillResult ( const SmartPtr<IOPackage>& p, qint64 nums,
                          quint64& latency );

private:
    IOClient* m_client;
    PRL_SECURITY_LEVEL m_secLevel;
    QString m_host;
    QTimer m_timer;
};

#endif //CLIENT_H
