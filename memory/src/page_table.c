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
	int pages_required = ceil_div(process_size, config->pageSize);
	int level2_pages_required = ceil_div(pages_required, config->entriesPerTable);
	int page_counter = 0;
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
			page->page = page_counter;
			page_counter++;

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

// Asigna primeros framesPerProcess libres e inicializa estructura para recorrerlas con clock/clock-m, y retorna el indice de la misma
int assign_process_frames()
{
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
			// TODO: Definir que hacer si no encuentra frames libres :(
		}
	}

	t_process_frame *process_frame = malloc(sizeof(t_process_frame));
	process_frame->frames = frames;
	process_frame->clock_hand = 0;

	// Se agrega a lista global de frames y se obtiene su posicion en la misma para retornar
	int process_frames_index = list_add(global_frames, process_frame);
	return process_frames_index;
}

// Libera todos los frames de un proceso
void unassign_process_frames(int frames_index)
{
	//log_info(logger,"UNAAA");
	
	if (frames_index < list_size(global_frames))
	{
		t_process_frame *process_frames = (t_process_frame *)list_get(global_frames, frames_index);

		void unassign_frame(void *elem)
		{
			t_frame_entry *entry = (t_frame_entry *)elem;
			//log_info(logger,"pre bit");
			frame_clear_assigned(frames_bitmap, entry->frame);
			//log_info(logger,"post bit");
			entry->frame = -1;
			entry->page_data = NULL;
		};

		list_iterate(process_frames->frames, unassign_frame);
	}
}

// Retorna la tabla de nivel 1 segun su indice en su lista global
t_ptbr1 *get_page_table1(int pt1_index)
{
	t_ptbr1 *level1_table = NULL;
	if (pt1_index < list_size(level1_tables))
	{
		// Obtiene la tabla de nivel 1
		level1_table = (t_ptbr1 *)list_get(level1_tables, pt1_index);
	}
	return level1_table;
}

// Retorna el indice de tabla nivel 2 en la lista global 2, correspondiente a la entry en el index (entry_index) de la tabla nivel 1, que esta en el index (pt1_index) de la lista global 1
int get_page_table2_index(uint32_t pt1_index, uint32_t entry_index)
{
	// Obtiene la tabla de nivel 1
	t_ptbr1 *level1_table = get_page_table1(pt1_index);

	int pt2_index = -1;
	if (level1_table != NULL && (entry_index < list_size(level1_table->entries)))
	{
		// Obtiene el index de pt2 en lista global
		pt2_index = (int *)list_get(level1_table->entries, entry_index);
	}
	return pt2_index;
}

// Retorna la tabla de nivel 2 segun su indice en su lista global
t_ptbr2 *get_page_table2(int pt2_index)
{
	t_ptbr2 *level2_table = NULL;

	if (pt2_index < list_size(level2_tables))
	{ // Obtiene la tabla de nivel 2
		level2_table = (t_ptbr2 *)list_get(level2_tables, pt2_index);
	}
	return level2_table;
}

// Retorna el numero de frame en memoria (cargandola previamente si fuese necesario)
int get_frame_number(int pt2_index, int entry_index, int pid, int frames_index)
{
	int frame = -1;

	// Obtiene la tabla de nivel 2
	t_ptbr2 *level2_table = get_page_table2(pt2_index);

	if (level2_table != NULL && (entry_index < list_size(level2_table->entries)))
	{
		// Obtiene la pagina con sus bits de estado
		t_page_entry *entry = (t_page_entry *)list_get(level2_table->entries, entry_index);

		if (entry->present)
		{
			// se settean los bits en read (used) o write (used y modified)
			frame = entry->frame;
		}
		else
		{
			// Page fault ++, no estaba presente
			pthread_mutex_lock(&mutex_log);
			log_error(logger, "Page fault for page %d", entry->page);
			pthread_mutex_unlock(&mutex_log);
			page_fault_counter++;

			if (frames_index < list_size(global_frames))
			{
				t_process_frame *process_frames = (t_process_frame *)list_get(global_frames, frames_index);

				// Chequear si se puede asignar directo
				if (has_free_frame(process_frames))
				{
					t_frame_entry *free_frame = find_first_free_frame(process_frames);

					if (free_frame != NULL) // No deberia pasar porque ya entro a has_free_frame
					{
						// Actualiza pagina en tabla de paginas
						entry->present = true;
						entry->frame = free_frame->frame;

						// Actualiza frame del proceso
						free_frame->page_data = entry;
						frame = entry->frame;
						get_swap(frame, entry->page, pid);
					}
				}
				else
				{
					pthread_mutex_lock(&mutex_log);
					log_warning(logger, "No free frames for process #%d, replacing...", pid);
					pthread_mutex_unlock(&mutex_log);

					// Reemplazo ++, no hay mas libres y se debe reemplazar con una existente
					page_replacement_counter++;
					frame = replace_algorithm(process_frames, entry, pid);
					pthread_mutex_lock(&mutex_log);
					log_warning(logger, "Frame victim %d", frame);
					pthread_mutex_unlock(&mutex_log);

				}
			}
		}
	}
	return frame;
}

void save_swap(int frame_number, int page_number, int pid)
{
	// Identifica que parte de memoria (frame) debe leer
	void *frame_ptr = get_frame(frame_number);

	// Obtiene lo leido del frame en memoria
	void *memory_value = read_frame(frame_ptr);

	// Lo escribe en la pagina correspondiente en swap
	swap_instruct=WRITE_SWAP;
	value_swap=memory_value;
	page_num_swap=page_number;
	pid_swap=pid;
	sem_post(&sem_swap);
	sem_wait(&swap_end);

	//swap_write_page(pid, page_number, memory_value);


	pthread_mutex_lock(&mutex_log);
	log_info(logger, "Removed PID #%d's page #%d from memory", pid, page_number);
	pthread_mutex_unlock(&mutex_log);
}

void get_swap(int frame_number, int page_number, int pid)
{
	// Obtiene lo leido de la pagina en swap
	swap_instruct=READ_SWAP;
	pid_swap=pid;
	page_num_swap=page_number;
	sem_post(&sem_swap);
	sem_wait(&swap_end);
	void *swap_value = read_from_swap;
	//void *swap_value = swap_get_page(pid, page_number);

	// Identifica que parte de memoria (frame) debe escribir
	void *frame_ptr = get_frame(frame_number);

	// int j;
	// for(j=0;j<config->pageSize;j++){
	// 	printf("%d ",((char *)frame_ptr)[j]);
	// }

	// Lo escribe en el frame correspondiente en memoria
	memcpy(frame_ptr, swap_value, config->pageSize);
	//write_frame(frame_ptr, swap_value);

	// int i;
	// for(i=0;i<config->pageSize;i++){
	// 	printf("%d ",((char *)frame_ptr)[i]);
	// }

	pthread_mutex_lock(&mutex_log);
	log_info(logger, "Loaded PID #%d's page #%d into memory", pid, page_number);
	pthread_mutex_unlock(&mutex_log);
}

int replace_algorithm(t_process_frame *process_frames, t_page_entry *entry, int pid)
{
	void _replace(t_frame_entry * curr_frame, t_page_entry * old_page)
	{
		int frame_number = curr_frame->frame;
		// Lleva a disco la pagina a reemplazar y la marca como no presente
		if (old_page->present && old_page->modified)
		{
			save_swap(frame_number, old_page->page, pid);
			old_page->modified = false;
		}
		old_page->present = false;

		// Trae la nueva pagina de disco (mismo frame y mismo proceso)
		get_swap(frame_number, entry->page, pid);

		// Actualiza pagina en tabla de paginas
		entry->present = true;
		entry->frame = frame_number;

		// Actualiza frame del proceso
		curr_frame->page_data = entry;
	};

	int frame = -1;
	for (int i = 0; i < (replaceAlgorithm == CLOCK_M ? 2 : 1); i++)
	{
		frame = two_clock_turns(process_frames, replaceAlgorithm == CLOCK_M, _replace);
		// TODO
		if (frame != -1)
		{
			return frame;
		}
	}

	return frame;
}

int two_clock_turns(t_process_frame *process_frames, bool check_modified, void *(*replace)(t_frame_entry *, t_page_entry *))
{
	int frame = -1;
	for (int j = 0; j < 2; j++)
	{
		// Vuelta numero j+1 del reloj
		for (int k = 0; k < config->framesPerProcess; k++)
		{
			if (process_frames->clock_hand < list_size(process_frames->frames))
			{
				// Empezando de donde apunta el puntero del clock, fijarse si es reemplazable o pasar a proxima frame
				t_frame_entry *curr_frame = (t_frame_entry *)list_get(process_frames->frames, process_frames->clock_hand);
				t_page_entry *curr_page = curr_frame->page_data;
				if (check_modified)
				{
					// El algoritmo es CLOCK-M
					if (!curr_page->used && !curr_page->modified)
					{
						// Si no esta en uso ni modificado (0, 0): Reemplaza
						replace(curr_frame, curr_page);
						// Deja el puntero incrementado para proxima vez
						increment_clock_hand(&(process_frames->clock_hand));
						return curr_frame->frame;
					}

					// 2da vuelta (j = 1): Ignora si esta modificada
					if (j == 1)
					{
						if (!curr_page->used)
						{
							// Si no esta en uso y pero si modificado (0, 1): Reemplaza
							replace(curr_frame, curr_page);
							// Deja el puntero incrementado para proxima vez
							increment_clock_hand(&(process_frames->clock_hand));
							return curr_frame->frame;
						}
						else
						{
							// Si esta en uso: Marca uso en 0 para proxima vuelta
							// - (1, 0) --> (0, 0)
							// - (1, 1) --> (0, 1)
							curr_page->used = false;
						}
					}
					// Si no reemplaza, pasa de frame
					increment_clock_hand(&(process_frames->clock_hand));
				}
				else
				{
					// El algoritmo es CLOCK
					if (!curr_page->used)
					{
						// Si no esta en uso: Reemplaza
						replace(curr_frame, curr_page);
						// Deja el puntero incrementado para proxima vez
						increment_clock_hand(&(process_frames->clock_hand));
						return curr_frame->frame;
					}
					else
					{
						// Si esta en uso: Marca uso en 0 para proxima vuelta y pasa de frame
						curr_page->used = false;
						increment_clock_hand(&(process_frames->clock_hand));
					}
				}
			}
		}
	}
	return frame;
}
