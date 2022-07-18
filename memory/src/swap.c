#include "swap.h"


t_swap_file *swapFile_create(char *path, uint32_t PID, size_t size,
                             size_t pageSize)
{ // TODO Asignar PID
    t_swap_file *self = malloc(sizeof(t_swap_file));

    char *_PID = string_duplicate(string_itoa(PID));
    string_append(&_PID, ".swap");
    char *aux = "/";
    string_append(&aux, _PID);
    string_append(&path, aux);

    pthread_mutex_lock(&mutex_log);
    log_info(logger, "- - - - - - PATH %s - - - - - -", path);
    pthread_mutex_unlock(&mutex_log);

    self->path = string_duplicate(path);
    self->size = size;
    self->pageSize = pageSize;
    self->maxPages = size / pageSize;
    self->fd = open(self->path, O_RDWR | O_CREAT, S_IRWXU);
    ftruncate(self->fd, self->size);
    void *mappedFile = mmap(NULL, self->size, PROT_READ | PROT_WRITE,
                            MAP_SHARED, self->fd, 0);
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
    log_info(logger, "File %s: Page %d Erased ; #PID %d", file->path, page,
             pid);
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
        if (!sf->entries[i].used)
        {
            hasFreeChunk = true;
            break;
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

// Algoritmo de asignacion fija.
// Si proceso existe en algun archivo && hay lugar para una pagina mas en su chunk --> se asigna
// Si proceso existe en algun archivo && no hay mas lugar --> no se asigna
// Si proceso NO existe en ningun archivo && hay un archivo con un chunk disponible --> se asigna
// Si proceso NO existe en ningun archivo && no hay ningun archivo con un chunk disponible --> no se asigna

bool fija_swap(uint32_t pid, uint32_t page, void *pageContent)
{
    t_swap_file *file = pidExists(pid);

    bool _hasFreeChunk(void *elem)
    {
        return hasFreeChunk((t_swap_file *)elem);
    };

    if (file == NULL)
        file = list_find(swapFiles, _hasFreeChunk);
    if (file == NULL)
        return false;

    int assignedIndex = swapFile_getIndex(file, pid, page);
    if (assignedIndex == -1)
    {
        int base = getChunk(file, pid);
        if (base == -1)
            base = findFreeChunk(file);
        if (base == -1)
            return false;
        int offset = swapFile_countPidPages(file, pid);
        if (offset == config->framesPerProcess)
            return false;
        assignedIndex = base + offset;
    }
    swapFile_writeAtIndex(file, assignedIndex, pageContent);
    swapFile_register(file, pid, page, assignedIndex);

    pthread_mutex_lock(&mutex_log);
    log_info(logger, "File %s: Page %u Saved ; Process %u ; Index %u",
             file->path, page, pid, assignedIndex);
    pthread_mutex_lock(&mutex_log);

    return true;
}
