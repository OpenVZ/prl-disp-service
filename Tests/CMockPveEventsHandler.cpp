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
///		CPveEventsHandler.cpp
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

#include "CMockPveEventsHandler.h"

#include <QMutexLocker>

CResult CMockPveEventsHandler::GetResult() {
	QMutexLocker _lock(&m_Mutex);
	return (m_Result);
}

bool CMockPveEventsHandler::event(QEvent *pEvent) {
	if ((PVE::VmEventClassType)pEvent->type() == PVE::CResultType) {
		CResult *pResult = (CResult *)pEvent;
		QMutexLocker _lock(&m_Mutex);
		m_Result = *pResult;
	}
	return (true);
}

void CMockPveEventsHandler::Clear() {
	QMutexLocker _lock(&m_Mutex);
	m_Result = CResult();
}
