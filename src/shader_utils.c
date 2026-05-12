#include "ZorpEngine/shader_utils.h"

#include <stdio.h>
#include <stdlib.h>


char *read_shader_file(const char *filepath, size_t *file_size) {
    // Open the given file.
    FILE *fp = fopen(filepath, "rb");
    if (fp == NULL) {
        return NULL;
    }

    // Get the length of the file.
    fseek(fp, 0, SEEK_END);
    *file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    // Read the file into the buffer.
    char *buf = malloc(*file_size);
    fread(buf, 1, *file_size, fp);
    fclose(fp);

    return buf;
}

