#!/usr/bin/env python
"""
Configure the host to allow the user to elevate privileges running the
specified script (by default to load kernel modules).

This script needs to be run as a privileged user.  It makes persistent changes
to the host configuration.  To undo those changes made by "add" subcommand, use
a "del" subcommand with the same parameters.
"""

import os, sys

sudoers = "/etc/sudoers"
sudoers_d = "/etc/sudoers.d"
suffix_map = {
		"linux2":	"lin",
		"darwin":	"mac",
		}

def def_script():
	return os.path.abspath(os.path.join(os.path.dirname(__file__),
		"kmod_%s.py" % suffix_map.get(sys.platform)))

def sudoers_file(user, script):
	s = script.replace(os.path.sep, '-').replace('.', '_')
	fn = "99-prl-vm-test-%s-%s" % (user, s)
	return os.path.join(sudoers_d, fn)

def mk_sudo_entry(user, script):
	return "%s ALL= NOPASSWD: %s" % (user, script)

def match_sudo_entry(ent, user, script):
	ent_s = ent.split()
	return len(ent_s) > 1 and ent_s[0] == user and ent_s[-1] == script

def sudo_entry_add(user, script = def_script()):
	sudo_entry_del(user, script)
	if os.path.isdir(sudoers_d):
		fn = sudoers_file(user, script)
		fp = open(fn, "w")
		os.fchmod(fp.fileno(), 0600)
	else:
		fp = open(sudoers, "a")
	print >> fp, mk_sudo_entry(user, script)
	fp.close()

def sudo_entry_del(user, script = def_script()):
	if os.path.isdir(sudoers_d):
		fn = sudoers_file(user, script)
		if os.path.exists(fn):
			os.unlink(fn)
	else:
		fp = open(sudoers, "r+")
		data = ''.join(l for l in fp
				if not match_sudo_entry(l, user, script))
		fp.seek(0)
		fp.truncate(0)
		fp.write(data)
		fp.close()

def bail_out(msg = None):
	if msg:
		print >> sys.stderr, msg
	print >> sys.stderr, "Usage: %s add|del <user> [<script>]" % sys.argv[0]
	sys.exit(1)

if __name__ == "__main__":
	if len(sys.argv) < 3 or len(sys.argv) > 4:
		bail_out("Invalid number of arguments")

	if sys.platform not in ("linux2", "darwin"):
		bail_out("Unsupported platform: %r" % sys.platform)

	verb = sys.argv[1]

	func = globals().get("sudo_entry_" + verb)
	if not func:
		bail_out("Unknown verb: %s" % verb)

	func(*sys.argv[2:])
