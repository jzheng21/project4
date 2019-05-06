#!/bin/bash

sbatch --constraint=elves --time=3:00:00 --mem-per-cpu=10G --nodes=1 --cpus-per-task=1 openmp_script.sh 1

for i in 2 4 8 16
do
sbatch --constraint=elves --time=3:00:00 --mem-per-cpu=3G --nodes=1 --cpus-per-task=$i openmp_script.sh $i
done
