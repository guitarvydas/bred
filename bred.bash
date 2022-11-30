#!/bin/bash
## usage: ./bred.bash message.bred container.u0d breddir >container.0d

# when released...
#pattern=pattern_${RANDOM}

# for now, during bootstrap
pattern=pattern

bdir=$3
support=--support=${bdir}/support.js
${bdir}/bred1.bash  $1 $2 $3 >${pattern}.ohm
${bdir}/bred2.bash  $1 $2 $3 >${pattern}.fab

./fab/fab - NestingGrammar ${pattern}.ohm ${pattern}.fab <$2

# when released...
#rm -f ${pattern}.ohm ${pattern}.fab

