#!/bin/bash
# idemo, run with tek4010 to display ICEMD vector data
# rricharz 2019

wait2s() {
# let tek4010 wait 2 seconds (not this script, which just shuffles stuff into the buffer!)
for i in {1..30}
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
    printf '<033:'
    echo tek4010 is displaying "$filename"
    cat "$filename"
    wait2s
  done

done

