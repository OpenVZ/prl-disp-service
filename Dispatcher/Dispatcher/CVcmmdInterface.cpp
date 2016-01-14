///////////////////////////////////////////////////////////////////////////////
///
/// @file CVcmmdInterface
///
/// Implements class VcmmdInterface
///
/// @author mnestratov
///
/// Copyright (c) 2005-2015 Parallels IP Holdings GmbH
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
///////////////////////////////////////////////////////////////////////////////

#include <prlcommon/Logging/Logging.h>
#include <prlcommon/PrlUuid/Uuid.h>
#include "CVcmmdInterface.h"

VcmmdInterface::VcmmdInterface(const QString& uuid, VcmmdState state):
m_uuid(uuid), m_state(state)
{
	m_cleanup = true;
	fixupUuid();
}

VcmmdInterface::~VcmmdInterface()
{
	if (!m_cleanup)
		return;

	switch (m_state)
	{
	case VcmmdVmActive:
		activate();
		break;
	case VcmmdVmUnregistered:
		deinit();
		break;
	default:;
	}
}

void VcmmdInterface::fixupUuid()
{
	PrlUuid uuid(m_uuid.toUtf8().data());
	m_uuid = uuid.toString(PrlUuid::WithoutBrackets).data();
}

bool VcmmdInterface::init(unsigned long long limit, unsigned long long guarantee)
{
	m_cleanup = true;

	if (VcmmdVmUnregistered != m_state)
		return true;

	struct vcmmd_ve_config config;
	vcmmd_ve_config_init(&config);
	vcmmd_ve_config_append(&config, VCMMD_VE_CONFIG_GUARANTEE, guarantee);
	vcmmd_ve_config_append(&config, VCMMD_VE_CONFIG_LIMIT, limit);

	m_err = vcmmd_register_ve(m_uuid.toUtf8().data(), VCMMD_VE_VM, &config, false);
	if (m_err)
	{
		WRITE_TRACE(DBG_FATAL, "vcmmd_register_ve failed. %s",
			vcmmd_strerror(m_err, m_errmsg, sizeof(m_errmsg)));
		return false;
	}
	return true;
}

void VcmmdInterface::commit(VcmmdState state)
{
	switch (state)
	{
	case VcmmdVmActive:
		activate();
		break;
	case VcmmdVmUnregistered:
		deinit();
		break;
	case VcmmdVmInactive:
		deactivate();
		break;
	default:;
	}
	m_cleanup = false;
}

void VcmmdInterface::activate()
{
	m_err = vcmmd_activate_ve(m_uuid.toUtf8().data());
	if (m_err)
	{
		WRITE_TRACE(DBG_FATAL, "vcmmd_activate_ve failed. %s",
			vcmmd_strerror(m_err, m_errmsg, sizeof(m_errmsg)));
	}
}

void VcmmdInterface::deactivate()
{
	m_err = vcmmd_deactivate_ve(m_uuid.toUtf8().data());
	if (m_err)
	{
		WRITE_TRACE(DBG_FATAL, "vcmmd_deactivate_ve failed. %s",
			vcmmd_strerror(m_err, m_errmsg, sizeof(m_errmsg)));
	}
}

void VcmmdInterface::deinit()
{
	m_err = vcmmd_unregister_ve(m_uuid.toUtf8().data());
	if (m_err)
	{
		WRITE_TRACE(DBG_FATAL, "vcmmd_unregister_ve failed. %s",
			vcmmd_strerror(m_err, m_errmsg, sizeof(m_errmsg)));
	}
}

bool VcmmdInterface::update(unsigned long long limit, unsigned long long guarantee)
{
	struct vcmmd_ve_config config;
	vcmmd_ve_config_init(&config);
	vcmmd_ve_config_append(&config, VCMMD_VE_CONFIG_GUARANTEE, guarantee);
	vcmmd_ve_config_append(&config, VCMMD_VE_CONFIG_LIMIT, limit);

	m_cleanup = false;
	m_err = vcmmd_update_ve(m_uuid.toUtf8().data(), &config, false);
	if (m_err)
	{
		WRITE_TRACE(DBG_FATAL, "vcmmd_update_ve failed. %s",
			vcmmd_strerror(m_err, m_errmsg, sizeof(m_errmsg)));
		return false;
	}
	return true;
}

