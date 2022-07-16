#ifndef PAGETABLE_H_
#define PAGETABLE_H_

#include <stdint.h>
#include <stdbool.h>
#include <commons/collections/dictionary.h>
#include <pthread.h>
#include "utils.h"
#include "swap.h"

t_list *level1_tables;
t_list *level2_tables;

pthread_mutex_t pageTablesMut;
t_dictionary *pageTables;

typedef struct t_page_entry
{
    // A que frame (index) de la memoria (de void*) corresponde
    int frame;
    // Bit de presencia
    bool present;
    // Bit de uso
    bool used;
    // Bit de modificado
    bool modified;
} t_page_entry;

typedef struct t_ptbr2
{
    // t_page_entry
    t_list *entries;
} t_ptbr2;

typedef struct t_ptbr1
{
    // int a t_ptbr2
    t_list *entries;
} t_ptbr1;

uint32_t pageTableAddEntry(t_ptbr2 *table, uint32_t newFrame);
void pageTable_destroyLastEntry(t_ptbr1 *pt);

int page_table_init(uint32_t process_size);
t_ptbr1 *get_page_table1(uint32_t pt1_index);

int get_page_table2_index(uint32_t pt1_index, uint32_t entry_index);
t_ptbr2 *get_page_table2(uint32_t pt2_index);

int get_frame_number(uint32_t pt2_index, uint32_t entry_index);
void *get_frame(uint32_t frame_number);
uint32_t get_frame_value(void *frame_ptr, uint32_t offset);

bool isFree(int frame_number);

uint32_t createPage(uint32_t pid, uint32_t pt1_entry);

void *readPage(uint32_t dir);
bool savePage(uint32_t pid, uint32_t pageNumber, void *pageContent);
uint32_t swapPage(uint32_t pid, uint32_t pt1_entry, uint32_t pt2_entry, uint32_t page);

void destroyPage(uint32_t pid, uint32_t page);

void page_table_destroy(t_ptbr1 *table);

uint32_t clock_m_alg(uint32_t start, uint32_t end);
uint32_t clock_alg(uint32_t start, uint32_t end);
bool fija_memoria(uint32_t *start, uint32_t *end, uint32_t pid);

#endif /* PAGETABLE_H_ */
