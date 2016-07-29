#!/usr/bin/env python

#
# This program converts virtuozzo virtual machines
# personal data into cloud-init format file
#
# Copyright (c) 2015-2016 Parallels IP Holdings GmbH
#
# This file is part of Virtuozzo Core. Virtuozzo Core is free
# software; you can redistribute it and/or modify it under the terms
# of the GNU General Public License as published by the Free Software
# Foundation; either version 2 of the License, or (at your option) any
# later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
# 02110-1301, USA.
#
# Our contact details: Parallels IP Holdings GmbH, Vordergasse 59, 8200
# Schaffhausen, Switzerland.
#

import io
import os
import sys
import json
import yaml
import uuid
import base64
import shutil
import tempfile
import argparse

# dirty workaround for proper UTF-8 handling
reload(sys)
sys.setdefaultencoding('utf-8')

def prepare_dir(path):
    d = os.path.dirname(path)
    if d and not os.path.exists(d):
        os.makedirs(d)

#
# Creates ISO image in Openstack datasource format
# Requirements:
#  * /openstack/latest/meta_data.json file with instance id
#  * /openstack/latest/user_data file with YAML-formatted settings
#  * LABEL=config-2
#
class Iso:
    def __init__(self, output, entry, label):
        self.output = output
        self.entry = entry
        self.label = label

    def generate(self):
        if os.system("/usr/bin/genisoimage -output " + self.output +
                     " -volid " + self.label + " -joliet -rock " + self.entry) != 0:
            sys.stderr.write("Error: " + message + "\n")
            sys.exit(1)

class OpenStackConfigDrive:
    def __init__(self, iso):
        d = "openstack/latest"
        self.user_data_path = os.path.join(d, "user_data")
        self.meta_data_path = os.path.join(d, "meta_data.json")
        self.cd_label = "config-2"
        t = "/vz/tmp"
        if os.path.exists(t) and not os.path.isdir(t):
            raise Exception(t + " is not a directory!")
        if not os.path.exists(t):
            os.makedirs(t)
        self.tmp_dir = tempfile.mkdtemp(dir=t)
        self.iso = iso

    def __del__(self):
        if os.path.exists(self.tmp_dir):
            shutil.rmtree(self.tmp_dir)

    def prepare_iso(self, storage):
        self.write_meta_data()
        self.write_user_data(storage.get_data())
        Iso(self.iso, self.tmp_dir, self.cd_label).generate()

class CloudBaseInit(OpenStackConfigDrive):
    def __init__(self, iso):
        OpenStackConfigDrive.__init__(self, iso)

    def __del__(self):
        OpenStackConfigDrive.__del__(self)

    def prepare_script(self, commands, data):
        s = ("#!/usr/bin/env python\r\n"
             "\r\n"
             "import os, struct, win32api, win32net\r\n"
             "import win32file, win32netcon, win32con, wmi\r\n"
             "\r\n"
             "def eject(drive):\r\n"
             "    FSCTL_LOCK_VOLUME = 0x0090018\r\n"
             "    FSCTL_DISMOUNT_VOLUME = 0x00090020\r\n"
             "    IOCTL_STORAGE_MEDIA_REMOVAL = 0x002D4804\r\n"
             "    IOCTL_STORAGE_EJECT_MEDIA = 0x002D4808\r\n"
             "\r\n"
             "    volume = r\"\\\\.\\{}\".format(drive)\r\n"
             "    h = win32file.CreateFile(volume, win32con.GENERIC_READ | win32con.GENERIC_WRITE,\r\n"
             "                             win32con.FILE_SHARE_READ | win32con.FILE_SHARE_WRITE, None,\r\n"
             "                             win32con.OPEN_EXISTING, 0, None)\r\n"
             "    win32file.DeviceIoControl(h, FSCTL_LOCK_VOLUME, bytes(\"\", \"UTF-8\"), 0, None)\r\n"
             "    win32file.DeviceIoControl(h, FSCTL_DISMOUNT_VOLUME, bytes(\"\", \"UTF-8\"), 0, None)\r\n"
             "\r\n"
             "    try:\r\n"
             "        win32file.DeviceIoControl(h, IOCTL_STORAGE_MEDIA_REMOVAL, struct.pack(\"B\", 0), 0, None)\r\n"
             "        win32file.DeviceIoControl(h, IOCTL_STORAGE_EJECT_MEDIA, bytes(\"\", \"UTF-8\"), 0, None)\r\n"
             "    finally:\r\n"
             "        win32file.CloseHandle(h)\r\n"
             "\r\n"
             "def run_setup(drive):\r\n"
             "    os.system(\"{}\\setup.exe\".format(drive))\r\n"
             "\r\n"
             "def run_on_cdrom(label, action):\r\n"
             "    w = wmi.WMI()\r\n"
             "    for d in w.Win32_CDROMDrive():\r\n"
             "        if d.VolumeName != label:\r\n"
             "            continue\r\n"
             "        action(d.Drive)\r\n"
             "\r\n"
             "def eject_cloudconfig_cd():\r\n"
             "    run_on_cdrom(\"config-2\", eject)\r\n"
             "\r\n"
             "def install_qga():\r\n"
             "    run_on_cdrom(\"vz-tools-win\", run_setup)\r\n"
             "\r\n"
             "def set_password(username, password):\r\n"
			 # This error codes should be in some python module, but they are absent, so I took them from
			 # https://msdn.microsoft.com/ru-ru/library/windows/desktop/aa370674(v=vs.85).aspx
             "    USER_ALREADY_EXISTS = 2224\r\n"
             "    PASSWORD_INSECURE = 2245\r\n"
             "    try:\r\n"
             "        try:\r\n"
             "            win32net.NetUserAdd(None, 1,\r\n"
             "                        {\"name\": username,\r\n"
             "                         \"password\":password,\r\n"
             "                         \"password_age\":0,\r\n"
             "                         \"priv\":win32netcon.USER_PRIV_USER,\r\n"
             "                         \"home_dir\":\"\",\r\n"
             "                         \"flags\":win32netcon.UF_SCRIPT | win32netcon.UF_DONT_EXPIRE_PASSWD})\r\n"
             "        except win32net.error as e:\r\n"
             "            if e.winerror == USER_ALREADY_EXISTS:\r\n"
             "                win32net.NetUserSetInfo(None, username, 1003, {\"password\": password})\r\n"
             "            else:\r\n"
             "                raise\r\n"
             "    except win32net.error as e:\r\n"
             "        if e.winerror == PASSWORD_INSECURE:\r\n"
             "            print(\"Password is insecure\")\r\n"
             "        else:\r\n"
             "            raise\r\n"
             "\r\n"
             "def main():\r\n"
             "    install_qga()\r\n")
        if "users" in data:
            for name, credentials in data["users"].iteritems():
                s += "    set_password(\"{}\",\"{}\")\r\n".format(name, credentials["password"].replace('"', '\\"'))

        for command in commands:
            if type(command) is unicode:
                s += "    os.system(\"{}\")\r\n".format(command)
            else:
                # windows cmd supports only double quotes
                s += "    os.system(\"{}\")\r\n".format(" ".join(command).replace("'", "\""))
        s += ("    eject_cloudconfig_cd()\r\n"
              "\r\n"
              "if __name__ == \"__main__\":\r\n"
              "    main()\r\n")
        return s

    def write_meta_data(self):
        meta_data = os.path.join(self.tmp_dir, self.meta_data_path);
        prepare_dir(meta_data)
        with io.open(meta_data, "w", newline="\r\n") as f:
            d = {"instance-id": str(uuid.uuid1())}
            f.write(unicode(json.dumps(d, meta_data, ensure_ascii=False, sort_keys=True,
                    indent=2, separators=(',', ': '))))
            f.write(u"\r\n")

    def write_user_data(self, data):
        s = []
        s.extend(data.get("runcmd", []))
        s.extend(data.get("bootcmd", []))
        user_data = os.path.join(self.tmp_dir, self.user_data_path);
        prepare_dir(user_data)

        with open(user_data, 'w') as f:
            f.write(self.prepare_script(s, data))

class CloudInit(OpenStackConfigDrive):
    def __init__(self, iso):
        OpenStackConfigDrive.__init__(self, iso)

    def __del__(self):
        OpenStackConfigDrive.__del__(self)

    def write_meta_data(self):
        meta_data = os.path.join(self.tmp_dir, self.meta_data_path);
        prepare_dir(meta_data)
        with open(meta_data, "w") as f:
            d = {"instance-id": str(uuid.uuid1()),
                    "uuid": str(uuid.uuid1())}
            json.dump(d, f, sort_keys=True, ensure_ascii=False,
                    indent=2, separators=(",", ": "))
            f.write("\n")

    def prepare_password_command(self, user, credentials):
        s = " -e" if credentials.get("is_encrypted", True) else ""
        b = base64.b64encode(user + ":" + credentials["password"])
        return "CRED=$(echo '{0}' | base64 -w 0 --decode); echo $CRED | chpasswd{1}".format(b, s)

    def useradd_command(self, user):
        return "/usr/sbin/useradd {0} 1>/dev/null 2>&1 || :".format(user)

    def run_in_bash(self, cmd):
        return ["/bin/bash", "-c", cmd]

    def write_user_data(self, data):
        user_data = os.path.join(self.tmp_dir, self.user_data_path)
        prepare_dir(user_data)
        with open(user_data, "w") as f:
            d = {"runcmd": data.get("runcmd", []),
                 "bootcmd": data.get("bootcmd", [])}

            if "users" in data:
                for name, credentials in data["users"].iteritems():
                    if not "password" in credentials:
                        continue
                    d["runcmd"].extend([self.run_in_bash(self.useradd_command(name)),
                        self.run_in_bash(self.prepare_password_command(name, credentials))])

            if "write_files" in data:
                d["write_files"] = data["write_files"]
            f.write("#cloud-config\n")
            f.write(yaml.safe_dump(d, width=4096, allow_unicode=True))
            f.write("\n")

#
# Stores and restores our data
#
class Datastore:
    def __init__(self, path):
        self.datasource = path
        try:
            with open(self.datasource, "r") as f:
                self.data = json.load(f)
        except:
            self.data = {}

    def get_data(self):
        return self.data

    def assign_user(self, name, password=None, is_encrypted=None, ssh_keys=None):
        if not "users" in self.data:
            self.data["users"] = {name: {}}

        if not name in self.data["users"]:
            self.data["users"][name] = {}
        if password is not None:
            self.data["users"][name]["password"] = password
        if is_encrypted is not None:
            self.data["users"][name]["is_encrypted"] = is_encrypted
        if ssh_keys is not None:
            self.data["users"][name]["ssh_keys"] = ssh_keys

    def add_command(self, tag, command):
        if tag in self.data:
            self.data[tag].append(command)
        else:
            self.data[tag] = [command]

    def add_run_command(self, command):
        self.add_command("runcmd", command)

    def add_boot_command(self, command):
        self.add_command("bootcmd", command)

    def dump(self):
        if not self.data:
            return
        prepare_dir(self.datasource)
        with open(self.datasource, "w") as f:
            s = json.dumps(self.data, f, ensure_ascii=False, sort_keys=True,
                    indent=2, separators=(',', ': '))
            f.write(s)

    def merge(self, files):
        def append_without_duplicates(input, data):
            if type(data) in (list, tuple, dict):
                output = data
            else:
                output = [data]
            for x in input:
                if x not in output:
                    output.append(x)
            return output

        data = self.data
        for p in files:
            if not os.path.isfile(p):
                continue
            with open(p, "r") as f:
                try:
                    blocks = json.load(f)
                except Exception as e:
                    blocks = None
                if not blocks:
                    continue

                for key, value in blocks.iteritems():
                    if key in data:
                        data[key] = append_without_duplicates(value, data[key])
                    else:
                        data[key] = value
        self.data = data
        self.dump()

# Handles user settings translation
def handle_users(args):
    args.storage.assign_user(args.name, args.password, not args.not_encrypted, args.ssh_keys)
    args.storage.dump()

# handle VM commands that should be run at start or boot
def handle_boot_commands(args):
    args.storage.add_boot_command(args.command)
    args.storage.dump()

def handle_run_commands(args):
    args.storage.add_run_command(args.command)
    args.storage.dump()

def handle_merge(args):
    args.storage.merge(args.files)
    args.storage.dump()
    args.handler.prepare_iso(args.storage)

def create_cmd_parser(sp, tag, desc, handler):
    s = sp.add_parser(tag, help=desc)
    s.add_argument("command", default=[], metavar="", help="Command to be processed", nargs="*")
    s.set_defaults(func=handler)

def main():
    p = argparse.ArgumentParser(prog="Cloud-init config maker", description="Add VM personalization info")
    p.add_argument("--format", type=str, action="store", help="Specifies format of output to use (cloudbase-init or cloud-init)")
    p.add_argument("--datastore", type=str, action="store", help="Specifies output settings file")
    p.add_argument("--output-iso", type=str, action="store", help="Specifies output iso image")
    sp = p.add_subparsers(help="help for subcommand")

    user_subparser = sp.add_parser("user", help="Adds or modifies user-information")
    user_subparser.add_argument("name", type=str, action="store", help="Specifies user name")
    user_subparser.add_argument("--password", type=str, action="store", help="Specifies user password")
    user_subparser.add_argument("--not-encrypted", action="store_true",
            default=False, help="Set this option if you use plain text password")
    user_subparser.add_argument("--ssh-keys", default=[], metavar="", help="Specifies SSH keys for user", nargs="*")
    user_subparser.set_defaults(func=handle_users)

    create_cmd_parser(sp, "run-command", "Manage run commands list", handle_run_commands)
    create_cmd_parser(sp, "boot-command", "Manage boot commands list", handle_boot_commands)

    merge_subparser = sp.add_parser("merge", help="merge arbitrary cloud-init file")
    merge_subparser.add_argument("files", default=[], metavar="", help="Specifies files with cloud-init config", nargs="*")
    merge_subparser.set_defaults(func=handle_merge)

    args = p.parse_args()

    args.storage = Datastore(args.datastore)
    if args.format == "cloudbase-init":
        args.handler = CloudBaseInit(args.output_iso)
    else:
        args.handler = CloudInit(args.output_iso)
    args.func(args)

if __name__ == '__main__':
    main()
