#include "page_table.h"

t_ptbr1 *initializePageTable()
{
	t_ptbr1 *newTable = malloc(sizeof(t_ptbr1));
	newTable->entryQuantity = 0;
	newTable->entries = NULL;
	newTable->tableNumber = pageTable_number;

	pageTable_number++;

	return newTable;
}

void destroyPageTable(t_ptbr1 *table){ // TODO Acomodar, posiblemente agregar lo mismo para ptbr2
    free(table->entries);
    free(table);
}

void _destroyPageTable(void *table){
    destroyPageTable((t_ptbr1*) table);
}

void page_table_destroy(t_ptbr1 *table)
{
	free(table->entries);
	free(table);
}

void _page_table_destroy(void *table)
{
	page_table_destroy((t_ptbr1 *)table);
}

/*void pageTable_destroyLastEntry(t_ptbr1 *pt)
{
	pthread_mutex_lock(&pageTablesMut);
	(pt->entryQuantity)--;
	pt->entries = realloc(pt->entries,
						  sizeof(t_page_entry) * (pt->entryQuantity));
	pthread_mutex_unlock(&pageTablesMut);
}*/

uint32_t pageTableAddEntry(t_ptbr2 *table, uint32_t newFrame)
{
	pthread_mutex_lock(&pageTablesMut);
	table->entries = realloc(table->entries,
							 sizeof(t_page_entry) * (table->pageQuantity + 1));

	t_page_entry *entry = list_get(table->entries, table->pageQuantity);
	entry->frame = newFrame;
	entry->present = false;

	uint32_t pgQty = table->pageQuantity;
	pthread_mutex_unlock(&pageTablesMut);

	(table->pageQuantity)++;
	return pgQty;
}

t_ptbr1 *getPageTable(uint32_t _PID, t_dictionary *pageTables)
{
	char *PID = string_itoa(_PID);
	t_ptbr1 *pt = (t_ptbr1 *)dictionary_get(pageTables, PID);
	free(PID);
	return pt;
}

t_ptbr2 *getPageTable2(uint32_t PID, uint32_t pt1_entry, t_dictionary *pageTables)
{
	pthread_mutex_lock(&pageTablesMut);
		t_ptbr1 *pt = getPageTable(PID, pageTables);
		t_ptbr2* pt2 = (t_ptbr2*) list_get(pt->entries, pt1_entry);
	pthread_mutex_unlock(&pageTablesMut);

	return pt2;
}

bool pageTable1_isEmpty(uint32_t PID)
{
	pthread_mutex_lock(&pageTablesMut);
		t_ptbr1 *pt = getPageTable(PID, pageTables);
		bool isEmpty = pt->entryQuantity == 0;
	pthread_mutex_unlock(&pageTablesMut);

	return isEmpty;
}

/*bool pageTable_isEmpty2(uint32_t PID)
{
	t_ptbr1 *pt = getPageTable(PID, pageTables);
	pthread_mutex_lock(&pageTablesMut);
	bool isEmpty = pt->entryQuantity == 0;
	pthread_mutex_unlock(&pageTablesMut);
	return isEmpty;
}*/

uint32_t pageTable_getFrame(uint32_t PID, uint32_t pt1_entry, uint32_t pt2_entry)
{
	uint32_t frame;

	pthread_mutex_lock(&pageTablesMut);
		t_ptbr2 *pt2 = getPageTable2(PID, pt1_entry, pageTables);
		frame = pt2_entry < pt2->pageQuantity ? ((t_page_entry*)list_get(pt2->entries, pt2_entry))->frame : -1;
	pthread_mutex_unlock(&pageTablesMut);

	return frame;
}

//////////////
bool isFree(uint32_t frame){
    pthread_mutex_lock(&metadataMut);
        bool free = (metadata->entries)[frame].isFree;
    pthread_mutex_unlock(&metadataMut);
    return free;
}

int32_t getFreeFrame(int32_t start, int32_t end){
    for(uint32_t i = start; i < end; i++){
        if(isFree(i)) return i;
    }
    return -1;
}

void* readPage(uint32_t pid, uint32_t pageNumber){

    t_swapFile* file = pidExists(pid);

    if(file == NULL){

    	pthread_mutex_lock(&mutex_log);
    	    	log_info(logger, "File PID Not Found");
    	pthread_mutex_unlock(&mutex_log);

        return 0;
    }
    int index = swapFile_getIndex(file, pid, pageNumber);
    if(index == -1){

    	pthread_mutex_lock(&mutex_log);
    		log_info(logger, "File Index Out of Bounds");
    	pthread_mutex_unlock(&mutex_log);

        return 0;
    }

    pthread_mutex_lock(&mutex_log);
    	log_info(logger, "File %s: Page %i Recovered ; Process %i ; Index %i", file->path, pageNumber, pid, index);
    pthread_mutex_unlock(&mutex_log);

    return swapFile_readAtIndex(file, index); //Page Data

}

bool savePage(uint32_t pid, int32_t pageNumber, void* pageContent){

    void* pageData = NULL;

    bool rc = fija_swap(pid, pageNumber, pageContent);

    free(pageData);

    return rc;
}

void writeFrame(t_memory *mem, uint32_t frame, void *from){
    void *frameAddress = memory_getFrame(mem, frame);

    pthread_mutex_lock(&metadataMut);
        metadata->entries[frame].modified = true;
        metadata->entries[frame].u = true;
    pthread_mutex_unlock(&metadataMut);

    pthread_mutex_lock(&memoryMut);
        memcpy(frameAddress, from, memoryConfig->pageSize);
    pthread_mutex_unlock(&memoryMut);
}

void *memory_getFrame(t_memory *mem, uint32_t frame){
        void* ptr = mem->memory + frame * memoryConfig->pageSize;
    return ptr;
}

uint32_t replace(uint32_t victim, uint32_t PID, uint32_t pt1_entry,uint32_t pt2_entry, uint32_t page){
    // Traer pagina pedida de swap.
    void *pageFromSwap = readPage(PID, page); //creo q read page

    // Chequear que se haya podido traer
    if (pageFromSwap == NULL){
        pthread_mutex_lock(&mutex_log);
            log_error(logger, "Cannot Load Page #%u ; Process #%u", page, PID);
        pthread_mutex_unlock(&mutex_log);
        return -1;
    }

    // Si el frame no esta libre se envia a swap la pagina que lo ocupa.
    // Esto es para que replace() se pueda utilizar tanto para cargar paginas a frames libres como para reemplazar.
    if (!isFree(victim)){

    	for (int i = 0; i < memoryConfig->swapDelay; i++) {
    		usleep(1000);
    	}

        // Enviar pagina reemplazada a swap.
        pthread_mutex_lock(&metadataMut);
            uint32_t victimPID  = (metadata->entries)[victim].PID;
            uint32_t victimPage = (metadata->entries)[victim].page;
            bool modified = (metadata->entries)[victim].modified;
        pthread_mutex_unlock(&metadataMut);

        //dropTLBEntry(victimPID, victimPage); TODO lo hace CPU, supongo que en este momento debemos pasarle la data

        pthread_mutex_lock(&memoryMut);
        	if(modified) savePage(victimPID, victimPage, memory_getFrame(memory, victim));
        pthread_mutex_unlock(&memoryMut);

        // Modificar tabla de paginas del proceso cuya pagina fue reemplazada.
        pthread_mutex_lock(&pageTablesMut);
            t_ptbr2 *ptReemplazado = getPageTable2(victimPID, pt1_entry, pageTables);
            ((t_page_entry*)list_get(ptReemplazado->entries, victimPage))->present = false; //TODO cambiar victim page por sus entries
            ((t_page_entry*) list_get(ptReemplazado->entries, victimPage))->frame = -1;
        pthread_mutex_unlock(&pageTablesMut);

        pthread_mutex_lock(&mutex_log);
            log_info(logger, "Replacement: Frame #%u: ; Page #%u ; Process #%u ;; Victim Page #%u ; Process #%u.", victim, page, PID, victimPage, victimPID);
        pthread_mutex_unlock(&mutex_log);

    } else {
        pthread_mutex_lock(&mutex_log);
            log_info(logger, "Assignment: Frame #%u: ; Page #%u ; Process #%u.", victim, page, PID);
        pthread_mutex_unlock(&mutex_log);
    }

    // Escribir pagina traida de swap a memoria.
    writeFrame(memory, victim, pageFromSwap);
    free(pageFromSwap);
    // Modificar tabla de paginas del proceso cuya pagina entra a memoria.
    pthread_mutex_lock(&pageTablesMut);
    	t_ptbr2 *ptReemplaza = getPageTable2(PID, pt1_entry, pageTables);
        ((t_page_entry*)list_get(ptReemplaza->entries, pt2_entry))->present = true;
        ((t_page_entry*) list_get(ptReemplaza->entries, pt2_entry))->frame = victim;
    pthread_mutex_unlock(&pageTablesMut);

    //addTLBEntry(PID, page, victim); TODO lo hace CPU, supongo que en este momento debemos pasarle la data

    // Modificar frame metadata.
    pthread_mutex_lock(&metadataMut);
        (metadata->entries)[victim].page = page;
        (metadata->entries)[victim].PID = PID;
        (metadata->entries)[victim].isFree = false;
        (metadata->entries)[victim].modified = false;
        (metadata->entries)[victim].u = true;
    pthread_mutex_unlock(&metadataMut);

    return victim;
}

uint32_t swapPage(uint32_t PID, uint32_t pt1_entry, uint32_t pt2_entry, uint32_t page){

    uint32_t start, end;
    fija_memoria(&start, &end, PID);

    uint32_t frameVictima = replace_algo(start, end);

    return replace(frameVictima, PID, pt1_entry, pt2_entry, page);
}

void swapFile_clearAtIndex(t_swapFile* sf, int index){
    void* mappedFile = mmap(NULL, sf->size, PROT_READ|PROT_WRITE, MAP_SHARED, sf->fd, 0);
    memset(mappedFile + index * sf->pageSize, 0, sf->pageSize);
    msync(mappedFile, sf->size, MS_SYNC);
    munmap(mappedFile, sf->size);
}

void swapFile_writeAtIndex(t_swapFile* sf, int index, void* pagePtr){
    void* mappedFile = mmap(NULL, sf->size, PROT_READ|PROT_WRITE,MAP_SHARED, sf->fd, 0);
    memcpy(mappedFile + index * sf->pageSize, pagePtr, sf->pageSize);
    msync(mappedFile, sf->size, MS_SYNC);
    munmap(mappedFile, sf->size);
}

t_swapFile *swapFile_create(char *path, int pageSize)
{
	t_swapFile *self = malloc(sizeof(t_swapFile));
	self->path = string_duplicate(path);
	self->pageSize = pageSize;
	self->maxPages = memoryConfig->memorySize / pageSize;

	 self->fd = open(self->path, O_RDWR | O_CREAT, S_IRWXU);
	 ftruncate(self->fd, self->size);
	 void* mappedFile = mmap(NULL, self->size, PROT_READ|PROT_WRITE,MAP_SHARED, self->fd, 0);
	 memset(mappedFile, 0, self->size);
	 msync(mappedFile, self->size, MS_SYNC);
	 munmap(mappedFile, self->size);


	 self->entries = calloc(1, sizeof(t_metadata) * self->maxPages);
	 for(int i = 0; i < self->maxPages; i++)
	 self->entries[i].used = false;

	return self;
}

uint32_t createPage(uint32_t PID, uint32_t pt1_entry)
{
	pthread_mutex_lock(&pageTablesMut);
		t_ptbr2 *table = getPageTable2(PID, pt1_entry, pageTables);
	pthread_mutex_unlock(&pageTablesMut);

	uint32_t pageNumber = pageTableAddEntry(table, 0); //TODO no se xq 0

	pthread_mutex_lock(&mutex_log);
		log_info(logger, "Page %i Created ; Process %u.", pageNumber, PID);
	pthread_mutex_unlock(&mutex_log);

	void *newPageContents = calloc(1, memoryConfig->pageSize);
	if(savePage(PID, pageNumber, newPageContents)){
		free(newPageContents);
		return pageNumber;
	}
	free(newPageContents);

	return -1;
}

int swapFile_getIndex(t_swapFile* sf, uint32_t pid, int32_t pageNumber){
    int found = -1;
    for(int i = 0; i < sf->maxPages; i++){
        if(sf->entries[i].pid == pid && sf->entries[i].pageNumber == pageNumber){
            found = i;
            break;
        }
    }

    return found;
}

void* swapFile_readAtIndex(t_swapFile* sf, int index){
    void* mappedFile = mmap(NULL, sf->size, PROT_READ|PROT_WRITE,MAP_SHARED, sf->fd, 0);
    void* pagePtr = malloc(sf->pageSize);
    memcpy(pagePtr, mappedFile + index * sf->pageSize, sf->pageSize);
    munmap(mappedFile, sf->size);
    return pagePtr;
}

bool swapFile_hasPid(t_swapFile* sf, uint32_t pid){
    bool hasPid = false;
    for(int i = 0; i < sf->maxPages; i++)
        if(sf->entries[i].used && sf->entries[i].pid == pid){
            hasPid = true;
            break;
    }

    return hasPid;
}

t_swapFile *pidExists(uint32_t pid)
{
	bool hasProcess(void *elem)
	{
		return swapFile_hasPid((t_swapFile *)elem, pid);
	};

	t_swapFile *file = list_find(swapFiles, hasProcess);

	return file;
}

int swapFile_countPidPages(t_swapFile* sf, uint32_t pid){
    int pages = 0;
    for(int i = 0; i < sf->maxPages; i++)
        if(sf->entries[i].used && sf->entries[i].pid == pid)
            pages++;

    return pages;
}

void swapFile_register(t_swapFile* sf, uint32_t pid, int32_t pageNumber, int index){
    sf->entries[index].used = true;
    sf->entries[index].pid = pid;
    sf->entries[index].pageNumber = pageNumber;
}

// Encuentra el indice de comienzo de un chunk libre (acorde a asignacion fija)
int findFreeChunk(t_swapFile* sf){
    for(int i = 0; i < sf->maxPages; i += memoryConfig->framesPerProcess){
        if(!(sf->entries[i].used)){
            return i;
        }
    }

    return -1;
}

// Encuentra el indice de comienzo del chunk donde se encuentra PID (acorde a asignacion fija)
int getChunk(t_swapFile* sf, uint32_t pid){
    int i = 0;
    while(sf->entries[i].pid != pid){
        if(i >= sf->maxPages) return -1;
        i += memoryConfig->framesPerProcess;
    }
    if(!sf->entries[i].used) return -1;

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

bool fija_memoria(int32_t *start, int32_t *end, uint32_t PID){
    *start = -1;

    pthread_mutex_lock(&metadataMut);
        for (uint32_t i = 0 ; i < memoryConfig->frameQty / memoryConfig->framesPerProcess ; i++){
            if ((metadata->firstFrame)[i] == PID){
                *start = i * memoryConfig->framesPerProcess;
                break;
            }
        }

        if (*start == -1){
            for (uint32_t i = 0 ; i < memoryConfig->frameQty / memoryConfig->framesPerProcess ; i++){
                if ((metadata->firstFrame)[i] == -1){
                    *start = i * memoryConfig->framesPerProcess;
                    (metadata->firstFrame)[i] = PID;
                    break;
                }
            }
        }

    pthread_mutex_unlock(&metadataMut);

    if (*start == -1) {
        pthread_mutex_lock(&mutex_log);
            log_debug(logger, "Could Not Find Available Frames for Process #%u.", PID);
        pthread_mutex_unlock(&mutex_log);
        *end = -1;
        return false;
    }

    *end = *start + memoryConfig->framesPerProcess;

    return true;
}

// Algoritmo de asignacion fija.
// Si proceso existe en algun archivo && hay lugar para una pagina mas en su chunk --> se asigna
// Si proceso existe en algun archivo && no hay mas lugar --> no se asigna
// Si proceso NO existe en ningun archivo && hay un archivo con un chunk disponible --> se asigna
// Si proceso NO existe en ningun archivo && no hay ningun archivo con un chunk disponible --> no se asigna

bool fija_swap(uint32_t pid, uint32_t page, void* pageContent){
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
		if (offset == memoryConfig->framesPerProcess) return false;
		assignedIndex = base + offset;
	}
	swapFile_writeAtIndex(file, assignedIndex, pageContent);
	swapFile_register(file, pid, page, assignedIndex);

	pthread_mutex_lock(&mutex_log);
		log_info(logger, "File %s: Page %u Saved ; Process %u ; Index %u", file->path, page, pid, assignedIndex);
	pthread_mutex_lock(&mutex_log);

	return true;
}

uint32_t clock_m_alg(uint32_t start, uint32_t end){
	uint32_t frame = getFreeFrame(start, end);

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
}

uint32_t clock_alg(uint32_t start, uint32_t end)
{
	uint32_t frame = getFreeFrame(start, end);

		if (frame != -1) {
			return frame;
		}

		uint32_t total = end - start;

		pthread_mutex_lock(&metadataMut);
		uint32_t *counter = metadata->firstFrame ? &(metadata->clock_counter[start / memoryConfig->framesPerProcess]) : &clock_counter; // TODO chequear

		while(1){
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
}

void destroy_swap_page(uint32_t pid, uint32_t page, int socket){
    t_swapFile* file = pidExists(pid);

    if(file == NULL){
        t_packet* response = create_packet(SWAP_ERROR, 0);
        socket_send_packet(socket, response);
        packet_destroy(response);

        return;
    }

    int index = swapFile_getIndex(file, pid, page);

    if(index == -1){
        t_packet* response = create_packet(SWAP_ERROR, 0);
        socket_send_packet(socket, response);
        packet_destroy(response);

        return;
    }

    swapFile_clearAtIndex(file, index);
    file->entries[index].used = false;

    t_packet* response = create_packet(SWAP_OK, 0);
    socket_send_packet(socket, response);
    packet_destroy(response);

    pthread_mutex_lock(&mutex_log);
    	log_info(logger, "File %s: Page %i Erased ; Process %u", file->path, page, pid);
    pthread_mutex_unlock(&mutex_log);
}
