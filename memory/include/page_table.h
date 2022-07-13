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
{
    uint32_t frame;
    bool present;
    bool used;
    bool modified;
} t_page_entry;

typedef struct t_ptbr2
{
    uint32_t tableNumber;
    t_list *entries; // t_page_entry
} t_ptbr2;

typedef struct t_ptbr1
{
	uint32_t tableNumber;
    t_list *entries; // t_ptbr2
} t_ptbr1;

uint32_t swap_files_counter;

t_ptbr1 *page_table_init();
void page_table_destroy(t_ptbr1 *table);
uint32_t pageTableAddEntry(t_ptbr2 *table, uint32_t newFrame);
void pageTable_destroyLastEntry(t_ptbr1 *pt);

t_ptbr2 *getPageTable2(uint32_t _PID, uint32_t pt1_entry, t_dictionary *pageTables);
t_ptbr1 *getPageTable(uint32_t _PID, t_dictionary *pageTables);

uint32_t pageTable_getFrame(uint32_t PID, uint32_t pt1_entry, uint32_t pt2_entry);
void *memory_getFrame(t_memory *mem, uint32_t frame);

typedef struct t_metadata
{
    uint32_t pid;
    uint32_t pageNumber;
    bool used;
} t_metadata;

typedef struct swapFile
{
    char* path;
    int fd;
    size_t size;
    size_t pageSize;
    int maxPages;
    t_pageMetadata* entries;
} t_swapFile;

t_swapFile* swapFile_create(char* path, uint32_t PID, size_t size, size_t pageSize);
int swapFile_getIndex(t_swapFile* sf, uint32_t pid, uint32_t pageNumber);
void* swapFile_readAtIndex(t_swapFile* sf, int index);

uint32_t swapPage(uint32_t PID, uint32_t pt1_entry, uint32_t pt2_entry, uint32_t page);

t_list *swapFiles;
uint32_t pageTable_number;

void swapFile_clearAtIndex(t_swapFile* sf, int index);
uint32_t clock_m_alg(uint32_t start, uint32_t end);
uint32_t clock_alg(uint32_t start, uint32_t end);
bool fija_memoria(uint32_t *start, uint32_t *end, uint32_t PID);
bool destroy_swap_page(uint32_t pid, uint32_t page);
bool read_swap_page(uint32_t pid, uint32_t page);
bool fija_swap(uint32_t pid, uint32_t page, void* pageContent);
void* readPage(uint32_t pid, uint32_t pageNumber);
void destroyPage(uint32_t pid, uint32_t page);
bool savePage(uint32_t pid, uint32_t pageNumber, void* pageContent);
t_swapFile *pidExists(uint32_t pid);
bool isFree(uint32_t frame);

#endif /* PAGETABLE_H_ */
