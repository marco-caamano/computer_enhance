all:  libreptester.a rep_test1 rep_test2

CC			=	gcc
CFLAGS		=	-I. -I../rdtsc -std=c11 -g -Wall -Werror -D_POSIX_C_SOURCE=200809L -D_DEFAULT_SOURCE
LD_FLAGS	= 	-L. -L../rdtsc
DEPS 		=	reptester.h

%.o: %.c $(DEPS)
	$(CC) $(CFLAGS) -c $< -o $@ 

libreptester.a: reptester.o *.h
	ar rcs libreptester.a reptester.o

rep_test1:	rep_test1.o libreptester.a
	$(CC) $(LD_FLAGS) $@.o -o $@ -lreptester -lrdtsc_utils

rep_test2:	rep_test2.o libreptester.a
	$(CC) $(LD_FLAGS) $@.o -o $@ -lreptester -lrdtsc_utils

.PHONY: clean

clean:
	rm -f *.o *.a a.out rep_test1 rep_test2
