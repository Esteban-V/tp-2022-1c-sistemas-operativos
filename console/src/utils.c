/*
 * utils.c
 *
 *  Created on: May 9, 2022
 *      Author: utn-so
 */

#include"utils.h"

FILE* open_file(char *path) {
	FILE *file = fopen(path, "r");
	if (file == NULL) {
		exit(-1);
	}
	return file;
}

t_log* create_logger() {
	new_logger = log_create("console.log", "CONSOLE", 1, LOG_LEVEL_INFO);
	return new_logger;
}

t_config* create_config() {
	new_config = config_create("console.config");
	return new_config;
}
