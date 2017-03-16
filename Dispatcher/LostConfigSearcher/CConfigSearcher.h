/*
 * Copyright (c) 2006-2017, Parallels International GmbH
 *
 * This file is part of Virtuozzo Core. Virtuozzo Core is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License as published by the Free Software Foundation;
 * either version 2 of the License, or (at your option) any later
 * version.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 * Our contact details: Parallels International GmbH, Vordergasse 59, 8200
 * Schaffhausen, Switzerland.
 */

/********************************************************************
 * created:	2006/07/20
 * created:	20:7:2006   12:43
 * filename: 	CConfigSearcher.h
 * file base:	CConfigSearcher
 * file ext:	h
 * author:		artemr
 *
 * purpose:	Header for config searcher object
 *********************************************************************/

#include <prlcommon/Interfaces/ParallelsNamespace.h>
class QString;
class QStringList;
#ifdef _WIN_
typedef int (__stdcall * SearchConfigHandler)(const QString &,void * );
#else
typedef int (* SearchConfigHandler)(const QString &,void * );
#endif

class CConfigSearcher
{
public:
	enum ConfigVersion
	{
		InvalidConfig,
		v1_0,
		v2_0,
		v2_1,
		v2_2,
		v3_0,
		v3_1
	};
public:
	CConfigSearcher(SearchConfigHandler lpfnCallback,
				const QStringList & cExtList,
				void * lpcDspTaskHelper)
		:m_lpfnCallBack(lpfnCallback)
	{
		m_cExtList = cExtList;
		m_lpcDspTaskHelper = lpcDspTaskHelper;
	}
	~CConfigSearcher(){};
	PRL_RESULT Find(bool bAllDrives);
protected:
	PRL_RESULT DirSearchProc(const QString & strDir);
protected:
	SearchConfigHandler m_lpfnCallBack;
	QStringList  m_cExtList;
	void * m_lpcDspTaskHelper;
};
