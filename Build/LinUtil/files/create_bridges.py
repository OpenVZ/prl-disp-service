#!/usr/bin/python

from ConfigParser import SafeConfigParser
import glob
import os
from StringIO import StringIO
from subprocess import Popen, PIPE, call
import re


CONFIG_PATH = "/etc/sysconfig/network-scripts"
SECTION_NAME = "section"
BACKUP_PREFIX = "vz_preserved-"


class FakeSection(object):
    """Wrapper for input stream inserting sections."""
    def __init__(self, f):
        self.f = f
        self.section_read = False

    def readline(self):
        if self.section_read:
            return self.f.readline()
        self.section_read = True
        return "[%s]\n" % SECTION_NAME


class NoSectionConfigParser(SafeConfigParser):
    """Config parser for files without section headers.

    Has dict-like interface, but supports ConfigParser-like as well.
    """
    def __init__(self):
        SafeConfigParser.__init__(self)
        self.optionxform = str
        self.add_section(SECTION_NAME)

    def _read(self, fp, fpname):
        SafeConfigParser._read(self, FakeSection(fp), fpname)

    def get(self, option, default):
        try:
            return SafeConfigParser.get(self, SECTION_NAME, option)
        except:
            return default

    def set(self, option, value):
        SafeConfigParser.set(self, SECTION_NAME, option, value)

    def __setitem__(self, option, value):
        self.set(option, value)

    def __getitem__(self, option):
        return SafeConfigParser.get(self, SECTION_NAME, option)

    def delete(self, option):
        if not self.remove_option(SECTION_NAME, option):
            raise ValueError("No such option")

    def items(self):
        return SafeConfigParser.items(self, SECTION_NAME)

    def write(self, fp):
        for (key, value) in self._sections[SECTION_NAME].items():
            if key == "__name__":
                continue
            if value or (self._optcre == self.OPTCRE):
                key = "=".join((key, str(value)))
            fp.write("%s\n" % (key))

    def __iter__(self):
        return (item[0] for item in self.items())


class NetworkServiceManager(object):
    """Controls network services.

    Remembers status that was on creation.
    Does NOT update status according to commands executed.
    Works with a single 'active' network service.
    """

    # Known network services in decreasing priority.
    known_services = ["NetworkManager", "network"]

    def __init__(self):
        self.services = {}
        self.active_service = None
        for service in self.known_services:
            status = self.get_network_service_status(service)
            self.services[service] = status
            if status["running"]:
                self.active_service = service

    @staticmethod
    def get_network_service_status(netservice):
        p = Popen(["systemctl", "show", netservice,
                   "--property=LoadState,ActiveState,SubState"], stdout=PIPE, stderr=PIPE)
        stdout, stderr = p.communicate()
        if p.returncode != 0:
            print "Systemctl exited with code %d" % p.returncode
            return {"loaded": False, "running": False}
        cp = NoSectionConfigParser()
        cp.readfp(StringIO(stdout))
        try:
            if cp["LoadState"] != "loaded":
                return {"loaded": False, "running": False}
            if cp["ActiveState"] == "active" and cp["SubState"] == "running":
                return {"loaded": True, "running": True}
            return {"loaded": True, "running": False}
        except:
            return {"loaded": False, "running": False}

    @staticmethod
    def call_network_service(netservice, command):
        """Execute command via systemctl."""
        if netservice not in NetworkServiceManager.known_services:
            raise ValueError("Network service unknown")
        return call(["systemctl", command, netservice])

    def stop_networking(self):
        if not self.active_service:
            # Silently ignore
            return 0
        return self.call_network_service(self.active_service, "stop")

    def start_networking(self):
        if not self.active_service:
            # Silently ignore
            return 0
        return self.call_network_service(self.active_service, "start")


def dequote(s):
    """Remove outer quotes."""
    s = s.strip()
    if len(s) >= 2 and s[0] == s[-1] and s[0] in ["\'", "\""]:
        s = s[1:-1]
    return s

def read_configs():
    interfaces = {}
    for ifcfg in glob.glob("ifcfg-*"):
        cp = NoSectionConfigParser()
        cp.read(ifcfg)
        interfaces[ifcfg.replace("ifcfg-", "")] = cp
    return interfaces

def get_current_bridge_id(interfaces):
    bridge_re = re.compile(r"^br(\d+)$")
    current_bridge_id = 0
    for iface in interfaces:
        # Get bridge id.
        match = bridge_re.match(iface)
        if match and int(match.group(1)) >= current_bridge_id:
            current_bridge_id = int(match.group(1)) + 1
    return current_bridge_id


def need_device(cp):
    return dequote(cp.get("DEVICE", "")) == ""

def add_device(iface, cp):
    cp["DEVICE"] = iface
    filename = "ifcfg-%s" % iface
    return (filename, BACKUP_PREFIX + filename)


def need_bridge(iface, cp):
    if dequote(cp.get("BRIDGE", "")) != "":
        return False
    elif (iface == "lo" or
            dequote(cp.get("NAME", "").lower()) == "loopback"):
        return False
    elif dequote(cp.get("TYPE", "").lower()) in ["venet", "bridge"]:
        # TODO: Or only for existing bridges?
        return False
    elif dequote(cp.get("ONBOOT", "").lower()) != "yes":
        return False
    elif dequote(cp.get("MASTER", "").lower()) != "":
        return False
    elif dequote(cp.get("TEAM_MASTER", "").lower()) != "":
        return False
    return True

def create_bridge(iface, cp, bridge_id, to_write):
    global current_bridge_id

    print "Creating bridge config for %s" % iface
    br_name = "br%s" % bridge_id
    bridge = NoSectionConfigParser()
    bridge["DEVICE"] = "\"%s\"" % br_name
    cp["BRIDGE"] = "\"%s\"" % br_name
    # Move attributes.
    for attr in cp:
        if attr in ["DEVICE", "HWADDR", "UUID", "BRIDGE", "TYPE", "DEVICETYPE", "NAME"]:
            continue
        if attr.startswith("BOND"):
            continue
        bridge[attr] = cp[attr]
        if attr != "ONBOOT":
            cp.delete(attr)
    bridge["TYPE"] = "\"Bridge\""
    bridge["DELAY"] = "\"2\""
    bridge["STP"] = "\"off\""
    bridge["UUID"] = "\"%s\"" % open(
            "/proc/sys/kernel/random/uuid").read().strip()
    # Save to config files. (ConfigParser, filename, backup).
    to_write.append((bridge, "ifcfg-%s" % br_name, None))

    filename = "ifcfg-%s" % iface
    return (filename, BACKUP_PREFIX + filename)


def write_configs(to_write):
    written = []
    error = False
    for cp, filename, backup in to_write:
        try:
            if backup:
                os.rename(filename, backup)
                written.append((filename, backup))
                cp.write(open(filename, "w"))
            else:
                cp.write(open(filename, "w"))
                written.append((filename, backup))
        except:
            error = True
            break

    if error:
        # Fallback.Restore original configs.
        for filename, backup in written:
            # Best effort.
            try:
                if backup:
                    os.rename(backup, filename)
                else:
                    os.remove(filename)
            except:
                pass

def rename_device(iface, cp):
    need_restart = False
    niface = dequote(cp.get("DEVICE", ""))
    f_iface = "ifcfg-%s" % iface
    if not niface or niface == iface:
        return f_iface, iface, need_restart
    f_niface = "ifcfg-%s" % niface
    try:
        if os.path.exists(f_niface):
            os.rename(f_niface, BACKUP_PREFIX + f_niface)
            need_restart = True
        os.rename(f_iface, f_niface)
    except:
        return f_iface, iface, need_restart
    return f_niface, niface, need_restart

def proceed_devices():
    """Create bridge configs for interfaces.

    Exclude:
        bridge
        loopback
        venet
        slave eths
        already having an attached bridge
    """
    # Read interface configs.
    os.chdir(CONFIG_PATH)
    interfaces = read_configs()
    # We enumerate new bridges starting from max existing brN.
    current_bridge_id = get_current_bridge_id(interfaces)

    to_write = []
    for iface, cp in sorted(interfaces.items()):
        filename, backup = None, None
        if dequote(cp.get("ONBOOT", "").lower()) != "yes":
            continue
        if need_device(cp):
            filename, backup = add_device(iface, cp)
        filename, iface, need_restart = rename_device(iface, cp)
        if need_restart:
            return proceed_devices()
        if need_bridge(iface, cp):
            filename, backup = create_bridge(iface, cp, current_bridge_id, to_write)
            current_bridge_id += 1
        if filename:
            to_write.append((cp, filename, backup))

    return to_write

def create_bridges():
    # If nothing changed, there is no need to restart networking.
    to_write = proceed_devices()

    if not to_write:
        return

    netmanager = NetworkServiceManager()
    # If networking is running, stop it.
    netmanager.stop_networking()

    write_configs(to_write)

    # If networking was running, restart it.
    netmanager.start_networking()


if __name__ == "__main__":
    create_bridges()

