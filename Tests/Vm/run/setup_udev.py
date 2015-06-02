#!/usr/bin/env python
"""
Configure the Linux host to make /dev/prl_* world-writable.

This script needs to be run as a privileged user.  It makes persistent changes
to the host configuration.  To undo the changes made by "add" subcommand, use
"del" subcommand.
"""

import os, sys

rule_file = "/etc/udev/rules.d/99-prl-devices.rules"

def udev_rule_add():
	fp = open(rule_file, "w")
	print >> fp, 'KERNEL=="prl_*", MODE="0666"'
	fp.close()

def udev_rule_del():
	os.unlink(rule_file)

def bail_out(msg = None):
	if msg:
		print >> sys.stderr, msg
	print >> sys.stderr, "Usage: %s add|del" % sys.argv[0]
	sys.exit(1)

if __name__ == "__main__":
	if len(sys.argv) != 2:
		bail_out("Invalid number of arguments")

	if sys.platform != "linux2":
		bail_out("Unsupported platform: %r" % sys.platform)

	verb = sys.argv[1]

	func = globals().get("udev_rule_" + verb)
	if not func:
		bail_out("Unknown verb: %s" % verb)

	func()
