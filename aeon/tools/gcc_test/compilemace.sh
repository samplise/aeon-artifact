#!/bin/bash

# NOTICE: This script assumes you are at the mace build directory.
gcc_dir=/scratch/chuangw/opt
parallel_build=4

#for ver in $gcc_dir/*
for ver in $(find $gcc_dir/* -maxdepth 0 -type d);
do
  echo "Compile ContextLattice using " $ver
  make clean 2>&1 | tee $ver.log
  cmake  -D CMAKE_CXX_COMPILER=$ver/bin/g++ .. 2>&1 | tee -a $ver.log
  make -j $parallel_build 2>&1 | tee -a $ver.log
  make test 2>&1 | tee -a $ver.log
done
