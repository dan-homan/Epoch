#!/usr/bin/env python3
#
# Wrapper to launch an Epoch executable from the run directory.
# Use this as the engine command in cutechess, with the executable
# name as the argument, e.g.:
#
#   python3 run_exchess.py Epoch_v2026_03_01
#
# Ensures Epoch runs with run/ as its working directory so it finds
# search.par, main_bk.dat, etc.  Uses os.execv so Epoch gets direct
# stdin/stdout access with no overhead.
#

import os
import sys

run_dir = os.path.dirname(os.path.abspath(__file__))
os.chdir(run_dir)

exe = sys.argv[1] if len(sys.argv) > 1 else "Epoch"

if not os.path.isabs(exe):
    exe = os.path.join(run_dir, exe)

os.execv(exe, [exe] + sys.argv[2:])
