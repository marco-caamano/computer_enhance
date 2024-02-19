all: librdtsc_utils.a rdtsc rdtsc_test perf_test1 perf_test2 perf_test3

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

perf_test1: perf_test1.c librdtsc_utils.a
	gcc -std=c11 -g -Wall -Werror -D_POSIX_C_SOURCE=200809L -D_DEFAULT_SOURCE -L. perf_test1.c -o perf_test1  -lrdtsc_utils

perf_test2: perf_test2.c librdtsc_utils.a
	gcc -std=c11 -g -Wall -Werror -D_POSIX_C_SOURCE=200809L -D_DEFAULT_SOURCE -L. perf_test2.c -o perf_test2  -lrdtsc_utils

perf_test3: perf_test3.c librdtsc_utils.a
	gcc -std=c11 -g -Wall -Werror -D_POSIX_C_SOURCE=200809L -D_DEFAULT_SOURCE -L. perf_test3.c -o perf_test3  -lrdtsc_utils

clean:
	rm -f *.o *.a rdtsc rdtsc_utils rdtsc_test perf_test1 perf_test2 perf_test3
