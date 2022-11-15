all: firstToSecond

firstToSecond:
	./fab/fab - Bred bred.ohm bredohm.fab <test.txt >pattern.ohm
	./fab/fab - Bred bred.ohm bredfab.fab --support=support.js <test.txt >pattern.fab
	# ./fab/fab - Run pattern.ohm pattern.fab <container.0d

bothways:
	./fab/fab - Bred bred.ohm bredohm.fab <test.txt
	./fab/fab - Bred bred.ohm bredfab.fab --support=support.js <test.txt
	./fab/fab - Bred bred.ohm bredohm.fab <test2.txt
	./fab/fab - Bred bred.ohm bredfab.fab --support=support.js <test2.txt

install: repos npmstuff

repos:
	multigit -r

npmstuff:
	npm install ohm-js yargs atob pako
