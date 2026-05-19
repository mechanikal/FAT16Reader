//
// Created by root on 5/19/26.
//

#ifndef FAT16READER_CLUSTER_TOOLS_H
#define FAT16READER_CLUSTER_TOOLS_H

#include <stdint.h>
#include <stdlib.h>

#define LAST_CLUSTER16 0xFFF8

struct clusters_chain_t {
    uint16_t *clusters;
    size_t size;
};

struct clusters_chain_t *get_chain_fat16(const void * buffer, size_t size, uint16_t first_cluster);
void cluster_free(struct clusters_chain_t * clustersChain);

#endif //FAT16READER_CLUSTER_TOOLS_H
