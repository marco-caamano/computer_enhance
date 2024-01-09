simulator: simulator.c
	gcc -std=c11 -g -Wall -Werror -D_POSIX_C_SOURCE=200809L simulator.c -o simulator

test: simulator
	./run_tests.sh

clean:
	rm -f simulator tests/*.bin tests/out_*
