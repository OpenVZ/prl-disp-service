///////////////////////////////////////////////////////////////////////////////
///
/// @file CVcmmdInterface
///
/// Implements class VcmmdInterface
///
/// @author mnestratov
///
/// Copyright (c) 2005-2017, Parallels International GmbH
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
///////////////////////////////////////////////////////////////////////////////
#pragma once

#include <QString>
#include <QScopedPointer>
#include <QSharedPointer>
#include <libvcmmd/vcmmd.h>
#include <boost/utility/result_of.hpp>
#include <prlcommon/PrlCommonUtilsBase/SysError.h>
#include "CDspVmConfigManager.h"
#include <prlsdk/PrlErrors.h>
#include <prlxmlmodel/VcmmdConfig/CVcmmdConfig.h>
#include <boost/optional.hpp>

/*
 * This is a helper class to interact with vcmmd daemon.
 * There are three use case:
 * 1. Registering
 * 2. Unregistering
 * 3. Updating
 *
 * Registering/unregistering is always a transaction. I.e.
 * corresponding preparation call should always be followed by
 * commit call in case of success or revert in case of failure.
 * For instance:
 *  - register(init->VM start OK->activate) or
 *  - register(init->VM start FAIL->deinit)
 *  - unregister(deactivate->stop VM OK->unregister) or
 *  - unregister(deactivate->stop VM fails->activate)
 * The class is written in such a way that it requires explicit
 * calling in case of VM operation succeeds, and all the cleanup
 * will be done implicitly in the class destructor.
 *
 */

namespace Vcmmd
{
namespace Config
{
///////////////////////////////////////////////////////////////////////////////
// struct DAO

struct DAO
{
	PRL_RESULT getPersistent(CVcmmdConfig& config_) const;
	PRL_RESULT getRuntime(CVcmmdConfig& config_) const;
	PRL_RESULT set(const CVcmmdConfig& config_) const;
};

namespace Vm
{
///////////////////////////////////////////////////////////////////////////////
// struct Model

struct Model
{
	typedef ::Vm::Config::MemGuarantee guarantee_type;

	void setRam(quint64 value_)
	{
		m_ram = value_;
	}

	const boost::optional<quint64>& getRam() const
	{
		return m_ram;
	}

	void setVideoRam(quint64 value_)
	{
		m_vram = value_;
	}

	const boost::optional<quint64>& getVideoRam() const
	{
		return m_vram;
	}

	void setCpuMask(const QString& value_)
	{
		m_cpuMask = value_;
	}

	const QString& getCpuMask() const
	{
		return m_cpuMask;
	}

	void setNodeMask(const QString& value_)
	{
		m_nodeMask = value_;
	}

	const QString& getNodeMask() const
	{
		return m_nodeMask;
	}

	void setGuarantee(const guarantee_type& value_)
	{
		m_guarantee = value_;
	}

	const boost::optional<guarantee_type>& getGuarantee() const
	{
		return m_guarantee;
	}

private:
	boost::optional<quint64> m_ram;
	boost::optional<quint64> m_vram;
	QString m_cpuMask, m_nodeMask;
	boost::optional<guarantee_type> m_guarantee;
};

///////////////////////////////////////////////////////////////////////////////
// struct Marshal

struct Marshal
{
	explicit Marshal(vcmmd_ve_config& dataStore_): m_dataStore(&dataStore_)
	{
	}

	void load(Model& dst_);
	void save(const Model& ve_);

private:
	vcmmd_ve_config* m_dataStore;
};

} // namespace Vm
} // namespace Config
} // namespace Vcmmd

