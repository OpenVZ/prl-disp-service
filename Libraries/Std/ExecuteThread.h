//////////////////////////////////////////////////////////////////////////
///
/// The class used to create thread and call back exec() function from it.
/// Used in classes, which need to get data asyncronously by events or
/// completions.
///
/// Copyright (c) 2012-2015 Parallels IP Holdings GmbH
///
/// This file is part of Virtuozzo Core Libraries. Virtuozzo Core
/// Libraries is free software; you can redistribute it and/or modify it
/// under the terms of the GNU Lesser General Public License as published
/// by the Free Software Foundation; either version 2.1 of the License, or
/// (at your option) any later version.
///
/// This library is distributed in the hope that it will be useful,
/// but WITHOUT ANY WARRANTY; without even the implied warranty of
/// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
/// Lesser General Public License for more details.
///
/// You should have received a copy of the GNU Lesser General Public
/// License along with this library.  If not, see
/// <http://www.gnu.org/licenses/> or write to Free Software Foundation,
/// 51 Franklin Street, Fifth Floor Boston, MA 02110, USA.
///
/// Our contact details: Parallels IP Holdings GmbH, Vordergasse 59, 8200
/// Schaffhausen, Switzerland.
///
/////////////////////////////////////////////////////////////////////////
#pragma once

#include <QThread>

class ExecuteThread : public QThread {
public:
	typedef void (* ExecutionFunc)(void *);
public:
	ExecuteThread() : m_function(NULL) { };
	~ExecuteThread() { };
	void setExec(ExecutionFunc f, void *p)
	{
		m_function = f;
		m_parameter = p;
	};
private:
	virtual void run()
	{
		if (!m_function)
			return;

		m_function(m_parameter);
	};
private:
	void *m_parameter;
	ExecutionFunc m_function;
};
