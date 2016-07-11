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

    def prepare_script(self, commands):
        # codepage should be fixed to correctly use unicode
        s = u"rem cmd\r\n\r\nchcp 65001\r\n\r\n"
        for command in commands:
            if type(command) is unicode:
                s += command + u"\r\n"
            else:
                # windows cmd supports only double quotes
                s += unicode(u" ".join(command)).replace(u"'", u"\"") + u"\r\n"
        return s

    def prepare_password_command(self, name, credentials):
        if credentials.get(u"is_encrypted", False):
            return u""
        p = credentials[u"password"].replace("'", "''")
        s = u"net user '{}' '{}'".format(name, p)
        r = u"{} || {} /add".format(s, s)
        return r

    def write_meta_data(self):
        meta_data = os.path.join(self.tmp_dir, self.meta_data_path);
        prepare_dir(meta_data)
        with io.open(meta_data, "w", newline=u"\r\n") as f:
            d = {u"instance-id": str(uuid.uuid1()).encode("unicode-escape")}
            f.write(json.dumps(d, meta_data, ensure_ascii=False, sort_keys=True,
                    indent=2, separators=(u',', u': ')))
            f.write(u"\r\n")

    def write_user_data(self, data):
        s = []
        s.extend(data.get(u"runcmd", []))
        s.extend(data.get(u"bootcmd", []))
        if u"nettoolcmd" in data:
            s.append(data[u"nettoolcmd"])
        user_data = os.path.join(self.tmp_dir, self.user_data_path);
        prepare_dir(user_data)
        if u"users" in data:
            for name, credentials in data[u"users"].iteritems():
                x = self.prepare_password_command(name, credentials)
                if not x:
                    continue
                s.append(x)

        with open(user_data, 'w') as f:
            f.write(self.prepare_script(s).encode("utf-8"))
            f.write(u"\r\n".encode("utf-8"))

class CloudInit(OpenStackConfigDrive):
    def __init__(self, iso):
        OpenStackConfigDrive.__init__(self, iso)

    def __del__(self):
        OpenStackConfigDrive.__del__(self)

    def write_meta_data(self):
        meta_data = os.path.join(self.tmp_dir, self.meta_data_path);
        prepare_dir(meta_data)
        with open(meta_data, "w") as f:
            d = {u"instance-id": str(uuid.uuid1()).decode("unicode-escape"),
                    u"uuid": str(uuid.uuid1()).decode("unicode-escape")}
            json.dump(d, f, sort_keys=True, ensure_ascii=False,
                    indent=2, separators=(u",", u": "))
            f.write("\n")

    def prepare_password_command(self, user, credentials):
        if credentials.get(u"is_encrypted", True):
            return u"echo -e '{0}':'{1}' | chpasswd -e".format(user, credentials[u"password"])
        b = base64.b64encode(credentials[u"password"].encode("unicode-escape"))
        return u"echo -e '{0}':\"'\"$(echo '{1}'  | base64 -w 0 --decode)\"'\" | chpasswd".format(user, b)

    def useradd_command(self, user):
        return u"/usr/sbin/useradd {0} 1>/dev/null 2>&1 || :".format(user)

    def run_in_bash(self, cmd):
        return [u"/bin/bash", u"-c", cmd]

    def write_user_data(self, data):
        user_data = os.path.join(self.tmp_dir, self.user_data_path)
        prepare_dir(user_data)
        with open(user_data, "w") as f:
            d = {u"runcmd": data.get(u"runcmd", []),
                 u"bootcmd": data.get(u"bootcmd", [])}
            if u"nettoolcmd" in data:
                d[u"runcmd"].extend(data[u"nettoolcmd"])

            if u"users" in data:
                for name, credentials in data[u"users"].iteritems():
                    if not "password" in credentials:
                        continue
                    d[u"runcmd"].extend([self.run_in_bash(self.useradd_command(name)),
                        self.run_in_bash(self.prepare_password_command(name, credentials))])

            if u"write_files" in data:
                d[u"write_files"] = data[u"write_files"]
            f.write(u"#cloud-config\n")
            f.write(yaml.safe_dump(d, width=4096, allow_unicode=True))
            f.write(u"\n")

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
        if not u"users" in self.data:
            self.data[u"users"] = {name: {}}

        if not name in self.data[u"users"]:
            self.data[u"users"][name] = {}
        if password is not None:
            self.data[u"users"][name][u"password"] = password
        if is_encrypted is not None:
            self.data[u"users"][name][u"is_encrypted"] = is_encrypted
        if ssh_keys is not None:
            self.data[u"users"][name][u"ssh_keys"] = ssh_keys

    def add_command(self, tag, command):
        if tag in self.data:
            self.data[tag].append(command)
        else:
            self.data[tag] = [command]

    def add_run_command(self, command):
        self.add_command(u"runcmd", command)

    def add_boot_command(self, command):
        self.add_command(u"bootcmd", command)

    def assign_nettool_command(self, command):
        self.data[u"nettoolcmd"] = command

    def dump(self):
        if not self.data:
            return
        prepare_dir(self.datasource)
        with open(self.datasource, "w") as f:
            s = json.dumps(self.data, f, ensure_ascii=False, sort_keys=True,
                    indent=2, separators=(u',', u': '))
            f.write(s.encode("utf-8"))

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
                    # nettoolcmd cannot be merged,
                    # should only appear in input file
                    if key == u"nettoolcmd":
                        continue
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

# prl_disp_service uses prl_nettool program for handling networking parameters
# We should overwrite this command every time when networking settings changed
def handle_nettool_commands(args):
    args.storage.assign_nettool_command(args.command)
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
    def unicode_convert(s):
        return unicode(s, "utf8")
    p = argparse.ArgumentParser(prog="Cloud-init config maker", description="Add VM personalization info")
    p.add_argument("--format", type=unicode_convert, action="store", help="Specifies format of output to use (cloudbase-init or cloud-init)")
    p.add_argument("--datastore", type=unicode_convert, action="store", help="Specifies output settings file")
    p.add_argument("--output-iso", type=unicode_convert, action="store", help="Specifies output iso image")
    sp = p.add_subparsers(help="help for subcommand")

    user_subparser = sp.add_parser("user", help="Adds or modifies user-information")
    user_subparser.add_argument("name", type=unicode_convert, action="store", help="Specifies user name")
    user_subparser.add_argument("--password", type=unicode_convert, action="store", help="Specifies user password")
    user_subparser.add_argument("--not-encrypted", action="store_true",
            default=False, help="Set this option if you use plain text password")
    user_subparser.add_argument("--ssh-keys", default=[], metavar="", help="Specifies SSH keys for user", nargs="*")
    user_subparser.set_defaults(func=handle_users)

    create_cmd_parser(sp, "run-command", "Manage run commands list", handle_run_commands)
    create_cmd_parser(sp, "boot-command", "Manage boot commands list", handle_boot_commands)
    create_cmd_parser(sp, "nettool-command", "Manage prl-nettool commands list", handle_nettool_commands)

    merge_subparser = sp.add_parser("merge", help="merge arbitrary cloud-init file")
    merge_subparser.add_argument("files", default=[], metavar="", help="Specifies files with cloud-init config", nargs="*")
    merge_subparser.set_defaults(func=handle_merge)

    args = p.parse_args()

    args.storage = Datastore(args.datastore)
    if args.format == u"cloudbase-init":
        args.handler = CloudBaseInit(args.output_iso)
    else:
        args.handler = CloudInit(args.output_iso)
    args.func(args)

if __name__ == '__main__':
    main()
