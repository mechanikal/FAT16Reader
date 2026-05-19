//
// Created by root on 3/26/25.
//
#ifndef SO2_1_FILE_READER_H
#define SO2_1_FILE_READER_H

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <inttypes.h>
#include <stdbool.h>

#define BLOCK_SIZE 512
#define DAY_MASK 31
#define MONTH_MASK 480
#define YEAR_MASK 65024
#define SEC_MASK 31
#define MIN_MASK 2016
#define HOUR_MASK 63488
#define LAST_CLUSTER16 0xFFF8

//structures
struct clusters_chain_t {
    uint16_t *clusters;
    size_t size;
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
struct disk_t{
    size_t size;
    FILE *memory;
};
struct table{
    char *data;
};
enum fat_type{
    FAT32,
    FAT16,
    FAT12,
};
struct boot_sector {
    uint8_t bootjmp[3];
    uint8_t oem_name[8];
    uint16_t bytes_per_sector;
    uint8_t sectors_per_cluster;
    uint16_t reserved_sector_count;
    uint8_t table_count;
    uint16_t root_entry_count;
    uint16_t total_sectors_16;
    uint8_t media_type;
    uint16_t table_size_16;
    uint16_t sectors_per_track;
    uint16_t head_side_count;
    uint32_t hidden_sector_count;
    uint32_t total_sectors_32;
    //--------------------------------
    uint8_t bios_drive_num;
    uint8_t reserved1;
    uint8_t boot_signature;
    uint32_t volume_id;
    uint8_t volume_label[11];
    uint8_t fat_type_label[8];
}__attribute__((packed));
struct volume_t{
    struct boot_sector bs;
    size_t total_sectors;
    size_t fat_size;
    size_t root_directory_size;
    uint32_t first_data_sector;
    uint32_t first_fat_sector;
    size_t total_data_sectors;
    size_t total_cluster;
    enum fat_type type;
    struct disk_t *disk;
    struct table file_allocation_table;
    char * directory;
    struct clusters_chain_t cluster_chain;
};
struct file_t{
    size_t size;
    char * set;
    char * end;
    char * cur;
    struct clusters_chain_t * chain;
    char * data;
};
struct dir_t{
    char *data;
    size_t entries;
    size_t entry_size;
    size_t offset;
};
//cluster chain reader
struct clusters_chain_t * cluster_innit();
void cluster_free(struct clusters_chain_t * clustersChain);
int add_cluster(struct clusters_chain_t * clustersChain, uint16_t cluster);
uint16_t read_cluster16(uint16_t previous,const void * const buffer);
struct clusters_chain_t *get_chain_fat16(const void * const buffer, size_t size, uint16_t first_cluster);
//read directory
int read_entry(struct dir_entry_t *entry, char * buffer, size_t buffer_size, size_t *offset);
//disk management
struct disk_t* disk_open_from_file(const char* volume_file_name);
int disk_read(struct disk_t* pdisk, int32_t first_sector, void* buffer, int32_t sectors_to_read);
int disk_close(struct disk_t* pdisk);
//fat management
struct volume_t* fat_open(struct disk_t* pdisk, uint32_t first_sector);
int fat_close(struct volume_t* pvolume);
//file management
struct file_t* file_open(struct volume_t* pvolume, const char* file_name);
int file_close(struct file_t* stream);
size_t file_read(void *ptr, size_t size, size_t nmemb, struct file_t *stream);
int32_t file_seek(struct file_t* stream, int32_t offset, int whence);
//dir manager
struct dir_t* dir_open(struct volume_t* pvolume, const char* dir_path);
int dir_read(struct dir_t* pdir, struct dir_entry_t* pentry);
int dir_close(struct dir_t* pdir);

#endif //SO2_1_FILE_READER_H
