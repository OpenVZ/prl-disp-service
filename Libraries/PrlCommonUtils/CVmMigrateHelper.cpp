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
#include <prlcommon/Logging/Logging.h>
#include <prlcommon/Interfaces/ParallelsNamespace.h>
#include <prlcommon/Interfaces/ParallelsQt.h>

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

PRL_RESULT CVmMigrateHelper::buildExternalTmpFiles(const QList<CVmHardDisk*> &disks,
						const QString &home,
						QStringList &tmpFiles)
{
	QSet<QString> dirs;

	// first build list of uniq directories to check
	foreach (const CVmHardDisk* disk, disks)
	{
		if (NULL == disk)
			continue;
		QFileInfo img(disk->getSystemName());
		// disk is in a vm bundle
		if (!img.isAbsolute() || img.absoluteFilePath().startsWith(home))
			continue;

		dirs.insert(img.dir().absolutePath());
	}

	// second actually create tmp files
	foreach (const QString &dir, dirs)
	{
		QString tmpFile;
		PRL_RESULT r = createSharedFile(dir, tmpFile);
		if (PRL_FAILED(r)) {
			foreach (const QString &tmpFile, tmpFiles)
				QFile::remove(tmpFile);
			return r;
		}
		// absolute path is passed for external disks check file
		tmpFiles.append(tmpFile);
	}

	return PRL_ERR_SUCCESS;
}

void CVmMigrateHelper::buildNonSharedDisks(const QStringList &checkFiles,
			const QList<CVmHardDisk*> &disks,
			QStringList &nonSharedDisks)
{
	QStringList nonSharedDirs;

	// first check out for non shared files
	foreach (const QString & checkFile, checkFiles)
	{
		QFileInfo i(checkFile);
		if (i.exists())
			continue;
		nonSharedDirs << i.dir().absolutePath();
	}

	// second find all external disks that are in non shared dirs
	foreach (const CVmHardDisk* disk, disks)
	{
		if (NULL == disk)
			continue;
		QFileInfo i(disk->getSystemName());
		// disk is in a vm bundle
		if (!i.isAbsolute())
			continue;
		if (!nonSharedDirs.contains(i.dir().absolutePath()))
			continue;
		// we use independent dir separators in protocol
		nonSharedDisks.append(QDir::fromNativeSeparators(i.absoluteFilePath()));
		WRITE_TRACE(DBG_INFO, "External disk %s is on private media.",
			    QSTR2UTF8(i.absoluteFilePath()));
	}
}

PRL_RESULT CVmMigrateHelper::createSharedFile(const QString &dir, QString &tmpFile)
{
	QTemporaryFile f(
		QString("%1/%2.XXXXXX")
			.arg(QDir::fromNativeSeparators(dir))
			.arg(Uuid::createUuid().toString())
		);

	if (!f.open())
	{
		WRITE_TRACE(DBG_FATAL,
			    "Failed to create/open temparary file %s for shared storage check",
			    QSTR2UTF8(f.fileName()));
		return PRL_ERR_OPERATION_FAILED;
	}

	f.setAutoRemove(false);
	tmpFile = f.fileName();

	return PRL_ERR_SUCCESS;
}
