#include "include/file_tools.h"
#include "include/table_tools.h"
#include "include/disc_tools.h"
#include "include/cluster_tools.h"

#include <string.h>
#include <errno.h>

//file management
struct file_t* file_open(struct volume_t* pvolume, const char* file_name,struct dir_t directory){
    struct file_t* file;
    struct dir_entry_t temp_entry;
    size_t occupied_clusters = 0;
    size_t cluster_size;
    size_t offset = 0;
    size_t data_offset = 0;
    size_t cluster_to_sector;

    if(pvolume==NULL||file_name==NULL){
        errno = EINVAL;
        return NULL;
    }
    file = malloc(sizeof(struct file_t));
    if(file==NULL){
        errno=ENOMEM;
        return NULL;
    }
    for (int i = 0; i < (int)(pvolume->bs.root_entry_count); ++i) {
        if(read_entry(&temp_entry,directory.data,directory.size,&offset)) {
            free(file);
            return NULL;
        }
        if(strcmp(file_name,temp_entry.name)==0){
            if(temp_entry.is_directory){
                errno = EISDIR;
                return NULL;
            }
            file->size=temp_entry.size;
            file->chain = get_chain_fat16(pvolume->file_allocation_table.data,temp_entry.size,temp_entry.first_cluster);
            if(file->chain==NULL){
                free(file);
                return NULL;
            }
            cluster_size = pvolume->bs.sectors_per_cluster * pvolume->bs.bytes_per_sector;
            while (occupied_clusters * cluster_size < file->size){
                occupied_clusters ++;
            }
            file->data= malloc(occupied_clusters * cluster_size);
            if(file->data==NULL){
                free(file);
                errno = ENOMEM;
                return NULL;
            }
            for (int j = 0; j < (int)(file->chain->cluster_count); j++){
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
