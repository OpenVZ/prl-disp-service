///////////////////////////////////////////////////////////////////////////////
///
/// @file CFileHelperDepPart.h
///
/// Dispatcher's subset of file system processing methods
///
/// @author sergeyt@
/// @owner sergeym@
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

#ifndef CFileHelperDepPart_H
#define CFileHelperDepPart_H

#include <prlsdk/PrlErrors.h>
#include "Libraries/PrlCommonUtils/CFileHelper.h"

class CDspTaskHelper;

class CFileHelperDepPart : public CFileHelper
{
public:
	 // file copy with cancel check
	 static PRL_RESULT CopyFileWithNotifications(const QString & strSrcFile,
		 const QString & strDestFile,
		 CAuthHelper* pDestOwner,
		 CDspTaskHelper * lpcTaskHelper,
		 PRL_DEVICE_TYPE devType,
		 int iDevNum);

	 static PRL_RESULT CopyDirectoryWithNotifications(const QString& strSourceDir,
		 const QString& strTargetDir,
		 CAuthHelper *pAuthHelper,
		 CDspTaskHelper * lpcTaskHelper,
		 PRL_DEVICE_TYPE devType,
		 int iDevNum,
		 bool bTargetDirNotParent = false
		 );

private:
	// file copy with cancel check
	static PRL_RESULT CopyFileWithNotifications(const QString & strSrcFile,
		const QString & strDestFile,
		CAuthHelper* pDestOwner,
		CDspTaskHelper * lpcTaskHelper,
		PRL_DEVICE_TYPE devType,
		int iDevNum,
		quint64 uiTotalCopySize,
		quint64 uiTotalCompleteSize
		);

	static PRL_RESULT CopyDirectoryWithNotifications(const QString& strSourceDir,
		const QString& strTargetDir,
		CAuthHelper *pAuthHelper,
		CDspTaskHelper * lpcTaskHelper,
		PRL_DEVICE_TYPE devType,
		int iDevNum,
		quint64 uiTotalCopySize,
		quint64& uiTotalCompleteSize,
		bool bTargetDirNotParent = false
		);

};

#endif
