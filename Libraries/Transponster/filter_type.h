/*
 * Copyright (c) 2020 Virtuozzo International GmbH. All rights reserved.
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
 * Our contact details: Virtuozzo International GmbH, Vordergasse 59, 8200
 * Schaffhausen, Switzerland.
 */

#ifndef __FILTER_TYPE_H__
#define __FILTER_TYPE_H__
#include "base.h"
#include "filter_data.h"
#include "filter_enum.h"
#include "patterns.h"
#include <boost/any.hpp>

namespace Libvirt
{

///////////////////////////////////////////////////////////////////////////////
// struct FilterNodeAttributes

namespace Filter
{
namespace Xml
{
struct FilterNodeAttributes
{
	const PName::value_type& getName() const
	{
		return m_name;
	}
	void setName(const PName::value_type& value_)
	{
		m_name = value_;
	}
	const boost::optional<VChain >& getChain() const
	{
		return m_chain;
	}
	void setChain(const boost::optional<VChain >& value_)
	{
		m_chain = value_;
	}
	const boost::optional<PPriorityType::value_type >& getPriority() const
	{
		return m_priority;
	}
	void setPriority(const boost::optional<PPriorityType::value_type >& value_)
	{
		m_priority = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;

private:
	PName::value_type m_name;
	boost::optional<VChain > m_chain;
	boost::optional<PPriorityType::value_type > m_priority;
};

} // namespace Xml
} // namespace Filter

///////////////////////////////////////////////////////////////////////////////
// struct Parameter

namespace Filter
{
namespace Xml
{
struct Parameter
{
	const PFilterParamName::value_type& getName() const
	{
		return m_name;
	}
	void setName(const PFilterParamName::value_type& value_)
	{
		m_name = value_;
	}
	const PFilterParamValue::value_type& getValue() const
	{
		return m_value;
	}
	void setValue(const PFilterParamValue::value_type& value_)
	{
		m_value = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	PFilterParamName::value_type m_name;
	PFilterParamValue::value_type m_value;
};

} // namespace Xml
} // namespace Filter

///////////////////////////////////////////////////////////////////////////////
// struct FilterrefNodeAttributes

namespace Filter
{
namespace Xml
{
struct FilterrefNodeAttributes
{
	const PFilter::value_type& getFilter() const
	{
		return m_filter;
	}
	void setFilter(const PFilter::value_type& value_)
	{
		m_filter = value_;
	}
	const QList<Parameter >& getParameterList() const
	{
		return m_parameterList;
	}
	void setParameterList(const QList<Parameter >& value_)
	{
		m_parameterList = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	PFilter::value_type m_filter;
	QList<Parameter > m_parameterList;
};

} // namespace Xml
} // namespace Filter

///////////////////////////////////////////////////////////////////////////////
// struct RuleNodeAttributes

namespace Filter
{
namespace Xml
{
struct RuleNodeAttributes
{
	RuleNodeAttributes();

	EActionType getAction() const
	{
		return m_action;
	}
	void setAction(EActionType value_)
	{
		m_action = value_;
	}
	EDirectionType getDirection() const
	{
		return m_direction;
	}
	void setDirection(EDirectionType value_)
	{
		m_direction = value_;
	}
	const boost::optional<PPriorityType::value_type >& getPriority() const
	{
		return m_priority;
	}
	void setPriority(const boost::optional<PPriorityType::value_type >& value_)
	{
		m_priority = value_;
	}
	const boost::optional<PStatematchType::value_type >& getStatematch() const
	{
		return m_statematch;
	}
	void setStatematch(const boost::optional<PStatematchType::value_type >& value_)
	{
		m_statematch = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;

private:
	EActionType m_action;
	EDirectionType m_direction;
	boost::optional<PPriorityType::value_type > m_priority;
	boost::optional<PStatematchType::value_type > m_statematch;
};

} // namespace Xml
} // namespace Filter

///////////////////////////////////////////////////////////////////////////////
// struct CommonL2Attributes

namespace Filter
{
namespace Xml
{
struct CommonL2Attributes
{
	const boost::optional<VAddrMAC >& getSrcmacaddr() const
	{
		return m_srcmacaddr;
	}
	void setSrcmacaddr(const boost::optional<VAddrMAC >& value_)
	{
		m_srcmacaddr = value_;
	}
	const boost::optional<VAddrMAC >& getSrcmacmask() const
	{
		return m_srcmacmask;
	}
	void setSrcmacmask(const boost::optional<VAddrMAC >& value_)
	{
		m_srcmacmask = value_;
	}
	const boost::optional<VAddrMAC >& getDstmacaddr() const
	{
		return m_dstmacaddr;
	}
	void setDstmacaddr(const boost::optional<VAddrMAC >& value_)
	{
		m_dstmacaddr = value_;
	}
	const boost::optional<VAddrMAC >& getDstmacmask() const
	{
		return m_dstmacmask;
	}
	void setDstmacmask(const boost::optional<VAddrMAC >& value_)
	{
		m_dstmacmask = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;

private:
	boost::optional<VAddrMAC > m_srcmacaddr;
	boost::optional<VAddrMAC > m_srcmacmask;
	boost::optional<VAddrMAC > m_dstmacaddr;
	boost::optional<VAddrMAC > m_dstmacmask;
};

} // namespace Xml
} // namespace Filter

///////////////////////////////////////////////////////////////////////////////
// struct Mac

namespace Filter
{
namespace Xml
{
struct Mac
{
	const boost::optional<EVirYesNo >& getMatch() const
	{
		return m_match;
	}
	void setMatch(const boost::optional<EVirYesNo >& value_)
	{
		m_match = value_;
	}
	const CommonL2Attributes& getCommonL2Attributes() const
	{
		return m_commonL2Attributes;
	}
	void setCommonL2Attributes(const CommonL2Attributes& value_)
	{
		m_commonL2Attributes = value_;
	}
	const boost::optional<VMacProtocolid >& getProtocolid() const
	{
		return m_protocolid;
	}
	void setProtocolid(const boost::optional<VMacProtocolid >& value_)
	{
		m_protocolid = value_;
	}
	const boost::optional<PCommentType::value_type >& getComment() const
	{
		return m_comment;
	}
	void setComment(const boost::optional<PCommentType::value_type >& value_)
	{
		m_comment = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	boost::optional<EVirYesNo > m_match;
	CommonL2Attributes m_commonL2Attributes;
	boost::optional<VMacProtocolid > m_protocolid;
	boost::optional<PCommentType::value_type > m_comment;
};

} // namespace Xml
} // namespace Filter

///////////////////////////////////////////////////////////////////////////////
// struct VlanAttributes

namespace Filter
{
namespace Xml
{
struct VlanAttributes
{
	const boost::optional<VVlanVlanid >& getVlanid() const
	{
		return m_vlanid;
	}
	void setVlanid(const boost::optional<VVlanVlanid >& value_)
	{
		m_vlanid = value_;
	}
	const boost::optional<VMacProtocolid >& getEncapProtocol() const
	{
		return m_encapProtocol;
	}
	void setEncapProtocol(const boost::optional<VMacProtocolid >& value_)
	{
		m_encapProtocol = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;

private:
	boost::optional<VVlanVlanid > m_vlanid;
	boost::optional<VMacProtocolid > m_encapProtocol;
};

} // namespace Xml
} // namespace Filter

///////////////////////////////////////////////////////////////////////////////
// struct Vlan

namespace Filter
{
namespace Xml
{
struct Vlan
{
	const boost::optional<EVirYesNo >& getMatch() const
	{
		return m_match;
	}
	void setMatch(const boost::optional<EVirYesNo >& value_)
	{
		m_match = value_;
	}
	const CommonL2Attributes& getCommonL2Attributes() const
	{
		return m_commonL2Attributes;
	}
	void setCommonL2Attributes(const CommonL2Attributes& value_)
	{
		m_commonL2Attributes = value_;
	}
	const VlanAttributes& getVlanAttributes() const
	{
		return m_vlanAttributes;
	}
	void setVlanAttributes(const VlanAttributes& value_)
	{
		m_vlanAttributes = value_;
	}
	const boost::optional<PCommentType::value_type >& getComment() const
	{
		return m_comment;
	}
	void setComment(const boost::optional<PCommentType::value_type >& value_)
	{
		m_comment = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	boost::optional<EVirYesNo > m_match;
	CommonL2Attributes m_commonL2Attributes;
	VlanAttributes m_vlanAttributes;
	boost::optional<PCommentType::value_type > m_comment;
};

} // namespace Xml
} // namespace Filter

///////////////////////////////////////////////////////////////////////////////
// struct SrcmacandmaskAttributes

namespace Filter
{
namespace Xml
{
struct SrcmacandmaskAttributes
{
	const boost::optional<VAddrMAC >& getSrcmacaddr() const
	{
		return m_srcmacaddr;
	}
	void setSrcmacaddr(const boost::optional<VAddrMAC >& value_)
	{
		m_srcmacaddr = value_;
	}
	const boost::optional<VAddrMAC >& getSrcmacmask() const
	{
		return m_srcmacmask;
	}
	void setSrcmacmask(const boost::optional<VAddrMAC >& value_)
	{
		m_srcmacmask = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;

private:
	boost::optional<VAddrMAC > m_srcmacaddr;
	boost::optional<VAddrMAC > m_srcmacmask;
};

} // namespace Xml
} // namespace Filter

///////////////////////////////////////////////////////////////////////////////
// struct StpAttributes

namespace Filter
{
namespace Xml
{
struct StpAttributes
{
	const boost::optional<VUint8range >& getType() const
	{
		return m_type;
	}
	void setType(const boost::optional<VUint8range >& value_)
	{
		m_type = value_;
	}
	const boost::optional<VUint8range >& getFlags() const
	{
		return m_flags;
	}
	void setFlags(const boost::optional<VUint8range >& value_)
	{
		m_flags = value_;
	}
	const boost::optional<VUint16range >& getRootPriority() const
	{
		return m_rootPriority;
	}
	void setRootPriority(const boost::optional<VUint16range >& value_)
	{
		m_rootPriority = value_;
	}
	const boost::optional<VUint16range >& getRootPriorityHi() const
	{
		return m_rootPriorityHi;
	}
	void setRootPriorityHi(const boost::optional<VUint16range >& value_)
	{
		m_rootPriorityHi = value_;
	}
	const boost::optional<VAddrMAC >& getRootAddress() const
	{
		return m_rootAddress;
	}
	void setRootAddress(const boost::optional<VAddrMAC >& value_)
	{
		m_rootAddress = value_;
	}
	const boost::optional<VAddrMAC >& getRootAddressMask() const
	{
		return m_rootAddressMask;
	}
	void setRootAddressMask(const boost::optional<VAddrMAC >& value_)
	{
		m_rootAddressMask = value_;
	}
	const boost::optional<VUint32range >& getRootCost() const
	{
		return m_rootCost;
	}
	void setRootCost(const boost::optional<VUint32range >& value_)
	{
		m_rootCost = value_;
	}
	const boost::optional<VUint32range >& getRootCostHi() const
	{
		return m_rootCostHi;
	}
	void setRootCostHi(const boost::optional<VUint32range >& value_)
	{
		m_rootCostHi = value_;
	}
	const boost::optional<VUint16range >& getSenderPriority() const
	{
		return m_senderPriority;
	}
	void setSenderPriority(const boost::optional<VUint16range >& value_)
	{
		m_senderPriority = value_;
	}
	const boost::optional<VUint16range >& getSenderPriorityHi() const
	{
		return m_senderPriorityHi;
	}
	void setSenderPriorityHi(const boost::optional<VUint16range >& value_)
	{
		m_senderPriorityHi = value_;
	}
	const boost::optional<VAddrMAC >& getSenderAddress() const
	{
		return m_senderAddress;
	}
	void setSenderAddress(const boost::optional<VAddrMAC >& value_)
	{
		m_senderAddress = value_;
	}
	const boost::optional<VAddrMAC >& getSenderAddressMask() const
	{
		return m_senderAddressMask;
	}
	void setSenderAddressMask(const boost::optional<VAddrMAC >& value_)
	{
		m_senderAddressMask = value_;
	}
	const boost::optional<VUint16range >& getPort() const
	{
		return m_port;
	}
	void setPort(const boost::optional<VUint16range >& value_)
	{
		m_port = value_;
	}
	const boost::optional<VUint16range >& getPortHi() const
	{
		return m_portHi;
	}
	void setPortHi(const boost::optional<VUint16range >& value_)
	{
		m_portHi = value_;
	}
	const boost::optional<VUint16range >& getAge() const
	{
		return m_age;
	}
	void setAge(const boost::optional<VUint16range >& value_)
	{
		m_age = value_;
	}
	const boost::optional<VUint16range >& getAgeHi() const
	{
		return m_ageHi;
	}
	void setAgeHi(const boost::optional<VUint16range >& value_)
	{
		m_ageHi = value_;
	}
	const boost::optional<VUint16range >& getMaxAge() const
	{
		return m_maxAge;
	}
	void setMaxAge(const boost::optional<VUint16range >& value_)
	{
		m_maxAge = value_;
	}
	const boost::optional<VUint16range >& getMaxAgeHi() const
	{
		return m_maxAgeHi;
	}
	void setMaxAgeHi(const boost::optional<VUint16range >& value_)
	{
		m_maxAgeHi = value_;
	}
	const boost::optional<VUint16range >& getHelloTime() const
	{
		return m_helloTime;
	}
	void setHelloTime(const boost::optional<VUint16range >& value_)
	{
		m_helloTime = value_;
	}
	const boost::optional<VUint16range >& getHelloTimeHi() const
	{
		return m_helloTimeHi;
	}
	void setHelloTimeHi(const boost::optional<VUint16range >& value_)
	{
		m_helloTimeHi = value_;
	}
	const boost::optional<VUint16range >& getForwardDelay() const
	{
		return m_forwardDelay;
	}
	void setForwardDelay(const boost::optional<VUint16range >& value_)
	{
		m_forwardDelay = value_;
	}
	const boost::optional<VUint16range >& getForwardDelayHi() const
	{
		return m_forwardDelayHi;
	}
	void setForwardDelayHi(const boost::optional<VUint16range >& value_)
	{
		m_forwardDelayHi = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;

private:
	boost::optional<VUint8range > m_type;
	boost::optional<VUint8range > m_flags;
	boost::optional<VUint16range > m_rootPriority;
	boost::optional<VUint16range > m_rootPriorityHi;
	boost::optional<VAddrMAC > m_rootAddress;
	boost::optional<VAddrMAC > m_rootAddressMask;
	boost::optional<VUint32range > m_rootCost;
	boost::optional<VUint32range > m_rootCostHi;
	boost::optional<VUint16range > m_senderPriority;
	boost::optional<VUint16range > m_senderPriorityHi;
	boost::optional<VAddrMAC > m_senderAddress;
	boost::optional<VAddrMAC > m_senderAddressMask;
	boost::optional<VUint16range > m_port;
	boost::optional<VUint16range > m_portHi;
	boost::optional<VUint16range > m_age;
	boost::optional<VUint16range > m_ageHi;
	boost::optional<VUint16range > m_maxAge;
	boost::optional<VUint16range > m_maxAgeHi;
	boost::optional<VUint16range > m_helloTime;
	boost::optional<VUint16range > m_helloTimeHi;
	boost::optional<VUint16range > m_forwardDelay;
	boost::optional<VUint16range > m_forwardDelayHi;
};

} // namespace Xml
} // namespace Filter

///////////////////////////////////////////////////////////////////////////////
// struct Stp

namespace Filter
{
namespace Xml
{
struct Stp
{
	const boost::optional<EVirYesNo >& getMatch() const
	{
		return m_match;
	}
	void setMatch(const boost::optional<EVirYesNo >& value_)
	{
		m_match = value_;
	}
	const SrcmacandmaskAttributes& getSrcmacandmaskAttributes() const
	{
		return m_srcmacandmaskAttributes;
	}
	void setSrcmacandmaskAttributes(const SrcmacandmaskAttributes& value_)
	{
		m_srcmacandmaskAttributes = value_;
	}
	const StpAttributes& getStpAttributes() const
	{
		return m_stpAttributes;
	}
	void setStpAttributes(const StpAttributes& value_)
	{
		m_stpAttributes = value_;
	}
	const boost::optional<PCommentType::value_type >& getComment() const
	{
		return m_comment;
	}
	void setComment(const boost::optional<PCommentType::value_type >& value_)
	{
		m_comment = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	boost::optional<EVirYesNo > m_match;
	SrcmacandmaskAttributes m_srcmacandmaskAttributes;
	StpAttributes m_stpAttributes;
	boost::optional<PCommentType::value_type > m_comment;
};

} // namespace Xml
} // namespace Filter

///////////////////////////////////////////////////////////////////////////////
// struct ArpAttributes

namespace Filter
{
namespace Xml
{
struct ArpAttributes
{
	const boost::optional<VAddrMAC >& getArpsrcmacaddr() const
	{
		return m_arpsrcmacaddr;
	}
	void setArpsrcmacaddr(const boost::optional<VAddrMAC >& value_)
	{
		m_arpsrcmacaddr = value_;
	}
	const boost::optional<VAddrIP >& getArpsrcipaddr() const
	{
		return m_arpsrcipaddr;
	}
	void setArpsrcipaddr(const boost::optional<VAddrIP >& value_)
	{
		m_arpsrcipaddr = value_;
	}
	const boost::optional<VAddrMAC >& getArpdstmacaddr() const
	{
		return m_arpdstmacaddr;
	}
	void setArpdstmacaddr(const boost::optional<VAddrMAC >& value_)
	{
		m_arpdstmacaddr = value_;
	}
	const boost::optional<VAddrIP >& getArpdstipaddr() const
	{
		return m_arpdstipaddr;
	}
	void setArpdstipaddr(const boost::optional<VAddrIP >& value_)
	{
		m_arpdstipaddr = value_;
	}
	const boost::optional<VUint16range >& getHwtype() const
	{
		return m_hwtype;
	}
	void setHwtype(const boost::optional<VUint16range >& value_)
	{
		m_hwtype = value_;
	}
	const boost::optional<VArpOpcodeType >& getOpcode() const
	{
		return m_opcode;
	}
	void setOpcode(const boost::optional<VArpOpcodeType >& value_)
	{
		m_opcode = value_;
	}
	const boost::optional<VUint16range >& getProtocoltype() const
	{
		return m_protocoltype;
	}
	void setProtocoltype(const boost::optional<VUint16range >& value_)
	{
		m_protocoltype = value_;
	}
	const boost::optional<EBoolean >& getGratuitous() const
	{
		return m_gratuitous;
	}
	void setGratuitous(const boost::optional<EBoolean >& value_)
	{
		m_gratuitous = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;

private:
	boost::optional<VAddrMAC > m_arpsrcmacaddr;
	boost::optional<VAddrIP > m_arpsrcipaddr;
	boost::optional<VAddrMAC > m_arpdstmacaddr;
	boost::optional<VAddrIP > m_arpdstipaddr;
	boost::optional<VUint16range > m_hwtype;
	boost::optional<VArpOpcodeType > m_opcode;
	boost::optional<VUint16range > m_protocoltype;
	boost::optional<EBoolean > m_gratuitous;
};

} // namespace Xml
} // namespace Filter

///////////////////////////////////////////////////////////////////////////////
// struct Arp

namespace Filter
{
namespace Xml
{
struct Arp
{
	const boost::optional<EVirYesNo >& getMatch() const
	{
		return m_match;
	}
	void setMatch(const boost::optional<EVirYesNo >& value_)
	{
		m_match = value_;
	}
	const CommonL2Attributes& getCommonL2Attributes() const
	{
		return m_commonL2Attributes;
	}
	void setCommonL2Attributes(const CommonL2Attributes& value_)
	{
		m_commonL2Attributes = value_;
	}
	const ArpAttributes& getArpAttributes() const
	{
		return m_arpAttributes;
	}
	void setArpAttributes(const ArpAttributes& value_)
	{
		m_arpAttributes = value_;
	}
	const boost::optional<PCommentType::value_type >& getComment() const
	{
		return m_comment;
	}
	void setComment(const boost::optional<PCommentType::value_type >& value_)
	{
		m_comment = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	boost::optional<EVirYesNo > m_match;
	CommonL2Attributes m_commonL2Attributes;
	ArpAttributes m_arpAttributes;
	boost::optional<PCommentType::value_type > m_comment;
};

} // namespace Xml
} // namespace Filter

///////////////////////////////////////////////////////////////////////////////
// struct Rarp

namespace Filter
{
namespace Xml
{
struct Rarp
{
	const boost::optional<EVirYesNo >& getMatch() const
	{
		return m_match;
	}
	void setMatch(const boost::optional<EVirYesNo >& value_)
	{
		m_match = value_;
	}
	const CommonL2Attributes& getCommonL2Attributes() const
	{
		return m_commonL2Attributes;
	}
	void setCommonL2Attributes(const CommonL2Attributes& value_)
	{
		m_commonL2Attributes = value_;
	}
	const ArpAttributes& getArpAttributes() const
	{
		return m_arpAttributes;
	}
	void setArpAttributes(const ArpAttributes& value_)
	{
		m_arpAttributes = value_;
	}
	const boost::optional<PCommentType::value_type >& getComment() const
	{
		return m_comment;
	}
	void setComment(const boost::optional<PCommentType::value_type >& value_)
	{
		m_comment = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	boost::optional<EVirYesNo > m_match;
	CommonL2Attributes m_commonL2Attributes;
	ArpAttributes m_arpAttributes;
	boost::optional<PCommentType::value_type > m_comment;
};

} // namespace Xml
} // namespace Filter

///////////////////////////////////////////////////////////////////////////////
// struct CommonIpAttributesP1

namespace Filter
{
namespace Xml
{
struct CommonIpAttributesP1
{
	const boost::optional<VAddrIP >& getSrcipaddr() const
	{
		return m_srcipaddr;
	}
	void setSrcipaddr(const boost::optional<VAddrIP >& value_)
	{
		m_srcipaddr = value_;
	}
	const boost::optional<VAddrMask >& getSrcipmask() const
	{
		return m_srcipmask;
	}
	void setSrcipmask(const boost::optional<VAddrMask >& value_)
	{
		m_srcipmask = value_;
	}
	const boost::optional<VAddrIP >& getDstipaddr() const
	{
		return m_dstipaddr;
	}
	void setDstipaddr(const boost::optional<VAddrIP >& value_)
	{
		m_dstipaddr = value_;
	}
	const boost::optional<VAddrMask >& getDstipmask() const
	{
		return m_dstipmask;
	}
	void setDstipmask(const boost::optional<VAddrMask >& value_)
	{
		m_dstipmask = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;

private:
	boost::optional<VAddrIP > m_srcipaddr;
	boost::optional<VAddrMask > m_srcipmask;
	boost::optional<VAddrIP > m_dstipaddr;
	boost::optional<VAddrMask > m_dstipmask;
};

} // namespace Xml
} // namespace Filter

///////////////////////////////////////////////////////////////////////////////
// struct CommonPortAttributes

namespace Filter
{
namespace Xml
{
struct CommonPortAttributes
{
	const boost::optional<VUint16range >& getSrcportstart() const
	{
		return m_srcportstart;
	}
	void setSrcportstart(const boost::optional<VUint16range >& value_)
	{
		m_srcportstart = value_;
	}
	const boost::optional<VUint16range >& getSrcportend() const
	{
		return m_srcportend;
	}
	void setSrcportend(const boost::optional<VUint16range >& value_)
	{
		m_srcportend = value_;
	}
	const boost::optional<VUint16range >& getDstportstart() const
	{
		return m_dstportstart;
	}
	void setDstportstart(const boost::optional<VUint16range >& value_)
	{
		m_dstportstart = value_;
	}
	const boost::optional<VUint16range >& getDstportend() const
	{
		return m_dstportend;
	}
	void setDstportend(const boost::optional<VUint16range >& value_)
	{
		m_dstportend = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;

private:
	boost::optional<VUint16range > m_srcportstart;
	boost::optional<VUint16range > m_srcportend;
	boost::optional<VUint16range > m_dstportstart;
	boost::optional<VUint16range > m_dstportend;
};

} // namespace Xml
} // namespace Filter

///////////////////////////////////////////////////////////////////////////////
// struct Ip

namespace Filter
{
namespace Xml
{
struct Ip
{
	const boost::optional<EVirYesNo >& getMatch() const
	{
		return m_match;
	}
	void setMatch(const boost::optional<EVirYesNo >& value_)
	{
		m_match = value_;
	}
	const CommonL2Attributes& getCommonL2Attributes() const
	{
		return m_commonL2Attributes;
	}
	void setCommonL2Attributes(const CommonL2Attributes& value_)
	{
		m_commonL2Attributes = value_;
	}
	const CommonIpAttributesP1& getCommonIpAttributesP1() const
	{
		return m_commonIpAttributesP1;
	}
	void setCommonIpAttributesP1(const CommonIpAttributesP1& value_)
	{
		m_commonIpAttributesP1 = value_;
	}
	const CommonPortAttributes& getCommonPortAttributes() const
	{
		return m_commonPortAttributes;
	}
	void setCommonPortAttributes(const CommonPortAttributes& value_)
	{
		m_commonPortAttributes = value_;
	}
	const boost::optional<VIpProtocolType >& getProtocol() const
	{
		return m_protocol;
	}
	void setProtocol(const boost::optional<VIpProtocolType >& value_)
	{
		m_protocol = value_;
	}
	const boost::optional<VSixbitrange >& getDscp() const
	{
		return m_dscp;
	}
	void setDscp(const boost::optional<VSixbitrange >& value_)
	{
		m_dscp = value_;
	}
	const boost::optional<PCommentType::value_type >& getComment() const
	{
		return m_comment;
	}
	void setComment(const boost::optional<PCommentType::value_type >& value_)
	{
		m_comment = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	boost::optional<EVirYesNo > m_match;
	CommonL2Attributes m_commonL2Attributes;
	CommonIpAttributesP1 m_commonIpAttributesP1;
	CommonPortAttributes m_commonPortAttributes;
	boost::optional<VIpProtocolType > m_protocol;
	boost::optional<VSixbitrange > m_dscp;
	boost::optional<PCommentType::value_type > m_comment;
};

} // namespace Xml
} // namespace Filter

///////////////////////////////////////////////////////////////////////////////
// struct CommonIpv6AttributesP1

namespace Filter
{
namespace Xml
{
struct CommonIpv6AttributesP1
{
	const boost::optional<VAddrIPv6 >& getSrcipaddr() const
	{
		return m_srcipaddr;
	}
	void setSrcipaddr(const boost::optional<VAddrIPv6 >& value_)
	{
		m_srcipaddr = value_;
	}
	const boost::optional<VAddrMaskv6 >& getSrcipmask() const
	{
		return m_srcipmask;
	}
	void setSrcipmask(const boost::optional<VAddrMaskv6 >& value_)
	{
		m_srcipmask = value_;
	}
	const boost::optional<VAddrIPv6 >& getDstipaddr() const
	{
		return m_dstipaddr;
	}
	void setDstipaddr(const boost::optional<VAddrIPv6 >& value_)
	{
		m_dstipaddr = value_;
	}
	const boost::optional<VAddrMaskv6 >& getDstipmask() const
	{
		return m_dstipmask;
	}
	void setDstipmask(const boost::optional<VAddrMaskv6 >& value_)
	{
		m_dstipmask = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;

private:
	boost::optional<VAddrIPv6 > m_srcipaddr;
	boost::optional<VAddrMaskv6 > m_srcipmask;
	boost::optional<VAddrIPv6 > m_dstipaddr;
	boost::optional<VAddrMaskv6 > m_dstipmask;
};

} // namespace Xml
} // namespace Filter

///////////////////////////////////////////////////////////////////////////////
// struct Ipv6

namespace Filter
{
namespace Xml
{
struct Ipv6
{
	const boost::optional<EVirYesNo >& getMatch() const
	{
		return m_match;
	}
	void setMatch(const boost::optional<EVirYesNo >& value_)
	{
		m_match = value_;
	}
	const CommonL2Attributes& getCommonL2Attributes() const
	{
		return m_commonL2Attributes;
	}
	void setCommonL2Attributes(const CommonL2Attributes& value_)
	{
		m_commonL2Attributes = value_;
	}
	const CommonIpv6AttributesP1& getCommonIpv6AttributesP1() const
	{
		return m_commonIpv6AttributesP1;
	}
	void setCommonIpv6AttributesP1(const CommonIpv6AttributesP1& value_)
	{
		m_commonIpv6AttributesP1 = value_;
	}
	const CommonPortAttributes& getCommonPortAttributes() const
	{
		return m_commonPortAttributes;
	}
	void setCommonPortAttributes(const CommonPortAttributes& value_)
	{
		m_commonPortAttributes = value_;
	}
	const boost::optional<VIpProtocolType >& getProtocol() const
	{
		return m_protocol;
	}
	void setProtocol(const boost::optional<VIpProtocolType >& value_)
	{
		m_protocol = value_;
	}
	const boost::optional<PCommentType::value_type >& getComment() const
	{
		return m_comment;
	}
	void setComment(const boost::optional<PCommentType::value_type >& value_)
	{
		m_comment = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	boost::optional<EVirYesNo > m_match;
	CommonL2Attributes m_commonL2Attributes;
	CommonIpv6AttributesP1 m_commonIpv6AttributesP1;
	CommonPortAttributes m_commonPortAttributes;
	boost::optional<VIpProtocolType > m_protocol;
	boost::optional<PCommentType::value_type > m_comment;
};

} // namespace Xml
} // namespace Filter

///////////////////////////////////////////////////////////////////////////////
// struct Anonymous5679

namespace Filter
{
namespace Xml
{
struct Anonymous5679
{
	const VIpsetNameType& getIpset() const
	{
		return m_ipset;
	}
	void setIpset(const VIpsetNameType& value_)
	{
		m_ipset = value_;
	}
	const PIpsetFlagsType::value_type& getIpsetflags() const
	{
		return m_ipsetflags;
	}
	void setIpsetflags(const PIpsetFlagsType::value_type& value_)
	{
		m_ipsetflags = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;

private:
	VIpsetNameType m_ipset;
	PIpsetFlagsType::value_type m_ipsetflags;
};

} // namespace Xml
} // namespace Filter

///////////////////////////////////////////////////////////////////////////////
// struct CommonIpAttributesP2

namespace Filter
{
namespace Xml
{
struct CommonIpAttributesP2
{
	const boost::optional<VAddrIP >& getSrcipfrom() const
	{
		return m_srcipfrom;
	}
	void setSrcipfrom(const boost::optional<VAddrIP >& value_)
	{
		m_srcipfrom = value_;
	}
	const boost::optional<VAddrIP >& getSrcipto() const
	{
		return m_srcipto;
	}
	void setSrcipto(const boost::optional<VAddrIP >& value_)
	{
		m_srcipto = value_;
	}
	const boost::optional<VAddrIP >& getDstipfrom() const
	{
		return m_dstipfrom;
	}
	void setDstipfrom(const boost::optional<VAddrIP >& value_)
	{
		m_dstipfrom = value_;
	}
	const boost::optional<VAddrIP >& getDstipto() const
	{
		return m_dstipto;
	}
	void setDstipto(const boost::optional<VAddrIP >& value_)
	{
		m_dstipto = value_;
	}
	const boost::optional<VSixbitrange >& getDscp() const
	{
		return m_dscp;
	}
	void setDscp(const boost::optional<VSixbitrange >& value_)
	{
		m_dscp = value_;
	}
	const boost::optional<VUint16range >& getConnlimitAbove() const
	{
		return m_connlimitAbove;
	}
	void setConnlimitAbove(const boost::optional<VUint16range >& value_)
	{
		m_connlimitAbove = value_;
	}
	const boost::optional<PStateflagsType::value_type >& getState() const
	{
		return m_state;
	}
	void setState(const boost::optional<PStateflagsType::value_type >& value_)
	{
		m_state = value_;
	}
	const boost::optional<Anonymous5679 >& getAnonymous5679() const
	{
		return m_anonymous5679;
	}
	void setAnonymous5679(const boost::optional<Anonymous5679 >& value_)
	{
		m_anonymous5679 = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;

private:
	boost::optional<VAddrIP > m_srcipfrom;
	boost::optional<VAddrIP > m_srcipto;
	boost::optional<VAddrIP > m_dstipfrom;
	boost::optional<VAddrIP > m_dstipto;
	boost::optional<VSixbitrange > m_dscp;
	boost::optional<VUint16range > m_connlimitAbove;
	boost::optional<PStateflagsType::value_type > m_state;
	boost::optional<Anonymous5679 > m_anonymous5679;
};

} // namespace Xml
} // namespace Filter

///////////////////////////////////////////////////////////////////////////////
// struct Tcp

namespace Filter
{
namespace Xml
{
struct Tcp
{
	const boost::optional<EVirYesNo >& getMatch() const
	{
		return m_match;
	}
	void setMatch(const boost::optional<EVirYesNo >& value_)
	{
		m_match = value_;
	}
	const boost::optional<VAddrMAC >& getSrcmacaddr() const
	{
		return m_srcmacaddr;
	}
	void setSrcmacaddr(const boost::optional<VAddrMAC >& value_)
	{
		m_srcmacaddr = value_;
	}
	const CommonPortAttributes& getCommonPortAttributes() const
	{
		return m_commonPortAttributes;
	}
	void setCommonPortAttributes(const CommonPortAttributes& value_)
	{
		m_commonPortAttributes = value_;
	}
	const CommonIpAttributesP1& getCommonIpAttributesP1() const
	{
		return m_commonIpAttributesP1;
	}
	void setCommonIpAttributesP1(const CommonIpAttributesP1& value_)
	{
		m_commonIpAttributesP1 = value_;
	}
	const CommonIpAttributesP2& getCommonIpAttributesP2() const
	{
		return m_commonIpAttributesP2;
	}
	void setCommonIpAttributesP2(const CommonIpAttributesP2& value_)
	{
		m_commonIpAttributesP2 = value_;
	}
	const boost::optional<PTcpflagsType::value_type >& getFlags() const
	{
		return m_flags;
	}
	void setFlags(const boost::optional<PTcpflagsType::value_type >& value_)
	{
		m_flags = value_;
	}
	const boost::optional<PCommentType::value_type >& getComment() const
	{
		return m_comment;
	}
	void setComment(const boost::optional<PCommentType::value_type >& value_)
	{
		m_comment = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	boost::optional<EVirYesNo > m_match;
	boost::optional<VAddrMAC > m_srcmacaddr;
	CommonPortAttributes m_commonPortAttributes;
	CommonIpAttributesP1 m_commonIpAttributesP1;
	CommonIpAttributesP2 m_commonIpAttributesP2;
	boost::optional<PTcpflagsType::value_type > m_flags;
	boost::optional<PCommentType::value_type > m_comment;
};

} // namespace Xml
} // namespace Filter

///////////////////////////////////////////////////////////////////////////////
// struct Udp

namespace Filter
{
namespace Xml
{
struct Udp
{
	const boost::optional<EVirYesNo >& getMatch() const
	{
		return m_match;
	}
	void setMatch(const boost::optional<EVirYesNo >& value_)
	{
		m_match = value_;
	}
	const boost::optional<VAddrMAC >& getSrcmacaddr() const
	{
		return m_srcmacaddr;
	}
	void setSrcmacaddr(const boost::optional<VAddrMAC >& value_)
	{
		m_srcmacaddr = value_;
	}
	const CommonPortAttributes& getCommonPortAttributes() const
	{
		return m_commonPortAttributes;
	}
	void setCommonPortAttributes(const CommonPortAttributes& value_)
	{
		m_commonPortAttributes = value_;
	}
	const CommonIpAttributesP1& getCommonIpAttributesP1() const
	{
		return m_commonIpAttributesP1;
	}
	void setCommonIpAttributesP1(const CommonIpAttributesP1& value_)
	{
		m_commonIpAttributesP1 = value_;
	}
	const CommonIpAttributesP2& getCommonIpAttributesP2() const
	{
		return m_commonIpAttributesP2;
	}
	void setCommonIpAttributesP2(const CommonIpAttributesP2& value_)
	{
		m_commonIpAttributesP2 = value_;
	}
	const boost::optional<PCommentType::value_type >& getComment() const
	{
		return m_comment;
	}
	void setComment(const boost::optional<PCommentType::value_type >& value_)
	{
		m_comment = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	boost::optional<EVirYesNo > m_match;
	boost::optional<VAddrMAC > m_srcmacaddr;
	CommonPortAttributes m_commonPortAttributes;
	CommonIpAttributesP1 m_commonIpAttributesP1;
	CommonIpAttributesP2 m_commonIpAttributesP2;
	boost::optional<PCommentType::value_type > m_comment;
};

} // namespace Xml
} // namespace Filter

///////////////////////////////////////////////////////////////////////////////
// struct Sctp

namespace Filter
{
namespace Xml
{
struct Sctp
{
	const boost::optional<EVirYesNo >& getMatch() const
	{
		return m_match;
	}
	void setMatch(const boost::optional<EVirYesNo >& value_)
	{
		m_match = value_;
	}
	const boost::optional<VAddrMAC >& getSrcmacaddr() const
	{
		return m_srcmacaddr;
	}
	void setSrcmacaddr(const boost::optional<VAddrMAC >& value_)
	{
		m_srcmacaddr = value_;
	}
	const CommonPortAttributes& getCommonPortAttributes() const
	{
		return m_commonPortAttributes;
	}
	void setCommonPortAttributes(const CommonPortAttributes& value_)
	{
		m_commonPortAttributes = value_;
	}
	const CommonIpAttributesP1& getCommonIpAttributesP1() const
	{
		return m_commonIpAttributesP1;
	}
	void setCommonIpAttributesP1(const CommonIpAttributesP1& value_)
	{
		m_commonIpAttributesP1 = value_;
	}
	const CommonIpAttributesP2& getCommonIpAttributesP2() const
	{
		return m_commonIpAttributesP2;
	}
	void setCommonIpAttributesP2(const CommonIpAttributesP2& value_)
	{
		m_commonIpAttributesP2 = value_;
	}
	const boost::optional<PCommentType::value_type >& getComment() const
	{
		return m_comment;
	}
	void setComment(const boost::optional<PCommentType::value_type >& value_)
	{
		m_comment = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	boost::optional<EVirYesNo > m_match;
	boost::optional<VAddrMAC > m_srcmacaddr;
	CommonPortAttributes m_commonPortAttributes;
	CommonIpAttributesP1 m_commonIpAttributesP1;
	CommonIpAttributesP2 m_commonIpAttributesP2;
	boost::optional<PCommentType::value_type > m_comment;
};

} // namespace Xml
} // namespace Filter

///////////////////////////////////////////////////////////////////////////////
// struct IcmpAttributes

namespace Filter
{
namespace Xml
{
struct IcmpAttributes
{
	const boost::optional<VUint8range >& getType() const
	{
		return m_type;
	}
	void setType(const boost::optional<VUint8range >& value_)
	{
		m_type = value_;
	}
	const boost::optional<VUint8range >& getCode() const
	{
		return m_code;
	}
	void setCode(const boost::optional<VUint8range >& value_)
	{
		m_code = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;

private:
	boost::optional<VUint8range > m_type;
	boost::optional<VUint8range > m_code;
};

} // namespace Xml
} // namespace Filter

///////////////////////////////////////////////////////////////////////////////
// struct Icmp

namespace Filter
{
namespace Xml
{
struct Icmp
{
	const boost::optional<EVirYesNo >& getMatch() const
	{
		return m_match;
	}
	void setMatch(const boost::optional<EVirYesNo >& value_)
	{
		m_match = value_;
	}
	const boost::optional<VAddrMAC >& getSrcmacaddr() const
	{
		return m_srcmacaddr;
	}
	void setSrcmacaddr(const boost::optional<VAddrMAC >& value_)
	{
		m_srcmacaddr = value_;
	}
	const CommonIpAttributesP1& getCommonIpAttributesP1() const
	{
		return m_commonIpAttributesP1;
	}
	void setCommonIpAttributesP1(const CommonIpAttributesP1& value_)
	{
		m_commonIpAttributesP1 = value_;
	}
	const CommonIpAttributesP2& getCommonIpAttributesP2() const
	{
		return m_commonIpAttributesP2;
	}
	void setCommonIpAttributesP2(const CommonIpAttributesP2& value_)
	{
		m_commonIpAttributesP2 = value_;
	}
	const IcmpAttributes& getIcmpAttributes() const
	{
		return m_icmpAttributes;
	}
	void setIcmpAttributes(const IcmpAttributes& value_)
	{
		m_icmpAttributes = value_;
	}
	const boost::optional<PCommentType::value_type >& getComment() const
	{
		return m_comment;
	}
	void setComment(const boost::optional<PCommentType::value_type >& value_)
	{
		m_comment = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	boost::optional<EVirYesNo > m_match;
	boost::optional<VAddrMAC > m_srcmacaddr;
	CommonIpAttributesP1 m_commonIpAttributesP1;
	CommonIpAttributesP2 m_commonIpAttributesP2;
	IcmpAttributes m_icmpAttributes;
	boost::optional<PCommentType::value_type > m_comment;
};

} // namespace Xml
} // namespace Filter

///////////////////////////////////////////////////////////////////////////////
// struct Igmp

namespace Filter
{
namespace Xml
{
struct Igmp
{
	const boost::optional<EVirYesNo >& getMatch() const
	{
		return m_match;
	}
	void setMatch(const boost::optional<EVirYesNo >& value_)
	{
		m_match = value_;
	}
	const boost::optional<VAddrMAC >& getSrcmacaddr() const
	{
		return m_srcmacaddr;
	}
	void setSrcmacaddr(const boost::optional<VAddrMAC >& value_)
	{
		m_srcmacaddr = value_;
	}
	const CommonIpAttributesP1& getCommonIpAttributesP1() const
	{
		return m_commonIpAttributesP1;
	}
	void setCommonIpAttributesP1(const CommonIpAttributesP1& value_)
	{
		m_commonIpAttributesP1 = value_;
	}
	const CommonIpAttributesP2& getCommonIpAttributesP2() const
	{
		return m_commonIpAttributesP2;
	}
	void setCommonIpAttributesP2(const CommonIpAttributesP2& value_)
	{
		m_commonIpAttributesP2 = value_;
	}
	const boost::optional<PCommentType::value_type >& getComment() const
	{
		return m_comment;
	}
	void setComment(const boost::optional<PCommentType::value_type >& value_)
	{
		m_comment = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	boost::optional<EVirYesNo > m_match;
	boost::optional<VAddrMAC > m_srcmacaddr;
	CommonIpAttributesP1 m_commonIpAttributesP1;
	CommonIpAttributesP2 m_commonIpAttributesP2;
	boost::optional<PCommentType::value_type > m_comment;
};

} // namespace Xml
} // namespace Filter

///////////////////////////////////////////////////////////////////////////////
// struct All

namespace Filter
{
namespace Xml
{
struct All
{
	const boost::optional<EVirYesNo >& getMatch() const
	{
		return m_match;
	}
	void setMatch(const boost::optional<EVirYesNo >& value_)
	{
		m_match = value_;
	}
	const boost::optional<VAddrMAC >& getSrcmacaddr() const
	{
		return m_srcmacaddr;
	}
	void setSrcmacaddr(const boost::optional<VAddrMAC >& value_)
	{
		m_srcmacaddr = value_;
	}
	const CommonIpAttributesP1& getCommonIpAttributesP1() const
	{
		return m_commonIpAttributesP1;
	}
	void setCommonIpAttributesP1(const CommonIpAttributesP1& value_)
	{
		m_commonIpAttributesP1 = value_;
	}
	const CommonIpAttributesP2& getCommonIpAttributesP2() const
	{
		return m_commonIpAttributesP2;
	}
	void setCommonIpAttributesP2(const CommonIpAttributesP2& value_)
	{
		m_commonIpAttributesP2 = value_;
	}
	const boost::optional<PCommentType::value_type >& getComment() const
	{
		return m_comment;
	}
	void setComment(const boost::optional<PCommentType::value_type >& value_)
	{
		m_comment = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	boost::optional<EVirYesNo > m_match;
	boost::optional<VAddrMAC > m_srcmacaddr;
	CommonIpAttributesP1 m_commonIpAttributesP1;
	CommonIpAttributesP2 m_commonIpAttributesP2;
	boost::optional<PCommentType::value_type > m_comment;
};

} // namespace Xml
} // namespace Filter

///////////////////////////////////////////////////////////////////////////////
// struct Esp

namespace Filter
{
namespace Xml
{
struct Esp
{
	const boost::optional<EVirYesNo >& getMatch() const
	{
		return m_match;
	}
	void setMatch(const boost::optional<EVirYesNo >& value_)
	{
		m_match = value_;
	}
	const boost::optional<VAddrMAC >& getSrcmacaddr() const
	{
		return m_srcmacaddr;
	}
	void setSrcmacaddr(const boost::optional<VAddrMAC >& value_)
	{
		m_srcmacaddr = value_;
	}
	const CommonIpAttributesP1& getCommonIpAttributesP1() const
	{
		return m_commonIpAttributesP1;
	}
	void setCommonIpAttributesP1(const CommonIpAttributesP1& value_)
	{
		m_commonIpAttributesP1 = value_;
	}
	const CommonIpAttributesP2& getCommonIpAttributesP2() const
	{
		return m_commonIpAttributesP2;
	}
	void setCommonIpAttributesP2(const CommonIpAttributesP2& value_)
	{
		m_commonIpAttributesP2 = value_;
	}
	const boost::optional<PCommentType::value_type >& getComment() const
	{
		return m_comment;
	}
	void setComment(const boost::optional<PCommentType::value_type >& value_)
	{
		m_comment = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	boost::optional<EVirYesNo > m_match;
	boost::optional<VAddrMAC > m_srcmacaddr;
	CommonIpAttributesP1 m_commonIpAttributesP1;
	CommonIpAttributesP2 m_commonIpAttributesP2;
	boost::optional<PCommentType::value_type > m_comment;
};

} // namespace Xml
} // namespace Filter

///////////////////////////////////////////////////////////////////////////////
// struct Ah

namespace Filter
{
namespace Xml
{
struct Ah
{
	const boost::optional<EVirYesNo >& getMatch() const
	{
		return m_match;
	}
	void setMatch(const boost::optional<EVirYesNo >& value_)
	{
		m_match = value_;
	}
	const boost::optional<VAddrMAC >& getSrcmacaddr() const
	{
		return m_srcmacaddr;
	}
	void setSrcmacaddr(const boost::optional<VAddrMAC >& value_)
	{
		m_srcmacaddr = value_;
	}
	const CommonIpAttributesP1& getCommonIpAttributesP1() const
	{
		return m_commonIpAttributesP1;
	}
	void setCommonIpAttributesP1(const CommonIpAttributesP1& value_)
	{
		m_commonIpAttributesP1 = value_;
	}
	const CommonIpAttributesP2& getCommonIpAttributesP2() const
	{
		return m_commonIpAttributesP2;
	}
	void setCommonIpAttributesP2(const CommonIpAttributesP2& value_)
	{
		m_commonIpAttributesP2 = value_;
	}
	const boost::optional<PCommentType::value_type >& getComment() const
	{
		return m_comment;
	}
	void setComment(const boost::optional<PCommentType::value_type >& value_)
	{
		m_comment = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	boost::optional<EVirYesNo > m_match;
	boost::optional<VAddrMAC > m_srcmacaddr;
	CommonIpAttributesP1 m_commonIpAttributesP1;
	CommonIpAttributesP2 m_commonIpAttributesP2;
	boost::optional<PCommentType::value_type > m_comment;
};

} // namespace Xml
} // namespace Filter

///////////////////////////////////////////////////////////////////////////////
// struct Udplite

namespace Filter
{
namespace Xml
{
struct Udplite
{
	const boost::optional<EVirYesNo >& getMatch() const
	{
		return m_match;
	}
	void setMatch(const boost::optional<EVirYesNo >& value_)
	{
		m_match = value_;
	}
	const boost::optional<VAddrMAC >& getSrcmacaddr() const
	{
		return m_srcmacaddr;
	}
	void setSrcmacaddr(const boost::optional<VAddrMAC >& value_)
	{
		m_srcmacaddr = value_;
	}
	const CommonIpAttributesP1& getCommonIpAttributesP1() const
	{
		return m_commonIpAttributesP1;
	}
	void setCommonIpAttributesP1(const CommonIpAttributesP1& value_)
	{
		m_commonIpAttributesP1 = value_;
	}
	const CommonIpAttributesP2& getCommonIpAttributesP2() const
	{
		return m_commonIpAttributesP2;
	}
	void setCommonIpAttributesP2(const CommonIpAttributesP2& value_)
	{
		m_commonIpAttributesP2 = value_;
	}
	const boost::optional<PCommentType::value_type >& getComment() const
	{
		return m_comment;
	}
	void setComment(const boost::optional<PCommentType::value_type >& value_)
	{
		m_comment = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	boost::optional<EVirYesNo > m_match;
	boost::optional<VAddrMAC > m_srcmacaddr;
	CommonIpAttributesP1 m_commonIpAttributesP1;
	CommonIpAttributesP2 m_commonIpAttributesP2;
	boost::optional<PCommentType::value_type > m_comment;
};

} // namespace Xml
} // namespace Filter

///////////////////////////////////////////////////////////////////////////////
// struct CommonIpv6AttributesP2

namespace Filter
{
namespace Xml
{
struct CommonIpv6AttributesP2
{
	const boost::optional<VAddrIPv6 >& getSrcipfrom() const
	{
		return m_srcipfrom;
	}
	void setSrcipfrom(const boost::optional<VAddrIPv6 >& value_)
	{
		m_srcipfrom = value_;
	}
	const boost::optional<VAddrIPv6 >& getSrcipto() const
	{
		return m_srcipto;
	}
	void setSrcipto(const boost::optional<VAddrIPv6 >& value_)
	{
		m_srcipto = value_;
	}
	const boost::optional<VAddrIPv6 >& getDstipfrom() const
	{
		return m_dstipfrom;
	}
	void setDstipfrom(const boost::optional<VAddrIPv6 >& value_)
	{
		m_dstipfrom = value_;
	}
	const boost::optional<VAddrIPv6 >& getDstipto() const
	{
		return m_dstipto;
	}
	void setDstipto(const boost::optional<VAddrIPv6 >& value_)
	{
		m_dstipto = value_;
	}
	const boost::optional<VSixbitrange >& getDscp() const
	{
		return m_dscp;
	}
	void setDscp(const boost::optional<VSixbitrange >& value_)
	{
		m_dscp = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;

private:
	boost::optional<VAddrIPv6 > m_srcipfrom;
	boost::optional<VAddrIPv6 > m_srcipto;
	boost::optional<VAddrIPv6 > m_dstipfrom;
	boost::optional<VAddrIPv6 > m_dstipto;
	boost::optional<VSixbitrange > m_dscp;
};

} // namespace Xml
} // namespace Filter

///////////////////////////////////////////////////////////////////////////////
// struct TcpIpv6

namespace Filter
{
namespace Xml
{
struct TcpIpv6
{
	const boost::optional<EVirYesNo >& getMatch() const
	{
		return m_match;
	}
	void setMatch(const boost::optional<EVirYesNo >& value_)
	{
		m_match = value_;
	}
	const boost::optional<VAddrMAC >& getSrcmacaddr() const
	{
		return m_srcmacaddr;
	}
	void setSrcmacaddr(const boost::optional<VAddrMAC >& value_)
	{
		m_srcmacaddr = value_;
	}
	const CommonPortAttributes& getCommonPortAttributes() const
	{
		return m_commonPortAttributes;
	}
	void setCommonPortAttributes(const CommonPortAttributes& value_)
	{
		m_commonPortAttributes = value_;
	}
	const CommonIpv6AttributesP1& getCommonIpv6AttributesP1() const
	{
		return m_commonIpv6AttributesP1;
	}
	void setCommonIpv6AttributesP1(const CommonIpv6AttributesP1& value_)
	{
		m_commonIpv6AttributesP1 = value_;
	}
	const CommonIpv6AttributesP2& getCommonIpv6AttributesP2() const
	{
		return m_commonIpv6AttributesP2;
	}
	void setCommonIpv6AttributesP2(const CommonIpv6AttributesP2& value_)
	{
		m_commonIpv6AttributesP2 = value_;
	}
	const boost::optional<PTcpflagsType::value_type >& getFlags() const
	{
		return m_flags;
	}
	void setFlags(const boost::optional<PTcpflagsType::value_type >& value_)
	{
		m_flags = value_;
	}
	const boost::optional<PCommentType::value_type >& getComment() const
	{
		return m_comment;
	}
	void setComment(const boost::optional<PCommentType::value_type >& value_)
	{
		m_comment = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	boost::optional<EVirYesNo > m_match;
	boost::optional<VAddrMAC > m_srcmacaddr;
	CommonPortAttributes m_commonPortAttributes;
	CommonIpv6AttributesP1 m_commonIpv6AttributesP1;
	CommonIpv6AttributesP2 m_commonIpv6AttributesP2;
	boost::optional<PTcpflagsType::value_type > m_flags;
	boost::optional<PCommentType::value_type > m_comment;
};

} // namespace Xml
} // namespace Filter

///////////////////////////////////////////////////////////////////////////////
// struct UdpIpv6

namespace Filter
{
namespace Xml
{
struct UdpIpv6
{
	const boost::optional<EVirYesNo >& getMatch() const
	{
		return m_match;
	}
	void setMatch(const boost::optional<EVirYesNo >& value_)
	{
		m_match = value_;
	}
	const boost::optional<VAddrMAC >& getSrcmacaddr() const
	{
		return m_srcmacaddr;
	}
	void setSrcmacaddr(const boost::optional<VAddrMAC >& value_)
	{
		m_srcmacaddr = value_;
	}
	const CommonPortAttributes& getCommonPortAttributes() const
	{
		return m_commonPortAttributes;
	}
	void setCommonPortAttributes(const CommonPortAttributes& value_)
	{
		m_commonPortAttributes = value_;
	}
	const CommonIpv6AttributesP1& getCommonIpv6AttributesP1() const
	{
		return m_commonIpv6AttributesP1;
	}
	void setCommonIpv6AttributesP1(const CommonIpv6AttributesP1& value_)
	{
		m_commonIpv6AttributesP1 = value_;
	}
	const CommonIpv6AttributesP2& getCommonIpv6AttributesP2() const
	{
		return m_commonIpv6AttributesP2;
	}
	void setCommonIpv6AttributesP2(const CommonIpv6AttributesP2& value_)
	{
		m_commonIpv6AttributesP2 = value_;
	}
	const boost::optional<PCommentType::value_type >& getComment() const
	{
		return m_comment;
	}
	void setComment(const boost::optional<PCommentType::value_type >& value_)
	{
		m_comment = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	boost::optional<EVirYesNo > m_match;
	boost::optional<VAddrMAC > m_srcmacaddr;
	CommonPortAttributes m_commonPortAttributes;
	CommonIpv6AttributesP1 m_commonIpv6AttributesP1;
	CommonIpv6AttributesP2 m_commonIpv6AttributesP2;
	boost::optional<PCommentType::value_type > m_comment;
};

} // namespace Xml
} // namespace Filter

///////////////////////////////////////////////////////////////////////////////
// struct SctpIpv6

namespace Filter
{
namespace Xml
{
struct SctpIpv6
{
	const boost::optional<EVirYesNo >& getMatch() const
	{
		return m_match;
	}
	void setMatch(const boost::optional<EVirYesNo >& value_)
	{
		m_match = value_;
	}
	const boost::optional<VAddrMAC >& getSrcmacaddr() const
	{
		return m_srcmacaddr;
	}
	void setSrcmacaddr(const boost::optional<VAddrMAC >& value_)
	{
		m_srcmacaddr = value_;
	}
	const CommonPortAttributes& getCommonPortAttributes() const
	{
		return m_commonPortAttributes;
	}
	void setCommonPortAttributes(const CommonPortAttributes& value_)
	{
		m_commonPortAttributes = value_;
	}
	const CommonIpv6AttributesP1& getCommonIpv6AttributesP1() const
	{
		return m_commonIpv6AttributesP1;
	}
	void setCommonIpv6AttributesP1(const CommonIpv6AttributesP1& value_)
	{
		m_commonIpv6AttributesP1 = value_;
	}
	const CommonIpv6AttributesP2& getCommonIpv6AttributesP2() const
	{
		return m_commonIpv6AttributesP2;
	}
	void setCommonIpv6AttributesP2(const CommonIpv6AttributesP2& value_)
	{
		m_commonIpv6AttributesP2 = value_;
	}
	const boost::optional<PCommentType::value_type >& getComment() const
	{
		return m_comment;
	}
	void setComment(const boost::optional<PCommentType::value_type >& value_)
	{
		m_comment = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	boost::optional<EVirYesNo > m_match;
	boost::optional<VAddrMAC > m_srcmacaddr;
	CommonPortAttributes m_commonPortAttributes;
	CommonIpv6AttributesP1 m_commonIpv6AttributesP1;
	CommonIpv6AttributesP2 m_commonIpv6AttributesP2;
	boost::optional<PCommentType::value_type > m_comment;
};

} // namespace Xml
} // namespace Filter

///////////////////////////////////////////////////////////////////////////////
// struct Icmpv6

namespace Filter
{
namespace Xml
{
struct Icmpv6
{
	const boost::optional<EVirYesNo >& getMatch() const
	{
		return m_match;
	}
	void setMatch(const boost::optional<EVirYesNo >& value_)
	{
		m_match = value_;
	}
	const boost::optional<VAddrMAC >& getSrcmacaddr() const
	{
		return m_srcmacaddr;
	}
	void setSrcmacaddr(const boost::optional<VAddrMAC >& value_)
	{
		m_srcmacaddr = value_;
	}
	const CommonIpv6AttributesP1& getCommonIpv6AttributesP1() const
	{
		return m_commonIpv6AttributesP1;
	}
	void setCommonIpv6AttributesP1(const CommonIpv6AttributesP1& value_)
	{
		m_commonIpv6AttributesP1 = value_;
	}
	const CommonIpv6AttributesP2& getCommonIpv6AttributesP2() const
	{
		return m_commonIpv6AttributesP2;
	}
	void setCommonIpv6AttributesP2(const CommonIpv6AttributesP2& value_)
	{
		m_commonIpv6AttributesP2 = value_;
	}
	const IcmpAttributes& getIcmpAttributes() const
	{
		return m_icmpAttributes;
	}
	void setIcmpAttributes(const IcmpAttributes& value_)
	{
		m_icmpAttributes = value_;
	}
	const boost::optional<PCommentType::value_type >& getComment() const
	{
		return m_comment;
	}
	void setComment(const boost::optional<PCommentType::value_type >& value_)
	{
		m_comment = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	boost::optional<EVirYesNo > m_match;
	boost::optional<VAddrMAC > m_srcmacaddr;
	CommonIpv6AttributesP1 m_commonIpv6AttributesP1;
	CommonIpv6AttributesP2 m_commonIpv6AttributesP2;
	IcmpAttributes m_icmpAttributes;
	boost::optional<PCommentType::value_type > m_comment;
};

} // namespace Xml
} // namespace Filter

///////////////////////////////////////////////////////////////////////////////
// struct AllIpv6

namespace Filter
{
namespace Xml
{
struct AllIpv6
{
	const boost::optional<EVirYesNo >& getMatch() const
	{
		return m_match;
	}
	void setMatch(const boost::optional<EVirYesNo >& value_)
	{
		m_match = value_;
	}
	const boost::optional<VAddrMAC >& getSrcmacaddr() const
	{
		return m_srcmacaddr;
	}
	void setSrcmacaddr(const boost::optional<VAddrMAC >& value_)
	{
		m_srcmacaddr = value_;
	}
	const CommonIpv6AttributesP1& getCommonIpv6AttributesP1() const
	{
		return m_commonIpv6AttributesP1;
	}
	void setCommonIpv6AttributesP1(const CommonIpv6AttributesP1& value_)
	{
		m_commonIpv6AttributesP1 = value_;
	}
	const CommonIpv6AttributesP2& getCommonIpv6AttributesP2() const
	{
		return m_commonIpv6AttributesP2;
	}
	void setCommonIpv6AttributesP2(const CommonIpv6AttributesP2& value_)
	{
		m_commonIpv6AttributesP2 = value_;
	}
	const boost::optional<PCommentType::value_type >& getComment() const
	{
		return m_comment;
	}
	void setComment(const boost::optional<PCommentType::value_type >& value_)
	{
		m_comment = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	boost::optional<EVirYesNo > m_match;
	boost::optional<VAddrMAC > m_srcmacaddr;
	CommonIpv6AttributesP1 m_commonIpv6AttributesP1;
	CommonIpv6AttributesP2 m_commonIpv6AttributesP2;
	boost::optional<PCommentType::value_type > m_comment;
};

} // namespace Xml
} // namespace Filter

///////////////////////////////////////////////////////////////////////////////
// struct EspIpv6

namespace Filter
{
namespace Xml
{
struct EspIpv6
{
	const boost::optional<EVirYesNo >& getMatch() const
	{
		return m_match;
	}
	void setMatch(const boost::optional<EVirYesNo >& value_)
	{
		m_match = value_;
	}
	const boost::optional<VAddrMAC >& getSrcmacaddr() const
	{
		return m_srcmacaddr;
	}
	void setSrcmacaddr(const boost::optional<VAddrMAC >& value_)
	{
		m_srcmacaddr = value_;
	}
	const CommonIpv6AttributesP1& getCommonIpv6AttributesP1() const
	{
		return m_commonIpv6AttributesP1;
	}
	void setCommonIpv6AttributesP1(const CommonIpv6AttributesP1& value_)
	{
		m_commonIpv6AttributesP1 = value_;
	}
	const CommonIpv6AttributesP2& getCommonIpv6AttributesP2() const
	{
		return m_commonIpv6AttributesP2;
	}
	void setCommonIpv6AttributesP2(const CommonIpv6AttributesP2& value_)
	{
		m_commonIpv6AttributesP2 = value_;
	}
	const boost::optional<PCommentType::value_type >& getComment() const
	{
		return m_comment;
	}
	void setComment(const boost::optional<PCommentType::value_type >& value_)
	{
		m_comment = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	boost::optional<EVirYesNo > m_match;
	boost::optional<VAddrMAC > m_srcmacaddr;
	CommonIpv6AttributesP1 m_commonIpv6AttributesP1;
	CommonIpv6AttributesP2 m_commonIpv6AttributesP2;
	boost::optional<PCommentType::value_type > m_comment;
};

} // namespace Xml
} // namespace Filter

///////////////////////////////////////////////////////////////////////////////
// struct AhIpv6

namespace Filter
{
namespace Xml
{
struct AhIpv6
{
	const boost::optional<EVirYesNo >& getMatch() const
	{
		return m_match;
	}
	void setMatch(const boost::optional<EVirYesNo >& value_)
	{
		m_match = value_;
	}
	const boost::optional<VAddrMAC >& getSrcmacaddr() const
	{
		return m_srcmacaddr;
	}
	void setSrcmacaddr(const boost::optional<VAddrMAC >& value_)
	{
		m_srcmacaddr = value_;
	}
	const CommonIpv6AttributesP1& getCommonIpv6AttributesP1() const
	{
		return m_commonIpv6AttributesP1;
	}
	void setCommonIpv6AttributesP1(const CommonIpv6AttributesP1& value_)
	{
		m_commonIpv6AttributesP1 = value_;
	}
	const CommonIpv6AttributesP2& getCommonIpv6AttributesP2() const
	{
		return m_commonIpv6AttributesP2;
	}
	void setCommonIpv6AttributesP2(const CommonIpv6AttributesP2& value_)
	{
		m_commonIpv6AttributesP2 = value_;
	}
	const boost::optional<PCommentType::value_type >& getComment() const
	{
		return m_comment;
	}
	void setComment(const boost::optional<PCommentType::value_type >& value_)
	{
		m_comment = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	boost::optional<EVirYesNo > m_match;
	boost::optional<VAddrMAC > m_srcmacaddr;
	CommonIpv6AttributesP1 m_commonIpv6AttributesP1;
	CommonIpv6AttributesP2 m_commonIpv6AttributesP2;
	boost::optional<PCommentType::value_type > m_comment;
};

} // namespace Xml
} // namespace Filter

///////////////////////////////////////////////////////////////////////////////
// struct UdpliteIpv6

namespace Filter
{
namespace Xml
{
struct UdpliteIpv6
{
	const boost::optional<EVirYesNo >& getMatch() const
	{
		return m_match;
	}
	void setMatch(const boost::optional<EVirYesNo >& value_)
	{
		m_match = value_;
	}
	const boost::optional<VAddrMAC >& getSrcmacaddr() const
	{
		return m_srcmacaddr;
	}
	void setSrcmacaddr(const boost::optional<VAddrMAC >& value_)
	{
		m_srcmacaddr = value_;
	}
	const CommonIpv6AttributesP1& getCommonIpv6AttributesP1() const
	{
		return m_commonIpv6AttributesP1;
	}
	void setCommonIpv6AttributesP1(const CommonIpv6AttributesP1& value_)
	{
		m_commonIpv6AttributesP1 = value_;
	}
	const CommonIpv6AttributesP2& getCommonIpv6AttributesP2() const
	{
		return m_commonIpv6AttributesP2;
	}
	void setCommonIpv6AttributesP2(const CommonIpv6AttributesP2& value_)
	{
		m_commonIpv6AttributesP2 = value_;
	}
	const boost::optional<PCommentType::value_type >& getComment() const
	{
		return m_comment;
	}
	void setComment(const boost::optional<PCommentType::value_type >& value_)
	{
		m_comment = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	boost::optional<EVirYesNo > m_match;
	boost::optional<VAddrMAC > m_srcmacaddr;
	CommonIpv6AttributesP1 m_commonIpv6AttributesP1;
	CommonIpv6AttributesP2 m_commonIpv6AttributesP2;
	boost::optional<PCommentType::value_type > m_comment;
};

} // namespace Xml
} // namespace Filter

///////////////////////////////////////////////////////////////////////////////
// struct Rule

namespace Filter
{
namespace Xml
{
struct Rule
{
	const RuleNodeAttributes& getRuleNodeAttributes() const
	{
		return m_ruleNodeAttributes;
	}
	void setRuleNodeAttributes(const RuleNodeAttributes& value_)
	{
		m_ruleNodeAttributes = value_;
	}
	const boost::optional<QList<Mac > >& getMacList() const
	{
		return m_macList;
	}
	void setMacList(const boost::optional<QList<Mac > >& value_)
	{
		m_macList = value_;
	}
	const boost::optional<QList<Vlan > >& getVlanList() const
	{
		return m_vlanList;
	}
	void setVlanList(const boost::optional<QList<Vlan > >& value_)
	{
		m_vlanList = value_;
	}
	const boost::optional<QList<Stp > >& getStpList() const
	{
		return m_stpList;
	}
	void setStpList(const boost::optional<QList<Stp > >& value_)
	{
		m_stpList = value_;
	}
	const boost::optional<QList<Arp > >& getArpList() const
	{
		return m_arpList;
	}
	void setArpList(const boost::optional<QList<Arp > >& value_)
	{
		m_arpList = value_;
	}
	const boost::optional<QList<Rarp > >& getRarpList() const
	{
		return m_rarpList;
	}
	void setRarpList(const boost::optional<QList<Rarp > >& value_)
	{
		m_rarpList = value_;
	}
	const boost::optional<QList<Ip > >& getIpList() const
	{
		return m_ipList;
	}
	void setIpList(const boost::optional<QList<Ip > >& value_)
	{
		m_ipList = value_;
	}
	const boost::optional<QList<Ipv6 > >& getIpv6List() const
	{
		return m_ipv6List;
	}
	void setIpv6List(const boost::optional<QList<Ipv6 > >& value_)
	{
		m_ipv6List = value_;
	}
	const boost::optional<QList<Tcp > >& getTcpList() const
	{
		return m_tcpList;
	}
	void setTcpList(const boost::optional<QList<Tcp > >& value_)
	{
		m_tcpList = value_;
	}
	const boost::optional<QList<Udp > >& getUdpList() const
	{
		return m_udpList;
	}
	void setUdpList(const boost::optional<QList<Udp > >& value_)
	{
		m_udpList = value_;
	}
	const boost::optional<QList<Sctp > >& getSctpList() const
	{
		return m_sctpList;
	}
	void setSctpList(const boost::optional<QList<Sctp > >& value_)
	{
		m_sctpList = value_;
	}
	const boost::optional<QList<Icmp > >& getIcmpList() const
	{
		return m_icmpList;
	}
	void setIcmpList(const boost::optional<QList<Icmp > >& value_)
	{
		m_icmpList = value_;
	}
	const boost::optional<QList<Igmp > >& getIgmpList() const
	{
		return m_igmpList;
	}
	void setIgmpList(const boost::optional<QList<Igmp > >& value_)
	{
		m_igmpList = value_;
	}
	const boost::optional<QList<All > >& getAllList() const
	{
		return m_allList;
	}
	void setAllList(const boost::optional<QList<All > >& value_)
	{
		m_allList = value_;
	}
	const boost::optional<QList<Esp > >& getEspList() const
	{
		return m_espList;
	}
	void setEspList(const boost::optional<QList<Esp > >& value_)
	{
		m_espList = value_;
	}
	const boost::optional<QList<Ah > >& getAhList() const
	{
		return m_ahList;
	}
	void setAhList(const boost::optional<QList<Ah > >& value_)
	{
		m_ahList = value_;
	}
	const boost::optional<QList<Udplite > >& getUdpliteList() const
	{
		return m_udpliteList;
	}
	void setUdpliteList(const boost::optional<QList<Udplite > >& value_)
	{
		m_udpliteList = value_;
	}
	const boost::optional<QList<TcpIpv6 > >& getTcpIpv6List() const
	{
		return m_tcpIpv6List;
	}
	void setTcpIpv6List(const boost::optional<QList<TcpIpv6 > >& value_)
	{
		m_tcpIpv6List = value_;
	}
	const boost::optional<QList<UdpIpv6 > >& getUdpIpv6List() const
	{
		return m_udpIpv6List;
	}
	void setUdpIpv6List(const boost::optional<QList<UdpIpv6 > >& value_)
	{
		m_udpIpv6List = value_;
	}
	const boost::optional<QList<SctpIpv6 > >& getSctpIpv6List() const
	{
		return m_sctpIpv6List;
	}
	void setSctpIpv6List(const boost::optional<QList<SctpIpv6 > >& value_)
	{
		m_sctpIpv6List = value_;
	}
	const boost::optional<QList<Icmpv6 > >& getIcmpv6List() const
	{
		return m_icmpv6List;
	}
	void setIcmpv6List(const boost::optional<QList<Icmpv6 > >& value_)
	{
		m_icmpv6List = value_;
	}
	const boost::optional<QList<AllIpv6 > >& getAllIpv6List() const
	{
		return m_allIpv6List;
	}
	void setAllIpv6List(const boost::optional<QList<AllIpv6 > >& value_)
	{
		m_allIpv6List = value_;
	}
	const boost::optional<QList<EspIpv6 > >& getEspIpv6List() const
	{
		return m_espIpv6List;
	}
	void setEspIpv6List(const boost::optional<QList<EspIpv6 > >& value_)
	{
		m_espIpv6List = value_;
	}
	const boost::optional<QList<AhIpv6 > >& getAhIpv6List() const
	{
		return m_ahIpv6List;
	}
	void setAhIpv6List(const boost::optional<QList<AhIpv6 > >& value_)
	{
		m_ahIpv6List = value_;
	}
	const boost::optional<QList<UdpliteIpv6 > >& getUdpliteIpv6List() const
	{
		return m_udpliteIpv6List;
	}
	void setUdpliteIpv6List(const boost::optional<QList<UdpliteIpv6 > >& value_)
	{
		m_udpliteIpv6List = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	RuleNodeAttributes m_ruleNodeAttributes;
	boost::optional<QList<Mac > > m_macList;
	boost::optional<QList<Vlan > > m_vlanList;
	boost::optional<QList<Stp > > m_stpList;
	boost::optional<QList<Arp > > m_arpList;
	boost::optional<QList<Rarp > > m_rarpList;
	boost::optional<QList<Ip > > m_ipList;
	boost::optional<QList<Ipv6 > > m_ipv6List;
	boost::optional<QList<Tcp > > m_tcpList;
	boost::optional<QList<Udp > > m_udpList;
	boost::optional<QList<Sctp > > m_sctpList;
	boost::optional<QList<Icmp > > m_icmpList;
	boost::optional<QList<Igmp > > m_igmpList;
	boost::optional<QList<All > > m_allList;
	boost::optional<QList<Esp > > m_espList;
	boost::optional<QList<Ah > > m_ahList;
	boost::optional<QList<Udplite > > m_udpliteList;
	boost::optional<QList<TcpIpv6 > > m_tcpIpv6List;
	boost::optional<QList<UdpIpv6 > > m_udpIpv6List;
	boost::optional<QList<SctpIpv6 > > m_sctpIpv6List;
	boost::optional<QList<Icmpv6 > > m_icmpv6List;
	boost::optional<QList<AllIpv6 > > m_allIpv6List;
	boost::optional<QList<EspIpv6 > > m_espIpv6List;
	boost::optional<QList<AhIpv6 > > m_ahIpv6List;
	boost::optional<QList<UdpliteIpv6 > > m_udpliteIpv6List;
};

} // namespace Xml
} // namespace Filter

///////////////////////////////////////////////////////////////////////////////
// struct VChoice5120

namespace Filter
{
namespace Xml
{
typedef Choice<mpl::vector<Element<Filter::Xml::FilterrefNodeAttributes, Name::Strict<706> >, Element<Filter::Xml::Rule, Name::Strict<5252> > > > VChoice5120Impl;
typedef VChoice5120Impl::value_type VChoice5120;

} // namespace Xml
} // namespace Filter

///////////////////////////////////////////////////////////////////////////////
// struct Filter

namespace Filter
{
namespace Xml
{
struct Filter
{
	const FilterNodeAttributes& getFilterNodeAttributes() const
	{
		return m_filterNodeAttributes;
	}
	void setFilterNodeAttributes(const FilterNodeAttributes& value_)
	{
		m_filterNodeAttributes = value_;
	}
	const boost::optional<VUUID >& getUuid() const
	{
		return m_uuid;
	}
	void setUuid(const boost::optional<VUUID >& value_)
	{
		m_uuid = value_;
	}
	const QList<VChoice5120 >& getChoice5120List() const
	{
		return m_choice5120List;
	}
	void setChoice5120List(const QList<VChoice5120 >& value_)
	{
		m_choice5120List = value_;
	}
	bool load(const QDomElement& );
	bool save(QDomElement& ) const;
	bool save(QDomDocument& ) const;

private:
	FilterNodeAttributes m_filterNodeAttributes;
	boost::optional<VUUID > m_uuid;
	QList<VChoice5120 > m_choice5120List;
};

} // namespace Xml
} // namespace Filter

///////////////////////////////////////////////////////////////////////////////
// struct FilterNodeAttributes traits

template<>
struct Traits<Filter::Xml::FilterNodeAttributes>
{
	typedef Ordered<mpl::vector<Attribute<Filter::Xml::PName, Name::Strict<107> >, Optional<Attribute<Filter::Xml::VChain, Name::Strict<5288> > >, Optional<Attribute<Filter::Xml::PPriorityType, Name::Strict<1243> > > > > marshal_type;

	static int parse(Filter::Xml::FilterNodeAttributes& , QStack<QDomElement>& );
	static int generate(const Filter::Xml::FilterNodeAttributes& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Parameter traits

template<>
struct Traits<Filter::Xml::Parameter>
{
	typedef Ordered<mpl::vector<Attribute<Filter::Xml::PFilterParamName, Name::Strict<107> >, Attribute<Filter::Xml::PFilterParamValue, Name::Strict<1086> > > > marshal_type;

	static int parse(Filter::Xml::Parameter& , QStack<QDomElement>& );
	static int generate(const Filter::Xml::Parameter& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct FilterrefNodeAttributes traits

template<>
struct Traits<Filter::Xml::FilterrefNodeAttributes>
{
	typedef Ordered<mpl::vector<Attribute<Filter::Xml::PFilter, Name::Strict<764> >, ZeroOrMore<Element<Filter::Xml::Parameter, Name::Strict<1084> > > > > marshal_type;

	static int parse(Filter::Xml::FilterrefNodeAttributes& , QStack<QDomElement>& );
	static int generate(const Filter::Xml::FilterrefNodeAttributes& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct RuleNodeAttributes traits

template<>
struct Traits<Filter::Xml::RuleNodeAttributes>
{
	typedef Ordered<mpl::vector<Attribute<Filter::Xml::EActionType, Name::Strict<883> >, Attribute<Filter::Xml::EDirectionType, Name::Strict<5305> >, Optional<Attribute<Filter::Xml::PPriorityType, Name::Strict<1243> > >, Optional<Attribute<Filter::Xml::PStatematchType, Name::Strict<5307> > > > > marshal_type;

	static int parse(Filter::Xml::RuleNodeAttributes& , QStack<QDomElement>& );
	static int generate(const Filter::Xml::RuleNodeAttributes& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct CommonL2Attributes traits

template<>
struct Traits<Filter::Xml::CommonL2Attributes>
{
	typedef Unordered<mpl::vector<Optional<Attribute<Filter::Xml::VAddrMAC, Name::Strict<5309> > >, Optional<Attribute<Filter::Xml::VAddrMAC, Name::Strict<5311> > >, Optional<Attribute<Filter::Xml::VAddrMAC, Name::Strict<5312> > >, Optional<Attribute<Filter::Xml::VAddrMAC, Name::Strict<5313> > > > > marshal_type;

	static int parse(Filter::Xml::CommonL2Attributes& , QStack<QDomElement>& );
	static int generate(const Filter::Xml::CommonL2Attributes& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Mac traits

template<>
struct Traits<Filter::Xml::Mac>
{
	typedef Ordered<mpl::vector<Optional<Attribute<Filter::Xml::EVirYesNo, Name::Strict<1015> > >, Fragment<Filter::Xml::CommonL2Attributes >, Optional<Attribute<Filter::Xml::VMacProtocolid, Name::Strict<5340> > >, Optional<Attribute<Filter::Xml::PCommentType, Name::Strict<5377> > > > > marshal_type;

	static int parse(Filter::Xml::Mac& , QStack<QDomElement>& );
	static int generate(const Filter::Xml::Mac& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct VlanAttributes traits

template<>
struct Traits<Filter::Xml::VlanAttributes>
{
	typedef Unordered<mpl::vector<Optional<Attribute<Filter::Xml::VVlanVlanid, Name::Strict<5342> > >, Optional<Attribute<Filter::Xml::VMacProtocolid, Name::Strict<5344> > > > > marshal_type;

	static int parse(Filter::Xml::VlanAttributes& , QStack<QDomElement>& );
	static int generate(const Filter::Xml::VlanAttributes& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Vlan traits

template<>
struct Traits<Filter::Xml::Vlan>
{
	typedef Ordered<mpl::vector<Optional<Attribute<Filter::Xml::EVirYesNo, Name::Strict<1015> > >, Fragment<Filter::Xml::CommonL2Attributes >, Fragment<Filter::Xml::VlanAttributes >, Optional<Attribute<Filter::Xml::PCommentType, Name::Strict<5377> > > > > marshal_type;

	static int parse(Filter::Xml::Vlan& , QStack<QDomElement>& );
	static int generate(const Filter::Xml::Vlan& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct SrcmacandmaskAttributes traits

template<>
struct Traits<Filter::Xml::SrcmacandmaskAttributes>
{
	typedef Unordered<mpl::vector<Optional<Attribute<Filter::Xml::VAddrMAC, Name::Strict<5309> > >, Optional<Attribute<Filter::Xml::VAddrMAC, Name::Strict<5311> > > > > marshal_type;

	static int parse(Filter::Xml::SrcmacandmaskAttributes& , QStack<QDomElement>& );
	static int generate(const Filter::Xml::SrcmacandmaskAttributes& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct StpAttributes traits

template<>
struct Traits<Filter::Xml::StpAttributes>
{
	typedef Ordered<mpl::vector<Optional<Attribute<Filter::Xml::VUint8range, Name::Strict<105> > >, Optional<Attribute<Filter::Xml::VUint8range, Name::Strict<5345> > >, Optional<Attribute<Filter::Xml::VUint16range, Name::Strict<5346> > >, Optional<Attribute<Filter::Xml::VUint16range, Name::Strict<5347> > >, Optional<Attribute<Filter::Xml::VAddrMAC, Name::Strict<5348> > >, Optional<Attribute<Filter::Xml::VAddrMAC, Name::Strict<5349> > >, Optional<Attribute<Filter::Xml::VUint32range, Name::Strict<5350> > >, Optional<Attribute<Filter::Xml::VUint32range, Name::Strict<5352> > >, Optional<Attribute<Filter::Xml::VUint16range, Name::Strict<5353> > >, Optional<Attribute<Filter::Xml::VUint16range, Name::Strict<5354> > >, Optional<Attribute<Filter::Xml::VAddrMAC, Name::Strict<5355> > >, Optional<Attribute<Filter::Xml::VAddrMAC, Name::Strict<5356> > >, Optional<Attribute<Filter::Xml::VUint16range, Name::Strict<212> > >, Optional<Attribute<Filter::Xml::VUint16range, Name::Strict<5357> > >, Optional<Attribute<Filter::Xml::VUint16range, Name::Strict<5358> > >, Optional<Attribute<Filter::Xml::VUint16range, Name::Strict<5359> > >, Optional<Attribute<Filter::Xml::VUint16range, Name::Strict<5360> > >, Optional<Attribute<Filter::Xml::VUint16range, Name::Strict<5361> > >, Optional<Attribute<Filter::Xml::VUint16range, Name::Strict<5362> > >, Optional<Attribute<Filter::Xml::VUint16range, Name::Strict<5363> > >, Optional<Attribute<Filter::Xml::VUint16range, Name::Strict<5364> > >, Optional<Attribute<Filter::Xml::VUint16range, Name::Strict<5365> > > > > marshal_type;

	static int parse(Filter::Xml::StpAttributes& , QStack<QDomElement>& );
	static int generate(const Filter::Xml::StpAttributes& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Stp traits

template<>
struct Traits<Filter::Xml::Stp>
{
	typedef Ordered<mpl::vector<Optional<Attribute<Filter::Xml::EVirYesNo, Name::Strict<1015> > >, Fragment<Filter::Xml::SrcmacandmaskAttributes >, Fragment<Filter::Xml::StpAttributes >, Optional<Attribute<Filter::Xml::PCommentType, Name::Strict<5377> > > > > marshal_type;

	static int parse(Filter::Xml::Stp& , QStack<QDomElement>& );
	static int generate(const Filter::Xml::Stp& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct ArpAttributes traits

template<>
struct Traits<Filter::Xml::ArpAttributes>
{
	typedef Unordered<mpl::vector<Optional<Attribute<Filter::Xml::VAddrMAC, Name::Strict<5366> > >, Optional<Attribute<Filter::Xml::VAddrIP, Name::Strict<5367> > >, Optional<Attribute<Filter::Xml::VAddrMAC, Name::Strict<5368> > >, Optional<Attribute<Filter::Xml::VAddrIP, Name::Strict<5369> > >, Optional<Attribute<Filter::Xml::VUint16range, Name::Strict<5370> > >, Optional<Attribute<Filter::Xml::VArpOpcodeType, Name::Strict<5371> > >, Optional<Attribute<Filter::Xml::VUint16range, Name::Strict<5373> > >, Optional<Attribute<Filter::Xml::EBoolean, Name::Strict<5374> > > > > marshal_type;

	static int parse(Filter::Xml::ArpAttributes& , QStack<QDomElement>& );
	static int generate(const Filter::Xml::ArpAttributes& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Arp traits

template<>
struct Traits<Filter::Xml::Arp>
{
	typedef Ordered<mpl::vector<Optional<Attribute<Filter::Xml::EVirYesNo, Name::Strict<1015> > >, Fragment<Filter::Xml::CommonL2Attributes >, Fragment<Filter::Xml::ArpAttributes >, Optional<Attribute<Filter::Xml::PCommentType, Name::Strict<5377> > > > > marshal_type;

	static int parse(Filter::Xml::Arp& , QStack<QDomElement>& );
	static int generate(const Filter::Xml::Arp& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Rarp traits

template<>
struct Traits<Filter::Xml::Rarp>
{
	typedef Ordered<mpl::vector<Optional<Attribute<Filter::Xml::EVirYesNo, Name::Strict<1015> > >, Fragment<Filter::Xml::CommonL2Attributes >, Fragment<Filter::Xml::ArpAttributes >, Optional<Attribute<Filter::Xml::PCommentType, Name::Strict<5377> > > > > marshal_type;

	static int parse(Filter::Xml::Rarp& , QStack<QDomElement>& );
	static int generate(const Filter::Xml::Rarp& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct CommonIpAttributesP1 traits

template<>
struct Traits<Filter::Xml::CommonIpAttributesP1>
{
	typedef Unordered<mpl::vector<Optional<Attribute<Filter::Xml::VAddrIP, Name::Strict<5314> > >, Optional<Attribute<Filter::Xml::VAddrMask, Name::Strict<5316> > >, Optional<Attribute<Filter::Xml::VAddrIP, Name::Strict<5318> > >, Optional<Attribute<Filter::Xml::VAddrMask, Name::Strict<5319> > > > > marshal_type;

	static int parse(Filter::Xml::CommonIpAttributesP1& , QStack<QDomElement>& );
	static int generate(const Filter::Xml::CommonIpAttributesP1& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct CommonPortAttributes traits

template<>
struct Traits<Filter::Xml::CommonPortAttributes>
{
	typedef Unordered<mpl::vector<Optional<Attribute<Filter::Xml::VUint16range, Name::Strict<5335> > >, Optional<Attribute<Filter::Xml::VUint16range, Name::Strict<5336> > >, Optional<Attribute<Filter::Xml::VUint16range, Name::Strict<5337> > >, Optional<Attribute<Filter::Xml::VUint16range, Name::Strict<5338> > > > > marshal_type;

	static int parse(Filter::Xml::CommonPortAttributes& , QStack<QDomElement>& );
	static int generate(const Filter::Xml::CommonPortAttributes& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Ip traits

template<>
struct Traits<Filter::Xml::Ip>
{
	typedef Ordered<mpl::vector<Optional<Attribute<Filter::Xml::EVirYesNo, Name::Strict<1015> > >, Fragment<Filter::Xml::CommonL2Attributes >, Fragment<Filter::Xml::CommonIpAttributesP1 >, Fragment<Filter::Xml::CommonPortAttributes >, Optional<Attribute<Filter::Xml::VIpProtocolType, Name::Strict<203> > >, Optional<Attribute<Filter::Xml::VSixbitrange, Name::Strict<5324> > >, Optional<Attribute<Filter::Xml::PCommentType, Name::Strict<5377> > > > > marshal_type;

	static int parse(Filter::Xml::Ip& , QStack<QDomElement>& );
	static int generate(const Filter::Xml::Ip& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct CommonIpv6AttributesP1 traits

template<>
struct Traits<Filter::Xml::CommonIpv6AttributesP1>
{
	typedef Unordered<mpl::vector<Optional<Attribute<Filter::Xml::VAddrIPv6, Name::Strict<5314> > >, Optional<Attribute<Filter::Xml::VAddrMaskv6, Name::Strict<5316> > >, Optional<Attribute<Filter::Xml::VAddrIPv6, Name::Strict<5318> > >, Optional<Attribute<Filter::Xml::VAddrMaskv6, Name::Strict<5319> > > > > marshal_type;

	static int parse(Filter::Xml::CommonIpv6AttributesP1& , QStack<QDomElement>& );
	static int generate(const Filter::Xml::CommonIpv6AttributesP1& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Ipv6 traits

template<>
struct Traits<Filter::Xml::Ipv6>
{
	typedef Ordered<mpl::vector<Optional<Attribute<Filter::Xml::EVirYesNo, Name::Strict<1015> > >, Fragment<Filter::Xml::CommonL2Attributes >, Fragment<Filter::Xml::CommonIpv6AttributesP1 >, Fragment<Filter::Xml::CommonPortAttributes >, Optional<Attribute<Filter::Xml::VIpProtocolType, Name::Strict<203> > >, Optional<Attribute<Filter::Xml::PCommentType, Name::Strict<5377> > > > > marshal_type;

	static int parse(Filter::Xml::Ipv6& , QStack<QDomElement>& );
	static int generate(const Filter::Xml::Ipv6& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Anonymous5679 traits

template<>
struct Traits<Filter::Xml::Anonymous5679>
{
	typedef Ordered<mpl::vector<Attribute<Filter::Xml::VIpsetNameType, Name::Strict<5329> >, Attribute<Filter::Xml::PIpsetFlagsType, Name::Strict<5331> > > > marshal_type;

	static int parse(Filter::Xml::Anonymous5679& , QStack<QDomElement>& );
	static int generate(const Filter::Xml::Anonymous5679& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct CommonIpAttributesP2 traits

template<>
struct Traits<Filter::Xml::CommonIpAttributesP2>
{
	typedef Unordered<mpl::vector<Optional<Attribute<Filter::Xml::VAddrIP, Name::Strict<5320> > >, Optional<Attribute<Filter::Xml::VAddrIP, Name::Strict<5321> > >, Optional<Attribute<Filter::Xml::VAddrIP, Name::Strict<5322> > >, Optional<Attribute<Filter::Xml::VAddrIP, Name::Strict<5323> > >, Optional<Attribute<Filter::Xml::VSixbitrange, Name::Strict<5324> > >, Optional<Attribute<Filter::Xml::VUint16range, Name::Strict<5326> > >, Optional<Attribute<Filter::Xml::PStateflagsType, Name::Strict<126> > >, Optional<Fragment<Filter::Xml::Anonymous5679 > > > > marshal_type;

	static int parse(Filter::Xml::CommonIpAttributesP2& , QStack<QDomElement>& );
	static int generate(const Filter::Xml::CommonIpAttributesP2& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Tcp traits

template<>
struct Traits<Filter::Xml::Tcp>
{
	typedef Ordered<mpl::vector<Optional<Attribute<Filter::Xml::EVirYesNo, Name::Strict<1015> > >, Optional<Attribute<Filter::Xml::VAddrMAC, Name::Strict<5309> > >, Fragment<Filter::Xml::CommonPortAttributes >, Fragment<Filter::Xml::CommonIpAttributesP1 >, Fragment<Filter::Xml::CommonIpAttributesP2 >, Optional<Attribute<Filter::Xml::PTcpflagsType, Name::Strict<5345> > >, Optional<Attribute<Filter::Xml::PCommentType, Name::Strict<5377> > > > > marshal_type;

	static int parse(Filter::Xml::Tcp& , QStack<QDomElement>& );
	static int generate(const Filter::Xml::Tcp& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Udp traits

template<>
struct Traits<Filter::Xml::Udp>
{
	typedef Ordered<mpl::vector<Optional<Attribute<Filter::Xml::EVirYesNo, Name::Strict<1015> > >, Optional<Attribute<Filter::Xml::VAddrMAC, Name::Strict<5309> > >, Fragment<Filter::Xml::CommonPortAttributes >, Fragment<Filter::Xml::CommonIpAttributesP1 >, Fragment<Filter::Xml::CommonIpAttributesP2 >, Optional<Attribute<Filter::Xml::PCommentType, Name::Strict<5377> > > > > marshal_type;

	static int parse(Filter::Xml::Udp& , QStack<QDomElement>& );
	static int generate(const Filter::Xml::Udp& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Sctp traits

template<>
struct Traits<Filter::Xml::Sctp>
{
	typedef Ordered<mpl::vector<Optional<Attribute<Filter::Xml::EVirYesNo, Name::Strict<1015> > >, Optional<Attribute<Filter::Xml::VAddrMAC, Name::Strict<5309> > >, Fragment<Filter::Xml::CommonPortAttributes >, Fragment<Filter::Xml::CommonIpAttributesP1 >, Fragment<Filter::Xml::CommonIpAttributesP2 >, Optional<Attribute<Filter::Xml::PCommentType, Name::Strict<5377> > > > > marshal_type;

	static int parse(Filter::Xml::Sctp& , QStack<QDomElement>& );
	static int generate(const Filter::Xml::Sctp& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct IcmpAttributes traits

template<>
struct Traits<Filter::Xml::IcmpAttributes>
{
	typedef Unordered<mpl::vector<Optional<Attribute<Filter::Xml::VUint8range, Name::Strict<105> > >, Optional<Attribute<Filter::Xml::VUint8range, Name::Strict<5339> > > > > marshal_type;

	static int parse(Filter::Xml::IcmpAttributes& , QStack<QDomElement>& );
	static int generate(const Filter::Xml::IcmpAttributes& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Icmp traits

template<>
struct Traits<Filter::Xml::Icmp>
{
	typedef Ordered<mpl::vector<Optional<Attribute<Filter::Xml::EVirYesNo, Name::Strict<1015> > >, Optional<Attribute<Filter::Xml::VAddrMAC, Name::Strict<5309> > >, Fragment<Filter::Xml::CommonIpAttributesP1 >, Fragment<Filter::Xml::CommonIpAttributesP2 >, Fragment<Filter::Xml::IcmpAttributes >, Optional<Attribute<Filter::Xml::PCommentType, Name::Strict<5377> > > > > marshal_type;

	static int parse(Filter::Xml::Icmp& , QStack<QDomElement>& );
	static int generate(const Filter::Xml::Icmp& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Igmp traits

template<>
struct Traits<Filter::Xml::Igmp>
{
	typedef Ordered<mpl::vector<Optional<Attribute<Filter::Xml::EVirYesNo, Name::Strict<1015> > >, Optional<Attribute<Filter::Xml::VAddrMAC, Name::Strict<5309> > >, Fragment<Filter::Xml::CommonIpAttributesP1 >, Fragment<Filter::Xml::CommonIpAttributesP2 >, Optional<Attribute<Filter::Xml::PCommentType, Name::Strict<5377> > > > > marshal_type;

	static int parse(Filter::Xml::Igmp& , QStack<QDomElement>& );
	static int generate(const Filter::Xml::Igmp& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct All traits

template<>
struct Traits<Filter::Xml::All>
{
	typedef Ordered<mpl::vector<Optional<Attribute<Filter::Xml::EVirYesNo, Name::Strict<1015> > >, Optional<Attribute<Filter::Xml::VAddrMAC, Name::Strict<5309> > >, Fragment<Filter::Xml::CommonIpAttributesP1 >, Fragment<Filter::Xml::CommonIpAttributesP2 >, Optional<Attribute<Filter::Xml::PCommentType, Name::Strict<5377> > > > > marshal_type;

	static int parse(Filter::Xml::All& , QStack<QDomElement>& );
	static int generate(const Filter::Xml::All& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Esp traits

template<>
struct Traits<Filter::Xml::Esp>
{
	typedef Ordered<mpl::vector<Optional<Attribute<Filter::Xml::EVirYesNo, Name::Strict<1015> > >, Optional<Attribute<Filter::Xml::VAddrMAC, Name::Strict<5309> > >, Fragment<Filter::Xml::CommonIpAttributesP1 >, Fragment<Filter::Xml::CommonIpAttributesP2 >, Optional<Attribute<Filter::Xml::PCommentType, Name::Strict<5377> > > > > marshal_type;

	static int parse(Filter::Xml::Esp& , QStack<QDomElement>& );
	static int generate(const Filter::Xml::Esp& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Ah traits

template<>
struct Traits<Filter::Xml::Ah>
{
	typedef Ordered<mpl::vector<Optional<Attribute<Filter::Xml::EVirYesNo, Name::Strict<1015> > >, Optional<Attribute<Filter::Xml::VAddrMAC, Name::Strict<5309> > >, Fragment<Filter::Xml::CommonIpAttributesP1 >, Fragment<Filter::Xml::CommonIpAttributesP2 >, Optional<Attribute<Filter::Xml::PCommentType, Name::Strict<5377> > > > > marshal_type;

	static int parse(Filter::Xml::Ah& , QStack<QDomElement>& );
	static int generate(const Filter::Xml::Ah& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Udplite traits

template<>
struct Traits<Filter::Xml::Udplite>
{
	typedef Ordered<mpl::vector<Optional<Attribute<Filter::Xml::EVirYesNo, Name::Strict<1015> > >, Optional<Attribute<Filter::Xml::VAddrMAC, Name::Strict<5309> > >, Fragment<Filter::Xml::CommonIpAttributesP1 >, Fragment<Filter::Xml::CommonIpAttributesP2 >, Optional<Attribute<Filter::Xml::PCommentType, Name::Strict<5377> > > > > marshal_type;

	static int parse(Filter::Xml::Udplite& , QStack<QDomElement>& );
	static int generate(const Filter::Xml::Udplite& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct CommonIpv6AttributesP2 traits

template<>
struct Traits<Filter::Xml::CommonIpv6AttributesP2>
{
	typedef Unordered<mpl::vector<Optional<Attribute<Filter::Xml::VAddrIPv6, Name::Strict<5320> > >, Optional<Attribute<Filter::Xml::VAddrIPv6, Name::Strict<5321> > >, Optional<Attribute<Filter::Xml::VAddrIPv6, Name::Strict<5322> > >, Optional<Attribute<Filter::Xml::VAddrIPv6, Name::Strict<5323> > >, Optional<Attribute<Filter::Xml::VSixbitrange, Name::Strict<5324> > > > > marshal_type;

	static int parse(Filter::Xml::CommonIpv6AttributesP2& , QStack<QDomElement>& );
	static int generate(const Filter::Xml::CommonIpv6AttributesP2& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct TcpIpv6 traits

template<>
struct Traits<Filter::Xml::TcpIpv6>
{
	typedef Ordered<mpl::vector<Optional<Attribute<Filter::Xml::EVirYesNo, Name::Strict<1015> > >, Optional<Attribute<Filter::Xml::VAddrMAC, Name::Strict<5309> > >, Fragment<Filter::Xml::CommonPortAttributes >, Fragment<Filter::Xml::CommonIpv6AttributesP1 >, Fragment<Filter::Xml::CommonIpv6AttributesP2 >, Optional<Attribute<Filter::Xml::PTcpflagsType, Name::Strict<5345> > >, Optional<Attribute<Filter::Xml::PCommentType, Name::Strict<5377> > > > > marshal_type;

	static int parse(Filter::Xml::TcpIpv6& , QStack<QDomElement>& );
	static int generate(const Filter::Xml::TcpIpv6& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct UdpIpv6 traits

template<>
struct Traits<Filter::Xml::UdpIpv6>
{
	typedef Ordered<mpl::vector<Optional<Attribute<Filter::Xml::EVirYesNo, Name::Strict<1015> > >, Optional<Attribute<Filter::Xml::VAddrMAC, Name::Strict<5309> > >, Fragment<Filter::Xml::CommonPortAttributes >, Fragment<Filter::Xml::CommonIpv6AttributesP1 >, Fragment<Filter::Xml::CommonIpv6AttributesP2 >, Optional<Attribute<Filter::Xml::PCommentType, Name::Strict<5377> > > > > marshal_type;

	static int parse(Filter::Xml::UdpIpv6& , QStack<QDomElement>& );
	static int generate(const Filter::Xml::UdpIpv6& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct SctpIpv6 traits

template<>
struct Traits<Filter::Xml::SctpIpv6>
{
	typedef Ordered<mpl::vector<Optional<Attribute<Filter::Xml::EVirYesNo, Name::Strict<1015> > >, Optional<Attribute<Filter::Xml::VAddrMAC, Name::Strict<5309> > >, Fragment<Filter::Xml::CommonPortAttributes >, Fragment<Filter::Xml::CommonIpv6AttributesP1 >, Fragment<Filter::Xml::CommonIpv6AttributesP2 >, Optional<Attribute<Filter::Xml::PCommentType, Name::Strict<5377> > > > > marshal_type;

	static int parse(Filter::Xml::SctpIpv6& , QStack<QDomElement>& );
	static int generate(const Filter::Xml::SctpIpv6& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Icmpv6 traits

template<>
struct Traits<Filter::Xml::Icmpv6>
{
	typedef Ordered<mpl::vector<Optional<Attribute<Filter::Xml::EVirYesNo, Name::Strict<1015> > >, Optional<Attribute<Filter::Xml::VAddrMAC, Name::Strict<5309> > >, Fragment<Filter::Xml::CommonIpv6AttributesP1 >, Fragment<Filter::Xml::CommonIpv6AttributesP2 >, Fragment<Filter::Xml::IcmpAttributes >, Optional<Attribute<Filter::Xml::PCommentType, Name::Strict<5377> > > > > marshal_type;

	static int parse(Filter::Xml::Icmpv6& , QStack<QDomElement>& );
	static int generate(const Filter::Xml::Icmpv6& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct AllIpv6 traits

template<>
struct Traits<Filter::Xml::AllIpv6>
{
	typedef Ordered<mpl::vector<Optional<Attribute<Filter::Xml::EVirYesNo, Name::Strict<1015> > >, Optional<Attribute<Filter::Xml::VAddrMAC, Name::Strict<5309> > >, Fragment<Filter::Xml::CommonIpv6AttributesP1 >, Fragment<Filter::Xml::CommonIpv6AttributesP2 >, Optional<Attribute<Filter::Xml::PCommentType, Name::Strict<5377> > > > > marshal_type;

	static int parse(Filter::Xml::AllIpv6& , QStack<QDomElement>& );
	static int generate(const Filter::Xml::AllIpv6& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct EspIpv6 traits

template<>
struct Traits<Filter::Xml::EspIpv6>
{
	typedef Ordered<mpl::vector<Optional<Attribute<Filter::Xml::EVirYesNo, Name::Strict<1015> > >, Optional<Attribute<Filter::Xml::VAddrMAC, Name::Strict<5309> > >, Fragment<Filter::Xml::CommonIpv6AttributesP1 >, Fragment<Filter::Xml::CommonIpv6AttributesP2 >, Optional<Attribute<Filter::Xml::PCommentType, Name::Strict<5377> > > > > marshal_type;

	static int parse(Filter::Xml::EspIpv6& , QStack<QDomElement>& );
	static int generate(const Filter::Xml::EspIpv6& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct AhIpv6 traits

template<>
struct Traits<Filter::Xml::AhIpv6>
{
	typedef Ordered<mpl::vector<Optional<Attribute<Filter::Xml::EVirYesNo, Name::Strict<1015> > >, Optional<Attribute<Filter::Xml::VAddrMAC, Name::Strict<5309> > >, Fragment<Filter::Xml::CommonIpv6AttributesP1 >, Fragment<Filter::Xml::CommonIpv6AttributesP2 >, Optional<Attribute<Filter::Xml::PCommentType, Name::Strict<5377> > > > > marshal_type;

	static int parse(Filter::Xml::AhIpv6& , QStack<QDomElement>& );
	static int generate(const Filter::Xml::AhIpv6& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct UdpliteIpv6 traits

template<>
struct Traits<Filter::Xml::UdpliteIpv6>
{
	typedef Ordered<mpl::vector<Optional<Attribute<Filter::Xml::EVirYesNo, Name::Strict<1015> > >, Optional<Attribute<Filter::Xml::VAddrMAC, Name::Strict<5309> > >, Fragment<Filter::Xml::CommonIpv6AttributesP1 >, Fragment<Filter::Xml::CommonIpv6AttributesP2 >, Optional<Attribute<Filter::Xml::PCommentType, Name::Strict<5377> > > > > marshal_type;

	static int parse(Filter::Xml::UdpliteIpv6& , QStack<QDomElement>& );
	static int generate(const Filter::Xml::UdpliteIpv6& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Rule traits

template<>
struct Traits<Filter::Xml::Rule>
{
	typedef Ordered<mpl::vector<Fragment<Filter::Xml::RuleNodeAttributes >, Optional<ZeroOrMore<Element<Filter::Xml::Mac, Name::Strict<673> > > >, Optional<ZeroOrMore<Element<Filter::Xml::Vlan, Name::Strict<205> > > >, Optional<ZeroOrMore<Element<Filter::Xml::Stp, Name::Strict<1227> > > >, Optional<ZeroOrMore<Element<Filter::Xml::Arp, Name::Strict<5261> > > >, Optional<ZeroOrMore<Element<Filter::Xml::Rarp, Name::Strict<5263> > > >, Optional<ZeroOrMore<Element<Filter::Xml::Ip, Name::Strict<689> > > >, Optional<ZeroOrMore<Element<Filter::Xml::Ipv6, Name::Strict<1226> > > >, Optional<ZeroOrMore<Element<Filter::Xml::Tcp, Name::Strict<515> > > >, Optional<ZeroOrMore<Element<Filter::Xml::Udp, Name::Strict<831> > > >, Optional<ZeroOrMore<Element<Filter::Xml::Sctp, Name::Strict<5272> > > >, Optional<ZeroOrMore<Element<Filter::Xml::Icmp, Name::Strict<5273> > > >, Optional<ZeroOrMore<Element<Filter::Xml::Igmp, Name::Strict<5275> > > >, Optional<ZeroOrMore<Element<Filter::Xml::All, Name::Strict<765> > > >, Optional<ZeroOrMore<Element<Filter::Xml::Esp, Name::Strict<5276> > > >, Optional<ZeroOrMore<Element<Filter::Xml::Ah, Name::Strict<5277> > > >, Optional<ZeroOrMore<Element<Filter::Xml::Udplite, Name::Strict<5278> > > >, Optional<ZeroOrMore<Element<Filter::Xml::TcpIpv6, Name::Strict<5279> > > >, Optional<ZeroOrMore<Element<Filter::Xml::UdpIpv6, Name::Strict<5281> > > >, Optional<ZeroOrMore<Element<Filter::Xml::SctpIpv6, Name::Strict<5282> > > >, Optional<ZeroOrMore<Element<Filter::Xml::Icmpv6, Name::Strict<5283> > > >, Optional<ZeroOrMore<Element<Filter::Xml::AllIpv6, Name::Strict<5284> > > >, Optional<ZeroOrMore<Element<Filter::Xml::EspIpv6, Name::Strict<5285> > > >, Optional<ZeroOrMore<Element<Filter::Xml::AhIpv6, Name::Strict<5286> > > >, Optional<ZeroOrMore<Element<Filter::Xml::UdpliteIpv6, Name::Strict<5287> > > > > > marshal_type;

	static int parse(Filter::Xml::Rule& , QStack<QDomElement>& );
	static int generate(const Filter::Xml::Rule& , QDomElement& );
};

///////////////////////////////////////////////////////////////////////////////
// struct Filter traits

template<>
struct Traits<Filter::Xml::Filter>
{
	typedef Ordered<mpl::vector<Fragment<Filter::Xml::FilterNodeAttributes >, Optional<Element<Text<Filter::Xml::VUUID >, Name::Strict<151> > >, ZeroOrMore<Filter::Xml::VChoice5120Impl > > > marshal_type;

	static int parse(Filter::Xml::Filter& , QStack<QDomElement>& );
	static int generate(const Filter::Xml::Filter& , QDomElement& );
};

} // namespace Libvirt

#endif // __FILTER_TYPE_H__
