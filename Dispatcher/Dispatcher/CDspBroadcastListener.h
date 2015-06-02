////////////////////////////////////////////////////////////////////////////////
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
/// @file
///	CDspBroadcastListener.h
///
/// @brief
///	Definition of the class CDspBroadcastListener
///
/// @brief
///	This class implements Dispatcher's broadcast listener
///
/// @author sergeyt
///	ilya@, sandro@
///
/// @date
///	2007-08-28
///
/// @history
///
////////////////////////////////////////////////////////////////////////////////

#ifndef DSPBROADCASTLISTENER_H
#define DSPBROADCASTLISTENER_H

#include <QHostAddress>

class QUdpSocket;

/** Implementation of broadcast messages processor */
class CDspBroadcastListener : public QObject
{
Q_OBJECT

public:
	/** Class constructor */
	CDspBroadcastListener();
	/**
	 * Global processor initialization method
	 */
	static void InitializeGlobalAnswers();

private slots:
	/** Slot that processing socket data */
	void processPendingDatagrams();

private:
//	void broadcastDatagram();
	/** Sends reply to specified requester */
	void sendReply(const QHostAddress &host, const quint16 &port, const QByteArray &datagram);
	/**
	 * Processing received datagram
	 * @param processing datagram
	 * @param requester hostname
	 * @param requester port
	 */
	void processDatagram(const QByteArray &datagram, const QHostAddress &host, const quint16 &port);

private:
	/** Pointer to processing UDP socket object */
	QUdpSocket *m_pUdpSocket;
};

#endif // DSPBROADCASTLISTENER_H
