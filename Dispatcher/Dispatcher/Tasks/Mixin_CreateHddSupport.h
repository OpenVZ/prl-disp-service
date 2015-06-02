////////////////////////////////////////////////////////////////////////////////
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
/// @file
///	Mixin_CreateHddSupport.h
///
/// @brief
///	Definition of the class CDspTaskHelper
///
/// @brief
///	This class implements long running tasks helper class
///
/// @author sergeyt
///	SergeyT@
///
////////////////////////////////////////////////////////////////////////////////

#ifndef __Mixin_CreateHddSupport_H_
#define __Mixin_CreateHddSupport_H_

#include <QString>
#include <QSemaphore>
#include <QList>

#include "CDspTaskHelper.h"
#include <prlsdk/PrlErrors.h>

class Mixin_CreateHddSupport;

bool HddCallbackHelperFunc ( int iDone, int iTotal,
							 Mixin_CreateHddSupport* );

//common base class to use in callback
class Mixin_CreateHddSupport : public CDspTaskHelper
{
	friend  bool HddCallbackHelperFunc ( int iDone, int iTotal,
										 Mixin_CreateHddSupport* );
public:
	Mixin_CreateHddSupport( SmartPtr<CDspClient>&,
							const SmartPtr<IOPackage>&, bool bForceQuestionsSign = false );
	virtual ~Mixin_CreateHddSupport();

	/** Wait for wakeup from HDD callback */
	void wait ();

	/** Wakes up task from callback */
	void wakeTask ();

	/**
	 * Overridable method which returns VM uuid - HDD owner
	 */
	virtual QString getVmUuid();

	// Create XML file for physical drive
	static PRL_RESULT ConfigurePhysical(
		const QString& qsFullPath,
		const QString& qsDiskName,
		CAuthHelper* pAuthHelper,
		CVmEvent* pOutErrParams );

protected:
	// if no error occurred return PRL_ERR_SUCCESS
	PRL_RESULT	getHddErrorCode();
	QString		getHddAdvancedError();

private:
	int  getHddCurrentPercent();
	void setHddCurrentPercent(int i);

	void setHddErrorCode( PRL_RESULT errCode );
	// may be empty
	void setHddAdvancedError( const QString& strAdvancedErrorInfo );

private:
	int m_nCurrentPercent;
	QString m_strJobUuidCreateHdd;

	PRL_RESULT	m_HddError;
	QString		m_strAdvancedErrorInfo;
	QSemaphore  m_semaphore;
};


#endif //__Mixin_CreateHddSupport_H_
