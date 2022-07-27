#include "utils.h"

t_memoryConfig *getMemoryConfig(char *path)
{
    t_memoryConfig *memoryConfig = malloc(sizeof(t_memoryConfig));
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

void destroyMemoryConfig(t_memoryConfig *config)
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
    uint32_t value=0;
    memcpy(frame_ptr + offset, &value, sizeof(uint32_t));
    return value;
}

// Retorna toda la pagina de pageSize ubicada en el frame (con comienzo en frame_ptr)
void *get_frame_value(void *frame_ptr)
{
    void *value;
    memcpy(&value, frame_ptr, config->pageSize);
    return value;
}

// Escribe en toda la pagina de pageSize ubicada en el frame (con comienzo en frame_ptr) el valor recibido
void write_frame_value(void *frame_ptr, void *value)
{
    memcpy(frame_ptr, &value, config->pageSize);
}

// Determina si de las framesPerProcess frames asignadas al proceso, hay libres para cargarles paginas
bool has_free_frame(t_process_frame *process_frames)
{
    bool _has_frame(void *entry)
    {
        return ((t_frame_entry *)entry)->frame != -1;
    };

    int cant_present = list_count_satisfying(process_frames->frames, _has_frame);
    return cant_present < config->framesPerProcess;
}

int find_first_free_frame(t_process_frame *process_frames)
{

    bool _is_free(void *entry)
    {
        return ((t_frame_entry *)entry)->frame != -1;
    };

    return (int) list_find(process_frames->frames, _is_free);
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
    if (*clock_hand >= config->framesInMemory)
    {
        *clock_hand = *clock_hand % config->framesInMemory;
    }
}

void sync_bitmap(t_bitarray *bitmap)
{
    msync(bitmap->bitarray, config->framesInMemory, MS_SYNC);
}
