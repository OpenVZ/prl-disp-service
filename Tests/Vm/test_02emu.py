#!/usr/bin/env python
"""
Emulation test wrapper
"""

import os
import sys
import re
import subprocess
import unittest
import time
import shutil
import glob
from multiprocessing import cpu_count


heredir = os.path.dirname(__file__)


def run(*args):
	p = subprocess.Popen(args, stdout = subprocess.PIPE)
	res = p.communicate()
	return (p.returncode, res[0])


class EmuTest(unittest.TestCase):
	@classmethod
	def setUpClass(cls):
		nullfd = cls.nullfd = os.open(os.devnull, os.O_WRONLY)

		if sys.platform == 'darwin':
			zbuild_path = os.path.abspath(os.path.join(heredir, '..', '..', '..', 'z-Build'))
			if os.path.exists(os.path.join(zbuild_path, 'Debug', 'nasm.app')):
				os.environ['PATH'] = os.path.join(zbuild_path, 'Debug', 'nasm.app', 'Contents', 'MacOS') + ':' + os.environ['PATH']
			elif os.path.exists(os.path.join(zbuild_path, 'Release', 'nasm.app')):
				os.environ['PATH'] = os.path.join(zbuild_path, 'Release', 'nasm.app', 'Contents', 'MacOS') + ':' + os.environ['PATH']

		if subprocess.call(['make', '-C', os.path.join(heredir, 'test_02emu')], stdout = nullfd, stderr = nullfd) != 0:
			raise Exception('Failed to build the test guest!')

		cls.tmp_on_run = False

		if sys.platform == 'darwin':
			if os.path.exists('/Volumes/emutestvms'):
				(ret, _) = run("bash", "-c", "hdiutil detach $(mount | grep /Volumes/emutestvms | cut -d ' ' -f 1)")
				if ret != 0:
					raise Exception('Failed to detach the temporary storage')

			vm_size_est = 78*1024**2
			stor_size_in_blocks = int((vm_size_est*cpu_count() + 1024**3)/1024**3)*1024**3/256
			(ret, disk) = run('hdiutil', 'attach', '-nomount', 'ram://%d' % (stor_size_in_blocks))
			if ret != 0:
				raise Exception('Failed to create a RAM disk!')

			cls.ramdisk = re.match('(\S+)\s', disk).group(1)

			if run('diskutil', 'erasevolume', 'JHFS+', 'emutestvms', cls.ramdisk)[0] != 0:
				raise Exception( 'Failed to erase the RAM disk!')

			cls.tmp_stor = '/Volumes/emutestvms'


		elif sys.platform == 'linux2':
			(ret, mounts) = run('mount')
			if ret != 0:
				raise Exception('Failed to get mounts!')

			cls.tmp_stor = None
			tmpfs_on_run = False

			for m in mounts.split("\n"):
				r = re.match('(\w+) on (\S+)', m)
				if r is not None:
					if r.group(2) == '/run':
						tmpfs_on_run = True

					elif r.group(1) == 'emutestvms':
						cls.tmp_stor = r.group(2)
						break

			if cls.tmp_stor is None and tmpfs_on_run:
				uid = os.getuid()

				if os.path.exists(os.path.join('/run', 'user', str(uid))):
					cls.tmp_stor = os.path.join('/run', 'user', str(uid), 'emutestvms')

				if cls.tmp_stor is not None:
					cls.tmp_on_run = True
					shutil.rmtree(cls.tmp_stor, True)
					os.mkdir(cls.tmp_stor)



		cls.run_dir = os.path.join(heredir, 'test_02emu-%d' % (int(time.time())))
		os.mkdir(cls.run_dir)

		if cls.tmp_stor is None:
			cls.tmp_stor = os.path.join(cls.run_dir, 'vm')
			os.mkdir(cls.tmp_stor)



	@classmethod
	def tearDownClass(cls):
		os.close(cls.nullfd)

		if sys.platform == 'darwin':
			if run('hdiutil', 'detach', cls.ramdisk)[0] != 0:
				raise Exception('Failed to unmount the RAM disk!')

		elif sys.platform == 'linux2':
			shutil.rmtree(cls.tmp_stor, True)

		if len(glob.glob(os.path.join(cls.run_dir, '*'))) == 0:
			os.rmdir(cls.run_dir)



	def setUp(self):
		map(lambda d: shutil.rmtree(d, True), glob.glob(os.path.join(EmuTest.tmp_stor, '*')))



	def runSuite(self, suite):
		os.environ['PYTHONPATH'] = heredir
		suite_run_dir = os.path.join(EmuTest.run_dir, suite)
		os.mkdir(suite_run_dir)

		res = subprocess.call(
			[
				'python',
				os.path.join(heredir, 'test_02emu', 'testgen', 'driver.py'),
				'-t', EmuTest.tmp_stor,
				'-f', suite_run_dir,
				'-s', '10',
				'-l', '1',
				'-i', suite
			]
		)

		self.assertEqual(res, 0, 'The suite %s failed with the code %d' % (suite, res))

		shutil.rmtree(suite_run_dir)



	def testEmuGP64(self):
		self.runSuite('gp')



	def testEmuDR64(self):
		self.runSuite('dr')



	def testEmuSSEControl64(self):
		self.runSuite('ssecontrol')




if __name__ == "__main__":
	unittest.main()
