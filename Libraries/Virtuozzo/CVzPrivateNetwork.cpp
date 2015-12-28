/*
 * Copyright (c) 2011-2015 Parallels IP Holdings GmbH
 *
 * This file is part of Virtuozzo Core Libraries. Virtuozzo Core
 * Libraries is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <http://www.gnu.org/licenses/> or write to Free Software Foundation,
 * 51 Franklin Street, Fifth Floor Boston, MA 02110, USA.
 *
 * Our contact details: Parallels IP Holdings GmbH, Vordergasse 59, 8200
 * Schaffhausen, Switzerland.
 */
#include <QHostAddress>

#include "CVzPrivateNetwork.h"
#include <prlcommon/PrlCommonUtilsBase/NetworkUtils.h>
#include <prlcommon/Logging/Logging.h>

#define PRIV_NETS_FILE "/proc/vz/privnet/sparse"
#define PRIV_NETS_FILE6 "/proc/vz/privnet/sparse6"
#define PRIV_NETS_LEGACY_FILE "/proc/vz/privnet/legacy"
#define PRIV_NETS_LEGACY_FILE6 "/proc/vz/privnet/legacy6"

static void read_ids_from_file(const char *path, QSet<unsigned int> &ids)
{
	QFile file(path);
	if (file.open(QIODevice::ReadOnly))
	{
		QString line = file.readLine();
		while (!line.isEmpty()) {
			QStringList tmp = line.split(": ", QString::SkipEmptyParts);
			unsigned int id = tmp.at(0).toUInt();
			if (!ids.contains(id))
				ids.insert(id);
			line = file.readLine();
		}
	}
	file.close();
}

static int generate_new_id(const CPrivateNetworks *pNets)
{
	QSet<unsigned int> known_ids;

	/* Retrieve already set up IDs from the system */
	read_ids_from_file(PRIV_NETS_FILE, known_ids);
	read_ids_from_file(PRIV_NETS_FILE6, known_ids);

	/* add IDs from configured private networks */
	if (pNets)
	{
		foreach(CPrivateNetwork *pNetwork, pNets->m_lstPrivateNetwork)
		{
			unsigned int id = pNetwork->getNetworkID();
			if (id != PRL_PRIVNET_GENERATE_ID && !known_ids.contains(id))
				known_ids.insert(id);
		}
	}

	/* search for first available ID */
	for (unsigned int i = 1; i < (unsigned int)known_ids.size() + 2; i++)
		if (!known_ids.contains(i))
			return i;
	return PRL_PRIVNET_GENERATE_ID;
}

static PRL_RESULT add_sparse_network(CPrivateNetwork *pNewNet, CVmEvent *pEvent)
{
	QFile PrivFile(PRIV_NETS_FILE);
	if (!PrivFile.open(QIODevice::WriteOnly | QIODevice::Unbuffered))
		return PRL_ERR_FILE_NOT_FOUND;

	bool bIPv6FileExists = false;
	QFile PrivFile6(PRIV_NETS_FILE6);
	if (PrivFile6.open(QIODevice::WriteOnly | QIODevice::Unbuffered))
		bIPv6FileExists = true;

	/* add new private network */
	QString addCmd = QString("+%1\n").arg(pNewNet->getNetworkID());
	if (PrivFile.write(QSTR2UTF8(addCmd)) < 0 ||
		(bIPv6FileExists && PrivFile6.write(QSTR2UTF8(addCmd)) < 0)) {
		if (pEvent)
			pEvent->addEventParameter(new CVmEventParameter(PVE::String,
						QString(pNewNet->getNetworkID()),
						EVT_PARAM_MESSAGE_PARAM_0));
		return PRL_ERR_INVALID_ARG;
	}

	/* add IP subnets for new network */
	QList<QString> IPs = pNewNet->getNetAddresses();
	foreach (QString ip_mask, IPs)
	{
		bool bIPv4 = false;
		bool bIPv6 = false;

		// special value: grant access to all IPs not covered by
		// any private network.
		if (ip_mask == QString("*"))
		{
			bIPv4 = true;
			bIPv6 = true;
		}
		else
		{
			QString ip, mask;
			if (!NetworkUtils::ParseIpMask(ip_mask, ip, mask))
			{
				WRITE_TRACE(DBG_FATAL, "Can't determine IP"
						" address '%s' family",
						QSTR2UTF8(ip_mask));
				if (pEvent)
					pEvent->addEventParameter(
						new CVmEventParameter(PVE::String, ip_mask,
						EVT_PARAM_MESSAGE_PARAM_0));
				return PRL_NET_IPPRIVATE_NETWORK_INVALID_IP;
			}
			QHostAddress ip_addr(ip);
			if (ip_addr.protocol() == QAbstractSocket::IPv4Protocol)
				bIPv4 = true;
			else if (ip_addr.protocol() == QAbstractSocket::IPv6Protocol)
				bIPv6 = true;
		}
		addCmd = QString("+%1:%2\n").arg(pNewNet->getNetworkID()).arg(ip_mask);
		if ((bIPv4 && PrivFile.write(QSTR2UTF8(addCmd)) < 0) ||
			(bIPv6FileExists && bIPv6 && PrivFile6.write(QSTR2UTF8(addCmd)) < 0)) {
			if (pEvent) {
				pEvent->addEventParameter(
					new CVmEventParameter(PVE::String, ip_mask,
					EVT_PARAM_MESSAGE_PARAM_0));
				pEvent->addEventParameter(
					new CVmEventParameter(PVE::String,
					pNewNet->getName(),
					EVT_PARAM_MESSAGE_PARAM_1));
			}
			return PRL_NET_IPPRIVATE_NETWORK_INVALID_IP;
		}
	}
	return PRL_ERR_SUCCESS;
}

static PRL_RESULT add_legacy_network(CPrivateNetwork *pNewNet, CVmEvent *pEvent)
{
	QFile PrivFile(PRIV_NETS_LEGACY_FILE);
	if (!PrivFile.open(QIODevice::WriteOnly | QIODevice::Unbuffered))
		return PRL_ERR_FILE_NOT_FOUND;

	bool bIPv6FileExists = false;
	QFile PrivFile6(PRIV_NETS_LEGACY_FILE6);
	if (PrivFile6.open(QIODevice::WriteOnly | QIODevice::Unbuffered))
		bIPv6FileExists = true;

	/* add IP subnets for new network */
	QList<QString> IPs = pNewNet->getNetAddresses();
	foreach (QString ip_mask, IPs)
	{
		QString ip, mask;
		if (!NetworkUtils::ParseIpMask(ip_mask, ip, mask))
		{
			WRITE_TRACE(DBG_FATAL, "Can't determine IP"
					" address '%s' family",
					QSTR2UTF8(ip_mask));
			if (pEvent)
				pEvent->addEventParameter(
						new CVmEventParameter(PVE::String, ip_mask,
							EVT_PARAM_MESSAGE_PARAM_0));
			return PRL_NET_IPPRIVATE_NETWORK_INVALID_IP;
		}
		QHostAddress ip_addr(ip);
		qint64 ret = -1;
		if (ip_addr.protocol() == QAbstractSocket::IPv4Protocol)
		{
			QString addCmd = QString("+%1/32\n").arg(ip_mask);
			ret = PrivFile.write(QSTR2UTF8(addCmd));
		}
		else if (ip_addr.protocol() == QAbstractSocket::IPv6Protocol)
		{
			if (!bIPv6FileExists)
				return PRL_ERR_FILE_NOT_FOUND;
			QString addCmd = QString("+%1/128\n").arg(ip_mask);
			ret = PrivFile6.write(QSTR2UTF8(addCmd));
		}

		if (ret < 0)
		{
			if (pEvent) {
				pEvent->addEventParameter(
					new CVmEventParameter(PVE::String, ip_mask,
					EVT_PARAM_MESSAGE_PARAM_0));
				pEvent->addEventParameter(
					new CVmEventParameter(PVE::String,
					pNewNet->getName(),
					EVT_PARAM_MESSAGE_PARAM_1));
			}
			return PRL_NET_IPPRIVATE_NETWORK_INVALID_IP;
		}
	}
	return PRL_ERR_SUCCESS;
}

PRL_RESULT CVzPrivateNetwork::AddPrivateNetwork(CPrivateNetwork *pNewNet,
	const CPrivateNetworks *pNets, CVmEvent *pEvent)
{
#ifdef _LIN_
	if (pNewNet->getNetworkID() == PRL_PRIVNET_GENERATE_ID &&
			!pNewNet->isGlobal())
	{
		/* search for first available ID if required */
		unsigned int id = generate_new_id(pNets);
		if (id == PRL_PRIVNET_GENERATE_ID)
		{
			WRITE_TRACE(DBG_FATAL, "Can't find suitable ID for"
				" new IP private network '%s'",
				QSTR2UTF8(pNewNet->getName()));
			return PRL_ERR_UNEXPECTED;
		}
		pNewNet->setNetworkID(id);
	}

	if (!pNewNet->isGlobal())
		return add_sparse_network(pNewNet, pEvent);
	else
		return add_legacy_network(pNewNet, pEvent);
#else
	Q_UNUSED(pNewNet);
	Q_UNUSED(pNets);
	Q_UNUSED(pEvent);
	return PRL_ERR_UNIMPLEMENTED;
#endif
}

PRL_RESULT CVzPrivateNetwork::UpdatePrivateNetwork(
	CPrivateNetwork *pNet, const CPrivateNetwork *pOldNet,
	CVmEvent *pEvent)
{
#ifdef _LIN_
	if (pOldNet)
		RemovePrivateNetwork(pOldNet);
	return AddPrivateNetwork(pNet, NULL, pEvent);
#else
	Q_UNUSED(pNet);
	Q_UNUSED(pOldNet);
	Q_UNUSED(pEvent);
	return PRL_ERR_UNIMPLEMENTED;
#endif
}

static PRL_RESULT remove_legacy_network(const CPrivateNetwork *pNet)
{
	QFile PrivFile(PRIV_NETS_LEGACY_FILE);
	if (!PrivFile.open(QIODevice::WriteOnly | QIODevice::Unbuffered))
		return PRL_ERR_FILE_NOT_FOUND;

	bool bIPv6FileExists = false;
	QFile PrivFile6(PRIV_NETS_LEGACY_FILE6);
	if (PrivFile6.open(QIODevice::WriteOnly | QIODevice::Unbuffered))
		bIPv6FileExists = true;

	/* remove IP subnets from the network */
	QList<QString> IPs = pNet->getNetAddresses();
	foreach (QString ip_mask, IPs)
	{
		QString ip, mask;
		if (!NetworkUtils::ParseIpMask(ip_mask, ip, mask))
		{
			WRITE_TRACE(DBG_FATAL, "Can't determine IP"
					" address '%s' family",
					QSTR2UTF8(ip_mask));
			return PRL_NET_IPPRIVATE_NETWORK_INVALID_IP;
		}
		QHostAddress ip_addr(ip);
		qint64 ret = -1;
		if (ip_addr.protocol() == QAbstractSocket::IPv4Protocol)
		{
			QString addCmd = QString("-%1/32\n").arg(ip_mask);
			ret = PrivFile.write(QSTR2UTF8(addCmd));
		}
		else if (ip_addr.protocol() == QAbstractSocket::IPv6Protocol)
		{
			if (!bIPv6FileExists)
				return PRL_ERR_FILE_NOT_FOUND;
			QString addCmd = QString("-%1/128\n").arg(ip_mask);
			ret = PrivFile6.write(QSTR2UTF8(addCmd));
		}
		if (ret < 0)
			return PRL_NET_IPPRIVATE_NETWORK_INVALID_IP;
	}
	return PRL_ERR_SUCCESS;
}

static PRL_RESULT remove_sparse_network(const CPrivateNetwork *pNet)
{
	if (pNet->getNetworkID() == PRL_PRIVNET_GENERATE_ID)
	{
		WRITE_TRACE(DBG_FATAL, "The IP private network '%s' is invalid",
				QSTR2UTF8(pNet->getName()));
		return PRL_NET_IPPRIVATE_NETWORK_DOES_NOT_EXIST;
	}

	QFile PrivFile(PRIV_NETS_FILE);
	if (!PrivFile.open(QIODevice::WriteOnly | QIODevice::Unbuffered))
		return PRL_ERR_FILE_NOT_FOUND;

	bool bIPv6FileExists = false;
	QFile PrivFile6(PRIV_NETS_FILE6);
	if (PrivFile6.open(QIODevice::WriteOnly | QIODevice::Unbuffered))
		bIPv6FileExists = true;

	QString removeCmd = QString("-%1").arg(pNet->getNetworkID());
	if (PrivFile.write(QSTR2UTF8(removeCmd)) < 0)
		return PRL_ERR_INVALID_ARG;
	if (bIPv6FileExists && PrivFile6.write(QSTR2UTF8(removeCmd)) < 0)
		return PRL_ERR_INVALID_ARG;
	return PRL_ERR_SUCCESS;
}

PRL_RESULT CVzPrivateNetwork::RemovePrivateNetwork(const CPrivateNetwork *pNet, CVmEvent *pEvent)
{
#ifdef _LIN_
	Q_UNUSED(pEvent);
	if (!pNet->isGlobal())
		return remove_sparse_network(pNet);
	else
		return remove_legacy_network(pNet);
#else
	Q_UNUSED(pNet);
	Q_UNUSED(pEvent);
	return PRL_ERR_UNIMPLEMENTED;
#endif
}
