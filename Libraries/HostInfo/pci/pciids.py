#!/usr/bin/env python

import os
import sys
import re
import string

vendor_re = re.compile (r'^(?P<vendorid>[%s]{4})\s+(?P<vendorname>.*\S)\s*$' % (string.hexdigits))
device_re = re.compile (r'^\t(?P<deviceid>[%s]{4})\s+(?P<devicename>.*\S)\s*$' % (string.hexdigits))
subdevice_re = re.compile (r'^\t\t.*$' )

def main():

	try:
		fd = open(sys.argv[1])
		out = open(sys.argv[2], "w+")
	except:
		print "Usage: %s <input ids file> <output header file>" % sys.argv[0]
		return

	devices = []
	vendors = {}
	vname = vid = None
	for line in fd.readlines():
		if vendor_re.match(line):
			i = vendor_re.match(line)
			vid = i.group ('vendorid').lower()
			vname = i.group ('vendorname').strip()
			vendors[int(vid,16)] = vname
		elif device_re.match(line):
			i = device_re.match(line)
			did = i.group ('deviceid').lower()
			dname = i.group ('devicename').strip()
			dname = dname.replace("\"", "\\\"")
			dname = dname.replace("?", "\\?")
			nvid = int(vid,16)
			ndid = int(did,16)
			devices.append( [nvid, ndid, vname, dname, (nvid << 16) + ndid] )
		elif line.find('List of known device classes') != -1:
			break # list of device ids completed
		elif line.strip().startswith('#') or line.strip() == '':
			pass # skipping comments
		elif subdevice_re.match(line):
			pass # skipping subdevices in our filter
		else:
			print '(!) Unparsed line: %s\n' % (line)

	print >> out, \
"""/**
 * This file is AUTOGENERATER during Gen.py execution.
 */

#ifndef __AUTOGEN_PCI_IDS_H__
#define __AUTOGEN_PCI_IDS_H__

struct _prl_pci_ids {
	unsigned int vid;
	unsigned int did;
	const char *name;
} pci_ids[] = {
"""

	devices.sort( lambda x, y: cmp( x[4], y[4] ) )
	for dev in devices:
		print >> out, "\t{0x%.4x, 0x%.4x, \"%s %s\"}," % (
			dev[0], dev[1], dev[2], dev[3])

	print >> out, \
"""\t{0, 0, NULL}
};


struct _prl_pci_ven_ids {
	unsigned int vid;
	const char *name;
} pci_ven_ids[] = {
"""

	vendors_list = []
	for ven in vendors:
		vendors_list.append( [ven, vendors[ven]] )
	vendors_list.sort( lambda x, y: cmp(x[0],y[0]) )

	for ven in vendors_list:
		print >> out, "\t{0x%x, \"%s\"}," % (ven[0], ven[1])

	print >> out, \
"""\t{0, NULL}
};

#endif /* __AUTOGEN_PCI_IDS_H__ */
"""

	fd.close()
	out.close()

if __name__ == "__main__":
	main()
