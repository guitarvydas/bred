all:
	./fab/fab - Bred bred.ohm bred.fab <test.txt

install: repos npmstuff

repos:
	multigit -r

npmstuff:
	npm install ohm-js yargs atob pako
