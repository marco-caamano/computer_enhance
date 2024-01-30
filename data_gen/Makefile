all: data_gen bindata_reader

haversine.o: haversine.c
	gcc -std=c11 -g -Wall -Werror -D_POSIX_C_SOURCE=200809L -c haversine.c -o haversine.o

data_gen.o: data_gen.c
	gcc -std=c11 -g -Wall -Werror -D_POSIX_C_SOURCE=200809L -c data_gen.c -o data_gen.o

bindata_reader.o: bindata_reader.c
	gcc -std=c11 -g -Wall -Werror -D_POSIX_C_SOURCE=200809L -c bindata_reader.c -o bindata_reader.o

data_gen: data_gen.o haversine.o
	gcc -std=c11 -g -Wall -Werror -D_POSIX_C_SOURCE=200809L haversine.o data_gen.o -o data_gen -lm

bindata_reader: bindata_reader.o haversine.o
	gcc -std=c11 -g -Wall -Werror -D_POSIX_C_SOURCE=200809L haversine.o bindata_reader.o -o bindata_reader -lm

clean:
	rm -f data_gen bindata_reader *.o datagen_seed_*.bin datagen_seed_*.json datagen_seed_*.txt
