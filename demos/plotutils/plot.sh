#!/bin/sh
# erentar2002 2024

# This script plots certain properties of elements on the periodic table

# Dependency: gnu plotutils https://pkgs.org/search/?q=plotutils
# This package can be installed by running `apt install plotutils` on debian and its derivatives

# To display the plot, use ./tek4010 demos/plotuils/plot.sh

# From top to bottom:
#    - curl: download the periodic table data
#            source: https://gist.github.com/GoodmanSciences/c2dd862cd38f21b0ad36b8f96b4bf1ee
#
#    - awk: split file by seperator ","
#           then filter missing values 
#           then print atomic mass, then density multiplied by atomic radius
#
#    - sed: remove the top row which includes names
#
#    - graph: -L plot title
#             -X x axis title;
#             -Y y axis title;
#             -F font;
#             -m line type;
#             -S symbol type and size;
#             -w width;
#             -r move horizontally
#
#    - plot: -T use tek device
curl -s 'https://gist.githubusercontent.com/GoodmanSciences/c2dd862cd38f21b0ad36b8f96b4bf1ee/raw/1d92663004489a5b6926e944c1b3d9ec5c40900e/Periodic%2520Table%2520of%2520Elements.csv' \
| awk \
    '{
        split($0,a,",");
        if(a[4]*a[20]*a[17]){       # discard rows with missing columns
            print a[1],a[20]*a[17]
        }
    }' \
| sed 1d \
| graph \
    -L "Density of elements" \
    -X "atomic number" \
    -Y "density(g/cm^3) * atomic radius(rcovH)" \
    -F "HersheySans" \
    -m 2 \
    -S 3 0.03 \
    -w 1 \
    -r 0 \
| plot -T tek

# On the line containing `awk`, columns are selected at the print statement.
# This script selects the 4th and 20th columns, which are atomic mass and density respectively.

# Full documentation for the `graph` utility is available at https://www.gnu.org/software/plotutils/manual/en/plotutils.pdf
