#include "console.h"

int main(int argc, char **argv)
{
	logger = log_create("./cfg/console.log", "CONSOLE", 1, LOG_LEVEL_INFO);
	config = config_create("./cfg/console.config");

	if (argc < 3)
	{
		log_error(logger, "Missing parameters");
		return EXIT_FAILURE;
	}

	if (argc > 3)
	{
		log_warning(logger, "Unused parameters");
	}

	if (!config || !config_has_property(config, "IP_KERNEL") || !config_has_property(config, "PUERTO_KERNEL"))
	{
		log_error(logger, "Config failed to load");
		return EXIT_FAILURE;
	}

	kernel_ip = config_get_string_value(config, "IP_KERNEL");
	kernel_port = config_get_string_value(config, "PUERTO_KERNEL");

	kernel_socket = connect_to(kernel_ip, kernel_port);

	if (kernel_socket == -1)
	{
		terminate_console(true);
	}

	pthread_mutex_lock(&mutex_log);
	log_info(logger, "Console connected to kernel");
	pthread_mutex_unlock(&mutex_log);

	char *code_path = argv[1];
	uint32_t process_size = atoi(argv[2]);

	instruction_list = list_create();

	FILE *instruction_file = open_file(code_path, error_opening_file);
	get_code(instruction_file);
	fclose(instruction_file);

	if (!list_size(instruction_list))
	{
		pthread_mutex_lock(&mutex_log);
		log_warning(logger, "Provided code has no instructions");
		pthread_mutex_unlock(&mutex_log);
		terminate_console(true);
	}

	process = create_process();
	if (!process)
	{
		terminate_console(true);
	}

	memcpy(process->instructions, instruction_list, sizeof(t_list));
	process->size = process_size;

	pthread_mutex_lock(&mutex_log);
	
	log_info(logger, "Process found with %d instructions",
			 list_size(process->instructions));
	
	pthread_mutex_unlock(&mutex_log);
	log_info(logger, "AAAAAAAAA");	
	t_packet *process_packet = create_packet(NEW_PROCESS, INITIAL_STREAM_SIZE);
	log_info(logger, "BBB");	
	stream_add_process(process_packet, process);
	log_info(logger, "CCCC");	
	if (kernel_socket != -1)
	{
		log_info(logger, "DDDD");	
		socket_send_packet(kernel_socket, process_packet);
		log_info(logger, "EEE");	
	}

	packet_destroy(process_packet);
	uint8_t result = socket_receive_header(kernel_socket);

	if (result)
	{
		pthread_mutex_lock(&mutex_log);
		log_error(logger, "Process exited with error code: %d", result);
		pthread_mutex_unlock(&mutex_log);
	}
	else
	{
		pthread_mutex_lock(&mutex_log);
		log_info(logger, "Process exited successfully");
		pthread_mutex_unlock(&mutex_log);
	}

	terminate_console(result);
}

void get_code(FILE *file)
{
	char *line_buf = NULL;
	size_t line_buf_size = 0;
	ssize_t lines_read;

	lines_read = getline(&line_buf, &line_buf_size, file);
	while (lines_read != -1)
	{
		list_add(instruction_list, parse_instruction(line_buf));
		lines_read = getline(&line_buf, &line_buf_size, file);
	}
}

t_instruction *parse_instruction(char *string)
{
	int i = 0;

	char **instruction_text = string_split(string, " ");
	char *id = instruction_text[0];
	string_trim(&id);
	t_instruction *instruction = create_instruction(string_length(id));
	memcpy(instruction->id, id, string_length(id) + 1);

	char *next_param;
	while ((next_param = instruction_text[i + 1]) != NULL)
	{
		string_trim(&next_param);
		int param = atoi(next_param);
		int *param_pointer = malloc(sizeof(int));
		memcpy(param_pointer, &param, sizeof(int));
		list_add(instruction->params, param_pointer);
		i++;
	}

	return instruction;
}

void error_opening_file()
{
	pthread_mutex_lock(&mutex_log);
	log_error(logger, "Provided file path does not exist");
	pthread_mutex_lock(&mutex_log);
	terminate_console(true);
}

void terminate_console(bool error)
{
	log_destroy(logger);
	config_destroy(config);
	process_destroy(process);
	close(kernel_socket);
	exit(error ? EXIT_FAILURE : EXIT_SUCCESS);
}
