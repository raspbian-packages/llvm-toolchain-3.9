"""
Test denied process attach.
"""

from __future__ import print_function



import os
import time
import lldb
from lldbsuite.test.decorators import *
from lldbsuite.test.lldbtest import *
from lldbsuite.test import lldbutil

exe_name = 'AttachDenied'  # Must match Makefile

class AttachDeniedTestCase(TestBase):

    mydir = TestBase.compute_mydir(__file__)

    @skipIfWindows
    @skipIfiOSSimulator
    def test_attach_to_process_by_id_denied(self):
        """Test attach by process id denied"""
        self.build()
        exe = os.path.join(os.getcwd(), exe_name)

        # Use a file as a synchronization point between test and inferior.
        pid_file_path = lldbutil.append_to_process_working_directory(
                "pid_file_%d" % (int(time.time())))
        self.addTearDownHook(lambda: self.run_platform_command("rm %s" % (pid_file_path)))

        # Spawn a new process
        popen = self.spawnSubprocess(exe, [pid_file_path])
        self.addTearDownHook(self.cleanupSubprocesses)

        pid = lldbutil.wait_for_file_on_target(self, pid_file_path)

        self.expect('process attach -p ' + pid,
                    startstr = 'error: attach failed:',
                    error = True)
