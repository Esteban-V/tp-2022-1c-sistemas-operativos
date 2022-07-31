#ifndef INCLUDE_TLB_H_
#define INCLUDE_TLB_H_

#include <stdio.h>
#include <stdlib.h>

#include <commons/log.h>
#include <commons/collections/list.h>

#include <pthread.h>

#include "utils.h"

typedef struct tlb_entry
{
    uint32_t page;
    int32_t frame;
    bool isFree;
} t_tlb_entry;

typedef struct tlb
{
    t_tlb_entry *entries;
    uint32_t entryQty;
    t_list *victimQueue;
} t_tlb;

pthread_mutex_t tlb_mutex;

t_tlb *tlb;

int tlb_hit_counter = 0;
int tlb_miss_counter = 0;

void (*update_victim_queue)(t_tlb_entry *);

t_tlb *create_tlb();

void lru_tlb(t_tlb_entry *entry);
void fifo_tlb(t_tlb_entry *entry);

void clean_tlb();
void drop_tlb_entry(uint32_t page, uint32_t frame);
void destroy_tlb();

#endif /* INCLUDE_TLB_H_ */
