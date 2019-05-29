/*
 * Copyright (c) 2015-2017, Parallels International GmbH
 * Copyright (c) 2017-2019 Virtuozzo International GmbH. All rights reserved.
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
 * Our contact details: Virtuozzo International GmbH, Vordergasse 59, 8200
 * Schaffhausen, Switzerland.
 */

#ifndef CDspHostSettingsHelper_H
#define CDspHostSettingsHelper_H

#include <QString>
#include <QList>


struct Credentials {
	Credentials(const QString& user_, const QString& pass_)
		: user(user_), password(pass_)
	{}

	QString user;
	QString password;
};
typedef QList<Credentials> CredentialsList;


class ProxyHelper
{
	public:
		void setProxy(const QString& host, quint16 port);
		bool getNextCredentials(QString& user, QString& pass);
		void clearCredentials();

		bool getProxy( QString& host, quint16& port );

	private:
		QString m_host;
		quint16 m_port;
		int m_nCurrCredential;
		CredentialsList m_creds;
};


class CDspHostSettingsHelper
{
	public:
		static CredentialsList getHTTPProxyCredentials(const QString& host, quint16 port );
};

#endif // CDspHostSettingsHelper_H

