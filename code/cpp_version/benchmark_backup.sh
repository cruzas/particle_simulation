#!/bin/bash
# Bash script used to collect data on performance of particle simulation.

declare -a nParticles=(10 100 1000 10000)

pos=$(( ${#nParticles[*]} - 1 ))
last=${nParticles[$pos]}

filename_serial="benchmark_serial.txt"

echo "num_particles" "," "time_ns" > $filename_serial

# TODO: Add up average times.
for i in "${nParticles[@]}"
do
  avg_time=`./particles_serial n=$i delta=0.5 total_time_interval=200 | cut -d= -f2`

  echo $i "," $avg_time >> $filename_serial

  # Only remove particle positions if not in last program execution.
  # I.e., keep only particle positions from last program execution.
  if [ $i -ne $last ]
  then
    rm particle_positions/positions_*.vtk
  fi
done

# Keeping the following three commented in case we later want to execute
# parallel code with more particles than in serial version. But we'd have
# to have an nParticles2 variable or something of the sort.
# declare -a nParticles2=(10 100 1000 10000)
# pos=$(( ${#nParticles2[*]} - 1 ))
# #last=${nParticles2[$pos]}
filename_parallel="benchmark_parallel.txt"

echo "num_particles" "," "time_ns" > $filename_parallel

# TODO: Add up average times.
for i in "${nParticles[@]}"
do
  avg_time=`./particles_parallel n=$i delta=0.5 total_time_interval=200 | cut -d= -f2`

  echo $i "," $avg_time >> $filename_parallel

  # Only remove particle positions if not in last program execution.
  # I.e., keep only particle positions from last program execution.
  if [ $i -ne $last ]
  then
    rm particle_positions/positions_*.vtk
  fi
done

echo All done
