all:
	./fab/fab - Bred bred.ohm bredohm.fab <test.txt
	./fab/fab - Bred bred.ohm bredfab.fab <test.txt

install: repos npmstuff

repos:
	multigit -r

npmstuff:
	npm install ohm-js yargs atob pako
