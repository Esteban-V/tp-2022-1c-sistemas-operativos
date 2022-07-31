#include "tlb.h"

t_tlb *create_tlb()
{
    // Inicializo estructura de TLB
    tlb = malloc(sizeof(t_tlb));
    tlb->entryQty = config->tlbEntryQty; // cantidad de entradas
    tlb->entries = (t_tlb_entry *)calloc(tlb->entryQty, sizeof(t_tlb_entry));
    tlb->victimQueue = list_create();

    if (strcmp(config->tlb_alg, "LRU") == 0)
    {
        update_victim_queue = lru_tlb;
    }
    else
    {
        update_victim_queue = fifo_tlb;
    }

    // Seteo todas las entradas como libres
    for (int i = 0; i < tlb->entryQty; i++)
    {
        tlb->entries[i].isFree = true;
    }

    return tlb;
}

// -1 si no esta en tlb
// TODO ejecutar en memoria cuando se  pide frame
int32_t get_tlb_frame(uint32_t pid, uint32_t page)
{
    pthread_mutex_lock(&tlb_mutex);
    int32_t frame = -1;
    for (int i = 0; i < tlb->entryQty; i++)
    {
        if (tlb->entries[i].page == page && tlb->entries[i].isFree == false)
        {
            update_victim_queue(&(tlb->entries[i]));
            frame = tlb->entries[i].frame;
            tlb_hit_counter++;
        }
    }
    pthread_mutex_unlock(&tlb_mutex);

    tlb_miss_counter++;

    return frame;
}

// TODO ejecutar en memoria cuando se reemplaza / asigna pagina a TP
void add_tlb_entry(uint32_t pid, uint32_t page, int32_t frame)
{
    pthread_mutex_lock(&tlb_mutex);

    // Busco si hay una entrada libre
    bool any_free_entry = false;
    for (int i = 0; (i < tlb->entryQty) && !any_free_entry; i++)
    {
        if (tlb->entries[i].isFree)
        {
            tlb->entries[i].page = page;
            tlb->entries[i].frame = frame;
            tlb->entries[i].isFree = false;

            pthread_mutex_lock(&mutex_log);
            log_info(logger, "TLB Assignment: Free Entry Used %d ; PID: %u, Page: %u, Frame: %u", i, pid, page, frame);
            pthread_mutex_unlock(&mutex_log);

            any_free_entry = true;
            list_add(tlb->victimQueue, &(tlb->entries[i]));
            break;
        }
    }

    // Si no hay entrada libre, reemplazo
    if (!any_free_entry)
    {
        if (!list_is_empty(tlb->victimQueue))
        {
            t_tlb_entry *victim = list_remove(tlb->victimQueue, 0);

            pthread_mutex_lock(&mutex_log);
            log_info(logger, "TLB Replacement: Replacement in Entry %d ; Old Entry Data: Page: %u, Frame: %u ; New Entry Data: Page: %u, Frame: %u ; PID: %u",
                     (victim - tlb->entries), victim->page, victim->frame, pid, page, frame);
            pthread_mutex_unlock(&mutex_log);

            victim->page = page;
            victim->frame = frame;
            victim->isFree = false;
            list_add(tlb->victimQueue, victim);
        }
    }

    pthread_mutex_unlock(&tlb_mutex);
}

void lru_tlb(t_tlb_entry *entry)
{
    bool isVictim(t_tlb_entry * victim)
    {
        return victim == entry;
    };

    t_tlb_entry *entryToBeMoved = list_remove_by_condition(tlb->victimQueue, (void *)isVictim);

    list_add(tlb->victimQueue, entryToBeMoved);
}

void fifo_tlb(t_tlb_entry *entry)
{
    // Intencionalmente vacío
}

// TODO ejecutar en memoria cuando se va a reemplazar pag
void drop_tlb_entry(uint32_t page, uint32_t frame)
{
    pthread_mutex_lock(&tlb_mutex);

    for (int i = 0; i < tlb->entryQty; i++)
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
    for (int i = 0; i < tlb->entryQty; i++)
    {
        tlb->entries[i].isFree = true;
    }
    pthread_mutex_unlock(&tlb_mutex);

    pthread_mutex_lock(&mutex_log);
    log_info(logger, "Cleared TLB on process switch");
    pthread_mutex_unlock(&mutex_log);
}
