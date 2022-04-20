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

int get_code_from_path(char* path);
t_instruction* create_instruction(char* name,int* params);
void destroy_instruction(t_instruction* instruct);
t_instruction* turn_string_into_instruction(char* string);

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
	printf("Code Path:");
	printf("%s",code_path);
	printf("Process Size:");
	printf("%s",process_size);
	printf("HOLA");
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

		t_instruction* instruct = turn_string_into_instruction(line_buf);
		printf("id: %s \n",instruct->instruction_id);
		int i=0;
		while(instruct->instruction_params[i]!=NULL){
			printf("param num %d : %d \n",i+1,instruct->instruction_params[i]);
			i++;
		}
		destroy_instruction(instruct);
		}
	fclose(instructionsFile);
	// leer archivo
		// parse_instruction()
	// escupir contenido de archivo en lista
	// cerrar archivo

	return 0;
}

t_instruction* turn_string_into_instruction(char* string){
	printf("HOLA");
	char * word = strtok(string, " ");
	int n=0;
	int* params=malloc(sizeof(int*));
	char* name_param=strtok(NULL," ");
	while(name_param!=NULL){

		params[n]=atoi(name_param);

		n++;
		name_param=strtok(NULL," ");
	}
	t_instruction* instruct = create_instruction(word,params);
	free(params);
	return instruct;
}

t_instruction* create_instruction(char* name,int* params){
	int n=0;
	t_instruction* instruct=malloc(sizeof(t_instruction));
	instruct->instruction_id=name;
	while(params[n]!=NULL){
		instruct->instruction_params[n] = params[n];
		n++;
	}
	return instruct;
}

void destroy_instruction(t_instruction* instruct){
	free(instruct->instruction_id);
	free(instruct->instruction_params);
	free(instruct);
}
