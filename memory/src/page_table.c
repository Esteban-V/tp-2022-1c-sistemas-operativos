#include"page_table.h"

t_ptbr1* initializePageTable1() {
	t_ptbr1 *newTable = malloc(sizeof(t_ptbr1));
	newTable->entryQuantity = 0;
	newTable->entries = NULL;
}

void destroyPageTable(t_ptbr1 *table) {
	free(table->entries);
	free(table);
}

void _destroyPageTable(void *table) {
	destroyPageTable((t_ptbr1*) table);
}

void pageTable_destroyLastEntry(t_ptbr1 *pt) {
	pthread_mutex_lock(&pageTablesMut);
	(pt->entryQuantity)--;
	pt->entries = realloc(pt->entries,
			sizeof(t_pageTableEntry) * (pt->entryQuantity));
	pthread_mutex_unlock(&pageTablesMut);
}

int32_t pageTableAddEntry(t_ptbr2 *table, uint32_t newFrame) {
	pthread_mutex_lock(&pageTablesMut);
	table->pageTableEntres = realloc(table->pageTableEntres,
			sizeof(t_pageTableEntry) * (table->pageQuantity + 1));
	(table->pageTableEntres)[table->pageQuantity].frame = newFrame;
	(table->pageTableEntres)[table->pageQuantity].present = false;
	(table->pageQuantity)++;
	int32_t pgQty = table->pageQuantity - 1;
	pthread_mutex_unlock(&pageTablesMut);
	return pgQty;
}

t_ptbr1* getPageTable(uint32_t _PID, t_dictionary *pagTables) {
	char *PID = string_itoa(_PID);
	t_ptbr1 *pt = (t_ptbr1*) dictionary_get(pagTables, PID);
	free(PID);
	return pt;
}

bool pageTable_isEmpty(uint32_t PID) {
	t_ptbr1 *pt = getPageTable(PID, pageTables);
	pthread_mutex_lock(&pageTablesMut);
	bool isEmpty = pt->entryQuantity == 0;
	pthread_mutex_unlock(&pageTablesMut);
	return isEmpty;
}

int32_t pageTable_getFrame(uint32_t PID, uint32_t page) {
	int32_t frame;

	pthread_mutex_lock(&pageTablesMut);
	t_ptbr1 *pt = getPageTable(PID, pageTables);
	//frame = page < pt->pageQuantity ? pt->entries[page].frame : -1;
	pthread_mutex_unlock(&pageTablesMut);

	return frame;
}

//////////////

t_swapFile* swapFile_create(char *path, int pageSize) {
	t_swapFile *self = malloc(sizeof(t_swapFile));
	self->path = string_duplicate(path);
	self->pageSize = pageSize;
	self->maxPages = memoryConfig->memorySize / pageSize;
	/*
	 self->fd = open(self->path, O_RDWR|O_CREAT, S_IRWXU);
	 ftruncate(self->fd, self->size);
	 void* mappedFile = mmap(NULL, self->size, PROT_READ|PROT_WRITE,MAP_SHARED, self->fd, 0);
	 memset(mappedFile, 0, self->size);
	 msync(mappedFile, self->size, MS_SYNC);
	 munmap(mappedFile, self->size);
	 */

	/* TODO self->entries = calloc(1, sizeof(t_pageMetadata) * self->maxPages);
	 for(int i = 0; i < self->maxPages; i++)
	 self->entries[i].used = false;
	 */
	return self;
}

int32_t createPage(uint32_t PID) {
	pthread_mutex_lock(&pageTablesMut);
	t_ptbr1 *table = getPageTable(PID, pageTables);
	pthread_mutex_unlock(&pageTablesMut);

	//TODO ? int32_t pageNumber = pageTableAddEntry(table, 0);

	//log_info(logger, "Creando pagina %i, para carpincho de PID %u.", pageNumber, PID);

	void *newPageContents = calloc(1, memoryConfig->pageSize);
	/*if(swapInterface_savePage(swapInterface, PID, pageNumber, newPageContents)){
	 free(newPageContents);
	 return pageNumber;
	 }*/
	free(newPageContents);
	return -1;
}

int swapFile_getIndex(t_swapFile *sf, uint32_t pid, int32_t pageNumber) {
	int found = -1;
	for (int i = 0; i < sf->maxPages; i++) {
		if (sf->entries[i].pid == pid
				&& sf->entries[i].pageNumber == pageNumber) {
			found = i;
			break;
		}
	}

	return found;
}

bool swapFile_hasPid(t_swapFile *sf, uint32_t pid) {
	bool hasPid = false;
	for (int i = 0; i < sf->maxPages; i++)
		if (sf->entries[i].used && sf->entries[i].pid == pid) {
			hasPid = true;
			break;
		}
	return hasPid;
}

t_swapFile* pidExists(uint32_t pid) {
	bool hasProcess(void *elem) {
		return swapFile_hasPid((t_swapFile*) elem, pid);
	}
	;
	t_swapFile *file = list_find(swapFiles, hasProcess);
	return file;
}

int swapFile_countPidPages(t_swapFile *sf, uint32_t pid) {
	int pages = 0;
	for (int i = 0; i < sf->maxPages; i++)
		if (sf->entries[i].used && sf->entries[i].pid == pid)
			pages++;
	return pages;
}

void swapFile_register(t_swapFile *sf, uint32_t pid, int32_t pageNumber,
		int index) {
	sf->entries[index].used = true;
	sf->entries[index].pid = pid;
	sf->entries[index].pageNumber = pageNumber;
}

// Encuentra el indice de comienzo de un chunk libre (acorde a asignacion fija)
int findFreeChunk(t_swapFile *sf) {
	for (int i = 0; i < sf->maxPages; i += memoryConfig->framesPerProcess) {
		if (!(sf->entries[i].used)) {
			return i;
		}
	}
	return -1;
}

// Encuentra el indice de comienzo del chunk donde se encuentra PID (acorde a asignacion fija)
int getChunk(t_swapFile *sf, uint32_t pid) {
	int i = 0;
	while (sf->entries[i].pid != pid) {
		if (i >= sf->maxPages)
			return -1;
		i += memoryConfig->framesPerProcess;
	}
	if (!sf->entries[i].used)
		return -1;
	return i;
}

// Se fija si en un dado swapFile hay un "chunk" libre (acorde a asignacion fija)
bool hasFreeChunk(t_swapFile* sf){
    bool hasFreeChunk = false;
    for(int i = 0; i < sf->maxPages; i += memoryConfig->framesPerProcess)
        if(!sf->entries[i].used){
            hasFreeChunk = true;
            break;
        }
    return hasFreeChunk;
}


// Algoritmo de asignacion fija.
// Si el proceso existe en algun archivo, y hay lugar para una pagina mas en su chunk, se asigna.
// Si el proceso existe en algun archivo, y no hay mas lugar, no se asigna.
// Si el proceso NO existe en ningun archivo, pero hay un archivo con un chunk disponible, se asigna.
// Si el proceso NO existe en ningun archivo, y no hay ningun archivo con un chunk disponible, no se asigna.
/*bool fija(uint32_t pid, int32_t page, void* pageContent){
    t_swapFile* file = pidExists(pid);
    bool _hasFreeChunk(void* elem){
        return hasFreeChunk((t_swapFile*)elem);
    };
    if (file == NULL)
        file = list_find(swapFiles, _hasFreeChunk);
    if (file == NULL)
        return false;

    int assignedIndex = swapFile_getIndex(file, pid, page);
    if (assignedIndex == -1){
        int base = getChunk(file, pid);
        if (base == -1) base = findFreeChunk(file);
        if (base == -1) return false;
        int offset = swapFile_countPidPages(file, pid);
        if (offset == config->framesPerProcess) return false;
        assignedIndex = base + offset;
    }
    swapFile_writeAtIndex(file, assignedIndex, pageContent);
    swapFile_register(file, pid, page, assignedIndex);

    log_info(logger, "Archivo %s: se almaceno la pagina %i del proceso %u en el indice %i", file->path, page, pid, assignedIndex);

    return true;
}*/

/*uint32_t clock_m_alg(int32_t start, int32_t end){
    int32_t frame = getFreeFrame(start, end);

    if (frame != -1) {
        return frame;
    }

    uint32_t total = end - start;

    pthread_mutex_lock(&metadataMut);
    uint32_t *counter = metadata->firstFrame ? &(metadata->clock_m_counter[start / memoryConfig->framesPerProcess]) : &clock_m_counter;

    while(1){
        for (uint32_t i = 0; i < total; i++){
            if (!metadata->entries[*counter].u && !metadata->entries[*counter].modified){
                pthread_mutex_unlock(&metadataMut);
                frame = *counter;
                *counter = start + ((*counter + 1) % total);
                return frame;
            }
            *counter = start + ((*counter + 1) % total);
        }
        for (uint32_t i = 0; i < total; i++){
            if (! metadata->entries[*counter].u){
                pthread_mutex_unlock(&metadataMut);
                frame = *counter;
                *counter = start + ((*counter + 1) % total);
                return frame;
            }
            metadata->entries[*counter].u = false;
            *counter = start + ((*counter + 1) % total);
        }
    }
}*/


uint32_t clock_alg(int32_t start, int32_t end){
    

}

