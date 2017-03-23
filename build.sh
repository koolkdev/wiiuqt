#!/bin/bash

mkdir -p bin

for app in nandBinCheck nandFixer
do
    cd $app
    qmake
    make -j4
    cp $app ../bin
    cd ..
done
