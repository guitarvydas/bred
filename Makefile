#all: temp
pattern=pattern
bdir=./

all: fab/fab transpile

dev: dev0a

dev0:
	./bred-generate.bash out spec0.bred . <src0.txt
	@echo generated out.ohm and out.fab

dev0a:
	./bred-generate.bash out spec0.bred .
	./bred-apply.bash    out            . <src0.txt


devmanual: manual

manual:
	# test that bred.ohm parses the spec
	./fab/fab - Bred ${bdir}/bred.ohm ${bdir}/bredidentity.fab <message.bred
	# test part 1 of the generated parser, using bredohm1.fab to generate part 1
	# part 1 generates the Nesting grammar.
	./fab/fab - Bred ${bdir}/bred.ohm ${bdir}/bredohm1.fab --support=support.js <message.bred >pattern.ohm
	# test part 2 of the generated parser, using bredohm2.fab to generate part 2
	# part 2 generates the "pattern" rule
	./fab/fab - Bred ${bdir}/bred.ohm ${bdir}/bredohm2.fab --support=support.js <message.bred >>pattern.ohm
	# part 3 generates a list of literals used in the match pattern (pattern1)
	# the list of literals might contain redundancy, but, it doesn't matter - the engine will skip the redundant bits if it matches the first (redundant) item
	./fab/fab - Bred ${bdir}/bred.ohm ${bdir}/bredohm3.fab --support=support.js <message.bred >>pattern.ohm
	# test fab generator - bredfab.fab creates pattern.fab
	./fab/fab - Bred ${bdir}/bred.ohm ${bdir}/bredfab.fab <message.bred --support=support.js >pattern.fab

	@echo
	@echo
	diff nestingFab.fab pattern.fab


install: repos npmstuff

fab/fab : ../fab/fab
	multigit -r

repos:
	multigit -r

npmstuff:
	npm install ohm-js yargs atob pako
