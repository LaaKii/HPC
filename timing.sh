allCores=(1 2 3 4 5 6 7 8)
allFieldsizes=(1024 2048 4096)
timestep=2
constantCoreFactor=1

for fieldsize in "${allFieldsizes[@]}"; do
  #printf "Fieldsize gets increased to %d\n" "$fieldsize" >> timing.txt
  echo "Fieldsize gets increased to $fieldsize"
  for core in "${allCores[@]}"; do
    #printf "Running field of %d x %d with %d core(s) \n" "$fieldsize" "$fieldsize" "$core" >> timing.txt
    echo "Running field of $fieldsize x $fieldsize with $core core(s)"
    for run in 1 2 3 4 5; do
      #printf "Run: $run\n" >> timing.txt
      echo "Run: $run"
      time ./gameoflife $timestep $constantCoreFactor $core $fieldsize $fieldsize
    done
  done
done
