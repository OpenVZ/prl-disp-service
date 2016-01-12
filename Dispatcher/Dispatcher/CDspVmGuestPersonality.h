///////////////////////////////////////////////////////////////////////////////
///
/// @file CDspVmGuestPersonality.h
///
/// VM guest has a number of "personal" data (IP address, passwords, user list)
/// That class mounts iso image into VM with predefined cloud-init data
///
/// @author aburluka
///
/// Copyright (c) 2015-2016 Parallels IP Holdings GmbH
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

#ifndef __DSPVMGUESTPERSONALITY_H__
#define __DSPVMGUESTPERSONALITY_H__

#include <QObject>
#include <prlxmlmodel/VmConfig/CVmConfiguration.h>

namespace Personalize
{
///////////////////////////////////////////////////////////////////////////////
// struct Configurator

struct Configurator
{
	Configurator(const CVmConfiguration& cfg_)
		: m_cfg(cfg_)
	{
	}

	bool setNettool(const QStringList& args_) const;
	bool setUserPassword(const QString& user_, const QString& passwd_, bool encrypted_) const;

private:
	bool execute(const QStringList& args_) const;

private:
	const CVmConfiguration& m_cfg;
};

} // namespace Personalize

class CDspVmGuestPersonality: public QObject
{
	Q_OBJECT

public slots:
	void slotVmConfigChanged(QString, QString);

private:
	QString prepareNewCdrom(const QString& image_, quint32 index_) const;
	QString getHomeDir(const QString& dirUuid_, const QString& vmUuid_) const;
};

#endif // __DSPVMGUESTPERSONALITY_H__
