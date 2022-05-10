/*
 ============================================================================
 Name        : console.c
 Author      : 
 Version     :
 ============================================================================
 */

#include "console.h"

/*
 Listado de instrucciones:
 NO_OP: 1 parámetro
 I/O y READ: 1 parámetro
 COPY y WRITE: 2 parámetros
 EXIT: 0 parámetros
 */

int main(int argc, char **argv) {
	logger = create_logger();
	config = create_config();

	if (argc < 3) {
		log_error(logger, "Missing params");
		return EXIT_FAILURE;
	}
	if (argc > 3) {
		log_warning(logger, "Unused params");
	}

	ip = config_get_string_value(config, "IP_KERNEL");
	port = config_get_string_value(config, "PUERTO_KERNEL");

	log_info(logger, "IP value is: %s\nPort value is: %s \n", ip, port);

	char *code_path = argv[1];
	uint32_t process_size = atoi(argv[2]);

	instruction_list = list_create();

	FILE *instruction_file = open_file(code_path);
	get_code(instruction_file);

	process = create_process(process_size);
	if (!process)
		return EXIT_FAILURE;
	memcpy(process->instructions, instruction_list, sizeof(t_list));

	log_info(logger, "Size: %d\n", process->size);
	log_info(logger, "Instructions:\n");
	list_iterate(process->instructions, (void*) log_instruction);

	server_socket = connect_to(ip, port);

	//Serializacion de la estructura proceso
	// serializacion_process(process);

	t_packet *process_packet = create_packet(NEW_PROCESS, 64);
	stream_process(process_packet);

	if (server_socket != -1) {
		socket_send_packet(server_socket, process_packet);
	}

	packet_destroy(process_packet);

	// esperar resultado
	// tirar info/error resultado con logger
	//process_destroy(process);
	terminate_console();
}

void get_code(FILE *file) {
	char *line_buf = NULL;
	size_t line_buf_size = 0;
	ssize_t lines_read;

	lines_read = getline(&line_buf, &line_buf_size, file);
	while (lines_read != -1) {
		list_add(instruction_list, parse_instruction(line_buf));
		lines_read = getline(&line_buf, &line_buf_size, file);
	}

	fclose(file);
}

t_instruction* parse_instruction(char *string) {
	int param;
	int i = 0;

	char **instruction_text = string_split(string, " ");
	char *id = instruction_text[0];
	t_instruction *instruction = create_instruction(string_length(id) + 1);
	memcpy(instruction->id, id, string_length(id) + 1);

	char *next_param;
	while ((next_param = instruction_text[i + 1]) != NULL) {
		param = atoi(next_param);
		list_add(instruction->params, param);
		i++;
	}

	return instruction;
}

void terminate_console() {
	log_destroy(logger);
	config_destroy(config);
	close(server_socket);
	exit(EXIT_SUCCESS);
}

void stream_process(t_packet *packet) {
	stream_add_UINT32(packet->payload, process->size);
	stream_add_LIST(packet->payload, process->instructions, stream_instruction);

}

void stream_instruction(t_stream_buffer *stream, void *elem) {
	t_instruction *instruction = (t_instruction*) elem;
	stream_add_STRING(stream, instruction->id);
	stream_add_LIST(stream, instruction->params, stream_add_UINT32);
}

/*
 void serializacion_process(t_process *process) {

 t_buffer *buffer = malloc(sizeof(t_buffer)); //creamos el buffer

 buffer->size = sizeof(uint8_t) + //le hacemos espacio para el process->size
 (sizeof(t_list) * process->instruction_count); //espacio para las instrucciones

 void *stream = malloc(buffer->size); // stream del tamanio del buffer

 int offset = 0; // desplazamiento

 memcpy(stream + offset, &process->size, sizeof(uint8_t));
 offset += sizeof(uint8_t); //copiamos al stream el size del process y nos desplazamos
 memcpy(stream + offset, &process->instruction_count, sizeof(uint8_t));
 offset += sizeof(uint8_t); //copiamos al stream el size del t_list count
 memcpy(stream + offset, &process->instructions,
 sizeof(t_list) * (process->instruction_count));

 buffer->stream = stream;

 free(process->instructions); //es la unica variable dinamica la liberamos

 t_packet *packet = malloc(sizeof(t_packet));

 packet->header = NEW_PROCESS;
 packet->payload = buffer;

 void *to_send = malloc(buffer->size + sizeof(uint8_t) + sizeof(uint32_t));
 offset = 0;

 //ahora seria como una especie de serializacion pero del paquete
 memcpy(to_send + offset, &(packet->header), sizeof(uint8_t));
 offset += sizeof(uint8_t); //vamos copiando y desplazando
 memcpy(to_send + offset, &(packet->payload->size), sizeof(uint32_t));
 offset += sizeof(uint32_t);
 memcpy(to_send + offset, packet->payload->stream, packet->payload->size);

 free(to_send);
 free(packet->payload->stream);
 free(packet->payload);
 free(packet);

 } // FALTA TERMINAR
 */

