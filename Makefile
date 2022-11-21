#all: temp
pattern=pattern
bdir=./

all: fab/fab firstToSecond

dev:
	./fab/fab - Bred ${bdir}/bred.ohm ${bdir}/bredohm1.fab <message.bred >${pattern}.ohm
	# ./dev.bash message.bred shortcontainer.u0d >shortcontainer.0d
	# ./dev.bash senderreceiver.bred connection2.u0d >connection2.0d
	cat ${pattern}.ohm

firstToSecond:
	./bred.bash message.bred container.0d >container.0d.out
	./bred.bash connection.bred connection.u0d >connection.0d
	./bred.bash senderreceiver.bred connection2.u0d >connection2.0d
	cat connection2.0d

install: repos npmstuff

fab/fab : ../fab/fab
	multigit -r

repos: fab/fab

npmstuff:
	npm install ohm-js yargs atob pako
