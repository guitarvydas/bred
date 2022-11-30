#!/bin/bash
## usage: ./bred-apply.bash fname breddir <src

# synonyms
fname=$1
bdir=$2

# run the nano-transpiler
${bdir}/fab/fab - NestingGrammar ${fname}.ohm ${fname}.fab
