#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <stdbool.h>
#ifndef _WIN32
#include <getopt.h>
#include <unistd.h>
#endif
#include <errno.h>
#include <math.h>
#define __USE_UNIX98
#include <sys/types.h>
#include <sys/stat.h>

#include "haversine.h"
#include "rdtsc_utils.h"

#define MY_ERROR(...) {                    \
        fprintf(stderr, "[%s:%d]", __FUNCTION__, __LINE__);      \
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

enum profile_blocks_e {
    BLOCK_INIT,
    BLOCK_FILE_READ,
    BLOCK_FREAD,
    BLOCK_PARSE_FILE,
    BLOCK_PARSE_DATA_FILE,
    BLOCK_HAVERSINE,
};

size_t file_size = 0;
uint8_t *file_data = NULL;

uint8_t *file_data_ptr = NULL;
size_t file_data_remaining = 0;

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

bool advance_until(char target) {
    uint32_t bytes_consumed = 0;
    bool found = false;
    while(!found && file_data_remaining>0) {
        bytes_consumed++;
        file_data_remaining--;
        if ((char)*file_data_ptr==target) {
            // found target and we have already consumed
            LOG("Found [%c] after [%" PRIu32 "] bytes read\n", target, bytes_consumed);
            found = true;
        }
        file_data_ptr++;
    }
    return found;
}

bool find_string(char *string) {
    bool found_it = false;
    int len = strlen(string);
    LOG("Looking for [%s] len[%d]\n", string, len);
    char *ptr = string;
    for (int i=0; i<len; i++) {
        found_it = advance_until(*ptr);
        if (!found_it) return false;
        ptr++;
    }
    // we found all elements
    return true;
}

// similar to advance_until but it will extract the consumed chars to a
// given buffer, without the last target char
bool extract_until(char* buffer, int max_buffer_len, char *target) {
    uint32_t bytes_consumed = 0;
    bool found = false;
    // clear buffer so that it is null terminated
    memset(buffer, 0, max_buffer_len);
    char *ptr = buffer;
    while(!found && file_data_remaining>0) {
        bytes_consumed++;
        file_data_remaining--;
        if ((char)*file_data_ptr==*target) {
            // found target and we have already consumed
            LOG("Found [%c] after [%" PRIu32 "] bytes read buffer[%s]\n", *target, bytes_consumed, buffer);
            found = true;
        } else {
            if (bytes_consumed>max_buffer_len) {
                MY_ERROR("Overran the target buffer\n");
                break;
            }
            // copy read char to buffer
            *ptr = *file_data_ptr;
            ptr++;
        }
        file_data_ptr++;
    }
    return false;
}

void parse_file(bool preallocate_entries) {
    double X0, Y0, X1, Y1 = 0;
    bool found_it;
    int ret;

    // first hit the start of json
    found_it = advance_until('{');
    if (!found_it) MY_ERROR("Failed to find start of json\n");

    // there may be other json items, we want to find "pairs"
    found_it = find_string("\"pairs\"");
    if (!found_it) MY_ERROR("Failed to find pairs item\n");
    found_it = advance_until(':');
    if (!found_it) MY_ERROR("Failed to find item separator\n");
    found_it = advance_until('[');    // start of json array
    if (!found_it) MY_ERROR("Failed to find start of json array\n");

    while(file_data_remaining>0) {
        uint8_t found_item = 0;
        uint8_t item_count = 0;
        double value;
        bool found_x0;
        bool found_y0;
        bool found_x1;
        bool found_y1;

        // parse array row
        found_it = advance_until('{');    // start of row item
        if (!found_it) break;

        while (item_count<4) {
            found_it = advance_until('"');    // Start of item id
            if (!found_it) break;
            // next we have the id of a value, can be x0, y0, x1, y1 and then the end '"' if the id
            // extract into item_name_buffer until we find the end of the identifier
            extract_until(item_name_buffer, MAX_ITEM_NAME_LEN, "\"");

            if (strncmp("x0",item_name_buffer,MAX_ITEM_NAME_LEN)==0) {
                found_item = FOUND_X0;
            } else if (strncmp("y0",item_name_buffer,MAX_ITEM_NAME_LEN)==0) {
                found_item = FOUND_Y0;
            } else if (strncmp("x1",item_name_buffer,MAX_ITEM_NAME_LEN)==0) {
                found_item = FOUND_X1;
            } else if (strncmp("y1",item_name_buffer,MAX_ITEM_NAME_LEN)==0) {
                found_item = FOUND_Y1;
            } else {
                MY_ERROR("Failed to identify item[%s]\n", item_name_buffer);
            }
            found_it = advance_until(':');    // start of value
            if (!found_it) break;

            if (item_count<3) {
                // first 3 values end with ','
                extract_until(item_value_buffer, MAX_VALUE_LEN, ",");
            } else {
                // the last item does not end with ',' instead we hit the end of the array item '}'
                extract_until(item_value_buffer, MAX_VALUE_LEN, "}");
            }
            
            ret = sscanf((char *)&item_value_buffer, "%lf", &value);
            if (ret!=1) {
                MY_ERROR("Failed to parse value from [%s]\n", item_value_buffer);
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
                    MY_ERROR("Invalid found_item[%d]\n", found_item);
                    break;
            }
            item_count++;
        }

        if (!found_x0 || !found_y0 || !found_x1 || !found_y1) {
            MY_ERROR("Invalid row format, failed to find all items\n");
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
                MY_ERROR("Failed to allocate [%zu]bytes for item", sizeof(struct list_item_s));
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
    LOG("Total Parsed data items [%zu]\n", data_item_count);
}

void calculate_haversine_average(bool preallocate_entries) {
    double H_DIST = 0;
    double sum = 0;
    double average = 0;
    uint32_t count_values = 0;

    TAG_DATA_BLOCK_START(BLOCK_HAVERSINE, "Haversine", data_item_count*sizeof(struct data_item_s));

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
            MY_ERROR("Sanity check failed for linked list");
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

    printf("Count                 %" PRIu32 " items\n", count_values);
    printf("sum H_DIST            %3.16f\n", sum);
    printf("average H_DIST        %3.16f\n\n", average);

    TAG_BLOCK_END(BLOCK_HAVERSINE);
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

size_t read_file_to_memory(char *filename, size_t size) {
    FILE *json_fp = NULL;

    TAG_DATA_BLOCK_START(BLOCK_FILE_READ, "FileReadToMemory", size);

    file_size = size;
    file_data = (uint8_t *)malloc(file_size);
    if (!file_data) {
        MY_ERROR("Malloc failed for size[%zu]\n", file_size);
    }

    file_data_ptr = file_data;
    file_data_remaining = size;

#ifdef _WIN32
    json_fp = fopen(filename, "rb");
#else
    json_fp = fopen(filename, "r");
#endif
    if (!json_fp) {
        MY_ERROR("Failed to open file [%d][%s]\n", errno, strerror(errno));
    }

    TAG_DATA_BLOCK_START(BLOCK_FREAD, "fread", size);

    size_t items_read = fread((void *)file_data, file_size, 1, json_fp);

    TAG_BLOCK_END(BLOCK_FREAD);

    if (items_read != 1) {
        MY_ERROR("Failed to read expected items [1] instead read[%zu]\n", items_read);
    }

    fclose(json_fp);

    TAG_BLOCK_END(BLOCK_FILE_READ);

    return file_size;
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
    int ret;
    struct stat statbuf = {};
    TAG_PROGRAM_START();

    TAG_BLOCK_START(BLOCK_INIT, "Init");

#ifdef _WIN32
    // fprintf(stderr, "Data Generator Binary Reader:\n");
    // fprintf(stderr, "-h            This help dialog.\n");
    // fprintf(stderr, "-i <bin_file> Path to binary file.\n");
    // fprintf(stderr, "-p            Estimate Max Item count and preallocate a memory for all items,\n");
    // fprintf(stderr, "              default is to malloc each item and use a linked list.\n");
    // fprintf(stderr, "-v            Enable Verbose Output.\n");
    for (int index=1; index<argc; ++index) {
        if (strcmp(argv[index], "-h")==0) {
            usage();
            exit(0);
        } else if (strcmp(argv[index], "-i")==0) {
            // must have at least index+2 arguments to contain a file
            if (argc<index+2) {
                printf("ERROR: missing Number of Data Points to generate\n");
                usage();
                exit(1);
            }
            input_file = strdup(argv[index+1]);
            // since we consume the next parameter then skip it
            ++index;
        } else if (strcmp(argv[index], "-p")==0) {
            preallocate_entries = true;
        } else if (strcmp(argv[index], "-v")==0) {
            verbose = true;
        }
    }
#else
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
                fprintf(stderr, "MY_ERROR Invalid command line option\n");
                usage();
                exit(1);
                break;
        }
    }
#endif

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
        MY_ERROR("Unable to get filestats[%d][%s]\n", errno, strerror(errno));
    }
    printf("PreAllocating entries         [%s]\n", preallocate_entries ? "True" : "False");
    printf("  File Size                   [%lu] bytes\n", statbuf.st_size);
#ifndef _WIN32
    printf("  IO Block Size               [%lu] bytes\n", statbuf.st_blksize);
    printf("  Allocated 512B Blocks       [%lu]\n", statbuf.st_blocks);
#endif
    size_t item_estimate = estimate_total_entries(statbuf.st_size);
    printf("  Estimated Items             [%zu]\n", item_estimate);
    printf("\n\n");

    if (preallocate_entries) {
        data_array = malloc(item_estimate * sizeof(struct data_item_s));
        if (!data_array) {
            MY_ERROR("Failed to malloc [%zu]bytes for data array\n", item_estimate * sizeof(struct data_item_s));
        }
    }

    TAG_BLOCK_END(BLOCK_INIT);

    size_t bytes_read = read_file_to_memory(input_file, statbuf.st_size);
    if (bytes_read != statbuf.st_size) {
        MY_ERROR("Failed to read expected bytes\n");
    }
    printf("Read %zu bytes\n", statbuf.st_size);

    printf("Start Parsing File\n");
    printf("------------------\n");

    TAG_DATA_BLOCK_START(BLOCK_PARSE_DATA_FILE, "ParseFileData", (uint64_t)statbuf.st_size);
    parse_file(preallocate_entries);
    TAG_BLOCK_END(BLOCK_PARSE_DATA_FILE);

    calculate_haversine_average(preallocate_entries);

    TAG_PROGRAM_END();

    printf("\n\n");
    printf("=============================\n");
    printf("JSON Data Parser Completed OK\n");
    printf("=============================\n");

    return 0;
}
