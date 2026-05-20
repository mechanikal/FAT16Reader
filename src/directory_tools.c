#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>
#include "include/directory_tools.h"
#include "include/disc_tools.h"
#include "internal.h"

// read single directory entry, shift offset
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

struct dir_t* root_dir_open(struct volume_t* pvolume) {
    if(pvolume==NULL){
        errno=EFAULT;
        return NULL;
    }
    struct dir_t *dir;
    dir= malloc(sizeof(struct dir_t));
    if(dir==NULL){
        errno = ENOMEM;
        return NULL;
    }
    dir->size=ENTRY_SIZE*pvolume->bs.root_entry_count;
    dir->data=pvolume->directory;
    dir->offset = 0;
    dir->cluster_chain = NULL;
    return dir;
}

// dir manager
struct dir_t* dir_open(struct volume_t* pvolume, const char* dir_path,struct dir_t * current_directory){
    struct dir_t* dir;
    struct dir_entry_t temp_entry;
    size_t offset = 0;
    size_t cluster_size;
    size_t data_offset = 0;
    size_t cluster_to_sector;

    if(pvolume==NULL||dir_path==NULL){
        errno=EFAULT;
        return NULL;
    }

    dir = malloc(sizeof(struct dir_t));
    if(dir==NULL){
        errno=ENOMEM;
        return NULL;
    }
    dir->size = 0;
    dir->data = NULL;
    dir->offset = 0;
    dir->cluster_chain = NULL;

    for (int i = 0; i < (int)(pvolume->bs.root_entry_count); ++i) {
        if(read_entry(&temp_entry,current_directory->data,current_directory->size,&offset)) {
            free(dir);
            return NULL;
        }
        if(strcmp(dir_path,temp_entry.name)==0){
            if(!temp_entry.is_directory){
                errno = ENOTDIR;
                free(dir);
                return NULL;
            }
            if(temp_entry.first_cluster == 0){
                free(dir);
                return root_dir_open(pvolume);
            }
            dir->cluster_chain = get_chain_fat16(pvolume->file_allocation_table.data,temp_entry.size,temp_entry.first_cluster);
            if(dir->cluster_chain==NULL){
                free(dir);
                return NULL;
            }
            cluster_size = pvolume->bs.sectors_per_cluster * pvolume->bs.bytes_per_sector;
            dir->size = dir->cluster_chain->cluster_count * cluster_size;
            dir->data= malloc(dir->size);
            if(dir->data==NULL){
                free(dir);
                errno = ENOMEM;
                return NULL;
            }
            for (int j = 0; j < (int)(dir->cluster_chain->cluster_count); j++){
                cluster_to_sector = pvolume->first_data_sector + (*(dir->cluster_chain->clusters+j)-2)*pvolume->bs.sectors_per_cluster;
                if(disk_read(pvolume->disk, (int)cluster_to_sector, dir->data + data_offset, pvolume->bs.sectors_per_cluster)==-1){
                    free(dir->data);
                    free(dir);
                    return NULL;
                }
                data_offset+=pvolume->bs.bytes_per_sector*pvolume->bs.sectors_per_cluster;
                if(data_offset>=dir->size)break;
            }
            return dir;
        }
    }
    return NULL;
}

int dir_read(struct dir_t* pdir, struct dir_entry_t* pentry) {
    if(pdir==NULL||pentry==NULL){
        errno=EFAULT;
        return 1;
    }
    while (1){
        if (read_entry(pentry, pdir->data, pdir->size, &pdir->offset) != 0) {
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
    pdir->size = 0;
    pdir->offset = 0;
    free(pdir);
    return 0;
}
