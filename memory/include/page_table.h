#ifndef PAGETABLE_H_
#define PAGETABLE_H_

#include <stdint.h>
#include <stdbool.h>
#include <commons/collections/dictionary.h>
#include <pthread.h>
#include "utils.h"

pthread_mutex_t pageTablesMut;
t_dictionary *pageTables;

typedef struct t_page_entry
{ // TODO Datos?
    uint32_t frame;
    bool present;
    bool used;
    bool modified;
} t_page_entry;

typedef struct t_ptbr2
{
    uint32_t tableNumber;
    uint32_t pageQuantity;
    t_list *entries; //t_page_entry
} t_ptbr2;

typedef struct t_ptbr1
{
    uint32_t entryQuantity;
    t_list *entries;
} t_ptbr1;

uint32_t swap_files_counter;

t_ptbr1 *initializePageTable();
void page_table_destroy(t_ptbr1 *table);
uint32_t pageTableAddEntry(t_ptbr2 *table, uint32_t newFrame);
void pageTable_destroyLastEntry(t_ptbr1 *pt);

void *memory_getFrame(t_memory *mem, uint32_t frame);

t_ptbr1 *getPageTable(uint32_t _PID, t_dictionary *pageTables);
uint32_t pageTable_getFrame(uint32_t PID, uint32_t page);
bool pageTable_isEmpty(uint32_t PID);

typedef struct t_metadata
{
    uint32_t pid;
    uint32_t pageNumber;
    bool used;
} t_metadata;

typedef struct swapFile
{
    char *path;
    int fd;
    int pageSize;
    int maxPages;
    t_list *entries; //t_metadata
} t_swapFile;

t_list *swapFiles;
t_swapFile *swapFile_create(char *path, int pageSize);

uint32_t clock_m_alg(uint32_t start, uint32_t end);
uint32_t clock_alg(uint32_t start, uint32_t end);
bool fija(uint32_t pid, uint32_t page, void *pageContent);

#endif /* PAGETABLE_H_ */
