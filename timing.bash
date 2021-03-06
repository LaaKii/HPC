#!/bin/bash
allCores=(1 2 3 4 5 6 7 8)
allFieldsizes=(1024 2048 4096)
timestep=2
constantCoreFactor=1

for fieldsize in "${allFieldsizes[@]}"; do
  printf "Fieldsize gets increased to %d x %d\n" "$fieldsize" "$fieldsize">> timing.txt
  for core in "${allCores[@]}"; do
    printf "Running field of %d x %d with %d core(s) \n" "$fieldsize" "$fieldsize" "$core" >> timing.txt
    for (( c=1; c<=5; c++ ))
    do
      echo "run $c" >> timing.txt
      { time ./gameoflife $timestep $constantCoreFactor $core $fieldsize $fieldsize ; } >> timing.txt 2>&1
    done
    echo "-------------------------" >> timing.txt
  done
done
