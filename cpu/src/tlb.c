#include "tlb.h"

t_tlb *create_tlb()
{
    tlb_replacement = 0;
    tlb_assignment = 0;
    tlb_hit_counter = 0;
    tlb_miss_counter = 0;

    pthread_mutex_init(&tlb_mutex, NULL);

    // Inicializa estructura de TLB
    tlb = malloc(sizeof(t_tlb));
    tlb->entries = (t_tlb_entry *)calloc(config->tlbEntryQty, sizeof(t_tlb_entry));
    tlb->victims = pQueue_create();

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
        t_tlb_entry *entry = &tlb->entries[i];
        entry->isFree = true;
        entry->index = i;
    }

    return tlb;
}

uint32_t find_tlb_entry(uint32_t page)
{
    uint32_t frame = -1;
    for (int i = 0; i < config->tlbEntryQty; i++)
    {
        pthread_mutex_lock(&tlb_mutex);
        t_tlb_entry *entry = &tlb->entries[i];
        if (entry->page == page && entry->isFree == false)
        {
            if (replaceAlgorithm == LRU)
            {
                lru_tlb(entry);
            }
            frame = entry->frame;
            pthread_mutex_unlock(&tlb_mutex);

            tlb_hit_counter++;

            pthread_mutex_lock(&mutex_log);
            log_error(logger, "TLB hit on page %d / frame %d", page, frame);
            pthread_mutex_unlock(&mutex_log);

            break;
        }
        else
        {
            pthread_mutex_unlock(&tlb_mutex);
        }
    }

    if (frame == -1)
    {
        pthread_mutex_lock(&mutex_log);
        log_error(logger, "TLB miss for page %d", page);
        pthread_mutex_unlock(&mutex_log);
        tlb_miss_counter++;
    }

    return frame;
}

void add_tlb_entry(uint32_t page, uint32_t frame)
{
    // Busco si hay una entrada libre
    bool any_free_entry = false;
    for (int i = 0; (i < config->tlbEntryQty) && !any_free_entry; i++)
    {
        pthread_mutex_lock(&tlb_mutex);
        t_tlb_entry *entry = &tlb->entries[i];
        pthread_mutex_unlock(&tlb_mutex);

        if (entry->isFree)
        {
            entry->page = page;
            entry->frame = frame;
            entry->isFree = false;

            pthread_mutex_lock(&tlb_mutex);
            pQueue_put(tlb->victims, entry);
            pthread_mutex_unlock(&tlb_mutex);

            any_free_entry = true;

            pthread_mutex_lock(&mutex_log);
            log_info(logger, "TLB assignment for page %d / frame %d", page, frame);
            pthread_mutex_unlock(&mutex_log);

            tlb_assignment++;

            break;
        }
    }

    // Si no hay entrada libre, reemplazo
    if (!any_free_entry)
    {
        pthread_mutex_lock(&tlb_mutex);
        if (!pQueue_is_empty(tlb->victims))
        {
            t_tlb_entry *victim = pQueue_take(tlb->victims);
            pthread_mutex_unlock(&tlb_mutex);

            pthread_mutex_lock(&mutex_log);
            log_warning(logger, "TLB replacement at entry #%d for page %d --> %d / frame %d --> %d",
                        victim->index, victim->page, page, victim->frame, frame);
            pthread_mutex_unlock(&mutex_log);

            tlb_replacement++;

            victim->page = page;
            victim->frame = frame;
            victim->isFree = false;

            pthread_mutex_lock(&tlb_mutex);
            pQueue_put(tlb->victims, victim);
        }
        pthread_mutex_unlock(&tlb_mutex);
    }
}

void lru_tlb(t_tlb_entry *entry)
{
    bool isVictim(t_tlb_entry * victim)
    {
        return victim->page == entry->page && victim->frame == entry->frame;
    };

    // Remueve la entry
    t_tlb_entry *entryToBeMoved = (t_tlb_entry *)pQueue_remove_by_condition(tlb->victims, (void*)isVictim);

    // Ubica a la misma atras de todo
    pQueue_put(tlb->victims, entryToBeMoved);
}

/* void drop_tlb_entry(uint32_t page, uint32_t frame)
{
    for (int i = 0; i < config->tlbEntryQty; i++)
    {
        pthread_mutex_lock(&tlb_mutex);
        t_tlb_entry *entry = &tlb->entries[i];
        if (entry->page == page && entry->frame == frame)
        {
            entry->page == -1;
            entry->frame == -1;
            entry->isFree = true;
            pthread_mutex_unlock(&tlb_mutex);
            return;
        }
    }
} */

void tlb_destroy()
{
    free(tlb->entries);
    pQueue_destroy(tlb->victims);
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
        t_tlb_entry *entry = &tlb->entries[i];
        entry->isFree = true;
    }
    pthread_mutex_unlock(&tlb_mutex);

    pthread_mutex_lock(&mutex_log);
    log_warning(logger, "Cleared TLB on process switch");
    pthread_mutex_unlock(&mutex_log);
}
