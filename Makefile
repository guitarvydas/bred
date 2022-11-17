#all: temp

all: fab/fab firstToSecond

firstToSecondDev:
	clear
	./fab/fab - Bred bred.ohm bredohm1.fab <test.txt >pattern.ohm
	./fab/fab - Bred bred.ohm bredohm2.fab <test.txt >>pattern.ohm
	echo try pattern.ohm against container.0d
	./fab/fab - Bred bred.ohm bredfab.fab --support=support.js <test.txt > pattern.fab
	./fab/fab - Bred bred.ohm bredfab.fab --support=support.js <test.txt > pattern.fab
	./fab/fab - Run pattern.ohm pattern.fab <container.0d


firstToSecond:
	clear
	./fab/fab - Bred bred.ohm bredohm1.fab <test.txt >pattern.ohm
	./fab/fab - Bred bred.ohm bredohm2.fab <test.txt >>pattern.ohm
	./fab/fab - Bred bred.ohm bredfab.fab --support=support.js <test.txt > pattern.fab
	./fab/fab - Run pattern.ohm pattern.fab <container.0d

install: repos npmstuff

fab/fab : ../fab/fab
	multigit -r

repos: fab/fab

npmstuff:
	npm install ohm-js yargs atob pako
