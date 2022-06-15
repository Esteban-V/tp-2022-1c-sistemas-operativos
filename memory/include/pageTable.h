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
    bool in_use;
    bool modified;
} t_pageTableEntry;

typedef struct t_ptbr2 {
    int32_t pageQuantity;
    t_pageTableEntry *entries;
} t_ptbr2;

typedef struct t_ptbr1 {
    int32_t pageQuantity;
    t_ptbr2 *entries;
} t_ptbr1;

t_ptbr1 *initializePageTable();

void destroyPageTable(t_ptbr1 *table);
void _destroyPageTable(void *table);

int32_t pageTableAddEntry(t_ptbr2 *table, uint32_t newFrame);

void pageTable_destroyLastEntry(t_ptbr1* pt);

t_ptbr1* getPageTable(uint32_t _PID, t_dictionary* pageTables);

bool pageTable_isEmpty(uint32_t PID);

int32_t pageTable_getFrame(uint32_t PID, uint32_t page);

#endif /* PAGETABLE_H_ */
