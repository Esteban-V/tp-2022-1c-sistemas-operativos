#include "swap.h"

void create_swap(uint32_t pid, uint32_t process_size)
{
    pthread_mutex_lock(&mutex_log);
    log_info(logger, "PID #%d --> Creating swap file sized %d", pid, process_size);
    pthread_mutex_unlock(&mutex_log);

    char *swap_file_path = string_from_format("%s/%d.swap", config->swapPath, pid);

    usleep(config->swapDelay * 1000);

    // Crear archivo
    FILE *swap_file;
    catch_syscall_err(swap_file = fopen(swap_file_path, "ab+"));
    catch_syscall_err(ftruncate(fileno(swap_file), process_size));
    void *swap = malloc(process_size);
    memset(swap, 0, process_size);
    fwrite(swap, process_size, 1, swap_file);
    free(swap);
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
    log_warning(logger, "Deleting process #%d swap file", pid);
    pthread_mutex_unlock(&mutex_log);

    char *swap_file_path = string_from_format("%s/%d.swap", config->swapPath, pid);

    usleep(config->swapDelay * 1000);
    munmap(findRela(pid)->dir, findRela(pid)->size); // desmapeamos el archivo swap del proceso (liberamos la variable)
    remove(swap_file_path);
}

void swap()
{
    sem_init(&sem_swap, 0, 0);
    sem_init(&swap_end, 0, 0);
    relations = list_create();
    while (true)
    {
        sem_wait(&sem_swap);
        usleep(config->swapDelay * 1000);
        switch (swap_instruct)
        {
        case CREATE_SWAPPP:
            create_swapp(pid_swap, pid_size_swap);
            break;

        case WRITE_SWAP:
            write_swapp(pid_swap, value_swap, page_num_swap);
            break;

        case READ_SWAP:
            read_from_swap = read_swap(pid_swap, page_num_swap);
            break;
        }
        sem_post(&swap_end);
    }
}

bool create_swapp(uint32_t pid, uint32_t size)
{
    char *swap_file_path = string_from_format("%s/%d.swap", config->swapPath, pid);

    int file = open(swap_file_path, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    if (file == -1)
    {
        //TODO ERROR
        return false;
    }

    ftruncate(file, size);
    relation_t *rel = malloc(sizeof(relation_t *));
    rel->size = size;
    rel->pid = pid;
    rel->dir = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, file, 0);

    list_add(relations, rel);

    if (errno != 0)
        log_error(logger, "Error en mmap: errno %i", errno);

    memset(rel->dir, 0, size);

    close(file);

    pthread_mutex_lock(&mutex_log);
    log_warning(logger, "Created swap file for process #%d", pid);
    pthread_mutex_unlock(&mutex_log);

    return true;
}
/*
void *findDir(uint32_t pid)
{
    bool find_dir(void *elem)
    {
        relation_t *rela = (relation_t *)elem;
        log_info(logger,"comparo con %d", rela->pid);
        return rela->pid == pid;
    }
    return ((relation_t *)list_find(relations, find_dir))->dir;
}
 */
relation_t *findRela(uint32_t pid)
{
    bool find_dir(void *elem)
    {
        relation_t *rela = (relation_t *)elem;
        //log_info(logger, "comparo con %d", rela->pid);
        return rela->pid == pid;
    }
    return ((relation_t *)list_find(relations, find_dir));
}

void *read_swap(uint32_t pid, uint32_t page_num)
{
    pthread_mutex_lock(&mutex_log);
    log_warning(logger, "Reading swap file for process #%d | Page #%d", pid, page_num);
    pthread_mutex_unlock(&mutex_log);

    void *mapped = findRela(pid)->dir;
    void *content = malloc(config->pageSize);
    memcpy(content, mapped + page_num * config->pageSize, config->pageSize);
    return content;
}

void write_swapp(uint32_t pid, void *value, uint32_t page_num)
{
    pthread_mutex_lock(&mutex_log);
    log_warning(logger, "Writing swap file for process #%d | Page #%d", pid, page_num);
    pthread_mutex_unlock(&mutex_log);

    //log_info(logger, "ANTES %d", pid);
    void *mapped = findRela(pid)->dir;
    //log_info(logger, "DESPUES");
    if (mapped == NULL)
        log_info(logger, "NO ENCONTRO");
    memcpy(mapped + page_num * config->pageSize, value, config->pageSize);
}