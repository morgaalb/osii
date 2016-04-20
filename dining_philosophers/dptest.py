#!/usr/bin/python

# Checks dp for illegal states.
# Pipe dp's output through this program.
# Usage example:
#    > dp | dptest.py

import sys
from string import strip

NUM_PHILOSOPHERS = 5

fork_count = {}
states = {}

# Read the first '------'
line = sys.stdin.readline()

def sanity_check():
	for k in states.keys():
		if (states[k] == "Eating" and fork_count[k] != 2) or (states[k] == "Thinking" and (fork_count.get(k, None) is not None and fork_count.get(k, None) != 0)):
			print "ERROR: FORK MISMATCH"
			print k, "is", states[k], "but is holding",
			print fork_count.get(k, 0), "forks."
			sys.exit(0)

def reset_state():
	for k in fork_count.keys():
		fork_count[k] = 0

while True:
	line = sys.stdin.readline()
	if line[0] == '-':
		print '-----------'
		sanity_check()
		reset_state()
	else:
		_, col1, col2, _ = map(strip, line.split('|'))
		print(col1, col2)

		if col1 == "Fork":
			try:
				fork_count[col2] += 1
			except KeyError:
				fork_count[col2] = 1
		else:
			states[col1] = col2
			
