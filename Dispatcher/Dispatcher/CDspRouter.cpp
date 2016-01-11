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

#include "CDspRouter.h"
#include <prlcommon/Interfaces/ParallelsNamespace.h>

#include <prlcommon/Logging/Logging.h>

/*****************************************************************************/

CDspRouter* CDspRouter::s_routerInstance = 0;

/*****************************************************************************/

CDspRoute::CDspRoute () :
	m_typeRangeBegin(0),
	m_typeRangeEnd(0)
{}

CDspRoute::CDspRoute ( quint32 typeBegin, quint32 typeEnd,
					   const char* handlerName ) :
	m_typeRangeBegin(typeBegin),
	m_typeRangeEnd(typeEnd),
	m_handlerName(handlerName)
{}

const QString& CDspRoute::handlerName () const
{
	return m_handlerName;
}

bool CDspRoute::packageCanBeRouted ( const SmartPtr<IOPackage>& p ) const
{
	if ( ! p.isValid() )
		return false;

	return (p->header.type >= m_typeRangeBegin &&
			p->header.type <= m_typeRangeEnd);
}

/*****************************************************************************/

CDspRouter& CDspRouter::instance ()
{
	if ( ! s_routerInstance )
		s_routerInstance = new CDspRouter;
	return *s_routerInstance;
}

CDspRouter::CDspRouter ()
{}

bool CDspRouter::registerRoutes ( IOService::IOSender::Type type,
								  const QList<CDspRoute>& routesList )
{
	QWriteLocker writeLocker( &m_rwLock );

	// Should be unique
	if ( m_routes.contains(type) ) {
		return false;
	}

	// Routes should be unique
	QHash<QString, QList<CDspRoute> > routes;
	QList<CDspRoute>::ConstIterator it = routesList.begin();
	while ( it != routesList.end() ) {
		const CDspRoute& route = *it;
		routes[route.handlerName()].append(route);
		++it;
	}

	m_routes[type] = routes;

	return true;
}

void CDspRouter::cleanRoutes ()
{
	QWriteLocker writeLocker( &m_rwLock );
	m_routes.clear();
}

bool CDspRouter::routePackage(
	CDspHandler* pHandler,
	IOSender::Handle h,
	const SmartPtr<IOPackage>& p )
{
	SmartPtr<CDspHandler> smartHandler =
		CDspHandlerRegistrator::instance().findHandlerByPtr( pHandler );

	if ( ! smartHandler.isValid() ) {
		WRITE_TRACE(DBG_FATAL, "Can't find handler by ptr=0x%p",
					pHandler);
		return false;
	}

	QReadLocker readLocker( &m_rwLock );

	IOSender::Type senderType = smartHandler->senderType();
	if ( ! m_routes.contains(senderType) ) {
		WRITE_TRACE(DBG_WARNING, "Can't find route for handler (name=%s, "
					"senderType=%d)", qPrintable(smartHandler->handlerName()),
					senderType);
		return false;
	}

	// Check routes

	// #455781 under read access (QReadLocker) we should call only const methods
	// to prevent app crash after simultaneously call QT_CONT::detach_helper().
	RoutesBySenderTypeHash::ConstIterator routes_it = m_routes.constFind(senderType);

	QHash<QString, QList<CDspRoute> >::ConstIterator it = routes_it.value().begin();
	while ( it != routes_it.value().end() ) {
		const QList<CDspRoute>& routes = it.value();
		foreach(CDspRoute route, routes)
		{
			if ( route.packageCanBeRouted(p) ) {
				SmartPtr<CDspHandler> nextHandler =
					CDspHandlerRegistrator::instance().findHandlerByName( route.handlerName() );
				if ( ! nextHandler.isValid() ) {
					WRITE_TRACE(DBG_FATAL, "Can't find handler by name=%s",
								qPrintable(route.handlerName()));
					return false;
				}

				Uuid receiverUuid = Uuid::toUuid( p->header.receiverUuid );

				if ( receiverUuid.isNull() )
					nextHandler->handleFromDispatcherPackage( smartHandler, h, p );
				else
					nextHandler->handleFromDispatcherPackage(
													 smartHandler, h,
													 receiverUuid.toString(), p );

				return true;
			}
		}

		++it;
	}

	// Route is not found
	WRITE_TRACE(DBG_WARNING, "Can't find route for handler (name=%s, "
				"senderType=%d)", qPrintable(smartHandler->handlerName()),
				senderType);
	return false;
}

/*****************************************************************************/
