#include "cachelab.h"
#include <getopt.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

int hits = 0;
int misses = 0;
int evictions = 0;
int stamp = 0;
int s = 0;
int e = 0;
int b = 0;
int s_bit = 0;
typedef struct {
    int tag;
    int stamp;
    int valid;
} cache_line, *cache_set, **cache;

cache init_LRU();
void scan_file(FILE* fp, cache c);
void free_LRU(cache c);
void update(cache c, unsigned long address);

int main(int argc, char* argv[])
{
    int opt;
    FILE *fp;
    fp = NULL;
    while ((opt = getopt(argc, argv, "s:E:b:t:")) != -1) {
        switch (opt) {
            case 's':
                s = atoi(optarg);
                break;
            case 'E':
                e = atoi(optarg);
                break;
            case 'b':
                b = atoi(optarg);
                break;
            case 't':
                fp = fopen(optarg, "r");
                break;
            default: 
                fprintf (stderr, "Unknown option `-%c'.\n", optopt);
                exit(0);
                break;
        }
    }
    if (s <= 0 || e <= 0 || b <= 0 || fp == NULL) {
        fprintf(stderr, "Not enough or invalid arguments\n");
        exit(0);
    }
    s_bit = s;
    s = 1 << s;
    //build the LRU cache
    cache c = init_LRU();

    //read the file line by line and update the cache
    scan_file(fp, c);

    //destroy the LRU and free all memory
    free_LRU(c);
    fclose(fp);
    printSummary(hits, misses, evictions);
    return 0;
}

cache init_LRU() {
    cache c = (cache)malloc(s * sizeof(cache_set));
    int i, j;
    for (i = 0; i < s; i++) {
        c[i] = (cache_set)malloc(e * sizeof(cache_line));
        for (j = 0; j < e; j++) {
            c[i][j].valid = 0;
            c[i][j].tag = -1;
            c[i][j].stamp = -1;
        }
    }
    return c;
}

void scan_file(FILE* fp, cache c) {
    char operation;
    unsigned long address;
    int byte;
    while (fscanf(fp, " %c %lx,%d\n", &operation, &address, &byte) != EOF) {
        //printf("%c, %lx, %d\n", operation, address, byte);
        switch (operation) {
            case 'L':
                update(c, address);
                break;
            case 'M':
                update(c, address);
            /* fall through */
            case 'S':
                update(c, address);
            default:
                break;
        }
        stamp++;
    }
}

void update(cache c, unsigned long address) {
    unsigned long max = -1;
    int tag = address >> (s_bit + b);
    int set = (address >> b) & (max >> (64 - s_bit));
    //printf("tag: %d, set: %d\n", tag, set);
    int i;
    //check if hit
    for (i = 0; i < e; i++) {
        if (c[set][i].tag == tag) {
            hits++;
            c[set][i].stamp = stamp;
            return;
        }
    }

    //printf("cur tag in cache is: %d\n", c[set][0].tag);    
    misses++;
    //check if full
    for (i = 0; i < e; i++) {
        if (c[set][i].valid == 0) {
            c[set][i].valid = 1;
            c[set][i].tag = tag;
            c[set][i].stamp = stamp;
            return;
        }
    }
    evictions++;
    //if full then evict one and add this to cache
    //printf("evict start\n");
    cache_line* evict = &c[set][0];
    for (i = 1; i < e; i++) {
        if (c[set][i].stamp < evict->stamp) {
            evict = &c[set][i];
        }
    }
    //printf("update the evict: %d\n", tag);
    evict->valid = 1;
    evict->tag = tag;
    evict->stamp = stamp;
}

void free_LRU(cache c) {
    int i;
    for (i = 0; i < s; i++) {
        free(c[i]);
    }
    free(c);
}