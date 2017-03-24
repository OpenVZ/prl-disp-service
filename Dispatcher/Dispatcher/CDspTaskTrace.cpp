///////////////////////////////////////////////////////////////////////////////
///
/// @file CDspTaskTrace.h
///
/// Task trace producer.
///
/// @author shrike
///
/// Copyright (c) 2005-2017 Parallels IP Holdings GmbH
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

#include <syslog.h>
#include "CDspTaskTrace.h"
#include <boost/property_tree/json_parser.hpp>
#include <prlcommon/ProtoSerializer/CProtoSerializer.h>
#include <prlcommon/PrlCommonUtilsBase/PrlStringifyConsts.h>

namespace Task
{
///////////////////////////////////////////////////////////////////////////////
// struct Trace

Trace::Trace(const SmartPtr<IOService::IOPackage>& request_)
{
	if (!request_.isValid())
		return;

	m_uuid = Uuid::toString(request_->header.uuid);
	Parallels::CProtoCommandPtr d = Parallels::CProtoSerializer::ParseCommand(request_);
	if (!d.isValid())
		return;

	m_type = PVE::DispatcherCommandToString(d->GetCommandId());
	m_vmUuid = d->GetVmUuid();
}

void Trace::start() const
{
	boost::property_tree::ptree t;
	t.put("start", m_uuid.toStdString());
	report(t);
}

void Trace::finish(PRL_RESULT code_) const
{
	boost::property_tree::ptree t;
	t.put("result", PRL_RESULT_TO_STRING(code_));
	report(t);
}

void Trace::report(const boost::property_tree::ptree& progress_) const
{
	boost::property_tree::ptree t;
	t.put("type", m_type.toStdString());
	t.put("uuid", m_uuid.toStdString());
	if (!m_vmUuid.isEmpty())
		t.put("vmUuid", m_vmUuid.toStdString());

	t.put_child("progress", progress_);

	std::stringstream s;
	boost::property_tree::json_parser::write_json(s, t, false);
	syslog(LOG_INFO, s.str().c_str());
}

void Trace::raze()
{
	closelog();
}

void Trace::setup()
{
	openlog("vz-dispatcher", LOG_PID, LOG_INFO | LOG_USER);
}

} // namespace Task

