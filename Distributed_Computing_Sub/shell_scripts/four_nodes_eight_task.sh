#!/bin/bash -l

#SBATCH --mem-per-cpu=3G
#SBATCH --time=10:00:00
#SBATCH --partition=killable.q
#SBATCH --constraint=elves
#SBATCH --nodes=4
#SBATCH --ntasks-per-node=8
#SBATCH --job-name=OpenMPI_FindLines

module load OpenMPI
mpirun $HOME/CurrentWork/OpenMPI_FindLines
