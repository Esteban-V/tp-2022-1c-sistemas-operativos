#ifndef PAGETABLE_H_
#define PAGETABLE_H_

#include <stdint.h>
#include <stdbool.h>
#include <commons/collections/dictionary.h>
#include <pthread.h>

pthread_mutex_t pageTablesMut;
t_dictionary* pageTables;

typedef struct pageTableEntry {
    bool present;
    uint32_t frame;
} t_pageTableEntry;

typedef struct t_ptbr {
    int32_t pageQuantity;
    t_pageTableEntry *entries;
} t_ptbr;

t_ptbr *initializePageTable();

void destroyPageTable(t_ptbr *table);
void _destroyPageTable(void *table);

int32_t pageTableAddEntry(t_ptbr *table, uint32_t newFrame);

void pageTable_destroyLastEntry(t_ptbr* pt);

t_ptbr* getPageTable(uint32_t _PID, t_dictionary* pageTables);

bool pageTable_isEmpty(uint32_t PID);

int32_t pageTable_getFrame(uint32_t PID, uint32_t page);

#endif /* PAGETABLE_H_ */
