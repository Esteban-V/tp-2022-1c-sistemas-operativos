#include "memmory.h"

int main()
{
	// Initialize logger
	logger = log_create("./cfg/memory.log", "MEMORY", 1, LOG_LEVEL_TRACE);
	// Initialize Config
	memoryConfig = getMemoryConfig("memory.config");
	// Initialize Metadata
	metadata = create_memory_metadata(memoryConfig);

	// Creacion de server
	int server_socket = create_server(memoryConfig->listenPort);
	log_info(logger, "Servidor de Memoria creado");

	// Initialize Variables
	memory = initializeMemory(memoryConfig);
	metadata->clock_m_counter = 0;
	pageTables = dictionary_create();
	// algoritmo = strcmp(memoryConfig->replaceAlgorithm, "CLOCK") ? clock_alg : clock_m_alg;

	while (1)
	{
		server_listen(server_socket, header_handler);
	}

	// Destroy
	destroyMemoryConfig(memoryConfig);
	dictionary_destroy_and_destroy_elements(pageTables, page_table_destroy);
	log_destroy(logger);

	return EXIT_SUCCESS;
}

/*bool process_suspension(t_packet *petition, int console_socket) { // TODO ADAPTAR A LO NUESTRO, CON 2 TABLAS
 uint32_t PID = stream_take_UINT32(petition->payload);

 pthread_mutex_lock(&pageTablesMut); // TODO: Revisar posibilidad de deadlock, verificar logica.
 t_ptbr1 *pt = getPageTable(PID, pageTables);
 //TODO TABLE NUMBER?
 uint32_t pages = (pt->entries)->pageQuantity;
 for (uint32_t i = 0; i < pages; i++){
 if ((pt->entries)->entries[i].present){
 void *pageContent = (void*) memory_getFrame(memory, (pt->entries)->entries[i].frame);
 swapInterface_savePage(swapInterface, PID, i, pageContent);
 pthread_mutex_lock(&metadataMut);
 metadata->entries[(pt->entries)->entries[i].frame].isFree = true;
 pthread_mutex_unlock(&metadataMut);
 (pt->entries)->entries[i].present = false;
 }
 }
 pthread_mutex_unlock(&pageTablesMut);

 if(metadata->firstFrame){
 pthread_mutex_lock(&metadataMut);
 for (uint32_t i = 0; i < memoryConfig->entriesPerTable / memoryConfig->framesPerProcess; i++){
 if(metadata->firstFrame[i] == PID) metadata->firstFrame[i] = -1;
 }
 pthread_mutex_unlock(&metadataMut);
 }

 freeProcessEntries(PID);

 return false;
 }*/

bool (*memory_handlers[7])(t_packet *petition, int socket) =
	{
		receive_pid};

void *header_handler(void *_client_socket)
{
	int client_socket = (int)_client_socket;
	bool serve = true;
	while (serve)
	{
		t_packet *packet = socket_receive_packet(client_socket);
		if (packet == NULL)
		{
			if (!socket_retry_packet(client_socket, &packet))
			{
				close(client_socket);
				break;
			}
		}
		serve = memory_handlers[packet->header](packet, client_socket);
		packet_destroy(packet);
	}
	return 0;
}

bool receive_pid(t_packet *petition, int kernel_socket)
{
	int pid = (int)stream_take_UINT32(petition->payload);

	if (!!pid)
	{
		t_ptbr1 *newPageTable = initializePageTable();

		log_info(logger, "PID #%d", pid);
		char *pid_key = string_itoa(pid);

		pthread_mutex_lock(&pageTablesMut);
		dictionary_put(pageTables, pid_key, (void *)newPageTable);
		pthread_mutex_unlock(&pageTablesMut);

		// free(pid_key);

		// TODO: Paginas
		// TODO: Enviar confirmacion / informacion a kernel
	}

	return false;
}

bool memory_write(t_packet *petition, int cpu_socket)
{
	return false;
}

bool memory_read(t_packet *petition, int cpu_socket)
{
	return false;
}

bool end_process(t_packet *petition, int cpu_socket)
{
	return false;
}

t_mem_metadata *create_memory_metadata(t_memoryConfig *config)
{
	t_mem_metadata *metadata = malloc(sizeof(t_mem_metadata));
	metadata->entryQty = config->entriesPerTable;
	metadata->counter = 0;
	metadata->entries = calloc(metadata->entryQty, sizeof(t_fr_metadata));
	metadata->clock_m_counter = NULL;
	metadata->firstFrame = NULL;

	uint32_t blockQuantity = config->entriesPerTable / config->framesPerProcess;

	metadata->firstFrame = calloc(blockQuantity, sizeof(uint32_t));
	memset(metadata->firstFrame, -1, sizeof(uint32_t) * blockQuantity);

	metadata->clock_m_counter = calloc(blockQuantity, sizeof(uint32_t));
	for (int i = 0; i < blockQuantity; i++)
	{
		metadata->clock_m_counter[i] = i * config->framesPerProcess;
	}

	for (int i = 0; i < metadata->entryQty; i++)
	{
		((metadata->entries)[i]).isFree = true;
		((metadata->entries)[i]).timeStamp = 0;
	}

	return metadata;
}

void memory_metadata_destroy(t_mem_metadata *meta)
{
	if (meta->firstFrame)
	{
		free(meta->firstFrame);
		free(meta->clock_m_counter);
	}

	free(meta->entries);
	free(meta);
}

t_memory *initializeMemory(t_memoryConfig *config)
{
	t_memory *newMemory = malloc(sizeof(t_memory));
	newMemory->memory = calloc(1, config->memorySize);

	return newMemory;
}
