#!/bin/sh
#
#

make test

tests=(1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20)
for i in "${tests[@]}"
do
  ./runTests -t $i > "$i.OUTPUT"
  sed '1d' "$i.OUTPUT" > "tempfile"
  mv "tempfile" "$i.OUTPUT"
done

for i in "${tests[@]}"
do
  ./runTests -t $i -e > "$i.EXPECTED"
  sed '1d' "$i.EXPECTED" > "tempfile"
  mv "tempfile" "$i.EXPECTED"
done

for i in "${tests[@]}"
do
  DIFF=$(diff "$i.OUTPUT" "$i.EXPECTED")
  if [ "$DIFF" == "" ]
  then
    echo "Test $i: PASSED"
  else
    echo "Test $i: FAILED"
  fi
done
