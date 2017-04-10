///////////////////////////////////////////////////////////////////////////////
///
/// @file CDspVmBrand.h
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

#ifndef __CVMPRIVATEBRAND_H__
#define __CVMPRIVATEBRAND_H__

#include "CDspTaskHelper.h"
#include "CVmFileListCopy.h"

class CDspClient;
class CAuthHelper;

namespace Vm
{
namespace Private
{
///////////////////////////////////////////////////////////////////////////////
// struct Brand

struct Brand
{
	typedef CVmFileListCopySource::objectList_type entryList_type;

	Brand(const QString& private_, CAuthHelper* user_):
		m_private(private_), m_user(user_)
	{
	}
	Brand(const QString& private_, const SmartPtr<CDspClient>& user_);

	PRL_RESULT stamp(boost::optional<CDspTaskFailure> sink_ = boost::none);
	PRL_RESULT remove();
	entryList_type getFiles() const;
	entryList_type getFolders() const;

private:
	QFileInfo getFolder() const;

	QDir m_private;
	CAuthHelper* m_user;
};

} // namespace Private
} // namespace Vm

#endif // __CVMPRIVATEBRAND_H__

