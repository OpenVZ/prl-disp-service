/*
 * Copyright (c) 2015 Parallels IP Holdings GmbH
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
 * Our contact details: Parallels IP Holdings GmbH, Vordergasse 59, 8200
 * Schaffhausen, Switzerland.
 */

#include <QString>
#include <QList>
#include <QPair>
#include <QFileInfo>
#include <prlsdk/PrlTypes.h>
#include <prlxmlmodel/VmConfig/CVmHardDisk.h>

#ifndef VMMIGRATEHELPER_H
#define VMMIGRATEHELPER_H

struct CVmMigrateHelper {

	typedef QList<QPair<QFileInfo, QString> > EntryList;

	static PRL_RESULT GetEntryListsVmHome(const QString & homeDir, EntryList & dirList, EntryList & fileList);

	static EntryList FillRemotePathsAbsolute(QList<QFileInfo> & list);
	static EntryList FillRemotePathsRelative(const QString & parent, QList<QFileInfo> & list);
	static bool isInsideSharedVmPrivate(const QString& path_);

	static PRL_RESULT createSharedFile(const QString &dir, QString &tmpFile);
	static PRL_RESULT buildExternalTmpFiles(const QList<CVmHardDisk*> &disks,
						const QString &home,
						QStringList &tmpFiles);

	static void buildNonSharedDisks(const QStringList &checkFiles,
					const QList<CVmHardDisk*> &disks,
					QStringList &nonSharedDisks);
};

#endif // VMMIGRATEHELPER_H
