#!/usr/bin/env python
"""
Discover and run all available tests.

The script assumes that

  - the relevant components (prl_vm_app, monitor, hypervisor and network
    modules) are built
  - the modules are loaded
  - prl_vm_app is on PATH

The calling user doesn't (and normally shouldn't) be root.

The non-zero exit code indicates that some of the tests failed.
"""

import os, sys, unittest

here = os.path.dirname(__file__)

test = unittest.TestLoader().discover(here)
result = unittest.TextTestRunner(verbosity = 255).run(test)

sys.exit(not result.wasSuccessful())
