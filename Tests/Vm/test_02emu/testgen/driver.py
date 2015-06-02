#!/usr/bin/python

import os
import sys
import string
import subprocess
import shutil
import time
import struct
import re
import select
import traceback
import itertools
import tempfile
import glob
import argparse
import random
import logging
import mmap
import signal
from multiprocessing import cpu_count

from vmtest import VmImg, VmApp

from gen import gen_variants
from ctx import gen_ctx_map


heredir = os.path.dirname(__file__)

# -----------------------------------------------------------------------------

ap = argparse.ArgumentParser()
ap.add_argument('-f', '--fail-dir', default='.', help='specifies the directory to store failed test cases')
ap.add_argument('-t', '--temp-storage', default='.', help='specifies the directory to store VMs')
ap.add_argument('-b', '--burst-size', type=int, default=1000, help='specifies the number of tests executed by a single VM')
ap.add_argument('-w', '--number-of-workers', type=int, default=cpu_count(), help='specifies the number of VMs running simultaneously')
ap.add_argument('-s', '--sample', type=int, help='specifies the ratio of the size of the random sample taken from the instruction table to the size of the table')
ap.add_argument('-l', '--failure-limit', type=int, help='specifies the number of failures to abort the test')
ap.add_argument('-o', '--system-flags', default='', help='additional system flags to be passed to a test VM')
ap.add_argument('-d', '--vm-watchdog-timeout', default=120, type=int, help='specifies the maximum VM running time')
ap.add_argument('-i', '--test-module', help='test description module')
opts = ap.parse_args()

# -----------------------------------------------------------------------------

if not opts.test_module.isalnum():
	logging.error('Invalid test module: "%s"', opts.test_module)
	quit(1)

exec "from %s import insn_descs" % (opts.test_module)

sf = """
devices.apm.enable=0;
devices.acpi=0;
devices.apic.disable=1;
devices.hpet.enable=0;
vm.ballooning.enabled=0;
vm.snapshot_compressed=0;
vm.mem_compressed=0;
vm.mem_anonymous=0;
kernel.emu_instr_print=1;
kernel.lock_by_block=0;
"""

vm_mem = (opts.burst_size*4*4096 + 2*1024**2)/(1024**2)
tests_in_burst = (vm_mem - 1)*1024*1024/(4*4096) - 4

nw = 1
if opts.number_of_workers > 0:
	nw = opts.number_of_workers
else:
	nw = cpu_count() - opts.number_of_workers

vm_dir = tempfile.mkdtemp(prefix = 'emutest', dir = opts.temp_storage)

fail_dir = tempfile.mkdtemp(prefix = 'fail-', dir = opts.fail_dir)

sf += opts.system_flags

test_switcher_pa = 1024**2
test_start_pa = 1024**2 + 16*4096

# -----------------------------------------------------------------------------

size_map = { 1 : 'B', 2 : 'H', 4 : 'I', 8 : 'Q' }

def read_all(filename):
	f = os.open(filename, os.O_RDONLY)
	res = os.read(f, os.fstat(f).st_size)
	os.close(f)
	return bytearray(res)



def write_all(filename, content, truncate):
	open_flags = os.O_WRONLY | os.O_CREAT
	if truncate:
		open_flags |= os.O_TRUNC
	f = os.open(filename, open_flags)
	os.write(f, str(content))
	os.close(f)



def write_vm_config(vm):
	vm.setSystemFlags(sf)
	vm.setIsoPath(os.path.abspath(os.path.join(heredir, '..', 'guest', 'boot-64.iso')))
	vm.setCpuNumber(1)
	vm.setMemorySize(vm_mem)
	vm.setVideoMemorySize(2)
	vm.removeNetwork()
	vm.writeCfg()



def bootstrap_vm(vm_dir):
	vm = VmImg(vm_dir)
	write_vm_config(vm)
	res = VmApp(os.path.join(vm_dir, 'config.pvs')).run(opts.vm_watchdog_timeout)
	if res != 0:
		raise Exception('The bootstrap VM process exited with code', res)

	swaps = glob.glob(os.path.join(vm_dir, '*.mem'))
	if len(swaps) == 0:
		raise Exception("There's no memory snapshot in the bootstrap VM directory")
	elif len(swaps) != 1:
		raise Exception("There're more than one memory snapshot in the VM directory")

	swap_name = os.path.basename(swaps[0])

	# remove the infinite loop in the beginning of the test switcher
	swapf = open(swaps[0], "r+b")
	swapf.seek(test_switcher_pa)
	swapf.write("\x90\x90")
	swapf.close()

	return swap_name



def write_at(m, offset, value):
	m[offset:(offset + len(value))] = value


def init_test(fmem, test, subtest):
	def rnd_string(size):
		return str(bytearray([random.choice(xrange(0, 0x100)) for i in xrange(0, size)]))

	class f:
		def __init__(self, offset, size):
			self.offset = offset
			self.size = size

		def write(self, fmem, offset, value):
			if isinstance(value, str):
				write_at(fmem, offset + self.offset, value)
			else:
				struct.pack_into(size_map[self.size], fmem, offset + self.offset, value)



	ctx_map = dict(gen_ctx_map(lambda x: (x[0], f(x[1], x[2]))))

	context_switcher_va = 0x1000
	code_inject_size = 4096
	context_inject_size = 1024
	code_va = 0x5000
	data_inject = test_start_pa + 1*0x1000 + 4*0x1000*subtest
	code_inject = test_start_pa + 2*0x1000 + 4*0x1000*subtest
	context_inject = test_start_pa + 3*0x1000 + 4*0x1000*subtest

	write_at(fmem, context_inject, "\x00"*context_inject_size)

	ctx_map['rax'].write(fmem, context_inject, ''.join(map(lambda i: struct.pack('Q', 0xdead << 48 | i << 32 | 0xdead << 16 | i), range(1, 15))))
	ctx_map['fcw'].write(fmem, context_inject, 0x037f)
	ctx_map['fsw'].write(fmem, context_inject, 0x0000)
	ctx_map['mxcsr'].write(fmem, context_inject, 0x1f80)
	ctx_map['mxcsr_mask'].write(fmem, context_inject, 0xffff)
	ctx_map['xstatebv'].write(fmem, context_inject, 0x0007)
	ctx_map['xmm0'].write(fmem, context_inject, rnd_string(16*16))
	ctx_map['ymmh0'].write(fmem, context_inject, rnd_string(16*16))
	ctx_map['mm0'].write(fmem, context_inject, ''.join(map(lambda i: rnd_string(10) + '\x00'*6, range(8))))
	ctx_map['xcr0'].write(fmem, context_inject, 7)

	for s in test.state:
		if not isinstance(s, tuple):
			raise Exception('Unsupported state description', s)

		cell = s[0]
		val = s[1]

		if isinstance(cell, int):
			if isinstance(val, str):
				write_at(fmem, data_inject + cell, val)
			else:
				struct.pack_into('Q', fmem, data_inject + cell, val)

		elif isinstance(cell, str):
			field_name = cell.lower()
			if field_name in ctx_map:
				ctx_map[field_name].write(fmem, context_inject, val)
			else:
				raise Exception('Invalid field', field_name)

		else:
			raise Exception('Unknown state description')

	def flat_code(item):
		if isinstance(item, tuple):
			return struct.pack(size_map[item[0]], item[1])
		else:
			if item <= 0xff:
				return str(bytearray([item]))
			else:
				raise Exception('Invalid instruction byte',  hex(item))

	code = ''.join(map(flat_code, test.insn))

	write_at(fmem, code_inject, "\xcc"*0x1000)
	write_at(fmem, code_inject, code)




def compare(mem1, mem2):
	return subprocess.call(['cmp', '-s', '-i', str(test_start_pa), mem1, mem2]) == 0


# -----------------------------------------------------------------------------

class worker:
	swap_name = None
	template_sav_content = None
	template_mem_content = None

	def __init__(self, vmid):
		self.vmid = vmid
		self.cfail = 0

		self.vm_dir = os.path.join(vm_dir, str(vmid))
		self.cfg = os.path.join(self.vm_dir, 'config.pvs')
		self.log = os.path.join(self.vm_dir, 'parallels.log')
		self.sav = os.path.join(self.vm_dir, 'config.sav')
		self.mem = os.path.join(self.vm_dir, worker.swap_name)

		self.vm = VmImg(self.vm_dir)
		write_vm_config(self.vm)
		os.mkdir(os.path.join(self.vm_dir, 'test'))

		self.mem_hvt = os.path.join(self.vm_dir, 'test', 'hvt.mem')
		self.log_hvt = os.path.join(self.vm_dir, 'test', 'hvt.log')
		self.mem_emu = os.path.join(self.vm_dir, 'test', 'emu.mem')
		self.log_emu = os.path.join(self.vm_dir, 'test', 'emu.log')
		self.init_mem = os.path.join(self.vm_dir, 'test', 'init.mem')



	def run_test(self, mode):
		struct.pack_into('I', worker.template_mem_content, test_switcher_pa + 0x2000 + 0x20, mode)
		write_all(self.sav, worker.template_sav_content, True)
		write_all(self.mem, worker.template_mem_content, False)

		if VmApp(self.cfg).run(opts.vm_watchdog_timeout) != 0:
			raise Exception('The VM has exited unexpectedly')



	def doit(self, tests):
		subtests = len(tests)
		for i in range(subtests):
			init_test(worker.template_mem_content, tests[i], i)

		struct.pack_into('I', worker.template_mem_content, test_switcher_pa + 0x2000, subtests - 1)


		try:
			self.run_test(0)
			os.rename(self.mem, self.mem_hvt)
			os.rename(self.log, self.log_hvt)

			self.run_test(1)
			os.rename(self.mem, self.mem_emu)
			os.rename(self.log, self.log_emu)

			if not compare(self.mem_hvt, self.mem_emu):
				d = os.path.join(fail_dir, str(self.vmid), str(self.cfail))
				os.makedirs(d)

				shutil.move(self.mem_hvt, d)
				shutil.move(self.mem_emu, d)
				shutil.move(self.log_emu, d)
				shutil.move(self.log_hvt, d)
				write_all(os.path.join(d, 'init.mem'), worker.template_mem_content, False)

				return 1


		except OSError as e:
			d = os.path.join(fail_dir, str(self.vmid), str(self.cfail))
			if not os.path.lexists(d):
				os.makedirs(d)

			for f in [self.init_mem, self.log]:
				if os.path.lexists(f):
					shutil.copy(f, d)

			return 1

		except Exception:
			shutil.copytree(self.vm_dir, os.path.join(fail_dir, str(self.vmid), str(self.cfail)))
			logging.exception('Worker %d got an exception', self.vmid)
			return 2

		return 0


# -----------------------------------------------------------------------------

def launch(tasks, worker):
	burst = list(itertools.islice(tasks, tests_in_burst))
	pid = 0

	if len(burst) > 0:
		pid = os.fork()
		if pid == 0:
			quit(worker.doit(burst))

	return pid



def main():
	vm_template_dir = os.path.join(vm_dir, 'emutest-vm-template')
	swap_name = None

	try:
		swap_name = bootstrap_vm(vm_template_dir)

	except Exception as e:
		shutil.copytree(vm_template_dir, os.path.join(fail_dir, 'emutest-vm-template'))
		raise e;

	worker.template_sav_content = read_all(os.path.join(vm_template_dir, 'config.sav'))
	worker.template_mem_content = read_all(os.path.join(vm_template_dir, swap_name))
	worker.swap_name = swap_name

	tasks = itertools.chain.from_iterable(itertools.imap(gen_variants, insn_descs))
	if opts.sample:
		"""
		We use the Bernoulli trials that guarantee only that
		the expected number of fetched tasks is opts.sample/100*len(tasks)
		since there's no way to determine len(tasks) in reasonable time
		"""
		tasks = itertools.ifilter(lambda x: random.randint(0, 99) <= opts.sample, tasks)

	children = dict()
	workers = []
	for i in range(nw):
		w = worker(i)
		pid = launch(tasks, w)
		if pid == 0:
			break
		workers.append(w)
		children[pid] = w.vmid


	fail_reason = 0

	while len(children) > 0 and fail_reason == 0:
		pid, status = os.wait3(0)[0:2]

		finished_worker = workers[children[pid]]
		children.pop(pid)

		if os.WEXITSTATUS(status) == 1:
			finished_worker.cfail += 1
		elif os.WEXITSTATUS(status) != 0:
			logging.error('Worker %d exited with status %x', finished_worker.vmid, status)
			fail_reason = 4
			continue

		pid = launch(tasks, finished_worker)
		if pid != 0:
			children[pid] = finished_worker.vmid

		if opts.failure_limit is not None:
			if sum([w.cfail for w in workers]) >= opts.failure_limit:
				logging.error('Failure limit reached, aborting the test')
				fail_reason = 32


	for c in children.keys():
		os.waitpid(c, 0)

	if len(os.listdir(fail_dir)) > 0:
		logging.error('Failed tests found')
		fail_reason |= 2

	if fail_reason <= 2:
		shutil.rmtree(vm_dir)
	if fail_reason == 0:
		shutil.rmtree(fail_dir)


	quit(fail_reason)



if __name__ == "__main__":
	main()
