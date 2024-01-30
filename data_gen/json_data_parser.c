#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <getopt.h>
#include <unistd.h>
#include <errno.h>

#include "haversine.h"

#define ERROR(...) {                    \
        fprintf(stderr, "[%d]", __LINE__);      \
        fprintf(stderr, __VA_ARGS__);   \
        exit(1);                        \
    }

#define APPROX_EARTH_RADIUS     6372.8
#define MAX_ITEM_NAME_LEN       8
#define MAX_VALUE_LEN           64

char item_name_buffer[MAX_ITEM_NAME_LEN] = {};
char item_value_buffer[MAX_VALUE_LEN] = {};

void usage(void) {
    fprintf(stderr, "Data Generator Binary Reader:\n");
    fprintf(stderr, "-h            This help dialog.\n");
    fprintf(stderr, "-i <bin_file> Path to binary file.\n");
}

/*
 * There is no clear guidance on how conformant this json parser should be
 * It is not the goal to implement a JSON parser but just something that can
 * read the previously generated file. We could cheat as we know the exact 
 * structure to expect:
 * 
 * {"pairs":[
 * {"x0":-152.1370425038677752, "y0":-71.3097241387282139, "x1":-163.3649409391753977, "y1":-67.8408444243673472},
 * {"x0":-97.5403914868554125, "y0":-54.3610062144515211, "x1":-91.8418793435403558, "y1":-45.5549272734461894}
 * ]}
 * 
 * But since we probably want a pig parser to optimize later
 * we are going to add some flexibility like:
 * - ignore whitespace
 * - ignore linefeed or carriage return
 * - data items do not need to be in the order x0, y0, x1, y1, but are tagged using lowercase
 * 
 * Will start with a char based parser which is really inneficient and slow
 * 
 * By using advance to char we may be skipping bad formatted json or other data we don't care about,
 * so this is not really a good way to validate the structure is actually real json
 */

void advance_until(FILE *fp, char target) {
    char read_data;
    int ret;
    uint64_t count = 0;
    while(1) {
        ret = fread(&read_data, sizeof(char), 1, fp);
        if (ret == 0) {
            ERROR("Ran out file contents searching for [%c]\n", target);
        }
        count++;
        if (read_data==target) {
            // found target and we have already consumed
            printf("Found [%c] after [%lu] bytes read\n", target, count);
            return;
        }
    }
}

void find_string(FILE *fp, char *string) {
    int len = strlen(string);
    printf("Looking for [%s] len[%d]\n", string, len);
    char *ptr = string;
    for (int i=0; i<len; i++) {
        advance_until(fp, *ptr);
        ptr++;
    }
}

// similar to advance_until but it will extract the consumed chars to a
// given buffer, without the last target char
void extract_until(FILE *fp, char* buffer, int max_buffer_len, char *target) {
    char read_data;
    int ret;
    uint64_t count = 0;
    // clear buffer so that it is null terminated
    memset(buffer, 0, max_buffer_len);
    char *ptr = buffer;
    while(1) {
        ret = fread(&read_data, sizeof(char), 1, fp);
        if (ret == 0) {
            ERROR("Ran out file contents searching for [%c]\n", *target);
        }
        count++;
        if (read_data==*target) {
            // found target and we have already consumed
            printf("Found [%c] after [%lu] bytes read buffer[%s]\n", *target, count, buffer);
            return;
        } else {
            if (count>max_buffer_len) {
                ERROR("Overran the target buffer\n");
            }
            // copy read char to buffer
            *ptr = read_data;
            ptr++;
        }
    }
}


int main (int argc, char *argv[]) {
    int opt;
    char *input_file = NULL;
    FILE *json_fp = NULL;
    // uint64_t count = 0;
    // int ret;
    // uint64_t binary_write_count = 0;
    // double X0, Y0, X1, Y1 = 0;
    // double H_DIST, H_DIST_CALC, sum = 0;
    // double delta = 0;

    while( (opt = getopt(argc, argv, "hi:")) != -1) {
        switch (opt) {
            case 'h':
                usage();
                exit(0);
                break;
            
            case 'i':
                input_file = strdup(optarg);
                break;

            default:
                fprintf(stderr, "ERROR Invalid command line option\n");
                usage();
                exit(1);
                break;
        }
    }

    if (!input_file) {
        usage();
        exit(1);
    }

    printf("================\n");
    printf("JSON Data Parser\n");
    printf("================\n");

    printf("Using input_file              [%s]\n", input_file);
    printf("\n\n");

    json_fp = fopen(input_file, "r");
    if (!json_fp) {
        ERROR("Failed to open binary file [%d][%s]\n", errno, strerror(errno));
    }

    // first hit the start of json
    advance_until(json_fp, '{');

    // there may be other json items, we want to find "pairs"
    find_string(json_fp, "\"pairs\"");
    advance_until(json_fp, ':');
    advance_until(json_fp, '[');    // start of json array

    advance_until(json_fp, '{');    // start of item
    
    advance_until(json_fp, '"');    // Start of item id
    // next we have the id of a value, can be x0, y0, x1, y1 and then the end '"' if the id
    // extract into item_name_buffer until we find the end of the identifier
    extract_until(json_fp, item_name_buffer, MAX_ITEM_NAME_LEN, "\"");


    // while (1) {
    //     ret = fread(&X0, sizeof(double), 1, json_fp);
    //     if (ret!=1) break;
    //     ret = fread(&Y0, sizeof(double), 1, json_fp);
    //     if (ret!=1) break;
    //     ret = fread(&X1, sizeof(double), 1, json_fp);
    //     if (ret!=1) break;
    //     ret = fread(&Y1, sizeof(double), 1, json_fp);
    //     if (ret!=1) break;
    //     ret = fread(&H_DIST, sizeof(double), 1, json_fp);
    //     if (ret!=1) break;
    //     H_DIST_CALC = ReferenceHaversine(X0, Y0, X1, Y1, APPROX_EARTH_RADIUS);
    //     delta = H_DIST - H_DIST_CALC;
    //     // printf("x0:%3.16f, y0:%3.16f, x1:%3.16f, y1:%3.16f | h_dist:%3.16f | h_dist_calc:%3.16f | delta:%3.16f\n", X0, Y0, X1, Y1, H_DIST, H_DIST_CALC, delta);
    //     if (delta!=0) {
    //         ERROR("Delta[%3.16f] for a row[%lu] is not zero\n", delta, count);
    //     }
    //     sum += H_DIST_CALC;
    //     binary_write_count += 5;
    //     count++;
    // }

    // printf("All read values match expected Haversine Distance\n\n");

   
    // double average = sum/count;
    // printf("binary_write_count     %lu\n", binary_write_count);
    // printf("binary_bytes_writen    %lu\n", binary_write_count*sizeof(double));
    // printf("sum H_DIST_CALC        %3.16f\n", sum);
    // printf("average H_DIST_CALC    %3.16f\n\n", average);


    fclose(json_fp);

    return 0;
}
