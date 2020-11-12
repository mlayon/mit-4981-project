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

            key = strtok(line, delimiter); // grab the token before the '=' sign
            printf("before: %s", value);
            value = strtok(NULL, delimiter); // grab the token after the '=' sign
            printf("after: %s\n", value);
            if (strcmp(key, "port") == 0) {
                conf->port = atoi(value);

            } else if (strcmp(key, "subprocess") == 0) {
                conf->subprocess = value[0];

            } else if (strcmp(key, "root") == 0) {
                conf->root = value;

            } else if (strcmp(key, "error") == 0) {
                conf->errorpage = value;

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
    conf->root = "..";
    conf->errorpage = "index.html";
    return 1;
}
