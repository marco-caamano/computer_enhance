all: data_gen

haversine.o: haversine.c
	gcc -std=c11 -g -Wall -Werror -D_POSIX_C_SOURCE=200809L -c haversine.c -o haversine.o

data_gen.o: data_gen.c
	gcc -std=c11 -g -Wall -Werror -D_POSIX_C_SOURCE=200809L -c data_gen.c -o data_gen.o

data_gen: data_gen.o haversine.o
	gcc -std=c11 -g -Wall -Werror -D_POSIX_C_SOURCE=200809L -L. haversine.o data_gen.o -o data_gen -lm

clean:
	rm -f data_gen *.o datagen_seed_*.bin datagen_seed_*.json datagen_seed_*.txt
