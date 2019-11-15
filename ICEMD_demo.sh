#!/bin/bash
# rricharz 2019

wait5s() {
# let tek4010 wait 5 seconds (not this script, which just shuffles stuff into the buffer!)
for i in {1..100}
  do
    printf '\007'
  done
}

while true
do

  for filename in pltfiles/ICEMD_pltfiles/*.plt
  do
    # erase screen
    printf '\033\014'
    echo tek4010 is displaying "$filename"
    cat "$filename"
    wait5s
  done

done

