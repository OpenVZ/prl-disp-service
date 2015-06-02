#! /usr/bin/env python

import os
import sys
import subprocess

def execWithCapture(command, argv):
    """ Run command using subprocess and return output """

    try:
        pipe = subprocess.Popen([command] + argv,
                                stdout=subprocess.PIPE,
                                stderr=subprocess.STDOUT)
    except OSError, (errno, msg):
        raise RuntimeError, "Error running " + command + ": " + msg

    rc = pipe.stdout.read()
    pipe.wait()
    return rc

def isApple():
    """ Detect is it Apple harwdare using ACPI tables information """

    acpi_tool_path = '/usr/sbin/acpi_hdr_dump'
    if not os.path.isfile(acpi_tool_path):
        # This is not PSBM distribution
        sys.exit(1)
    buf = execWithCapture(acpi_tool_path, ["/proc/acpi/dsdt"])
    if buf.lower().find("apple") != -1:
        return True
    return False

def macHwType():
    """ Detect the type of Apple hardware using DMIBIOS information"""

    buf = execWithCapture("/usr/sbin/dmidecode",
                              ["-s", "system-product-name"])
    if not buf:
        return 'Xserve3,1'
    return buf

def main():
    hwtype = 'PC'
    if isApple():
        hwtype = macHwType()
    print hwtype

if __name__ == "__main__":
    main()
