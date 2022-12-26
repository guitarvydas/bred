#!/bin/bash
## usage: ./bred-transpile.bash message.bred breddir <src

#synonyms

# when released...
pattern=pattern_${RANDOM}

# for now, during bootstrap
fname=${pattern}
spec=$1
bdir=$2

# compile (generate) matcher/replacer
${bdir}/bred-generate.bash ${fname} ${spec} ${bdir}
# apply matcher/replacer to source
${bdir}/bred-apply.bash ${fname} ${bdir}
