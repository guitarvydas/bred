#!/bin/bash
## usage: ./bred2.bash message.bred container.u0d breddir
# generate the .fab file corresponding to the pattern DSL in $1, and output it to stdout
pattern=pattern_${RANDOM}
bdir=$3
support=--support=${bdir}/support.js
./fab/fab - Bred ${bdir}/bred.ohm ${bdir}/bredfab.fab ${support} <$1 

