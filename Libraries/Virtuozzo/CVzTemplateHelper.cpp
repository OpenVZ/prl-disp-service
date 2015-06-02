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
#include <dlfcn.h>

#include <QString>
#include <prlsdk/PrlOses.h>
#include "Libraries/Logging/Logging.h"
#include "Libraries/PrlUuid/libuuid_unix/uuid.h"
#include "CVzHelper.h"
#include "CVzTemplateHelper.h"
#include "PrlLibvzttWrap.h"

#define VZTTLIB	"libvztt.so.1"

static void *s_vzttlib_handle = NULL;

//  allocate library functions ptr
#define LOAD_SYM(name) \
	static wrap##name name;

LOAD_ALL_SYM()

static void clean_all_base(char** _all)
{
	for (char** b = _all; *b != NULL; free(*b), ++b) ;
	free(_all);
};

static struct options_vztt *create_options()
{
	struct options_vztt *pOpt;

	pOpt = vztt_options_create();
	if (!pOpt) {
		WRITE_TRACE(DBG_FATAL, "Cannot allocate options_vztt!");
		return NULL;
	}
	vztt_options_set_debug(0, pOpt);
	return pOpt;
}

static void free_options(struct options_vztt *pOpt)
{
	vztt_options_free(pOpt);
}

static char ** create_tmpl_list(QStringList &lst)
{
	char ** pTmpl;
	int i;

	pTmpl = new(std::nothrow) char *[lst.size()];
	if (!pTmpl) {
		WRITE_TRACE(DBG_FATAL, "Cannot allocate templates list!");
		return NULL;
	}
	i = 0;
	foreach(QString str, lst)
		pTmpl[i++] = strdup(str.toUtf8().data());
	return pTmpl;
}

static void free_tmpl_list(char ** pTmpl, int size)
{
	for (int i = 0 ; i < size; i++)
		free(pTmpl[i]);
	delete [] pTmpl;
}

CVzTemplateHelper::CVzTemplateHelper()
{
	init_lib();
}

CVzTemplateHelper::~CVzTemplateHelper()
{
}

int CVzTemplateHelper::init_lib()
{
	WRITE_TRACE(DBG_FATAL, "Initialize libvztt");

	// is it initialized already?
	if (s_vzttlib_handle)
		return 0;

	s_vzttlib_handle = dlopen(VZTTLIB, RTLD_NOW);
	if (s_vzttlib_handle == NULL) {
		WRITE_TRACE(DBG_FATAL, "Failed to load %s: %s",
				VZTTLIB, dlerror());
		return -1;
	}
#if defined LOAD_SYM
	#undef LOAD_SYM
#endif
#define LOAD_SYM(name) \
	do { \
		name = (wrap##name) dlsym(s_vzttlib_handle, #name); \
		if (name == NULL) { \
			WRITE_TRACE(DBG_FATAL, "Failed to load %s: %s", #name, dlerror()); \
			dlclose(s_vzttlib_handle); \
			s_vzttlib_handle = NULL; \
			return -1; \
		} \
	} while (0);

	LOAD_ALL_SYM()

#undef LOAD_SYM

	vztt_init_logger(GetLogFileName(), 0);
	return 0;
}

PRL_UINT32 CVzTemplateHelper::convert_os_ver(const char *osname, const char *osver)
{
	if (!osname || !osver)
		return PVS_GUEST_VER_LIN_OTHER;

	if (!strcmp(osname, "suse"))
		return PVS_GUEST_VER_LIN_OPENSUSE;
	else if (!strcmp(osname, "sles"))
		return PVS_GUEST_VER_LIN_SUSE;
	else if (!strcmp(osname, "centos")) {
		if (!strcmp(osver, "7"))
			return PVS_GUEST_VER_LIN_CENTOS_7;
		else
			return PVS_GUEST_VER_LIN_CENTOS;
	}
	else if (!strcmp(osname, "redhat")) {
		if (!strcmp(osver, "el7"))
			return PVS_GUEST_VER_LIN_REDHAT_7;
		else
			return PVS_GUEST_VER_LIN_REDHAT;
	}
	else if (!strcmp(osname, "fedora-core"))
		return PVS_GUEST_VER_LIN_FEDORA;
	else if (!strcmp(osname, "debian"))
		return PVS_GUEST_VER_LIN_DEBIAN;
	else if (!strcmp(osname, "ubuntu"))
		return PVS_GUEST_VER_LIN_UBUNTU;

	return PVS_GUEST_VER_LIN_OTHER;
}

PRL_RESULT CVzTemplateHelper::err_vztt2prl(int retcode)
{
	switch (retcode) {
	case 0:
		return PRL_ERR_SUCCESS;
	case VZT_CANT_LOCK:
		return PRL_ERR_VM_IS_EXCLUSIVELY_LOCKED;
	case VZT_TMPL_NOT_FOUND:
		return PRL_ERR_TEMPLATE_NOT_FOUND;
	case VZT_FILE_NFOUND:
		return PRL_ERR_FILE_NOT_FOUND;
	case VZT_PM_FAILED:
		return PRL_ERR_OPERATION_FAILED;
	case VZT_TMPL_HAS_APPS:
		return PRL_ERR_TEMPLATE_HAS_APPS;
	default:
		return PRL_ERR_FAILURE;
	}

	return PRL_ERR_FAILURE;
}

PRL_RESULT CVzTemplateHelper::get_templates(QList<SmartPtr<CtTemplate> > &lstVzTmpl)
{
	struct options_vztt *pOpt;
	char** osTmplList = NULL;
	char **os;
	int rc;

	if (s_vzttlib_handle == NULL)
		return PRL_ERR_API_WASNT_INITIALIZED;

	rc = vztt_get_all_base(&osTmplList);
	if (0 != rc) {
		WRITE_TRACE(DBG_FATAL, "Cannot get list of EZ OS templates with %d", rc);
		return -1;
	}

	pOpt = create_options();
	if (!pOpt) {
		clean_all_base(osTmplList);
		return -1;
	}
	vztt_options_set_quiet(1, pOpt);
	vztt_options_set_fld_mask(VZTT_INFO_NAME | VZTT_INFO_ARCH |
			VZTT_INFO_CACHED | VZTT_INFO_SUMMARY, pOpt);

	for (os = osTmplList; *os != NULL; ++os) {
		struct tmpl_list_el** lst = NULL;
		rc = vztt2_get_templates_list(*os, pOpt, &lst);
		if (0 != rc) {
			WRITE_TRACE(DBG_FATAL, "Cannot list application"
				" templates of EZ OS template %s with %d",
				*os, rc);
			clean_all_base(osTmplList);
			free_options(pOpt);
			return -1;
		}

		for (struct tmpl_list_el** p = lst; *p != NULL; ++p) {
			if (!*p || !(*p)->info)
				continue;

			struct tmpl_info *info = (*p)->info;
			SmartPtr<CtTemplate> pTmpl(new CtTemplate);
			pTmpl->setName( QString(info->name) );
			pTmpl->setDescription( QString(info->summary) );
			if ((*p)->is_os)
				pTmpl->setType(PCT_TYPE_EZ_OS);
			else {
				pTmpl->setType(PCT_TYPE_EZ_APP);
				pTmpl->setOsTemplate(QString(*os));
			}
			if (info->osarch && !strcmp(info->osarch, "x86_64"))
				pTmpl->setCpuMode(PCM_CPU_MODE_64);
			pTmpl->setOsType(PVS_GUEST_TYPE_LINUX);
			pTmpl->setOsVersion(convert_os_ver(info->osname,
						info->osver));
			if (info->cached && !strcmp(info->cached, "yes"))
				pTmpl->setCached(true);

			lstVzTmpl.append(pTmpl);
		}
		vztt_clean_templates_list(lst);
	}
	free_options(pOpt);
	clean_all_base(osTmplList);
	return 0;
}

PRL_RESULT CVzTemplateHelper::install_templates_env(QString env_uuid, QStringList &lstVzTmpl)
{
	Q_UNUSED(env_uuid)

	struct options_vztt *pOpt;
	char ** pTmplLst;
	int rc;

	if (lstVzTmpl.empty())
		return PRL_ERR_SUCCESS;

	if (s_vzttlib_handle == NULL)
		return PRL_ERR_API_WASNT_INITIALIZED;

	pOpt = create_options();
	if (!pOpt)
		return PRL_ERR_MEMORY_ALLOC_ERROR;

	pTmplLst = create_tmpl_list(lstVzTmpl);
	if (!pTmplLst) {
		free_options(pOpt);
		return PRL_ERR_MEMORY_ALLOC_ERROR;
	}

	unsigned int ctid = -1;

	rc = vztt2_install_tmpl(ctid, pTmplLst, lstVzTmpl.size(), pOpt, NULL, NULL);
	if (0 != rc)
		WRITE_TRACE(DBG_FATAL, "Cannot install EZ application templates"
					" from CT %d with %d", ctid, rc);
	free_tmpl_list(pTmplLst, lstVzTmpl.size());
	free_options(pOpt);

	return err_vztt2prl(rc);
}

PRL_RESULT CVzTemplateHelper::remove_templates_env(QString env_uuid, QStringList &lstVzTmpl)
{
	Q_UNUSED(env_uuid)

	struct options_vztt *pOpt;
	char ** pTmplLst;
	int rc;

	if (lstVzTmpl.empty())
		return PRL_ERR_SUCCESS;

	if (s_vzttlib_handle == NULL)
		return PRL_ERR_API_WASNT_INITIALIZED;

	pOpt = create_options();
	if (!pOpt)
		return PRL_ERR_MEMORY_ALLOC_ERROR;

	pTmplLst = create_tmpl_list(lstVzTmpl);
	if (!pTmplLst) {
		free_options(pOpt);
		return PRL_ERR_MEMORY_ALLOC_ERROR;
	}

	unsigned int ctid = -1;

	rc = vztt2_remove_tmpl(ctid, pTmplLst, lstVzTmpl.size(), pOpt, NULL);
	if (0 != rc)
		WRITE_TRACE(DBG_FATAL, "Cannot remove EZ application templates"
				" from CT %d with %d", ctid, rc);
	free_tmpl_list(pTmplLst, lstVzTmpl.size());
	free_options(pOpt);

	return err_vztt2prl(rc);
}

PRL_RESULT CVzTemplateHelper::remove_template(QString sName, QString sOsTmplName)
{
	struct options_vztt *pOpt;
	int rc;

	if (sName.isEmpty())
		return PRL_ERR_SUCCESS;

	if (s_vzttlib_handle == NULL)
		return PRL_ERR_API_WASNT_INITIALIZED;

	pOpt = create_options();
	if (!pOpt)
		return PRL_ERR_MEMORY_ALLOC_ERROR;

	if (sOsTmplName.isEmpty()) {
		rc = vztt2_remove_os_template(sName.toUtf8().data(), pOpt);
		if (0 != rc)
			WRITE_TRACE(DBG_FATAL, "Cannot remove EZ OS template"
				" '%s', rc = %d", QSTR2UTF8(sName), rc);
	} else {
		vztt_options_set_for_obj(sOsTmplName.toUtf8().data(), pOpt);
		rc = vztt2_remove_app_template(sName.toUtf8().data(), pOpt);
		if (0 != rc)
			WRITE_TRACE(DBG_FATAL, "Cannot remove EZ application"
				" template '%s' for OS tempate '%s', rc = %d",
				QSTR2UTF8(sName), QSTR2UTF8(sOsTmplName), rc);
	}
	free_options(pOpt);

	return err_vztt2prl(rc);
}

PRL_RESULT CVzTemplateHelper::repair_env_private(const QString &sPrivate, const QString &sConfig)
{
	struct options_vztt *pOpt;
	int rc;

	if (s_vzttlib_handle == NULL)
		return PRL_ERR_API_WASNT_INITIALIZED;

	if (sPrivate.isEmpty() || sConfig.isEmpty())
		return PRL_ERR_INVALID_ARG;

	pOpt = create_options();
	if (!pOpt)
		return PRL_ERR_MEMORY_ALLOC_ERROR;

	rc = vztt2_repair(QSTR2UTF8(sPrivate), QSTR2UTF8(sConfig), pOpt);
	if (0 != rc)
		WRITE_TRACE(DBG_FATAL, "vztt2_repair() failed: private='%s', config='%s', rc=%d",
				QSTR2UTF8(sPrivate), QSTR2UTF8(sConfig), rc);
	free_options(pOpt);

	return err_vztt2prl(rc);
}

PRL_RESULT CVzTemplateHelper::is_ostemplate_exists(const QString &sOsTemplate)
{
	PRL_RESULT ret = PRL_ERR_SUCCESS;
	int rc;
	struct options_vztt *pOpt;
	struct tmpl_info info;

	if (s_vzttlib_handle == NULL)
		return PRL_ERR_API_WASNT_INITIALIZED;

	if (sOsTemplate.isEmpty()) {
		WRITE_TRACE(DBG_FATAL, "OS template name is empty");
		return PRL_ERR_INVALID_ARG;
	}

	pOpt = create_options();
	if (!pOpt)
		return PRL_ERR_MEMORY_ALLOC_ERROR;

	rc = vztt2_get_os_tmpl_info((char *)QSTR2UTF8(sOsTemplate), pOpt, &info);
	if (rc == 0) {
		vztt_clean_tmpl_info(&info);
	} else {
		WRITE_TRACE(DBG_FATAL, "vztt2_get_os_tmpl_info() failed: OS template='%s', rc=%d",
				QSTR2UTF8(sOsTemplate), rc);
		ret = err_vztt2prl(rc);
	}
	free_options(pOpt);

	return ret;
}

