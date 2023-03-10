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
constexpr const int DEFAULT_OUTPUT_WAIT_TIMER			= 1 * 1000;
constexpr const int DEFAULT_STORAGE_SIZE				= 256 * 1024 * 1024;
constexpr const int UEFISHELL_WELCOME_MESSAGE_SIZE		= 1000;

// return NVRAM template for vz8 and vz7
QString OVMF::getTemplate(Chipset_type machine_type)
{
	return machine_type == Chipset_type::CHIP_Q35 ? OVMF_VARS_SECBOOT : OVMF_VARS_4M;
}

//return firmware for vz8 and vz7
//SMM-enabled for vz8 and SMM-off for vz7, respectively
QString OVMF::getFirmware(Chipset_type machine_type)
{
	return machine_type == Chipset_type::CHIP_Q35 ? OVMF_CODE_SECBOOT : OVMF_CODE_4M;
}

NvramUpdater::NvramUpdater(const QString &path_, const Chipset_type chip) :
		m_path(path_) , m_chip(chip)
{
	m_newNvram = m_path.absolutePath() + "/" + VZ_VM_NVRAM_FILE_NAME;
	m_storage = m_path.absolutePath() + "/store.img";
}

NvramUpdater::~NvramUpdater()
{
	QFile::remove(m_storage);
}

QStringList NvramUpdater::generateQemuArgs(const QString &ovmfCode, const QString &ovmfVars, const QString &disk)
{
	QStringList result;
	result <<
			"-machine" << "q35,smm=on,accel=tcg" <<
			"-display" << "none" << "-no-user-config" << "-nodefaults" <<
			"-m" << "256" << "-smp" << "2,sockets=2,cores=1,threads=1" <<
			"-chardev" << "pty,id=charserial1" <<
			"-device" << "isa-serial,chardev=charserial1,id=serial1" <<
			"-drive" << QString("file=%1,if=pflash,format=raw,unit=0,readonly=on").arg(ovmfCode) <<
			"-drive" << QString("file=%1,if=pflash,format=qcow2,unit=1,readonly=off").arg(ovmfVars) <<
			"-drive" << QString("file=%1,format=raw,if=none,media=cdrom,id=drive-cd1,readonly=on").arg(OVMF_UEFI_SHELL_ISO_PATH) <<
			"-device" << "ide-cd,drive=drive-cd1,id=cd1,bus=ide.2,bootindex=1" <<
			"-drive" << QString("file=%1,format=raw,if=none,media=disk,id=drive-hd1,readonly=off").arg(disk) <<
			"-device" << "ide-hd,drive=drive-hd1,id=hd1,bus=ide.1,bootindex=2" <<
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
		lArgs = generateQemuArgs(m_chip == Chipset_type::CHIP_Q35 ? OVMF_CODE_SECBOOT : OVMF_CODE_OLD, m_path.absoluteFilePath(), m_storage);
	}
	else if (task == UEFI_TASKS::UEFI_RESTORE_VARS)
	{
		lArgs = generateQemuArgs(OVMF_CODE_4M, m_newNvram, m_storage);
	}
	else
	{
		WRITE_TRACE(DBG_FATAL, "UEFI shell utility ERROR: Unknown task: %d", static_cast<int>(task));
		return false;
	}

	uefiShell.start("/usr/libexec/qemu-kvm", lArgs);

	if (!uefiShell.waitForStarted(DEFAULT_WAIT_TIMER))
	{
		WRITE_TRACE(DBG_FATAL, "UEFI shell utility [%d] does not start: '%s':%d", static_cast<int>(task),
					QSTR2UTF8(uefiShell.errorString()), uefiShell.error());
		return false;
	}

	//Send the escape char to enter the UEFI shell early
	if (!sendUefiEscape(uefiShell))
		return false;

	//send all rest commands
	QString shell("fs0:\r\n");
	shell.append(task == UEFI_TASKS::UEFI_DUMP_VARS ? "dmpstore -all -s allvars.dat \r\n" : "dmpstore -l allvars.dat \r\n");
	shell.append("reset -s\r\n");
	qint64 ret = uefiShell.write(shell.toUtf8());
	WRITE_TRACE(DBG_DEBUG, "UEFI shell utility write %lld [%d] bytes", ret, shell.toUtf8().size());

	if (!uefiShell.waitForFinished(DEFAULT_WAIT_TIMER))
	{
		WRITE_TRACE(DBG_FATAL, "UEFI shell utility [%d] does not respond. Terminate it now.", static_cast<int>(task));
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
		WRITE_TRACE(DBG_FATAL, "UEFI shell utility [%d] failed with error: '%s':%d", static_cast<int>(task),
				QSTR2UTF8(uefiShell.errorString()), uefiShell.error());
		return false;
	}

	return true;
}

bool NvramUpdater::updateNVRAM()
{
	WRITE_TRACE(DBG_DEBUG, "Trying to update NVRAM: %s", QSTR2UTF8(m_path.absoluteFilePath()));

	//remove artifacts if present
	QFile::remove(m_newNvram);

	QFile storage(m_storage);
	if (!storage.open(QIODevice::WriteOnly | QIODevice::Truncate))
	{
		WRITE_TRACE(DBG_FATAL, "NVRAM Updater: can't create storage file '%s': %s",
				QSTR2UTF8(m_storage), QSTR2UTF8(storage.errorString()));
		return false;
	}
	if (!storage.resize(DEFAULT_STORAGE_SIZE))
	{
		WRITE_TRACE(DBG_FATAL, "NVRAM Updater: can't truncate storage file '%s'",
			QSTR2UTF8(m_storage));
		return false;
	}
	storage.close();

	QStringList cmdMkfs = QStringList() << "mkfs.fat" << "-n" << "EFIVARST" << m_storage;
	if (!runCmd(cmdMkfs))
		return false;

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
	QStringList cmd = QStringList() << QEMU_IMG_BIN << "info" << m_path.absoluteFilePath();
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
