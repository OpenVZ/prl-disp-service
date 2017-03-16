///////////////////////////////////////////////////////////////////////////////
///
/// @file CVmIdent
///
/// Implements struct CVmIdent
///
/// @author sergeyt
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
///////////////////////////////////////////////////////////////////////////////

#ifndef H_CVmIdent_H
#define H_CVmIdent_H

#include <QString>
#include <QPair>

typedef QPair<QString, QString> CVmIdent ;

inline CVmIdent MakeVmIdent(const QString &sVmUuid, const QString &sVmDirUuid)
{
    return qMakePair(sVmUuid, sVmDirUuid) ;
}
inline bool IsValidVmIdent(const CVmIdent &vm_ident)
{
    return !vm_ident.first.isEmpty() && !vm_ident.second.isEmpty() ;
}

#endif //H_CVmIdent_H
