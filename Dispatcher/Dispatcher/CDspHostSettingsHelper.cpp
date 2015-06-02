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

#include "CDspService.h"
#include "CDspHostSettingsHelper.h"

#include <Libraries/Logging/Logging.h>

CredentialsList CDspHostSettingsHelper::getHTTPProxyCredentials(const QString& host, quint16 port)
{
	Q_UNUSED(port);

	CredentialsList creds;
	if( !CDspService::instance()->getUserHelper().searchCachedProxyPassword( host, creds) )
		creds.clear();

	return creds;
}

void ProxyHelper::setProxy(const QString& host, quint16 port)
{
	m_host = host;
	m_port = port;

	clearCredentials();
	m_creds = CDspHostSettingsHelper::getHTTPProxyCredentials( host, port );
}

bool ProxyHelper::getProxy(QString& host, quint16& port)
{
	host = m_host;
	port = m_port;
	return !host.isEmpty();
}

bool ProxyHelper::getNextCredentials(QString& user, QString& pass)
{
	if (m_nCurrCredential >= m_creds.count())
		return false;

	user = m_creds.at(m_nCurrCredential).user;
	pass = m_creds.at(m_nCurrCredential).password;
	++m_nCurrCredential;

	return true;
}

void ProxyHelper::clearCredentials()
{
	m_creds.clear();
	m_nCurrCredential = 0;
}

