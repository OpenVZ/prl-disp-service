"""
Wrapper for VM applicatoin with javascript interpreter
"""

import subprocess, os, signal
from logging import debug, error

__all__ = ["VmApp"]

class VmApp:
	def __init__(self, vmcfg):
		cmd = ["prl_vm_app",
				"--mode", "pwe",
				"--openvm", os.path.abspath(vmcfg),
				]

		nullfd = os.open(os.devnull, os.O_WRONLY)

		debug("starting %r", cmd)
		# discard std{out,err} from vm_app and keep our output clean;
		# those messages are logged to parallels.log by vm_app
		self.p = subprocess.Popen(cmd, stdin = subprocess.PIPE,
				stdout = nullfd, stderr = subprocess.STDOUT)
		debug("started pid %d", self.p.pid)

		os.close(nullfd)

	def __del__(self):
		if self.p:
			self.p.kill()

	def write(self, s):
		"""

		"""
		debug("vm_app >>>\n%s<<<", s)
		self.p.stdin.write(s)
		self.p.stdin.flush()

	def wait(self, timeout = None):
		if timeout is not None:
			def timeout_handler(signum, frame):
				error("pid %d didn't exit in %d seconds", self.p.pid, timeout)
				self.p.kill()

			signal.signal(signal.SIGALRM, timeout_handler)
			signal.alarm(timeout)

		self.p.stdin.close()
		ret = self.p.wait()
		debug("pid %d exited with code %d", self.p.pid, ret)
		self.p = None

		if timeout is not None:
			signal.alarm(0)

		return ret

	def run(self, timeout = None):
		self.write("vm.VMStart()\n")
		return self.wait(timeout)
