#!/usr/bin/env python3

import os
import time

class LoadMonitor(object):
    def __init__(self, win_sz):
        self.win_sz = win_sz
        self.data = []

    def reset(self):
        self.data = []
    
    def add_data_point(self, pid):
        try:
            fp = open("/proc/%d/stat" % (pid,), "r")
            l = fp.read().split()
            fp.close()
        except FileNotFoundError:
            return False

        clk_tck = os.sysconf('SC_CLK_TCK')

        utime = int(l[13]) / clk_tck
        stime = int(l[14]) / clk_tck

        tup = (utime + stime, time.time())
        self.data.append(tup)

        # Remove old data if it's out of the window
        if len(self.data) > 2:
            if self.data[-1][1] - self.data[1][1] >= self.win_sz:
                self.data = self.data[1:]

        return True

    def get_load_avg(self):
        if len(self.data) < 2:
            return None
        delta_rt = self.data[-1][1] - self.data[0][1]
        delta_pt = self.data[-1][0] - self.data[0][0]
        return delta_pt / delta_rt

def _usage():
    print("Simple CPU load monitor tool")
    print("")
    print("  -h         display this help and exit")
    print("")
    print("  -p #       set the pid of the process to monitor")
    print("  -w #       set the approximate averaging window in seconds")

if __name__ == "__main__":
    import getopt
    import sys

    # Defaults
    win_sz = 5
    pid = None

    # Get args
    opts, args = getopt.getopt(sys.argv[1:], "hp:w:")
    for (o, v) in opts:
        if o == '-h':
            _usage()
            sys.exit(0)
        elif o == '-p':
            pid = int(v)
        elif o == '-w':
            win_sz = int(v)

    # Check
    if pid is None:
        sys.stderr.write("Error:  PID (-p) not specified.\n")
        sys.exit(1)

    # Run
    M = LoadMonitor(win_sz)
    while True:
        M.add_data_point(pid)
        lavg = M.get_load_avg()
        if lavg is not None:
            print(lavg)
        time.sleep(1)

# vim:ts=4:sw=4:et
