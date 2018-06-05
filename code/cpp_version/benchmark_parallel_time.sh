#!/bin/bash
# Bash script used to collect data on performance of particle simulation.

declare -a nsteps=(10 100 1000 10000)

pos=$(( ${#nsteps[*]} - 1 ))
last=${nsteps[$pos]}

filename_parallel="benchmark_parallel_time.txt"

echo "nsteps" "," "time_ms" > $filename_parallel

# Test with constant delta_t
delta_t=0.5
for i in "${nsteps[@]}"
do
sbatch <<-_EOF
#!/bin/bash
#SBATCH --job-name=PPtime${i}
#SBATCH --ntasks-per-node=1
#SBATCH --nodes=1
#SBATCH --time=23:59:59
#SBATCH --output=./output/ppt_${i}.out
#SBATCH --error=./errors/err_pt_${i}.err
#SBATCH --partition=gpu
#SBATCH --mem=128000

echo "nsteps=${i}"
echo "delta_t=${delta_t}"

module load use.own
module load gcc/6.1.0
module load pgi

# run the experiment
srun ./particles_parallel npart=10000 delta_t=$delta_t nsteps=$i
_EOF
done
