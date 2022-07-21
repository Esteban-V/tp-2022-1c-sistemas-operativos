#include "page_table.h"

// Crea tabla de nivel 1 y tablas de nivel 2 correspondientes, y retorna el indice de la 1era
int page_table_init(uint32_t process_size, int level1_index, int frames_index)
{
	// Creacion tabla nivel 1
	t_ptbr1 *level1_table = malloc(sizeof(t_ptbr1));
	level1_table->entries = list_create();

	// Se agrega a lista global 1 y se obtiene su posicion en la misma para retornar
	level1_index = list_add(level1_tables, level1_table);

	// Cantidad de tablas de 2do nivel necesarias segun tamaño del proceso
	int pages_required = process_size / (config->pageSize);
	int level2_pages_required = ceil_div(pages_required, config->entriesPerTable);

	// Creacion tablas nivel 2
	for (int i = 0; i < level2_pages_required; i++)
	{
		// Creacion tabla nivel 2
		t_ptbr2 *level2_table = malloc(sizeof(t_ptbr2));
		level2_table->entries = list_create();

		// Creacion paginas por tabla de nivel 2
		for (int j = 0; j < config->entriesPerTable; j++)
		{
			// Creacion pagina
			t_page_entry *page = malloc(sizeof(t_page_entry));
			page->frame = -1;
			page->present = false;
			page->modified = false;
			page->used = false;

			// Se agrega a la lista de entradas de su tabla nivel 2
			list_add(level2_table->entries, page);
		}

		// Se agrega a lista global 2 y se obtiene su posicion en la misma...
		int level2_index = list_add(level2_tables, level2_table);
		// para agregar a la lista de entradas de su tabla nivel 1
		list_add(level1_table->entries, level2_index);
	}

	t_list *frames = list_create();
	// Asignacion de frames
	for (int i = 0; i < config->framesPerProcess; i++)
	{
		// Creacion pagina
		t_frame_entry *frame_entry = malloc(sizeof(t_frame_entry));

		int frame_index = find_first_free_frame(frames_bitmap);
		if (frame_index != -1)
		{
			// Se le asigna una frame libre
			frame_entry->frame = frame_index;
			frame_entry->page_data = NULL;
			list_add(frames, frame_entry);
		}
		else
		{
			// No encuentra frame libre :(
		}
	}

	t_process_frame *process_frame = malloc(sizeof(t_process_frame));
	process_frame->frames = frames;
	process_frame->clock_hand = 0;

	frames_index = list_add(process_frames, process_frame);
}

// Retorna la tabla de nivel 1 segun su indice en su lista global
t_ptbr1 *get_page_table1(int pt1_index)
{
	// Obtiene la tabla de nivel 1
	t_ptbr1 *level1_table = (t_ptbr1 *)list_get(level1_tables, pt1_index);
	return level1_table;
}

// Retorna el indice de tabla nivel 2 en la lista global 2, correspondiente a la entry en el index (entry_index) de la tabla nivel 1, que esta en el index (pt1_index) de la lista global 1
int get_page_table2_index(uint32_t pt1_index, uint32_t entry_index)
{
	// Obtiene la tabla de nivel 1
	t_ptbr1 *level1_table = (t_ptbr1 *)list_get(level1_tables, pt1_index);
	// Obtiene el index de pt2 en lista global
	int pt2_index = *(int *)list_get(level1_table->entries, entry_index);
	return pt2_index;
}

// Retorna la tabla de nivel 2 segun su indice en su lista global
t_ptbr2 *get_page_table2(int pt2_index)
{
	// Obtiene la tabla de nivel 2
	t_ptbr2 *level2_table = (t_ptbr2 *)list_get(level2_tables, pt2_index);
	return level2_table;
}

// Retorna el numero de frame en memoria (cargandola previamente si fuese necesario)
int get_frame_number(uint32_t pt2_index, uint32_t entry_index, uint32_t pid)
{
	// Obtiene la tabla de nivel 2
	t_ptbr2 *level2_table = (t_ptbr2 *)list_get(level2_tables, pt2_index);
	// Obtiene la pagina con sus bits de estado
	t_page_entry *entry = (t_page_entry *)list_get(level2_table->entries, entry_index);

	int frame = -1;
	if (entry->present)
	{
		// entry->used = true;
		frame = entry->frame;
		return frame;
	}
	/* else
	{
		// replaceAlgorithm
		// CLOCK = 0 --> false

		t_page_entry *pointerIterator = NULL; // dictionary_get(clock_pointers_dictionary,string_itoa(pid));//getPointer();//falta implementar
		t_page_entry *actualPointer = NULL;	  // dictionary_get(clock_pointers_dictionary,string_itoa(pid));//getPointer();//falta implementar

		// me falta bastante
		for (int i = 0; i < config->entriesPerTable; i++)
		{

			if (pointerIterator == NULL)
			{
				// asignFrame(entry);//falta implementar
				pointerIterator = entry;
				if (i + 1 == config->entriesPerTable)
				{
					entry->next = actualPointer;
					actualPointer->previous = entry;
				}
				return entry->frame;
			}
			entry->previous = pointerIterator;
			pointerIterator = pointerIterator->next;
		}
		while (1)
		{
			if (!strcmp(config->replaceAlgorithm, "CLOCK-M"))
			{
				for (int j = 0; j < config->entriesPerTable; j++)
				{
					if ((actualPointer->used == 0) && (actualPointer->modified == 0))
					{
						// swapearrrrrrrrrr
						actualPointer->present = 0;
						entry->frame = actualPointer->frame;
						actualPointer->next->previous = entry;
						actualPointer->previous->next = entry;
						entry->present = 1;
						entry->next = actualPointer->next;
						entry->previous = actualPointer->previous;
						return entry->frame;
					}
					actualPointer = actualPointer->next;
				}
			}
			for (int k = 0; k < config->entriesPerTable; k++)
			{
				if (actualPointer->used == 0)
				{
					// swapearrrrrrrrrr
					actualPointer->present = 0;
					entry->frame = actualPointer->frame;
					actualPointer->next->previous = entry;
					actualPointer->previous->next = entry;
					entry->present = 1;
					entry->next = actualPointer->next;
					entry->previous = actualPointer->previous;
					return entry->frame;
				}
				actualPointer->used = 0;
				actualPointer = actualPointer->next;
			}
		}
	} */
	return frame;
}

bool can_assign_frame(t_list *entries)
{
	bool _page_is_present(void *entry)
	{
		return ((t_page_entry *)entry)->present == true;
	};

	int cant_present = list_count_satisfying(entries, _page_is_present);
	return cant_present < config->framesPerProcess;
}

// Retorna un puntero al comienzo del frame en memoria
void *get_frame(uint32_t frame_number)
{
	void *mem_ptr = memory->memory;
	int frame_index = frame_number * config->pageSize;
	void *frame_ptr = mem_ptr + frame_index;
	return frame_ptr;
}

// Retorna el valor ubicado en el frame (con comienzo en frame_ptr) + un desplazamiento
uint32_t get_frame_value(void *frame_ptr, uint32_t offset)
{
	// Todos los valores a leer/escribir en memoria serán numéricos enteros no signados de 4 bytes
	uint32_t value;
	memcpy(frame_ptr + offset, &value, sizeof(uint32_t));
	return value;
}

// TODO: Revisar si necesita mas free()
void page_table_destroy(t_ptbr1 *table)
{
	free(table->entries);
	free(table);
}

// De aca en adelante estas funciones no se revisaron
uint32_t pageTableAddEntry(t_ptbr2 *table, uint32_t newFrame)
{
	t_page_entry *entry;
	entry->frame = newFrame;
	entry->present = false;

	list_add(table->entries, entry);

	return list_size(table->entries);
}

bool isFree(int frame_number)
{
	pthread_mutex_lock(&metadataMut);
	bool free = (metadata->entries)[frame_number].isFree;
	pthread_mutex_unlock(&metadataMut);
	return free;
}

int find_first_free_frame(t_bitarray *bitmap)
{
	for (int i = 0; i < config->framesInMemory; i++)
	{
		if (!bitarray_test_bit(bitmap, i))
		{
			return i;
		}
	}
	return -1;
}

/*void *readPage(uint32_t dir)
{
	// leer
}*/

void* readPage(uint32_t pid, uint32_t pageNumber){

	t_swap_file* file = pidExists(pid);

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

bool savePage(uint32_t pid, uint32_t pageNumber, void *pageContent)
{
	void *pageData = NULL;
	bool rc = fija_swap(pid, pageNumber, pageContent);

	free(pageData);

	return rc;
}

void *memory_getFrame(uint32_t frame){
	void* ptr = memory->memory + frame * config->pageSize;
	return ptr;
}

void writeFrame(uint32_t frame, void *from)
{
	void *frameAddress = memory_getFrame(frame);

	pthread_mutex_lock(&metadataMut);
	metadata->entries[frame].modified = true;
	metadata->entries[frame].u = true;
	pthread_mutex_unlock(&metadataMut);

	pthread_mutex_lock(&memoryMut);
	memcpy(frameAddress, from, config->pageSize);
	pthread_mutex_unlock(&memoryMut);
}

//asigna y reemplaza
uint32_t replace(uint32_t victim, uint32_t PID, uint32_t pt2_index, uint32_t page) // TODO quitar metadata y adaptar a la estructura nueva
{
	// Traer pagina pedida de swap.
	void *pageFromSwap = readPage(PID, page);

	// Chequear que se haya podido traer
	if (pageFromSwap == NULL)
	{
		log_error(logger, "Cannot Load Page #%u ; Process #%u", page, PID);

		return -1;
	}

	// Si el frame no esta libre se envia a swap la pagina que lo ocupa.
	// Esto es para que replace() se pueda utilizar tanto para cargar paginas a frames libres como para reemplazar.
	if (!isFree(victim))
	{

		log_info(logger, "SWAP");

		// TODO pasar tamanio del proceso para determinar el file size
		list_add(swapFiles, swapFile_create(config->swapPath, PID, /*filesize*/ 4096, config->pageSize));

		usleep(config->swapDelay * 1000);

		// Enviar pagina reemplazada a swap.
		pthread_mutex_lock(&metadataMut);
		uint32_t victimPID = (metadata->entries)[victim].PID;
		uint32_t victimPage = (metadata->entries)[victim].page;
		bool modified = (metadata->entries)[victim].modified;
		pthread_mutex_unlock(&metadataMut);

		// drop_tlb_entry(victimPID, victimPage); TODO pasar a CPU

		pthread_mutex_lock(&memoryMut);
		if (modified) savePage(victimPID, victimPage, memory_getFrame(victim));
		pthread_mutex_unlock(&memoryMut);

		// Modificar tabla de paginas del proceso cuya pagina fue reemplazada.
		t_ptbr2 *ptReemplazado = get_page_table2(pt2_index);
		((t_page_entry *)list_get(ptReemplazado->entries, victimPage))->present =
			false; // TODO cambiar victim page por sus entries
		((t_page_entry *)list_get(ptReemplazado->entries, victimPage))->frame =
			-1;

		log_info(logger,
				 "Replacement: Frame #%u: ; Page #%u ; Process #%u ;; Victim Page #%u ; Process #%u.",
				 victim, page, PID, victimPage, victimPID);
	}
	else
	{
		log_info(logger, "Assignment: Frame #%u: ; Page #%u ; Process #%u.",
				 victim, page, PID);
	}

	// Escribir pagina traida de swap a memoria.
	writeFrame(victim, pageFromSwap);
	free(pageFromSwap);

	// Modificar tabla de paginas del proceso cuya pagina entra a memoria.
	t_ptbr2 *ptReemplaza = get_page_table2(pt2_index);
	((t_page_entry *)list_get(ptReemplaza->entries, pt2_index))->present = true;
	((t_page_entry *)list_get(ptReemplaza->entries, pt2_index))->frame = victim;

	// add_tlb_entry(PID, page, victim); TODO pasar a CPU

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

// Tmb swapea
uint32_t assignPage(uint32_t PID, uint32_t pt2_index, uint32_t page)
{
	/*uint32_t start, end;
	fija_memoria(&start, &end, PID);

	uint32_t frameVictima = replaceAlgorithm(start, end);

	return replace(frameVictima, PID, pt2_index, page);*/
}

/*
bool fija_memoria(uint32_t *start, uint32_t *end, uint32_t PID)
{
	*start = -1;

	pthread_mutex_lock(&metadataMut);
	for (uint32_t i = 0 ; i < config->framesInMemory / config->framesPerProcess; i++)
	{
		if ((metadata->firstFrame)[i] == PID)
		{
			*start = i * config->framesPerProcess;
			break;
		}
	}

	if (*start == -1)
	{
		for (uint32_t i = 0;
			 i < config->framesInMemory / config->framesPerProcess;
			 i++)
		{
			if ((metadata->firstFrame)[i] == -1)
			{
				*start = i * config->framesPerProcess;
				(metadata->firstFrame)[i] = PID;
				break;
			}
		}
	}

	pthread_mutex_unlock(&metadataMut);

	if (*start == -1)
	{
		log_info(logger, "Could Not Find Available Frames for Process #%u.",
				  PID);

		*end = -1;
		return false;
	}

	*end = *start + config->framesPerProcess;

	return true;
}
*/
