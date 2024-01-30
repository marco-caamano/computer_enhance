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

#define LOG(...) {                  \
        if (verbose) {              \
            printf(__VA_ARGS__);    \
        }                           \
    }

#define APPROX_EARTH_RADIUS     6372.8
#define MAX_ITEM_NAME_LEN       8
#define MAX_VALUE_LEN           64

#define FOUND_X0                0x1
#define FOUND_Y0                0x2
#define FOUND_X1                0x4
#define FOUND_Y1                0x8


char item_name_buffer[MAX_ITEM_NAME_LEN] = {};
char item_value_buffer[MAX_VALUE_LEN] = {};
bool verbose = false;

void usage(void) {
    fprintf(stderr, "Data Generator Binary Reader:\n");
    fprintf(stderr, "-h            This help dialog.\n");
    fprintf(stderr, "-i <bin_file> Path to binary file.\n");
    fprintf(stderr, "-v            Enable Verbose Output.\n");
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

bool advance_until(FILE *fp, char target) {
    char read_data;
    int ret;
    uint64_t count = 0;
    while(feof(fp)==0) {
        ret = fread(&read_data, sizeof(char), 1, fp);
        if (ret == 0) {
            LOG("Ran out file contents searching for [%c] feof[%d]\n", target, feof(fp));
        }
        count++;
        if (read_data==target) {
            // found target and we have already consumed
            LOG("Found [%c] after [%lu] bytes read\n", target, count);
            return true;
        }
    }
    return false;
}

bool find_string(FILE *fp, char *string) {
    bool found_it = false;
    int len = strlen(string);
    LOG("Looking for [%s] len[%d]\n", string, len);
    char *ptr = string;
    for (int i=0; i<len; i++) {
        found_it = advance_until(fp, *ptr);
        if (!found_it) return false;
        ptr++;
    }
    // we found all elements
    return true;
}

// similar to advance_until but it will extract the consumed chars to a
// given buffer, without the last target char
bool extract_until(FILE *fp, char* buffer, int max_buffer_len, char *target) {
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
            LOG("Found [%c] after [%lu] bytes read buffer[%s]\n", *target, count, buffer);
            return true;
        } else {
            if (count>max_buffer_len) {
                ERROR("Overran the target buffer\n");
                break;
            }
            // copy read char to buffer
            *ptr = read_data;
            ptr++;
        }
    }
    return false;
}


int main (int argc, char *argv[]) {
    int opt;
    bool found_it;
    char *input_file = NULL;
    FILE *json_fp = NULL;
    int ret;
    double X0, Y0, X1, Y1 = 0;
    double H_DIST, sum, average = 0;
    uint64_t count_values = 0;

    while( (opt = getopt(argc, argv, "hi:")) != -1) {
        switch (opt) {
            case 'h':
                usage();
                exit(0);
                break;
            
            case 'i':
                input_file = strdup(optarg);
                break;

            case 'v':
                verbose = true;
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
        ERROR("Failed to open file [%d][%s]\n", errno, strerror(errno));
    }

    printf("Start reading File\n");
    printf("------------------\n");

    // first hit the start of json
    found_it = advance_until(json_fp, '{');
    if (!found_it) ERROR("Failed to find start of json\n");

    // there may be other json items, we want to find "pairs"
    found_it = find_string(json_fp, "\"pairs\"");
    if (!found_it) ERROR("Failed to find pairs item\n");
    found_it = advance_until(json_fp, ':');
    if (!found_it) ERROR("Failed to find item separator\n");
    found_it = advance_until(json_fp, '[');    // start of json array
    if (!found_it) ERROR("Failed to find start of json array\n");

    while(feof(json_fp)==0) {
        uint8_t found_item = 0;
        uint8_t item_count = 0;
        double value;
        bool found_x0;
        bool found_y0;
        bool found_x1;
        bool found_y1;

        // parse array row
        found_it = advance_until(json_fp, '{');    // start of row item
        if (!found_it) break;

        while (item_count<4) {
            found_it = advance_until(json_fp, '"');    // Start of item id
            if (!found_it) break;
            // next we have the id of a value, can be x0, y0, x1, y1 and then the end '"' if the id
            // extract into item_name_buffer until we find the end of the identifier
            extract_until(json_fp, item_name_buffer, MAX_ITEM_NAME_LEN, "\"");

            if (strncmp("x0",item_name_buffer,MAX_ITEM_NAME_LEN)==0) {
                found_item = FOUND_X0;
            } else if (strncmp("y0",item_name_buffer,MAX_ITEM_NAME_LEN)==0) {
                found_item = FOUND_Y0;
            } else if (strncmp("x1",item_name_buffer,MAX_ITEM_NAME_LEN)==0) {
                found_item = FOUND_X1;
            } else if (strncmp("y1",item_name_buffer,MAX_ITEM_NAME_LEN)==0) {
                found_item = FOUND_Y1;
            } else {
                ERROR("Failed to identify item[%s]\n", item_name_buffer);
            }
            found_it = advance_until(json_fp, ':');    // start of value
            if (!found_it) break;

            if (item_count<3) {
                // first 3 values end with ','
                extract_until(json_fp, item_value_buffer, MAX_VALUE_LEN, ",");
            } else {
                // the last item does not end with ',' instead we hit the end of the array item '}'
                extract_until(json_fp, item_value_buffer, MAX_VALUE_LEN, "}");
            }
            
            ret = sscanf((char *)&item_value_buffer, "%lf", &value);
            if (ret!=1) {
                ERROR("Failed to parse value from [%s]\n", item_value_buffer);
            }
            LOG("Found value [%3.16f]\n", value);
            switch (found_item) {
                case FOUND_X0:
                    X0 = value;
                    found_x0 = true;
                    break;
                case FOUND_Y0:
                    Y0 = value;
                    found_y0 = true;
                    break;
                case FOUND_X1:
                    X1 = value;
                    found_x1 = true;
                    break;
                case FOUND_Y1:
                    Y1 = value;
                    found_y1 = true;
                    break;
                default:
                    ERROR("Invalid found_item[%d]\n", found_item);
                    break;
            }
            item_count++;
        }

        if (!found_x0 || !found_y0 || !found_x1 || !found_y1) {
            ERROR("Invalid row format, failed to find all items\n");
        }
        H_DIST = ReferenceHaversine(X0, Y0, X1, Y1, APPROX_EARTH_RADIUS);

        sum += H_DIST;
        count_values++;

        LOG("x0:%3.16f, y0:%3.16f, x1:%3.16f, y1:%3.16f | h_dist:%3.16f \n", X0, Y0, X1, Y1, H_DIST);
    }


    if (feof(json_fp)==1) {
        printf("-------------------\n");
        printf("Reached End of File\n\n");
    }

    average = sum/count_values;

    printf("Count                 %lu items\n", count_values);
    printf("sum H_DIST            %3.16f\n", sum);
    printf("average H_DIST        %3.16f\n\n", average);


    fclose(json_fp);

    return 0;
}
