#!/bin/bash

if [[ ! `pwd` =~ requests.easy$ ]]
then
    echo "please use convert_all.sh in the requests.easy directory"
    exit 1
fi

for ii in `ls -1 *.easy`
do
    jj=`echo $ii | sed 's/\.easy$//'`

    ./request_converter.pl $ii > ../requests/$jj
done
