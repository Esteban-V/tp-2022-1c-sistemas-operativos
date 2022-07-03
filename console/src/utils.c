#include "utils.h"

FILE *open_file(char *path, void (*error_clb)())
{
	FILE *file = fopen(path, "r");
	if (file == NULL)
	{
		error_clb();
	}
	return file;
}