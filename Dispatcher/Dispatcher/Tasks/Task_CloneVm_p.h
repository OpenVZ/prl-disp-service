////////////////////////////////////////////////////////////////////////////////
///
/// Copyright (c) 2005-2015 Parallels IP Holdings GmbH
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
/// Our contact details: Parallels IP Holdings GmbH, Vordergasse 59, 8200
/// Schaffhausen, Switzerland.
///
/// @file
///	Task_CloneVm_p.h
///
/// @brief
///	Private headers of the clone task.
///
/// @author shrike@
///
///
////////////////////////////////////////////////////////////////////////////////

#ifndef __Task_CloneVm_p_H_
#define __Task_CloneVm_p_H_

#include "CDspService.h"
#include "CDspTaskHelper.h"
#include "CDspVmNetworkHelper.h"
#include <prlcommon/Std/noncopyable.h>
#include "Tasks/Mixin_CreateVmSupport.h"
#include <prlcommon/HostUtils/HostUtils.h>
#include <prlxmlmodel/VmConfig/CVmConfiguration.h>
//#include "Libraries/VirtualDisk/DiskStatesManager.h"

class CDspVm;
class Task_CloneVm;
namespace Clone
{
struct Failure;
namespace Sink
{
struct Builder;
} // namespace Sink;

///////////////////////////////////////////////////////////////////////////////
// struct IsSame

template<class T, class U>
struct IsSame
{
	static const bool value = false;
};

template<class T>
struct IsSame<T, T>
{
	static const bool value = true;
};

///////////////////////////////////////////////////////////////////////////////
// struct EnableIfImpl

template<bool B, class T = void>
struct EnableIfImpl
{
};

template<class T>
struct EnableIfImpl<true, T>
{
	typedef T type;
};

///////////////////////////////////////////////////////////////////////////////
// struct EnableIf

template<bool B, class T = void>
struct EnableIf: EnableIfImpl<B, T>
{
};

///////////////////////////////////////////////////////////////////////////////
// struct Facade

struct Facade
{
	explicit Facade(Task_CloneVm& task_): m_task(&task_)
	{
	}

	CAuthHelper* getAuth() const
	{
		return &(getClient()->getAuthHelper());
	}
	Task_CloneVm& getTask() const
	{
		return *m_task;
	}
	SmartPtr<CDspClient> getClient() const;
	const QString& getNewVmName() const;
	const QString& getNewVmUuid() const;
	const QString getDirectory() const
	{
		return getClient()->getVmDirectoryUuid();
	}
	const SmartPtr<CVmConfiguration>& getConfig() const;
	SmartPtr<IOPackage> getRequest() const;
private:
	Task_CloneVm* m_task;
};

///////////////////////////////////////////////////////////////////////////////
// struct Toolkit

struct Toolkit: Facade
{
	explicit Toolkit(Task_CloneVm& task_): Facade(task_)
	{
	}

	bool canRead(const QString& path_) const;
	bool fileExists(const QString& path_) const;
	bool folderExists(const QString& path_) const;
	PRL_RESULT addFile(const QString& path_) const;
	PRL_RESULT addFolder(const QString& path_) const;
	PRL_RESULT getSpaceAvailable(QString path_, quint64& dst_) const;
	static bool equal(const QString& one_, const QString& another_);
};

namespace Source
{
///////////////////////////////////////////////////////////////////////////////
// struct Config

struct Config
{
	explicit Config(Task_CloneVm& task_): m_image(Facade(task_).getConfig())
	{
	}
	explicit Config(const Facade& facade_): m_image(facade_.getConfig())
	{
	}
	explicit Config(const SmartPtr<CVmConfiguration>& image_): m_image(image_)
	{
	}

	QString getName() const
	{
		return m_image->getVmIdentification()->getVmName();
	}
	QString getUuid() const
	{
		return m_image->getVmIdentification()->getVmUuid();
	}
	const CVmHardware& getHardware() const
	{
		return *(m_image->getVmHardwareList());
	}
	SmartPtr<CVmConfiguration> expel() const
	{
		return SmartPtr<CVmConfiguration>(new CVmConfiguration(*m_image));
	}
	bool isLinked() const;
	bool isTemplate() const
	{
		return m_image->getVmSettings()->getVmCommonOptions()->isTemplate();
	}
	bool canChangeSid() const;
	bool hasBootcampDevice() const;
private:
	SmartPtr<CVmConfiguration> m_image;
};

///////////////////////////////////////////////////////////////////////////////
// struct Exclusive

struct Exclusive: private Facade
{
	Exclusive(Task_CloneVm& task_, const QString& uuid_);

	PRL_RESULT lock(PVE::IDispatcherCommands command_);
	PRL_RESULT unlock();
	PVE::IDispatcherCommands getCommand() const
	{
		return m_command;
	}
private:
	QString m_uuid;
	CDspVmDirHelper* m_helper;
	PVE::IDispatcherCommands m_command;
};

///////////////////////////////////////////////////////////////////////////////
// struct Private

struct Private: private Toolkit
{
	explicit Private(Task_CloneVm& task_): Toolkit(task_)
	{
	}

	const QString& getRoot() const
	{
		return m_root;
	}
	QString getPath(const QString& path_) const;
	bool isInside(const QString& path_) const;
	bool checkAccess(const CVmDevice& device_) const;
	bool checkAccess(const CVmHardDisk& device_) const;
	PRL_RESULT setRoot(const QString& uuid_);
	PRL_RESULT getSpaceUsed(quint32 mode_, quint64& dst_) const;
	using Facade::getDirectory;
private:
	bool doGetPath(const QString& path_, QString& dst_) const;

	QString m_root;
	QString m_config;
};

///////////////////////////////////////////////////////////////////////////////
// struct Total

struct Total: noncopyable, private Facade
{
	Total(Task_CloneVm& task_, const Private& private_): Facade(task_),
			m_config(task_), m_lock(task_, m_config.getUuid()),
			m_private(&private_)
	{
	}

	const Config& getConfig() const
	{
		return m_config;
	}
	const Private& getPrivate() const
	{
		return *m_private;
	}
	PRL_RESULT lock(PVE::IDispatcherCommands command_)
	{
		return m_lock.lock(command_);
	}
	PRL_RESULT unlock()
	{
		return m_lock.unlock();
	}
	PRL_RESULT checkAccess() const;
	QList<CVmHardDisk* > copyHardDisks() const;
	PRL_RESULT getVm(SmartPtr<CDspVm>& dst_, bool& unregister_) const;
private:
	PRL_RESULT checkHardwareAccess(Failure& failure_) const;

	Config m_config;
	Exclusive m_lock;
	const Private* m_private;
};

///////////////////////////////////////////////////////////////////////////////
// struct Space

struct Space
{
	explicit Space(quint32 mode_): m_mode(mode_)
	{
	}

	PRL_RESULT operator()(const Total& source_, quint64& dst_) const;
private:
	quint32 m_mode;
};

} // namespace Source

namespace Sink
{
///////////////////////////////////////////////////////////////////////////////
// struct Private

struct Private: private Toolkit, private Mixin_CreateVmSupport
{
	explicit Private(Task_CloneVm& task_);

	const QString& getRoot() const
	{
		return m_folder;
	}
	void setRoot(const QString& root_)
	{
		m_folder = root_;
	}
	QString getPath(const QString& relative_) const;
	QString getCopyPath(const QString& full_) const;
	PRL_RESULT addRoot();
	PRL_RESULT addFile(const QString& relative_);
	PRL_RESULT addFile(SmartPtr<CVmConfiguration> config_);
	PRL_RESULT getSpaceAvailable(quint64& dst_) const;
private:
	QString m_folder;
};

} // namespace Sink

///////////////////////////////////////////////////////////////////////////////
// struct Failure

struct Failure
{
	explicit Failure(Task_CloneVm& task_): m_code(), m_task(&task_)
	{
	}

	PRL_RESULT getCode() const
	{
		return getError().getEventCode();
	}
	Failure& code(PRL_RESULT code_);
	Failure& token(const QString& token_);
	PRL_RESULT operator()();
	PRL_RESULT operator()(PRL_RESULT code_)
	{
		return code(code_)();
	}
	PRL_RESULT operator()(const CVmEvent& src_);
	PRL_RESULT operator()(const QString& first_);
	PRL_RESULT operator()(const Source::Total& source_);
	PRL_RESULT operator()(PRL_RESULT code_, const QString& first_)
	{
		return code(code_)(first_);
	}
	PRL_RESULT operator()(const QString& first_, const QString& second_);
private:
	CVmEvent& getError() const;

	PRL_RESULT m_code;
	Task_CloneVm* m_task;
};

///////////////////////////////////////////////////////////////////////////////
// struct Snapshot

struct Snapshot: private Facade
{
	explicit Snapshot(Task_CloneVm& task_);

	PRL_RESULT take(const Source::Total& source_);
	PRL_RESULT link(const QString& source_, const QString& target_);
private:
	bool skip(const Source::Total& source_) const;

	QString m_uuid;
	Failure m_failure;
//	SmartPtr<CDSManager> m_manager;
};

namespace HardDisk
{
///////////////////////////////////////////////////////////////////////////////
// struct Flavor

struct Flavor
{
	static QString getExternal(const CVmHardDisk& device_, const Facade& work_)
	{
		return getExternal(device_, work_.getNewVmName());
	}
	static QString getExternal(const CVmHardDisk& device_, const QString& name_);
	static QString getLocation(const CVmHardDisk& device_);
	static void update(CVmHardDisk& device_, const QString& name_);
};

///////////////////////////////////////////////////////////////////////////////
// struct Filter

struct Filter: private Facade
{
	explicit Filter(Task_CloneVm& task_);

	bool operator()(const CVmHardDisk& device_, const QString& name_) const;
private:
	PRL_RESULT m_warning;
};

} // namespace HardDisk

namespace Copy
{
///////////////////////////////////////////////////////////////////////////////
// struct Reporter

struct Reporter: noncopyable
{
	Reporter(Failure& failure_, const Source::Total& source_):
		m_failure(&failure_), m_custom(), m_source(&source_)
	{
		reset();
	}

	Reporter& reset()
	{
		return setCustom(m_failure->getCode());
	}
	Reporter& setCustom(PRL_RESULT value_)
	{
		m_custom = value_;
		return *this;
	}
	PRL_RESULT operator()(const QString& source_, const QString& target_) const
	{
		(*m_failure)(source_, target_);
		return m_custom;
	}
	PRL_RESULT operator()(PRL_RESULT error_, const QString& source_,
				const QString& target_) const;
private:
	Failure* m_failure;
	PRL_RESULT m_custom;
	const Source::Total* m_source;
};

///////////////////////////////////////////////////////////////////////////////
// struct Device

template<class D>
struct Device
{
	Device(D& config_, const Source::Total& source_, const Sink::Private& sink_):
		m_config(&config_)
	{
		QString s = getSource(config_);
		if (!source_.getPrivate().isInside(s))
		    return;

		m_source = s;
		m_target = sink_.getCopyPath(s);
	}

	bool isDisabled() const
	{
		return m_source.isEmpty();
	}
	template<class U>
	PRL_RESULT operator()(U copier_, const Reporter& reporter_)
	{
		if (isDisabled())
		    return PRL_ERR_SUCCESS;

		if (Toolkit::equal(m_source, m_target))
		    return reporter_(m_source, m_target);

		return copier_(*m_config, m_target);
	}
private:
	static QString getSource(const D& config_);

	D* m_config;
	QString m_source;
	QString m_target;
};

///////////////////////////////////////////////////////////////////////////////
// struct Batch
// NB. may be it is better to have the begin function to tell explicitly when
// the device type portion starts. this would flush anything uncommited from
// the previous portion.

struct Batch: private Toolkit
{
	Batch(const Toolkit& toolkit_, const Source::Total& source_):
		Toolkit(toolkit_), m_journal(), m_source(&source_)
	{
	}

	void setJournal(QStringList* value_)
	{
		m_journal = value_;
	}
	const Toolkit& getToolkit() const
	{
		return *this;
	}
	Batch& addFile(const QString& source_, const QString& target_);
	Batch& addPrivateFolder(const QString& source_, const QString& target_);
	PRL_RESULT addExternalFolder(const QString& source_, const QString& target_);
	PRL_RESULT addExternalFile(const QString& source_, const QString& target_);
	PRL_RESULT commit(PRL_DEVICE_TYPE kind_, Failure& reporter_)
	{
		return commit(kind_, Reporter(reporter_, *m_source));
	}
	PRL_RESULT commit(PRL_DEVICE_TYPE kind_, const Reporter& reporter_);
private:
	typedef QPair<QString, QString> item_type;

	QList<item_type> m_files;
	QList<item_type> m_folders;
	QStringList* m_journal;
	const Source::Total* m_source;
};

///////////////////////////////////////////////////////////////////////////////
// struct Floppy

struct Floppy
{
	explicit Floppy(Batch& batch_): m_batch(&batch_)
	{
	}
	static QString getLocation(const CVmFloppyDisk& device_)
	{
		return device_.getUserFriendlyName();
	}
	PRL_RESULT operator()(CVmFloppyDisk& device_, const QString& target_);
	PRL_RESULT operator()(CVmFloppyDisk& )
	{
		return PRL_ERR_SUCCESS;
	}
private:
	Batch* m_batch;
};

} // namespace Copy

namespace HardDisk
{
///////////////////////////////////////////////////////////////////////////////
// struct Copy

struct Copy
{
	explicit Copy(Clone::Copy::Batch& batch_): m_batch(&batch_)
	{
	}

	PRL_RESULT operator()(CVmHardDisk& device_, const Clone::Copy::Reporter& reporter_);
	PRL_RESULT operator()(CVmHardDisk& device_, const QString& target_);
private:
	Clone::Copy::Batch* m_batch;
};

///////////////////////////////////////////////////////////////////////////////
// struct Link

struct Link
{
	Link(const Toolkit& toolkit_, Snapshot& snapshot_, QStringList& journal_);

	PRL_RESULT operator()(CVmHardDisk& device_, const Clone::Copy::Reporter& reporter_);
	PRL_RESULT operator()(CVmHardDisk& device_, const QString& target_);
private:
	Toolkit m_toolkit;
	Snapshot* m_snapshot;
	QStringList* m_journal;
};

} // namespace HardDisk

///////////////////////////////////////////////////////////////////////////////
// struct Content

struct Content: private Copy::Batch
{
	Content(const Source::Total& source_, const Sink::Private& sink_,
		const ::Clone::Toolkit& toolkit_);

	const QStringList& getJournal() const
	{
		return m_journal;
	}
	PRL_RESULT copyInternals();
	PRL_RESULT copyNvram(CVmStartupBios& bios_);
	PRL_RESULT copyHardDisks(CVmHardware& hardware_);
	PRL_RESULT copySnapshots();
	PRL_RESULT linkHardDisks(Snapshot& snapshot_, CVmHardware& hardware_);
	PRL_RESULT copyFloppyDisks(CVmHardware& hardware_);
	PRL_RESULT copySerialPorts(CVmHardware& hardware_);
	PRL_RESULT copyParallelPorts(CVmHardware& hardware_);
private:
	PRL_RESULT copyFile(const char* name_);
	template<class U>
	PRL_RESULT copyHardDisks(CVmHardware& hardware_, U utility_);

	Failure m_failure;
	Sink::Private m_sink;
	QStringList m_journal;
	const Source::Total* m_source;
};

namespace Source
{
///////////////////////////////////////////////////////////////////////////////
// struct Work

struct Work: private Facade
{
	Work(Task_CloneVm& task_, Source::Total& source_): Facade(task_),
		m_source(&source_), m_space(new Space(0)),
		m_lock(PVE::DspCmdDirVmClone)
	{
	}

	void setSpace(Space* space_);
	void setSnapshot(Snapshot* snapshot_);
	PRL_RESULT operator()(Sink::Builder& sink_);
private:
	Source::Total* m_source;
	QScopedPointer<Space> m_space;
	PVE::IDispatcherCommands m_lock;
	QScopedPointer<Snapshot> m_snapshot;
};

namespace Vm
{
///////////////////////////////////////////////////////////////////////////////
// struct General

struct General
{
};

///////////////////////////////////////////////////////////////////////////////
// struct Linked

struct Linked
{
};

///////////////////////////////////////////////////////////////////////////////
// struct Bootcamp

struct Bootcamp
{
};

} // namespace Vm

namespace Template
{
///////////////////////////////////////////////////////////////////////////////
// struct Local

struct Local
{
};

///////////////////////////////////////////////////////////////////////////////
// class Shared

struct Shared
{
};

} // namespace Template
} // namespace Source

namespace Sink
{
///////////////////////////////////////////////////////////////////////////////
// struct Result

struct Result
{
	Result(SmartPtr<CVmConfiguration> config_, SmartPtr<QStringList> trash_):
		m_trash(trash_), m_config(config_)
	{
	}

	const SmartPtr<CVmConfiguration>& getConfig() const
	{
		return m_config;
	}
	void activate()
	{
		if (m_trash.isValid())
			m_trash->clear();
	}
private:
	SmartPtr<QStringList> m_trash;
	SmartPtr<CVmConfiguration> m_config;
};

///////////////////////////////////////////////////////////////////////////////
// struct Builder

struct Builder: private Facade
{
	Builder(Task_CloneVm& task_, const SmartPtr<CVmConfiguration>& grub_,
		Private& private_):
		Facade(task_), m_private(&private_), m_grub(grub_)
	{
	}

	PRL_RESULT addRoot();
	PRL_RESULT copyContent(const Source::Total& source_, Snapshot* snapshot_);
	PRL_RESULT saveConfig(const QString& name_, const QString& uuid_);
	PRL_RESULT checkSpace(const Source::Total& source_, const Source::Space& query_);

	Result getResult();
private:
	static void cleanse(QStringList* trash_);

	Private* m_private;
	SmartPtr<QStringList> m_trash;
	SmartPtr<CVmConfiguration> m_grub;
	SmartPtr<CVmConfiguration> m_config;
};

///////////////////////////////////////////////////////////////////////////////
// struct Dress

struct Dress: private Facade
{
	Dress(Task_CloneVm& task_, Private& private_,
		SmartPtr<CVmConfiguration> config_);

	void undoLibvirtDomain();
	void undoDirectoryItem();
	void undoClusterResource();
	PRL_RESULT setDirectoryItem();
	PRL_RESULT addClusterResource();
	PRL_RESULT changeSid();
	PRL_RESULT importBootcamps();
	PRL_RESULT addLibvirtDomain();
private:
	static CDspService& s();

	QString m_path;
	SmartPtr<CVmConfiguration> m_config;
};

namespace Vm
{
///////////////////////////////////////////////////////////////////////////////
// struct General

struct General
{
};

///////////////////////////////////////////////////////////////////////////////
// struct Linked

struct Linked
{
};

///////////////////////////////////////////////////////////////////////////////
// struct Bootcamp

struct Bootcamp
{
};

///////////////////////////////////////////////////////////////////////////////
// struct Base

template<class T>
struct Base
{
protected:
	static bool getBootcamps()
	{
		return false;
	}
};

template<>
struct Base<General>
{
	Base(): m_bootcamps()
	{
	}

	void setBootcamps(bool value_)
	{
		m_bootcamps = value_;
	}
protected:
	bool getBootcamps() const
	{
		return m_bootcamps;
	}
private:
	bool m_bootcamps;
};

} // namespace Vm

namespace Template
{
///////////////////////////////////////////////////////////////////////////////
// struct Local

struct Local
{
};

///////////////////////////////////////////////////////////////////////////////
// class Shared

struct Shared
{
};

} // namespace Template

///////////////////////////////////////////////////////////////////////////////
// struct Flavor

template<class T, class Enabled = void>
struct Flavor
{
};

template<class T>
struct Flavor<T, typename EnableIf<IsSame<T, Vm::General>::value ||
			IsSame<T, Vm::Linked>::value ||
			IsSame<T, Vm::Bootcamp>::value>::type>: Vm::Base<T>
{
	explicit Flavor(bool sid_): m_sid(sid_)
	{
	}

	static void prepare(const SmartPtr<CVmConfiguration>& grub_)
	{
		// #PDFM-20418
		grub_->getVmIdentification()->setCreationDate(QDateTime::currentDateTime());
		// #PSBM-415
		grub_->getVmIdentification()->setVmUptimeInSeconds(0);
		grub_->getVmIdentification()->setVmUptimeStartDateTime();
		grub_->getVmSettings()->getVmCommonOptions()->setTemplate(false);
		grub_->getVmIdentification()->setLinkedVmUuid(QString());
		grub_->getVmIdentification()->setServerUuid(CDspService::instance()
			->getDispConfigGuard().getDispConfig()
			->getVmServerIdentification()->getServerUuid());
		HostUtils::MacPrefixType prefix = HostUtils::MAC_PREFIX_VM;
		if (PVT_CT == grub_->getVmType())
			prefix = HostUtils::MAC_PREFIX_CT;

		foreach(CVmGenericNetworkAdapter* a, grub_->getVmHardwareList()->m_lstNetworkAdapters)
		{
			// regenerate mac address for cloned VM or CT
			a->setMacAddress(HostUtils::generateMacAddress(prefix));
			a->setHostInterfaceName
				(HostUtils::generateHostInterfaceName(a->getMacAddress()));
		}
		CDspVmNetworkHelper::updateHostMacAddresses(grub_, NULL, HMU_CHECK_NONEMPTY);
	}
	static SmartPtr<CVmConfiguration> getGrub(Task_CloneVm& task_)
	{
		Facade f(task_);
		// create new VM Configuration object
		SmartPtr<CVmConfiguration> output = Source::Config(f).expel();
		// PWE-3866
/*
		if ( PAM_WORKSTATION_EXTREME == ParallelsDirs::getAppExecuteMode() )
		{
			CVmCommonOptions* o = output->getVmSettings()->getVmCommonOptions();
			o->setVmColor(CDspService::instance()->getVmDirHelper().getUniqueVmColor(
					f.getDirectory() ) );
		}
*/
		prepare(output);
		// reset additional parameters in VM configuration
		CDspService::instance()->getVmDirHelper().resetAdvancedParamsFromVmConfig(output);
		CDspService::instance()->getVmDirHelper().resetSecureParamsFromVmConfig(output);
		return output;
	}

	PRL_RESULT operator()(Dress dress_)
	{
		PRL_RESULT e;
		stepList_type x = getSteps();
		stepList_type::const_iterator i = x.begin(), f = x.end();
		for (; i != f; ++i)
		{
			e = (dress_.*(i->first))();
			if (PRL_FAILED(e))
				goto undo;
		}
		return PRL_ERR_SUCCESS;
undo:
		f = x.begin();
		do
		{
			if (NULL != i->second)
				(dress_.*(i->second))();
		} while(f != i--);
		return e;
	}
private:
	typedef PRL_RESULT (Dress::* do_type)();
	typedef void (Dress::* undo_type)();
	typedef QPair<do_type, undo_type> step_type;
	typedef QList<step_type> stepList_type;

	stepList_type getSteps() const
	{
		stepList_type output;
		output.push_back(step_type(&Dress::addLibvirtDomain,
					&Dress::undoLibvirtDomain));
		output.push_back(step_type(&Dress::addClusterResource,
					&Dress::undoClusterResource));
		output.push_back(step_type(&Dress::setDirectoryItem,
					&Dress::undoDirectoryItem));
		if (this->getBootcamps())
		{
			output.push_back(
				step_type(&Dress::importBootcamps, NULL));
		}
		if (m_sid)
		{
			output.push_back(
				step_type(&Dress::changeSid, NULL));
		}
		return output;
	}

	bool m_sid;
};

template<class T>
struct Flavor<T, typename EnableIf<IsSame<T, Template::Local>::value ||
			IsSame<T, Template::Shared>::value>::type>
{
	static SmartPtr<CVmConfiguration> getGrub(Task_CloneVm& task_)
	{
		SmartPtr<CVmConfiguration> output = Flavor<Vm::General>::getGrub(task_);
		output->getVmSettings()->getVmCommonOptions()->setTemplate(true);
		foreach(CVmGenericNetworkAdapter* a, output->getVmHardwareList()->m_lstNetworkAdapters)
		{
			// reset IP addresses for templates
			a->setNetAddresses();
		}
		return output;
	}

	PRL_RESULT operator()(Dress dress_)
	{
		return dress_.setDirectoryItem();
	}
};

} // namespace Sink


namespace Work
{
///////////////////////////////////////////////////////////////////////////////
// struct NotSupported

template<class S, class D, class Enabled = void>
struct NotSupported
{
	static const bool value = false;
};

template<class S, class D>
struct NotSupported<S, D, typename EnableIf<
	(IsSame<S, Source::Vm::Bootcamp>::value && IsSame<D, Sink::Vm::Linked>::value) ||
	(IsSame<S, Source::Template::Shared>::value && IsSame<D, Sink::Vm::Linked>::value) ||
	(IsSame<S, Source::Vm::General>::value && IsSame<D, Sink::Vm::Bootcamp>::value) ||
	(IsSame<S, Source::Vm::Linked>::value && IsSame<D, Sink::Vm::Bootcamp>::value) ||
	(IsSame<S, Source::Template::Shared>::value && IsSame<D, Sink::Vm::Bootcamp>::value) ||
	(IsSame<S, Source::Vm::Linked>::value && IsSame<D, Sink::Template::Local>::value) ||
	(IsSame<S, Source::Vm::Linked>::value && IsSame<D, Sink::Template::Shared>::value) ||
	(IsSame<S, Source::Vm::Bootcamp>::value && IsSame<D, Sink::Template::Shared>::value)>::type>
{
	static const bool value = true;
};

///////////////////////////////////////////////////////////////////////////////
// struct Small

template<class S, class D, class Enabled = void>
struct Small;

template<>
struct Small<Source::Vm::Linked, Source::Vm::General>
{
	static PRL_RESULT do_(Task_CloneVm& , Source::Work& , Sink::Flavor<Source::Vm::General>& )
	{
		return PRL_ERR_UNIMPLEMENTED;
	}
};

template<class S, class D>
struct Small<S, D, typename EnableIf<NotSupported<S, D>::value>::type>
{
	static PRL_RESULT do_(Task_CloneVm& , Source::Work& , Sink::Flavor<D>& )
	{
		return PRL_ERR_VM_REQUEST_NOT_SUPPORTED;
	}
};

template<class S, class D>
struct Small<S, D, typename EnableIf<!NotSupported<S, D>::value>::type>
{
	static PRL_RESULT do_(Task_CloneVm& task_, Source::Work& source_, Sink::Flavor<D>& sink_)
	{
		Sink::Private p(task_);
		Sink::Builder b(task_, Sink::Flavor<D>::getGrub(task_), p);
		PRL_RESULT e = source_(b);
		if (PRL_FAILED(e))
			return e;

		Sink::Result r = b.getResult();
		e = sink_(Sink::Dress(task_, p, r.getConfig()));
		if (PRL_FAILED(e))
			return e;

		r.activate();
		return PRL_ERR_SUCCESS;
	}
};

///////////////////////////////////////////////////////////////////////////////
// struct SnapshotAware

template<class T>
struct SnapshotAware
{
	static PRL_RESULT do_(Task_CloneVm& task_, Source::Work& source_, Sink::Flavor<Sink::Vm::Linked>& sink_)
	{
		//In linked clone case skip verification due non significant disk space required
		//See https://bugzilla.sw.ru/show_bug.cgi?id=473298 for more details
		//FIXME: in any case it would be better to calculate necessary disk space except
		//virtual hard disks size. Actually procedure of check available disk space
		//should be the same as at Task_CreateSnapshot but it's fake on the moment and
		//should be fixed at first.
		source_.setSpace(NULL);
		source_.setSnapshot(new Snapshot(task_));
		return Small<T, Sink::Vm::Linked>::do_(task_, source_, sink_);
	}
};

///////////////////////////////////////////////////////////////////////////////
// struct Great

template<class S, class D>
struct Great
{
	static PRL_RESULT do_(Task_CloneVm& task_, Source::Work& source_, Sink::Flavor<D>& sink_)
	{
		return Small<S, D>::do_(task_, source_, sink_);
	}
};

template<>
struct Great<Source::Vm::General, Sink::Vm::General>
{
	static PRL_RESULT do_(Task_CloneVm& task_, Source::Work& source_, Sink::Flavor<Sink::Vm::General>& sink_)
	{
		sink_.setBootcamps(false);
		return Small<Source::Vm::General, Sink::Vm::General>::do_(task_, source_, sink_);
	}
};

template<>
struct Great<Source::Vm::Bootcamp, Sink::Vm::General>
{
	static PRL_RESULT do_(Task_CloneVm& task_, Source::Work& source_, Sink::Flavor<Sink::Vm::General>& sink_)
	{
		sink_.setBootcamps(true);
		source_.setSpace(new Source::Space(PSV_CALC_BOOT_CAMP_SIZE));
		return Small<Source::Vm::Bootcamp, Sink::Vm::General>::do_(task_, source_, sink_);
	}
};

template<>
struct Great<Source::Vm::General, Sink::Vm::Linked>: SnapshotAware<Source::Vm::General>
{
};

template<>
struct Great<Source::Template::Local, Sink::Vm::Linked>: SnapshotAware<Source::Template::Local>
{
};

} // namespace Work
} // namespace Clone

#endif // __Task_CloneVm_p_H_
