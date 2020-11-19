#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "config.h"

// Gets server configuration from a .config file
int get_config_file(Config *conf) {

    FILE *file;
    char line[128];
    char *key;
    char *value;
    char *delimiter = "=";
    
    // Opening file pointer
    file = fopen("config.conf", "r");
    if (file != NULL) {

        // reading through file
        while(fgets(line, sizeof(line), file) != NULL) {
            // printf("before key: %s", key);
            key = strtok(line, delimiter); // grab the token before the '=' sign
            // printf("after key: %s\n", key);
            // printf("before value: %s", value);
            value = strtok(NULL, delimiter); // grab the token after the '=' sign
            // printf("after value: %s\n", value);
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
