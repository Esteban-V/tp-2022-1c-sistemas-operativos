#include"swap.h"

/*int main(){
    logger = log_create("swamp.log", "SWAP", true, LOG_LEVEL_INFO);
    pthread_mutex_init(&mutex_log, NULL);

    config = getSwapConfig("swamp.config");
    swapFiles = list_create();
    for(int i = 0; swapConfig->swapFiles[i]; i++)
        list_add(swapFiles, swapFile_create(swapConfig->swapFiles[i], swapConfig->fileSize, swapConfig->pageSize));

    int serverSocket = createListenServer(swapConfig->swapIP, swapConfig->swapPort);
   
    while(1){
        int memorySocket = getNewClient(serverSocket);
        swapHeader asignType = socket_getHeader(memorySocket);
        if (asignType == ASIG_FIJA) asignacion = fija;
        else if (asignType == ASIG_GLOBAL) asignacion = global;

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