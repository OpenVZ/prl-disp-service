///////////////////////////////////////////////////////////////////////////////
///
/// @file CUnixSignalHandler.cpp
///
/// Class for handling unix signals in Qt-based code.
/// Implementation follows ideas from Qt's documentation article
/// "Calling Qt Functions From Unix Signal Handlers"
/// http://doc.qt.nokia.com/unix-signals.html
///
/// @author sivanov
/// @owner sergeym
///
/// Copyright (c) 2005-2017, Parallels International GmbH
///
/// This file is part of Virtuozzo Core Libraries. Virtuozzo Core
/// Libraries is free software; you can redistribute it and/or modify it
/// under the terms of the GNU Lesser General Public License as published
/// by the Free Software Foundation; either version 2.1 of the License, or
/// (at your option) any later version.
///
/// This library is distributed in the hope that it will be useful,
/// but WITHOUT ANY WARRANTY; without even the implied warranty of
/// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
/// Lesser General Public License for more details.
///
/// You should have received a copy of the GNU Lesser General Public
/// License along with this library.  If not, see
/// <http://www.gnu.org/licenses/> or write to Free Software Foundation,
/// 51 Franklin Street, Fifth Floor Boston, MA 02110, USA.
///
/// Our contact details: Parallels International GmbH, Vordergasse 59, 8200
/// Schaffhausen, Switzerland.
///
///////////////////////////////////////////////////////////////////////////////

#include "CUnixSignalHandler.h"
#include <QtCore/QHash>
#include <QtCore/QSocketNotifier>
#include <QtCore/QMutexLocker>

#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#include <prlcommon/Logging/Logging.h>

class CUnixSignalHandlerPrivate : public QObject
{
	Q_OBJECT

public:
	CUnixSignalHandlerPrivate( CUnixSignalHandler* owner, int signum );
	~CUnixSignalHandlerPrivate();

	static void signalHandler(int signal_, siginfo_t* info_, void* );

	bool setupSockets();
	bool setupHandler();

private slots:
	void onSignal();

public:
	static QHash<int, CUnixSignalHandler*> m_handlers;
	static QMutex m_mutex;

	CUnixSignalHandler* m_owner;
	int m_signum;
	int m_fd[2];
	QSocketNotifier* m_notifier;
	bool m_actionInstalled;
	struct sigaction m_prevAction;
};

QHash<int, CUnixSignalHandler*> CUnixSignalHandlerPrivate::m_handlers;
QMutex CUnixSignalHandlerPrivate::m_mutex;

CUnixSignalHandlerPrivate::CUnixSignalHandlerPrivate( CUnixSignalHandler* owner, int signum )
	: QObject( owner )
	, m_owner( owner )
	, m_signum( signum )
	, m_notifier( 0 )
	, m_actionInstalled( false )
{
}

CUnixSignalHandlerPrivate::~CUnixSignalHandlerPrivate()
{
	// Restore previous signal handler
	if ( m_actionInstalled )
		sigaction( m_signum, &m_prevAction, 0 );

	//Close socket pair
	if ( m_notifier ) {
		if ( ::close( m_fd[0] ) )
			WRITE_TRACE( DBG_FATAL, "Error on close socket" );
		if ( ::close( m_fd[1] ) )
			WRITE_TRACE( DBG_FATAL, "Error on close socket" );
	}
}

void CUnixSignalHandlerPrivate::signalHandler(int signal_, siginfo_t* info_, void* )
{
	CUnixSignalHandler* handler = m_handlers.value(signal_);
	if (NULL != handler)
	{
		int fd = handler->d->m_fd[0];
		pid_t a = NULL == info_ ? -1 : info_->si_pid;
		ssize_t nb = ::write(fd, &a, sizeof(a));
		Q_UNUSED(nb)
	}
}

bool CUnixSignalHandlerPrivate::setupSockets()
{
	if ( ::socketpair( AF_UNIX, SOCK_STREAM, 0, m_fd ) )
		return false;

	m_notifier = new QSocketNotifier( m_fd[1], QSocketNotifier::Read, this );
	connect( m_notifier, SIGNAL(activated(int)), this, SLOT(onSignal()) );
	return true;
}

bool CUnixSignalHandlerPrivate::setupHandler()
{
	struct sigaction newAction;

	newAction.sa_sigaction = CUnixSignalHandlerPrivate::signalHandler;
	sigemptyset( &newAction.sa_mask );
	newAction.sa_flags = SA_RESTART | SA_SIGINFO;

	if ( sigaction( m_signum, &newAction, &m_prevAction ) > 0 )
		return false;

	m_actionInstalled = true;
	return true;
}

void CUnixSignalHandlerPrivate::onSignal()
{
	m_notifier->setEnabled(false);
	pid_t tmp = 0;
	if (::read(m_fd[1], &tmp, sizeof(tmp)) == (ssize_t) sizeof(tmp)) {
		WRITE_TRACE(DBG_FATAL, "Signal %d sender pid is %d", m_signum, tmp);
		emit m_owner->signalReceived();
		emit m_owner->signalReceived(m_signum);
	}
	else {
		WRITE_TRACE(DBG_FATAL, "Error on read from socket");
	}

	m_notifier->setEnabled(true);
}

///////////////////////////////////////////////////////////////////////////////

CUnixSignalHandler::CUnixSignalHandler( int signum )
	: d( new CUnixSignalHandlerPrivate( this, signum ) )
{
}

CUnixSignalHandler* CUnixSignalHandler::installHandler( int signum )
{
	QMutexLocker locker( & CUnixSignalHandlerPrivate::m_mutex );

	CUnixSignalHandler* handler = CUnixSignalHandlerPrivate::m_handlers.value( signum );
	if ( ! handler ) {
		handler = new CUnixSignalHandler( signum );

		if ( handler->d->setupSockets() && handler->d->setupHandler() ) {
			CUnixSignalHandlerPrivate::m_handlers.insert( signum, handler );
		}
		else {
			delete handler;
			handler = 0;
		}
	}

	return handler;
}

void CUnixSignalHandler::ignore(int signum)
{
	struct sigaction newAction;

	newAction.sa_handler = SIG_IGN;
	sigemptyset(&newAction.sa_mask);
	newAction.sa_flags = 0;
	newAction.sa_flags |= SA_RESTART;

	sigaction(signum, &newAction, NULL);
}

CUnixSignalHandler* CUnixSignalHandler::getHandler( int signum )
{
	QMutexLocker locker( & CUnixSignalHandlerPrivate::m_mutex );
	return CUnixSignalHandlerPrivate::m_handlers.value( signum );
}

void CUnixSignalHandler::removeHandler( int signum )
{
	QMutexLocker locker( & CUnixSignalHandlerPrivate::m_mutex );
	CUnixSignalHandler* handler = CUnixSignalHandlerPrivate::m_handlers.value( signum );
	if ( handler ) {
		delete handler;
		CUnixSignalHandlerPrivate::m_handlers.remove( signum );
	}
}

int CUnixSignalHandler::signalId()
{
	return d->m_signum;
}

#include "CUnixSignalHandler.moc"
