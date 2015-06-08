#!/usr/bin/env python2

import sys

filename = sys.argv[1]
max_len = int(sys.argv[2])

sys.stdout.write('char *words[] = { ')

for line in open(filename, 'r').readlines():
    line = line[:-1]
    if not (any(not ('a' <= c <= 'z') for c in line) or
            (len(line) < 3) or
            (len(line) > max_len)):
        sys.stdout.write('"' + line + '", ')

print '};'
