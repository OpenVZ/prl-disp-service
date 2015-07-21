#!/usr/bin/python

from ConfigParser import SafeConfigParser
import glob
import os
from StringIO import StringIO
from subprocess import Popen, PIPE, call
import re


CONFIG_PATH = "/etc/sysconfig/network-scripts"
SECTION_NAME = "section"
BACKUP_EXT = ".old"


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


def create_bridges():
    """Create bridge configs for interfaces.

    Exclude:
        bridge
        loopback
        venet
        already having an attached bridge
    """
    # Read interface configs.
    os.chdir(CONFIG_PATH)
    interfaces = {}
    # We enumerate new bridges starting from max existing brN.
    bridge_re = re.compile(r"^br(\d+)$")
    current_bridge_id = 0
    for ifcfg in glob.glob("ifcfg-*"):
        iface = ifcfg.replace("ifcfg-", "")
        # Ignore backups.
        if iface.endswith(BACKUP_EXT):
            continue
        cp = NoSectionConfigParser()
        cp.read(ifcfg)
        interfaces[iface] = cp
        # Get bridge id.
        match = bridge_re.match(iface)
        if match and int(match.group(1)) >= current_bridge_id:
            current_bridge_id = int(match.group(1)) + 1

    # Get list of interfaces for which to create bridges.
    to_bridges = []
    for iface, cp in interfaces.items():
        if dequote(cp.get("BRIDGE", "")) != "":
            continue
        elif (iface == "lo" or
              dequote(cp.get("NAME", "").lower()) == "loopback"):
            continue
        elif dequote(cp.get("TYPE", "").lower()) in ["venet", "bridge"]:
            # TODO: Or only for existing bridges?
            continue
        to_bridges.append(iface)

    to_write = []
    # Create bridges.
    for iface in to_bridges:
        print "Creating bridge config for %s" % iface
        br_name = "br%s" % current_bridge_id
        bridge = NoSectionConfigParser()
        bridge["DEVICE"] = "\"%s\"" % br_name
        interfaces[iface]["BRIDGE"] = "\"%s\"" % br_name
        # Move attributes.
        for attr in interfaces[iface]:
            if attr in ["DEVICE", "HWADDR", "UUID", "BRIDGE", "TYPE"]:
                continue
            bridge[attr] = interfaces[iface][attr]
            if attr != "ONBOOT":
                interfaces[iface].delete(attr)
        bridge["TYPE"] = "\"Bridge\""
        bridge["DELAY"] = "\"2\""
        bridge["UUID"] = "\"%s\"" % open(
                "/proc/sys/kernel/random/uuid").read().strip()
        # Save to config files. (ConfigParser, filename, backup).
        to_write.append((bridge, "ifcfg-%s" % br_name, False))
        to_write.append((interfaces[iface], "ifcfg-%s" % iface, True))
        current_bridge_id += 1

    # If nothing changed, there is no need to restart networking.
    if len(to_write) == 0:
        return

    netmanager = NetworkServiceManager()
    # If networking is running, stop it.
    netmanager.stop_networking()

    # Write configs.
    written = []
    error = False
    for cp, filename, backup in to_write:
        try:
            if backup:
                os.rename(filename, filename + BACKUP_EXT)
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
                    os.rename(filename + BACKUP_EXT, filename)
                else:
                    os.remove(filename)
            except:
                pass

    # If networking was running, restart it.
    netmanager.start_networking()


if __name__ == "__main__":
    create_bridges()

