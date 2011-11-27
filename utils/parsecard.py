#!/usr/bin/env python

import sys

#    25     0x0B P          DATA               0x0F P LRC
# 000...000 1101 0 ddd0p ddd0p ddd0p ... ddd0p 1111 1 ccccc 00000 00000 ... 00000

# DATA - groups of 5-bits (3 data, pad, parity)
# ddd0p ddd0p ddd0p ... ddd0p

# DATA - groups of 3-bits, bits reversed
# ddd ddd ddd ... ddd

# DATA - keep last 26 bits
# dd ddd ddd ddd ddd ddd ddd ddd ddd

# DATA - first and last parity, 26 bits
# pd ddd ddd ddd ddd ddd ddd ddd ddp

# DATA - remove parity bits, 24 bits
# d ddd ddd ddd ddd ddd ddd ddd dd

# DATA - groups of 4 bits
# dddd dddd dddd dddd dddd dddd

# Facility# Card#
# dddd dddd dddd dddd dddd dddd

path = sys.argv[1]

fd = file(path, "r")
lines = fd.readlines()
fd.close()

header = lines[0]

ret = ""
for line in lines[1:]:
	line = line.strip()
	(tm, data, clock, present) = line.split(",")
	clock = int(clock)
	data = 1-int(data)
	if (clock == 0):
		ret += str(data)



leading = ret[0:25]
ret = ret[25:]

print ret

#print "leading", leading
#print "rest", ret
#print ""

# Break up the bits into 5-bit segments
segments = []
n = 0
while n < len(ret):
	segment = ret[n:n+5]
	segments.append(segment)
	n += 5

# Strip off the trailing groups of zero
while segments[-1] == "00000":
	del segments[-1]

# Remove the B segment
del segments[0]
# Remove the LRC
del segments[-1]
# Remove the F segment
del segments[-1]

# Transform the segments into 3-bit chunks, with the bits reversed
a = ""
for segment in segments:
	part = segment[0:3]
	a += part[2]+part[1]+part[0]

# Keep the last 26 bits and discard the rest
a = a[-26:]
# Discard first and last parity bits
a = a[1:-1]

print a, len(a)
n = 0
lst = []
while n < len(a):
	lst.append(a[n:n+4])
	n += 4

# Extract out the facility and card codes
facility = lst[0]+lst[1]
card = lst[2]+lst[3]+lst[4]+lst[5]

print int(facility, 2)
print int(card, 2)
