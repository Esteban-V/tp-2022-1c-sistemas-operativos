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

// Stats
int memory_access_counter;
int memory_read_counter;
int memory_write_counter;
int page_fault_counter;
int page_assignment_counter;
int page_replacement_counter;

int replaceAlgorithm2;

// 0 = CLOCK
// 1 = CLOCK_M
int page_table_init(uint32_t process_size, int algorithm);
int assign_process_frames();

t_ptbr1 *get_page_table1(int pt1_index);
int get_page_table2_index(uint32_t pt1_index, uint32_t entry_index);
t_ptbr2 *get_page_table2(int pt2_index);

int replace_algorithm(t_process_frame *process_frames, t_page_entry *entry, int pid);
int two_clock_turns(t_process_frame *process_frames, bool check_modified, void *(*replace)(t_frame_entry *, t_page_entry *));

#endif /* PAGETABLE_H_ */
