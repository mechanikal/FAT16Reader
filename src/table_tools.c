#include "include/disc_tools.h"
#include "include/table_tools.h"
#include "internal.h"
#include <stdint.h>
#include <errno.h>
#include <stdlib.h>
#include <memory.h>

// read file allocation table from pdisk
struct volume_t* fat_open(struct disk_t* pdisk, uint32_t first_sector){
    struct volume_t* volume;
    char sector[BLOCK_SIZE];
    char * fat_system_label;
    struct boot_sector *bs;
    struct table file_allocation_table1;
    struct table file_allocation_table2;
    if(disk_read(pdisk,(int32_t)first_sector,sector,1) != 0){
        return NULL;
    }
    if(*(uint16_t *)(sector + 510)!=BOOT_SECTOR_SIGNATURE){
        errno = EFAULT;
        return NULL;
    }
    volume = malloc(sizeof(struct volume_t));
    if(volume == NULL){
        errno = ENOMEM;
        return NULL;
    }
    bs = (struct boot_sector*)sector;
    fat_system_label = (char*)bs->fat_type_label;
    volume->total_sectors = bs->total_sectors_16;
    volume->fat_size = bs->table_size_16;
    volume->root_directory_size = ((bs->root_entry_count*32) + (bs->bytes_per_sector -1)) /bs->bytes_per_sector;
    volume->first_data_sector = bs->reserved_sector_count + (bs->table_count * bs->table_size_16) + volume->root_directory_size;
    volume->first_fat_sector = bs->reserved_sector_count;
    volume->total_data_sectors = volume->total_sectors - (bs->reserved_sector_count + (bs->table_count * volume->fat_size) + volume->root_directory_size);
    volume->total_cluster = volume->total_data_sectors/ bs->sectors_per_cluster;
    volume->disk = pdisk;
    volume->bs=*bs;
    if(memcmp(fat_system_label,"FAT16   ",8) == 0){
        volume->type = FAT16;
    } else if(memcmp(fat_system_label,"FAT12   ", 8) == 0){
        volume->type = FAT12;
    }else{
        free(volume);
        errno = EINVAL;
        return NULL;
    }
    file_allocation_table1.data = malloc(volume->bs.bytes_per_sector*volume->bs.table_size_16);
    if(file_allocation_table1.data == NULL){
        free(volume);
        errno = ENOMEM;
        return NULL;
    }
    file_allocation_table2.data = malloc(volume->bs.bytes_per_sector*volume->bs.table_size_16);
    if(file_allocation_table2.data == NULL){
        free(volume);
        free(file_allocation_table1.data);
        errno = ENOMEM;
        return NULL;
    }
    if(disk_read(pdisk,volume->bs.reserved_sector_count+volume->bs.table_size_16,file_allocation_table1.data,volume->bs.table_size_16)){
        free(file_allocation_table1.data);
        free(file_allocation_table2.data);
        free(volume);
        return NULL;
    }
    if(disk_read(pdisk,volume->bs.reserved_sector_count,file_allocation_table2.data,volume->bs.table_size_16)){
        free(file_allocation_table1.data);
        free(file_allocation_table2.data);
        free(volume);
        return NULL;
    }
    if(memcmp(file_allocation_table1.data,file_allocation_table2.data,volume->bs.table_size_16*volume->bs.bytes_per_sector)!=0){
        free(file_allocation_table1.data);
        free(file_allocation_table2.data);
        free(volume);
        errno = EINVAL;
        return NULL;
    }
    free(file_allocation_table2.data);
    volume->file_allocation_table.data = file_allocation_table1.data;
    volume->directory= malloc(volume->bs.bytes_per_sector*volume->root_directory_size);
    if(volume->directory==NULL){
        free(file_allocation_table1.data);
        free(volume);
        errno = ENOMEM;
        return NULL;
    }
    if(disk_read(pdisk,(int)(volume->bs.reserved_sector_count + (volume->bs.table_count*volume->bs.table_size_16)),volume->directory,volume->root_directory_size)){
        free(volume->directory);
        free(file_allocation_table1.data);
        free(volume);
        return NULL;
    }
    return volume;
}

int fat_close(struct volume_t* pvolume){
    if(pvolume == NULL){
        errno = EFAULT;
        return 1;
    }
    free(pvolume->file_allocation_table.data);
    free(pvolume->directory);
    free(pvolume);
    pvolume = NULL;
    return 0;
}
