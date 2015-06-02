import os

def pr(s):
	print ">>>>>>>>|" + s + "|<<<<<<<<"

pr("CPUs: %d" % os.sysconf("SC_NPROCESSORS_ONLN"))
pr("RAM: %d" % (os.sysconf("SC_PHYS_PAGES") * os.sysconf("SC_PAGE_SIZE")))
