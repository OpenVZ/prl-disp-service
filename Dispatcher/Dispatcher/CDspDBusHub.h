///////////////////////////////////////////////////////////////////////////////
///
/// @file CDspDBusHub.h
///
/// Client manager and handler, which is responsible for all clients and
/// packages for these clients or packages from these clients.
///
/// @author aburluka
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


#ifndef CDSPDBUSHUB_H
#define CDSPDBUSHUB_H

#include <QMap>
#include <QString>
#include <QObject>
#include <QScopedPointer>

class QDBusInterface;
class CDspDBusHub: public QObject
{
	Q_OBJECT

public:
	explicit CDspDBusHub();
	~CDspDBusHub();

public slots:
	void slotCpuFeaturesSync();

private:
	QScopedPointer<QDBusInterface> m_interface;
};

#endif //CDSPDBUSHUB_H
