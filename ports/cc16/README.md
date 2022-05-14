The MRS CC16 port
=================

This port is a version of the bare-arm port to the MRS CC16 (a CAN controller
based on the NXP S32K144). It could be considered an S32K144 port except for
the need to accommodate the CC16's bootloader, and the lack of external
serial ports.

To build, simply run `make` in this directory.  Deploying is an exercise for
the reader, but the MRS flasher can be used, or the open-source flasher at
https://github.com/John-Titor/MRS_tools. 

Console output is delivered on CAN1, ID 0x1ffffffe, with newlines included.
