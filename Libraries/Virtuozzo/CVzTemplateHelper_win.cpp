/*
 *  Copyright (c) 2015 Parallels IP Holdings GmbH
 *
 * This file is part of Virtuozzo Core Libraries. Virtuozzo Core
 * Libraries is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
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

#include <QString>
#include <Windows.h>
#include <prlsdk/PrlOses.h>
#include <prlcommon/Logging/Logging.h>
#include "CVzHelper.h"
#include "CVzTemplateHelper.h"

CVzTemplateHelper::CVzTemplateHelper()
{
	init_lib();
}

CVzTemplateHelper::~CVzTemplateHelper()
{
}

int CVzTemplateHelper::init_lib()
{
	return 0;
}

PRL_UINT32 CVzTemplateHelper::convert_os_ver(const char *osname, const char *osver)
{
	QString qs_osname(osname);
	QString qs_osver(osver);
	return (PRL_UINT32) CVzHelper::convert_os_ver(qs_osname, qs_osver);
}

int CVzTemplateHelper::get_templates(QList<SmartPtr<CtTemplate> > &lstVzTmpl)
{
	return CVzHelper::get_templates(lstVzTmpl);
}

PRL_RESULT CVzTemplateHelper::remove_template(QString sName, QString sOsTmplName)
{
	return CVzHelper::remove_template(sName, sOsTmplName);
}


PRL_RESULT CVzTemplateHelper::is_ostemplate_exists(const QString &sOsTemplate)
{
	return CVzHelper::is_ostemplate_exists(sOsTemplate);
}

PRL_RESULT CVzTemplateHelper::remove_templates_env(QString env_uuid, QStringList &lstVzTmpl)
{
	return CVzHelper::remove_templates_env(env_uuid, lstVzTmpl);
}

PRL_RESULT CVzTemplateHelper::install_templates_env(QString env_uuid, QStringList &lstVzTmpl)
{
	return CVzHelper::install_templates_env(env_uuid, lstVzTmpl);
}

PRL_RESULT CVzTemplateHelper::repair_env_private(const QString &sPrivate, const QString &sConfig)
{
	Q_UNUSED(sPrivate);
	Q_UNUSED(sConfig);
	PRL_RESULT ret = PRL_ERR_SUCCESS;
	return ret;
}
