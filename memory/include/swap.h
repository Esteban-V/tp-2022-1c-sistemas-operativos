#ifndef SWAP_H_
#define SWAP_H_

#include <stdint.h>
#include <stdbool.h>
#include <commons/collections/dictionary.h>
#include <pthread.h>
#include "utils.h"

uint32_t swap_files_counter;
t_list *swapFiles; // t_swap_file

typedef struct t_swap_file
{
    char *path;
    int fd;
    size_t size;
    size_t pageSize;
    int maxPages;
    t_pageMetadata *entries;
} t_swap_file;

t_swap_file *swapFile_create(uint32_t PID, size_t process_size);
void swapFile_register(t_swap_file *sf, uint32_t pid, uint32_t pageNumber, int index);
int swapFile_getIndex(t_swap_file *sf, uint32_t pid, uint32_t pageNumber);
void *swapFile_readAtIndex(t_swap_file *sf, int index);
void swapFile_writeAtIndex(t_swap_file *sf, int index, void *pagePtr);
void swapFile_clearAtIndex(t_swap_file *sf, int index);

bool swapFile_hasPid(t_swap_file *sf, uint32_t pid);
t_swap_file *pidExists(uint32_t pid);
int swapFile_countPidPages(t_swap_file *sf, uint32_t pid);

bool destroy_swap_page(uint32_t pid, uint32_t page);

int getChunk(t_swap_file *sf, uint32_t pid);
bool hasFreeChunk(t_swap_file *sf);
int findFreeChunk(t_swap_file *sf);

#endif /* SWAP_H_ */
