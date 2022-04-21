/*
 ============================================================================
 Name        : console.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include "console.h"

FILE* open_file(char* path);
void get_code(FILE* file, t_list* list);
t_instruction* parse_instruction(char* string);
t_instruction* new_instruction(size_t id_size);
void destroy_instruction(t_instruction* instruct);
void terminate_console();

int main(int argc, char** argv) {
	// iniciar logger


	if(argc < 3) {
		puts("Missing params");
		// tirar error con logger y cortar
		return EXIT_FAILURE;
	}
	if(argc > 3) {
		// tirar warning con logger e ignorar otros params
		puts('Unused params');
	}

	char* code_path = argv[1];
	char* process_size = argv[2];
	printf("Code Path: ");
	printf("%s\n",code_path);
	printf("Process Size: ");
	printf("%s\n",process_size);

	FILE* instruction_file = open_file(code_path);
	t_list* instruction_list = list_create();
	get_code(instruction_file, instruction_list);

	// abrir config

	// abrir conexion con server kernel segun datos config
	// enviar tamano
	// serializar lista
	// enviar lista (como paquete)

	// esperar resultado
	// tirar info/error resultado con logger

	terminate_console();
	return EXIT_SUCCESS;
}


void get_code(FILE* file, t_list* list) {
	char* line_buf = NULL;
	size_t line_buf_size = 0;
	ssize_t lines_read;

	t_instruction* instruction;

	lines_read = getline(&line_buf, &line_buf_size, file);
	while (lines_read != -1) {
		instruction = parse_instruction(line_buf);
		puts(instruction->id);

		lines_read = getline(&line_buf, &line_buf_size, file);
	}
	destroy_instruction(instruction);
	fclose(file);
}

t_instruction* parse_instruction(char* string) {
	int param;
	int i = 0;

	char* id = strtok(string, " ");
	t_instruction* instruction = new_instruction(strlen(id));
	instruction->id = id;

	char* params = strtok(NULL, " ");
	while(params != NULL){
		param = atoi(params);
		instruction->params[i] = param;
		params = strtok(NULL," ");
		i++;
	}
	return instruction;
}

FILE* open_file(char* path) {
	FILE* file = fopen(path, "r");
	if (file == NULL){
		exit(-1);
	}
	return file;
}

void terminate_console() {
	//close connection
}

t_instruction* new_instruction(size_t id_size) {
    // Try to allocate instruction structure.
    t_instruction *instruction = malloc(sizeof(t_instruction));
    if (instruction == NULL)
        return NULL;

    // Try to allocate instruction id and params, free structure if fail.
    instruction->id = malloc (id_size * sizeof (char));
    if (instruction->id == NULL) {
        free(instruction);
        return NULL;
    }

    instruction->params = malloc(2 * sizeof (int));
    if (instruction->params == NULL) {
		free(instruction);
		return NULL;
	}

    return instruction;
}

void destroy_instruction (t_instruction *instruction) {
    if (instruction != NULL) {
        free (instruction->id);
        free (instruction->params);
        free (instruction);
    }
}
