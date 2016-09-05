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
///	CDspBroadcastListener.cpp
///
/// @brief
///	Implementation of the class CDspBroadcastListener
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

#include <QtCore>
#include <QtNetwork>
#include <QMutex>

#include "CDspBroadcastListener.h"
#include <prlcommon/Messaging/CVmEvent.h>
#include <prlcommon/Messaging/CVmEventParameter.h>
#include <prlxmlmodel/DispConfig/CDispWorkspacePreferences.h>
#include "CDspService.h"
#include "Libraries/HostInfo/CHostInfo.h"
#include <prlxmlmodel/HostHardwareInfo/CHostHardwareInfo.h>
#include <prlxmlmodel/HostHardwareInfo/CHwOsVersion.h>
#include <prlcommon/Std/PrlAssert.h>

#include "Build/Current.ver"

// By adding this interface we enable allocations tracing in the module
#include "Interfaces/Debug.h"

#define CHECK_PROPER_BIND(bind_call)\
	if (!bind_call)\
	{\
		WRITE_TRACE(DBG_FATAL, "Failed to bind UDP interface with call '%s': %d '%s'", #bind_call, m_pUdpSocket->error(), qPrintable(m_pUdpSocket->errorString()));\
		PRL_ASSERT(0);\
	}\
	else\
		WRITE_TRACE(DBG_FATAL, "UDP interface successfully initialized!");

namespace {
/** Global answers object access synchronization object */
QMutex g_GlobalAnswersObjsMutex(QMutex::Recursive);
/** Global server info VM event object */
CVmEvent g_ServerInfoVmEvent;

}

CDspBroadcastListener::CDspBroadcastListener()
{
    InitializeGlobalAnswers();
	m_pUdpSocket = new QUdpSocket(this);

	CHECK_PROPER_BIND(m_pUdpSocket->bind(PARALLELS_SERVER_BROADCAST_PORT))

	bool bConnected = connect(m_pUdpSocket, SIGNAL(readyRead()), this, SLOT(processPendingDatagrams()));
	PRL_ASSERT(bConnected);
	Q_UNUSED(bConnected);
}

void CDspBroadcastListener::InitializeGlobalAnswers()
{
	QMutexLocker _lock(&g_GlobalAnswersObjsMutex);
	g_ServerInfoVmEvent.cleanupClassProperties();
	g_ServerInfoVmEvent.addEventParameter( new CVmEventParameter( PVE::UnsignedInt
		, QString("%1").arg( CDspService::instance()->getDispConfigGuard().getDispWorkSpacePrefs()->getDispatcherPort() )
		, EVT_PARAM_PRL_SERVER_INFO_COMMANDS_PORT ) );
	QString sOsVersion = CDspService::instance()->getHostInfo()->data()->getOsVersion()->getStringPresentation();
	if (sOsVersion.isEmpty())
		sOsVersion = "Unknown";
	g_ServerInfoVmEvent.addEventParameter( new CVmEventParameter( PVE::String
		, sOsVersion
		, EVT_PARAM_PRL_SERVER_INFO_OS_VERSION ) );
	g_ServerInfoVmEvent.addEventParameter( new CVmEventParameter( PVE::String
		, CDspService::instance()->getDispConfigGuard().getDispConfig()->getVmServerIdentification()->getServerUuid()
		, EVT_PARAM_PRL_SERVER_INFO_SERVER_UUID ) );
	g_ServerInfoVmEvent.addEventParameter( new CVmEventParameter( PVE::String
		, VER_FULL_BUILD_NUMBER_STR
		, EVT_PARAM_PRL_SERVER_INFO_PRODUCT_VERSION ) );
}

void CDspBroadcastListener::processPendingDatagrams()
{
	QHostAddress sender;
	quint16 senderPort;

	while (m_pUdpSocket->hasPendingDatagrams())
	{
		QByteArray datagram;
		datagram.resize(m_pUdpSocket->pendingDatagramSize());
		m_pUdpSocket->readDatagram(datagram.data(), datagram.size(), &sender, &senderPort);
		processDatagram(datagram, sender, senderPort);
	}
}

void CDspBroadcastListener::processDatagram(const QByteArray &datagram, const QHostAddress &host, const quint16 &port)
{
	QString sRequest = UTF8_2QSTR(datagram.data());

	if (sRequest == PRL_GET_SERVER_INFO_BROADCAST_SERVER_MSG)
	{
		QMutexLocker lock(&g_GlobalAnswersObjsMutex);
		sendReply(host, port, g_ServerInfoVmEvent.toString().toUtf8());
	}
}

void CDspBroadcastListener::sendReply(const QHostAddress &host, const quint16 &port, const QByteArray &datagram)
{
	m_pUdpSocket->writeDatagram(datagram.data(), datagram.size(), host, port);
}
