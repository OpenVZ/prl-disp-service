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

#include <prlsdk/PrlOses.h>
#include <prlcommon/Logging/Logging.h>
#include <prlcommon/PrlUuid/Uuid.h>
#include "CVcmmdInterface.h"

namespace
{
///////////////////////////////////////////////////////////////////////////////
// struct Status

struct Status
{
	explicit Status(int status_) : m_status(status_)
	{
	}

	Status& log(const char *name_, int level_ = DBG_FATAL);
	PRL_RESULT treat();

private:
	int m_status;
};

Status& Status::log(const char *name_, int level_)
{
	if (0 != m_status)
	{
		char e[255] = {};
		WRITE_TRACE(level_, " failed. %s: %s", name_,
				vcmmd_strerror(m_status, e, sizeof(e)));
	}
	return *this;
}

PRL_RESULT Status::treat()
{
	switch (m_status)
	{
	case 0:
		return PRL_ERR_SUCCESS;
	case VCMMD_ERROR_CONNECTION_FAILED:
		return PRL_ERR_VCMMD_NO_CONNECTION;
	case VCMMD_ERROR_UNABLE_APPLY_VE_GUARANTEE:
		return PRL_ERR_UNABLE_APPLY_MEMORY_GUARANTEE;
	case VCMMD_ERROR_POLICY_SET_INVALID_NAME:
		return PRL_ERR_INVALID_ARG;
	case VCMMD_ERROR_POLICY_SET_ACTIVE_VES:
		return PRL_ERR_RUNNING_VM_OR_CT;
	default:
		return PRL_ERR_FAILURE;
	}
}

} //anonymous namespace

namespace Vcmmd
{
///////////////////////////////////////////////////////////////////////////////
// struct Api

Api::Api(const QString& uuid_)
{
	PrlUuid uuid(uuid_.toUtf8().data());
	m_uuid = uuid.toString(PrlUuid::WithoutBrackets).data();
}

PRL_RESULT Api::init(const SmartPtr<CVmConfiguration>& config_)
{
	if (!config_.isValid())
		return PRL_ERR_INVALID_PARAM;

	Config::Vm::Model m;
	vcmmd_ve_config vcmmdConfig;
	CVmMemory* memory = config_->getVmHardwareList()->getMemory();
	m.setRam(quint64(memory->getRamSize()));
	m.setVideoRam(config_->
		getVmHardwareList()->getVideo()->getMemorySize());
	m.setCpuMask(config_->getVmHardwareList()->getCpu()->getCpuMask());
	m.setNodeMask(config_->getVmHardwareList()->getCpu()->getNodeMask());
	m.setGuarantee(Config::Vm::Model::guarantee_type(*memory));

	Config::Vm::Marshal(vcmmdConfig).save(m);
	vcmmd_ve_type_t vmType = VCMMD_VE_VM;
	switch(config_->getVmSettings()->getVmCommonOptions()->getOsType()){
		case PVS_GUEST_TYPE_WINDOWS:
			vmType = VCMMD_VE_VM_WINDOWS;
			break;
		case PVS_GUEST_TYPE_LINUX:
			vmType = VCMMD_VE_VM_LINUX;
			break;
	}

	int r = vcmmd_register_ve(qPrintable(m_uuid), vmType, &vcmmdConfig, 0);

	if (VCMMD_ERROR_VE_NAME_ALREADY_IN_USE != r) {
		vcmmd_ve_config_deinit(&vcmmdConfig);
		return Status(r).log("vcmmd_register_ve").treat();
	}

	r = vcmmd_unregister_ve(qPrintable(m_uuid));
	if (r) {
		vcmmd_ve_config_deinit(&vcmmdConfig);
		return Status(r).log("vcmmd_unregister_ve").treat();
	}

	r = vcmmd_register_ve(qPrintable(m_uuid), vmType, &vcmmdConfig, 0);
	vcmmd_ve_config_deinit(&vcmmdConfig);
	return Status(r).log("vcmmd_register_ve").treat();
}

PRL_RESULT Api::update(const Config::Vm::Model& patch_)
{
	vcmmd_ve_config c;
	Config::Vm::Marshal(c).save(patch_);
	int r = vcmmd_update_ve(qPrintable(m_uuid), &c, 0);
	vcmmd_ve_config_deinit(&c);
	return Status(r).log("vcmmd_update_ve").treat();
}

Prl::Expected<Config::Vm::Model, PRL_RESULT> Api::getConfig() const
{
	struct vcmmd_ve_config config;
	PRL_RESULT e = Status(vcmmd_get_ve_config(qPrintable(m_uuid), &config))
				.log("vcmmd_get_ve_config").treat();
	if (PRL_FAILED(e))
		return e;

	Config::Vm::Model output;
	Config::Vm::Marshal(config).load(output);
	vcmmd_ve_config_deinit(&config);
	return output;
}

void Api::deinit()
{
	int r = vcmmd_unregister_ve(qPrintable(m_uuid));
	Status(r).log("vcmmd_unregister_ve", r == VCMMD_ERROR_VE_NOT_REGISTERED ? DBG_WARNING : DBG_FATAL);
}

void Api::activate()
{
	int r = vcmmd_activate_ve(qPrintable(m_uuid), 0);
	Status(r).log("vcmmd_activate_ve", r == VCMMD_ERROR_VE_ALREADY_ACTIVE ? DBG_WARNING : DBG_FATAL);
}

void Api::deactivate()
{
	int r = vcmmd_deactivate_ve(qPrintable(m_uuid));
	Status(r).log("vcmmd_deactivate_ve", r == VCMMD_ERROR_VE_NOT_ACTIVE ? DBG_WARNING : DBG_FATAL);
}

///////////////////////////////////////////////////////////////////////////////
// struct Frontend

template <>
void Frontend<Unregistered>::commit()
{
	m_api.reset();
}

namespace Config
{
///////////////////////////////////////////////////////////////////////////////
// struct DAO

PRL_RESULT DAO::getPersistent(CVcmmdConfig& config_) const
{
	int ret;
	char name[256];

	if ((ret = vcmmd_get_policy_from_file(name, sizeof(name))))
		return Status(ret).log("vcmmd_get_policy_from_file").treat();

	config_.setPolicy(QString(name));
	return PRL_ERR_SUCCESS;
}

PRL_RESULT DAO::getRuntime(CVcmmdConfig& config_) const
{
	int ret;
	char name[256];

	if ((ret = vcmmd_get_current_policy(name, sizeof(name))))
		return Status(ret).log("vcmmd_get_current_policy").treat();

	config_.setPolicy(QString(name));
	return PRL_ERR_SUCCESS;
}

PRL_RESULT DAO::set(const CVcmmdConfig& config_) const
{
	return Status(vcmmd_set_policy(QSTR2UTF8(config_.getPolicy()))).log("vcmmd_set_policy").treat();
}

namespace Vm
{
///////////////////////////////////////////////////////////////////////////////
// struct Marshal
 
void Marshal::load(Model& dst_)
{
	uint64_t v = 0;
	if (vcmmd_ve_config_extract(m_dataStore, VCMMD_VE_CONFIG_LIMIT, &v))
		dst_.setRam(v >> 20);
	if (vcmmd_ve_config_extract(m_dataStore, VCMMD_VE_CONFIG_GUARANTEE, &v) &&
		dst_.getRam().get() && dst_.getRam().get() > 0)
	{
		uint64_t t = 0;
		quint32 g = ((v >> 20) * 100 + dst_.getRam().get() - 1) / dst_.getRam().get();
		vcmmd_ve_config_extract(m_dataStore, VCMMD_VE_CONFIG_GUARANTEE_TYPE, &t);
		switch (t)
		{
		case VCMMD_MEMGUARANTEE_AUTO:
			dst_.setGuarantee(Model::guarantee_type(g,
				PRL_MEMGUARANTEE_AUTO));
			break;
		case VCMMD_MEMGUARANTEE_PERCENTS:
			dst_.setGuarantee(Model::guarantee_type(g,
				PRL_MEMGUARANTEE_PERCENTS));
		}
	}
	if (vcmmd_ve_config_extract(m_dataStore, VCMMD_VE_CONFIG_VRAM, &v))
		dst_.setVideoRam(v >> 20);

	const char* s = NULL;
	if (vcmmd_ve_config_extract_string(m_dataStore, VCMMD_VE_CONFIG_CPU_LIST, &s))
		dst_.setCpuMask(s);
	if (vcmmd_ve_config_extract_string(m_dataStore, VCMMD_VE_CONFIG_NODE_LIST, &s))
		dst_.setNodeMask(s);
}

void Marshal::save(const Model& ve_)
{
	vcmmd_ve_config_init(m_dataStore);
	if (ve_.getRam())
	{
		vcmmd_ve_config_append(m_dataStore, VCMMD_VE_CONFIG_LIMIT,
			ve_.getRam().get() << 20);
		if (ve_.getGuarantee())
		{
			vcmmd_ve_config_append(m_dataStore, VCMMD_VE_CONFIG_GUARANTEE,
				ve_.getGuarantee().get()
					(ve_.getRam().get()) << 20);
			switch (ve_.getGuarantee().get().getType())
			{
			case PRL_MEMGUARANTEE_AUTO:
				vcmmd_ve_config_append(m_dataStore,
					VCMMD_VE_CONFIG_GUARANTEE_TYPE,
					VCMMD_MEMGUARANTEE_AUTO);
				break;
			default:
				vcmmd_ve_config_append(m_dataStore,
					VCMMD_VE_CONFIG_GUARANTEE_TYPE,
					VCMMD_MEMGUARANTEE_PERCENTS);
			}
		}
	}
	if (ve_.getVideoRam())
	{
		vcmmd_ve_config_append(m_dataStore, VCMMD_VE_CONFIG_VRAM,
			ve_.getVideoRam().get() << 20);
	}
	if (!ve_.getCpuMask().isEmpty())
	{
		vcmmd_ve_config_append_string(m_dataStore,
			VCMMD_VE_CONFIG_CPU_LIST, QSTR2UTF8(ve_.getCpuMask()));
	}
	if (!ve_.getNodeMask().isEmpty())
	{
		vcmmd_ve_config_append_string(m_dataStore, VCMMD_VE_CONFIG_NODE_LIST,
			QSTR2UTF8(ve_.getNodeMask()));
	}
}

} // namespace Vm
} // namespace Config
} // namespace Vcmmd

