#!/usr/bin/env python3

import select, sys

while True:
    res = select.select([sys.stdin], [], [], 0)
    if res[0]:
        s = sys.stdin.buffer.raw.read(8)
        if len(s) > 0:
            sys.stdout.buffer.raw.write(s)
            sys.stdout.buffer.raw.write(','.encode('ascii'))
        else:
            exit
