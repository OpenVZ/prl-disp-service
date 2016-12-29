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

#include <QFile>
#include <QTemporaryFile>
#include <boost/property_tree/json_parser.hpp>
#include <prlsdk/PrlOses.h>
#include <prlcommon/Logging/Logging.h>
#include <prlcommon/PrlUuid/Uuid.h>
#include "Libraries/PrlCommonUtils/CFileHelper.h"
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

vcmmd_ve_config* Api::init(quint64 limit_, const guarantee_type& guarantee_, vcmmd_ve_config& value_)
{
	vcmmd_ve_config_init(&value_);
	vcmmd_ve_config_append(&value_, VCMMD_VE_CONFIG_LIMIT, limit_);
	vcmmd_ve_config_append(&value_, VCMMD_VE_CONFIG_GUARANTEE,
		guarantee_(limit_ >> 20) << 20);
	switch (guarantee_.getType())
	{
	case PRL_MEMGUARANTEE_AUTO:
		vcmmd_ve_config_append(&value_, VCMMD_VE_CONFIG_GUARANTEE_TYPE, VCMMD_MEMGUARANTEE_AUTO);
		break;
	default:
		vcmmd_ve_config_append(&value_, VCMMD_VE_CONFIG_GUARANTEE_TYPE, VCMMD_MEMGUARANTEE_PERCENTS);
	}

	return &value_;
}

PRL_RESULT Api::init(const SmartPtr<CVmConfiguration>& config_)
{
	if (!config_.isValid())
		return PRL_ERR_INVALID_PARAM;

	CVmMemory* memory = config_->getVmHardwareList()->getMemory();
	quint64 vram = config_->
		getVmHardwareList()->getVideo()->getMemorySize();

	struct vcmmd_ve_config vcmmdConfig;
	vcmmd_ve_config_append(init(quint64(memory->getRamSize()) << 20,
		guarantee_type(*memory), vcmmdConfig),
		VCMMD_VE_CONFIG_VRAM, vram << 20);

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
		return treat(r, "vcmmd_register_ve");
	}

	r = vcmmd_unregister_ve(qPrintable(m_uuid));
	if (r) {
		vcmmd_ve_config_deinit(&vcmmdConfig);
		return treat(r, "vcmmd_unregister_ve");
	}

	r = vcmmd_register_ve(qPrintable(m_uuid), vmType, &vcmmdConfig, 0);
	vcmmd_ve_config_deinit(&vcmmdConfig);
	return treat(r, "vcmmd_register_ve");
}

PRL_RESULT Api::update(quint64 limit_, const guarantee_type& guarantee_)
{
	vcmmd_ve_config config;
	int r = vcmmd_update_ve(qPrintable(m_uuid), init(limit_, guarantee_, config), 0);
	vcmmd_ve_config_deinit(&config);
	return treat(r, "vcmmd_update_ve");
}

Prl::Expected<std::pair<quint64, quint64>, PRL_RESULT> Api::getConfig() const
{
	struct vcmmd_ve_config config;
	PRL_RESULT e = treat(vcmmd_get_ve_config(qPrintable(m_uuid), &config),
		"vcmmd_get_ve_config");
	if (PRL_FAILED(e))
		return e;

	std::pair<quint64, quint64> output;
	vcmmd_ve_config_extract(&config, VCMMD_VE_CONFIG_LIMIT, (uint64_t*)&output.first);
	vcmmd_ve_config_extract(&config, VCMMD_VE_CONFIG_GUARANTEE, (uint64_t*)&output.second);

	vcmmd_ve_config_deinit(&config);
	return output;
}

void Api::deinit()
{
	int r = vcmmd_unregister_ve(qPrintable(m_uuid));
	treat(r, "vcmmd_unregister_ve", r == VCMMD_ERROR_VE_NOT_REGISTERED ? DBG_WARNING : DBG_FATAL);
}

void Api::activate()
{
	int r = vcmmd_activate_ve(qPrintable(m_uuid), 0);
	treat(r, "vcmmd_activate_ve", r == VCMMD_ERROR_VE_ALREADY_ACTIVE ? DBG_WARNING : DBG_FATAL);
}

void Api::deactivate()
{
	int r = vcmmd_deactivate_ve(qPrintable(m_uuid));
	treat(r, "vcmmd_deactivate_ve", r == VCMMD_ERROR_VE_NOT_ACTIVE ? DBG_WARNING : DBG_FATAL);
}

PRL_RESULT Api::treat(int status_, const char* name_, int level_)
{
	if (0 == status_)
		return PRL_ERR_SUCCESS;

	char e[255] = {};
	WRITE_TRACE(level_, " failed. %s: %s", name_,
		vcmmd_strerror(status_, e, sizeof(e)));

	switch (status_)
	{
	case VCMMD_ERROR_CONNECTION_FAILED:
		return PRL_ERR_VCMMD_NO_CONNECTION;
	case VCMMD_ERROR_UNABLE_APPLY_VE_GUARANTEE:
		return PRL_ERR_UNABLE_APPLY_MEMORY_GUARANTEE;
	default:
		return PRL_ERR_FAILURE;
	}
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
// struct File

const QString File::s_configPath("/etc/vz/vcmmd.conf");

boost::property_tree::ptree File::read()
{
	boost::property_tree::ptree result;
	std::ifstream is(s_configPath.toStdString().c_str());
	if (!is.is_open())
		return result;

	try
	{
		boost::property_tree::json_parser::read_json(is, result);
	}
	catch (const boost::property_tree::json_parser::json_parser_error&)
	{
		WRITE_TRACE(DBG_FATAL, "Failed to parse %s", QSTR2UTF8(s_configPath));
	}
	is.close();

	return result;
}

PRL_RESULT File::write(const boost::property_tree::ptree& t_)
{
	QTemporaryFile f(s_configPath + "_XXX");
	if (!f.open())
		return PRL_ERR_FAILURE;

	std::stringstream ss;
	boost::property_tree::json_parser::write_json(ss, t_, true);

	QByteArray a;
	a.append(QString::fromStdString(ss.str()));
	f.write(a);
	f.close();

	if (!CFileHelper::AtomicMoveFile(f.fileName(), s_configPath))
	{
		WRITE_TRACE(DBG_FATAL, "Failed to rename %s to %s", QSTR2UTF8(f.fileName()), QSTR2UTF8(s_configPath));
		return PRL_ERR_FAILURE;
	}
	f.setAutoRemove(false);

	return PRL_ERR_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
// struct DAO

PRL_RESULT DAO::getPersistent(CVcmmdConfig& config_) const
{
	boost::property_tree::ptree t = File().read();
	config_.setPolicy(QString::fromStdString(
		t.get<std::string>("LoadManager.Policy")));
	return PRL_ERR_SUCCESS;
}

PRL_RESULT DAO::getRuntime(CVcmmdConfig& config_) const
{
	int ret;
	char name[256];

	if((ret = vcmmd_get_current_policy(name, sizeof(name))))
	{
		char e[256];
		WRITE_TRACE(DBG_FATAL, "vcmmd_get_current_policy error: %s",
				vcmmd_strerror(ret, e, sizeof(e)));
		return PRL_ERR_FAILURE;
	}

	config_.setPolicy(QString(name));
	return PRL_ERR_SUCCESS;
}

PRL_RESULT DAO::set(const CVcmmdConfig& config_) const
{
	File f;
	boost::property_tree::ptree t = f.read();
	t.put<std::string>("LoadManager.Policy",
				config_.getPolicy().toStdString());
	return f.write(t);
}

} //namespace Config

} // namespace Vcmmd

