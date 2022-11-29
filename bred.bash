#!/bin/bash
## usage: ./bred.bash message.bred container.u0d breddir >container.0d
pattern=pattern_${RANDOM}
bdir=$3
support=--support=${bdir}/support.js
${bdir}/bred1.bash - Bred ${bdir}/bred.ohm ${bdir}/bredohm1.fab ${support} <$1 >${pattern}.ohm
${bdir}/bred2.bash - Bred ${bdir}/bred.ohm ${bdir}/bredohm1.fab ${support} <$1 >${pattern}.fab

./fab/fab - NestingGrammar ${pattern}.ohm ${pattern}.fab <$2
rm -f ${pattern}.ohm ${pattern}.fab

