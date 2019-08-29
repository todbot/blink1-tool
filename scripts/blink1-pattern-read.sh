#!/bin/sh
#
# Demonstrate how to dump all the color pattern in a blink(1)
# 
# this probably requires a bash-alike
#

#n=32  # mk2: 32 (RAM) or 16 (flash) 
#n=16  # mk2: 32 (RAM) or 16 (flash) 
n=12  # mk1: 12

for i in `seq 1 $n` ; do
  ./blink1-tool --getpattline $i
done

