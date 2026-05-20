#include "fat16_reader.h"
#include <string.h>

void list_current_dir();
void cli_loop();
int change_directories(char * new_dir_path);
void preview_file(struct file_t * file);

struct volume_t * volume;
struct disk_t * disc;
struct dir_t * current_directory;

int main() {
    disc = disk_open_from_file("../tests/fat_test.img");
    volume = fat_open(disc,0);
    current_directory = root_dir_open(volume);
    cli_loop();
    dir_close(current_directory);
    fat_close(volume);
    disk_close(disc);
    return 0;
}

void cli_loop(){
    char input_buffer[30];
    char path_buffer[13];
    int res;
    while (1){
        scanf("%s",input_buffer);
        if (strcmp(input_buffer,"quit") == 0){
            return;
        }
        if (strcmp(input_buffer,"ls") == 0){
            list_current_dir();
        }
        if (strcmp(input_buffer,"cd") == 0){
            scanf("%s",path_buffer);
            res = change_directories(path_buffer);
            if (res != 0){
                printf("cant find directory\n");
            }
        }
        if (strcmp(input_buffer,"preview") == 0){
            scanf("%s",path_buffer);
            struct file_t * file;
            file = file_open(volume,path_buffer,*current_directory);
            if (file == NULL){
                printf("cant open file\n");
                continue;
            }
            preview_file(file);
            file_close(file);
        }
    }
}

void preview_file(struct file_t * file){
    char preview_buffer[101];
    preview_buffer[100] = '\0';
    size_t byte_count;
    byte_count = file_read(preview_buffer, sizeof(char),100,file);
    printf("previewing first %ld bytes:\n%s\n",byte_count,preview_buffer);
}

void list_current_dir(){
    current_directory->offset = 0;
    struct dir_entry_t entry;
    while(!dir_read(current_directory,&entry)){
        printf("\t");
        if(entry.is_directory){
            printf("/");
        }
        printf("%s\n",entry.name);
    }
}

int change_directories(char * new_dir_path){
    struct dir_t * new_dir = dir_open(volume,new_dir_path,current_directory);
    if (new_dir == NULL){
        return 1;
    }
    dir_close(current_directory);
    current_directory = new_dir;
    return 0;
}
