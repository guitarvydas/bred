#all: temp
pattern=pattern
bdir=./

all: fab/fab transpile

dev: dev0a dev0b dev0c dev0 dev00 dev1 dev2a dev2b dev3a dev4a dev4 dev4a dev5 devg dev6 devgrep

dev0a:
	./bred-generate.bash pattern spec0.bred . <src0.txt
	@ls -l pattern.*

dev0b:
	./bred-apply.bash    pattern            . <src0.txt

dev0c:
	./bred-generate.bash out spec0.bred .
	./bred-apply.bash    pattern            . <src0.txt

dev0:
	./bred-transpile.bash spec0.bred . <src0.txt

dev00:
	./bred-transpile.bash spec0.bred . <src0a.txt

dev1:
	./bred-transpile.bash spec1.bred . <src1.txt

dev2a:
	./bred-transpile.bash spec1.bred . <src2a.txt

dev2b:
	./bred-transpile.bash spec1.bred . <src2b.txt

dev2:
	./bred-transpile.bash spec1.bred . <src2.txt

dev3a:
	./bred-transpile.bash message.bred . <src3a.txt

dev4a:
	./bred-transpile.bash message1.bred . <src4a.txt

dev4:
	./bred-transpile.bash message.bred . <src4.txt

dev5:
	./bred message.bred . --keep <src5.txt

devg:
	./bred message.bred . --keep <srcg.txt

devjunk:
	./bred message.bred . --keep <srcjunk.txt

devgrep:
	./bred c.bred . --keep <grep.c

dev6:
	./bred c.bred . --keep <src6.txt


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
