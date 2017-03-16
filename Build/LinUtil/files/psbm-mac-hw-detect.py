#! /usr/bin/env python
#
# Copyright (c) 2015-2017, Parallels International GmbH
#
# This file is part of Virtuozzo Core. Virtuozzo Core is free
# software; you can redistribute it and/or modify it under the terms
# of the GNU General Public License as published by the Free Software
# Foundation; either version 2 of the License, or (at your option) any
# later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
# 02110-1301, USA.
#
# Our contact details: Parallels International GmbH, Vordergasse 59, 8200
# Schaffhausen, Switzerland.
#

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
