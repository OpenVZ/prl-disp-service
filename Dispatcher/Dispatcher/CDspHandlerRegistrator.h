///////////////////////////////////////////////////////////////////////////////
///
/// @file CDspHandlerRegistrator
///
/// Handler registrator
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

#ifndef CDSPHANDLERREGISTRATOR_H
#define CDSPHANDLERREGISTRATOR_H

#include <QReadWriteLock>

#include "Libraries/IOService/src/IOCommunication/IOServer.h"

using namespace IOService;

class CDspHandler
{
public:
	/**
	 * Constructor
	 * @param handlerName name of this handler
	 */
	CDspHandler ( IOSender::Type, const char* handlerName );

	/** Destructor */
	virtual ~CDspHandler ();

	/** Returns sender type */
	IOSender::Type senderType () const;

	/** Returns handler name */
	const QString& handlerName () const;

	/**
	 * Should be overloaded in child classes to do initialization
	 * after service starting.
	 */
	virtual void init ();

	/**
	 * Handle new client connection.
	 * @param h client handle
	 */
	virtual void handleClientConnected ( const IOSender::Handle& h );

	/**
	 * Handle client disconnection.
	 * @param h client handle
	 */
	virtual void handleClientDisconnected ( const IOSender::Handle& h );

	/**
	 * Handle package from the remote client.
	 * @param h client handle
	 * @param p package from client
	 */
	virtual void handleToDispatcherPackage ( const IOSender::Handle&,
											 const SmartPtr<IOPackage>& p );

	/**
	 * Handle package from other handler which should be sent by this handler.
	 * @param receiverHandler handler, which receives this package
	 * @param clientHandler transport client handler
	 * @param p package from handler to send
	 */
	virtual void handleFromDispatcherPackage (
								 const SmartPtr<CDspHandler>& receiverHandler,
								 const IOSender::Handle& clientHandler,
								 const SmartPtr<IOPackage>& p );

	/**
	 * Handle package from other handler which should be sent by this handler
	 * to direct receiver.
	 * @param receiverHandler handler, which receives this package
	 * @param clientHandler transport client handler
	 * @param directReceiverHandler transport client handler, to which
	 *                              this package should be directly send.
	 * @param p package from handler to send
	 */
	virtual void handleFromDispatcherPackage (
								 const SmartPtr<CDspHandler>& receiverHandler,
								 const IOSender::Handle& clientHandler,
								 const IOSender::Handle& directReceiverHandler,
								 const SmartPtr<IOPackage>& p );

	/**
	 * Handle client state change.
	 * @param h client handle
	 * @param st new server state
	 */
	virtual void handleClientStateChanged ( const IOSender::Handle& h,
											IOSender::State st );

	/**
	 * Handle client detach.
	 * @param h client handle
	 * @param dc detached client state.
	 */
	virtual void handleDetachClient ( const IOSender::Handle& h,
								  const IOCommunication::DetachedClient& );

private:
	IOSender::Type m_senderType;
	QString m_handlerName;
};

class CDspHandlerRegistrator
{
public:
	/** Returns single instance of registrator */
	static CDspHandlerRegistrator& instance ();

	bool registerHandler ( const SmartPtr<CDspHandler>& );
	void doHandlersInit ();
	void cleanHandlers ();

	SmartPtr<CDspHandler> findHandler ( IOSender::Type ) const;
	SmartPtr<CDspHandler> findHandlerByPtr ( const CDspHandler* ) const;
	SmartPtr<CDspHandler> findHandlerByName ( const QString& ) const;

	QList< SmartPtr<CDspHandler> > getHandlers () const;

private:
	CDspHandlerRegistrator ();
	~CDspHandlerRegistrator ();

	static CDspHandlerRegistrator* s_handlerRegistrator;

	QHash< IOSender::Type, SmartPtr<CDspHandler> > m_handlers;
	QHash< const CDspHandler*, SmartPtr<CDspHandler> > m_handlersPtrs;
	QHash< QString, SmartPtr<CDspHandler> > m_handlersNames;
	mutable QReadWriteLock m_rwLock;
	bool m_initWasDone;
};



#define REGISTER_HANDLER( senderType, handlerName, HandlerType ) \
namespace \
{ \
    class CDspHandlerRegistratorHelper \
    { \
    public: \
        CDspHandlerRegistratorHelper () \
		{ \
            SmartPtr<CDspHandler> handler( \
                new HandlerType(senderType, handlerName) ); \
            bool res = CDspHandlerRegistrator::instance().registerHandler( \
                           handler ); \
			PRL_ASSERT(res); \
			Q_UNUSED(res); \
		} \
    }; \
   CDspHandlerRegistratorHelper g_handlerRegistratorHelper__; \
}

#define DSP_ROUTE_TABLE_BEGIN \
namespace \
{ \
    class CDspRouterRegistratorHelper \
	{ \
    public: \
		CDspRouterRegistratorHelper () { \
            bool res = false;


#define DSP_ROUTE_TABLE_END \
	} }; \
    CDspRouterRegistratorHelper g_registratorHelper__; \
} //namespace

#define DSP_ROUTE_BEGIN( SenderType ) \
	res = CDspRouter::instance().registerRoutes( SenderType, QList<CDspRoute>() <<

#define DSP_ROUTE_END \
	CDspRoute() ); PRL_ASSERT(res); Q_UNUSED(res);

#define DSP_ROUTE( TypeRangeBegin, TypeRangeEnd, HandlerName ) \
    CDspRoute( TypeRangeBegin, TypeRangeEnd, HandlerName ) <<



#endif //CDSPHANDLERREGISTRATOR_H
