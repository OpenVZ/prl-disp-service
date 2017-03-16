/*
 *  Copyright (c) 2015-2017, Parallels International GmbH
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
 * Our contact details: Parallels International GmbH, Vordergasse 59, 8200
 * Schaffhausen, Switzerland.
 */

#ifndef _PRLLIBVZTTWRAP_H__
#define _PRLLIBVZTTWRAP_H__

#define _USE_DLOPEN_

#include "vz/vztt.h"

#define FUNC_DECL(res, name, args) \
	typedef res (*wrap##name) args;

FUNC_DECL(void, vztt_init_logger, (const char * logfile, int loglevel))
FUNC_DECL(int, vztt2_get_templates_list, (char *ostemplate, struct options_vztt *opts,
	struct tmpl_list_el ***ls))
FUNC_DECL(int, vztt2_install_tmpl,  (const char *ctid, char *tlist[], size_t size,
	struct options_vztt *opts_vztt, struct package ***pkg_updated,
	struct package ***pkg_removed))
FUNC_DECL(int, vztt2_remove_tmpl,  (const char *ctid, char *tlist[], size_t size,
	struct options_vztt *opts_vztt, struct package ***pkg_removed))
FUNC_DECL(int, vztt2_remove_os_template,  (char *tmpl, struct options_vztt *opts_vztt))
FUNC_DECL(int, vztt2_remove_app_template,  (char *tmpl, struct options_vztt *opts_vztt))
FUNC_DECL(int, vztt_get_all_base, (char ***arr))
FUNC_DECL(void, vztt_set_default_options, (struct options *opts))
FUNC_DECL(void, vztt_clean_templates_list, (struct tmpl_list_el **ls))
FUNC_DECL(struct options_vztt *, vztt_options_create, (void))
FUNC_DECL(void, vztt_options_free, (struct options_vztt *opts_vztt))
FUNC_DECL(void, vztt_options_set_quiet, (int enabled, struct options_vztt *opts_vztt))
FUNC_DECL(void, vztt_options_set_debug, (int debug, struct options_vztt *opts_vztt))
FUNC_DECL(void, vztt_options_set_fld_mask, (unsigned long fld_mask, struct options_vztt *opts_vztt))
FUNC_DECL(void, vztt_options_set_for_obj, (char *for_obj, struct options_vztt *opts_vztt))
FUNC_DECL(int, vztt2_repair,  (const char *ve_private, const char *veconf, struct options_vztt *opts_vztt))
FUNC_DECL(int, vztt2_get_os_tmpl_info, (char *ostemplate, struct options_vztt *opts_vztt, struct tmpl_info *info))
FUNC_DECL(void, vztt_clean_tmpl_info, (struct tmpl_info *info))


#define LOAD_ALL_SYM() \
LOAD_SYM(vztt_init_logger) \
LOAD_SYM(vztt2_get_templates_list) \
LOAD_SYM(vztt2_install_tmpl) \
LOAD_SYM(vztt2_remove_tmpl) \
LOAD_SYM(vztt2_remove_os_template) \
LOAD_SYM(vztt2_remove_app_template) \
LOAD_SYM(vztt_get_all_base) \
LOAD_SYM(vztt_set_default_options) \
LOAD_SYM(vztt_clean_templates_list) \
LOAD_SYM(vztt_options_create) \
LOAD_SYM(vztt_options_free) \
LOAD_SYM(vztt_options_set_debug) \
LOAD_SYM(vztt_options_set_quiet) \
LOAD_SYM(vztt_options_set_fld_mask) \
LOAD_SYM(vztt_options_set_for_obj) \
LOAD_SYM(vztt2_repair) \
LOAD_SYM(vztt2_get_os_tmpl_info) \
LOAD_SYM(vztt_clean_tmpl_info) \

#endif


