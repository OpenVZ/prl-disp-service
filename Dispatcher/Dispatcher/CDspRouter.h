///////////////////////////////////////////////////////////////////////////////
///
/// @file CDspRouter
///
/// Dispatcher router
///
/// @author romanp
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
///////////////////////////////////////////////////////////////////////////////

#ifndef CDSPROUTER_H
#define CDSPROUTER_H

#include <QHash>
#include <QReadWriteLock>
#include "CDspHandlerRegistrator.h"

#include "Libraries/IOService/src/IOCommunication/IOProtocol.h"

using namespace IOService;

class CDspRoute
{
public:
	CDspRoute ();
	CDspRoute ( quint32, quint32, const char* );

	const QString& handlerName () const;

	bool packageCanBeRouted ( const SmartPtr<IOPackage>& ) const;

private:
	quint32 m_typeRangeBegin;
	quint32 m_typeRangeEnd;
	QString m_handlerName;
};


class CDspRouter
{
public:
	static CDspRouter& instance ();

	bool registerRoutes ( IOService::IOSender::Type, const QList<CDspRoute>& );
	void cleanRoutes ();
	/**
	 * Routes specified package to necessary recipients
	 * @param pointer to the previous package handler object
	 * @param package sender object handle
	 * @param pointer to processing package object
	 * @return sign whether package was successfully routed
	 */
	bool routePackage(
		CDspHandler *pHandler,
		IOSender::Handle h,
		const SmartPtr<IOPackage> &p);

private:
	CDspRouter ();
	~CDspRouter ();

private:
	static CDspRouter* s_routerInstance;

	mutable QReadWriteLock m_rwLock;

	typedef QHash<QString, QList<CDspRoute> > RoutesHash;
	typedef QHash< IOService::IOSender::Type, RoutesHash > RoutesBySenderTypeHash;

	RoutesBySenderTypeHash m_routes;
};

#endif //CDSPROUTER_H
