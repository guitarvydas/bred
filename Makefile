#all: temp

all: fab/fab firstToSecond

dev:
	./dev.bash connection.bred connection.u0d
	cat pattern.ohm

firstToSecond:
	./bred.bash message.bred container.0d >container.0d.out

install: repos npmstuff

fab/fab : ../fab/fab
	multigit -r

repos: fab/fab

npmstuff:
	npm install ohm-js yargs atob pako
