///////////////////////////////////////////////////////////////////////////////
///
/// @file CDspBackupDevice.cpp
///
/// Custom storage support for HDD
///
/// @author ibazhitov
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
///////////////////////////////////////////////////////////////////////////////

#include "CDspService.h"
#include "CDspBackupDevice.h"
#include "Libraries/PrlCommonUtils/CFileHelper.h"
#include "Libraries/DispToDispProtocols/CVmBackupProto.h"
#include "prlxmlmodel/VmConfig/CVmConfiguration.h"
#include "prlxmlmodel/ParallelsObjects/CXmlModelHelper.h"
#include "Interfaces/ParallelsDispToDispProto.h"
#include "Libraries/Buse/Buse.h"

using namespace Parallels;

namespace Backup
{
namespace Device
{
namespace Details
{
///////////////////////////////////////////////////////////////////////////////
// struct Finding

bool Finding::isKindOf() const
{
	QUrl u;
	/* XXX: only "backup://" URLs are supported as a custom storage */
	return getStorageUrl(u) && u.scheme() == "backup";
}

bool Finding::isEnabled() const
{
	return (m_object->getEnabled() == PVE::DeviceEnabled)
		&& (m_object->getConnected() == PVE::DeviceConnected);
}

bool Finding::getStorageUrl(QUrl& dst_) const
{
	QUrl u = m_object->getStorageURL();
	if (u.isEmpty() || !u.isValid())
		return false;

	dst_ = u;
	return true;
}

///////////////////////////////////////////////////////////////////////////////
// struct IndexInRange

IndexInRange::IndexInRange(const QList<CVmHardDisk* >& range_)
{
	std::transform(range_.begin(), range_.end(),
		std::inserter(m_good, m_good.end()),
		std::mem_fun(&CVmHardDisk::getIndex));
}

///////////////////////////////////////////////////////////////////////////////
// struct Batch

void Batch::sweep()
{
	foreach(Event::Base *b, m_store)
	{
		delete b;
	}
	m_store.clear();
}

PRL_RESULT Batch::operator()(Event::Visitor& visitor_)
{
	PRL_RESULT output = PRL_ERR_SUCCESS;
	foreach(Event::Base *b, m_store)
	{
		if (PRL_FAILED(output = b->accept(visitor_)))
			break;
	}
	return output;
}

///////////////////////////////////////////////////////////////////////////////
// struct Difference

Difference::Difference(const operand_type& from_, const operand_type& to_,
	const Event::Factory& factory_)
	: m_from(from_), m_to(to_), m_factory(factory_), m_validated(false)
{
	std::partition(m_to.begin(), m_to.end(), IndexInRange(m_from));
	std::partition(m_from.begin(), m_from.end(), IndexInRange(m_to));
}

PRL_RESULT Difference::getNovel(Batch& batch_)
{
	PRL_RESULT output = validate();
	if (PRL_FAILED(output))
		return output;
	operand_type::iterator a = std::find_if(m_to.begin(), m_to.end(),
		std::not1(IndexInRange(m_from)));
	for (operand_type::iterator x = a, e = m_to.end(); x != e; ++x)
		batch_.addEvent(m_factory.novel(*x));
	return output;
}

PRL_RESULT Difference::getRemoved(Batch& batch_)
{
	PRL_RESULT output = validate();
	if (PRL_FAILED(output))
		return output;
	operand_type::iterator a = std::find_if(m_from.begin(), m_from.end(),
		std::not1(IndexInRange(m_to)));
	for (operand_type::iterator x = a, e = m_from.end(); x != e; ++x)
		batch_.addEvent(m_factory.removed(*x));
	return output;
}

PRL_RESULT Difference::getUpdated(Batch *enabled_, Batch *disabled_)
{
	PRL_RESULT output = validate();
	if (PRL_FAILED(output))
		return output;
	operand_type::iterator a = std::find_if(m_from.begin(), m_from.end(),
		std::not1(IndexInRange(m_to)));
	operand_type::iterator b = std::find_if(m_to.begin(), m_to.end(),
		std::not1(IndexInRange(m_from)));
	for (operand_type::iterator x = m_from.begin(); x != a; ++x)
	{
		operand_type::iterator y = std::find_if(m_to.begin(), b, IndexInRange(*x));
		Event::Factory::updated_type z = m_factory.updated(*x, *y);
		if (z.first && enabled_)
			enabled_->addEvent(Batch::store_type() << z.first);
		else
			delete z.first;
		if (z.second && disabled_)
			disabled_->addEvent(Batch::store_type() << z.second);
		else
			delete z.second;
	}
	return output;
}

PRL_RESULT Difference::validate()
{
	if (!m_validated)
	{
		m_result = Validator(m_from, m_to)();
		m_validated = true;
	}
	return m_result;
}

///////////////////////////////////////////////////////////////////////////////
// struct Transition

Transition::Transition(const Difference& diff_, Event::Visitor &visitor_,
	CVmEvent *topic_)
	: m_diff(diff_), m_visitor(visitor_), m_topic(topic_)
{
}

PRL_RESULT Transition::replace()
{
	Batch b;
	PRL_RESULT output = m_diff.getNovel(b);
	if (PRL_FAILED(output))
		return output;
	output = m_diff.getUpdated(&b, &b);
	if (PRL_FAILED(output))
		return output;
	return do_(b);
}

PRL_RESULT Transition::remove()
{
	Batch b;
	PRL_RESULT output = m_diff.getRemoved(b);
	if (PRL_FAILED(output))
		return output;
	return do_(b);
}

PRL_RESULT Transition::plant()
{
	Batch b;
	PRL_RESULT output = m_diff.getNovel(b);
	if (PRL_FAILED(output))
		return output;
	output = m_diff.getUpdated(&b, NULL);
	if (PRL_FAILED(output))
		return output;
	return do_(b);
}

PRL_RESULT Transition::do_(Batch& batch_)
{
	m_visitor.setTopic(m_topic);
	PRL_RESULT output = batch_(m_visitor);
	m_visitor.setTopic(NULL);
	return output;
}

///////////////////////////////////////////////////////////////////////////////
// struct Validator

bool Validator::checkModified()
{
	foreach(const CVmHardDisk *d, m_old)
	{
		if (check(Finding(*d), Url::Changed()))
			return true;
	}
	return false;
}

bool Validator::checkDuplicated()
{
	foreach(const CVmHardDisk *d, m_new)
	{
		Finding f(*d);
		if (f.getUrlPath().isEmpty())
			continue;
		if (check(f, Url::Same()))
			return true;
	}
	return false;
}

PRL_RESULT Validator::operator()()
{
	if (checkModified())
		return PRL_ERR_ATTACH_BACKUP_URL_CHANGE_PROHIBITED;
	if (checkDuplicated())
		return PRL_ERR_ATTACH_BACKUP_ALREADY_ATTACHED;
	return PRL_ERR_SUCCESS;
}

} // namespace Details

namespace Event
{
///////////////////////////////////////////////////////////////////////////////
// struct Base

Base::Base(const QString& uuid_, const CVmHardDisk& model_): m_uuid(uuid_),
	m_model(&model_)
{
}

Base::~Base()
{
}

///////////////////////////////////////////////////////////////////////////////
// struct Setup

PRL_RESULT Setup::accept(Visitor& visitor_)
{
	return visitor_.visit(*this);
}

PRL_RESULT Setup::setModel(Attach::Wrap::Hdd& src_)
{
	PRL_RESULT output = src_.save(*m_model);
	if (PRL_FAILED(output))
	{
		src_.disable();
		src_.getImage().remove();
		src_.destroy();
	}
	return output;
}

///////////////////////////////////////////////////////////////////////////////
// struct Enable

PRL_RESULT Enable::accept(Visitor& visitor_)
{
	return visitor_.visit(*this);
}

///////////////////////////////////////////////////////////////////////////////
// struct Disconnect

template<bool F>
Disconnect<F>::Disconnect(const QString& uuid_, const QString& home_, const CVmHardDisk& model_):
	Base(uuid_, model_), m_home(home_)
{
}

template<bool F>
PRL_RESULT Disconnect<F>::accept(Visitor& visitor_)
{
	return visitor_.visit(*this);
}

///////////////////////////////////////////////////////////////////////////////
// struct Cleanup

PRL_RESULT Cleanup::accept(Visitor& visitor_)
{
	return visitor_.visit(*this);
}

///////////////////////////////////////////////////////////////////////////////
// struct Traits<Setup>

QString Traits<Setup>::getRequestBody(const Setup& event_)
{
	return CDispToDispProtoSerializer::CreateVmBackupAttachCommand(
			event_.getVmUuid(),
			ElementToString<const CVmHardDisk*>(&event_.getModel(), XML_VM_CONFIG_EL_HARD_DISK),
			event_.getVmHome())->GetCommand()->toString();
}

CDspTaskHelper* Traits<Setup>::getTask(SmartPtr<CDspClient> user_,
					const SmartPtr<IOPackage>& package_)
{
	return new (std::nothrow)Task_AttachVmBackup(user_, package_);
}

PRL_RESULT Traits<Setup>::handle(const SmartPtr<CDspTaskHelper>& src_, Setup& dst_)
{
	Task_AttachVmBackup *t = static_cast<Task_AttachVmBackup *>(src_.getImpl());
	return dst_.setModel(*t->getResult());
}

///////////////////////////////////////////////////////////////////////////////
// struct Traits<Enable>

QString Traits<Enable>::getRequestBody(const Enable& event_)
{
	return CDispToDispProtoSerializer::CreateVmBackupConnectSourceCommand(
			event_.getVmUuid(),
			ElementToString<const CVmHardDisk*>(&event_.getModel(), XML_VM_CONFIG_EL_HARD_DISK))
			->GetCommand()->toString();
}

CDspTaskHelper* Traits<Enable>::getTask(SmartPtr<CDspClient> user_,
					const SmartPtr<IOPackage>& package_)
{
	return new (std::nothrow)Task_ConnectVmBackupSource(user_, package_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Disconnector

template<bool F>
PRL_RESULT Disconnector::operator()(const Disconnect<F>& event_)
{
	m_hdd.reset();
	Attach::Wrap::Factory f(event_.getVmUuid());
	PRL_RESULT e = f.create(event_.getModel(), event_.getVmHome());
	if (PRL_FAILED(e))
	{
		if (NULL != m_topic)
			m_topic->setEventCode(e);

		return e;
	}
	m_hdd.reset(f.getResult());
	m_hdd->disable();
	if (m_hdd->getImage().exists())
		m_hdd->getImage().remove();

	return PRL_ERR_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
// struct Handler

template<class T>
PRL_RESULT Handler::delegate(const T& event_)
{
	SmartPtr<CDspTaskHelper> x;
	PRL_RESULT output = PRL_ERR_UNINITIALIZED;
	if (!m_agent.isNull())
	{
		output = (*m_agent)(event_);
		if (PRL_SUCCEEDED(output))
			return PRL_ERR_SUCCESS;

		x = m_agent->getResult();
	}
	if (NULL != m_topic)
	{
		if (x.isValid())
			m_topic->fromString(x->getLastError()->toString());
		else
			m_topic->setEventCode(output);
	}
	return output;
}

PRL_RESULT Handler::visit(Setup& event_)
{
	PRL_RESULT e = delegate(event_);
	if (PRL_FAILED(e))
		return e;

	return Traits<Setup>::handle(m_agent->getResult(), event_);
}

PRL_RESULT Handler::visit(const Enable& event_)
{
	return delegate(event_);
}

PRL_RESULT Handler::visit(const Disable& event_)
{
	return Disconnector(m_topic)(event_);
}

PRL_RESULT Handler::visit(const Teardown& event_)
{
	Disconnector u(m_topic);
	PRL_RESULT e = u(event_);
	if (PRL_FAILED(e))
		return e;

	u.getHdd()->destroy();
	return PRL_ERR_SUCCESS;
}

PRL_RESULT Handler::visit(const Cleanup& event_)
{
	Attach::Source::Resource r;
	PRL_RESULT e = r.setURL(event_.getModel().getStorageURL());
	if (PRL_FAILED(e)) {
		if (m_topic)
			m_topic->setEventCode(e);
		return e;
	}

	Attach::Wrap::Image image(event_.getVmUuid(), r.getBackupId(), r.getDiskId());
	if (image.exists())
		image.remove();
	return PRL_ERR_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
// struct EditVm

PRL_RESULT EditVm::visit(const Disable& event_)
{
	if (CDspVm::getVmState(event_.getVmUuid(), m_uuid) == VMS_RUNNING)
		return PRL_ERR_SUCCESS;
	return m_handler.visit(event_);
}

///////////////////////////////////////////////////////////////////////////////
// struct Factory

Factory::result_type Factory::novel(CVmHardDisk* disk_) const
{
	result_type output;
	output << new Setup(m_uuid, m_home, *disk_);
	if (!Details::Finding(*disk_).isEnabled())
		output << new Disable(m_uuid, m_home, *disk_);

	return output;
}

Factory::result_type Factory::removed(CVmHardDisk* disk_) const
{
	result_type output;
	PRL_VM_TYPE type = PVT_VM;
	CDspService::instance()->getVmDirManager().getVmTypeByUuid(m_uuid, type);
	switch(type)
	{
	case PVT_VM:
		output << new Teardown(m_uuid, m_home, *disk_);
		break;
	case PVT_CT:
		output << new Cleanup(m_uuid, *disk_);
		break;
	}
	return output;
}

Factory::updated_type Factory::updated(CVmHardDisk* from_, CVmHardDisk* to_) const
{
	updated_type output;
	bool x = Details::Finding(*to_).isEnabled();
	if (x != Details::Finding(*from_).isEnabled())
	{
		if (x)
			output.first = new Enable(m_uuid, *to_);
		else
			output.second = new Disable(m_uuid, m_home, *to_);
	}
	return output;
}

Factory::result_type Factory::disabled(CVmHardDisk* disk_) const
{
	result_type output;
	if (!Details::Finding(*disk_).isEnabled())
		output << new Disable(m_uuid, m_home, *disk_);
	return output;
}

} // namespace Event

namespace Agent
{
///////////////////////////////////////////////////////////////////////////////
// struct Alone

SmartPtr<IOPackage> Alone::getRequest(int type_, const QString& body_)
{
	return DispatcherPackage::createInstance(type_, body_);
}

PRL_RESULT Alone::run(CDspTaskHelper& task_)
{
	task_.start();
	task_.wait();
	return task_.getLastError()->getEventCode();
}

///////////////////////////////////////////////////////////////////////////////
// struct Unit

SmartPtr<IOPackage> Nested::getRequest(int type_, const QString& body_) const
{
	SmartPtr<IOPackage> output = DispatcherPackage::duplicateInstance(
			m_parent->getRequestPackage(), body_);
	if (output.isValid())
		output->header.type = type_;

	return output;
}

///////////////////////////////////////////////////////////////////////////////
// struct Unit

template<class T, class U>
PRL_RESULT Unit::do_(T policy_, const U& event_)
{
	SmartPtr<IOPackage> p = policy_.getRequest(
					Event::Traits<U>::getRequestType(),
					Event::Traits<U>::getRequestBody(event_));
	if (!p.isValid())
		return PRL_ERR_OUT_OF_MEMORY;

	CDspTaskHelper* t = Event::Traits<U>::getTask(m_user, p);
	if (NULL == t)
		return PRL_ERR_OUT_OF_MEMORY;

	m_result = CDspService::instance()->getTaskManager().registerTask(t);
	if (!m_result.isValid())
		return PRL_ERR_OPERATION_WAS_CANCELED;

	return policy_.run(*m_result);
}

} // namespace Agent

///////////////////////////////////////////////////////////////////////////////
// struct Dao

QList<CVmHardDisk* > Dao::getAll() const
{
	return getEnabled() + getDisabled();
}

QList<CVmHardDisk* > Dao::getEnabled() const
{
	QList<CVmHardDisk* > output;
	foreach(CVmHardDisk* d, getStore())
	{
		Details::Finding f(d);
		if (f.isKindOf() && f.isEnabled())
			output << d;
	}
	return output;
}

QList<CVmHardDisk* > Dao::getDisabled() const
{
	QList<CVmHardDisk* > output;
	foreach(CVmHardDisk* d, getStore())
	{
		Details::Finding f(d);
		if (f.isKindOf() && !f.isEnabled())
			output << d;
	}
	return output;
}

bool Dao::deleteAll()
{
	QList<CVmHardDisk* > a = getAll();
	foreach(CVmHardDisk* d, a)
	{
		getStore().removeOne(d);
		delete d;
	}
	return !a.isEmpty();
}


///////////////////////////////////////////////////////////////////////////////
// struct Service

Service::Service(const Dao::dataSource_type& dataSource_): m_dao(dataSource_),
	m_home(dataSource_->getVmIdentification()->getHomePath()),
	m_uuid(dataSource_->getVmIdentification()->getVmUuid()), m_topic(),
	m_visitor(new Event::Handler())
{
	PRL_VM_TYPE type = PVT_VM;
	CDspService::instance()->getVmDirManager().getVmTypeByUuid(m_uuid, type);
	if (type == PVT_VM)
		m_home = CFileHelper::GetFileRoot(m_home);
}

void Service::enable()
{
	foreach(CVmHardDisk* d, m_dao.getEnabled())
	{
		(void)m_visitor->visit(Event::Enable(m_uuid, *d));
	}
}

void Service::disable()
{
	foreach(CVmHardDisk* d, m_dao.getEnabled())
	{
		(void)m_visitor->visit(Event::Disable(m_uuid, m_home, *d));
	}
}

void Service::teardown()
{
	foreach(CVmHardDisk* d, m_dao.getAll())
	{
		(void)m_visitor->visit(Event::Teardown(m_uuid, m_home, *d));
	}
}

Details::Transition Service::getTransition(const Dao::dataSource_type& new_)
{
	Details::Difference d(m_dao.getAll(), Dao(new_).getAll(),
		Event::Factory(m_uuid, m_home));
	return Details::Transition(d, *m_visitor.data(), m_topic);
}

PRL_RESULT Service::setDifference(const Dao::dataSource_type& new_)
{
	Details::Transition t = getTransition(new_);
	PRL_RESULT output = t.replace();
	if (PRL_FAILED(output))
		return output;
	return t.remove();
}

bool Service::isAttached(const QString& backupId)
{
	QStringList found;
	PRL_RESULT res = Buse::Buse().find(backupId, found);
	if (PRL_FAILED(res))
		return false;
	return !found.isEmpty();
}

} // namespace Device
} // namespace Backup

