The MRS CC16 port
=================

This port is a version of the bare-arm port to the MRS CC16 (a CAN controller
based on the NXP S32K144). It could be considered an S32K144 port except for
the need to accommodate the CC16's bootloader, and the lack of external
serial ports.

To build, simply run `make` in this directory.  Deploying is an exercise for
the reader. Console output is delivered on CAN1, ID 0x1ffffffe, with newlines
included.

There are some simple demonstration code strings (see `main.c`) which are
compiled and executed when the firmware starts.  They produce output on the
system's stdout.
