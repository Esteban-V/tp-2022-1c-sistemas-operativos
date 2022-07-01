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

	if (!config || !config_has_property(config, "IP_KERNEL")
			|| !config_has_property(config, "PUERTO_KERNEL")) {
		log_error(logger, "Config failed to load");
		return EXIT_FAILURE;
	}

	kernel_ip = config_get_string_value(config, "IP_KERNEL");
	kernel_port = config_get_string_value(config, "PUERTO_KERNEL");

	char *code_path = argv[1];
	uint32_t process_size = atoi(argv[2]);

	instruction_list = list_create();

	FILE *instruction_file = open_file(code_path);
	get_code(instruction_file);

	process = create_process();
	if (!process) {
		terminate_console(true);
	}

	memcpy(process->instructions, instruction_list, sizeof(t_list));
	process->size = process_size;
	log_info(logger, "Read process with %d instructions",
			list_size(process->instructions));

	kernel_socket = connect_to(kernel_ip, kernel_port);

	if (!kernel_socket) {
		terminate_console(true);
	}

	log_info(logger, "Console connected to kernel");

	t_packet *process_packet = create_packet(NEW_PROCESS, 64);
	stream_add_process(process_packet, process);

	if (kernel_socket != -1) {
		socket_send_packet(kernel_socket, process_packet);
	}

	packet_destroy(process_packet);
	uint8_t result = socket_receive_header(kernel_socket);
	bool error = result != PROCESS_OK;

	if (error) {
		log_error(logger, "Console exited with error code %d", result);
	} else {
		log_info(logger, "Console exited successfully");
	}

	terminate_console(error);
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
	int i = 0;

	char **instruction_text = string_split(string, " ");
	char *id = instruction_text[0];
	string_trim(&id);
	t_instruction *instruction = create_instruction(string_length(id));
	memcpy(instruction->id, id, string_length(id) + 1);

	char *next_param;
	while ((next_param = instruction_text[i + 1]) != NULL) {
		int param = atoi(next_param);
		int *param_pointer = malloc(sizeof(int));
		memcpy(param_pointer, &param, sizeof(int));
		list_add(instruction->params, param_pointer);
		i++;
	}

	return instruction;
}

void terminate_console(bool error) {
	log_destroy(logger);
	config_destroy(config);
	process_destroy(process);
	close(kernel_socket);
	exit(error ? EXIT_FAILURE : EXIT_SUCCESS);
}

