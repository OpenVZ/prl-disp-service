#!/usr/bin/env python
"""
Basic test for booting of a VM
"""

import sys, os, time, unittest, re
from logging import debug

import vmtest

heredir, fname = os.path.split(__file__)
testdir = os.path.join(heredir, os.path.splitext(fname)[0])

timeout = 60

msg_s = r">>>>>>>>\|"
msg_e = r"\|<<<<<<<<"
cpus_re = re.compile(msg_s + r"CPUs: (\d+)" + msg_e)
ram_re = re.compile(msg_s + r"RAM: (\d+)" + msg_e)

class BootTest(unittest.TestCase):
	def setUp(self):
		vmdir = "%s-vm-%d" % (testdir, int(time.time()))
		self.vm = vmtest.VmImg(vmdir)
		self.boot_iso = self.vm.getIso()
		vmtest.mktestiso(self.boot_iso, testdir)
		self.serlog = vmtest.GrepLog(self.vm.getSerialLog())

		self.ncpus = int(self.vm.cfg.find("./Hardware/Cpu/Number").text)
		self.ram = int(self.vm.cfg.find("./Hardware/Memory/RAM").text)

	def tearDown(self):
		self.serlog.close()

	def testBoot(self):
		self.vmapp = vmtest.VmApp(self.vm.cfgfile)
		self.vmapp.write("vm.VMStart()\n")

		cpus_match = self.serlog.greploop(cpus_re.match, timeout)
		self.assert_(cpus_match)
		ncpus = int(cpus_match.group(1))
		self.assertEqual(ncpus, self.ncpus)

		ram_match = self.serlog.greploop(ram_re.match, timeout)
		self.assert_(ram_match)
		ram_mb = int(ram_match.group(1)) >> 20
		self.assert_(0 < self.ram - ram_mb < 50)

		self.vmapp.write("quit()\n")
		self.assertEqual(self.vmapp.wait(), 0)

if __name__ == "__main__":
	import logging
	logging.getLogger().setLevel(logging.DEBUG)
	unittest.main()
