#!/bin/sh
#
#

make test

tests=(1 2 3 4 5 6 7 8 9 10 11 12 13 14)
for i in "${tests[@]}"
do
  ./runTests -t $i > "$i.OUTPUT"
done

for i in "${tests[@]}"
do
  ./runTests -t $i -e > "$i.EXPECTED"
done
