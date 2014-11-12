#!/bin/bash

# debug option
if [[ $# -eq 1 && $1 == "-d" ]]; then
  # clean compilation
  make clean; make
  gdb --args ./3600dns @129.10.112.152 joshua.ccs.neu.edu
elif [[ $# -eq 1 && $1 == "-s" ]]; then
	/course/cs3600f14/code/solutions/project3/3600dns @129.10.112.152 joshua.ccs.neu.edu
else
  # clean compilation
  make clean; make
	./3600dns @129.10.112.152 joshua.ccs.neu.edu
fi
