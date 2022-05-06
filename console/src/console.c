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

void log_params(int param) {
	log_info(logger, " %d", param);
};

void log_instruction(t_instruction *inst) {
	log_info(logger, inst->id);
	list_iterate(inst->params, (void*) log_params);
};

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
	int process_size = atoi(argv[2]);

	instruction_list = list_create();

	FILE *instruction_file = open_file(code_path);
	get_code(instruction_file);

	process = process_create(process_size);
	if (!process)
		return EXIT_FAILURE;
	memcpy(process->instructions, instruction_list, sizeof(t_list));

	log_info(logger, "Size: %d\n", process->size);
	log_info(logger, "Instructions:\n");
	list_iterate(process->instructions, (void*) log_instruction);

	connection = create_connection(ip, port);
	send_message_to(code_path, connection);

	// serializar proceso
	// enviar proceso (como paquete)

	// esperar resultado
	// tirar info/error resultado con logger

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
	t_instruction *instruction = instruction_create(string_length(id) + 1);
	memcpy(instruction->id, id, string_length(id) + 1);

	char *next_param;
	while ((next_param = instruction_text[i + 1]) != NULL) {
		param = atoi(next_param);
		list_add(instruction->params, param);
		i++;
	}

	return instruction;
}

FILE* open_file(char *path) {
	FILE *file = fopen(path, "r");
	if (file == NULL) {
		exit(-1);
	}
	return file;
}

void terminate_console() {
	log_destroy(logger);
	config_destroy(config);
	destroy_connection(connection);
	exit(EXIT_SUCCESS);
}

t_instruction* instruction_create(size_t id_size) {
	// Try to allocate instruction structure.
	t_instruction *instruction = malloc(sizeof(t_instruction));
	if (instruction == NULL)
		return NULL;

	// Try to allocate instruction id and params, free structure if fail.
	instruction->id = malloc(id_size * sizeof(char));
	if (instruction->id == NULL) {
		free(instruction);
		return NULL;
	}

	instruction->params = list_create();
	if (instruction->params == NULL) {
		free(instruction->id);
		free(instruction);
		return NULL;
	}

	return instruction;
}

void instruction_destroy(t_instruction *instruction) {
	if (instruction != NULL) {
		free(instruction->id);
		free(instruction->params);
		free(instruction);
	}
}

t_process* process_create(int size) {
	// Try to allocate process structure.
	t_process *process = malloc(sizeof(t_process));
	if (process == NULL) {
		return NULL;
	}

	// Try to allocate process size and instructions, free structure if fail.
	process->size = malloc(sizeof(int));
	if (process->size == NULL) {
		free(process);
		return NULL;
	}
	process->size = size;
	process->instructions = list_create();

	if (process->instructions == NULL) {
		free(process->size);
		free(process);
		return NULL;
	}

	return process;
}

void destroy_instruction_iteratee(void *elem) {
	instruction_destroy((t_instruction*) elem);
}

void process_destroy(t_process *process) {
	if (process != NULL) {
		free(process->size);
		list_iterate(process->instructions, destroy_instruction_iteratee);
		free(process->instructions);
	}
}

t_log* create_logger() {
	t_log *nuevo_logger;
	nuevo_logger = log_create("console.log", "CONSOLE", 1, LOG_LEVEL_INFO);
	return nuevo_logger;
}

t_config* create_config() {
	t_config *nuevo_config;
	nuevo_config = config_create("console.config");
	return nuevo_config;
}
