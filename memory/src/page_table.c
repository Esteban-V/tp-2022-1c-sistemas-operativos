#include "page_table.h"

// Crea tabla de nivel 1 y tablas de nivel 2 correspondientes, y retorna el indice de la 1era
int page_table_init(uint32_t process_size)
{
	// Creacion tabla nivel 1
	t_ptbr1 *level1_table = malloc(sizeof(t_ptbr1));
	level1_table->entries = list_create();

	// Se agrega a lista global 1 y se obtiene su posicion en la misma para retornar
	int level1_index = list_add(level1_tables, level1_table);

	// Cantidad de tablas de 2do nivel necesarias segun tamaño del proceso
	int pages_required = process_size / (memoryConfig->pageSize);
	// ceil = (a / b) + ((a % b) != 0)
	int level2_pages_required = (pages_required / memoryConfig->entriesPerTable) + ((pages_required % memoryConfig->entriesPerTable) != 0);

	// Creacion tablas nivel 2
	for (int i = 0; i < level2_pages_required; i++)
	{
		// Creacion tabla nivel 2
		t_ptbr2 *level2_table = malloc(sizeof(t_ptbr2));
		level2_table->entries = list_create();

		// Creacion paginas por tabla de nivel 2
		for (int j = 0; j < memoryConfig->entriesPerTable; j++)
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

	return level1_index;
}

// Retorna la tabla de nivel 1 segun su indice en su lista global
t_ptbr1 *get_page_table1(uint32_t pt1_index)
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
t_ptbr2 *get_page_table2(uint32_t pt2_index)
{
	// Obtiene la tabla de nivel 2
	t_ptbr2 *level2_table = (t_ptbr2 *)list_get(level2_tables, pt2_index);
	return level2_table;
}

// Retorna el numero de frame en memoria (cargandola previamente si fuese necesario)
int get_frame_number(uint32_t pt2_index, uint32_t entry_index)
{
	// Obtiene la tabla de nivel 2
	t_ptbr2 *level2_table = (t_ptbr2 *)list_get(level2_tables, pt2_index);
	// Obtiene la pagina con sus bits de estado
	t_page_entry *entry = (t_page_entry *)list_get(level2_table->entries, entry_index);

	int frame = -1;
	if (entry->present)
	{
		frame = entry->frame;
	}
	else
	{
		if (can_assign_frame(level2_table->entries))
		{
			// - Cargar la pagina en un frame (cual?)

			// TODO: Definir de donde se elije el frame a asignarle

			// Opcion 1
			// Tener una pool de frames por proceso ("elegidas" en process_new/page_table_init)
			// y almacenar cual de esas esta libre
			// Se elije la primera libre
			/// Mirar implementacion de fija_memoria (es medio bardo, nos la complica)

			// Opcion 2
			// Tener un bitmap de frames libres que se actualiza todo el tiempo
			// Se elije de aquellos frames libres "globales", es diferente cada vez
			// y en diferentes momentos ese frame esta reservado por diferentes procesos sin casarse a ninguno
		}
		else
		{
			// - Identificar que pagina present==true reemplazar segun algoritmo
		}

		// - Actualizar datos de la entry de su tabla nivel 2 (y de la reemplazada si la hubo)
		// - Obtener frame en donde se guardo la pagina pedida (luego de la carga o reemplazo)
	}

	// Retorna que frame le corresponde, ahora que ya esta cargada en memoria
	return frame;
}

bool can_assign_frame(t_list *entries)
{
	bool _page_is_present(void *entry)
	{
		return ((t_page_entry *)entry)->present == true;
	};

	int cant_present = list_count_satisfying(entries, _page_is_present);
	return cant_present < memoryConfig->framesPerProcess;
}

// Retorna un puntero al comienzo del frame en memoria
void *get_frame(uint32_t frame_number)
{
	void *mem_ptr = memory->memory;
	int frame_index = frame_number * memoryConfig->pageSize;
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

	pthread_mutex_lock(&pageTablesMut);
	list_add(table->entries, entry);
	pthread_mutex_unlock(&pageTablesMut);

	return list_size(table->entries);
}

//////////////
bool isFree(int frame_number)
{
	pthread_mutex_lock(&metadataMut);
	bool free = (metadata->entries)[frame_number].isFree;
	pthread_mutex_unlock(&metadataMut);
	return free;
}

uint32_t getFreeFrame(uint32_t start, uint32_t end)
{
	for (uint32_t i = start; i < end; i++)
	{
		if (isFree(i))
			return i;
	}
	return -1;
}

void *readPage(uint32_t dir)
{
	// leer
}

bool savePage(uint32_t pid, uint32_t pageNumber, void *pageContent)
{
	void *pageData = NULL;
	bool rc = fija_swap(pid, pageNumber, pageContent);

	free(pageData);

	return rc;
}

/* void writeFrame(t_memory *mem, uint32_t frame, void *from)
{
	void *frameAddress = memory_getFrame(mem, frame);

	pthread_mutex_lock(&metadataMut);
	metadata->entries[frame].modified = true;
	metadata->entries[frame].u = true;
	pthread_mutex_unlock(&metadataMut);

	pthread_mutex_lock(&memoryMut);
	memcpy(frameAddress, from, memoryConfig->pageSize);
	pthread_mutex_unlock(&memoryMut);
} */

/* uint32_t replace(uint32_t victim, uint32_t PID, uint32_t pt1_entry,
				 uint32_t pt2_entry, uint32_t page)
{
	// Traer pagina pedida de swap.
	void *pageFromSwap = readPage(page); // creo q read page

	// Chequear que se haya podido traer
	if (pageFromSwap == NULL)
	{
		pthread_mutex_lock(&mutex_log);
		log_error(logger, "Cannot Load Page #%u ; Process #%u", page, PID);
		pthread_mutex_unlock(&mutex_log);
		return -1;
	}

	// Si el frame no esta libre se envia a swap la pagina que lo ocupa.
	// Esto es para que replace() se pueda utilizar tanto para cargar paginas a frames libres como para reemplazar.
	if (!isFree(victim))
	{

		pthread_mutex_lock(&mutex_log);
		log_info(logger, "SWAP");
		pthread_mutex_unlock(&mutex_log);

		// TODO determinar file size
		list_add(swapFiles, swapFile_create(memoryConfig->swapPath, PID,
											4096, memoryConfig->pageSize));

		usleep(memoryConfig->swapDelay);

		// Enviar pagina reemplazada a swap.
		pthread_mutex_lock(&metadataMut);
		uint32_t victimPID = (metadata->entries)[victim].PID;
		uint32_t victimPage = (metadata->entries)[victim].page;
		bool modified = (metadata->entries)[victim].modified;
		pthread_mutex_unlock(&metadataMut);

		// dropTLBEntry(victimPID, victimPage); TODO lo hace CPU, supongo que en este momento debemos pasarle la data

		pthread_mutex_lock(&memoryMut);
		if (modified)
			savePage(victimPID, victimPage, memory_getFrame(memory, victim));
		pthread_mutex_unlock(&memoryMut);

		// Modificar tabla de paginas del proceso cuya pagina fue reemplazada.
		pthread_mutex_lock(&pageTablesMut);
		t_ptbr2 *ptReemplazado = getPageTable2(victimPID, pt1_entry,
											   pageTables);
		((t_page_entry *)list_get(ptReemplazado->entries, victimPage))->present =
			false; // TODO cambiar victim page por sus entries
		((t_page_entry *)list_get(ptReemplazado->entries, victimPage))->frame =
			-1;
		pthread_mutex_unlock(&pageTablesMut);

		pthread_mutex_lock(&mutex_log);
		log_info(logger,
				 "Replacement: Frame #%u: ; Page #%u ; Process #%u ;; Victim Page #%u ; Process #%u.",
				 victim, page, PID, victimPage, victimPID);
		pthread_mutex_unlock(&mutex_log);
	}
	else
	{
		pthread_mutex_lock(&mutex_log);
		log_info(logger, "Assignment: Frame #%u: ; Page #%u ; Process #%u.",
				 victim, page, PID);
		pthread_mutex_unlock(&mutex_log);
	}

	// Escribir pagina traida de swap a memoria.
	writeFrame(memory, victim, pageFromSwap);
	free(pageFromSwap);
	// Modificar tabla de paginas del proceso cuya pagina entra a memoria.
	pthread_mutex_lock(&pageTablesMut);
	t_ptbr2 *ptReemplaza = getPageTable2(PID, pt1_entry, pageTables);
	((t_page_entry *)list_get(ptReemplaza->entries, pt2_entry))->present = true;
	((t_page_entry *)list_get(ptReemplaza->entries, pt2_entry))->frame = victim;
	pthread_mutex_unlock(&pageTablesMut);

	// addTLBEntry(PID, page, victim); TODO lo hace CPU, supongo que en este momento debemos pasarle la data

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

uint32_t swapPage(uint32_t PID, uint32_t pt1_entry, uint32_t pt2_entry,
				  uint32_t page)
{

	uint32_t start, end;
	fija_memoria(&start, &end, PID);

	uint32_t frameVictima = replace_algo(start, end);

	return replace(frameVictima, PID, pt1_entry, pt2_entry, page);
}

bool fija_memoria(uint32_t *start, uint32_t *end, uint32_t PID)
{
	*start = -1;

	pthread_mutex_lock(&metadataMut);
	for (uint32_t i = 0;
		 i < memoryConfig->framesInMemory / memoryConfig->framesPerProcess; i++)
	{
		if ((metadata->firstFrame)[i] == PID)
		{
			*start = i * memoryConfig->framesPerProcess;
			break;
		}
	}

	if (*start == -1)
	{
		for (uint32_t i = 0;
			 i < memoryConfig->framesInMemory / memoryConfig->framesPerProcess;
			 i++)
		{
			if ((metadata->firstFrame)[i] == -1)
			{
				*start = i * memoryConfig->framesPerProcess;
				(metadata->firstFrame)[i] = PID;
				break;
			}
		}
	}

	pthread_mutex_unlock(&metadataMut);

	if (*start == -1)
	{
		pthread_mutex_lock(&mutex_log);
		log_debug(logger, "Could Not Find Available Frames for Process #%u.",
				  PID);
		pthread_mutex_unlock(&mutex_log);
		*end = -1;
		return false;
	}

	*end = *start + memoryConfig->framesPerProcess;

	return true;
}
 */
uint32_t clock_m_alg(uint32_t start, uint32_t end)
{
	uint32_t frame = getFreeFrame(start, end);

	if (frame != -1)
	{
		return frame;
	}

	uint32_t total = end - start;

	pthread_mutex_lock(&metadataMut);
	uint32_t *counter =
		metadata->firstFrame ? &(metadata->clock_m_counter[start / memoryConfig->framesPerProcess]) : &clock_m_counter;

	while (1)
	{
		for (uint32_t i = 0; i < total; i++)
		{
			if (!metadata->entries[*counter].u && !metadata->entries[*counter].modified)
			{
				pthread_mutex_unlock(&metadataMut);
				frame = *counter;
				*counter = start + ((*counter + 1) % total);
				return frame;
			}
			*counter = start + ((*counter + 1) % total);
		}
		for (uint32_t i = 0; i < total; i++)
		{
			if (!metadata->entries[*counter].u)
			{
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

	if (frame != -1)
	{
		return frame;
	}

	uint32_t total = end - start;

	pthread_mutex_lock(&metadataMut);
	uint32_t *counter =
		metadata->firstFrame ? &(metadata->clock_counter[start / memoryConfig->framesPerProcess]) : &clock_counter; // TODO chequear

	while (1)
	{
		for (uint32_t i = 0; i < total; i++)
		{
			if (!metadata->entries[*counter].u)
			{
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
