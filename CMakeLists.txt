cmake_minimum_required(VERSION 3.20)
project(FAT16Reader C)

set(CMAKE_C_STANDARD 11)

include_directories(.)

add_executable(FAT16Reader
        src/file_tools.c
        include/file_tools.h
        tests/main.c src/cluster_tools.c include/cluster_tools.h src/table_tools.c src/disc_tools.c include/disc_tools.h include/table_tools.h src/directory_tools.c include/directory_tools.h fat16_reader.h src/internal.h)
