////////////////////////////////////////////////////////////////////////////////
///
/// Copyright (c) 2005-2017, Parallels International GmbH
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
/// Our contact details: Parallels International GmbH, Vordergasse 59, 8200
/// Schaffhausen, Switzerland.
///
/// @file
///	CDspVzLicense.h
///
/// @brief
///	Interface of the class CDspVzLicense
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

#ifndef DspVzLicense_H
#define DspVzLicense_H

#include <QString>
#include <QPair>
#include <QList>
#include <utility>
#include <map>
#include <boost/tuple/tuple.hpp>

#include <prlcommon/Std/SmartPtr.h>
#include <prlcommon/Messaging/CVmEvent.h>
#include <prlcommon/PrlCommonUtilsBase/SysError.h>
#include <prlcommon/PrlCommonUtilsBase/ErrorSimple.h>

class CDspVzLicense
{
	typedef boost::tuple<std::string, std::string, PVE::ParamFieldDataType>
		nameEventType_t;
public:
	CDspVzLicense() : m_status(PRL_ERR_LICENSE_NOT_VALID) {}

public:
	void load();
	Prl::Expected<void, Error::Simple> update();
	Prl::Expected<void, Error::Simple> install(const QString& key);
	bool isValid() const;
	PRL_RESULT getStatus() const;
	SmartPtr<CVmEvent> getVmEvent() const;

private:
	int  parseStatus(const std::string& s) const;
	void parseNameValue(const std::string& name, const std::string& value);
	Prl::Expected<void, Error::Simple> runProgram(const QString& cmdline);

	PRL_RESULT m_status;
	std::map<const nameEventType_t*, std::string> m_values;
};

#endif //H_CDspVzLicense_HHH
