#ifndef CONFIG_H
#define CONFIG_H

typedef struct {
    int port;
    char subprocess;
} Config;

int get_config_file(Config *conf, char *filename);
int get_config_defaults(Config *conf);

#endif