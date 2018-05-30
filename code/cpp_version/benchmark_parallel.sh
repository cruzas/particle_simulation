#!/bin/bash
# Bash script used to collect data on performance of particle simulation.

declare -a nParticles=(1000000, 10000000, 100000000, 1000000000)

pos=$(( ${#nParticles[*]} - 1 ))
last=${nParticles[$pos]}

filename_parallel="benchmark_parallel.txt"

echo "num_particles" "," "time_ns" > $filename_parallel

# Test with constant delta_t
delta_t=0.5
for i in "${nParticles[@]}"
do
sbatch <<-_EOF
#!/bin/bash
#SBATCH --job-name=PP${i}
#SBATCH --ntasks-per-node=1
#SBATCH --nodes=1
#SBATCH --time=23:59:59
#SBATCH --output=./output/pp_${i}.out
#SBATCH --error=./errors/err_p_${i}.err
#SBATCH --partition=gpu
#SBATCH --mem=128000

echo "num_particles=${i}"
echo "delta_t=${delta_t}"

module load use.own
module load gcc/6.1.0
module load pgi

# run the experiment
srun ./particles_parallel npart=$i delta_t=$delta_t nsteps=100
_EOF
done
