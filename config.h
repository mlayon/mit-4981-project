#ifndef CONFIG_H
#define CONFIG_H

typedef struct {
    int port;
    char subprocess;
    char *root;
    char *error;
} Config;

int get_config_file(Config *conf);
int get_config_defaults(Config *conf);
void free_space(Config *conf);
int parse_line(char *line, char **key, char **value);
char *trim(char *str);

#endif
