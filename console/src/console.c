/*
 ============================================================================
 Name        : console.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <console.h>

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

	puts(code_path);
	puts(process_size);

	int code = get_code_from_path(code_path);

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
	Instruction* instruction_list;

	// leer archivo
		// parse_instruction()
	// escupir contenido de archivo en lista
	// cerrar archivo

	return instruction_list;
}
