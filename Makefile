#all: temp

all: fab/fab firstToSecond

firstToSecond:
	./bred.bash message.bred container.0d >container.0d.out
	./bred.bash connection.bred connection.u0d >connection.0d

install: repos npmstuff

fab/fab : ../fab/fab
	multigit -r

repos: fab/fab

npmstuff:
	npm install ohm-js yargs atob pako
