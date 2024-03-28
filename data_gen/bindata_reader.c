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

#include "haversine.h"

#define MY_ERROR(...) {                    \
        fprintf(stderr, "[%d]", __LINE__);      \
        fprintf(stderr, __VA_ARGS__);   \
        exit(1);                        \
    }

#define APPROX_EARTH_RADIUS     6372.8

void usage(void) {
    fprintf(stderr, "Data Generator Binary Reader:\n");
    fprintf(stderr, "-h            This help dialog.\n");
    fprintf(stderr, "-i <bin_file> Path to binary file.\n");
}


int main (int argc, char *argv[]) {
    int opt;
    char *input_file = NULL;
    FILE *binary_fp = NULL;
    uint32_t count = 0;
    int ret;
    uint32_t binary_write_count = 0;
    double X0, Y0, X1, Y1 = 0;
    double H_DIST, H_DIST_CALC, sum = 0;
    double delta = 0;

#ifdef _WIN32
    for (int index=1; index<argc; ++index) {
        if (strcmp(argv[index], "-h")==0) {
            usage();
            exit(0);
        } else if (strcmp(argv[index], "-i")==0) {
            // must have at least index+2 arguments to contain a file
            if (argc<index+2) {
                printf("ERROR: missing input file parameter\n");
                usage();
                exit(1);
            }
            input_file = strdup(argv[index+1]);
            // since we consume the next parameter then skip it
            ++index;
        }
    }
#else
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

    printf("============================\n");
    printf("Data Generator Binary Reader\n");
    printf("============================\n");

    printf("Using input_file              [%s]\n", input_file);
    printf("\n\n");

    binary_fp = fopen(input_file, "r");
    if (!binary_fp) {
        MY_ERROR("Failed to open binary file [%d][%s]\n", errno, strerror(errno));
    }

    while (1) {
        ret = fread(&X0, sizeof(double), 1, binary_fp);
        if (ret!=1) break;
        ret = fread(&Y0, sizeof(double), 1, binary_fp);
        if (ret!=1) break;
        ret = fread(&X1, sizeof(double), 1, binary_fp);
        if (ret!=1) break;
        ret = fread(&Y1, sizeof(double), 1, binary_fp);
        if (ret!=1) break;
        ret = fread(&H_DIST, sizeof(double), 1, binary_fp);
        if (ret!=1) break;
        H_DIST_CALC = ReferenceHaversine(X0, Y0, X1, Y1, APPROX_EARTH_RADIUS);
        delta = H_DIST - H_DIST_CALC;
        // printf("x0:%3.16f, y0:%3.16f, x1:%3.16f, y1:%3.16f | h_dist:%3.16f | h_dist_calc:%3.16f | delta:%3.16f\n", X0, Y0, X1, Y1, H_DIST, H_DIST_CALC, delta);
        if (delta!=0) {
            MY_ERROR("Delta[%3.16f] for a row[%" PRIu32 "] is not zero\n", delta, count);
        }
        sum += H_DIST_CALC;
        binary_write_count += 5;
        count++;
    }

    printf("All read values match expected Haversine Distance\n\n");

   
    double average = sum/count;
    printf("binary_write_count     %" PRIu32 "\n", binary_write_count);
    printf("binary_bytes_writen    %zu\n", binary_write_count*sizeof(double));
    printf("sum H_DIST_CALC        %3.16f\n", sum);
    printf("average H_DIST_CALC    %3.16f\n\n", average);


    fclose(binary_fp);

    return 0;
}
