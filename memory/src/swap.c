#include "swap.h"

t_swap_file *swapFile_create(uint32_t PID, size_t process_size)
{
    pthread_mutex_lock(&mutex_log);
    log_info(logger, "PID #%d - Creating Swap File - Size: %d", PID, process_size);
    pthread_mutex_lock(&mutex_log);

    t_swap_file *self = malloc(sizeof(t_swap_file));

    char *path = string_from_format("%s/%d.swap", config->swapPath, PID);

    self->path = string_duplicate(path);
    self->size = process_size;
    self->pageSize = config->pageSize;
    self->maxPages = process_size / config->pageSize;
    self->fd = open(self->path, O_RDWR | O_CREAT, S_IRWXU);
    ftruncate(self->fd, self->size);

    void *mappedFile = mmap(NULL, self->size, PROT_READ | PROT_WRITE, MAP_SHARED, self->fd, 0);

    memset(mappedFile, 0, self->size);
    msync(mappedFile, self->size, MS_SYNC);
    munmap(mappedFile, self->size);

    self->entries = calloc(1, sizeof(t_pageMetadata) * self->maxPages);
    for (int i = 0; i < self->maxPages; i++)
        self->entries[i].used = false;

    return self;
}

void swapFile_register(t_swap_file *sf, uint32_t pid, uint32_t pageNumber,
                       int index)
{
    sf->entries[index].used = true;
    sf->entries[index].pid = pid;
    sf->entries[index].pageNumber = pageNumber;
}

int swapFile_getIndex(t_swap_file *sf, uint32_t pid, uint32_t pageNumber)
{
    int found = -1;
    for (int i = 0; i < sf->maxPages; i++)
    {
        if (sf->entries[i].pid == pid && sf->entries[i].pageNumber == pageNumber)
        {
            found = i;
            break;
        }
    }

    return found;
}

void *swapFile_readAtIndex(t_swap_file *sf, int index)
{
    void *mappedFile = mmap(NULL, sf->size, PROT_READ | PROT_WRITE, MAP_SHARED,
                            sf->fd, 0);
    void *pagePtr = malloc(sf->pageSize);
    memcpy(pagePtr, mappedFile + index * sf->pageSize, sf->pageSize);
    munmap(mappedFile, sf->size);
    return pagePtr;
}

void swapFile_writeAtIndex(t_swap_file *sf, int index, void *pagePtr)
{
    void *mappedFile = mmap(NULL, sf->size, PROT_READ | PROT_WRITE, MAP_SHARED,
                            sf->fd, 0);
    memcpy(mappedFile + index * sf->pageSize, pagePtr, sf->pageSize);
    msync(mappedFile, sf->size, MS_SYNC);
    munmap(mappedFile, sf->size);
}

void swapFile_clearAtIndex(t_swap_file *sf, int index)
{
    void *mappedFile = mmap(NULL, sf->size, PROT_READ | PROT_WRITE, MAP_SHARED,
                            sf->fd, 0);
    memset(mappedFile + index * sf->pageSize, 0, sf->pageSize);
    msync(mappedFile, sf->size, MS_SYNC);
    munmap(mappedFile, sf->size);
}

bool swapFile_hasPid(t_swap_file *sf, uint32_t pid)
{
    bool hasPid = false;
    for (int i = 0; i < sf->maxPages; i++)
        if (sf->entries[i].used && sf->entries[i].pid == pid)
        {
            hasPid = true;
            break;
        }

    return hasPid;
}

t_swap_file *pidExists(uint32_t pid)
{
    bool hasProcess(void *elem)
    {
        return swapFile_hasPid((t_swap_file *)elem, pid);
    };

    t_swap_file *file = list_find(swapFiles, hasProcess);

    return file;
}

int swapFile_countPidPages(t_swap_file *sf, uint32_t pid)
{
    int pages = 0;
    for (int i = 0; i < sf->maxPages; i++)
        if (sf->entries[i].used && sf->entries[i].pid == pid)
            pages++;

    return pages;
}

// Return true if error
bool destroy_swap_page(uint32_t pid, uint32_t page)
{
    t_swap_file *file = pidExists(pid);

    pthread_mutex_lock(&mutex_log);
    log_info(logger, "PID #%d - Deleting Swap File", pid);
    pthread_mutex_unlock(&mutex_log);

    if (file == NULL)
    {
        return true;
    }

    int index = swapFile_getIndex(file, pid, page);

    if (index == -1)
    {
        return true;
    }

    swapFile_clearAtIndex(file, index);
    file->entries[index].used = false;

    pthread_mutex_lock(&mutex_log);
    log_info(logger, "File %s: Page %d Erased ; #PID %d", file->path, page, pid);
    pthread_mutex_unlock(&mutex_log);

    return false;
}

// Encuentra el indice de comienzo del chunk donde se encuentra PID (acorde a asignacion fija)
int getChunk(t_swap_file *sf, uint32_t pid)
{
    int i = 0;
    while (sf->entries[i].pid != pid)
    {
        if (i >= sf->maxPages)
            return -1;
        i += config->framesPerProcess;
    }
    if (!sf->entries[i].used)
        return -1;

    return i;
}

// Se fija si en un dado swapFile hay un "chunk" libre (acorde a asignacion fija)
bool hasFreeChunk(t_swap_file *sf)
{
    bool hasFreeChunk = false;
    for (int i = 0; i < sf->maxPages; i += config->framesPerProcess)
    {
        if (!sf->entries[i].used)
        {
            hasFreeChunk = true;
            break;
        }
    }

    return hasFreeChunk;
}

// Encuentra el indice de comienzo de un chunk libre (acorde a asignacion fija)
int findFreeChunk(t_swap_file *sf)
{
    for (int i = 0; i < sf->maxPages; i += config->framesPerProcess)
    {
        if (!(sf->entries[i].used))
        {
            return i;
        }
    }

    return -1;
}