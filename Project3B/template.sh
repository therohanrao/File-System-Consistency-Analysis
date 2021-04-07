#!/bin/bash

./argparser $1

if [ $? -eq 1 ]
then
    exit 1
fi

./lab3b.py $1
exit $?



