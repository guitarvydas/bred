#!/bin/bash
## usage: ./bred.bash message.bred container.u0d >container.0d
pattern=pattern_${RANDOM}
./fab/fab - Bred bred.ohm bredohm1.fab <$1 >${pattern}.ohm
./fab/fab - Bred bred.ohm bredohm2.fab <$1 >>${pattern}.ohm
./fab/fab - Bred bred.ohm bredfab.fab --support=support.js <$1 > ${pattern}.fab
./fab/fab - Run ${pattern}.ohm ${pattern}.fab <$2
