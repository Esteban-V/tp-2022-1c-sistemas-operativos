#ifndef SWAP_H_
#define SWAP_H_

#include <stdint.h>
#include <stdbool.h>
#include <commons/collections/dictionary.h>
#include <pthread.h>
#include "utils.h"

void create_swap(uint32_t pid, size_t process_size);
void swap_write_page(uint32_t pid, int page, void *data);
void *swap_get_page(uint32_t pid, int page);
void delete_swap(uint32_t pid);

#endif /* SWAP_H_ */
