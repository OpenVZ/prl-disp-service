/*
 * SysError.h: error reference crossplatform functions
 *
 * Copyright (C) 1999-2014 Parallels IP Holdings GmbH
 *
 * This file is part of Parallels SDK. Parallels SDK is free
 * software; you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License,
 * or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <http://www.gnu.org/licenses/> or write to Free Software Foundation,
 * 51 Franklin Street, Fifth Floor Boston, MA 02110, USA.
 *
 * Our contact details: Parallels IP Holdings GmbH, Vordergasse 59, 8200
 * Schaffhausen, Switzerland.
 */

#ifndef __ERROR_SIMPLE_H__
#define __ERROR_SIMPLE_H__

namespace Error
{

struct Simple
{
	Simple(PRL_RESULT result_, const QString& str_ = QString())
		: m_data(std::make_pair(result_, str_))
	{
	}

	CVmEvent convertToEvent(const QString &paramName_ =
					EVT_PARAM_DETAIL_DESCRIPTION) const
	{
		CVmEvent e;

		e.setEventCode(m_data.first);
		e.addEventParameter(new CVmEventParameter(PVE::String,
					m_data.second, paramName_));

		return e;
	}

	PRL_RESULT code() const
	{
		return m_data.first;
	}

protected:
	QString& details()
	{
		return m_data.second;
	}

private:
	std::pair<PRL_RESULT, QString> m_data;
};

} // namespace Error

#endif // __ERROR_SIMPLE_H__
