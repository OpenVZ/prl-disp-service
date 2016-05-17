////////////////////////////////////////////////////////////////////////////////
///
/// Copyright (c) 2005-2016 Parallels IP Holdings GmbH
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
/// @file
///	CDspVzLicense.cpp
///
/// @brief
///	Implementation of the class CDspVzLicense
///
/// @author yur
///	yur@virtuozzo.com
///
/// @date
///	2016-05-12
///
/// @history
///
////////////////////////////////////////////////////////////////////////////////

#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>
#include <fstream>
#include <string>
#include <boost/algorithm/string.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/assign.hpp>
#include <boost/foreach.hpp>

#include "CDspVzLicense.h"

#include <prlxmlmodel/Messaging/CVmEventParameter.h>
#include <prlcommon/Logging/Logging.h>
#include <prlcommon/Std/PrlAssert.h>
#include <prlcommon/Std/PrlTime.h>
#include <prlcommon/HostUtils/HostUtils.h>

namespace {
const std::string unlimValueStr = "65535";
}

int CDspVzLicense::parseStatus(const std::string& s) const
{
	static std::map<std::string, int> q = boost::assign::map_list_of
		("UNKNOWN", PRL_ERR_LICENSE_NOT_VALID)
		("INVALID", PRL_ERR_LICENSE_NOT_VALID)
		("EXPIRED", PRL_ERR_LICENSE_EXPIRED)
		("ERROR", PRL_ERR_LICENSE_NOT_VALID)
		("INACTIVE", PRL_ERR_LICENSE_GRACED)
		("VALID", PRL_ERR_LICENSE_VALID)
		("ACTIVE", PRL_ERR_LICENSE_VALID)
		("GRACED", PRL_ERR_LICENSE_GRACED);

	std::map<std::string, int>::const_iterator it = q.find(s);

	if (it == q.end())
		return PRL_ERR_LICENSE_NOT_VALID;
	else
		return it->second;
}

void CDspVzLicense::parseNameValue(const std::string& name, const std::string& value)
{
	static std::vector<nameEventType_t> q = boost::assign::tuple_list_of
		("nr_vms", EVT_PARAM_PRL_VZLICENSE_VMS_TOTAL, PVE::UnsignedInt)
		("cpu_total", EVT_PARAM_PRL_VZLICENSE_CPU_TOTAL, PVE::UnsignedInt)
		("graceperiod", EVT_PARAM_PRL_VZLICENSE_GRACE_PERIOD, PVE::UnsignedInt);

	static std::vector<nameEventType_t> p = boost::assign::tuple_list_of
		("owner_name", EVT_PARAM_PRL_LICENSE_COMPANY, PVE::String)
		("expiration", EVT_PARAM_PRL_VZLICENSE_EXPIRATION_DATE, PVE::String)
		("start_date", EVT_PARAM_PRL_VZLICENSE_START_DATE, PVE::String)
		("update_date", EVT_PARAM_PRL_VZLICENSE_UPDATE_DATE, PVE::String)
		("product", EVT_PARAM_PRL_VZLICENSE_PRODUCT, PVE::String)
		("key_number", EVT_PARAM_PRL_VZLICENSE_KEY_NUMBER, PVE::String)
		("version", EVT_PARAM_PRL_VZLICENSE_VERSION, PVE::String)
		("platform", EVT_PARAM_PRL_VZLICENSE_PLATFORM, PVE::String)
		("hwid", EVT_PARAM_PRL_VZLICENSE_HWID, PVE::String)
		("serial", EVT_PARAM_PRL_VZLICENSE_ORIGINAL_LICENSE_KEY, PVE::String)
		("serial", EVT_PARAM_PRL_VZLICENSE_SERIAL_NUMBER, PVE::String)
		("serial", EVT_PARAM_PRL_LICENSE_KEY, PVE::String);

	BOOST_FOREACH(const nameEventType_t& t, p) {
		if (boost::get<0>(t) == name)
			m_values.insert(std::make_pair(&t, value));
	}

	BOOST_FOREACH(const nameEventType_t& t, q) {
		if (boost::get<0>(t) != name)
			continue;

		std::string val = value;

		if (value == "unlimited") {
			val = unlimValueStr;
		} else if (value == "combined") {
			continue;
		} else {
			std::vector<std::string> v;

			boost::split(v, value, boost::is_any_of(" "));
			val = v[0];
		}

		m_values.insert(std::make_pair(&t, val));
	}
}

void CDspVzLicense::load()
{
	const std::string delims = "=\"\n";

	FILE *fd = popen("vzlicview --class VZSRV", "r");
	if (!fd)
		return;

	char buf[128];
	while(fgets(buf, sizeof(buf), fd) != NULL) {
		std::string s = buf;
		boost::trim(s);

		std::vector<std::string> v;
		boost::split(v, s, boost::is_any_of(delims), boost::token_compress_on);

		if (v.size() < 2)
			continue;

		std::string name = v[0];
		std::string val = v[1];

		parseNameValue(name, val);

		if (name == "status")
			m_status = parseStatus(val);
	}

	pclose(fd);
}

bool CDspVzLicense::isValid() const
{
	return PRL_LICENSE_IS_VALID(getStatus());
}

PRL_RESULT CDspVzLicense::getStatus() const
{
	return m_status;
}

SmartPtr<CVmEvent> CDspVzLicense::getVmEvent() const
{
	SmartPtr<CVmEvent> evt(new CVmEvent());

	evt->addEventParameter(new CVmEventParameter(
				PVE::Boolean,
				QString::number(isValid()),
				EVT_PARAM_PRL_LICENSE_IS_VALID));
	evt->addEventParameter(new CVmEventParameter(
				PVE::UnsignedInt,
				QString::number((unsigned int) getStatus()),
				EVT_PARAM_PRL_LICENSE_STATUS));

	typedef std::pair<const nameEventType_t*,std::string> val_t;
	BOOST_FOREACH(const val_t& it, m_values) {
		evt->addEventParameter(new CVmEventParameter(
				boost::get<2>(*it.first),
				QString::fromUtf8(it.second.c_str()),
				boost::get<1>(*it.first).c_str()));
	}

	return evt;
}

Prl::Expected<void, Error::Simple> CDspVzLicense::runProgram(const QString& cmdline)
{
	QProcess proc;

	DefaultExecHandler handler(proc, cmdline);
	if (!HostUtils::RunCmdLineUtilityEx(cmdline, proc, -1)(handler).isSuccess()) {
		return Error::Simple(PRL_ERR_OPERATION_FAILED, handler.getStderr());
	}

	return Prl::Expected<void, Error::Simple>();
}

Prl::Expected<void, Error::Simple> CDspVzLicense::update()
{
	return runProgram("vzlicupdate");
}

Prl::Expected<void, Error::Simple> CDspVzLicense::install(const QString& key)
{
	QString x = "vzlicload -p ";
	x += key;
	return runProgram(x);
}
