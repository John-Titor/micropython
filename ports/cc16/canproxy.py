#!/usr/bin/env python3
"""
CAN proxy, bridges CAN console to stdin/out for use with mpremote

    canproxy --interface <interface-name> --channel <channel> --bitrate <bitrate-kbps>
"""

import can
import select
import sys
import termios


def _recv_callback(msg):
    if (msg.arbitration_id == 0x1ffffffe):
        sys.stdout.buffer.raw.write(msg.data)
        sys.stdout.flush()


def proxy(interface, channel, bitrate):
    bus = can.Bus(interface=interface,
                  channel=channel,
                  bitrate=bitrate * 1000)
    notifier = can.Notifier(bus, [_recv_callback])
    try:
        while True:
            res = select.select([sys.stdin], [], [], 0)
            if res[0]:
                s = sys.stdin.buffer.raw.read(8)
                msg = can.Message(arbitration_id=0x1ffffffd,
                                  is_extended_id=True,
                                  data=s)
                bus.send(msg)

    except KeyboardInterrupt:
        notifier.stop()
#    termios.tcsetattr(sys.stdin.fileno(), termios.TCSANOW, orig_attr)


if __name__ == "__main__":
    import argparse

    parser = argparse.ArgumentParser(description='CAN console proxy')
    parser.add_argument('--interface',
                        type=str,
                        metavar='INTERFACE',
                        required=True,
                        help='name of the interface as known to python-can')
    parser.add_argument('--channel',
                        type=str,
                        metavar='CHANNEL',
                        default='',
                        help='interface channel name (e.g. for Anagate units, hostname:portname')
    parser.add_argument('--bitrate',
                        type=int,
                        default=500,
                        metavar='BITRATE_KBPS',
                        help='CAN bitrate (kBps')

    args = parser.parse_args()
    proxy(args.interface, args.channel, args.bitrate)
