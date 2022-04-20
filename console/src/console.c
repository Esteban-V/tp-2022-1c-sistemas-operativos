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

t_instruction turn_string_into_instruction(char* string);

int main(int argc, char** argv) {
	// iniciar logger

	if(argc < 3) {
		// tirar error con logger y cortar
		return EXIT_FAILURE;
	}
	if(argc > 3) {
		// tirar warning con logger e ignorar otros params
		puts('Unused params');
	}

	char* code_path = argv[1];
	char* process_size = argv[2];
	puts("Code Path:");
	puts(code_path);
	puts("Process Size:");
	puts(process_size);


	get_code_from_path(code_path);

	t_list* listOfInstructions = list_create();
	// abrir config

	// abrir conexion con server kernel segun datos config
	// enviar tamano
	// enviar lista (como paquete)

	// esperar resultado
	// tirar info/error resultado con logger

	// cerrar conexion
	return EXIT_SUCCESS;
}



int get_code_from_path(char* path) {
	FILE* instructionsFile = fopen(path,"r");
	if(instructionsFile==NULL){
			puts("couldnt open file");
			return -1;
		}

	char* line_buf = NULL;
	size_t line_buf_size = 0;
	ssize_t line;


	while((line =getline(&line_buf,&line_buf_size,instructionsFile))!=-1){
		puts(line_buf);
		t_instruction instruct = turn_string_into_instruction(line_buf);
		printf("id: %s \n",instruct.instruction_id);
		int i=0;
		while(instruct.instruction_params[i]!=NULL){
			printf("param num %d : %d \n",i+1,instruct.instruction_params[i]);
			i++;
		}
		}
	fclose(instructionsFile);
	// leer archivo
		// parse_instruction()
	// escupir contenido de archivo en lista
	// cerrar archivo

	return 0;
}

t_instruction turn_string_into_instruction(char* string){
	char * word = strtok(string, " ");
	int param;
	int n=0;
	t_instruction instruct;
	instruct.instruction_id = word;
	word = strtok(NULL," ");
			while(word!=NULL){
				puts(word);
				param=atoi(word);
				instruct.instruction_params[n] = param;
				n++;
				word = strtok(NULL," ");

				if(word==NULL){
					puts("ES NULO");
				}
			}
			return instruct;
}
