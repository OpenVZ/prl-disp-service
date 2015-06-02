/*
 * Copyright (c) 2015 Parallels IP Holdings GmbH
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

#ifndef __CCPUHELPER_H__
#define __CCPUHELPER_H__

#include "XmlModel/DispConfig/CDispCpuPreferences.h"
#include "XmlModel/HostHardwareInfo/CHwCpu.h"
#include "XmlModel/VmConfig/CVmConfiguration.h"
#include "XmlModel/CpuFeatures/CCpuPool.h"
#include "Libraries/Std/SmartPtr.h"

class CCpuHelper {
public:
	static CDispCpuPreferences *get_cpu_mask();
	static PRL_RESULT maskUpdate(CDispCpuPreferences old_mask, CDispCpuPreferences new_mask);
	static bool update(CVmConfiguration &conf);
	static void fill_cpu_info(CHwCpu &cpu);
	static CCpuPoolInfo *getPoolInfo();
	static bool loadPoolsList(QList<CCpuPool> &list);
	static PRL_RESULT moveToPool(const char *name);
	static PRL_RESULT recalcPool(const char *name);

	static bool sync();

private:
	static PRL_RESULT execFeaturesCmd(const QString &cmdline);
};

#endif// __CCPUHELPER_H__
