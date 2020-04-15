
#ifndef NX_SHELL_CONFIG_H
#define NX_SHELL_CONFIG_H

#include <switch.h>

typedef struct {
	int sort = 0;
	bool dark_theme = false;
	char cwd[FS_MAX_PATH];
} config_t;

extern config_t config;

int Config_Save(config_t config);
int Config_Load(void);

#endif
