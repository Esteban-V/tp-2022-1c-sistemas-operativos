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

	process = create_process();
	if (!process)
		return EXIT_FAILURE;

	memcpy(process->instructions, instruction_list, sizeof(t_list));
	process->size = process_size;

	log_info(logger, "Size: %d\n", process->size);
	log_info(logger, "Instructions:\n");
	void _log_instruction(void *elem) {
		log_instruction(logger, elem);
	}

	list_iterate(process->instructions, _log_instruction);

	server_socket = connect_to(ip, port);

	t_packet *process_packet = create_packet(NEW_PROCESS, 64);
	stream_add_process(process_packet);

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
	t_instruction *instruction = create_instruction(string_length(id));
	memcpy(instruction->id, id, string_length(id) + 1);

	char *next_param;
	while ((next_param = instruction_text[i + 1]) != NULL) {
		param = atoi(next_param);
		list_add(instruction->params, param);
		i++;
	}

	return instruction;
}

void stream_add_process(t_packet *packet) {
	stream_add_UINT32(packet->payload, process->size);
	stream_add_LIST(packet->payload, process->instructions,
			stream_add_instruction);
}

void stream_add_instruction(t_stream_buffer *stream, void *elem) {
	t_instruction *instruction = (t_instruction*) elem;
	stream_add_STRING(stream, instruction->id);
	stream_add_LIST(stream, instruction->params, stream_add_UINT32);
}

void terminate_console() {
	log_destroy(logger);
	config_destroy(config);
	close(server_socket);
	exit(EXIT_SUCCESS);
}

