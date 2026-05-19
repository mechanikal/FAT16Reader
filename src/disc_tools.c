#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include "include/disc_tools.h"
#include "internal.h"

// open FAT 16 img file as a disc
struct disk_t* disk_open_from_file(const char* volume_file_name){
    FILE *memory;
    if(volume_file_name == NULL){
        errno = EFAULT;
        return NULL;
    }
    struct disk_t *disk;
    disk = malloc(sizeof(struct disk_t));
    if(disk == NULL){
        errno=ENOMEM;
        return NULL;
    }
    memory = fopen(volume_file_name, "rb");
    if(memory == NULL){
        errno = ENOENT;
        free(disk);
        return NULL;
    }
    disk->memory = memory;
    fseek(memory,0,SEEK_END);
    disk->size = ftell(memory);
    fseek(memory,0,SEEK_SET);
    return disk;
}

// read disc contents into pdisk
int disk_read(struct disk_t* pdisk, int32_t first_sector, void* buffer, int32_t sectors_to_read) {
    if (pdisk == NULL || buffer == NULL || sectors_to_read <= 0 || first_sector < 0 || pdisk->memory == NULL) {
        errno = EFAULT;
        return -1;
    }
    size_t size_test;
    size_test = first_sector * BLOCK_SIZE + sectors_to_read * BLOCK_SIZE;
    if (size_test > pdisk->size) {
        errno = ERANGE;
        return -1;
    }
    fseek(pdisk->memory, first_sector * BLOCK_SIZE, SEEK_SET);
    if(fread(buffer, BLOCK_SIZE, sectors_to_read, pdisk->memory)!=(size_t)sectors_to_read){
        errno = EINVAL;
        return -1;
    }
    return 0;
}

// deallocate disc memory
int disk_close(struct disk_t* pdisk){
    if(pdisk == NULL){
        errno = EFAULT;
        return -1;
    }
    fclose(pdisk->memory);
    free(pdisk);
    pdisk = NULL;
    return 0;
}
