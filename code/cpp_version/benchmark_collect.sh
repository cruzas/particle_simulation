#!/bin/bash
# Bash script used to collect data on performance of particle simulation.
# Author: Samuel A. Cruz Alegría

declare -a nParticles=(10 100 1000 10000)

filename_serial="benchmark_serial.txt"

# Execute code without using OpenACC.
for i in "${nParticles[@]}"
do
  echo "nParticles="$i >> $filename_serial
  # TODO: fix so that avg_duration is added to line that says nParticles=...
  ./particles_serial n=$i delta=0.5 total_time_interval=10 >> $filename_serial

  # mpirun -np $rank ./main $i $i 100 0.01 > results/res_r_"$rank"_n_"$num_threads"_"$i".txt
done

# TODO: Execute code using OpenACC.

echo All done
