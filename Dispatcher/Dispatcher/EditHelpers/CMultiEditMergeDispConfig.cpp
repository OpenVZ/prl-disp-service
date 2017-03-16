///////////////////////////////////////////////////////////////////////////////
///
/// @file CMultiEditMergeDispConfig.cpp
///
/// This class implements logic for simultaneously
/// edit & merge dispatcher configuration
///
/// @author myakhin@
///
/// Copyright (c) 2005-2017, Parallels International GmbH
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
/// Our contact details: Parallels International GmbH, Vordergasse 59, 8200
/// Schaffhausen, Switzerland.
///
/////////////////////////////////////////////////////////////////////////////////

#include "CMultiEditMergeDispConfig.h"
#include <prlcommon/PrlCommonUtilsBase/PrlStringifyConsts.h>


#define DISPATCHER_CONFIG_EDIT_ID	QString("dispatcher configuration")


void CMultiEditMergeDispConfig::registerBeginEdit(const IOSender::Handle& sUserAccessToken,
												  SmartPtr<CDispatcherConfig >& pPrev,
												  const QString& qsIdExt)
{
	CMultiEditMergeHelper<CDispatcherConfig >::registerBeginEdit(
		DISPATCHER_CONFIG_EDIT_ID + qsIdExt, sUserAccessToken, pPrev);
}

void CMultiEditMergeDispConfig::registerCommit(const IOSender::Handle& sUserAccessToken,
											   const QString& qsIdExt)
{
	CMultiEditMergeHelper<CDispatcherConfig >::registerCommit(
		DISPATCHER_CONFIG_EDIT_ID + qsIdExt, sUserAccessToken);
}

bool CMultiEditMergeDispConfig::tryToMerge( const IOSender::Handle& sUserAccessToken,
											SmartPtr<CDispatcherConfig >& pNewConf,
											const SmartPtr<CDispatcherConfig >& pCurrConf,
											const QString& qsIdExt)
{
	return CMultiEditMergeHelper<CDispatcherConfig >::tryToMerge(
		DISPATCHER_CONFIG_EDIT_ID + qsIdExt, sUserAccessToken, pNewConf, pCurrConf);
}

bool CMultiEditMergeDispConfig::merge(const SmartPtr<CDispatcherConfig>& pPrev,
									  SmartPtr<CDispatcherConfig>& pNew,
									  const SmartPtr<CDispatcherConfig>& pCurr)
{
	int rc = pNew->mergeDocuments( pCurr.getImpl(), pPrev.getImpl(), moEnableAllOptions );

	WRITE_TRACE( DBG_INFO, "Merge was finished with result %#x %s, '%s'",
					rc,
					PRL_RESULT_TO_STRING(rc ),
					QSTR2UTF8(pNew->GetErrorMessage()) );

	return 0 == rc;
}
