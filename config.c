#include <stdio.h>
#include "config.h"

// Gets server configuration from a .config file
// src: https://stackoverflow.com/questions/29431081/config-file-for-web-server-to-receive-get-request-in-c/29431696
int get_config_file(Config *conf, char *filename) {

    FILE *file;
    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    
    // Opening file pointer
    file = fopen(filename, "r");
    if (file != NULL) {

        while ((read = getline(&line, &len, file)) != -1) {

            // Find key names in file 
            if (strstr(line, "port") != NULL) {
                conf->port = atoi(line + 4);
            }
        }
        fclose(file);
        return 1;
    }

    return 0;
}

// Empty get_config function, sets server config defaults
int get_config_defaults(Config *conf) {
    conf->port = 49157;
    conf->subprocess = 't';
    return 1;
}