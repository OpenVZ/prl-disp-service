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

#ifndef __CDSPTASKTRACE_H__
#define __CDSPTASKTRACE_H__

//#include <QString>
#include <prlcommon/Std/SmartPtr.h>
#include <boost/property_tree/ptree.hpp>
#include <prlcommon/IOService/IOCommunication/IOProtocol.h>

namespace Task
{
///////////////////////////////////////////////////////////////////////////////
// struct Trace

struct Trace
{
	explicit Trace(const SmartPtr<IOService::IOPackage>& request_);

	void start() const;
	void finish(PRL_RESULT code_) const;
	void report(const boost::property_tree::ptree& progress_) const;

	static void raze();
	static void setup();

private:
	QString m_type, m_uuid, m_vmUuid;
};

} // namespace Task

#endif // __CDSPTASKTRACE_H__

