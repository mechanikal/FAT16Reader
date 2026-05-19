#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>
#include "include/directory_tools.h"
#include "include/table_tools.h"

//read directory entry
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

// dir manager
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
