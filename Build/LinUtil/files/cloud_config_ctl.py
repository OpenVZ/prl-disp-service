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

import yaml
import sys
import argparse
import os
import tempfile
import shutil
import ast

output_file = input_file = "user-config"
iso_dir = ""

def report_error(message):
    sys.stderr.write("Error: " + message + "\n")
    if iso_dir:
        shutil.rmtree(iso_dir)
    sys.exit(1)

# Writes result to specified path in cloud-init format
def save_result(result, path):
    f = open(output_file, "w+")
    f.write("#cloud-config\n")
    f.write(yaml.dump(result, width=4096))

# Handles user settings translation
# e.g.
# cloud_ctl.py user john --password pass --not-encrypted --ssh-keys 'first key' 'second key'
# will result in
# users:
# - name: john
#   passwd: pass
#   ssh-authorized-keys: ['first key', 'second key']
def handle_users(args, data):
    def create_user(args):
        if args.not_encrypted:
            passwd_tag = "plain_text_passwd"
        else:
            passwd_tag = "passwd"

        t = {"name": args.name, passwd_tag: args.password}
        if "ssh_keys" in vars(args):
            t["ssh-authorized-keys"] = vars(args)["ssh_keys"]
        return t

    def merge_user(lhs, rhs):
        if "password" in vars(args) or "not_encrypted" in vars(args):
            if "passwd" in lhs:
                p = lhs["passwd"]
                del lhs["passwd"]
            elif 'plain_text_passwd' in lhs:
                p = lhs["plain_text_passwd"]
                del lhs["plain_text_passwd"]
            if "password" in vars(args):
                p = vars(args)["password"]
            if args.not_encrypted:
                lhs["plain_text_passwd"] = p
            else:
                lhs["passwd"] = p

        if "ssh_keys" in vars(args):
            lhs["ssh-authorized-keys"].extend(vars(args)["ssh_keys"])
        return lhs

    def check_data(args):
        if not "ssh_keys" in vars(args) and not "password" in vars(args):
            report_error("Specify password or ssh-key")

    if not data:
        data = {}

    found = False
    for key, value in data.iteritems():
        if "users" != key:
            continue
        found = True
        user_already_exists = False
        for n, j in enumerate(data[key]):
            if j["name"] != args.name:
                continue
            user_already_exists = True
            data[key][n] = merge_user(j, create_user(args))
        if user_already_exists:
            break
        check_data(args)
        data[key].insert(0, create_user(args))

    if not found:
        check_data(args)
        data["users"]=[create_user(args)]

    save_result(data, output_file)

# handle VM commands that should be run at start or boot
def handle_commands(tag, args, data):
    found = False

    if not args.command:
        return

    if not data:
        data = {}

    for key, value in data.iteritems():
        if tag != key:
            continue
        found = True
        if args.action == "add":
            data[key].insert(0, args.command)
        elif args.action == "delete":
            if not args.command in value:
                continue
            data[key].remove(args.command)
            if not data[key]:
                data.pop(key, None)
    if not found and args.action == "add":
        data[tag] = [args.command]

    save_result(data, output_file)

def handle_boot_commands(args, data):
    handle_commands("bootcmd", args, data)

def handle_run_commands(args, data):
    handle_commands("runcmd", args, data)

# We cannot use cloud-init network settings cause they need network device name
# prl_disp_service uses prl_nettool program for handling networking parameters 
# We should overwrite this command every time when networking settings changed
def handle_nettool_commands(args, data):
    found = False

    if not args.command:
        return

    if not data:
        data = {}

    for key, value in data.iteritems():
        if "runcmd" != key:
            continue
        found = True
        prl_nettool_found = False
        for n, j in enumerate(data[key]):
            try:
                l = ast.literal_eval(str(j))
                if os.path.basename(l[0].split(' ', 1)[0]).startswith("prl_nettool"):
                    prl_nettool_found = True
                    data[key][n] = args.command
                    break
            except:
                continue
        if not prl_nettool_found:
            data[key].insert(0, args.command)
    if not found:
        data["runcmd"] = [args.command]

    save_result(data, output_file)

def handle_merge(args, data):
    def append_without_duplicates(input, data):
        if type(data) is list or type(data) is tuple:
            output = data
        else:
            output = [data]
        for x in input:
            if x not in output:
                output.append(x)
        return output

    if not args.files:
        return

    data = {}
    for p in args.files:
        if not os.path.isfile(p):
            continue
        with open(p, "r") as f:
            blocks = yaml.load(f)
            if not blocks:
                continue

            for key, value in blocks.iteritems():
                if key in data:
                    data[key] = append_without_duplicates(blocks[key], data[key])
                else:
                    data[key] = blocks[key]

    if data:
        save_result(data, output_file)

def create_cmd_parser(sp, tag, desc, handler):
    s = sp.add_parser(tag, help=desc)
    s.add_argument("action", type=str, action="store", help="Action (add or delete)")
    s.add_argument("command", default=[], metavar="", help="Command to be processed", nargs="*")
    s.set_defaults(func=handler)

p = argparse.ArgumentParser(prog="Cloud-init config maker", description="Add VM personalization info")
p.add_argument("--output", type=str, action="store", help="Specifies output file")
p.add_argument("--input", type=str, action="store", help="Specifies input file")
p.add_argument("--output-iso", type=str, action="store", help="Specifies output iso image")
p.add_argument("--input-iso", type=str, action="store", help="Specifies input iso image")
sp = p.add_subparsers(help="help for subcommand")

user_subparser = sp.add_parser("user", help="Adds or modifies user-information")
user_subparser.add_argument("name", type=str, action="store", help="Specifies user name")
user_subparser.add_argument("--password", type=str, action="store", help="Specifies user password")
user_subparser.add_argument("--ssh-keys", default=[], metavar="", help="Specifies SSH keys for user", nargs="*")
user_subparser.add_argument("--not-encrypted", action="store_true",
        default=False, help="Set this option if you use plain text password")
user_subparser.set_defaults(func=handle_users)

create_cmd_parser(sp, "run-command", "Manage run commands list", handle_run_commands)
create_cmd_parser(sp, "boot-command", "Manage boot commands list", handle_boot_commands)
nettool_subparser = sp.add_parser("nettool-command", help="Manage prl-nettool commands list")
nettool_subparser.add_argument("command", default=[], metavar="", help="Command to be processed", nargs="*")
nettool_subparser.set_defaults(func=handle_nettool_commands)

merge_subparser = sp.add_parser("merge", help="merge arbitrary cloud-init file")
merge_subparser.add_argument("files", default=[], metavar="", help="Specifies files with cloud-init config", nargs="*")
merge_subparser.set_defaults(func=handle_merge)

args = p.parse_args()

if args.input_iso:
    if os.path.isfile(args.input_iso):
        iso_dir = tempfile.mkdtemp()
        iso_dir = os.path.join(iso_dir, "")
        if os.system("/usr/bin/7z x " + args.input_iso + " -o" + iso_dir) != 0:
            report_error("7z failed")
    input_file = iso_dir + input_file

if args.input:
    input_file = iso_dir + args.input
    if os.path.exists(input_file) and not os.path.isfile(input_file):
        report_error(input_file + " is not a file")

tmp = args.output if args.output else output_file
if args.output_iso:
    output_file = iso_dir + tmp
else:
    output_file = tmp

blocks = {}
if os.path.isfile(input_file):
    with open(input_file, "r") as f:
        blocks = yaml.load(f)
        if not blocks:
            blocks = {}

args.func(args, blocks)

if args.output_iso:
    if not iso_dir:
        iso_dir = os.path.join(tempfile.mkdtemp(), "")
        shutil.copy(output_file, iso_dir)

    if os.system("/usr/bin/genisoimage -output " + args.output_iso +
            " -volid cidata -joliet -rock " + iso_dir + "*") != 0:
        report_error("genisoimage failed")

if iso_dir:
    shutil.rmtree(iso_dir)

