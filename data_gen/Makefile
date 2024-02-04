all: data_gen bindata_reader json_data_parser

haversine.o: haversine.c
	gcc -std=c11 -g -Wall -Werror -D_POSIX_C_SOURCE=200809L -c haversine.c -o haversine.o

data_gen.o: data_gen.c
	gcc -std=c11 -g -Wall -Werror -D_POSIX_C_SOURCE=200809L -c data_gen.c -o data_gen.o

bindata_reader.o: bindata_reader.c
	gcc -std=c11 -g -Wall -Werror -D_POSIX_C_SOURCE=200809L -c bindata_reader.c -o bindata_reader.o

json_data_parser.o: json_data_parser.c
	gcc -std=c11 -g -Wall -Werror -D_POSIX_C_SOURCE=200809L -I../rdtsc -c json_data_parser.c -o json_data_parser.o

data_gen: data_gen.o haversine.o
	gcc -std=c11 -g -Wall -Werror -D_POSIX_C_SOURCE=200809L haversine.o data_gen.o -o data_gen -lm

bindata_reader: bindata_reader.o haversine.o
	gcc -std=c11 -g -Wall -Werror -D_POSIX_C_SOURCE=200809L haversine.o bindata_reader.o -o bindata_reader -lm

json_data_parser: json_data_parser.o haversine.o
	gcc -std=c11 -g -Wall -Werror -D_POSIX_C_SOURCE=200809L -L../rdtsc haversine.o json_data_parser.o -o json_data_parser -lm -lrdtsc_utils

clean:
	rm -f data_gen bindata_reader json_data_parser *.o test_data_seed_*.bin test_data_seed_*.json test_data_seed_*.txt
