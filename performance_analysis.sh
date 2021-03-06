thread_nums=(1 2 4 6 8)
field_sizes=(1024 2048 4096)
time_steps=1

# write csv header
echo time_steps, fieldsize, threads, nx, ny, predicted memory, actual memory, time > timing.csv

for fieldsize in "${field_sizes[@]}"; do
  for thread_num in "${thread_nums[@]}"; do
    for run in {1..5}; do
      # if possible vary nx and ny so its not always 1 x num_threads
      nx=$(($thread_num / $run))
      if (( $nx > 1 )) && (($nx * $run == $thread_num)); then
        ny=$run
      else
        nx=1
        ny=$thread_num
      fi
      echo "$nx x $ny: $(($nx * $ny))"

      # 4 * int + 1 * long + 2 * width * height * double + num_threads * int * 9 + 2000
      predicted_memory=$((4 * 4 + 1 * 8 + 2 * $thread_num * $fieldsize * 8 + $thread_num * 4 * 9 + 2000))

      printf "$time_steps, $fieldsize, $thread_num, $nx, $ny, $predicted_memory, " >> timing.csv
      /usr/bin/time -f "%M, %U" ./gameoflife $time_steps $nx $ny $fieldsize $fieldsize >> timing.csv  2>&1

      sleep 0.2
    done
  done
done
