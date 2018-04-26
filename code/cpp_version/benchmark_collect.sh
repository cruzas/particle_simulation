#!/bin/bash
# Bash script used to collect data on performance of particle simulation.

declare -a nParticles=(10 100 1000 10000)

#pos=$(( ${#nParticles[*]} - 1 ))
#last=${nParticles[$pos]}

filename_serial="benchmark_serial.txt"

echo "num_particles" "," "time_ns" > $filename_serial

# TODO: Add up average times.

for i in "${nParticles[@]}"
do
  iParticles=$i
  # echo "nParticles="$i >> $filename_serial
  # TODO: fix so that avg_duration is added to line that says nParticles=...
  avg_time=`./particles_serial n=$i delta=0.5 total_time_interval=100 | cut -d= -f2`

  echo $iParticles "," $avg_time >> $filename_serial

  rm particle_positions/positions_*.vtk

  # mpirun -np $rank ./main $i $i 100 0.01 > results/res_r_"$rank"_n_"$num_threads"_"$i".txt
done

# TODO: Execute code using OpenACC.
# declare -a nParticles=(10 100 1000 10000)

#pos=$(( ${#nParticles[*]} - 1 ))
#last=${nParticles[$pos]}

filename_parallel="benchmark_parallel.txt"

echo "num_particles" "," "time_ns" > $filename_parallel

# TODO: Add up average times.

for i in "${nParticles[@]}"
do
  iParticles=$i
  # echo "nParticles="$i >> $filename_serial
  # TODO: fix so that avg_duration is added to line that says nParticles=...
  avg_time=`./particles_parallel n=$i delta=0.5 total_time_interval=100 | cut -d= -f2`

  echo $iParticles "," $avg_time >> $filename_parallel

  rm particle_positions/positions_*.vtk
done

echo All done
