#!/bin/bash -l

#SBATCH -p connectx5
#SBATCH -N 32
#SBATCH -t 00:30:00
#SBATCH -J hickok-salesman
#SBATCH --exclusive

set -e

ulimit -c unlimited

export GASNET_BACKTRACE=1
export LD_PRELOAD="$HOME/gperftools-install/lib/libtcmalloc.so"
export TCMALLOC_LARGE_ALLOC_REPORT_THRESHOLD=$((4 * 1024 * 1024 * 1024))

# Each node has 2 sockets, 8 hyperthreaded cores per socket

# for INPUT in ../support/tspfile04.txt  ../support/tspfile14.txt  ../support/tspfile15.txt  ../support/tspfile16.txt  ../support/tspfile17.txt  ../support/tspfile20.txt; do
for INPUT in ../support/tspfile15.txt; do
    # for EXE in ./tsp_shmem_LLPQ.out ../shmem_PPQ/tsp_shmem_PPQ.out ../shmem_pure/tsp_shmem_pure.out; do
    for EXE in ./tsp_asyncshmem.out ../shmem_LLPQ/tsp_shmem_LLPQ.out; do
        echo INPUT = $INPUT , EXE = $EXE
        srun --nodes=2 --ntasks-per-node=2 --ntasks-per-socket=1 $EXE $INPUT
        echo
    done
done
