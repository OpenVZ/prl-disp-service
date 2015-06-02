///////////////////////////////////////////////////////////////////////////////
///
/// @file CUnixSignalHandler.h
///
/// Class for handling unix signals in Qt-based code.
/// Implementation follows ideas from Qt's documentation article
/// "Calling Qt Functions From Unix Signal Handlers"
/// http://doc.qt.nokia.com/unix-signals.html
///
/// @author sivanov
/// @owner sergeym
///
/// Copyright (c) 2005-2015 Parallels IP Holdings GmbH
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
/// Our contact details: Parallels IP Holdings GmbH, Vordergasse 59, 8200
/// Schaffhausen, Switzerland.
///
///////////////////////////////////////////////////////////////////////////////

#ifndef C_UNIX_SIGNAL_HANDLER_H
#define C_UNIX_SIGNAL_HANDLER_H

#include <QtCore/QObject>

class CUnixSignalHandlerPrivate;

class CUnixSignalHandler : public QObject
{
	Q_OBJECT

public:
	static CUnixSignalHandler* installHandler( int signum );
	static CUnixSignalHandler* getHandler( int signum );
	static void removeHandler( int signum );

	int signalId();

signals:
	void signalReceived();
	void signalReceived( int signum );

private:
	CUnixSignalHandler( int signum );

private:
	friend class CUnixSignalHandlerPrivate;
	CUnixSignalHandlerPrivate* d;
};

#endif // C_UNIX_SIGNAL_HANDLER_H
