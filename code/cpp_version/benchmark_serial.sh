#!/bin/bash
# Bash script used to collect data on performance of particle simulation.

declare -a nParticles=(1000000 2500000 5000000 10000000)

pos=$(( ${#nParticles[*]} - 1 ))
last=${nParticles[$pos]}

filename_serial="benchmark_serial.txt"

echo "num_particles" "," "time_ms" > $filename_serial

# Test with constant delta_t
delta_t=0.5
for i in "${nParticles[@]}"
do
sbatch <<-_EOF
#!/bin/bash
#SBATCH --job-name=PS${i}
#SBATCH --ntasks-per-node=1
#SBATCH --nodes=1
#SBATCH --time=23:59:59
#SBATCH --output=./output/ps_${i}.out
#SBATCH --error=./errors/err_s_${i}.err
#SBATCH --partition=gpu
#SBATCH --mem=128000

echo "num_particles=${i}"
echo "delta_t=${delta_t}"

module load use.own
module load gcc/6.1.0
module load pgi

# run the experiment
srun ./particles_serial npart=$i delta_t=$delta_t nsteps=10
_EOF
done
