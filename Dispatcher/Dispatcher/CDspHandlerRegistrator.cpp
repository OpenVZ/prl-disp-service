///////////////////////////////////////////////////////////////////////////////
///
/// @file CDspRouter
///
/// Dispatcher handler registrator
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

#include "CDspHandlerRegistrator.h"
#include "Libraries/Std/PrlAssert.h"
/*****************************************************************************/

CDspHandlerRegistrator* CDspHandlerRegistrator::s_handlerRegistrator = 0;

/*****************************************************************************/

CDspHandler::CDspHandler ( IOSender::Type type, const char* name ) :
	m_senderType(type),
	m_handlerName(name)
{}

CDspHandler::~CDspHandler ()
{}

void CDspHandler::init ()
{}

IOSender::Type CDspHandler::senderType () const
{
	return m_senderType;
}

const QString& CDspHandler::handlerName () const
{
	return m_handlerName;
}

void CDspHandler::handleClientConnected ( const IOSender::Handle& )
{
}

void CDspHandler::handleClientDisconnected ( const IOSender::Handle& )
{
}

void CDspHandler::handleToDispatcherPackage ( const IOSender::Handle&,
								 const SmartPtr<IOPackage>& )
{
}

void CDspHandler::handleFromDispatcherPackage (
					 const SmartPtr<CDspHandler>&,
					 const IOSender::Handle&,
					 const SmartPtr<IOPackage>& )
{
}

void CDspHandler::handleFromDispatcherPackage (
					 const SmartPtr<CDspHandler>&,
					 const IOSender::Handle&,
					 const IOSender::Handle&,
					 const SmartPtr<IOPackage>& )
{
}

void CDspHandler::handleClientStateChanged ( const IOSender::Handle&, IOSender::State )
{
}

void CDspHandler::handleDetachClient ( const IOSender::Handle&, const IOCommunication::DetachedClient& )
{
}

/*****************************************************************************/

CDspHandlerRegistrator& CDspHandlerRegistrator::instance ()
{
	if ( ! s_handlerRegistrator )
		s_handlerRegistrator = new CDspHandlerRegistrator;
	return *s_handlerRegistrator;
}

CDspHandlerRegistrator::CDspHandlerRegistrator () :
	m_initWasDone(false)
{}

bool CDspHandlerRegistrator::registerHandler (
    const SmartPtr<CDspHandler>& handler )
{
	if ( ! handler.isValid() )
		return false;

	QWriteLocker locker( &m_rwLock );

	if ( m_handlers.contains(handler->senderType()) )
		return false;

	const QString& name = handler->handlerName();
	foreach ( SmartPtr<CDspHandler> h, m_handlers.values() )
		if ( h->handlerName() == name )
			return false;

	m_handlers[handler->senderType()] = handler;
	m_handlersPtrs[handler.getImpl()] = handler;
	m_handlersNames[handler->handlerName()] = handler;

	return true;
}

void CDspHandlerRegistrator::doHandlersInit ()
{
	QWriteLocker locker( &m_rwLock );
	PRL_ASSERT( false == m_initWasDone );
	foreach ( SmartPtr<CDspHandler> handler, m_handlers ) {
		handler->init();
	}
	m_initWasDone = true;
}

void CDspHandlerRegistrator::cleanHandlers ()
{
	QWriteLocker locker( &m_rwLock );
	m_handlersNames.clear();
	m_handlersPtrs.clear();
	m_handlers.clear();
	m_initWasDone = false;
}

SmartPtr<CDspHandler> CDspHandlerRegistrator::findHandler (
    IOService::IOSender::Type type ) const
{
	QReadLocker locker( &m_rwLock );
	if ( m_handlers.contains(type) )
		return m_handlers.value(type);

	return SmartPtr<CDspHandler>();
}

SmartPtr<CDspHandler> CDspHandlerRegistrator::findHandlerByPtr (
    const CDspHandler* handlerPtr ) const
{
	QReadLocker locker( &m_rwLock );
	if ( m_handlersPtrs.contains(handlerPtr) )
		return m_handlersPtrs.value(handlerPtr);

	return SmartPtr<CDspHandler>();
}

SmartPtr<CDspHandler> CDspHandlerRegistrator::findHandlerByName (
    const QString& handlerName ) const
{
	QReadLocker locker( &m_rwLock );
	if ( m_handlersNames.contains(handlerName) )
		return m_handlersNames.value(handlerName);

	return SmartPtr<CDspHandler>();
}

QList< SmartPtr<CDspHandler> > CDspHandlerRegistrator::getHandlers () const
{
	QReadLocker locker( &m_rwLock );
	return m_handlers.values();
}

/*****************************************************************************/
