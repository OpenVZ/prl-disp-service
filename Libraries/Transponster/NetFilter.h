///////////////////////////////////////////////////////////////////////////////
///
/// @file NetFilter.h
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

#ifndef __NetFilter_H__
#define __NetFilter_H__

#include <prlxmlmodel/VmConfig/CNetPktFilter.h>

namespace Transponster
{
class NetFilter
{
public:
    // First non-combinable, then combinable filters
    enum BuiltinFilter
    {
        FILTER_CUSTOM       = 0,
        FILTER_EMPTY        = 1u << 0,
        FILTER_IP_SPOOFING  = 1u << 1, // combinable
        FILTER_MAC_SPOOFING = 1u << 2, // combinable
        FILTER_PROMISC      = 1u << 3, // combinable
    };

    typedef uint FilterMask_t;
    typedef QPair<QString, QString> ParamPair_t;

    NetFilter();
    NetFilter(const CNetPktFilter& filter);

    bool isPreventPromisc() const;
    void setPreventPromisc(bool value = true);
    bool isPreventMacSpoof() const;
    void setPreventMacSpoof(bool value = true);
    bool isPreventIpSpoof() const;
    void setPreventIpSpoof(bool value = true);
    /**
     * Check if filter is managed by Dispatcher
     *
     * @return true if filter is not managed by Dispatcher
     */
    bool isCustomFilter() const;
    QString getFilterRef() const;
    void setFilterRef(QString value);
    void setVzFilter(QString mac);
    QList<ParamPair_t> getParams() const;
    void setParams(QList<ParamPair_t> value);

    // not counting the custom option in
    static const uint S_NON_COMPANIABLE_FILTERS = 1;
    static const uint S_COMPANIABLE_FILTERS = 3;
    static const QString S_VZ_FILTER_PREFIX;
    static const QString S_VZ_FILTER_MASK;
private:
    /**
     * Returns bitwise OR of BuiltinFilter values, 
     * corresponding to enabled builtin filters.
     */
    FilterMask_t getFilterMask() const;
    /**
     * Sets filter name using FilterMask_t
     * 
     * @param mask Bitwise OR of BuiltinFilter values.
     */ 
    void setFilterByMask(FilterMask_t mask);
    /**
     * Check if one of the builtin filters are enabled
     *
     * @param filter BuiltinFilter to check presence
     * @return true if enabled, else false
     */
    bool isBuiltinFilter(BuiltinFilter filter) const;
    /**
     * Check if one of the builtin filters are set in the given mask
     *
     * @param filter BuiltinFilter to check presence
     * @param mask FilterMask_t to be examined.
     * @return true if enabled, else false
     */
    bool isBuiltinFilter(BuiltinFilter filter, FilterMask_t mask) const;
    /**
     * Turn on/off builtin filter
     * 
     * @param filter Filter to change state
     * @param value desired filter state
     */
    void setBuiltinFilter(BuiltinFilter filter, bool value);

    /**
     * Get the array, which maps builtin filter enums
     * with their filter names.
    */
    static QMap<BuiltinFilter, QString> getFilterNames();
    /**
     * Get the list of all combinations of builtin filters, except the one,
     * which belongs to the VM (starts with "vz-filter-")
     */
    static QList<QString> getBuiltinFilters(); 
    /**
     * Converting CNetPktFilterParam object to simple QPairs (Name, Value)
     */
    static QList<ParamPair_t> convertParamsToPairs(
                                            QList<CNetPktFilterParam*> params
                                                               );

    QString m_filterref;
    QList<ParamPair_t> m_params;
};
} // namespace Transponster

#endif // __NetFilter_H__
