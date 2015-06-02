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

#include "CFileHelper.h"
#include "CVmMigrateHelper.h"
#include "Libraries/Logging/Logging.h"
#include "Interfaces/ParallelsNamespace.h"
#include "Interfaces/ParallelsQt.h"

#include <QDir>

namespace {

typedef CVmMigrateHelper::EntryList EntryList;

PRL_RESULT GetEntryLists(const QString & topDir, QList<QFileInfo> & dirList, QList<QFileInfo> & fileList)
{
	QFileInfo dirInfo;
	QFileInfoList entryList;
	QDir dir;
	QDir startDir(topDir);
	int i, top_i;
	QFileInfo config, config_backup, log, statlog;

	dirInfo.setFile(topDir);
	if (!dirInfo.exists()) {
		WRITE_TRACE(DBG_FATAL, "Directory %s does not exist", QSTR2UTF8(topDir));
		return (PRL_ERR_VMDIR_INVALID_PATH);
	}

	QString dirInfoPath;
	dirList.append(dirInfo);
	top_i = dirList.size() - 1;
	for (i = top_i; i < dirList.size(); ++i) {
		/* CDir::absoluteDir() is equal CDir::dir() : return parent directory */
		dir.setPath(dirList.at(i).absoluteFilePath());

		entryList = dir.entryInfoList(QDir::NoDotAndDotDot | QDir::Files | QDir::Dirs | QDir::Hidden);

		WRITE_TRACE(DBG_DEBUG, "Directory %s", QSTR2UTF8(dirList.at(i).absoluteFilePath()));

		foreach (const QFileInfo& fileInfo, entryList) {
			if (dirInfo == fileInfo) {
				WRITE_TRACE(DBG_FATAL, "Infinite recursion in : %s", QSTR2UTF8(dirInfo.absoluteFilePath()));
				return (PRL_ERR_FAILURE);
			}
			if (!fileInfo.absoluteFilePath().startsWith(topDir)) {
				WRITE_TRACE(DBG_FATAL, "Path %s does not starts from dir (%s)",
					QSTR2UTF8(fileInfo.absoluteFilePath()),
					QSTR2UTF8(topDir));
				return PRL_ERR_FAILURE;
			}
			if (fileInfo.isDir())
				dirList.append(fileInfo);
			else
				fileList.append(fileInfo);
			WRITE_TRACE(DBG_DEBUG, "%x\t%s.%s\t%s",
				int(fileInfo.permissions()),
				QSTR2UTF8(fileInfo.owner()),
				QSTR2UTF8(fileInfo.group()),
				QSTR2UTF8(fileInfo.absoluteFilePath()));
		}
		entryList.clear();
	}
	// remove top directory
	dirList.removeAt(top_i);

	return PRL_ERR_SUCCESS;
}

}

EntryList CVmMigrateHelper::FillRemotePathsRelative(const QString & parent, QList<QFileInfo> & list)
{
	EntryList r;
	QDir d(parent);
	foreach (const QFileInfo &fi, list) {
		QString p = d.relativeFilePath(fi.absoluteFilePath());
		r.append(qMakePair(fi, p));
	}
	return r;
}

EntryList CVmMigrateHelper::FillRemotePathsAbsolute(QList<QFileInfo> & list)
{
	EntryList r;
	foreach (const QFileInfo &fi, list)
		r.append(qMakePair(fi, fi.absoluteFilePath()));
	return r;
}

PRL_RESULT CVmMigrateHelper::GetEntryListsExternal(const QStringList & paths, EntryList & dirList, EntryList & fileList)
{
	foreach (const QString & path, paths) {
		QList<QFileInfo> dl, fl;

		PRL_RESULT rc = GetEntryLists(path, dl, fl);
		if (PRL_FAILED(rc))
			return rc;
		dirList.append(FillRemotePathsAbsolute(dl));
		fileList.append(FillRemotePathsAbsolute(fl));
	}

	return PRL_ERR_SUCCESS;
}

PRL_RESULT CVmMigrateHelper::GetEntryListsVmHome(const QString & home, EntryList & dirList, EntryList & fileList)
{
	QList<QFileInfo> dl, fl;
	PRL_RESULT rc = GetEntryLists(home, dl, fl);
	if (PRL_FAILED(rc))
		return rc;

	// filter some files out
	QDir homeDir(home);
	fl.removeOne(QFileInfo(home, VMDIR_DEFAULT_VM_CONFIG_FILE));
	fl.removeOne(QFileInfo(home, VMDIR_DEFAULT_VM_CONFIG_FILE VMDIR_DEFAULT_VM_BACKUP_SUFFIX));
	fl.removeOne(QFileInfo(home, "parallels.log"));

	// copy statistic file to different path on remote side
	QFileInfo statlog(home, PRL_VMTIMING_LOGFILENAME);
	bool hasStat = fl.removeOne(statlog);

	dirList.append(FillRemotePathsRelative(home, dl));
	fileList.append(FillRemotePathsRelative(home, fl));

	if (hasStat)
		fileList.append(qMakePair(statlog, QString(PRL_VMTIMING_LOGFILENAME VMDIR_DEFAULT_VM_MIGRATE_SUFFIX)));

	return rc;
}

bool CVmMigrateHelper::isInsideSharedVmPrivate(const QString& path_)
{
	QFileInfo d(path_);
	while (!d.isRoot())
	{
		QString p = d.absolutePath();
		d.setFile(p);
		if (!d.exists())
			continue;

		if (!CFileHelper::isSharedFS(p))
			break;

		if (d.fileName().endsWith(VMDIR_DEFAULT_BUNDLE_SUFFIX))
			return true;
	}
	return false;
}
