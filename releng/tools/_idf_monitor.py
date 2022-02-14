#!/usr/bin/env python3

# This module is just a thin wrapper around idf_monitor.py. It's goal is the
# only one - monkeypath the original implementation so it uses rmake instead of
# make

import importlib.util
import subprocess
import os

idf_monitor_path = os.path.join(os.environ["IDF_PATH"], "tools", "idf_monitor.py")
spec = importlib.util.spec_from_file_location("idf_monitor", idf_monitor_path)
idf_monitor = importlib.util.module_from_spec(spec)
spec.loader.exec_module(idf_monitor)

# Original implementation in the case it is needed
_run_make = idf_monitor.Monitor.run_make

def alternative_run_make(self, target):
    with self:
        target = target.replace("encrypted-", "")
        assert target in ["flash", "app-flash"]

        img = os.environ["ROFI_MONITORED_TARGET"]
        imgTarget = os.path.splitext(img)[0]
        toolsPath = os.path.join(os.environ["ROFI_ROOT"], "releng", "tools")

        subprocess.check_call(["python", os.path.join(toolsPath, "rmake"), imgTarget])
        subprocess.check_call(["python", os.path.join(toolsPath, "rflash"), img])


idf_monitor.Monitor.run_make = alternative_run_make

if __name__ == "__main__":
    idf_monitor.main()
