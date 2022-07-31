#include "tlb.h"

tlb_hit_counter = 0;
tlb_miss_counter = 0;

t_tlb *create_tlb()
{
    // Inicializa estructura de TLB
    tlb = malloc(sizeof(t_tlb));
    tlb->entries = (t_tlb_entry *)calloc(config->tlbEntryQty, sizeof(t_tlb_entry));
    tlb->victimQueue = list_create();

    if (!strcmp(config->tlb_alg, "FIFO"))
    {
        replaceAlgorithm = FIFO;
    }
    else if (!strcmp(config->tlb_alg, "LRU"))
    {
        replaceAlgorithm = LRU;
    }
    else
    {
        replaceAlgorithm = FIFO;

        pthread_mutex_lock(&mutex_log);
        log_warning(logger, "Wrong replace algorithm set in config --> Using FIFO");
        pthread_mutex_unlock(&mutex_log);
    }

    // Setea todas las entradas como libres
    for (int i = 0; i < config->tlbEntryQty; i++)
    {
        t_tlb_entry entry = tlb->entries[i];
        entry.isFree = true;
    }

    return tlb;
}

uint32_t get_tlb_frame(uint32_t page)
{
    uint32_t frame = -1;
    for (int i = 0; i < config->tlbEntryQty; i++)
    {
        pthread_mutex_lock(&tlb_mutex);
        if (tlb->entries[i].page == page && tlb->entries[i].isFree == false)
        {
            if (replaceAlgorithm == LRU)
            {
                lru_tlb(&(tlb->entries[i]));
            }
            frame = tlb->entries[i].frame;
            pthread_mutex_unlock(&tlb_mutex);

            tlb_hit_counter++;
        }
    }

    if (frame == -1)
    {
        tlb_miss_counter++;
    }

    return frame;
}

// TODO ejecutar en memoria cuando se reemplaza / asigna pagina a TP
void add_tlb_entry(uint32_t page, uint32_t frame)
{
    // Busco si hay una entrada libre
    bool any_free_entry = false;
    for (int i = 0; (i < config->tlbEntryQty) && !any_free_entry; i++)
    {
        pthread_mutex_lock(&tlb_mutex);
        if (tlb->entries[i].isFree)
        {
            tlb->entries[i].page = page;
            tlb->entries[i].frame = frame;
            tlb->entries[i].isFree = false;

            list_add(tlb->victimQueue, &(tlb->entries[i]));
            pthread_mutex_unlock(&tlb_mutex);

            pthread_mutex_lock(&mutex_log);
            log_info(logger, "TLB assignment for page %d / frame %d", page, frame);
            pthread_mutex_unlock(&mutex_log);

            any_free_entry = true;
            break;
        }
    }

    // Si no hay entrada libre, reemplazo
    if (!any_free_entry)
    {
        pthread_mutex_lock(&tlb_mutex);
        if (!list_is_empty(tlb->victimQueue))
        {
            t_tlb_entry *victim = list_remove(tlb->victimQueue, 0);

            pthread_mutex_lock(&mutex_log);
            log_info(logger, "TLB replacement for page %d --> %d / frame %d --> %d",
                     victim->page, page, victim->frame, frame);
            pthread_mutex_unlock(&mutex_log);

            victim->page = page;
            victim->frame = frame;
            victim->isFree = false;
            list_add(tlb->victimQueue, victim);

            pthread_mutex_unlock(&tlb_mutex);
        }
    }
}

void lru_tlb(t_tlb_entry *entry)
{
    bool isVictim(t_tlb_entry * victim)
    {
        return victim->page == entry->page && victim->frame == entry->frame;
    };

    t_tlb_entry *entryToBeMoved = list_remove_by_condition(tlb->victimQueue, (void *)isVictim);

    list_add(tlb->victimQueue, entryToBeMoved);
}

// TODO ejecutar en memoria cuando se va a reemplazar pag
void drop_tlb_entry(uint32_t page, uint32_t frame)
{
    pthread_mutex_lock(&tlb_mutex);

    for (int i = 0; i < config->tlbEntryQty; i++)
    {
        if (tlb->entries[i].page == page && tlb->entries[i].frame == frame)
        {
            tlb->entries[i].isFree = true;
            pthread_mutex_unlock(&tlb_mutex);
            return;
        }
    }

    pthread_mutex_unlock(&tlb_mutex);
}

void destroy_tlb()
{
    free(tlb->entries);
    list_destroy(tlb->victimQueue);
    free(tlb);

    pthread_mutex_lock(&mutex_log);
    log_info(logger, "Destroyed TLB successfully");
    pthread_mutex_unlock(&mutex_log);
}

void clean_tlb()
{
    pthread_mutex_lock(&tlb_mutex);
    for (int i = 0; i < config->tlbEntryQty; i++)
    {
        tlb->entries[i].isFree = true;
    }
    pthread_mutex_unlock(&tlb_mutex);

    pthread_mutex_lock(&mutex_log);
    log_info(logger, "Cleared TLB on process switch");
    pthread_mutex_unlock(&mutex_log);
}
