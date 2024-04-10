#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdbool.h>
#ifdef _WIN32
#include <Windows.h>
#include <fcntl.h>
#include <io.h>
#else
#error "This Version is only for WIN32"
#endif
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "reptester.h"
#include "rdtsc_utils.h"

#define MY_ERROR(...) {                 \
        fprintf(stderr, __VA_ARGS__);   \
        exit(1);                        \
    }

struct test_context {
    char *name;
    char *filename;
    size_t filesize;
    uint8_t *buffer;
    FILE *fp;                   // used by fread
    int file_id;                // used by _read
    HANDLE file_handle;         // used by ReadFile
    uint64_t min_cpu_ticks;
    uint64_t max_cpu_ticks;
};

// ===================================================================================
// fread test
// ===================================================================================
void fread_env_setup(void *context) {
    struct test_context *ctx = (struct test_context *)context;

    printf("\nTest[%s] starting\n", ctx->name);

    ctx->fp = fopen(ctx->filename, "rb");
    if (!ctx->fp) {
        MY_ERROR("Failed to open file [%d][%s]\n", errno, strerror(errno));
    }
    // printf("[%s] File Opened OK\n", __FUNCTION__);
}

void fread_env_teardown(void *context) {
    struct test_context *ctx = (struct test_context *)context;
    // printf("[%s] name[%s]\n", __FUNCTION__, ctx->name);

    fclose(ctx->fp);
}

void fread_setup(void *context) {
    struct test_context *ctx = (struct test_context *)context;
    // reset file to start
    int ret = fseek(ctx->fp, 0, SEEK_SET);
    if (ret!=0) {
        MY_ERROR("Fseek Failed\n");
    }
}

void fread_teardown(void *context) {
    // struct test_context *ctx = (struct test_context *)context;
}

void fread_main(void *context) {
    bool new_new_line = false;
    struct test_context *ctx = (struct test_context *)context;

    // Measure Actual Process
    uint64_t start = GET_CPU_TICKS();
    size_t items_read = fread((void *)ctx->buffer, ctx->filesize, 1, ctx->fp);
    uint64_t elapsed_ticks = GET_CPU_TICKS() - start;

    if (items_read != 1) {
        MY_ERROR("Failed to read expected items [1] instead read[%zu] | ferror(%d)\n", items_read, ferror(ctx->fp));
    }

    if (ctx->min_cpu_ticks==0 || elapsed_ticks < ctx->min_cpu_ticks) {
        ctx->min_cpu_ticks = elapsed_ticks;
        printf(" | New MinTime");
        print_data_speed(ctx->filesize, elapsed_ticks);
        new_new_line = true;
    }
    if (elapsed_ticks > ctx->max_cpu_ticks) {
        ctx->max_cpu_ticks = elapsed_ticks;
        printf(" | New MaxTime");
        print_data_speed(ctx->filesize, elapsed_ticks);
        new_new_line = true;
    }

    if (new_new_line) {
        printf("\n");
    }
}

// ===================================================================================
// _read test
// ===================================================================================
void _read_env_setup(void *context) {
    struct test_context *ctx = (struct test_context *)context;
    printf("\nTest[%s] starting\n", ctx->name);
}

void _read_env_teardown(void *context) {
    // struct test_context *ctx = (struct test_context *)context;
}

void _read_setup(void *context) {
    struct test_context *ctx = (struct test_context *)context;

    ctx->file_id = _open(ctx->filename, _O_BINARY|_O_RDONLY);

    if (ctx->file_id == -1) {
        MY_ERROR("_open failed\n");
    }
}

void _read_teardown(void *context) {
    struct test_context *ctx = (struct test_context *)context;
    _close(ctx->file_id);
}


void _read_main(void *context) {
    bool new_new_line = false;
    struct test_context *ctx = (struct test_context *)context;

    // Measure Actual Process
    uint64_t start = GET_CPU_TICKS();
    int items_read = _read(ctx->file_id, (void *)ctx->buffer,  (unsigned int)ctx->filesize);
    uint64_t elapsed_ticks = GET_CPU_TICKS() - start;

    if (items_read != ctx->filesize) {
        MY_ERROR("Failed to read expected items [%zu] instead read[%d]\n", ctx->filesize, items_read);
    }

    if (ctx->min_cpu_ticks==0 || elapsed_ticks < ctx->min_cpu_ticks) {
        ctx->min_cpu_ticks = elapsed_ticks;
        printf(" | New MinTime");
        print_data_speed(ctx->filesize, elapsed_ticks);
        new_new_line = true;
    }
    if (elapsed_ticks > ctx->max_cpu_ticks) {
        ctx->max_cpu_ticks = elapsed_ticks;
        printf(" | New MaxTime");
        print_data_speed(ctx->filesize, elapsed_ticks);
        new_new_line = true;
    }

    if (new_new_line) {
        printf("\n");
    }
}

// ===================================================================================
// FileRead test
// ===================================================================================
void readfile_env_setup(void *context) {
    struct test_context *ctx = (struct test_context *)context;
    printf("\nTest[%s] starting\n", ctx->name);
}

void readfile_env_teardown(void *context) {
    // struct test_context *ctx = (struct test_context *)context;
}

void readfile_setup(void *context) {
    struct test_context *ctx = (struct test_context *)context;

    ctx->file_handle = CreateFileA(ctx->filename, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if(ctx->file_handle == INVALID_HANDLE_VALUE) {
        MY_ERROR("CreateFileA failed\n");
    }
}

void readfile_teardown(void *context) {
    struct test_context *ctx = (struct test_context *)context;
    CloseHandle(ctx->file_handle);
}


void readfile_main(void *context) {
    bool new_new_line = false;
    int bytes_read = 0;
    bool result;
    struct test_context *ctx = (struct test_context *)context;

    // Measure Actual Process
    uint64_t start = GET_CPU_TICKS();
    result = ReadFile(ctx->file_handle, ctx->buffer, ctx->filesize, &bytes_read, 0);
    uint64_t elapsed_ticks = GET_CPU_TICKS() - start;
    if (bytes_read != ctx->filesize) {
        MY_ERROR("Failed to read expected items [%zu] instead read[%d]\n", ctx->filesize, bytes_read);
    }

    if (ctx->min_cpu_ticks==0 || elapsed_ticks < ctx->min_cpu_ticks) {
        ctx->min_cpu_ticks = elapsed_ticks;
        printf(" | New MinTime");
        print_data_speed(ctx->filesize, elapsed_ticks);
        new_new_line = true;
    }
    if (elapsed_ticks > ctx->max_cpu_ticks) {
        ctx->max_cpu_ticks = elapsed_ticks;
        printf(" | New MaxTime");
        print_data_speed(ctx->filesize, elapsed_ticks);
        new_new_line = true;
    }

    if (new_new_line) {
        printf("\n");
    }
}

// ===================================================================================
// WriteBytes test
// ===================================================================================
void writebytes_env_setup(void *context) {
    struct test_context *ctx = (struct test_context *)context;
    printf("\nTest[%s] starting\n", ctx->name);
}

void writebytes_env_teardown(void *context) {
    // struct test_context *ctx = (struct test_context *)context;
}

void writebytes_setup(void *context) {
    // struct test_context *ctx = (struct test_context *)context;
}

void writebytes_teardown(void *context) {
    // struct test_context *ctx = (struct test_context *)context;
}

void writebytes_main(void *context) {
    bool new_new_line = false;
    int bytes_read = 0;
    bool result;
    struct test_context *ctx = (struct test_context *)context;

    // Measure Actual Process
    uint64_t start = GET_CPU_TICKS();

    for (uint64_t index=0; index<ctx->filesize; ++index) {
        ctx->buffer[index] = (uint8_t)index;
    }

    uint64_t elapsed_ticks = GET_CPU_TICKS() - start;

    if (ctx->min_cpu_ticks==0 || elapsed_ticks < ctx->min_cpu_ticks) {
        ctx->min_cpu_ticks = elapsed_ticks;
        printf(" | New MinTime");
        print_data_speed(ctx->filesize, elapsed_ticks);
        new_new_line = true;
    }
    if (elapsed_ticks > ctx->max_cpu_ticks) {
        ctx->max_cpu_ticks = elapsed_ticks;
        printf(" | New MaxTime");
        print_data_speed(ctx->filesize, elapsed_ticks);
        new_new_line = true;
    }

    if (new_new_line) {
        printf("\n");
    }
}

// ===================================================================================
// WriteLongBytes test
// ===================================================================================
void writelongbytes_env_setup(void *context) {
    struct test_context *ctx = (struct test_context *)context;
    printf("\nTest[%s] starting\n", ctx->name);
}

void writelongbytes_env_teardown(void *context) {
    // struct test_context *ctx = (struct test_context *)context;
}

void writelongbytes_setup(void *context) {
    // struct test_context *ctx = (struct test_context *)context;
}

void writelongbytes_teardown(void *context) {
    // struct test_context *ctx = (struct test_context *)context;
}

void writelongbytes_main(void *context) {
    bool new_new_line = false;
    int bytes_read = 0;
    bool result;
    struct test_context *ctx = (struct test_context *)context;

    // Measure Actual Process
    uint64_t start = GET_CPU_TICKS();

    uint64_t *ptr = (uint64_t *)ctx->buffer;
    for (uint64_t index=0; index< (ctx->filesize/sizeof(uint64_t)); ++index) {
        ptr[index] = index; 
    }

    uint64_t elapsed_ticks = GET_CPU_TICKS() - start;

    if (ctx->min_cpu_ticks==0 || elapsed_ticks < ctx->min_cpu_ticks) {
        ctx->min_cpu_ticks = elapsed_ticks;
        printf(" | New MinTime");
        print_data_speed(ctx->filesize, elapsed_ticks);
        new_new_line = true;
    }
    if (elapsed_ticks > ctx->max_cpu_ticks) {
        ctx->max_cpu_ticks = elapsed_ticks;
        printf(" | New MaxTime");
        print_data_speed(ctx->filesize, elapsed_ticks);
        new_new_line = true;
    }

    if (new_new_line) {
        printf("\n");
    }
}

// ===================================================================================
// ===================================================================================

void print_stats(void *context) {
    struct test_context *ctx = (struct test_context *)context;

    printf("Slowest Speed");
    print_data_speed(ctx->filesize, ctx->max_cpu_ticks);
    printf("\n");
    printf("Fastest Speed");
    print_data_speed(ctx->filesize, ctx->min_cpu_ticks);
    printf("\n");
}


void usage(void) {
    fprintf(stderr, "File Read RepTester Usage:\n");
    fprintf(stderr, "-h             This help dialog.\n");
    fprintf(stderr, "-i <filename>  Use <filename> as input.\n");
    fprintf(stderr, "-t <runtime>   Set runtime in seconds. (defaults to 10seconds)\n");
}

int main (int argc, char *argv[]) {
    int opt;
    char *filename = NULL;
    int runtime = 10;
    struct stat statbuf = {0};
    int ret;
    size_t filesize = 0;
    uint8_t *buffer = NULL;

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
            filename = strdup(argv[index+1]);
            // since we consume the next parameter then skip it
            ++index;
        } else if (strcmp(argv[index], "-t")==0) {
            // must have at least index+2 arguments to contain a file
            if (argc<index+2) {
                printf("ERROR: missing output file parameter\n");
                usage();
                exit(1);
            }
            runtime = atoi(argv[index+1]);
            // since we consume the next parameter then skip it
            ++index;
        }
    }

    if (!filename) {
        MY_ERROR("Must pass a filename\n");
    }

    printf("==========================\n");
    printf("File Read Repetition Tests\n");
    printf("==========================\n");

    printf("Using filename  [%s]\n", filename);
    printf("Using runtime   [%d]seconds\n", runtime);

    ret = stat(filename, &statbuf);
    if (ret != 0) {
        MY_ERROR("Unable to get filestats[%d][%s]\n", errno, strerror(errno));
    }

    filesize = statbuf.st_size;
    buffer = malloc(filesize);
    if (!buffer) {
         MY_ERROR("Malloc failed for size[%zu]\n", filesize);
    }

    // ===================================================================================
    // fread test
    // ===================================================================================
    struct test_context fread_context = {0};
    fread_context.name = "fread";
    fread_context.filename = filename;
    fread_context.filesize = filesize;
    fread_context.buffer = buffer;

    struct rep_tester_config fread_test = {0};
    fread_test.test_name = fread_context.name;
    fread_test.env_setup = fread_env_setup;
    fread_test.test_setup = fread_setup;
    fread_test.test_main = fread_main;
    fread_test.test_teardown = fread_teardown;
    fread_test.env_teardown = fread_env_teardown;
    fread_test.print_stats = print_stats;
    fread_test.test_runtime_seconds = runtime;

    // ===================================================================================
    // _read test
    // ===================================================================================
    struct test_context _read_context = {0};
    _read_context.name = "_read";
    _read_context.filename = filename;
    _read_context.filesize = filesize;
    _read_context.buffer = buffer;

    struct rep_tester_config _read_test = {0};
    _read_test.test_name = _read_context.name;
    _read_test.env_setup = _read_env_setup;
    _read_test.test_setup = _read_setup;
    _read_test.test_main = _read_main;
    _read_test.test_teardown = _read_teardown;
    _read_test.env_teardown = _read_env_teardown;
    _read_test.print_stats = print_stats;
    _read_test.test_runtime_seconds = runtime;

    // ===================================================================================
    // ReadFile test
    // ===================================================================================
    struct test_context readfile_context = {0};
    readfile_context.name = "ReadFile";
    readfile_context.filename = filename;
    readfile_context.filesize = filesize;
    readfile_context.buffer = buffer;

    struct rep_tester_config readfile_test = {0};
    readfile_test.test_name = readfile_context.name;
    readfile_test.env_setup = readfile_env_setup;
    readfile_test.test_setup = readfile_setup;
    readfile_test.test_main = readfile_main;
    readfile_test.test_teardown = readfile_teardown;
    readfile_test.env_teardown = readfile_env_teardown;
    readfile_test.print_stats = print_stats;
    readfile_test.test_runtime_seconds = runtime;

    // ===================================================================================
    // WriteBytes test
    // ===================================================================================
    struct test_context writebytes_context = {0};
    writebytes_context.name = "WriteBytes";
    writebytes_context.filename = filename;
    writebytes_context.filesize = filesize;
    writebytes_context.buffer = buffer;

    struct rep_tester_config writebytes_test = {0};
    writebytes_test.test_name = writebytes_context.name;
    writebytes_test.env_setup = writebytes_env_setup;
    writebytes_test.test_setup = writebytes_setup;
    writebytes_test.test_main = writebytes_main;
    writebytes_test.test_teardown = writebytes_teardown;
    writebytes_test.env_teardown = writebytes_env_teardown;
    writebytes_test.print_stats = print_stats;
    writebytes_test.test_runtime_seconds = runtime;

    // ===================================================================================
    // WriteLongBytes test
    // ===================================================================================
    struct test_context writelongbytes_context = {0};
    writelongbytes_context.name = "WriteLongBytes";
    writelongbytes_context.filename = filename;
    writelongbytes_context.filesize = filesize;
    writelongbytes_context.buffer = buffer;

    struct rep_tester_config writelongbytes_test = {0};
    writelongbytes_test.test_name = writelongbytes_context.name;
    writelongbytes_test.env_setup = writelongbytes_env_setup;
    writelongbytes_test.test_setup = writelongbytes_setup;
    writelongbytes_test.test_main = writelongbytes_main;
    writelongbytes_test.test_teardown = writelongbytes_teardown;
    writelongbytes_test.env_teardown = writelongbytes_env_teardown;
    writelongbytes_test.print_stats = print_stats;
    writelongbytes_test.test_runtime_seconds = runtime;

    // ===================================================================================
    // Run Tests
    // ===================================================================================

    rep_tester(&writebytes_test, &writebytes_context);
    rep_tester(&writelongbytes_test, &writelongbytes_context);
    rep_tester(&fread_test, &fread_context);
    rep_tester(&_read_test, &_read_context);
    rep_tester(&readfile_test, &readfile_context);

    // ===================================================================================

    printf("\n\n");
    printf("=========================\n");
    printf("Summary\n");
    printf("=========================\n\n");
    
    printf("Test [%s]\n", fread_context.name);
    print_stats(&fread_context);
    printf("\n");

    printf("Test [%s]\n", _read_context.name);
    print_stats(&_read_context);
    printf("\n");

    printf("Test [%s]\n", readfile_context.name);
    print_stats(&readfile_context);
    printf("\n");

    printf("Test [%s]\n", writebytes_context.name);
    print_stats(&writebytes_context);
    printf("\n");

    printf("Test [%s]\n", writelongbytes_context.name);
    print_stats(&writelongbytes_context);
    printf("\n");


    free(buffer);

    return 0;
}
