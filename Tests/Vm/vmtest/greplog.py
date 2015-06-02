"""
Incremental grep of a growing log file for the expected expression
"""
import time
from logging import debug

__all__ = ["GrepLog"]

class GrepLog:
	def __init__(self, fname, create = True):
		if create:
			open(fname, "a").close()
		self.fp = open(fname, "rU")
		self.fp.seek(0, 2)

	def close(self):
		self.fp.close()

	def grep(self, matcher):
		debug("grep %r for %s", self.fp.name, matcher)
		while True:
			pos = self.fp.tell()
			l = self.fp.readline()
			if not l:
				return None
			# if the line is incomplete (we raced with the writer)
			# rewind to the beinning of it otherwise on the
			# following calls we'll match against a split line
			if not l.endswith("\n"):
				self.fp.seek(pos, 0)
				return None

			ret = matcher(l)
			if ret:
				debug("found %r", ret)
				return ret

	def greploop(self, matcher, timeout, interval = 3):
		time_end = time.time() + timeout
		while True:
			ret = self.grep(matcher)
			if ret:
				return ret
			if time.time() > time_end:
				return None

			debug("waiting for %s to appear in %r", matcher,
					self.fp.name)
			time.sleep(interval)
