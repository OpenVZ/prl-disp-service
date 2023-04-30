/*
 * Copyright (c) 2015-2017, Parallels International GmbH
 * Copyright (c) 2017-2019 Virtuozzo International GmbH. All rights reserved.
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
 * Our contact details: Virtuozzo International GmbH, Vordergasse 59, 8200
 * Schaffhausen, Switzerland.
 */

#ifndef OVMFHELPER_H
#define OVMFHELPER_H

#include <Libraries/CpuFeatures/ChipsetHelper.h>

struct OVMF
{
	static QString getFirmware(Chipset_type machine_type);
	static QString getTemplate(Chipset_type machine_type);
};

class NvramUpdater
{

	enum class UEFI_TASKS {
		UEFI_DUMP_VARS,
		UEFI_RESTORE_VARS,
	};

public:
	NvramUpdater(const CVmConfiguration &config_);
	~NvramUpdater();
	bool updateNVRAM();


	bool isOldVerison();
	static QStringList generateQemuArgs(const QString &ovmfCode, const QString &ovmfVars, const QString &disk);
	const QString &getNewNvramPath() const { return m_newNvram; }

private:

	bool runUefiShell(const UEFI_TASKS task);
	bool sendUefiEscape(QProcess &p);
	bool runCmd(const QStringList &cmd);

	bool lock();
	void unlock();
	const QString findBootHDD() const;
	const CVmHardDisk *findDiskByIndex(const QList<CVmHardDisk* >& list, unsigned int index) const;

private:
	const CVmConfiguration&		m_input;
	QFileInfo					m_oldNvram;
	QFileInfo					m_tmpNvram;
	QString						m_newNvram;
	QString						m_storage;

	static QMutex s_mutexNvramList;
	static QList<QString> s_NvramList;
};

#endif // OVMFHELPER_H
