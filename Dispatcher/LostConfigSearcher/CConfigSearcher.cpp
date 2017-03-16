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
 * created:	20:7:2006   12:45
 * filename: 	CConfigSearcher.cpp
 *
 * file base:	CConfigSearcher
 * file ext:	cpp
 * author:		artemr
 *
 * purpose:	config searcher object realization
 *********************************************************************/

#include <QString>
#include <QDir>
#include <QFileInfo>
#include <QThread>
#include <QStringList>
#include "CConfigSearcher.h"
#include "CDspTaskHelper.h"
#ifdef _WIN_
#include "windows.h"
#endif

// By adding this interface we enable allocations tracing in the module
#include "Interfaces/Debug.h"


PRL_RESULT CConfigSearcher::Find(bool bAllDrives)
{
	QDir * 	lpcDir = new QDir;
	// get list of drives for win and root for linux
	QFileInfoList cDrivesList = lpcDir->drives();
	delete lpcDir;
	// cycle on all drives
	for(int i = 0 ; i < cDrivesList.size() ; i++)
	{
		if(!bAllDrives)
#ifdef _WIN_
		switch(GetDriveType((LPWSTR)(cDrivesList.at(i).filePath().utf16())))
		{
			// only fixed disks processed
			case DRIVE_FIXED:
				break;
			default:
				continue;
		}
#endif
		if(DirSearchProc(cDrivesList.at(i).filePath())!=PRL_ERR_SUCCESS)
			return PRL_ERR_SEARCH_CONFIG_OPERATION_CANCELED;
	}
	return PRL_ERR_SUCCESS;
}

PRL_RESULT CConfigSearcher::DirSearchProc(const QString & strDir)
{
	// return if user canceled operation
	if(((CDspTaskHelper*)m_lpcDspTaskHelper)->operationIsCancelled())
		return PRL_ERR_SEARCH_CONFIG_OPERATION_CANCELED;

	QDir	* lpcDir = new QDir(strDir);
	// set filter for directories and files
	lpcDir->setFilter(QDir::Files | QDir::NoDotAndDotDot | QDir::AllDirs);
	// set filter for our config files
	lpcDir->setNameFilters(m_cExtList);
	// get list of files and directoryes
	QFileInfoList cFileList = lpcDir->entryInfoList();

	delete lpcDir;

	// recursive searching dirs
	for(int i = 0; i < cFileList.size();i++ )
	{
		QFileInfo cFileInfo = cFileList.at(i);
		if(cFileInfo.isDir())
		{
			// #125021 to prevent infinity recursion by QT bug in QDir::entryInfoList()
			// #431558 compare paths by spec way to prevent errors with symlinks, unexisting files, ...
			if (CFileHelper::IsPathsEqual(strDir, cFileInfo.absoluteFilePath()))
				continue;

			if(DirSearchProc(cFileInfo.filePath())!=PRL_ERR_SUCCESS)
				return PRL_ERR_SEARCH_CONFIG_OPERATION_CANCELED;
		}
		else
		{
			m_lpfnCallBack(cFileInfo.filePath(),m_lpcDspTaskHelper);
		}
	}
	return PRL_ERR_SUCCESS;
}
