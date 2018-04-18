#!/bin/bash
# Bash script used to collect data on performance of particle simulation.
# Author: Samuel A. Cruz Alegría

declare -a nParticles=(10 100 1000 10000)

for i in "${nParticles[@]}"
do
  echo "nParticles="$i

  ./particles_serial n=$i delta=0.5 total_time_interval=100

  # mpirun -np $rank ./main $i $i 100 0.01 > results/res_r_"$rank"_n_"$num_threads"_"$i".txt
done

echo All done
