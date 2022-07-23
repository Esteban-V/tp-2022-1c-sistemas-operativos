#include "swap.h"

void create_swap(uint32_t pid, size_t process_size)
{
    pthread_mutex_lock(&mutex_log);
    log_info(logger, "PID #%d --> Creating swap file sized %d", pid, process_size);
    pthread_mutex_lock(&mutex_log);

    char *swap_file_path = string_from_format("%s/%d.swap", config->swapPath, pid);

    // uint32_t frame_start = frame_index * sizeof(uint32_t);
    // uint32_t frame_end = frame_start + config->pageSize;

    usleep(config->swapDelay * 1000);

    // Crear archivo
    FILE *swap_file;
    catch_syscall_err(swap_file = fopen(swap_file_path, "ab+"));
    catch_syscall_err(ftruncate(swap_file, process_size));
    catch_syscall_err(fclose(swap_file));

    free(swap_file_path);
}

void swap_write_page(uint32_t pid, int page, void *data)
{
    char *swap_file_path = string_from_format("%s/%d.swap", config->swapPath, pid);
    FILE *swap_file;
    catch_syscall_err(swap_file = fopen(swap_file_path, "ab+"));
    fseek(swap_file, page * config->pageSize, SEEK_SET);

    fwrite(data, config->pageSize, 1, swap_file);

    fclose(swap_file);
    free(swap_file_path);
}

void *swap_get_page(uint32_t pid, int page)
{
    char *swap_file_path = string_from_format("%s/%d.swap", config->swapPath, pid);
    FILE *swap_file;
    catch_syscall_err(swap_file = fopen(swap_file_path, "ab+"));
    fseek(swap_file, page * config->pageSize, SEEK_SET);

    void *read_page = malloc(config->pageSize);
    fread(read_page, config->pageSize, 1, swap_file);

    fclose(swap_file);
    free(swap_file_path);

    return read_page;
}

void delete_swap(uint32_t pid)
{
    pthread_mutex_lock(&mutex_log);
    log_info(logger, "PID #%d --> Deleting swap file", pid);
    pthread_mutex_unlock(&mutex_log);

    char *swap_file_path = string_from_format("%s/%d.swap", config->swapPath, pid);

    usleep(config->swapDelay * 1000);
    remove(swap_file_path);
}