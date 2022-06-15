 /*while(1){
        int memorySocket = getNewClient(serverSocket);
        swapHeader asignType = socket_getHeader(memorySocket);

        t_packet* petition;
        while(1){
            petition = socket_getPacket(memorySocket);
            if(petition == NULL){
                if(!retry_getPacket(memorySocket, &petition)){
                    close(memorySocket);
                    break;
                }
            }
            for(int i = 0; i < swapConfig->delay; i++){
                usleep(1000);
            }
            petitionHandler[petition->header](petition, memorySocket);
            destroyPacket(petition);
        }
    }
    return EXIT_SUCCESS;
}*/

#include "swap.h"

void swapFile_destroy(t_swapFile* self){
    close(self->fd);
    free(self->path);
    free(self->entries);
    free(self);
}

bool swapFile_isFull(t_swapFile* sf){
    return !swapFile_hasRoom(sf);
}

bool swapFile_hasRoom(t_swapFile* sf){
    bool hasRoom = false;
    for(int i = 0; i < sf->maxPages; i++)
        if(!sf->entries[i].used){
            hasRoom = true;
            break;
        }
    return hasRoom;
}

bool swapFile_hasPid(t_swapFile* sf, uint32_t pid){
    bool hasPid = false;
    for(int i = 0; i < sf->maxPages; i++)
        if(sf->entries[i].used && sf->entries[i].pid == pid){
            hasPid = true;
            break;
        }
    return hasPid;
}

int swapFile_countPidPages(t_swapFile* sf, uint32_t pid){
    int pages = 0;
    for(int i = 0; i < sf->maxPages; i++)
        if(sf->entries[i].used && sf->entries[i].pid == pid)
            pages++;
    return pages;
}

bool swapFile_isFreeIndex(t_swapFile* sf, int index){
    return sf->entries[index].used;
}

int swapFile_countFreeIndexes(t_swapFile* sf){
    int indices = 0;
    for(int i = 0; i < sf->maxPages; i++)
        if(!sf->entries[i].used) indices++;
    return indices;
}

int swapFile_getIndex(t_swapFile* sf, uint32_t pid, int32_t pageNumber){
    int found = -1;
    for(int i = 0; i < sf->maxPages; i++){
        if(sf->entries[i].pid == pid && sf->entries[i].pageNumber == pageNumber){
            found = i;
            break;
        }
    }

    return found;
}

int swapFile_findFreeIndex(t_swapFile* sf){
    int i = 0;
    while(sf->entries[i].used){
        if(i >= sf->maxPages) return -1;
        i++;
    }
    return i;
}

void swapFile_register(t_swapFile* sf, uint32_t pid, int32_t pageNumber, int index){
    sf->entries[index].used = true;
    sf->entries[index].pid = pid;
    sf->entries[index].pageNumber = pageNumber;
}
