all:  librdtsc_utils.a perf_test1 perf_test2 perf_test3 perf_test4 rdtsc_test

CC			=	gcc
CFLAGS		=	-I. -std=c11 -g -Wall -Werror -D_POSIX_C_SOURCE=200809L -D_DEFAULT_SOURCE
LD_FLAGS	= -L.
DEPS 		=	rdtsc_utils.h

ifdef PROFILER
CFLAGS += "-DPROFILER=1"
endif

%.o: %.c $(DEPS)
	$(CC) $(CFLAGS) -c $< -o $@ 

librdtsc_utils.a: rdtsc_utils.o *.h
	ar rcs librdtsc_utils.a rdtsc_utils.o

perf_test1:	perf_test1.o
	$(CC) $(LD_FLAGS) $@.o -o $@ -lrdtsc_utils

perf_test2:	perf_test2.o
	$(CC) $(LD_FLAGS) $@.o -o $@ -lrdtsc_utils

perf_test3:	perf_test3.o
	$(CC) $(LD_FLAGS) $@.o -o $@ -lrdtsc_utils

perf_test4:	perf_test4.o
	$(CC) $(LD_FLAGS) $@.o -o $@ -lrdtsc_utils

rdtsc_test: rdtsc_test.o
	$(CC) $(LD_FLAGS) $@.o -o $@ -lrdtsc_utils

.PHONY: clean

clean:
	rm -f *.o *.a a.out rdtsc rdtsc_utils rdtsc_test perf_test1 perf_test2 perf_test3 perf_test4
