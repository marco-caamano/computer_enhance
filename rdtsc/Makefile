all: librdtsc_utils.a rdtsc rdtsc_test

rdtsc: rdtsc.c
	gcc -std=c11 -g -Wall -Werror -D_POSIX_C_SOURCE=200809L -D_DEFAULT_SOURCE rdtsc.c -o rdtsc

rdtsc_utils.o: rdtsc_utils.c
	gcc -std=c11 -g -Wall -Werror -D_POSIX_C_SOURCE=200809L -D_DEFAULT_SOURCE -c rdtsc_utils.c -o rdtsc_utils.o

librdtsc_utils.a: rdtsc_utils.o *.h
	ar rcs librdtsc_utils.a rdtsc_utils.o

rdtsc_test.o: rdtsc_test.c
	gcc -std=c11 -g -Wall -Werror -D_POSIX_C_SOURCE=200809L -D_DEFAULT_SOURCE -I. -c rdtsc_test.c -o rdtsc_test.o

rdtsc_test: rdtsc_utils.o rdtsc_test.o librdtsc_utils.a
	gcc -std=c11 -g -Wall -Werror -D_POSIX_C_SOURCE=200809L -D_DEFAULT_SOURCE -L. rdtsc_utils.o rdtsc_test.o -o rdtsc_test -lrdtsc_utils


clean:
	rm -f *.o *.a rdtsc rdtsc_utils rdtsc_test
