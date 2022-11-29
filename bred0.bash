#!/bin/bash
## perform identity parse and output to stdout (useful for syntax check during bootstrap)
## usage: ./bred0.bash message.bred container.u0d breddir >container.0d
bdir=$3
support=--support=${bdir}/support.js
./fab/fab - Bred ${bdir}/bred.ohm ${bdir}/bredidentidy.fab ${support} <$1
