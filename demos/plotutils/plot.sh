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
#           then filter missing values in column 20 (density)
#           then filter up to element 105
#           then print atomic mass and density
#
#    - sed: remove the top row which includes names
#
#    - graph: -X x axis title;
#             -Y y axis title;
#             -F font;
#             -m line type;
#             -S symbol type and size;
#             -w width;
#             -r move horizontally
#
#    - plot: -T use tek device
curl 'https://gist.githubusercontent.com/GoodmanSciences/c2dd862cd38f21b0ad36b8f96b4bf1ee/raw/1d92663004489a5b6926e944c1b3d9ec5c40900e/Periodic%2520Table%2520of%2520Elements.csv' \
| awk '{split($0,a,","); if(length(a[20])!=0&&a[4]<105){print a[4],a[20]}}' | \
sed 1d \
| graph -X "atomic mass" -Y "density" -F "HersheySans" -m 1 -S 2 0.03 -w 1 -r 0 \
| plot -T tek

# On the line containing `awk`, columns are selected at the print statement.
# This script selects the 4th and 20th columns, which are atomic mass and density respectively.

# Full documentation for the `graph` utility is available at https://www.gnu.org/software/plotutils/manual/en/plotutils.pdf
