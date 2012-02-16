#!/usr/bin/env python

#
# Script for generating a really big mock log file for testing
#

import datetime

date = datetime.datetime(2012, 2, 5)

for n in range(3000):
	print date, "[MESG]", "Hello world %d" % n
	date += datetime.timedelta(0, 10*60)

