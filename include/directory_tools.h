//
// Created by root on 5/19/26.
//

#ifndef FAT16READER_DIRECTORY_TOOLS_H
#define FAT16READER_DIRECTORY_TOOLS_H

#include <stdbool.h>
#include <stdint.h>

#define DAY_MASK 31
#define MONTH_MASK 480
#define YEAR_MASK 65024
#define SEC_MASK 31
#define MIN_MASK 2016
#define HOUR_MASK 63488

struct dir_t{
    char *data;
    size_t entries;
    size_t entry_size;
    size_t offset;
};

struct dir_entry_t{
    char name[13];
    uint16_t first_cluster;
    size_t size;
    bool is_archived;
    bool is_readonly;
    bool is_system;
    bool is_hidden;
    bool is_directory;
    struct {
        uint8_t day;
        uint8_t month;
        uint16_t year;
    }creation_date;
    struct {
        uint8_t hour;
        uint8_t minute;
        uint8_t second;
    }creation_time;
};

int read_entry(struct dir_entry_t *entry, char * buffer, size_t buffer_size, size_t *offset);

#endif //FAT16READER_DIRECTORY_TOOLS_H
