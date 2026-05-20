#include "include/cluster_tools.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>

// create object which will store cluster information
struct cluster_chain_t * cluster_innit(){
    struct cluster_chain_t *clustersChain = malloc(sizeof(struct cluster_chain_t));
    if(clustersChain==NULL) {
        errno = ENOMEM;
        return NULL;
    }
    clustersChain->cluster_count=0;
    clustersChain->clusters=NULL;
    return clustersChain;
}
void cluster_free(struct cluster_chain_t * clusters_chain){
    if(clusters_chain->cluster_count != 0)
        free(clusters_chain->clusters);
    clusters_chain->clusters=NULL;
    clusters_chain->cluster_count=0;
    free(clusters_chain);
}

// add cluster to chain
int add_cluster(struct cluster_chain_t * clustersChain, uint16_t cluster){
    uint16_t * temp = realloc(clustersChain->clusters, (clustersChain->cluster_count + 1) * sizeof(uint16_t));
    if(temp==NULL){
        errno = ENOMEM;
        return 1;
    }
    clustersChain->clusters=temp;
    *(clustersChain->clusters+clustersChain->cluster_count)=cluster;
    clustersChain->cluster_count ++;
    return 0;
}

// navigate to next cluster
uint16_t read_cluster16(uint16_t previous,const void * const buffer){
    const uint16_t * cluster_buffer = (const uint16_t*)buffer;
    return *(cluster_buffer + previous);
}

// read cluster chain
struct cluster_chain_t *get_chain_fat16(const void * const buffer, size_t size, uint16_t first_cluster){
    struct cluster_chain_t *clustersChain;
    uint16_t cluster=first_cluster;

    if(buffer==NULL||size<0) { // allow size 0 for directories
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
    if(clustersChain->cluster_count == 0){
        cluster_free(clustersChain);
        errno = EINVAL;
        return NULL;
    }
    return clustersChain;
}
