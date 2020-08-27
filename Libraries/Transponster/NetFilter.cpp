///////////////////////////////////////////////////////////////////////////////
///
/// @file NetFilter.cpp
///
/// Helper for handling filterref
///
/// @author alexander.alekseev
///
/// Copyright (c) 2020 Virtuozzo International GmbH, All rights reserved.
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
/// Our contact details: Virtuozzo International GmbH, Vordergasse 59, 8200
/// Schaffhausen, Switzerland.
///
///////////////////////////////////////////////////////////////////////////////

#include "NetFilter.h"

namespace Transponster
{
///////////////////////////////////////////////////////////////////////////////
// class NetFilter

const QString NetFilter::S_VZ_FILTER_PREFIX = "vz-filter-";

bool NetFilter::isBuiltinFilter(BuiltinFilter filter) const
{
	return isBuiltinFilter(filter, getFilterMask());
}

bool NetFilter::isBuiltinFilter(BuiltinFilter filter, FilterMask_t mask) const
{
	return mask & static_cast<FilterMask_t>(filter);
}

void NetFilter::setBuiltinFilter(BuiltinFilter filter, bool value)
{
	if (value)
		setFilterByMask(getFilterMask() | static_cast<FilterMask_t>(filter));
	else
		setFilterByMask(getFilterMask() & ~static_cast<FilterMask_t>(filter));
}

QMap<NetFilter::BuiltinFilter, QString> NetFilter::getFilterNames()
{
	QMap<BuiltinFilter, QString> filter_names;
	filter_names.insert(FILTER_IP_SPOOFING,  "no-ip-spoofing");
	filter_names.insert(FILTER_MAC_SPOOFING, "no-mac-spoofing");
	filter_names.insert(FILTER_PROMISC,      "no-promisc");
	return filter_names;
}

QList<QString> NetFilter::getBuiltinFilters()
{
	QStringList builtin;

	builtin.append("no-ip-spoofing");
	builtin.append("no-ip-spoofing-no-mac-spoofing");
	builtin.append("no-ip-spoofing-no-mac-spoofing-no-promisc");
	builtin.append("no-ip-spoofing-no-promisc");
	builtin.append("no-mac-spoofing");
	builtin.append("no-mac-spoofing-no-promisc");
	builtin.append("no-promisc");

	return builtin;
}

QList<QPair<QString, QString> > NetFilter::convertParamsToPairs(
											QList<CNetPktFilterParam*> params
																   )
{
	QList<QPair<QString, QString> > result;
	foreach(const CNetPktFilterParam* param, params)
		result.append(qMakePair(param->getName(), param->getValue()));
	
	return result;
}

bool NetFilter::isCustomFilter() const
{
	// for the future filter
	if (getFilterRef().startsWith(S_VZ_FILTER_PREFIX))
		return false;

	return !getBuiltinFilters().contains(getFilterRef());
}

NetFilter::FilterMask_t NetFilter::getFilterMask() const
{
	if (isCustomFilter())
		return FILTER_CUSTOM;

	FilterMask_t mask = 0u;
	QMap<BuiltinFilter, QString> filter_names = getFilterNames();
	foreach(const BuiltinFilter& filter_enum, filter_names.keys())
	{
		if (getFilterRef().contains(filter_names.value(filter_enum)))
			mask |= filter_enum;
	}
	return mask;
}

void NetFilter::setFilterByMask(NetFilter::FilterMask_t mask)
{
	QStringList filters;
	QMap<BuiltinFilter, QString> filter_names = getFilterNames();
	foreach(const BuiltinFilter& filter_enum, filter_names.keys())
	{
		if (isBuiltinFilter(filter_enum, mask))
			filters.append(filter_names.value(filter_enum));
	}
	setFilterRef(filters.join("-"));
}

NetFilter::NetFilter(const CNetPktFilter& filter_model)
{
	if (!filter_model.getFilterRef().isEmpty())
	{
		setFilterRef(filter_model.getFilterRef());
		m_params = convertParamsToPairs(filter_model.m_lstParameters);
	} else
	{
		FilterMask_t mask = 0u;
		mask |= filter_model.isPreventIpSpoof()  ?
					static_cast<FilterMask_t>(FILTER_IP_SPOOFING)  : 0u;
		mask |= filter_model.isPreventMacSpoof() ?
					static_cast<FilterMask_t>(FILTER_MAC_SPOOFING) : 0u;
		mask |= filter_model.isPreventPromisc()  ?
					static_cast<FilterMask_t>(FILTER_PROMISC)      : 0u;
		setFilterByMask(mask);
	}
}

bool NetFilter::isPreventPromisc() const
{
	return isBuiltinFilter(FILTER_PROMISC);
}

void NetFilter::setPreventPromisc(bool value)
{
	setBuiltinFilter(FILTER_PROMISC, value);
}

bool NetFilter::isPreventMacSpoof() const
{
	return isBuiltinFilter(FILTER_MAC_SPOOFING);
}

void NetFilter::setPreventMacSpoof(bool value)
{
	setBuiltinFilter(FILTER_MAC_SPOOFING, value);
}

bool NetFilter::isPreventIpSpoof() const
{
	return isBuiltinFilter(FILTER_IP_SPOOFING);
}

void NetFilter::setPreventIpSpoof(bool value)
{
	setBuiltinFilter(FILTER_IP_SPOOFING, value);
}

QString NetFilter::getFilterRef() const
{
	return m_filterref;
}

void NetFilter::setFilterRef(QString value)
{
	m_filterref = value;
}

QList<NetFilter::ParamPair_t> NetFilter::getParams() const
{
	return m_params;
}

void NetFilter::setParams(QList<NetFilter::ParamPair_t> value)
{
	m_params = value;
}
} // namespace Transponster
