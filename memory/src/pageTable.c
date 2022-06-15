#include "pageTable.h"
#include <stdbool.h>
#include <stdlib.h>
#include <commons/string.h>
#include <commons/collections/dictionary.h>
#include <pthread.h>


t_ptbr1 *initializePageTable1(){
	t_ptbr1 *newTable = malloc(sizeof(t_ptbr1));
    newTable->pageQuantity = 0;
    newTable->entries = NULL;
    return newTable;
}


void destroyPageTable(t_ptbr1 *table){
    free(table->entries);
    free(table);
}

void _destroyPageTable(void *table){
    destroyPageTable((t_ptbr1*) table);
}

void pageTable_destroyLastEntry(t_ptbr1* pt){
    pthread_mutex_lock(&pageTablesMut);
        (pt->pageQuantity)--;
        pt->entries = realloc(pt->entries, sizeof(t_pageTableEntry)*(pt->pageQuantity));
    pthread_mutex_unlock(&pageTablesMut);
}

int32_t pageTableAddEntry(t_ptbr2 *table, uint32_t newFrame){
    pthread_mutex_lock(&pageTablesMut);
        table->entries = realloc(table->entries, sizeof(t_pageTableEntry)*(table->pageQuantity + 1));
        (table->entries)[table->pageQuantity].frame = newFrame;
        (table->entries)[table->pageQuantity].present = false;
        (table->pageQuantity)++;
        int32_t pgQty = table->pageQuantity -1;
    pthread_mutex_unlock(&pageTablesMut);
    return pgQty;
}

t_ptbr1* getPageTable(uint32_t _PID, t_dictionary* pagTables) {
    char *PID = string_itoa(_PID);
    t_ptbr1* pt = (t_ptbr1*) dictionary_get(pagTables, PID);
    free(PID);
    return pt;
}

bool pageTable_isEmpty(uint32_t PID) {
	t_ptbr1 *pt = getPageTable(PID, pageTables);
    pthread_mutex_lock(&pageTablesMut);
        bool isEmpty = pt->pageQuantity == 0;
    pthread_mutex_unlock(&pageTablesMut);
    return isEmpty;
}

int32_t pageTable_getFrame(uint32_t PID, uint32_t page){
    int32_t frame;
    
    pthread_mutex_lock(&pageTablesMut);
    t_ptbr1 *pt = getPageTable(PID, pageTables);
        //frame = page < pt->pageQuantity ? pt->entries[page].frame : -1;
    pthread_mutex_unlock(&pageTablesMut);

    return frame;
}
