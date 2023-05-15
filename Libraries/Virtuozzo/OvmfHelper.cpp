#include "OvmfHelper.h"
#include <prlcommon/Interfaces/VirtuozzoNamespace.h>
#include <prlcommon/HostUtils/HostUtils.h>


constexpr const char OVMF_VARS_SECBOOT[]	= "/usr/share/OVMF/OVMF_VARS.secboot.fd";
constexpr const char OVMF_CODE_SECBOOT[]	= "/usr/share/OVMF/OVMF_CODE.secboot.fd";

constexpr const char OVMF_VARS_4M[]			= "/usr/share/OVMF/OVMF_VARS_4M.fd";
constexpr const char OVMF_CODE_4M[]			= "/usr/share/OVMF/OVMF_CODE_4M.fd";

constexpr const char OVMF_VARS_OLD[]			= "/usr/share/OVMF/OVMF_VARS.fd";
constexpr const char OVMF_CODE_OLD[]			= "/usr/share/OVMF/OVMF_CODE.fd";

constexpr const char OVMF_UEFI_SHELL_ISO_PATH[]	= "/usr/share/OVMF/UefiShell.iso";

constexpr const int DEFAULT_WAIT_TIMER					= 20 * 1000;
constexpr const int DEFAULT_OUTPUT_WAIT_TIMER			= 10;
constexpr const int DEFAULT_STORAGE_SIZE				= 256 * 1024 * 1024;
constexpr const int UEFISHELL_WELCOME_MESSAGE_SIZE		= 2000;


QMutex NvramUpdater::s_mutexNvramList;
QList<QString> NvramUpdater::s_NvramList;

// return NVRAM template for vz8 and vz7
QString OVMF::getTemplate(Chipset_type machine_type)
{
	return machine_type == Chipset_type::Q35 ? OVMF_VARS_SECBOOT : OVMF_VARS_4M;
}

//return firmware for vz8 and vz7
//SMM-enabled for vz8 and SMM-off for vz7, respectively
QString OVMF::getFirmware(Chipset_type machine_type)
{
	return machine_type == Chipset_type::Q35 ? OVMF_CODE_SECBOOT : OVMF_CODE_4M;
}


bool NvramUpdater::upgrade(CVmConfiguration &config_)
{
	//upgrade old NVRAM to new 4M only for non Chipset_type::Q35.
	//Chipset_type::Q35 uses OVMF_CODE_SECBOOT. It is not need update
	//do not upgrade running VM, because their NVRAM is locked
	CVmStartupBios* pBios = config_.getVmSettings()->getVmStartupOptions()->getBios();
	if (pBios && pBios->isEfiEnabled() && !config_.getVmSettings()->getClusterOptions()->isRunning())
	{
		NvramUpdater n(config_);
		if (n.isOldVerison() && n.updateNVRAM())
		{
			WRITE_TRACE(DBG_INFO, "NVRAM Updated successfully for VM '%s'", QSTR2UTF8(config_.getVmIdentification()->getVmName()));
			pBios->setNVRAM(n.getNewNvramPath());
			return true;
		}
	}
	return false;
}

NvramUpdater::NvramUpdater(const CVmConfiguration &config_) :
		m_input(config_),
		m_oldNvram(config_.getVmSettings()->getVmStartupOptions()->getBios()->getNVRAM())
{
	m_tmpNvram = m_oldNvram.absolutePath() + "/NVRAM_tmp_update.dat";
	m_newNvram = m_oldNvram.absolutePath() + "/" + VZ_VM_NVRAM_FILE_NAME;
	m_storage = findBootHDD();
}

NvramUpdater::~NvramUpdater()
{
	QFile::remove(m_tmpNvram.absoluteFilePath());
	unlock();
}

bool NvramUpdater::lock()
{
	QMutexLocker lock(&s_mutexNvramList);
	if (s_NvramList.contains(m_newNvram))
		return false;

	s_NvramList.append(m_newNvram);
	return true;
}

void NvramUpdater::unlock()
{
	QMutexLocker lock(&s_mutexNvramList);
	s_NvramList.removeAll(m_newNvram);
}

const CVmHardDisk *NvramUpdater::findDiskByIndex(const QList<CVmHardDisk* >& list, unsigned int index) const
{
	for(const CVmHardDisk *hdd : list)
		if (hdd->getIndex() == index)
			return hdd;
	return NULL;
}

const QString NvramUpdater::findBootHDD() const
{
	QString disk;
	for(const CVmStartupOptions::CVmBootDevice *dev :
			m_input.getVmSettings()->getVmStartupOptions()->getBootDeviceList())
	{
		if (dev->deviceType != PDE_HARD_DISK || !dev->inUseStatus)
			continue;
		const CVmHardDisk *hdd = findDiskByIndex(m_input.getVmHardwareList()->m_lstHardDisks, dev->deviceIndex);
		if (!hdd)
			continue;
		disk = hdd->getSystemName();
		break;
	}
	return disk;
}

QStringList NvramUpdater::generateQemuArgs(const QString &ovmfCode, const QString &ovmfVars, const QString &disk)
{
	QStringList result;
	result <<
			"-machine" << "pc-i440fx-vz7.12.0,smm=on,accel=tcg" <<
			"-display" << "none" << "-no-user-config" << "-nodefaults" <<
			"-m" << "256" << "-smp" << "2,sockets=2,cores=1,threads=1" <<
			"-chardev" << "pty,id=charserial1" <<
			"-device" << "isa-serial,chardev=charserial1,id=serial1" <<
			"-drive" << QString("file=%1,if=pflash,format=raw,unit=0,readonly=on").arg(ovmfCode) <<
			"-drive" << QString("file=%1,if=pflash,format=qcow2,unit=1,readonly=off").arg(ovmfVars) <<
			"-drive" << QString("file=%1,format=raw,if=none,media=cdrom,id=drive-cd1,readonly=on").arg(OVMF_UEFI_SHELL_ISO_PATH) <<
			"-device" << "ide-cd,drive=drive-cd1,id=cd1,bootindex=1" <<
			"-drive" << QString("file=%1,format=qcow2,readonly=off,if=none,media=disk,id=drive-hd2").arg(disk) <<
			"-device" << "ide-hd,drive=drive-hd2,id=hd2,bootindex=2" <<
			"-global" << "driver=cfi.pflash01,property=secure,value=on" <<
			"-serial" << "stdio";

	return result;
}

bool NvramUpdater::runCmd(const QStringList &cmd)
{
	QString output;
	QProcess proc;
	if (!HostUtils::RunCmdLineUtility(cmd, output, DEFAULT_WAIT_TIMER, &proc))
	{
		WRITE_TRACE(DBG_FATAL, "NVRAM Updater: Cmd '%s' failed with error '%s':%d",
					QSTR2UTF8(cmd.join(" ")),
					QSTR2UTF8(proc.errorString()),
					proc.exitCode());
		return false;
	}
	return true;
}

static void cleanup_output(QByteArray &out)
{
	for (int i = 31; i < 41; i++)
		out.replace(QString("\e[%1m").arg(i), "");

	for (int i = 11; i < 14; i++)
	{
		for (int j = 1; j < 10; j++)
			out.replace(QString("\e[%1;0%2H").arg(i).arg(j), "");
		for (int j = 10; j < 40; j++)
			out.replace(QString("\e[%1;%2H").arg(i).arg(j), "");
	}

	//other corner cases of ansi escape sequences
	out.replace("\e[0m", "");
	out.replace("\e[1m", "");
	out.replace("\e[01;01H", "");
	out.replace("\e[2J", "");
	out.replace("\e[=3h", "");
	out.replace("\e", "");
	out.replace("\r", "");
}

bool NvramUpdater::sendUefiEscape(QProcess &p)
{
	QByteArray out;
	int i = 0;
	do {
		p.waitForReadyRead(DEFAULT_OUTPUT_WAIT_TIMER);
		out.append(p.readAllStandardOutput());
	} while ( i++ < UEFISHELL_WELCOME_MESSAGE_SIZE && out.size() < UEFISHELL_WELCOME_MESSAGE_SIZE &&
			  !out.contains("in 5 seconds to skip") && p.state() == QProcess::Running);
	cleanup_output(out);
	if (!out.contains("in 5 seconds to skip"))
	{
		WRITE_TRACE(DBG_FATAL, "UEFI shell utility. Waiting UefiShell load Error. Output return %d bytes after %d iteration. Output:\n%s\n", out.size(), i, out.data());
		return false;
	}
	WRITE_TRACE(DBG_DEBUG, "UEFI shell utility: Output return %d bytes after %d iteration. Output:\n%s\n", out.size(), i, out.data());
	p.write("\e");
	return true;
}

bool NvramUpdater::runUefiShell(const UEFI_TASKS task)
{
	QProcess uefiShell;
	QStringList lArgs;
	if (task == UEFI_TASKS::UEFI_DUMP_VARS)
	{
		lArgs = generateQemuArgs(OVMF_CODE_OLD, m_tmpNvram.absoluteFilePath(), m_storage);
	}
	else if (task == UEFI_TASKS::UEFI_RESTORE_VARS)
	{
		lArgs = generateQemuArgs(OVMF_CODE_4M, m_newNvram, m_storage);
	}
	else
	{
		WRITE_TRACE(DBG_FATAL, "UEFI shell utility ERROR: Unknown task: %d", task);
		return false;
	}

	uefiShell.start("/usr/libexec/qemu-kvm", lArgs);

	if (!uefiShell.waitForStarted(DEFAULT_WAIT_TIMER))
	{
		WRITE_TRACE(DBG_FATAL, "UEFI shell utility [%d] does not start: '%s':%d", task,
					QSTR2UTF8(uefiShell.errorString()), uefiShell.error());
		return false;
	}

	//Send the escape char to enter the UEFI shell early
	if (!sendUefiEscape(uefiShell))
		return false;

	//send all rest commands
	QString shell("fs1:\r\n");//'Q35' chipset UEFI shell uses 'fs0', for 'pc-i440f' chipset UEFI shell uses 'fs1'
	shell.append(task == UEFI_TASKS::UEFI_DUMP_VARS ? "dmpstore -all -s allvars.dat \r\n" : "dmpstore -l allvars.dat \r\n");
	shell.append("reset -s\r\n");
	qint64 ret = uefiShell.write(shell.toUtf8());
	WRITE_TRACE(DBG_DEBUG, "UEFI shell utility write %lld [%d] bytes", ret, shell.toUtf8().size());

	if (!uefiShell.waitForFinished(DEFAULT_WAIT_TIMER))
	{
		WRITE_TRACE(DBG_FATAL, "UEFI shell utility [%d] does not respond. Terminate it now.", task);
		uefiShell.terminate();
		if (!uefiShell.waitForFinished(DEFAULT_WAIT_TIMER))
		{
			uefiShell.kill();
			uefiShell.waitForFinished(DEFAULT_WAIT_TIMER);
		}
	}

	QByteArray out;
	out.append(uefiShell.readAllStandardOutput());
	cleanup_output(out);
	WRITE_TRACE(DBG_DEBUG, "UEFI shell utility finished. Output contains %d bytes. Output:\n%s\n", out.size(), out.data());

	if (0 != uefiShell.exitCode())
	{
		WRITE_TRACE(DBG_FATAL, "UEFI shell utility [%d] failed with error: '%s':%d", task,
				QSTR2UTF8(uefiShell.errorString()), uefiShell.error());
		return false;
	}

	return true;
}

bool NvramUpdater::updateNVRAM()
{
	WRITE_TRACE(DBG_DEBUG, "Trying to update NVRAM: %s", QSTR2UTF8(m_oldNvram.absoluteFilePath()));

	//remove artifacts if present
	QFile::remove(m_newNvram);

	if (!QFile::exists(m_storage))
	{
		WRITE_TRACE(DBG_FATAL, "NVRAM Updater: Storage file '%s' is absent",
				QSTR2UTF8(m_storage));
		return false;
	}

	//create var from template
	QStringList cmdQemuConvert = QStringList() << QEMU_IMG_BIN << "convert" << "-f" << "raw" << "-O" << "qcow2" << OVMF_VARS_4M << m_newNvram;
	if (!runCmd(cmdQemuConvert))
		return false;

	if (!runUefiShell(UEFI_TASKS::UEFI_DUMP_VARS))
		return false;

	if (!runUefiShell(UEFI_TASKS::UEFI_RESTORE_VARS))
		return false;

	WRITE_TRACE(DBG_DEBUG, "NVRAM Updated successfully to %s", QSTR2UTF8(m_newNvram));
	return true;
}

bool NvramUpdater::isOldVerison()
{
	if (m_oldNvram.filePath() == m_newNvram)
		return false;

	if (!lock())
	{
		WRITE_TRACE(DBG_WARNING, "NVRAM Updater: trying to update NVRAM simultaneously '%s'",
					QSTR2UTF8(m_oldNvram.filePath()));
		return false;
	}

	if (!m_oldNvram.exists())
	{
		WRITE_TRACE(DBG_FATAL, "NVRAM Updater: File '%s' is absent",
					QSTR2UTF8(m_oldNvram.filePath()));
		return false;
	}

	QFile::remove(m_tmpNvram.absoluteFilePath());
	QFile::copy(m_oldNvram.absoluteFilePath(), m_tmpNvram.absoluteFilePath());
	if (!m_tmpNvram.exists())
	{
		WRITE_TRACE(DBG_FATAL, "NVRAM Updater: Cannot create temporary file to update NVRAM '%s'",
					QSTR2UTF8(m_oldNvram.filePath()));
		return false;
	}

	if (static_cast<Chipset_type>(m_input.getVmHardwareList()->getChipset()->getType()) == Chipset_type::Q35)
	{
		WRITE_TRACE(DBG_FATAL, "NVRAM Updater: VM with Q35 chipset does not need update NVRAM file '%s'",
					QSTR2UTF8(m_oldNvram.filePath()));
		return false;
	}

	QStringList cmd = QStringList() << QEMU_IMG_BIN << "info" << m_tmpNvram.absoluteFilePath();
	QString output;
	QProcess proc;
	if (!HostUtils::RunCmdLineUtility(cmd, output, DEFAULT_WAIT_TIMER, &proc))
	{
		WRITE_TRACE(DBG_FATAL, "NVRAM Updater: Cmd '%s' failed: '%s'; exit code: %d",
					QSTR2UTF8(cmd.join(" ")),
					QSTR2UTF8(proc.errorString()),
					proc.exitCode());
		return false;
	}

	if (output.isEmpty())
	{
		WRITE_TRACE(DBG_WARNING, "NVRAM Updater: Cmd '%s' return empty string",
					QSTR2UTF8(cmd.join(" ")));
		return false;
	}

	//old NVRAM format contains 'virtual size: 128K (131072 bytes)' field
	return output.split('\n').filter("virtual size").join(" ").contains("128K");
}
