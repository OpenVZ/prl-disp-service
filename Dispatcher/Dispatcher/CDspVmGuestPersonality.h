///////////////////////////////////////////////////////////////////////////////
///
/// @file CDspVmGuestPersonality.h
///
/// VM guest has a number of "personal" data (IP address, passwords, user list)
/// That class mounts iso image into VM with predefined cloud-init data
///
/// @author aburluka
///
/// Copyright (c) 2015-2017, Parallels International GmbH
/// Copyright (c) 2017-2019 Virtuozzo International GmbH, All rights reserved.
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
/// Our contact details: Virtuozzo International GmbH, Vordergasse 59, 8200
/// Schaffhausen, Switzerland.
///
/////////////////////////////////////////////////////////////////////////////////

#ifndef __DSPVMGUESTPERSONALITY_H__
#define __DSPVMGUESTPERSONALITY_H__

#include <QObject>
#include <prlxmlmodel/VmConfig/CVmConfiguration.h>
#include <boost/function.hpp>

namespace Personalize
{
bool isCloudConfigCd(const CVmDevice* disk_);

///////////////////////////////////////////////////////////////////////////////
// struct Configurator

struct Configurator
{
	Configurator(const CVmConfiguration& cfg_);

	bool setNettool(const QStringList& args_) const;
	bool setUserPassword(const QString& user_, const QString& passwd_, bool encrypted_) const;
	bool merge() const;
	bool clean() const;

private:
	bool execute(const QStringList& args_) const;

private:
	const QString m_vmHome;
	const quint32 m_osVersion;
};

} // namespace Personalize

class CDspVmGuestPersonality: public QObject
{
	Q_OBJECT

public slots:
	void slotVmPersonalityChanged(QString, QString);

private:
	quint32 findGap(const CVmHardware& hardware_,
			boost::function<quint32(const CVmClusteredDevice*)> getter_) const;
	QString tryToConnect(const CVmHardware& hardware_, const QString& image_) const;
	QString prepareNewCdrom(const CVmHardware& hardware_, const QString& image_) const;
};

#endif // __DSPVMGUESTPERSONALITY_H__
