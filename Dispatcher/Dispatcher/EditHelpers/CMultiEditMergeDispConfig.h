///////////////////////////////////////////////////////////////////////////////
///
/// @file CMultiEditMergeDispConfig.h
///
/// This class implements logic for simultaneously
/// edit & merge dispatcher configuration
///
/// @author myakhin@
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
/////////////////////////////////////////////////////////////////////////////////

#ifndef CMultiEditMergeDispConfig_H
#define CMultiEditMergeDispConfig_H

#include "CMultiEditMergeHelper.h"
#include "XmlModel/DispConfig/CDispatcherConfig.h"


class CMultiEditMergeDispConfig: public CMultiEditMergeHelper<CDispatcherConfig >
{
public:

	void registerBeginEdit( const IOSender::Handle& sUserAccessToken,
							SmartPtr<CDispatcherConfig >& pPrev, const QString& qsIdExt = QString());

	void registerCommit(const IOSender::Handle& sUserAccessToken, const QString& qsIdExt = QString());

	bool tryToMerge(const IOSender::Handle& sUserAccessToken,
					SmartPtr<CDispatcherConfig >& pNewConf,
					const SmartPtr<CDispatcherConfig >& pCurrConf,
					const QString& qsIdExt = QString());

protected:

	virtual bool merge(const SmartPtr<CDispatcherConfig>& pPrev,
					   SmartPtr<CDispatcherConfig>& pNew,
					   const SmartPtr<CDispatcherConfig>& pCurr);

};


#endif	// CMultiEditMergeDispConfig_H
