#!/bin/bash

for a in cch lfch
do
  #for c in 1 8 16 24 32 40 48 56 64 72
  for c in 1 #5 6 7 8 9 10 11 12 13 14 15 # 8 16 24 32 40 48 56 64 72
  do
    ./benchmark -a $a -n $c
  done
done

