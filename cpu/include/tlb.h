#ifndef INCLUDE_TLB_H_
#define INCLUDE_TLB_H_

#include <stdio.h>
#include <stdlib.h>

#include <commons/log.h>
#include <commons/collections/list.h>

#include <pthread.h>

#include "queue.h"
#include "utils.h"

enum e_replaceAlgorithm
{
    FIFO = 0,
    LRU = 1
};

enum e_replaceAlgorithm replaceAlgorithm;

typedef struct tlb_entry
{
    uint32_t page;
    uint32_t frame;
    bool isFree;
} t_tlb_entry;

typedef struct tlb
{
    t_tlb_entry *entries;
    // t_tlb_entry
    t_pQueue *victims;
} t_tlb;

pthread_mutex_t tlb_mutex;

t_tlb *tlb;

int tlb_hit_counter;
int tlb_miss_counter;

t_tlb *create_tlb();
void add_tlb_entry(uint32_t page, uint32_t frame);

uint32_t get_tlb_frame(uint32_t page);
void lru_tlb(t_tlb_entry *entry);

void clean_tlb();
void drop_tlb_entry(uint32_t page, uint32_t frame);
void tlb_destroy();

#endif /* INCLUDE_TLB_H_ */
