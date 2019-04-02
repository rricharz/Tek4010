#!/bin/bash
# rricharz 2019

for filename in pltfiles/*.plt
do
    # erase screen
    printf '\033\014'

    echo tek4010 is displaying "$filename"

    cat "$filename"

    # let tek4010 wait 5 seconds (not this script, which just shuffles stuff into the buffer!)
    for i in {1..100}
    do
        printf '\007'
    done
done



