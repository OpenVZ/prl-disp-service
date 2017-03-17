///////////////////////////////////////////////////////////////////////////////
///
/// @file CDspVmBrand.cpp
///
/// VM private brand.
///
/// @author shrike
///
/// Copyright (c) 2005-2017 Parallels IP Holdings GmbH
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
///////////////////////////////////////////////////////////////////////////////

#include "CDspClient.h"
#include "CDspVmBrand.h"
#include <boost/foreach.hpp>
#include <prlcommon/Logging/Logging.h>
#include <Libraries/PrlCommonUtils/CFileHelper.h>

namespace Vm
{
namespace Private
{
///////////////////////////////////////////////////////////////////////////////
// struct Brand

Brand::Brand(const QString& private_, const SmartPtr<CDspClient>& user_):
	m_private(private_), m_user()
{
	if (user_.isValid())
		m_user = &user_->getAuthHelper();
}

PRL_RESULT Brand::stamp()
{       
	QString x = getFolder().absoluteFilePath();
	if (!CFileHelper::WriteDirectory(x, m_user))
	{       
		WRITE_TRACE(DBG_FATAL, "Cannot create folder %s", qPrintable(x));
		return PRL_ERR_MAKE_DIRECTORY;
	}
	QString y = QDir(x).absoluteFilePath(PRODUCT_RELEASE_FILE);
	if (!QFile::copy("/etc/" PRODUCT_RELEASE_FILE, y))
	{       
		WRITE_TRACE(DBG_FATAL, "Cannot brand %s", qPrintable(y));
		return PRL_ERR_COPY_VM_INFO_FILE;
	}
	QFile b(QDir(x).absoluteFilePath("dispatcher-build"));
	if (!b.open(QIODevice::WriteOnly))
	{       
		WRITE_TRACE(DBG_FATAL, "Cannot open %s", qPrintable(b.fileName()));
		return PRL_ERR_OPEN_FAILED;
	}
	if (-1 == b.write(VER_FULL_BUILD_NUMBER_STR"\n"))
	{       
		WRITE_TRACE(DBG_FATAL, "Cannot write to %s", qPrintable(b.fileName()));
		return PRL_ERR_OPEN_FAILED;
	}
	return PRL_ERR_SUCCESS;
}

PRL_RESULT Brand::remove()
{       
	QString x = getFolder().absoluteFilePath(); 
	if (CFileHelper::DirectoryExists(x, m_user) && !CFileHelper::ClearAndDeleteDir(x))
	{       
		WRITE_TRACE(DBG_FATAL, "Cannot remove folder %s", qPrintable(x));
		return PRL_ERR_FAILURE;
	}
	return PRL_ERR_SUCCESS;
}

QFileInfo Brand::getFolder() const
{       
	return QFileInfo(m_private, ".brand");
}

Brand::entryList_type Brand::getFiles() const
{
	entryList_type output;
	QDir x(getFolder().absoluteFilePath());
	QFileInfo a[] = {QFileInfo(x, PRODUCT_RELEASE_FILE), QFileInfo(x, "dispatcher-build")};
	BOOST_FOREACH(const QFileInfo& i, a)
	{
		if (i.exists())
		{
			output << qMakePair(i, QString("%1/%2")
				.arg(x.dirName()).arg(i.fileName()));
		}
	}
	return output;
}

Brand::entryList_type Brand::getFolders() const
{
	QFileInfo x = getFolder();
	entryList_type output;
	if (x.exists())
		output << qMakePair(x, x.fileName());

	return output;
}

} // namespace Private
} // namespace Vm

