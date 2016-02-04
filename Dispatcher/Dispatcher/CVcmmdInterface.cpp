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

namespace Vcmmd
{
///////////////////////////////////////////////////////////////////////////////
// struct Api

Api::Api(const QString& uuid_)
{
	PrlUuid uuid(uuid_.toUtf8().data());
	m_uuid = uuid.toString(PrlUuid::WithoutBrackets).data();
}

bool Api::init(quint64 limit_, quint64 guarantee_)
{
	struct vcmmd_ve_config config;
	vcmmd_ve_config_init(&config);
	vcmmd_ve_config_append(&config, VCMMD_VE_CONFIG_GUARANTEE, guarantee_);
	vcmmd_ve_config_append(&config, VCMMD_VE_CONFIG_LIMIT, limit_);

	int r = vcmmd_register_ve(qPrintable(m_uuid), VCMMD_VE_VM, &config);

	if (VCMMD_ERROR_VE_NAME_ALREADY_IN_USE != r)
		return treat(r, "vcmmd_register_ve");

	return treat(vcmmd_unregister_ve(qPrintable(m_uuid)), "vcmmd_unregister_ve") &&
		treat(vcmmd_register_ve(qPrintable(m_uuid), VCMMD_VE_VM, &config),
			"vcmmd_register_ve");
}

bool Api::update(quint64 limit_, quint64 guarantee_)
{
	struct vcmmd_ve_config config;
	vcmmd_ve_config_init(&config);
	vcmmd_ve_config_append(&config, VCMMD_VE_CONFIG_GUARANTEE, guarantee_);
	vcmmd_ve_config_append(&config, VCMMD_VE_CONFIG_LIMIT, limit_);

	return treat(vcmmd_update_ve(qPrintable(m_uuid), &config),
		"vcmmd_update_ve");
}

void Api::deinit()
{
	int r = vcmmd_unregister_ve(qPrintable(m_uuid));
	treat(r, "vcmmd_unregister_ve", r == VCMMD_ERROR_VE_NOT_REGISTERED ? DBG_WARNING : DBG_FATAL);
}

void Api::activate()
{
	int r = vcmmd_activate_ve(qPrintable(m_uuid));
	treat(r, "vcmmd_activate_ve", r == VCMMD_ERROR_VE_ALREADY_ACTIVE ? DBG_WARNING : DBG_FATAL);
}

void Api::deactivate()
{
	int r = vcmmd_deactivate_ve(qPrintable(m_uuid));
	treat(r, "vcmmd_deactivate_ve", r == VCMMD_ERROR_VE_NOT_ACTIVE ? DBG_WARNING : DBG_FATAL);
}

bool Api::treat(int status_, const char* name_, int level_)
{
	if (0 == status_)
		return true;

	char e[255] = {};
	WRITE_TRACE(level_, " failed. %s: %s", name_,
		vcmmd_strerror(status_, e, sizeof(e)));
	return false;
}

///////////////////////////////////////////////////////////////////////////////
// struct Frontend

template <>
void Frontend<Unregistered>::commit()
{
	m_api.reset();
}

} // namespace Vcmmd

