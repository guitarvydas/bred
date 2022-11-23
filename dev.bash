#!/bin/bash
## usage: ./bred.bash message.bred container.u0d >container.0d
pattern=pattern
bdir=./
./fab/fab - Bred ${bdir}/bred.ohm ${bdir}/bredohm1.fab <$1 >${pattern}.ohm
./fab/fab - Bred ${bdir}/bred.ohm ${bdir}/bredohm2.fab <$1 >>${pattern}.ohm
cat pattern.ohm
./fab/fab - Bred ${bdir}/bred.ohm ${bdir}/bredfab.fab --support=${bdir}/support.js <$1 > ${pattern}.fab
./fab/fab - NestingGrammar ${pattern}.ohm ${pattern}.fab <$2

