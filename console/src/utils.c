#include"utils.h"

FILE* open_file(char *path) {
	FILE *file = fopen(path, "r");
	if (file == NULL) {
		exit(-1);
	}
	return file;
}