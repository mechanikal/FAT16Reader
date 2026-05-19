//
// Created by root on 3/26/25.
//
#include "file_reader.h"

//cluster chain reader
struct clusters_chain_t * cluster_innit(){
    struct clusters_chain_t *clustersChain = malloc(sizeof(struct clusters_chain_t));
    if(clustersChain==NULL)return NULL;
    clustersChain->size=0;
    clustersChain->clusters=NULL;
    return clustersChain;
}
void cluster_free(struct clusters_chain_t * clustersChain){
    if(clustersChain->size != 0)
        free(clustersChain->clusters);
    clustersChain->clusters=NULL;
    clustersChain->size=0;
    free(clustersChain);
}
int add_cluster(struct clusters_chain_t * clustersChain, uint16_t cluster){
    uint16_t * temp = realloc(clustersChain->clusters,(clustersChain->size+1)* sizeof(uint16_t));
    if(temp==NULL)return 1;
    clustersChain->clusters=temp;
    *(clustersChain->clusters+clustersChain->size)=cluster;
    clustersChain->size ++;
    return 0;
}
uint16_t read_cluster16(uint16_t previous,const void * const buffer){
    const uint16_t * cluster_buffer = (const uint16_t*)buffer;
    return *(cluster_buffer + previous);
}
struct clusters_chain_t *get_chain_fat16(const void * const buffer, size_t size, uint16_t first_cluster){
    struct clusters_chain_t *clustersChain;
    if(buffer==NULL||size<=0)return NULL;
    clustersChain = cluster_innit();
    uint16_t cluster;
    cluster=first_cluster;
    if(clustersChain==NULL)return NULL;
    while (cluster<LAST_CLUSTER16&&cluster>=2){
        /*if(cluster>=2*size){
            cluster_free(clustersChain);
            return NULL;
        }*/
        if(add_cluster(clustersChain,cluster)){
            cluster_free(clustersChain);
            return NULL;
        }
        cluster= read_cluster16(cluster,buffer);
    }
    if(clustersChain->size==0){
        cluster_free(clustersChain);
        return NULL;
    }
    return clustersChain;
}
//read directory
int read_entry(struct dir_entry_t *entry, char * buffer, size_t buffer_size, size_t *offset){
    char c;
    uint8_t name_len=0;
    uint8_t extension_len=0;
    uint8_t attributes;
    uint16_t date,time;
    uint32_t size;
    for (int i = 0; i < 8; ++i) {
        if(*offset + i >= buffer_size)return 1;
        c = buffer[*offset + i];
        if(i==0 && c == 0x00)return 1;
        if(c!=' ')name_len++;
    }
    for (int i = 0; i < 8; ++i) {
        c = buffer[*offset + i];
        if(c!=' '){
            *(entry->name+i)=c;
        }
    }
    *offset += 8;
    *(entry->name+name_len)='\0';
    for (int i = 0; i < 3; ++i) {
        if(*offset + i >= buffer_size)return 1;
        c = buffer[*offset + i];
        if(c!=' ')extension_len++;
    }
    if(extension_len!=0){
        *(entry->name+name_len)='.';
        for (int i = 0; i < extension_len; ++i) {\
            if(*offset + i >= buffer_size)return 1;
            c = buffer[*offset + i];
            *(entry->name+name_len+i+1)=c;
        }
        *(entry->name+name_len+extension_len+1)='\0';
    }
    *offset += 3;
    attributes = buffer[*offset];
    entry->is_archived = (attributes >> 5)&1;
    entry->is_readonly = (attributes)&1;
    entry->is_system = (attributes >> 2)&1;
    entry->is_hidden = (attributes >> 1)&1;
    entry->is_directory = (attributes >> 4)&1;
    *offset+=1;
    *offset+=2;
    if(*offset >= buffer_size)return 1;
    if(*offset + 2 >= buffer_size)return 1;
    memcpy(&time, buffer + *offset, sizeof(uint16_t ));
    *offset+=2;
    if(*offset + 2 >= buffer_size)return 1;
    memcpy(&date, buffer + *offset, sizeof(uint16_t ));
    *offset+=2;
    entry->creation_date.day=date&DAY_MASK;
    entry->creation_date.month=(date&MONTH_MASK)>>5;
    entry->creation_date.year=((date&YEAR_MASK)>>9)+1980;
    entry->creation_time.second=(time&SEC_MASK)*2;
    entry->creation_time.minute=(time&MIN_MASK)>>5;
    entry->creation_time.hour=(time&HOUR_MASK)>>11;
    if(*offset + 14 >= buffer_size)return 1;
    *offset += 8;
    memcpy(&entry->first_cluster, buffer + *offset, sizeof(uint16_t));
    *offset += 2;
    if(*offset + 2 >= buffer_size)return 1;
    memcpy(&size, buffer + *offset, sizeof(uint32_t));
    entry->size=size;
    *offset+=4;
    return 0;
}
//disk management
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
    if(fread(buffer, 512, sectors_to_read, pdisk->memory)!=(size_t)sectors_to_read){
        return -1;
    }
    return 0;
}
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
//fat management
struct volume_t* fat_open(struct disk_t* pdisk, uint32_t first_sector){
    struct volume_t* volume;
    char sector[512];
    char * fat_system_label;
    struct boot_sector *bs;
    struct table file_allocation_table1;
    struct table file_allocation_table2;
    if(disk_read(pdisk,(int32_t)first_sector,sector,1) != 0){
        return NULL;
    }
    if(*(uint16_t *)(sector + 510)!=0xAA55){
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
//file management
struct file_t* file_open(struct volume_t* pvolume, const char* file_name){
    struct file_t* file;
    struct dir_entry_t temp_entry;
    size_t actual_size = 0;
    size_t sector_size;
    size_t offset = 0;
    size_t data_offset = 0;
    size_t cluster_to_sector;
    if(pvolume==NULL||file_name==NULL){
        return NULL;
    }
    file = malloc(sizeof(struct file_t));
    if(file==NULL){
        errno=ENOMEM;
        return NULL;
    }
    for (int i = 0; i < (int)(pvolume->bs.root_entry_count); ++i) {
        if(read_entry(&temp_entry,pvolume->directory,pvolume->bs.root_entry_count*pvolume->bs.bytes_per_sector,&offset)) {
            free(file);
            return NULL;
        }
        if(strcmp(file_name,temp_entry.name)==0){
            file->size=temp_entry.size;
            file->chain = get_chain_fat16(pvolume->file_allocation_table.data,temp_entry.size,temp_entry.first_cluster);
            if(file->chain==NULL){
                free(file);
                return NULL;
            }
            sector_size = pvolume->bs.sectors_per_cluster*pvolume->bs.bytes_per_sector;
            while (actual_size < file->size){
                actual_size += sector_size;
            }
            file->data= malloc(actual_size);
            if(file->data==NULL){
                free(file);
                return NULL;
            }
            for (int j = 0; j < (int)(file->chain->size); j++){
                cluster_to_sector = pvolume->first_data_sector + (*(file->chain->clusters+j)-2)*pvolume->bs.sectors_per_cluster;
                if(disk_read(pvolume->disk, (int)cluster_to_sector, file->data + data_offset, pvolume->bs.sectors_per_cluster)==-1){
                    free(file->data);
                    free(file);
                    return NULL;
                }
                data_offset+=pvolume->bs.bytes_per_sector*pvolume->bs.sectors_per_cluster;
                if(data_offset>=file->size)break;
            }
            file->set=file->data;
            file->cur=file->set;
            file->end=file->set+file->size;
            return file;
        }
    }
    return NULL;
}
int file_close(struct file_t* stream){
    if(stream==NULL){
        return -1;
    }
    free(stream->data);
    cluster_free(stream->chain);
    free(stream);
    stream=NULL;
    return 0;
}
size_t file_read(void *ptr, size_t size, size_t nmemb, struct file_t *stream){
    size_t read = 0;
    if(ptr==NULL||size==0||nmemb==0||stream==NULL){
        errno = EFAULT;
        return -1;
    }
    for (int i = 0; i < (int)(nmemb); ++i) {
        for (int j = 0; j < (int)(size); ++j) {
            if(stream->cur==stream->end){
                return read;
            }
            *((char *)(ptr)+j+size*i)=*(stream->cur);
            stream->cur+=1;
        }
        read++;
    }
    return read;
}
int32_t file_seek(struct file_t* stream, int32_t offset, int whence){
    int32_t shift;
    char * temp;
    if(stream==NULL){
        errno = EFAULT;
        return -1;
    }
    switch (whence) {
        case 0:
            temp = stream->set+offset;
            break;
        case 1:
            temp = stream->cur+offset;
            break;
        case 2:
            temp = stream->end+offset;
            break;
        default:
            errno = EINVAL;
            return -1;
    }
    if(temp<stream->set||temp>stream->end){
        errno = ENXIO;
        return -1;
    }
    shift = (int)(temp - stream->cur);
    stream->cur = temp;
    return shift;
}
//dir manager
struct dir_t* dir_open(struct volume_t* pvolume, const char* dir_path) {
    if(pvolume==NULL||dir_path==NULL){
        errno=EFAULT;
        return NULL;
    }
    if(strcmp(dir_path,"\\")!=0){
        errno=ENOENT;
        return NULL;
    }
    struct dir_t *dir;
    dir= malloc(sizeof(struct dir_t));
    if(dir==NULL){
        errno = ENOMEM;
        return NULL;
    }
    dir->entries=pvolume->bs.root_entry_count;
    dir->entry_size=32;
    dir->data=pvolume->directory;
    dir->offset = 0;
    return dir;
}
int dir_read(struct dir_t* pdir, struct dir_entry_t* pentry) {
    if(pdir==NULL||pentry==NULL){
        errno=EFAULT;
        return 1;
    }
    while (1){
        if (read_entry(pentry, pdir->data, pdir->entries * pdir->entry_size, &pdir->offset) != 0) {
            return 1;
        }
        if((unsigned char )*(pentry->name)!=0xE5){
            break;
        }
    }

    return 0;
}
int dir_close(struct dir_t* pdir){
    if(pdir==NULL){
        errno = EFAULT;
        return -1;
    }
    pdir->data=NULL;
    pdir->offset=0;
    pdir->entries=0;
    free(pdir);
    return 0;
}
