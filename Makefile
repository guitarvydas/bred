#all: temp

all: fab/fab firstToSecond

dev:
	./bred.bash connection.bred connection.u0d

firstToSecond:
	./bred.bash message.bred container.0d >container.0d.out

install: repos npmstuff

fab/fab : ../fab/fab
	multigit -r

repos: fab/fab

npmstuff:
	npm install ohm-js yargs atob pako
