#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <getopt.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>

#include "haversine.h"

/*
 * X -180 to 180 degrees
 * Y -90 to 90 degrees
 */

#define ERROR(...) {                    \
        fprintf(stderr, __VA_ARGS__);   \
        exit(1);                        \
    }

#define MAX_FILENAME_LEN        512
#define MAX_TIMESTAMP_LEN       64
#define X_RADIUS                180
#define Y_RADIUS                90
#define APPROX_EARTH_RADIUS     6372.8

double pos_min = 0;
double pos_max = 0;
double neg_min = 0;
double neg_max = 0;


double get_random_data(double radius) {
    int rnum;
    rnum =  rand();
    double fnum = ((double)(rnum-RAND_MAX/2)/RAND_MAX)*2*radius;
    if (fnum>0) {
        if (fnum>pos_max) {
            pos_max = fnum;
        }
        if (pos_min==0 && fnum!=0)  {
            pos_min = fnum;
        } else if (fnum<pos_min) {
            pos_min = fnum;
        }
    } else {
        if (fnum<neg_max) {
            neg_max = fnum;
        }
        if (neg_min==0 && fnum!=0) {
            neg_min = fnum;
        } else if (fnum>neg_min) {
            neg_min = fnum;
        }
    }
    return fnum;
}


void usage(void) {
    fprintf(stderr, "Data Generator Usage:\n");
    fprintf(stderr, "-h         This help dialog.\n");
    fprintf(stderr, "-n <count> Generate count data points.\n");
    fprintf(stderr, "-s <seed>  Set the Seed.\n");
}


int main (int argc, char *argv[]) {
    int opt;
    char json_outfile[MAX_FILENAME_LEN] = {};
    char binary_outfile[MAX_FILENAME_LEN] = {};
    char stats_outfile[MAX_FILENAME_LEN] = {};
    char timestamp[MAX_TIMESTAMP_LEN] = {};
    FILE *json_fp = NULL;
    FILE *binary_fp = NULL;
    FILE *stats_fp = NULL;
    uint64_t count = 0;
    unsigned int seed = 0;
    time_t temp;
    struct tm *timeptr;
    int ret;
    uint64_t binary_write_count = 0;

    while( (opt = getopt(argc, argv, "hn:s:")) != -1) {
        switch (opt) {
            case 'h':
                usage();
                exit(0);
                break;
            
            case 'n':
                count = strtoull(optarg, NULL, 0);
                break;

            case 's':
                seed = strtoul(optarg, NULL, 0);
                break;

            default:
                fprintf(stderr, "ERROR Invalid command line option\n");
                usage();
                exit(1);
                break;
        }
    }

    if (count==0) {
        usage();
        exit(1);
    }
    if (seed==0) {
        usage();
        exit(1);
    }

    printf("==============\n");
    printf("Data Generator\n");
    printf("==============\n");

    temp = time(NULL);
    timeptr = localtime(&temp);
    // ret = strftime((char *)&timestamp, MAX_TIMESTAMP_LEN,"%Y%m%d_%H%M%S", timeptr);
    ret = strftime((char *)&timestamp, MAX_TIMESTAMP_LEN,"%Y%m%d", timeptr);
    if (ret>=MAX_TIMESTAMP_LEN) {
       ERROR("Timestamp overflow\n");
    }

    ret = snprintf((char *)&json_outfile, MAX_FILENAME_LEN, "datagen_seed_%u_count_%lu_timestamp_%s.json", seed, count, timestamp);
    if (ret>=MAX_FILENAME_LEN) {
       ERROR("Json filename overflow\n");
    }

    ret = snprintf((char *)&binary_outfile, MAX_FILENAME_LEN, "datagen_seed_%u_count_%lu_timestamp_%s.bin", seed, count, timestamp);
    if (ret>=MAX_FILENAME_LEN) {
       ERROR("Json filename overflow\n");
    }

    ret = snprintf((char *)&stats_outfile, MAX_FILENAME_LEN, "datagen_seed_%u_count_%lu_timestamp_%s.txt", seed, count, timestamp);
    if (ret>=MAX_FILENAME_LEN) {
       ERROR("Json filename overflow\n");
    }

    printf("Using Count              [%lu]\n", count);
    printf("Using Seed               [%u]\n", seed);
    printf("Using RAND_MAX           [%u]\n", RAND_MAX);
    printf("Using timestamp          [%s]\n", timestamp);
    printf("Using json_outfile       [%s]\n", json_outfile);
    printf("Using binary_outfile     [%s]\n", binary_outfile);
    printf("Using stats_outfile      [%s]\n", stats_outfile);
    printf("\n\n");

    json_fp = fopen(json_outfile, "w");
    if (!json_fp) {
        ERROR("Failed to open json file [%d][%s]\n", errno, strerror(errno));
    }
    binary_fp = fopen(binary_outfile, "w");
    if (!binary_fp) {
        ERROR("Failed to open binary file [%d][%s]\n", errno, strerror(errno));
    }
    stats_fp = fopen(stats_outfile, "w");
    if (!stats_fp) {
        ERROR("Failed to open stats file [%d][%s]\n", errno, strerror(errno));
    }

    /* start json structure */
    fprintf(json_fp, "{\"pairs\":[\n");

    srand(seed);

    double sum = 0;
    bool first_line = true;
    for (uint64_t i=0; i<count; i++) {
        double X0, Y0, X1, Y1;
        double H_DIST;
        X0 = get_random_data(X_RADIUS);
        Y0 = get_random_data(Y_RADIUS);
        X1 = get_random_data(X_RADIUS);
        Y1 = get_random_data(Y_RADIUS);
        H_DIST = ReferenceHaversine(X0, Y0, X1, Y1, APPROX_EARTH_RADIUS);
        sum += H_DIST;

        /*
        {"x0":102.1633205722960440, "y0":-24.9977499718717624, "x1":-14.3322557404258362, "y1":62.6708294856625940},
        */
        if (!first_line) fprintf(json_fp, ",\n");
        fprintf(json_fp, "{\"x0\":%3.16f, \"y0\":%3.16f, \"x1\":%3.16f, \"y1\":%3.16f}", X0, Y0, X1, Y1);
        fwrite(&X0, sizeof(double), 1, binary_fp);
        fwrite(&Y0, sizeof(double), 1, binary_fp);
        fwrite(&X1, sizeof(double), 1, binary_fp);
        fwrite(&Y1, sizeof(double), 1, binary_fp);
        fwrite(&H_DIST, sizeof(double), 1, binary_fp);
        binary_write_count += 5;
        first_line = false;

    }
    /* close out json structure */
    fprintf(json_fp, "\n]}\n");

    printf("\n\n");
    printf("pos_min     %3.16f\n", pos_min);
    printf("pos_max     %3.16f\n", pos_max);
    printf("neg_min     %3.16f\n", neg_min);
    printf("neg_max     %3.16f\n", neg_max);
    printf("\n\n");

    fprintf(stats_fp,"pos_min     %3.16f\n", pos_min);
    fprintf(stats_fp,"pos_max     %3.16f\n", pos_max);
    fprintf(stats_fp,"neg_min     %3.16f\n", neg_min);
    fprintf(stats_fp,"neg_max     %3.16f\n", neg_max);

    
    double average = sum/count;
    printf("binary_write_count     %lu\n", binary_write_count);
    printf("binary_bytes_writen    %lu\n", binary_write_count*sizeof(double));
    printf("sum H_DIST             %3.16f\n", sum);
    printf("average H_DIST         %3.16f\n\n", average);
    fprintf(stats_fp,"binary_write_count     %lu\n", binary_write_count);
    fprintf(stats_fp,"binary_bytes_writen    %lu\n", binary_write_count*sizeof(double));
    fprintf(stats_fp, "sum H_DIST            %3.16f\n", sum);
    fprintf(stats_fp, "average H_DIST        %3.16f\n\n", average);


    fclose(json_fp);
    fclose(binary_fp);
    fclose(stats_fp);

    return 0;
}
