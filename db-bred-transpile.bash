#!/bin/bash
## usage: ./db-bred-transpile.bash message.bred breddir <src
## this version is for bootstrap debug only

#synonyms

# when released...
pattern=pattern

# for now, during bootstrap
fname=${pattern}
spec=$1
bdir=$2

# compile (generate) matcher/replacer
${bdir}/bred-generate.bash ${fname} ${spec} ${bdir}
# apply matcher/replacer to source
${bdir}/bred-apply.bash ${fname} ${bdir}

