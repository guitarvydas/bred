#all: temp
pattern=pattern
bdir=./

all: fab/fab firstToSecond

dev: manual

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
	# # test fab generator - bredfab.fab creates pattern.fab
	# ./fab/fab - Bred ${bdir}/bred.ohm ${bdir}/bredfab.fab <message.bred --support=support.js >pattern.fab
	# #
	# ./fab/fab - NestingGrammar pattern.ohm pattern.fab <shortcontainer.u0d >shortcontainer.0d
	# #
	# ./dev.bash message.bred shortcontainer.u0d >shortcontainer.00d
	# cat shortcontainer.00d

	# # ./dev.bash outputport.bred shortcontainer.00d >shortcontainer.000d
	# # cat shortcontainer.000d

	# # ./dev.bash outputport.bred outputport.u0d >outputport.0d
	# # cat outputport.0d

	@echo
	@echo
	diff pattern.ohm nestingGrammar.ohm


firstToSecond:
	./bred.bash connection.bred connection.u0d >connection.0d
	./bred.bash senderreceiver.bred connection2.u0d >connection2.0d
	cat connection2.0d

install: repos npmstuff

fab/fab : ../fab/fab
	multigit -r

repos:
	multigit -r

npmstuff:
	npm install ohm-js yargs atob pako
