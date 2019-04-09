#!/bin/bash
# rricharz 2019

wait5s() {
# let tek4010 wait 5 seconds (not this script, which just shuffles stuff into the buffer!)
for i in {1..100}
  do
    printf '\007'
  done
}

head -n 26 tek4010.c

wait5s
# erase screen
    printf '\033\014'

cat pltfiles/00README.txt

wait5s
wait5s

cat dodekagon.plt

wait5s

for filename in pltfiles/*.plt
do
    # erase screen
    printf '\033\014'
    echo tek4010 is displaying "$filename"
    cat "$filename"
    wait5s
done



