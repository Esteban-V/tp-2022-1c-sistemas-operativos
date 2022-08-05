#include "../include/utils.h"

t_memory_config *getMemoryConfig(char *path)
{
    t_memory_config *memoryConfig = malloc(sizeof(t_memory_config));
    memoryConfig->config = config_create(path);
    memoryConfig->listenPort = config_get_string_value(memoryConfig->config, "PUERTO_ESCUCHA");
    memoryConfig->memorySize = config_get_int_value(memoryConfig->config, "TAM_MEMORIA");
    memoryConfig->pageSize = config_get_int_value(memoryConfig->config, "TAM_PAGINA");
    memoryConfig->entriesPerTable = config_get_int_value(memoryConfig->config, "ENTRADAS_POR_TABLA");
    memoryConfig->memoryDelay = config_get_int_value(memoryConfig->config, "RETARDO_MEMORIA");
    memoryConfig->replaceAlgorithm = config_get_string_value(memoryConfig->config, "ALGORITMO_REEMPLAZO");
    memoryConfig->framesPerProcess = config_get_int_value(memoryConfig->config, "MARCOS_POR_PROCESO");
    memoryConfig->swapDelay = config_get_int_value(memoryConfig->config, "RETARDO_SWAP");
    memoryConfig->swapPath = config_get_string_value(memoryConfig->config, "PATH_SWAP");
    memoryConfig->framesInMemory = memoryConfig->memorySize / memoryConfig->pageSize;
    return memoryConfig;
}

void destroyMemoryConfig(t_memory_config *config)
{
    config_destroy(config->config);
    free(config);
}

int ceil_div(int a, int b)
{
    return (a / b) + ((a % b) != 0);
}

// Retorna un puntero al comienzo del frame en memoria
void *get_frame(uint32_t frame_number)
{
    void *mem_ptr = memory->memory;
    int frame_index = frame_number * config->pageSize;
    void *frame_ptr = mem_ptr + frame_index;

    return frame_ptr;
}

// Retorna el valor de 32 bits/4 bytes ubicado en el frame (con comienzo en frame_ptr) + un desplazamiento
uint32_t read_frame_value(void *frame_ptr, uint32_t offset)
{
    // Todos los valores a leer/escribir en memoria serán numéricos enteros no signados de 4 bytes
    uint32_t *value;
    memcpy(&value, frame_ptr + offset, sizeof(uint32_t));
    return value;
}

// Retorna toda la pagina ubicada en el frame (con comienzo en frame_ptr)
void *read_frame(void *frame_ptr)
{
    void *value;
    memcpy(value, frame_ptr, config->pageSize);
    return value;
}

// Escribe un valor de 32 bits/4 bytes ubicados en el frame (con comienzo en frame_ptr) + un desplazamiento
void write_frame_value(void *frame_ptr, uint32_t offset, uint32_t value)
{
    memcpy(frame_ptr + offset, &value, sizeof(uint32_t));
}

// Escribe/pisa toda la pagina ubicada en el frame (con comienzo en frame_ptr)
void write_frame(void *frame_ptr, void *new_page)
{
    memcpy(frame_ptr, &new_page, config->pageSize);
}

// Settea los bits de uso y modificado de una pagina leida/escrita
void set_page_bits(int frames_index, int frame, bool modified)
{
    if (frames_index < list_size(global_frames))
    {
        t_process_frame *process_frames = (t_process_frame *)list_get(global_frames, frames_index);
        t_frame_entry *frame_entry = find_frame(process_frames, frame);

        if (frame_entry != NULL)
        {
            frame_entry->page_data->used = true;
            frame_entry->page_data->modified = modified;
        }
    }
}

// Determina si de las framesPerProcess frames asignadas al proceso, hay libres para cargarles paginas
bool has_free_frame(t_process_frame *process_frames)
{
    bool _is_in_use(void *_entry)
    {
        t_frame_entry *entry = (t_frame_entry *)_entry;
        return entry->page_data != NULL;
    };

    int in_use = list_count_satisfying(process_frames->frames, _is_in_use);

    return in_use < config->framesPerProcess;
}

t_frame_entry *find_first_free_frame(t_process_frame *process_frames)
{
    bool found = false;
    bool _is_free(void *_entry)
    {
        t_frame_entry *entry = (t_frame_entry *)_entry;
        found = entry->page_data == NULL;
        return found;
    };

    t_frame_entry *frame_entry = (t_frame_entry *)list_find(process_frames->frames, _is_free);
    if (!found)
        return NULL;

    return frame_entry;
}

t_frame_entry *find_frame(t_process_frame *process_frames, int frame)
{
    bool found = false;
    bool _is_frame(void *_entry)
    {
        t_frame_entry *entry = (t_frame_entry *)_entry;
        found = entry->frame == frame && entry->page_data->present;
        return found;
    };

    t_frame_entry *frame_entry = (t_frame_entry *)list_find(process_frames->frames, _is_frame);
    if (!found)
        return NULL;

    return frame_entry;
}

int find_first_unassigned_frame(t_bitarray *bitmap)
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

void frame_set_assigned(t_bitarray *bitmap, int index)
{
    bitarray_set_bit(bitmap, index);
    sync_bitmap(bitmap);
}

void frame_clear_assigned(t_bitarray *bitmap, int index)
{
    bitarray_clean_bit(bitmap, index);
    sync_bitmap(bitmap);
}

void increment_clock_hand(int *clock_hand)
{
    (*clock_hand)++;
    if (*clock_hand >= config->framesPerProcess)
    {
        *clock_hand = *clock_hand % config->framesPerProcess;
    }
}

void sync_bitmap(t_bitarray *bitmap)
{
    msync(bitmap->bitarray, config->framesInMemory, MS_SYNC);
}
