#include <stdio.h>
#include "utils.h"

void Utils_GetSizeString(char *string, s64 size) {
	double double_size = (double)size;

	int i = 0;
	static const char *units[] = {"B", "KB", "MB", "GB", "TB", "PB", "EB", "ZB", "YB"};

	while (double_size >= 1024.0f) {
		double_size /= 1024.0f;
		i++;
	}

	sprintf(string, "%.*f %s", (i == 0) ? 0 : 2, double_size, units[i]);
}
