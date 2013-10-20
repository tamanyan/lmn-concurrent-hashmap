#!/bin/bash

for a in cch
do
  for c in 1 8 16 24 32 40 48 56 64 72
  do
    ./benchmark -a $a -n $c
  done
done

