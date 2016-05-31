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

#include "CCpuHelper.h"

#include <QString>
#include <QMap>
#include <QSet>
#include <memory>

#include <boost/variant/static_visitor.hpp>
#include <boost/mpl/for_each.hpp>
#include <boost/mpl/range_c.hpp>

#include <prlcommon/HostUtils/HostUtils.h>

namespace
{

static const char CPUFEATURES_BINARY[] = "/usr/sbin/cpufeatures";
/* TODO move to some hidden place */
static const char CPUPOOLS_BINARY[] = "/usr/sbin/cpupools";

struct MaskTag
{
};
struct FeaturesTag
{
};
struct MaskCapsTag
{
};
struct VmFeaturesTag
{
};
struct PoolFeaturesTag
{
};

template <typename Tag>
struct TagToModel;

template <>
struct TagToModel<MaskTag>
{
	typedef CDispCpuPreferences type;
};

template <>
struct TagToModel<FeaturesTag>
{
	typedef CHwCpu type;
};

template <>
struct TagToModel<MaskCapsTag>
{
	typedef CHwCpu type;
};

template <>
struct TagToModel<VmFeaturesTag>
{
	typedef CVmRunTimeOptions type;
};

template <>
struct TagToModel<PoolFeaturesTag>
{
	typedef CCpuFeatures type;
};

template <typename Tag>
class Traits
{
public:
	typedef typename TagToModel<Tag>::type xmlModel_type;
	typedef unsigned int (xmlModel_type::*getter_type)() const;
	typedef void (xmlModel_type::*setter_type)(unsigned int);
	typedef QMap<QString, getter_type> gettersMap_type;
	typedef QMap<QString, setter_type> settersMap_type;

	static const gettersMap_type& getGetters()
	{
		return s_accessors.m_getters;
	}
	static const settersMap_type& getSetters()
	{
		return s_accessors.m_setters;
	}
private:
	struct Accessors
	{
		gettersMap_type m_getters;
		settersMap_type m_setters;
	};

	static Accessors createAccessors();

	static const Accessors s_accessors;
};

template <typename Tag>
class FeaturesSetIn;

template <typename Tag>
class FeaturesSetOut
{
public:
	typedef Traits<Tag> traits_type;
	typedef typename traits_type::xmlModel_type xmlModel_type;
	typedef typename traits_type::gettersMap_type gettersMap_type;

	explicit FeaturesSetOut(xmlModel_type &model);
	QString createArgs() const;
	bool operator==(const FeaturesSetOut<Tag> &other) const;

	unsigned int getValue(const QString &name) const;

protected:
	xmlModel_type *m_model;
};

template <typename Tag>
class FeaturesSetIn : public FeaturesSetOut<Tag>
{
public:
	typedef Traits<Tag> traits_type;
	typedef typename traits_type::xmlModel_type xmlModel_type;
	typedef typename traits_type::settersMap_type settersMap_type;

	explicit FeaturesSetIn(xmlModel_type &model);
	bool setValue(const QString &name, unsigned int value);
	bool parse(const QString &out);
	void fill(unsigned int value);

	template <typename OtherTag>
	FeaturesSetIn<Tag> &operator=(const FeaturesSetOut<OtherTag> &other);
};

template <typename Tag>
FeaturesSetOut<Tag>::FeaturesSetOut(xmlModel_type &model) :
	m_model(&model)
{
}

template <typename Tag>
FeaturesSetIn<Tag>::FeaturesSetIn(xmlModel_type &model) :
	FeaturesSetOut<Tag>(model)
{
}

template <typename Tag>
unsigned int FeaturesSetOut<Tag>::getValue(const QString &name) const
{
	if (!traits_type::getSetters().contains(name))
		return 0;
	return (this->m_model->*traits_type::getGetters()[name])();
}

template <typename Tag>
bool FeaturesSetIn<Tag>::setValue(const QString &name, unsigned int value)
{
	if (!traits_type::getSetters().contains(name))
		return false;
	(this->m_model->*traits_type::getSetters()[name])(value);
	return true;
}

#define INSERT_ACCESSOR(RegisterStr, FeatureName) \
	a.m_getters.insert(RegisterStr, &XML_MODEL::get##FeatureName);\
	a.m_setters.insert(RegisterStr, &XML_MODEL::set##FeatureName);

typedef Traits<MaskTag> MaskTraits;
typedef Traits<FeaturesTag> FeaturesTraits;
typedef Traits<MaskCapsTag> MaskCapsTraits;
typedef Traits<VmFeaturesTag> VmFeaturesTraits;
typedef Traits<PoolFeaturesTag> PoolFeaturesTraits;

template<>
MaskTraits::Accessors MaskTraits::createAccessors()
{
	MaskTraits::Accessors a;
#define XML_MODEL CDispCpuPreferences
	INSERT_ACCESSOR("cpuid_00000001_ECX", EXT_FEATURES_MASK);
	INSERT_ACCESSOR("cpuid_00000001_EDX", FEATURES_MASK);
	INSERT_ACCESSOR("cpuid_80000001_ECX", EXT_80000001_ECX_MASK);
	INSERT_ACCESSOR("cpuid_80000001_EDX", EXT_80000001_EDX_MASK);
	INSERT_ACCESSOR("cpuid_80000007_EDX", EXT_80000007_EDX_MASK);
	INSERT_ACCESSOR("cpuid_00000007_EBX", EXT_00000007_EBX_MASK);
	INSERT_ACCESSOR("cpuid_80000008_EAX", EXT_80000008_EAX);
	INSERT_ACCESSOR("cpuid_0000000D_01_EAX", EXT_0000000D_EAX_MASK);
#undef XML_MODEL
	return a;
}

template<>
FeaturesTraits::Accessors FeaturesTraits::createAccessors()
{
	FeaturesTraits::Accessors a;
#define XML_MODEL CHwCpu
	INSERT_ACCESSOR("cpuid_00000001_ECX", EXT_FEATURES);
	INSERT_ACCESSOR("cpuid_00000001_EDX", FEATURES);
	INSERT_ACCESSOR("cpuid_80000001_ECX", EXT_80000001_ECX);
	INSERT_ACCESSOR("cpuid_80000001_EDX", EXT_80000001_EDX);
	INSERT_ACCESSOR("cpuid_80000007_EDX", EXT_80000007_EDX);
	INSERT_ACCESSOR("cpuid_00000007_EBX", EXT_00000007_EBX);
	INSERT_ACCESSOR("cpuid_80000008_EAX", EXT_80000008_EAX);
	INSERT_ACCESSOR("cpuid_0000000D_01_EAX", EXT_0000000D_EAX);
#undef XML_MODEL
	return a;
}

template<>
MaskCapsTraits::Accessors MaskCapsTraits::createAccessors()
{
	MaskCapsTraits::Accessors a;
#define XML_MODEL CHwCpu
	INSERT_ACCESSOR("cpuid_00000001_ECX", EXT_FEATURES_MASKING_CAP);
	INSERT_ACCESSOR("cpuid_00000001_EDX", FEATURES_MASKING_CAP);
	INSERT_ACCESSOR("cpuid_80000001_ECX", EXT_80000001_ECX_MASKING_CAP);
	INSERT_ACCESSOR("cpuid_80000001_EDX", EXT_80000001_EDX_MASKING_CAP);
	INSERT_ACCESSOR("cpuid_80000007_EDX", EXT_80000007_EDX_MASKING_CAP);
	INSERT_ACCESSOR("cpuid_00000007_EBX", EXT_00000007_EBX_MASKING_CAP);
	INSERT_ACCESSOR("cpuid_80000008_EAX", EXT_80000008_EAX_MASKING_CAP);
	INSERT_ACCESSOR("cpuid_0000000D_01_EAX", EXT_0000000D_EAX_MASKING_CAP);
#undef XML_MODEL
	return a;
}

template<>
VmFeaturesTraits::Accessors VmFeaturesTraits::createAccessors()
{
	VmFeaturesTraits::Accessors a;
#define XML_MODEL CVmRunTimeOptions
	INSERT_ACCESSOR("cpuid_00000001_ECX", EXT_FEATURES_MASK);
	INSERT_ACCESSOR("cpuid_00000001_EDX", FEATURES_MASK);
	INSERT_ACCESSOR("cpuid_80000001_ECX", EXT_80000001_ECX_MASK);
	INSERT_ACCESSOR("cpuid_80000001_EDX", EXT_80000001_EDX_MASK);
	INSERT_ACCESSOR("cpuid_80000007_EDX", EXT_80000007_EDX_MASK);
	INSERT_ACCESSOR("cpuid_00000007_EBX", EXT_00000007_EBX_MASK);
	INSERT_ACCESSOR("cpuid_80000008_EAX", EXT_80000008_EAX);
	/* INSERT_ACCESSOR("cpuid_0000000D_01_EAX", EXT_0000000D_EAX_MASK); */
#undef XML_MODEL
	return a;
}

template<>
PoolFeaturesTraits::Accessors PoolFeaturesTraits::createAccessors()
{
	PoolFeaturesTraits::Accessors a;
#define XML_MODEL CCpuFeatures
	INSERT_ACCESSOR("cpuid_00000001_ECX", EXT_FEATURES);
	INSERT_ACCESSOR("cpuid_00000001_EDX", FEATURES);
	INSERT_ACCESSOR("cpuid_80000001_ECX", EXT_80000001_ECX);
	INSERT_ACCESSOR("cpuid_80000001_EDX", EXT_80000001_EDX);
	INSERT_ACCESSOR("cpuid_80000007_EDX", EXT_80000007_EDX);
	INSERT_ACCESSOR("cpuid_00000007_EBX", EXT_00000007_EBX);
	INSERT_ACCESSOR("cpuid_80000008_EAX", EXT_80000008_EAX);
	INSERT_ACCESSOR("cpuid_0000000D_01_EAX", EXT_0000000D_EAX);
#undef XML_MODEL
	return a;
}

template <typename Tag>
const typename Traits<Tag>::Accessors Traits<Tag>::s_accessors = Traits<Tag>::createAccessors();

template <typename Model>
struct ParseTraits;

template <typename Tag>
struct ParseTraits< FeaturesSetIn<Tag> >
{
	static bool checkAllSetted(const QSet<QString> &processed);
	static bool setValue(FeaturesSetIn<Tag> &model, const QString &name, const QString &value);
};

template <>
struct ParseTraits<CCpuPoolInfo>
{
	static const char POOL_INFO_NAME[];
	static const char POOL_INFO_VENDOR[];

	static bool checkAllSetted(const QSet<QString> &processed);
	static bool setValue(CCpuPoolInfo &model, const QString &name, const QString &value);
};

template <typename Tag>
bool ParseTraits< FeaturesSetIn<Tag> >::checkAllSetted(const QSet<QString> &processed)
{
	foreach (const QString &r, Traits<Tag>::getSetters().keys())
		if (!processed.contains(r))
			return false;
	return true;
}

template <typename Tag>
bool ParseTraits< FeaturesSetIn<Tag> >::setValue(FeaturesSetIn<Tag> &model, const QString &name, const QString &value)
{
	bool ok;
	unsigned int v = value.toUInt(&ok, 0);
	if (!ok)
	{
		WRITE_TRACE(DBG_FATAL, "can't parse register value: '%s'", QSTR2UTF8(value));
		return false;
	}
	model.setValue(name, v);
	return true;
}

const char ParseTraits<CCpuPoolInfo>::POOL_INFO_NAME[] = "name";
const char ParseTraits<CCpuPoolInfo>::POOL_INFO_VENDOR[] = "vendor";

bool ParseTraits<CCpuPoolInfo>::checkAllSetted(const QSet<QString> &processed)
{
	return processed.contains(POOL_INFO_NAME) && processed.contains(POOL_INFO_VENDOR);
}

bool ParseTraits<CCpuPoolInfo>::setValue(CCpuPoolInfo &model, const QString &name, const QString &value)
{
	if (name == POOL_INFO_NAME)
	{
		model.setName(value);
		return true;
	}
	else if (name == POOL_INFO_VENDOR)
	{
		model.setVendor(value);
		return true;
	}
	return false;
}

template <typename Model>
bool parseKeyValue(const QString &str, Model &model)
{
	QStringList pairs = str.split("\n", QString::SkipEmptyParts);
	QSet<QString> processed;

	foreach(const QString &p, pairs)
	{
		QStringList kv = p.split("=");
		if (kv.size() != 2)
		{
			WRITE_TRACE(DBG_FATAL, "not key-value output line: '%s'", QSTR2UTF8(p));
			return false;
		}
		if (!ParseTraits<Model>::setValue(model, kv[0], kv[1]))
			return false;
		processed.insert(kv[0]);
	}

	if (!ParseTraits<Model>::checkAllSetted(processed))
	{
		WRITE_TRACE(DBG_FATAL, "can't find some keys in output: '%s'", QSTR2UTF8(str));
		return false;
	}

	return true;
}

template <typename Tag>
bool FeaturesSetIn<Tag>::parse(const QString &out)
{
	return parseKeyValue(out, *this);
}

template <typename Tag>
QString FeaturesSetOut<Tag>::createArgs() const
{
	QString args;
	typename gettersMap_type::const_iterator i;

	for (i = traits_type::getGetters().begin(); i != traits_type::getGetters().end(); ++i)
	{
		unsigned int v = (m_model->*(i.value()))();
		QString h = QString("%1").arg((uint)v, 8, 16, QChar('0')).toUpper();
		args += QString("-r %1=0x%2 ").arg(i.key()).arg(h);
	}
	return args;
}

template <typename Tag>
void FeaturesSetIn<Tag>::fill(unsigned int value)
{
	typedef typename traits_type::setter_type setter_type;
	foreach (const setter_type &s, traits_type::getSetters().values())
		(this->m_model->*s)(value);
}

template <typename Tag>
bool FeaturesSetOut<Tag>::operator==(const FeaturesSetOut<Tag> &other) const
{
	foreach (const typename traits_type::getter_type &g, traits_type::getGetters().values())
	{
		if ((m_model->*g)() != (other.m_model->*g)())
			return false;
	}

	return true;
}

template <typename Tag>
template <typename OtherTag>
FeaturesSetIn<Tag> &FeaturesSetIn<Tag>::operator=(const FeaturesSetOut<OtherTag> &other)
{
	foreach (const QString &k, traits_type::getSetters().keys())
		this->setValue(k, other.getValue(k));
	return *this;
}

template <typename FeaturesSetIn>
bool fillCpuDump(FeaturesSetIn &fs, const QString &opt)
{
	QString out, cmd = QString("%1 dump %2").arg(CPUFEATURES_BINARY).arg(opt);

	if (!HostUtils::RunCmdLineUtility(cmd, out, 60 * 1000))
		return false;

	return fs.parse(out);
}

bool checkBinaryExists(const QString &name)
{
	 QFileInfo fi(name);
	 return fi.exists();
}

FeaturesSetIn<MaskTag> fixupValidProperty(CDispCpuPreferences &m)
{
	FeaturesSetIn<MaskTag> fs(m);
	if (!m.isCpuFeaturesMaskValid())
	{
		m.setCpuFeaturesMaskValid(true);
		fs.fill(0xFFFFFFFF);
	}
	return fs;
}

enum
{
	ERR_CPUFEATURES_BIN_INCORRECT_MASK = 6,
	ERR_CPUFEATURES_BIN_INCOMPATIBLE_NODE = 8,
	ERR_CPUFEATURES_BIN_RUNNING_VM_OR_CT = 10,
	ERR_CPUFEATURES_BIN_NOT_IN_POOLS = 16,
	ERR_CPUFEATURES_BIN_POOLS_MANAGEMENT = 17,
	ERR_CPUFEATURES_BIN_DEFAULT_POOL_ARG = 18
};

struct FeaturesCmdHandler : public ExecHandlerBase
{
	FeaturesCmdHandler(QProcess &process, const QString &cmd) :
		ExecHandlerBase(process, cmd), m_retCode(PRL_ERR_FAILURE)
	{
	}

	void exitCode(int code)
	{
		if (code == 0)
		{
			m_retCode = PRL_ERR_SUCCESS;
		}
		else
		{
			logExitCode();
			m_retCode = convert(code);
		}
	}

	PRL_RESULT getRetCode() const
	{
		return m_retCode;
	}

private:
	static int convert(int code)
	{
		switch (code)
		{
		case ERR_CPUFEATURES_BIN_INCORRECT_MASK:
			return PRL_ERR_CPUFEATURES_INCORRECT_MASK;
		case ERR_CPUFEATURES_BIN_RUNNING_VM_OR_CT:
			return PRL_ERR_CPUFEATURES_RUNNING_VM_OR_CT;
		case ERR_CPUFEATURES_BIN_POOLS_MANAGEMENT:
			return PRL_ERR_CPUFEATURES_POOLS_MANAGMENT;
		case ERR_CPUFEATURES_BIN_NOT_IN_POOLS:
			return PRL_ERR_CPUFEATURES_NOT_IN_POOLS;
		case ERR_CPUFEATURES_BIN_DEFAULT_POOL_ARG:
			return PRL_ERR_CPUFEATURES_DEFAULT_POOL_ARG;
		case ERR_CPUFEATURES_BIN_INCOMPATIBLE_NODE:
			return PRL_ERR_CPUFEATURES_INCOMPATIBLE_NODE;
		default:
			return PRL_ERR_FAILURE;
		}
	}

	PRL_RESULT m_retCode;
};

struct DumpInfoHandler : public ExecHandlerBase
{
	DumpInfoHandler(QProcess &process, const QString &cmd, CCpuPoolInfo &info) :
		ExecHandlerBase(process, cmd), m_info(&info)
	{
	}

	void exitCode(int code)
	{
		if (code == 0) {
			m_success = parseKeyValue(UTF8_2QSTR(getProcess().readAllStandardOutput()), *m_info);
			return;
		}

		m_success = code == ERR_CPUFEATURES_BIN_NOT_IN_POOLS;
		m_loglevel = m_success ? DBG_INFO : DBG_FATAL;
		logExitCode();

	}

	bool isSuccess() const
	{
		return m_success;
	}

private:
	bool m_success;
	CCpuPoolInfo *m_info;
};

///////////////////////////////////////////////////////////////////////////////
// struct Config

struct Config
{
	typedef CVmRunTimeOptions type;
	typedef unsigned int (type::*getter_type)() const;
};

///////////////////////////////////////////////////////////////////////////////
// struct Register

struct Register
{
	typedef std::map<const char*, int> map_type;

	Register(const char *features[], Config::getter_type getter) : m_get(getter)
	{
		for (unsigned int i = 0; features[i] != NULL; ++i)
		{
			if (*features[i] != 0)
				m_features.insert(map_type::value_type(features[i], i));
		}
	}

	void appendDisabled(const Config::type &input, QSet<QString> &features) const
	{
		unsigned int m = (input.*m_get)();

		foreach(const map_type::value_type& v, m_features)
		{
			if (((1UL << v.second) & m) == 0)
				features.insert(QString(v.first));
		}
	}


private:
	map_type m_features;
	Config::getter_type m_get;
};

///////////////////////////////////////////////////////////////////////////////
// struct IntegralToInt

template<typename T>
struct IntegralToInt
{
	typedef boost::mpl::int_<T::value> type;
};

///////////////////////////////////////////////////////////////////////////////
// struct List

struct List
{
	typedef QList<Register> type;

	List();

	QSet<QString> getDisabled(const Config::type &input);

private:
	QList<Register> m_registers;
};

///////////////////////////////////////////////////////////////////////////////
// struct Visitor

struct Visitor : boost::static_visitor<void>
{
	Visitor(List::type &list) : m_list(&list)
	{
	}

	void operator()(boost::mpl::int_<PCFE_FEATURES>)
	{
		static const char *f[] = {
			"fpu", "vme", "de", "pse", "tsc", "msr", "pae", "mce", "cx8", "apic", "", "sep",
			"mtrr",	"pge", "mca", "cmov", "pat", "pse36", "pn", "clflush", "", "ds", "acpi",
			"mmx", "fxsr", "sse", "sse2", "ss", "ht", "tm", "ia64", "pbe", NULL
		};

		m_list->push_back(Register(f, &Config::type::getFEATURES_MASK));
	}

	void operator()(boost::mpl::int_<PCFE_EXT_FEATURES>)
	{
		static const char *f[] = {
			"pni", "pclmuldq", "dtes64", "monitor", "ds_cpl", "vmx", "smx", "est",	"tm2",
			"ssse3", "cid",	"",	"fma", "cx16", "xtpr", "pdcm",	"",	"pcid",	"dca", "sse4.1",
			"sse4.2", "x2apic",	"movbe", "popcnt", "tsc-deadline", "aes",	"xsave",
			"osxsave", "avx", "f16c", "rdrand", "hypervisor", NULL
		};

		m_list->push_back(Register(f, &Config::type::getEXT_FEATURES_MASK));
	}

	void operator()(boost::mpl::int_<PCFE_EXT_00000007_EBX>)
	{
		static const char *f[] = {
			"fsgsbase", "tsc_adjust", "sgx", "bmi1", "hle", "avx2",	"",	"smep",	"bmi2",
			"erms", "invpcid", "rtm", "", "", "mpx", "", "avx512f", "",
			"rdseed", "adx", "smap", "", "", "cflushopt", "", "", "avx512pf",	"avx512er",
			"avx512cd",	"", "", "", NULL
		};

		m_list->push_back(Register(f, &Config::type::getEXT_00000007_EBX_MASK));
	}

	void operator()(boost::mpl::int_<PCFE_EXT_80000001_ECX>)
	{
		static const char *f[] = {
			"lahf_lm", "cmp_legacy", "svm",	"extapic", "cr8_legacy", "abm", "sse4a",
			"misalignsse", "3dnowprefetch",	"osvw",	"ibs", "xop", "skinit",	"wdt", "", "lwp",
			"fma4", "tce", "", "nodeid_msr", "", "tbm",	"topoext", "perfctr_core", "perfctr_nb",
			"", "dbx", "perftsc", "perfctr_l2", NULL
		};

		m_list->push_back(Register(f, &Config::type::getEXT_80000001_ECX_MASK));
	}

	void operator()(boost::mpl::int_<PCFE_EXT_80000001_EDX>)
	{
		static const char *f[] = {
			"fpu", "vme", "de",	"pse", "tsc", "msr", "pae", "mce", "cx8", "apic", "", "syscall",
			"mtrr", "pge", "mca", "cmov", "pat", "pse36", "", "mp", "nx", "", "mmxext", "mmx",
			"fxsr", "fxsr_opt", "pdpe1gb", "rdtscp", "", "lm", "3dnowext", "3dnow", NULL
		};

		m_list->push_back(Register(f, &Config::type::getEXT_80000001_EDX_MASK));
	}

	void operator()(boost::mpl::int_<PCFE_EXT_80000007_EDX>)
	{
		static const char *f[] = {NULL};

		m_list->push_back(Register(f, &Config::type::getEXT_80000007_EDX_MASK));
	}

	void operator()(boost::mpl::int_<PCFE_EXT_80000008_EAX>)
	{
		static const char *f[] = {NULL};

		m_list->push_back(Register(f, &Config::type::getEXT_80000008_EAX));
	}

	void operator()(boost::mpl::int_<PCFE_EXT_0000000D_EAX>)
	{
		static const char *f[] = {NULL};

		m_list->push_back(Register(f, &Config::type::getEXT_0000000D_EAX_MASK));
	}

private:
	List::type *m_list;
};

///////////////////////////////////////////////////////////////////////////////
// struct List

List::List()
{
	typedef boost::mpl::range_c<int, PCFE_FEATURES, PCFE_MAX> range_type;

	Visitor v(m_registers);
	boost::mpl::for_each<range_type, IntegralToInt<boost::mpl::_1> >(boost::ref(v));
}

QSet<QString> List::getDisabled(const Config::type &input)
{
	QSet<QString> features;

	foreach(const Register &r, m_registers)
		r.appendDisabled(input, features);

	return features;
}

Q_GLOBAL_STATIC(List, getRegisters);

}// anonyomus namespace

PRL_RESULT CCpuHelper::execFeaturesCmd(const QString &cmdline)
{
	QProcess process;
	FeaturesCmdHandler handler(process, cmdline);
	return HostUtils::RunCmdLineUtilityEx(cmdline, process, 60 * 1000)(handler).getRetCode();
}

CDispCpuPreferences *CCpuHelper::get_cpu_mask()
{
	std::auto_ptr<CDispCpuPreferences> mask(new CDispCpuPreferences);
	FeaturesSetIn<MaskTag> fm(*mask);

	mask->setCpuFeaturesMaskValid(true);
	if (!checkBinaryExists(CPUFEATURES_BINARY))
	{
		fm.fill(0xFFFFFFFF);
	}
	else
	{
		if (!fillCpuDump(fm, "--mask"))
			mask.reset();
	}
	return mask.release();
}

void CCpuHelper::fill_cpu_info(CHwCpu &cpu)
{
	FeaturesSetIn<FeaturesTag> ff(cpu);
	FeaturesSetIn<MaskCapsTag> fmc(cpu);
	// degrade to no features on errors
	if (!checkBinaryExists(CPUFEATURES_BINARY) || !fillCpuDump(ff, "--original"))
		ff.fill(0x00000000);
	// degrade to no mask caps on errors
	if (!checkBinaryExists(CPUFEATURES_BINARY) || !fillCpuDump(fmc, "--mask-caps"))
		fmc.fill(0x00000000);
}

PRL_RESULT CCpuHelper::maskUpdate(CDispCpuPreferences new_mask)
{
	FeaturesSetIn<MaskTag> fs_new = fixupValidProperty(new_mask);

	if (!checkBinaryExists(CPUFEATURES_BINARY))
		return PRL_ERR_CPUFEATURES_NO_BINARY;

	return execFeaturesCmd(QString("%1 set %2").arg(CPUFEATURES_BINARY).arg(fs_new.createArgs()));
}

bool CCpuHelper::update(CVmConfiguration &conf)
{
	std::auto_ptr<CDispCpuPreferences> m(CCpuHelper::get_cpu_mask());
	if (!m.get())
		return false;

	update(conf, *m);

	return true;
}

void CCpuHelper::update(CVmConfiguration &conf, CDispCpuPreferences mask)
{
	CVmRunTimeOptions *r = conf.getVmSettings()->getVmRuntimeOptions();

	FeaturesSetIn<VmFeaturesTag> fs_vm(*r);
	FeaturesSetIn<MaskTag> fs_mask(mask);
	fs_vm = fs_mask;
	r->setCpuFeaturesMaskValid(true);
}

bool CCpuHelper::isMasksEqual(CDispCpuPreferences mask1, CDispCpuPreferences mask2)
{
	FeaturesSetIn<MaskTag> fs1 = fixupValidProperty(mask1);
	FeaturesSetIn<MaskTag> fs2 = fixupValidProperty(mask2);

	return (fs1 == fs2);
}

bool CCpuHelper::sync()
{
	QString out, cmd = QString("%1 --quiet sync").arg(CPUFEATURES_BINARY);

	return !checkBinaryExists(CPUFEATURES_BINARY) || HostUtils::RunCmdLineUtility(cmd, out, 60 * 1000);
}

CCpuPoolInfo *CCpuHelper::getPoolInfo()
{
	std::auto_ptr<CCpuPoolInfo> info(new CCpuPoolInfo());

	info->setName("");
	info->setVendor("");

	if (!checkBinaryExists(CPUPOOLS_BINARY))
		return info.release();

	QProcess process;
	QString cmd = QString("%1 --quiet dump-pool --info").arg(CPUPOOLS_BINARY);

	DumpInfoHandler handler(process, cmd, *info.get());
	if (HostUtils::RunCmdLineUtilityEx(cmd, process, 60 * 1000)(handler).isSuccess())
		return info.release();
	else
		return NULL;

}

bool CCpuHelper::loadPoolsList(QList<CCpuPool> &list)
{
	QString out;

	list.clear();
	if (!checkBinaryExists(CPUPOOLS_BINARY))
		return true;

	if (!HostUtils::RunCmdLineUtility(QString("%1 --quiet list-pools").arg(CPUPOOLS_BINARY), out, 60 * 1000))
		return false;
	QStringList names = out.split("\n", QString::SkipEmptyParts);
	foreach(const QString &name, names)
	{
		CCpuPool pool;

		FeaturesSetIn<PoolFeaturesTag> f(*pool.getCpuMask());
		QString cmd = QString("%1 --quiet dump-pool --mask --pool-name %2").arg(CPUPOOLS_BINARY).arg(name);
		if (!HostUtils::RunCmdLineUtility(cmd, out, 60 * 1000))
			return false;
		if (!f.parse(out))
			return false;

		cmd = QString("%1 --quiet dump-pool --info --pool-name %2").arg(CPUPOOLS_BINARY).arg(name);
		if (!HostUtils::RunCmdLineUtility(cmd, out, 60 * 1000))
			return false;
		if (!parseKeyValue(out, *pool.getCpuPoolInfo()))
			return false;

		list << pool;
	}
	return true;
}

PRL_RESULT CCpuHelper::joinPool()
{
	return execFeaturesCmd(QString("%1 --quiet join").arg(CPUPOOLS_BINARY));
}

PRL_RESULT CCpuHelper::leavePool()
{
	return execFeaturesCmd(QString("%1 --quiet leave").arg(CPUPOOLS_BINARY));
}

PRL_RESULT CCpuHelper::moveToPool(const char *name)
{
	return execFeaturesCmd(QString("%1 --quiet move %2").arg(CPUPOOLS_BINARY).arg(name));
}

PRL_RESULT CCpuHelper::recalcPool(const char *name)
{
	return execFeaturesCmd(QString("%1 --quiet recalc %2").arg(CPUPOOLS_BINARY).arg(name));
}

QSet<QString> CCpuHelper::getDisabledFeatures(const CVmConfiguration &conf)
{
	return getRegisters()->getDisabled(conf.getVmSettings()->getVmRuntimeOptions());
}
