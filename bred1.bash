#!/bin/bash
## usage: ./bred1.bash message.bred container.u0d breddir
pattern=pattern_${RANDOM}
bdir=$3
support=--support=${bdir}/support.js
./fab/fab - Bred ${bdir}/bred.ohm ${bdir}/bredohm1.fab ${support} <$1
./fab/fab - Bred ${bdir}/bred.ohm ${bdir}/bredohm2.fab ${support} <$1
./fab/fab - Bred ${bdir}/bred.ohm ${bdir}/bredohm3.fab  ${support} <$1

