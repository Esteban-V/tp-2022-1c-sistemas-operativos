#ifndef PAGETABLE_H_
#define PAGETABLE_H_

#include <stdint.h>
#include <stdbool.h>
#include <commons/collections/dictionary.h>
#include <pthread.h>
#include "swap.h"

// Lista de t_ptbr1
t_list *level1_tables;
// Lista de t_ptbr2
t_list *level2_tables;

int page_table_init(uint32_t process_size);
int assign_process_frames();

t_ptbr1 *get_page_table1(int pt1_index);
int get_page_table2_index(uint32_t pt1_index, uint32_t entry_index);
t_ptbr2 *get_page_table2(int pt2_index);

void replace_page_in_frame(uint32_t victim_frame, uint32_t PID, uint32_t pt2_index, uint32_t page);

#endif /* PAGETABLE_H_ */
