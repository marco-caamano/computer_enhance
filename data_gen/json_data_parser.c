#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <getopt.h>
#include <unistd.h>
#include <errno.h>
#include <math.h>
#define __USE_UNIX98
#include <sys/types.h>
#include <sys/stat.h>

#include "haversine.h"
#include "rdtsc_utils.h"

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

#define JSON_OVERHEAD_SIZE      14
#define JSON_MAX_ROW_SIZE       111 // with trailing coma
#define JSON_MIN_ROW_SIZE       101 // with trailing coma

#define FOUND_X0                0x1
#define FOUND_Y0                0x2
#define FOUND_X1                0x4
#define FOUND_Y1                0x8

/*
 * We will support several parsing JSON options:
 * - put all items in a linked list (will malloc for each item)
 * - estimate the max items and preallocated the required memmory
 *   to store them all and avoid all the small malloc and linked
 *   list overhead
 */
struct data_item_s {
    double x0;
    double y0;
    double x1;
    double y1;
};

struct list_item_s {
    struct data_item_s data_item;
    struct list_item_s *next_item;
};

// when using linked list
struct list_item_s *list_head = NULL;
struct list_item_s *list_tail = NULL;

// when using preallocated memory array
struct data_item_s *data_array = NULL;

// how many data items were used
size_t data_item_count = 0;

char item_name_buffer[MAX_ITEM_NAME_LEN] = {};
char item_value_buffer[MAX_VALUE_LEN] = {};
bool verbose = false;

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

void parse_file(FILE *json_fp, bool preallocate_entries) {
    double X0, Y0, X1, Y1 = 0;
    bool found_it;
    int ret;

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
        LOG("Parsed x0:%3.16f, y0:%3.16f, x1:%3.16f, y1:%3.16f \n", X0, Y0, X1, Y1);

        if (preallocate_entries) {
            // use preallocated array
            struct data_item_s *item = &data_array[data_item_count];
            item->x0 = X0;
            item->y0 = Y0;
            item->x1 = X1;
            item->y1 = Y1;
        } else {
            // use linked list
            struct list_item_s *item = malloc(sizeof(struct list_item_s));
            if (!item) {
                ERROR("Failed to allocate [%lu]bytes for item", sizeof(struct list_item_s));
            }
            item->data_item.x0 = X0;
            item->data_item.y0 = Y0;
            item->data_item.x1 = X1;
            item->data_item.y1 = Y1;
            item->next_item = NULL;
            // add to linked list
            if (!list_head) {
                // first item, it is both head and tail
                list_head = item;
                list_tail = item;
            } else {
                // there are already items in the list
                // append to the tail
                list_tail->next_item = item;
                list_tail = item;
            }
        }
        data_item_count++;
    }
    LOG("Total Parsed data items [%lu]\n", data_item_count);
}

void calculate_haversine_average(bool preallocate_entries) {
    double H_DIST, sum, average = 0;
    uint64_t count_values = 0;

    if (preallocate_entries) {
        // use preallocated array
        for (size_t i=0; i<data_item_count; i++) {
            struct data_item_s *item = &data_array[i];
            H_DIST = ReferenceHaversine(item->x0, item->y0,
                                        item->x1, item->y1,
                                        APPROX_EARTH_RADIUS);
            LOG("x0:%3.16f, y0:%3.16f, x1:%3.16f, y1:%3.16f | h_dist:%3.16f \n", 
                item->x0, item->y0, item->x1, item->y1, H_DIST);

            sum += H_DIST;
            count_values++;
        }
    } else {
        // sanity check
        if (!list_head || !list_tail) {
            ERROR("Sanity check failed for linked list");
        }
        // use linked list
        struct list_item_s *item = list_head;
        while (item) {
            H_DIST = ReferenceHaversine(item->data_item.x0, item->data_item.y0,
                                        item->data_item.x1, item->data_item.y1,
                                        APPROX_EARTH_RADIUS);
            LOG("x0:%3.16f, y0:%3.16f, x1:%3.16f, y1:%3.16f | h_dist:%3.16f \n", 
                item->data_item.x0,
                item->data_item.y0,
                item->data_item.x1,
                item->data_item.y1,
                H_DIST);

            sum += H_DIST;
            count_values++;

            // move to next item
            item = item->next_item;
        }
    }
    average = sum/count_values;

    printf("Count                 %lu items\n", count_values);
    printf("sum H_DIST            %3.16f\n", sum);
    printf("average H_DIST        %3.16f\n\n", average);
}

/*
 * This will give us an estimate on how many data entries are in the file without
 * parsing it. This will always be off as we target the max case were all lines are
 * as small as possible, so the real item count will be lower than this, but this
 * gives us a ballpark figure
 */
size_t estimate_total_entries(off_t filesize) {
    size_t data_bytes = filesize - JSON_OVERHEAD_SIZE;
    size_t estimated_max_items = ceil((float)((float)data_bytes / (float)JSON_MIN_ROW_SIZE));
    return estimated_max_items;
}

void usage(void) {
    fprintf(stderr, "Data Generator Binary Reader:\n");
    fprintf(stderr, "-h            This help dialog.\n");
    fprintf(stderr, "-i <bin_file> Path to binary file.\n");
    fprintf(stderr, "-p            Estimate Max Item count and preallocate a memory for all items,\n");
    fprintf(stderr, "              default is to malloc each item and use a linked list.\n");
    fprintf(stderr, "-v            Enable Verbose Output.\n");
}

int main (int argc, char *argv[]) {
    int opt;
    bool preallocate_entries = false;
    char *input_file = NULL;
    FILE *json_fp = NULL;
    int ret;
    struct stat statbuf = {};
    uint64_t program_start = GET_CPU_TICKS();

    while( (opt = getopt(argc, argv, "hi:p")) != -1) {
        switch (opt) {
            case 'h':
                usage();
                exit(0);
                break;
            
            case 'i':
                input_file = strdup(optarg);
                break;
            
            case 'p':
                preallocate_entries = true;
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
    ret = stat(input_file, &statbuf);
    if (ret != 0) {
        ERROR("Unable to get filestats[%d][%s]\n", errno, strerror(errno));
    }
    printf("Using input_file              [%s]\n", input_file);
    printf("PreAllocating entries         [%s]\n", preallocate_entries ? "True" : "False");
    printf("  File Size                   [%lu] bytes\n", statbuf.st_size);
    printf("  IO Block Size               [%lu] bytes\n", statbuf.st_blksize);
    printf("  Allocated 512B Blocks       [%lu]\n", statbuf.st_blocks);
    size_t item_estimate = estimate_total_entries(statbuf.st_size);
    printf("  Estimated Items             [%lu]\n", item_estimate);
    printf("\n\n");

    if (preallocate_entries) {
        data_array = malloc(item_estimate * sizeof(struct data_item_s));
        if (!data_array) {
            ERROR("Failed to malloc [%lu]bytes for data array\n", item_estimate * sizeof(struct data_item_s));
        }
    }

    uint64_t program_init = GET_CPU_TICKS();

    json_fp = fopen(input_file, "r");
    if (!json_fp) {
        ERROR("Failed to open file [%d][%s]\n", errno, strerror(errno));
    }

    printf("Start Parsing File\n");
    printf("------------------\n");

    parse_file(json_fp, preallocate_entries);

    if (feof(json_fp)==1) {
        printf("-------------------\n");
        printf("Reached End of File\n\n");
    }

    fclose(json_fp);

    uint64_t program_parse_done = GET_CPU_TICKS();

    calculate_haversine_average(preallocate_entries);

    uint64_t program_end = GET_CPU_TICKS();

    uint64_t program_elapsed = program_end - program_start;

    uint64_t program_elapsed_init = program_init - program_start;
    uint64_t program_elapsed_parse = program_parse_done - program_init;
    uint64_t program_elapsed_haversine = program_end - program_parse_done;

    printf("   Progran Runtime %lu ticks\t(100%%)\t\t[%lu]ms\t\tCPU Freq: %lu\n",
        program_elapsed,
        get_ms_from_cpu_ticks(program_elapsed),
        guess_cpu_freq(100));

    printf("              Init %lu ticks\t\t(%03.2f%%)\t\t[%lu]ms\n",
        program_elapsed_init,
        ((float)program_elapsed_init/(float)program_elapsed)*100,
        get_ms_from_cpu_ticks(program_elapsed_init));

    printf("        Parse File %lu ticks\t(%03.2f%%)\t[%lu]ms\n",
        program_elapsed_parse,
        ((float)program_elapsed_parse/(float)program_elapsed)*100,
        get_ms_from_cpu_ticks(program_elapsed_parse));

    printf("   Parse Haversine %lu ticks\t(%03.2f%%)\t\t[%lu]ms\n",
        program_elapsed_haversine,
        ((float)program_elapsed_haversine/(float)program_elapsed)*100,
        get_ms_from_cpu_ticks(program_elapsed_haversine));

    printf("\n\n");
    printf("=============================\n");
    printf("JSON Data Parser Completed OK\n");
    printf("=============================\n");

    return 0;
}
