#include "cachelab.h"
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <getopt.h>

/* struct for each cache line */
typedef struct cache_line{
    int block_num; /* number of block num */
    int tag;       /* tags */
    int valid;     /* valid bit */
    int lru_time;  /* last visit time */
    char* add_list;
} cache_line;
/* global variable */
int m_hit      = 0;
int m_miss     = 0;
int m_eviction = 0;
int m_time     = 0;
int E, s, b, v = 0;

unsigned convert_to_binary(unsigned hex);
int get_set_index(unsigned add, int bit_b, int bit_s);
int get_tag(unsigned, int, int);
void load_data(cache_line *, int, int, int);



int main(int argc, char* argv[])
{
    int o;
    
    const char* optString = "vE:s:b:t:v";
    char* filename;
    // first parse the options
    while ((o = getopt(argc, argv, optString)) != -1)
    {
        switch(o){
            case 'E':
                E = atoi(optarg);
                break;
            case 's':
                s = atoi(optarg);
                break;
            case 'b':
                b = atoi(optarg);
                break;
            case 't':
                filename = (char*)malloc(strlen(optarg) * sizeof(char*));
                strcpy(filename, optarg);
                break;
            case 'v':
                v = 1;
                break;
                
        }
    }
    

    // compute each 
    int s_num = (int) pow(2.0, s);  /* number of sets */
    int b_num = (int) pow(2.0, b);  /* number of blocks in each line */
    printf("Sets:%d\t lines:%d\t blocks:%d\n", s_num, E, b_num);    
    cache_line cache[s_num][b_num]; /* Container to save datas */
    for(int i = 0; i < s_num; ++i){
        for(int j = 0; j < b_num; ++j){
            cache[i][j].tag = 0;
            cache[i][j].valid = 0;
        }
    }

    /* open the trace file */ 
    FILE* fp = fopen(filename, "r");
    char identifier;
    unsigned address;
    int size;
    /* Reading lines like " M 20, 1" */
    while(fscanf(fp, " %c %x,%d", &identifier, &address, &size) != -1){
        /* execute corresponding command */
        // int binary_add = convert_to_binary(address);
        ++m_time;
        int set_index  = get_set_index(address, b, s); 
        int tag_index  = get_tag(address, b, s);
        if(v == 1){
            printf("%c %x,%d", identifier, address, size);
        }
        // printf("Addr:%d\t Set_index: %d\t Tag:%d\n", address, set_index, tag_index);

        if(identifier == 'I'){
            continue;
        }else if(identifier == 'L' || identifier == 'S'){
            /* Load data from cache */
            /* first Parse the identifier */
            load_data(cache[set_index], set_index, tag_index, E);   
        }else if(identifier == 'M'){
            /* first load_data */
            load_data(cache[set_index], set_index, tag_index, E);
            load_data(cache[set_index], set_index, tag_index, E);
        }
        if(v)
            printf("\n");
    }

    printSummary(m_hit, m_miss, m_eviction);
    return 0;
}


// unsigned convert_to_binary(unsigned hex){
//     /* convert the num into the binary-style inorder to compare the tag and set bits */
    
// }


/* Get the index of set */
int get_set_index(unsigned add, int bit_b, int bit_s){
    add = add >> bit_b;
    unsigned cmp = -1; // "0b1111111....111"
    cmp = cmp >> (64 - bit_s);
    return cmp & add;
}

/* Get the tag sequence */
int get_tag(unsigned add, int bit_b, int bit_s){
    add = add >> (bit_b + bit_s);
    unsigned cmp = -1; // "0b1111111....111"
    cmp = cmp >> (64 - bit_s - bit_b);
    return cmp & add;
    return 0;
}

/* execute the 'L' instruction */
void load_data(cache_line* set_row, int set_index, int tag, int lines){
    /* first iter all the cache_line in this set, see whether exist the tag and record the first empty line */
    int empty_line = -1;
    int hit = 0;
    int minimal_time_index = -1;
    int minimal_lru = 1000000;
    cache_line* cur;
    for(int i = 0 ; i < lines; ++i){
        cur = (cache_line*)(set_row + i);
        if(cur->valid == 0 && empty_line == -1){
            // free
            empty_line = i;
        }else if(cur->valid == 1 && cur->tag == tag){
            // hit;
            hit = 1;
            break;
        }else{
            if(cur->lru_time < minimal_lru){
                minimal_lru = cur->lru_time;
                minimal_time_index = i;
            }
        }
    }
    if(hit == 1){
        ++m_hit;
        cur->lru_time = m_time;
        if(v){
            printf(" hit");
        } 
        // update the LRU
    }else{
        // miss
        if(v)
            printf(" miss");
        if(empty_line != -1){
            // printf(" evinct")
            cache_line* new_cache_line = (cache_line*)(set_row + empty_line);
            new_cache_line->tag = tag;
            new_cache_line->valid = 1;
            new_cache_line->lru_time = m_time;
        }else{
            // choose a eviction one 
            if(v)
                printf(" eviction");
            cache_line* new_cache_line = (cache_line*)(set_row + minimal_time_index);
            new_cache_line->tag = tag;
            new_cache_line->valid = 1;
            new_cache_line->lru_time = m_time;
            ++m_eviction;
        }
        ++m_miss;
    }

}