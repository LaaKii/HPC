#!/bin/bash
thread_nums=(1 2 4 6 8)
#thread_nums=(1)
field_sizes=(1024 2048 4096)
#field_sizes=(1024)
time_steps=1

# write csv header
echo time_steps, fieldsize, threads, Px, Py, predicted memory, actual memory, time > results.csv

for fieldsize in "${field_sizes[@]}"; do
  for thread_num in "${thread_nums[@]}"; do
    for run in {1..5}; do
      # if possible vary Px and Py so its not always 1 x num_threads
      Px=$(($thread_num / $run))
      if (( $Px > 1 )) && (($Px * $run == $thread_num)); then
        Py=$run
      else
        Px=1
        Py=$thread_num
      fi
      echo "$Px x $Py: $(($Px * $Py))"

      # 4 * int + 1 * long + 2 * width * height * double + num_threads * int * 9 + 2000
      predicted_memory=$((4 * 4 + 1 * 8 + 2 * $thread_num * $fieldsize * 8 + $thread_num * 4 * 9 + 2000))
#      echo $time_steps $Px $Py $fieldsize $fieldsize
      printf "$time_steps, $fieldsize, $thread_num, $Px, $Py, $predicted_memory, " >> results.csv
      /usr/bin/time -f "%M, %U" ./gameoflife $time_steps $fieldsize $fieldsize $Px $Py >> results.csv  2>&1

      sleep 0.2
    done
  done
done
