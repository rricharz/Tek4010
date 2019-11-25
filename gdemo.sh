#!/bin/bash
# gdemo, run with tek4010 to display grayscale images
# rricharz 2019

wait2s() {
# let tek4010 wait 2 seconds (not this script, which just shuffles stuff into the buffer!)
for i in {1..30}
  do
    printf '\007'
  done
}

for filename in pltfiles//PointPlot/*.plt
do
    # erase screen
    printf '\033\014'
    cat "$filename"
    wait2s
done



