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
#ifndef __CVZTEMPLATEHELPER_H__
#define __CVZTEMPLATEHELPER_H__

#include <prlsdk/PrlTypes.h>
#include "Libraries/Std/SmartPtr.h"
#include "XmlModel/CtTemplate/CtTemplate.h"

class CVzTemplateHelper {

public:
	CVzTemplateHelper();
	~CVzTemplateHelper();

	static int init_lib();

	/* maps VZTT_ERR_* codes to PRL_ERR_* ones */
	static PRL_RESULT err_vztt2prl(int retcode);

	/* Retrieves list of templates from the node */
	int get_templates(QList<SmartPtr<CtTemplate> > &lstVzTmpl);

	/* Removes template from the node */
	PRL_RESULT remove_template(QString sName, QString sOsTmplName);

	/* Installs list of app templates to the Container */
	PRL_RESULT install_templates_env(QString env_uuid, QStringList &lstVzTmpl);

	/* Removes list of app templates to the Container */
	PRL_RESULT remove_templates_env(QString env_uuid, QStringList &lstVzTmpl);

	/* Repair private area of the Container */
	PRL_RESULT repair_env_private(const QString &sPrivate, const QString &sConfig);

	/* Is OS template exist on node? */
	PRL_RESULT is_ostemplate_exists(const QString &sOsTemplate);
private:
	/* maps vz distros naming to prl types */
	static PRL_UINT32 convert_os_ver(const char *osname, const char *osver);

};

#endif /*__CVZTEMPLATEHELPER_H__ */
