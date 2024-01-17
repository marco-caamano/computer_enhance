simulator: simulator.c
	gcc -std=c11 -g -Wall -Werror -D_POSIX_C_SOURCE=200809L -I../libdecoder_8086 -L../libdecoder_8086 simulator.c -o simulator -ldecoder

test: simulator
	./run_tests.sh

clean:
	rm -f simulator tests/*.bin tests/out_*
