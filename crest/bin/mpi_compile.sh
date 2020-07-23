#!/bin/sh
../cil/bin/cilly -c --save-temps --doCrestInstrument -I../include $1
filename=${1%.c}
mpicc -c $filename.cil.c -o $filename.cil.o
mpic++ -L../lib/ $filename.cil.o -lcrest -o $filename
mpirun -n 4 ./$filename
