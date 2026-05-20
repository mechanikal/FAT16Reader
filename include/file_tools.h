#ifndef FILE_TOOLS_H
#define FILE_TOOLS_H

#include "include/table_tools.h"
#include "include/directory_tools.h"
#include <stdlib.h>

struct file_t{
    size_t size;
    char * set;
    char * end;
    char * cur;
    struct cluster_chain_t * chain;
    char * data;
};

struct file_t* file_open(struct volume_t* pvolume, const char* file_name, struct dir_t directory);
size_t file_read(void *ptr, size_t size, size_t nmemb, struct file_t *stream);
int32_t file_seek(struct file_t* stream, int32_t offset, int whence);
int file_close(struct file_t* stream);

#endif //FILE_TOOLS_H
