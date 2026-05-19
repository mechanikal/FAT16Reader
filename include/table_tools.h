#ifndef FAT16READER_TABLE_TOOLS_H
#define FAT16READER_TABLE_TOOLS_H

#include "include/cluster_tools.h"

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

struct volume_t* fat_open(struct disk_t* pdisk, uint32_t first_sector);
int fat_close(struct volume_t* pvolume);

#endif //FAT16READER_TABLE_TOOLS_H
