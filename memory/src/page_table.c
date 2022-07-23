#include "page_table.h"

// Crea tabla de nivel 1 y tablas de nivel 2 correspondientes, y retorna el indice de la 1era
int page_table_init(uint32_t process_size, int *level1_index, int *process_frames_index)
{
	// Creacion tabla nivel 1
	t_ptbr1 *level1_table = malloc(sizeof(t_ptbr1));
	level1_table->entries = list_create();

	// Se agrega a lista global 1 y se obtiene su posicion en la misma para retornar
	*level1_index = list_add(level1_tables, level1_table);

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

	// Asignacion de frames fija
	t_list *frames = list_create();

	for (int i = 0; i < config->framesPerProcess; i++)
	{
		t_frame_entry *frame_entry = malloc(sizeof(t_frame_entry));
		int frame_index = find_first_unassigned_frame(frames_bitmap);
		if (frame_index != -1)
		{
			// Se marca el frame como asignado
			frame_set_assigned(frames_bitmap, frame_index);

			// Se le asigna un frame libre de memoria
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

	// Se agrega a lista global de frames y se obtiene su posicion en la misma para retornar
	*process_frames_index = list_add(process_frames, process_frame);
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
int get_frame_number(uint32_t pt2_index, uint32_t entry_index, uint32_t pid, uint32_t framesIndex)
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
	}else{

		t_process_frame* processFrames = list_get(process_frames,framesIndex);

		if(processFrames->clock_hand<config->framesPerProcess){
			entry->present=1;
			t_frame_entry* currentFrame=(t_frame_entry *)list_get(processFrames->frames,processFrames->clock_hand);
			currentFrame->page_data=entry;
			processFrames->clock_hand++;
			frame= currentFrame->frame;

			return frame;
		}else{
			while(1){
				if(!strcmp(config->replaceAlgorithm, "CLOCK-M")){
					for(int i=0;i<config->framesPerProcess;i++){
						t_frame_entry* currentFrame=(t_frame_entry *)list_get(processFrames->frames,processFrames->clock_hand%config->framesPerProcess);
						t_page_entry* pageInFrame =	currentFrame->page_data;
						processFrames->clock_hand++;

						if((pageInFrame->used==0) && (pageInFrame->modified==0)){
							pageInFrame->present=0;
							//SWAP
							//replace(currentFrame, PID, pt2_index, page);
							currentFrame->page_data=entry;
							frame=currentFrame->frame;

							return frame;
						}
					}
				}
				for(int i=0 ; i<config->framesPerProcess ; i++){
						t_frame_entry* currentFrame=(t_frame_entry *)list_get(processFrames->frames,processFrames->clock_hand%config->framesPerProcess);
						t_page_entry* pageInFrame =	currentFrame->page_data;
						processFrames->clock_hand++;

						if(pageInFrame->used == 0){
							pageInFrame->present=0;
							//SWAP
							//replace(currentFrame, PID, pt2_index, page);
							currentFrame->page_data=entry;
							frame=currentFrame->frame;

							return frame;
						}
						pageInFrame->used=0;
				}
			}
		}

	}
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

void *readPage(uint32_t pid, uint32_t pageNumber)
{

	t_swap_file *file = pidExists(pid);

	if (file == NULL)
	{
		pthread_mutex_lock(&mutex_log);
		log_info(logger, "File PID Not Found");
		pthread_mutex_unlock(&mutex_log);

		return 0;
	}

	int index = swapFile_getIndex(file, pid, pageNumber);

	if (index == -1)
	{
		pthread_mutex_lock(&mutex_log);
		log_info(logger, "File Index Out of Bounds");
		pthread_mutex_unlock(&mutex_log);

		return 0;
	}

	pthread_mutex_lock(&mutex_log);
	log_info(logger, "File %s: Page %i Recovered ; Process %i ; Index %i", file->path, pageNumber, pid, index);
	pthread_mutex_unlock(&mutex_log);

	return swapFile_readAtIndex(file, index); // Page Data
}

bool savePage(uint32_t pid, uint32_t pageNumber, void *pageContent)
{
	void *pageData = NULL;
	bool rc = fija_swap(pid, pageNumber, pageContent);

	free(pageData);

	return rc;
}

void *memory_getFrame(uint32_t frame)
{
	void *ptr = memory->memory + frame * config->pageSize;

	return ptr;
}

t_frame_entry* process_get_frame_entry(uint32_t pid, uint32_t frame) {
	t_process_frame* process_frames = (t_process_frame*) list_get(process_frames, pid);

	// TODO chequear que este bien el list_get
	return (t_frame_entry*) list_get(process_frames->frames, frame % config->entriesPerTable);
}

void writeFrame(uint32_t pid, uint32_t frame, void *from)
{
	void *frameAddress = memory_getFrame(frame);

	// TODO chequear que este bien, o si falta actualizar algo
	t_frame_entry* frame_entry = process_get_frame_entry(pid, frame);
	pthread_mutex_lock(&metadataMut);
	frame_entry->page_data->modified = true;
	frame_entry->page_data->used = true;
	pthread_mutex_unlock(&metadataMut);

	pthread_mutex_lock(&memoryMut);
	memcpy(frameAddress, from, config->pageSize);
	pthread_mutex_unlock(&memoryMut);
}

void replace_page_in_frame(uint32_t victim_frame, uint32_t PID, uint32_t pt2_index, uint32_t page){
	log_info(logger, "SWAP");

	usleep(config->swapDelay*1000);

	// Enviar pagina reemplazada a swap.
	//t_frame_entry* frame_entry = process_get_frame_entry(/*PID de la pagina a ser reemplazada*/, victim_frame);
	pthread_mutex_lock(&metadataMut); // TODO cambiar en process_frames (o en las tablas globales, nidea), quitar metadata
	uint32_t victimPID = (metadata->entries)[victim_frame].PID;
	uint32_t victimPage = (metadata->entries)[victim_frame].page;
	bool modified = (metadata->entries)[victim_frame].modified;
	pthread_mutex_unlock(&metadataMut);

	pthread_mutex_lock(&memoryMut);
	if (modified) savePage(victimPID, victimPage, memory_getFrame(victim_frame));
	pthread_mutex_unlock(&memoryMut);

	// Modificar tabla de paginas del proceso cuya pagina fue reemplazada.
	t_ptbr2 *ptReemplazado = get_page_table2(pt2_index);
	((t_page_entry *)list_get(ptReemplazado->entries, victimPage))->present = false; // TODO cambiar victim page por sus entries
	((t_page_entry *)list_get(ptReemplazado->entries, victimPage))->frame = -1;

	log_info(logger, "Replacement: Frame #%u: ; Page #%u ; Process #%u ;; Victim Page #%u ; Process #%u.",
			 victim_frame, page, PID, victimPage, victimPID);
}
