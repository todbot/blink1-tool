#!/bin/sh
#
# Demonstrate how to fill the blink(1) with a particular color pattern
#
#

echo "writing pattern, 32 positions"
blink1-tool -g -m 550 --rgb 8,9,10      --setpattline 0
blink1-tool -g -m 550 --rgb 11,12,13    --setpattline 1
blink1-tool -g -m 550 --rgb 21,22,23    --setpattline 2
blink1-tool -g -m 550 --rgb 31,32,33    --setpattline 3
blink1-tool -g -m 550 --rgb 41,42,43    --setpattline 4
blink1-tool -g -m 550 --rgb 51,52,53    --setpattline 5
blink1-tool -g -m 550 --rgb 61,62,63    --setpattline 6
blink1-tool -g -m 550 --rgb 71,72,73    --setpattline 7
blink1-tool -g -m 550 --rgb 81,82,83    --setpattline 8
blink1-tool -g -m 550 --rgb 91,92,93    --setpattline 9 
blink1-tool -g -m 550 --rgb 101,102,103 --setpattline 10 
blink1-tool -g -m 550 --rgb 111,112,113 --setpattline 11
blink1-tool -g -m 550 --rgb 121,122,123 --setpattline 12
blink1-tool -g -m 550 --rgb 131,132,133 --setpattline 13
blink1-tool -g -m 550 --rgb 141,142,143 --setpattline 14
blink1-tool -g -m 550 --rgb 151,152,153 --setpattline 15
blink1-tool -g -m 550 --rgb 161,162,163 --setpattline 16
blink1-tool -g -m 550 --rgb 171,172,173 --setpattline 17
blink1-tool -g -m 550 --rgb 181,183,183 --setpattline 18
blink1-tool -g -m 550 --rgb 191,192,193 --setpattline 19
blink1-tool -g -m 550 --rgb 201,202,203 --setpattline 20
blink1-tool -g -m 550 --rgb 211,212,213 --setpattline 21
blink1-tool -g -m 550 --rgb 221,222,223 --setpattline 22
blink1-tool -g -m 550 --rgb 231,232,233 --setpattline 23
blink1-tool -g -m 550 --rgb 241,242,243 --setpattline 24
blink1-tool -g -m 550 --rgb 251,252,253 --setpattline 25
blink1-tool -g -m 550 --rgb 123,132,231 --setpattline 26
blink1-tool -g -m 550 --rgb 111,222,166 --setpattline 27
blink1-tool -g -m 550 --rgb 33,66,99    --setpattline 28
blink1-tool -g -m 550 --rgb 55,44,33    --setpattline 29
blink1-tool -g -m 550 --rgb 22,33,44    --setpattline 30
blink1-tool -g -m 550 --rgb 8,6,7       --setpattline 31
blink1-tool --savepattern  

sleep 1

echo "reading back stored pattern"

#n=32  # mk2: 32 (RAM) or 16 (flash) 
n=32  # mk2: 32 (RAM) or 16 (flash) 
#n=12  # mk1: 12
for i in `seq 1 31` ; do
  blink1-tool --getpattline $i
done

echo "playing pattern"
blink1-tool --play 1

