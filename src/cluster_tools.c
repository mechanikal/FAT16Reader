#include "include/cluster_tools.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>

// create object which will store cluster information
struct clusters_chain_t * cluster_innit(){
    struct clusters_chain_t *clustersChain = malloc(sizeof(struct clusters_chain_t));
    if(clustersChain==NULL) {
        errno = ENOMEM;
        return NULL;
    }
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

// add cluster to chain
int add_cluster(struct clusters_chain_t * clustersChain, uint16_t cluster){
    uint16_t * temp = realloc(clustersChain->clusters,(clustersChain->size+1)* sizeof(uint16_t));
    if(temp==NULL){
        errno = ENOMEM;
        return 1;
    }
    clustersChain->clusters=temp;
    *(clustersChain->clusters+clustersChain->size)=cluster;
    clustersChain->size ++;
    return 0;
}

// navigate to next cluster
uint16_t read_cluster16(uint16_t previous,const void * const buffer){
    const uint16_t * cluster_buffer = (const uint16_t*)buffer;
    return *(cluster_buffer + previous);
}

// read cluster chain
struct clusters_chain_t *get_chain_fat16(const void * const buffer, size_t size, uint16_t first_cluster){
    struct clusters_chain_t *clustersChain;
    uint16_t cluster=first_cluster;

    if(buffer==NULL||size<=0) {
        errno = EFAULT;
        return NULL;
    }
    clustersChain = cluster_innit();
    if(clustersChain==NULL) {
        return NULL;
    }

    while (cluster<LAST_CLUSTER16&&cluster>=2){
        if(add_cluster(clustersChain,cluster)){
            cluster_free(clustersChain);
            return NULL;
        }
        cluster= read_cluster16(cluster,buffer);
    }
    if(clustersChain->size==0){
        cluster_free(clustersChain);
        errno = EINVAL;
        return NULL;
    }
    return clustersChain;
}
