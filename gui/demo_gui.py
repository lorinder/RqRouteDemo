#!/usr/bin/env python3

import math
import os
import subprocess
import time

from tkinter import *
from tkinter import ttk

from load_monitor import LoadMonitor

def _terminate_subprocess_list(sb_list):
    # send SIGTERM
    for proc in sb_list:
        proc.terminate()

    # If it didn't work, send SIGKILL
    for proc in sb_list:
        try:
            proc.wait(0.1)
        except:
            proc.kill()

class main_window(object):

    def __init__(self):
        self.root = Tk()
        self.root.title("Video Streaming using RaptorQ - Implemented with CodornicesRq")

        main_frame = ttk.Frame(self.root)
        main_frame.grid(column=0, row=0, sticky=(N, W, E, S))

        self.video_frame = Frame(main_frame,
                                width=640,
                                height=400)
        self.video_frame.grid(column=0, row=0, sticky=(N, W, E, S))

        # Generate the UI elements
        ui_frame = ttk.Frame(main_frame)
        ui_frame.grid(column=0, row=1, sticky=(N, W, E, S))

        co = 0
        loss_prob_label = ttk.Label(ui_frame,
                                padding=(3,3),
                                anchor=W,
                                text="Packet loss probability")
        loss_prob_label.grid(column=co, row=0, sticky=(E))
        self.tv_loss_prob = StringVar()
        self.tv_loss_prob.set("0%")
        loss_prob_vallabel = ttk.Label(ui_frame,
                                padding=(3,3),
                                width=7,
                                anchor=E,
                                textvariable=self.tv_loss_prob)
        loss_prob_vallabel.grid(column=co + 1, row=0)
        self.loss_prob_scale = ttk.Scale(ui_frame,
                                orient=HORIZONTAL,
                                length=200, from_=0, to=1,
                                command=self.set_loss_prob)
        self.loss_prob_scale.grid(column=co + 2, row=0)
        pad_frame = ttk.Frame(ui_frame,
                                padding=(3,3),
                                width=20,
                                height=1)
        pad_frame.grid(column=co + 3, row=0)

        co = 4
        self.redundancy_label = ttk.Label(ui_frame,
                                padding=(3,3),
                                anchor=W,
                                text="Repair fraction")
        self.redundancy_label.grid(column=co, row=0, sticky=(E))
        self.redundancy_label.state(['disabled'])
        self.tv_redundancy = StringVar()
        self.tv_redundancy.set("0%")
        self.redundancy_vallabel = ttk.Label(ui_frame,
                                padding=(3,3),
                                width=7,
                                anchor=E,
                                textvariable=self.tv_redundancy)
        self.redundancy_vallabel.grid(column=co + 1, row=0)
        self.redundancy_vallabel.state(['disabled'])
        self.redundancy_scale = ttk.Scale(ui_frame,
                                orient=HORIZONTAL,
                                length=200, from_=0, to=1,
                                command=self.set_redundancy)
        self.redundancy_scale.grid(column=co + 2, row=0)
        self.redundancy_scale.state(['disabled'])

        self.redundancy_scale_val_if_on = self._real2scale(0.25)

        co = 7
        self.onoff_state = IntVar()
        onoff_button = ttk.Checkbutton(ui_frame,
                                text = 'Enable RaptorQ',
                                padding=(3,3),
                                variable = self.onoff_state,
                                onvalue=1,
                                offvalue=0,
                                command=self.on_onoff_changed)
        onoff_button.grid(column=co, row=0)

        self.tv_onoff_state_label = StringVar(value="[ No protection ]")
        self.onoff_state_label = ttk.Label(ui_frame,
                                padding=(3,3),
                                anchor=E,
                                textvariable=self.tv_onoff_state_label,
                                foreground='red')
        self.onoff_state_label.grid(column=co+1, row=0)

        co = 9
        self.tv_cpu_load = StringVar()
        self.cpu_load_label = ttk.Label(ui_frame,
                                padding=(3,3),
                                anchor=E,
                                textvariable=self.tv_cpu_load)
        self.cpu_load_label.grid(column=co, row=0)
        self.recv_loadmon = LoadMonitor(5)
        self.sender_loadmon = LoadMonitor(5)
        self.root.after(1000, self.update_cpu_load)

        co = 10
        padding = ttk.Frame(ui_frame)
        padding.grid(column=co, row=0)
        ui_frame.columnconfigure(co, weight=1)

        restart_button = ttk.Button(ui_frame,
                                padding=(3,3),
                                text="Restart",
                                command=self.on_restart)
        restart_button.grid(column=co + 1, row = 0,
                                sticky=(N, W, E, S))

        # Add padding around all elements
        for child in ui_frame.winfo_children():
            child.grid_configure(padx=3, pady=3)

        # Sizegrip on lower right hand side.
        ttk.Sizegrip(ui_frame).grid(column=999,
                                row=0,
                                sticky=(E, S))

        # Resize scaling settings
        self.root.columnconfigure(0, weight=1)
        self.root.rowconfigure(0, weight=1)
        main_frame.columnconfigure(0, weight=1)
        main_frame.rowconfigure(0, weight=1)

        # Processes
        self.mpv_proc = None
        self.recv_proc = None
        self.sender_proc = None

        self.root.protocol("WM_DELETE_WINDOW", self.on_close)

    def _scale2real(self, scale_val):
        return 0.5 * float(scale_val)**2

    def _real2scale(self, rval):
        return str(math.sqrt(2 * rval))

    def set_loss_prob(self, scale_val):
        p = self._scale2real(scale_val)
        self.tv_loss_prob.set("%.2g%%" % (p * 100.))
        self.sender_proc.stdin.write("drop-prob %g\n" % (p,))
        self.sender_proc.stdin.flush()

    def set_redundancy(self, scale_val):
        r = self._scale2real(scale_val)
        self.tv_redundancy.set("%.2g%%" % (r * 100.))
        self.sender_proc.stdin.write("overhead %g\n" % (r/(1.-r),))
        self.sender_proc.stdin.flush()

    def update_cpu_load(self):
        if self.recv_proc:
            self.recv_loadmon.add_data_point(self.recv_proc.pid)
        if self.sender_proc:
            self.sender_loadmon.add_data_point(self.sender_proc.pid)
        def get_load_str(loadmon):
            v = loadmon.get_load_avg()
            if v is None:
                return "N/A"
            return "%.2g%%" % (v * 100,)
        self.tv_cpu_load.set(
          "CPU load:  sender: %s  receiver: %s" \
          % (get_load_str(self.sender_loadmon),
             get_load_str(self.recv_loadmon)))
        self.root.after(1000, self.update_cpu_load)

    def on_restart(self):
        self.terminate_subprocesses() 
        self.start_subprocesses()

        # set the correct probabilities
        self.set_loss_prob(self.loss_prob_scale['value'])
        self.set_redundancy(self.redundancy_scale['value'])

    def on_onoff_changed(self):
        if self.onoff_state.get() == 0:
            # Save redundancy value for later use
            self.redundancy_scale_val_if_on \
              = self.redundancy_scale['value']

            # Set redundancy to 0 and disable sliders
            zero = str(0)
            self.redundancy_scale.state(['disabled'])
            self.redundancy_label.state(['disabled'])
            self.redundancy_vallabel.state(['disabled'])
            self.redundancy_scale['value'] = zero
            self.set_redundancy(zero)

            # Update state_label
            self.tv_onoff_state_label.set('[ No protection ]')
            self.onoff_state_label['foreground'] = 'red'
        else:
            # Enable slider
            self.redundancy_scale.state(['!disabled'])
            self.redundancy_label.state(['!disabled'])
            self.redundancy_vallabel.state(['!disabled'])

            # Set redundancy slider value
            self.redundancy_scale['value'] \
              = self.redundancy_scale_val_if_on
            self.set_redundancy(self.redundancy_scale_val_if_on)

            # Update state_label
            self.tv_onoff_state_label.set('[ RaptorQ protection ]')
            self.onoff_state_label['foreground'] = 'green'

    def on_close(self):
        self.terminate_subprocesses()

        # close the application
        try:
            self.root.destroy()
        except:
            pass

    def gui_loop(self):
        self.root.mainloop()

    def start_subprocesses(self):
        # Open a pipe so the receiver process can signal
        # when it's operational.
        ready_read_fd, ready_write_fd = os.pipe()
        os.set_inheritable(ready_write_fd, True)

        self.recv_proc = subprocess.Popen(
                            ['./build/receiver/receiver',
                             '-r', '%d' % (ready_write_fd,) ],
                            cwd='..',
                            pass_fds=(ready_write_fd,),
                            stdout=subprocess.DEVNULL,
                            stderr=subprocess.DEVNULL)
        self.mpv_proc = subprocess.Popen(
                            ['mpv', '--no-input-default-bindings',
                            '--profile=low-latency', '-ao', 'null',
                            '--osc=no', '--playlist',
                            '../seg_recv/dummy_playlist.m3u8',
                            '--wid=%r' % self.video_frame.winfo_id()],
                            stdout=subprocess.DEVNULL,
                            stderr=subprocess.DEVNULL)

        # Wait for the receiver process to be ready before
        # starting the sender process.
        x = os.read(ready_read_fd, 1)
        os.close(ready_read_fd)

        # Start the sender process
        self.sender_proc = subprocess.Popen(['./build/sender/sender'],
                            cwd='..',
                            stdin=subprocess.PIPE,
                            stdout=subprocess.DEVNULL,
                            stderr=subprocess.DEVNULL,
                            universal_newlines=True)

    def terminate_subprocesses(self):
        _terminate_subprocess_list([self.mpv_proc,
                            self.recv_proc,
                            self.sender_proc])
        self.mpv_proc, self.recv_proc, self.sender_proc = (None, None, None)
        self.sender_loadmon.reset()
        self.recv_loadmon.reset()

M = main_window()
M.start_subprocesses()
M.gui_loop()

# vim:ts=4:sw=4:et
