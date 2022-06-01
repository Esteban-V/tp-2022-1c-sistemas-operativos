#include"memory.h"

int main(){

    // Initialize logger
    logger = log_create("./memory.log", "MEMORY", 1, LOG_LEVEL_TRACE);

    config = getMemoryConfig("memory.config");

    tlb = createTLB();

    while(1){

    }

    log_destroy(logger);
    destroyTLB(tlb);

    return EXIT_SUCCESS;
}