RaptorQ Demo Software
=====================

Required software:
	* C compiler toolchain, inclusive cmake
	* CodornicesRq or TvRQ installed
	NOTE: TvRq is intended for compliance, but not to be fast. 
	It can be found here: https://github.com/lorinder/TvRQ
	CodornicesRq is both compliant and fast. 
	More information is available here: https://www.codornices.info/
	* python 3 with tkinter
	* mpv video player
	* ffmpeg
	* tears of steel video 1080p h.264, see below.

Steps to prepare:

1)  First, the video needs to be downloaded and segmented:

    download:

        $ cd <demo_software_root>
	$ cd segmenter
	$ mkdir original
	$ curl -o original/tears_of_steel_1080p.mov \
	  http://ftp.nluug.nl/pub/graphics/blender/demo/movies/ToS/tears_of_steel_1080p.mov

    segment:
        $ ./segment.sh

    If all goes well, this should create a directory segm_out/ in
    segmenter/ which contains a list.m3u8 file, a dummy_playlist.m3u8 and a
    bunch of seg-NNN.mkv files (the video segments).

    Link the segments from the toplevel checkout dir.

       $ cd ..
       $ ln -s segmenter/segm_out segments

2) Build the software.  This builds the sender and receiver binaries.

       $ mkdir build
       $ cd build

  By default the demo software uses the CodornicesRq libraries to integrate
  RaptorQ.

       $ cmake ..

  If you would like to build with TvRQ libraries instead, use the following
  command instead of the one mentioned above:

       $ cmake -D CMAKE_RQ_API=TVRQ ..

  * NOTE: TvRQ is a rudimentary implementation of RaptorQ and not optimized
  for performance. When used with the demo, it does not provide a smooth
  experience.

  Then,

       $ cmake --build .
       $ cd ..

3) Create a seg_recv directory which contains the FIFOs seg-NNN.mkv
   Those are FIFOs, not regular files.  They're used to hook up the
   receiver to the video player.

       $ ./receiver/mk_seg_recv.sh

   Check that the seg_recv directory exists and contains the FIFOs plus
   the dummy playlist as expected.

4) Increase the receive buffer size for all types of connections. Ensure you
   have root priveleges or sudo access.

       $ sudo sysctl -w net.core.rmem_max=4448256
       $ sudo sysctl -w net.core.rmem_default=4448256

5) Now we're ready to launch the demo app.

       $ cd gui
       $ ./demo_gui.py

   This should open a window with the video playing and two sliders on
   the bottom which can be used to set packet loss probability and
   redundancy.

   With all the setup done in steps 1-4 above, the demo can be launched
   again indefinitely with step 5.

6) The demo, by default runs the receiver and sender on the same machine. In
   order to run the sender and receiver on different machines, do the following
   after completing step 4 on both receiver and sender machines.

   On the receiver machine, open a terminal:

       $ cd <demo_software_root>
       $ mpv --no-input-default-bindings --profile=low-latency -ao null --osc=no --playlist ./seg_recv/dummy_playlist.m3u8

   On the receiver machine, open another terminal:

       $ cd <demo_software_root>
       $ ./build/receiver/receiver

   The receiver is now ready to receive the segments and the player is ready to
   play them out.

   On the sender machine, open a terminal:

       $ cd <demo_software_root>
       $ ./build/sender/sender -r <receiver_ip>

Comments
--------

In cases where the demo crashes unexpectedly, e.g. due to a programming
bug, it can happen that not all its processes exit cleanly; instead they
may linger around.  Check for them as follows:

	$ pgrep receiver
	$ pgrep mpv
	$ pgrep sender

And kill any surviving such processes before starting the demo again,
e.g., using pkill.
