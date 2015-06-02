/////////////////////////////////////////////////////////////////////////////
///
/// Copyright (C) 2006-2015 Parallels IP Holdings GmbH
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
/// @file
///		CMockPveEventsHandler.h
///
/// @author
///		sergeyt
///
/// @brief
///		Stub class for CPveControl events handling.
///
/// @brief
///		None.
///
/////////////////////////////////////////////////////////////////////////////

#ifndef CMockPveEventsHandler_H
#define CMockPveEventsHandler_H

#include <QObject>
#include <QMutex>

#include "XmlModel/Messaging/CResult.h"

class CMockPveEventsHandler : public QObject {
Q_OBJECT
public:
	CMockPveEventsHandler(QObject *pParent = 0) : QObject(pParent) {}
	CResult GetResult();
	void Clear();

private:
	bool event(QEvent *e);

private:
	/** Storing result of the last PveControl event received */
	CResult m_Result;
	/** Mutex for synchronize simultaneously multi threads operations with storing result */
	QMutex m_Mutex;
};

#endif
