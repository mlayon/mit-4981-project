#ifndef HELPER_H
#define HELPER_H

void error(char *msg);
void parse_url(char filename[], char uri[], char cgiargs[], char *html_root);
void display_content(int childfd, FILE *stream, char filename[], char filetype[], struct stat sbuf);
void print_response_header(FILE *stream,  char filename[],char filetype[], struct stat sbuf);
void cerror(int childfd, FILE *stream, char *errorfile);
size_t get_file_size(const char *filename);
void bind_port(int parentfd, struct sockaddr_in serveraddr, int portno);

#endif
