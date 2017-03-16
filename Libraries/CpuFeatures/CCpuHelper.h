/*
 * Copyright (c) 2015-2017, Parallels International GmbH
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

#ifndef __CCPUHELPER_H__
#define __CCPUHELPER_H__

#include <prlxmlmodel/DispConfig/CDispCpuPreferences.h>
#include <prlxmlmodel/HostHardwareInfo/CHwCpu.h>
#include <prlxmlmodel/VmConfig/CVmConfiguration.h>
#include <prlxmlmodel/CpuFeatures/CCpuPool.h>
#include <prlcommon/Std/SmartPtr.h>

class CCpuHelper {
public:
	static CDispCpuPreferences *get_cpu_mask();
	static PRL_RESULT maskUpdate(CDispCpuPreferences new_mask);
	static bool update(CVmConfiguration &conf);
	static void update(CVmConfiguration &conf, CDispCpuPreferences mask);
	static bool isMasksEqual(CDispCpuPreferences mask1, CDispCpuPreferences mask2);
	static void fill_cpu_info(CHwCpu &cpu);
	static CCpuPoolInfo *getPoolInfo();
	static bool loadPoolsList(QList<CCpuPool> &list);
	static PRL_RESULT joinPool();
	static PRL_RESULT leavePool();
	static PRL_RESULT moveToPool(const char *name);
	static PRL_RESULT recalcPool(const char *name);
	static QSet<QString> getDisabledFeatures(const CVmConfiguration &conf);

	static bool sync();

private:
	static PRL_RESULT execFeaturesCmd(const QString &cmdline);
};

#endif// __CCPUHELPER_H__
