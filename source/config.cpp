#include <stdio.h>
#include <string.h>
#include <jansson.h>
#include "config.h"
#include "fs.h"

#define CONFIG_VERSION 0

config_t config;
const char *config_file = "{\n\t\"config_ver\": %d,\n\t\"sort\": %d,\n\t\"dark_theme\": %d,\n\t\"last_dir\": \"%s\"\n}";
static int config_version_holder = 0;
static const int buf_size = 128 + FS_MAX_PATH;

int Config_Save(config_t config) {
    Result ret = 0;
    char *buf = (char *)calloc(buf_size, sizeof(char));
    u64 len = snprintf(buf, buf_size, config_file, CONFIG_VERSION, config.sort, config.dark_theme, config.cwd);
    
    if (R_FAILED(ret = FS_WriteFile("/switch/NX-Shell/config.json", buf, len))) {
        free(buf);
        return ret;
    }
    
    free(buf);
    return 0;
}

int Config_Load(void) {
    Result ret = 0;
    
    if (!FS_DirExists("/switch/"))
        fsFsCreateDirectory(fs, "/switch");
    if (!FS_DirExists("/switch/NX-Shell/"))
        fsFsCreateDirectory(fs, "/switch/NX-Shell");
    
    if (!FS_FileExists("/switch/NX-Shell/config.json")) {
        config.sort = 0;
        config.dark_theme = true;
        strcpy(config.cwd, "/");
        return Config_Save(config);
    }
    
    s64 size = 0;
    FS_GetFileSize("/switch/NX-Shell/config.json", &size);

    char *buf =  (char *)calloc(size, sizeof(char));
    if (R_FAILED(ret = FS_ReadFile("/switch/NX-Shell/config.json", buf, size))) {
        free(buf);
        return ret;
    }

    json_t *root;
    json_error_t error;
    root = json_loads(buf, 0, &error);
    free(buf);

    if (!root) {
        printf("error: on line %d: %s\n", error.line, error.text);
        return -1;
    }

    json_t *config_ver = json_object_get(root, "config_ver");
    config_version_holder = json_integer_value(config_ver);

    json_t *sort = json_object_get(root, "sort");
    config.sort = json_integer_value(sort);

    json_t *dark_theme = json_object_get(root, "dark_theme");
    config.dark_theme = json_integer_value(dark_theme);

    json_t *last_dir = json_object_get(root, "last_dir");
    strcpy(config.cwd, json_string_value(last_dir));

    if (!FS_DirExists(config.cwd))
        strcpy(config.cwd, "/");
    
    // Delete config file if config file is updated. This will rarely happen.
    if (config_version_holder  < CONFIG_VERSION) {
        fsFsDeleteFile(fs, "/switch/NX-Shell/config.json");
        config.sort = 0;
        config.dark_theme = true;
        strcpy(config.cwd, "/");
        return Config_Save(config);
    }

    json_decref(root);
    return 0;
}
