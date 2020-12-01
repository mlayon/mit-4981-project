#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "config.h"

// Gets server configuration from a .conf file
int get_config_file(Config *conf) {

    FILE *file;
    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    char *key;
    char *value;
    
    // Opening file pointer
    file = fopen("config.conf", "r");
    if (file != NULL) {

        // reading through file
        while ((read = getline(&line, &len, file)) != -1) {
            if (parse_line(line, &key, &value))
                continue;

            if (strcmp(key, "port") == 0) {
                conf->port = atoi(value);

            } else if (strcmp(key, "subprocess") == 0) {
                conf->subprocess = value[0];

            } else if (strcmp(key, "root") == 0) {
                conf->root = malloc(strlen(value) + 1);
                strcpy(conf->root, value);  

            } else if (strcmp(key, "error") == 0) {
                conf->error = malloc(strlen(value) + 1);
                strcpy(conf->error, value);

            }
        }
        fclose(file);
        return 1;
    }

    // If "config.conf" file is not found, use the defaults
    get_config_defaults(conf);

    return 0;
}

// Empty get_config function, sets server config defaults
int get_config_defaults(Config *conf) {
    conf->port = 49157;
    conf->subprocess = 't';
    conf->root = ".";
    conf->error = "404.html";
    return 1;
}

// Free memory allocated for root and error configs
void free_space(Config *conf) {
    free(conf->root);
    free(conf->error);
}

// Trims spaces 
char *trim(char *str) {
    char *start = str;
    char *end = str + strlen(str);

    while(*start && isspace(*start))
        start++;

    while(end > start && isspace(*(end - 1)))
        end--;

    *end = '\0';
    return start;
}

// Parse through line and look for key-value pair
int parse_line(char *line, char **key, char **value) {
    char *ptr = strchr(line, '=');
    if (ptr == NULL)
        return -1;

    *ptr++ = '\0';
    *key = trim(line);
    *value = trim(ptr);

    return 0;
}
