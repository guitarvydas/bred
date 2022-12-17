#!/bin/bash
## usage: ./bred-generate.bash fname message.bred breddir

# synonyms
fname=$1
spec=$2
bdir=$3
support=--support=${bdir}/support.js

# generate .ohm file
${bdir}/fab/fab - Bred ${bdir}/bred.ohm ${bdir}/bredohm1.fab           <${spec}  >${fname}.ohm
${bdir}/fab/fab - Bred ${bdir}/bred.ohm ${bdir}/bredohm2.fab           <${spec} >>${fname}.ohm
${bdir}/fab/fab - Bred ${bdir}/bred.ohm ${bdir}/bredohm3.fab           <${spec} >>${fname}.ohm
# generate .fab file
${bdir}/fab/fab - Bred ${bdir}/bred.ohm ${bdir}/bredfab.fab ${support} <${spec} >${fname}.fab

