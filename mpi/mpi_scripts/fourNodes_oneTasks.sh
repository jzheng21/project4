#!/bin/bash -l

#SBATCH --mem-per-cpu=3G
#SBATCH --time=1:00:00
#SBATCH --partition=killable.q
#SBATCH --constraint=elves
#SBATCH --nodes=4
#SBATCH --ntasks-per-node=1
#SBATCH --job-name=3way-mpi

module load OpenMPI
mpirun $HOME/project4/mpi/mpi_comp
