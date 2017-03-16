/*
 * Copyright (c) 2015-2017, Parallels International GmbH
 *
 * This file is part of Virtuozzo Core Libraries. Virtuozzo Core
 * Libraries is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <http://www.gnu.org/licenses/> or write to Free Software Foundation,
 * 51 Franklin Street, Fifth Floor Boston, MA 02110, USA.
 *
 * Our contact details: Parallels International GmbH, Vordergasse 59, 8200
 * Schaffhausen, Switzerland.
 */

////////////////////////////////////////////////////////////////////////////////
///
/// CVzHardDisk.h
///
///
////////////////////////////////////////////////////////////////////////////////

#ifndef CVZHARDDISK_H
#define CVZHARDDISK_H

#include <prlxmlmodel/VmConfig/CVmHardDisk.h>

class CVzHardDisk : public CVmHardDisk
{
public:
	QString getSystemUuid() const;
	void setSystemUuid(QString value = QString());
private:
	QString		m_qsSystemUuid;
};

#endif
