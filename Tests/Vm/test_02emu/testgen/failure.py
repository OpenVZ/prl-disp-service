#!/usr/bin/python

import os
import sys
import subprocess
import struct

from ctx import gen_ctx_map


ctx_map = gen_ctx_map(lambda x: x)


def compare(mem1, mem2, out, init_mem):
	tc_size = 4*0x1000
	tc_begin = 1024*1024 + 16*0x1000
	tc_code = 2*0x1000

	ntests = (os.stat(mem1).st_size - 3*1024*1024)/(4*0x1000) - 4

	for i in range(0, ntests):
		tc_pos = tc_begin + i*tc_size

		p = subprocess.Popen(['bash', '-c', 'diff <(hexdump -v -n %d -s %d %s) <(hexdump -v -n %d -s %d %s)' %
								(tc_size, tc_pos, mem1, tc_size, tc_pos, mem2)],
								 stdout = subprocess.PIPE,
								 stderr = subprocess.PIPE)

		if p.wait() != 0:
			dsm = ''

			out.write('='*80 + "\n")
			out.write('Test %d failed' % i)

			out.write("\n\nFailed code is\n")
			dis = subprocess.Popen(['ndisasm', '-b' + sys.argv[2], '-k', '0,%d' % (tc_pos + tc_code), mem1],
									stdout = subprocess.PIPE,
									stderr = subprocess.PIPE)
			dis.stdout.readline()
			for j in range(0, 10):
				out.write(dis.stdout.readline())
			dis.terminate()

			out.write("\n\nInitial state\n")
			for f in ctx_map:
				size_map = { 1 : 'B', 2 : 'H', 4 : 'I', 8 : 'Q' }
				init_mem.seek(tc_pos + 0x1000*3 + f[1])
				out.write("%s = %s\n" % (f[0], hex(struct.unpack(size_map[f[2]], init_mem.read(f[2]))[0])))


			out.write("\n\nMachine state difference is\n")
			out.write(p.stdout.read())
			out.write("\n" + '='*80 + "\n\n")




for vm in os.listdir(os.path.join(sys.argv[1])):
	for t in os.listdir(os.path.join(sys.argv[1], vm)):
		print 'Generating the failure report for the testcase %s failed in the VM %s' % (t, vm)

		fail_path = os.path.join(sys.argv[1], vm, t)
		hvt_mem = os.path.join(fail_path, 'hvt.mem')
		emu_mem = os.path.join(fail_path, 'emu.mem')
		if not all(map(os.path.lexists, [hvt_mem, emu_mem])):
			print 'The testcase contains a VMM abort'
			print
			continue

		compare(hvt_mem, emu_mem,
				open(os.path.join(fail_path, 'error'), 'w'),
				open(os.path.join(fail_path, 'init.mem'), 'r')
				)
