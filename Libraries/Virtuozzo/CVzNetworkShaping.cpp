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

#ifndef _WIN_
	#include <unistd.h>
#endif

#include <stdlib.h>
#include "CVzNetworkShaping.h"
#include <prlcommon/Logging/Logging.h>

#define NETWORK_SHAPING_CONF	"/etc/vz/vz.conf"
#define NETWORK_CLASSES_CONF	"/etc/vz/conf/networks_classes"

#define PRL_CMD_WORK_TIMEOUT 5 * 1000
static PRL_RESULT run_prg(const char *name, const QStringList &lstArgs)
{
	QProcess proc;
	WRITE_TRACE(DBG_INFO, "%s %s", name, lstArgs.join(" ").toUtf8().constData());
	proc.start(name, lstArgs);
	if (!proc.waitForFinished(PRL_CMD_WORK_TIMEOUT))
	{
		WRITE_TRACE(DBG_FATAL, "%s tool not responding. Terminate it now.", name);
		proc.kill();
		return PRL_ERR_OPERATION_FAILED;
	}

	if (0 != proc.exitCode())
	{
		WRITE_TRACE(DBG_FATAL, "%s utility failed: %s %s [%d]\nout=%s\nerr=%s",
				name, name,
				lstArgs.join(" ").toUtf8().constData(),
				proc.exitCode(),
				proc.readAllStandardOutput().data(),
				proc.readAllStandardError().data());
		return PRL_ERR_OPERATION_FAILED;
	}
	return PRL_ERR_SUCCESS;
}

static QString get_BANDWIDTH_string(const CNetworkShapingConfig &conf)
{
	QString str;
	foreach(CDeviceBandwidth *entry, conf.m_lstDevicesBandwidth) {
		str += QString("%1:%2 ")
				.arg(entry->getDevice())
				.arg(entry->getBandwidth());
	}
	return str;
}

static QString get_TOTALRATE_string(const CNetworkShapingConfig &conf)
{
	QString str;
	foreach(CNetworkShaping *entry, conf.m_lstNetworkShaping) {
		str += QString("%1:%2:%3 ")
				.arg(entry->getDevice().isEmpty() ?
						"*" : entry->getDevice())
				.arg(entry->getClassId())
				.arg(entry->getTotalRate());
	}
	return str;
}

static QString get_RATEMPU_string(const CNetworkShapingConfig &conf)
{
	QString str;
	foreach(CNetworkShaping *entry, conf.m_lstNetworkShaping) {
		PRL_INT32 mpu = entry->getRateMPU();
		switch (mpu) {
		case NRM_DISABLED:
			break;
		case NRM_ENABLED:
			str += QString("%1:%2 ")
				.arg(entry->getDevice().isEmpty() ?
						"*" : entry->getDevice())
				.arg(entry->getClassId());
			break;
		default:
			str += QString("%1:%2:%3 ")
				.arg(entry->getDevice().isEmpty() ?
						"*" : entry->getDevice())
				.arg(entry->getClassId())
				.arg(mpu);
			break;
		}
	}
	return str;
}

static QString get_RATE_string(const CNetworkShapingConfig &conf)
{
	QString str;
	foreach(CNetworkShaping *entry, conf.m_lstNetworkShaping) {
		if (entry->getRate() != 0)
			str += QString("*:%1:%2 ")
				.arg(entry->getClassId())
				.arg(entry->getRate());
	}
	return str;
}

int CVzNetworkShaping::update_network_shaping_config(const CNetworkShapingConfig &conf)
{
	FILE *rfp, *wfp;
	char buf[4096];
	char r_path[512];
	char tmp_path[4096], orig_path[4096];
	bool bandwidth_printed = false;
	bool totalrate_printed = false;
	bool ratempu_printed = false;
	bool rate_printed = false;
	bool enable_printed = false;
	QStringList args;
	PRL_RESULT res;

	if (realpath(NETWORK_SHAPING_CONF, r_path) == NULL)
		snprintf(r_path, sizeof(r_path), "%s", NETWORK_SHAPING_CONF);

	if ((rfp = fopen(r_path, "r")) == NULL) {
		WRITE_TRACE(DBG_FATAL, "failed to open %s: %m", r_path);
		return -1;
	}

	snprintf(orig_path, sizeof(orig_path), "%s.orig", r_path);
	snprintf(tmp_path, sizeof(tmp_path), "%s.tmp", r_path);
	if ((wfp = fopen(tmp_path, "w")) == NULL) {
		WRITE_TRACE(DBG_FATAL, "failed to open %s: %m", tmp_path);
		fclose(rfp);
		return -1;
	}
	while (fgets(buf, sizeof(buf), rfp) != NULL) {
		char *p = buf;

		while (*p != '\0' && isspace(*p)) p++;

		if (strncmp(p, "BANDWIDTH=", 10) == 0) {
			QString tmp = get_BANDWIDTH_string(conf);
			if (!tmp.isEmpty())
				fprintf(wfp, "BANDWIDTH=\"%s\"\n", QSTR2UTF8(tmp));
			else
				fprintf(wfp, "%s", buf);
			bandwidth_printed = true;
		} else if (strncmp(p, "TOTALRATE=", 10) == 0) {
			QString tmp = get_TOTALRATE_string(conf);
			if (!tmp.isEmpty())
				fprintf(wfp, "TOTALRATE=\"%s\"\n", QSTR2UTF8(tmp));
			totalrate_printed = true;
		} else if (strncmp(p, "RATEMPU=", 8) == 0) {
			QString tmp = get_RATEMPU_string(conf);
			if (!tmp.isEmpty())
				fprintf(wfp, "RATEMPU=\"%s\"\n", QSTR2UTF8(tmp));
			ratempu_printed = true;
		} else if (strncmp(p, "RATE=", 5) == 0) {
			QString tmp = get_RATE_string(conf);
			if (!tmp.isEmpty())
				fprintf(wfp, "RATE=\"%s\"\n", QSTR2UTF8(tmp));
			rate_printed = true;
		} else if (strncmp(p, "TRAFFIC_SHAPING=", 16) == 0) {
			fprintf(wfp, "TRAFFIC_SHAPING=\"%s\"\n",
					conf.isEnabled() ? "yes" : "no");
			enable_printed = true;
		} else {
			fprintf(wfp, "%s", buf);
		}
	}
	fclose(rfp);
	if (!bandwidth_printed) {
		QString tmp = get_BANDWIDTH_string(conf);
		if (!tmp.isEmpty())
			fprintf(wfp, "TOTALRATE=\"%s\"\n", QSTR2UTF8(tmp));
	}
	if (!totalrate_printed) {
		QString tmp = get_TOTALRATE_string(conf);
		if (!tmp.isEmpty())
			fprintf(wfp, "TOTALRATE=\"%s\"\n", QSTR2UTF8(tmp));
	}
	if (!ratempu_printed) {
		QString tmp = get_RATEMPU_string(conf);
		if (!tmp.isEmpty())
			fprintf(wfp, "RATEMPU=\"%s\"\n", QSTR2UTF8(tmp));
	}
	if (!rate_printed) {
		QString tmp = get_RATE_string(conf);
		if (!tmp.isEmpty())
			fprintf(wfp, "RATE=\"%s\"\n", QSTR2UTF8(tmp));
	}
	if (!enable_printed) {
		fprintf(wfp, "TRAFFIC_SHAPING=\"%s\"\n", conf.isEnabled() ? "yes" : "no");
	}
	fclose(wfp);
	// create backup of vz config for restore in case of failure
	if (link(r_path, orig_path)) {
		WRITE_TRACE(DBG_FATAL, "link %s -> %s :%m",
				r_path, orig_path);
		return -1;
	}
	if (rename(tmp_path, r_path)) {
		WRITE_TRACE(DBG_FATAL, "rename %s -> %s :%m",
				tmp_path, r_path);
		return -1;
	}

	// Apply changes
	args += "shaperrestart";
	res = run_prg("/etc/init.d/vz", args);
	if (PRL_FAILED(res)) {
		WRITE_TRACE(DBG_FATAL, "Failed to restart network shaping,"
			" error = 0x%x", res);
		rename(orig_path, r_path);
		return -1;
	}

	unlink(orig_path);
	return 0;
}

static int parse_BANDWIDTH(QList<CDeviceBandwidth> &lst, const char *str)
{
	QStringList lstStr;
	QString sStr(str);

	// "dev:class:totalrate ..."
	sStr.remove(QChar('"'));
	lstStr = sStr.split(QRegExp("\\s+"), QString::SkipEmptyParts);

	foreach(QString sEntry, lstStr) {
		QStringList lstParams;

		lstParams = sEntry.split(":");

		if (lstParams.count() != 2)
			continue;

		QString sDev = lstParams[0];
		PRL_UINT32 nBandwidth = lstParams[1].toUInt();

		CDeviceBandwidth entry;

		entry.setDevice(sDev);
		entry.setBandwidth(nBandwidth);
		lst += entry;
	}
	return 0;
}

static int parse_TOTALRATE(QList<CNetworkShaping> &lst, const char *str)
{
	QStringList lstStr;
	QString sStr(str);

	// "dev:class:totalrate ..."
	sStr.remove(QChar('"'));
	lstStr = sStr.split(QRegExp("\\s+"), QString::SkipEmptyParts);

	foreach(QString sEntry, lstStr) {
		QStringList lstParams;

		lstParams = sEntry.split(":");

		if (lstParams.count() != 3)
			continue;

		QString sDev = lstParams[0];
		PRL_UINT32 nClassId = lstParams[1].toUInt();
		PRL_UINT32 nTotalRate = lstParams[2].toUInt();

		CNetworkShaping entry;

		entry.setDevice(sDev);
		entry.setClassId(nClassId);
		entry.setTotalRate(nTotalRate);
		lst += entry;
	}
	return 0;
}

static int parse_RATEMPU(QList<CNetworkShaping> &lst, const char *str)
{
	QStringList lstStr;
	QString sStr(str);

	// "dev:class[:mpu] ..."
	sStr.remove(QChar('"'));
	lstStr = sStr.split(QRegExp("\\s+"), QString::SkipEmptyParts);

	foreach(const QString& sEntry, lstStr) {
		QStringList lstParams;

		lstParams = sEntry.split(":");

		if (lstParams.count() != 3 && lstParams.count() != 2)
			continue;

		PRL_UINT32 nClassId = lstParams[1].toUInt();
		PRL_INT32 nRateMPU = (lstParams.count() == 2 ?
				NRM_ENABLED : lstParams[2].toInt());

		CNetworkShaping entry;

		entry.setDevice(lstParams[0]);
		entry.setClassId(nClassId);
		entry.setRateMPU(nRateMPU);
		lst += entry;
	}
	return 0;
}

static int parse_RATE(QList<CNetworkShaping> &lst, const char *str)
{
	QStringList lstStr;
	QString sStr(str);

	// "dev:class:totalrate ..."
	sStr.remove(QChar('"'));
	lstStr = sStr.split(QRegExp("\\s+"), QString::SkipEmptyParts);

	foreach(QString sEntry, lstStr) {
		QStringList lstParams;

		lstParams = sEntry.split(":");

		if (lstParams.count() != 3)
			continue;

		QString sDev = lstParams[0];
		PRL_UINT32 nClassId = lstParams[1].toUInt();
		PRL_UINT32 nTotalRate = lstParams[2].toUInt();

		CNetworkShaping entry;

		entry.setDevice(sDev);
		entry.setClassId(nClassId);
		entry.setRate(nTotalRate);
		lst += entry;
	}
	return 0;
}

/* add merged TOTALRATE & RATE to the shaping configuration */
static void merge_lst(CNetworkShapingConfig &conf,
		QList<CDeviceBandwidth> &lstBANDWIDTH,
		QList<CNetworkShaping > &lstTOTALRATE,
		QList<CNetworkShaping> &lstRATEMPU,
		QList<CNetworkShaping > &lstRATE)
{
	CNetworkShaping default_rate, default_ratempu;
	foreach(CNetworkShaping totalrate, lstTOTALRATE)
	{
		foreach(CNetworkShaping rate, lstRATE)
		{
			if (rate.getDevice() == "*")
				default_rate = rate;
			if (rate.getClassId() == totalrate.getClassId() &&
			    rate.getDevice() == totalrate.getDevice())
			{
				totalrate.setRate(rate.getRate());
				break;
			}
		}
		if (totalrate.getRate() == 0)
			 totalrate.setRate(default_rate.getRate());

		bool ratempu_set = false;
		// If dev:class is not in RATEMPU then packet rate limit is disabled.
		default_ratempu.setRateMPU(NRM_DISABLED);
		foreach(const CNetworkShaping& ratempu, lstRATEMPU)
		{
			// Classes are independent in this case, we do not care about inclusion.
			if (ratempu.getDevice() == "*" &&
			    ratempu.getClassId() == totalrate.getClassId())
			{
				default_ratempu = ratempu;
			}
			if (ratempu.getClassId() == totalrate.getClassId() &&
			    ratempu.getDevice() == totalrate.getDevice())
			{
				totalrate.setRateMPU(ratempu.getRateMPU());
				ratempu_set = true;
				break;
			}
		}
		if (!ratempu_set)
			 totalrate.setRateMPU(default_ratempu.getRateMPU());

		conf.m_lstNetworkShaping += new CNetworkShaping(totalrate);
	}

	foreach(CDeviceBandwidth device, lstBANDWIDTH)
	{
		conf.m_lstDevicesBandwidth += new CDeviceBandwidth(device);
	}
}

int CVzNetworkShaping::get_network_shaping_config(CNetworkShapingConfig &conf)
{
	FILE *fp;
	char buf[4096];

        if ((fp = fopen(NETWORK_SHAPING_CONF, "r")) == NULL) {
		WRITE_TRACE(DBG_FATAL, "failed to open %s: %m", NETWORK_SHAPING_CONF);
                return -1;
	}

	CNetworkShapingConfig newConf;
	QList<CDeviceBandwidth> lstBANDWIDTH;
	QList<CNetworkShaping > lstTOTALRATE;
	QList<CNetworkShaping > lstRATEMPU;
	QList<CNetworkShaping > lstRATE;

	while (fgets(buf, sizeof(buf), fp) != NULL) {
		char *p = buf;

		while (*p != '\0' && isspace(*p)) p++;

		if (strncmp(p, "BANDWIDTH=", 10) == 0) {
			parse_BANDWIDTH(lstBANDWIDTH, p + 10);
		} else if (strncmp(p, "TOTALRATE=", 10) == 0) {
			parse_TOTALRATE(lstTOTALRATE, p + 10);
		} else if (strncmp(p, "RATEMPU=", 8) == 0) {
			parse_RATEMPU(lstRATEMPU, p + 8);
		} else if (strncmp(p, "RATE=", 5) == 0) {
			parse_RATE(lstRATE, p + 5);
		} else if (strncmp(p, "TRAFFIC_SHAPING=", 16) == 0) {
			if (strstr(p + 16, "yes") != NULL)
				newConf.setEnabled(true);
		}
	}
	fclose(fp);

	merge_lst(newConf, lstBANDWIDTH, lstTOTALRATE, lstRATEMPU, lstRATE);

	conf.fromString(newConf.toString());

	return 0;
}

int CVzNetworkShaping::update_network_classes_config(const CNetworkClassesConfig &conf)
{
	PRL_RESULT res;
	FILE *wfp;

        if ((wfp = fopen(NETWORK_CLASSES_CONF ".tmp", "w")) == NULL) {
		WRITE_TRACE(DBG_FATAL, "failed to open %s: %m", NETWORK_CLASSES_CONF);
                return -1;
	}

	foreach(CNetworkClass *entry, conf.m_lstNetworkClasses) {
		foreach(QString sNet, entry->getNetworkList()) {
			fprintf(wfp, "%d %s\n",
					entry->getClassId(),
					QSTR2UTF8(sNet));
		}
	}
	fclose(wfp);

	QStringList args;

	args += "class_load";
	args += "-s";
	args += NETWORK_CLASSES_CONF ".tmp";
	// Apply changes
	res = run_prg("/usr/sbin/vztactl", args);
	if (PRL_SUCCEEDED(res))
		rename(NETWORK_CLASSES_CONF ".tmp", NETWORK_CLASSES_CONF);

        return res;
}

int add_network_class_entry(CNetworkClassesConfig &conf, PRL_UINT32 nClassId, const char *net, PRL_UINT32 nMask)
{
	foreach(CNetworkClass *entry, conf.m_lstNetworkClasses) {
		if (entry->getClassId() == nClassId) {
			QList<QString> lst = entry->getNetworkList();
			lst += QString("%1/%2")
					.arg(net)
					.arg(nMask);
			entry->setNetworkList(lst);
			return 0;
		}
	}
	CNetworkClass *entry = new CNetworkClass();
	entry->setClassId(nClassId);
	QList<QString> lst;
	lst += QString("%1/%2")
			.arg(net)
			.arg(nMask);

	entry->setNetworkList(lst);
	conf.m_lstNetworkClasses += entry;

	return 0;
}

int CVzNetworkShaping::get_network_classes_config(CNetworkClassesConfig &conf)
{
	FILE *fp;
	char buf[512];

        if ((fp = fopen(NETWORK_CLASSES_CONF, "r")) == NULL) {
		WRITE_TRACE(DBG_FATAL, "failed to open %s: %m", NETWORK_CLASSES_CONF);
                return -1;
	}
	conf.ClearLists();
	while (fgets(buf, sizeof(buf), fp) != NULL) {
		char net[64];
		PRL_UINT32 nClassId, nMask;
		char *p = buf;

		while (*p != '\0' && isspace(*p)) p++;

		if (*p == '#')
			continue;
		if (sscanf(buf, "%u%*[\t ]%41[^/]/%u",
					&nClassId, net, &nMask) == 3)
		{
			add_network_class_entry(conf, nClassId, net, nMask);
		}
	}
	fclose(fp);

	return 0;
}

int CVzNetworkShaping::set_rate(unsigned int id, const CVmNetworkRates &rates)
{
	QStringList args;

	args += "setrate";
	args += QString("%1").arg(id);

	args += "--ratebound";
	if (rates.isRateBound())
		args += "yes";
	else
		args += "no";

	foreach(CVmNetworkRate *rate, rates.m_lstNetworkRates) {
		QString str;
		// FIXME: validate network class and skip if not valid
		args += "--rate";
		args += QString("*:%1:%2")
				.arg(rate->getClassId())
				.arg(rate->getRate());
	}

	PRL_RESULT res = run_prg("/usr/sbin/vzctl", args);

	return res;
}

