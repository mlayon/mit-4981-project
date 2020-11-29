#ifndef HELPER_H
#define HELPER_H

void error(char *msg);
void parse_url(char filename[], char uri[], char cgiargs[], char *html_root);
void display_content(int childfd, FILE *stream, int fd, char *p, char filename[], char filetype[], struct stat sbuf);
void cerror(int childfd, FILE *stream, char *errorfile);
size_t get_file_size(const char *filename);
void get_error_check(int childfd, FILE *stream, char *errorfile);
void bind_port(int parentfd, struct sockaddr_in serveraddr, int portno);

#endif