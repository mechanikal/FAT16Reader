//
// Created by root on 5/19/26.
//

#ifndef FAT16READER_DISC_TOOLS_H
#define FAT16READER_DISC_TOOLS_H

#include <stdio.h>
#include <stdint.h>

struct disk_t{
    size_t size;
    FILE *memory;
};

int disk_read(struct disk_t* pdisk, int32_t first_sector, void* buffer, int32_t sectors_to_read);
struct disk_t* disk_open_from_file(const char* volume_file_name);
int disk_close(struct disk_t* pdisk);

#endif //FAT16READER_DISC_TOOLS_H
