// How to run:
// gcc -std=c11 -Wall -Werror -pedantic -o gui gui.c -lform -lncurses
#include <unistd.h>
#include <ncurses.h>
#include <form.h>
#include <string.h>
#include <ctype.h> // for isspace
#include <stdlib.h>
#include <sys/stat.h>
 #include <fcntl.h> 

#define FIELD_COUNT 4
size_t strnlen(const char *s, size_t maxlen);
int access(const char *pathname, int mode);
static FORM *form;
static FIELD *field[9];
static WINDOW *win_body, *win_form;
static char *keys[] = {
    "port",
    "subprocess",
    "root",
    "error"
};

// Returns 1 if directory exists
int is_directory_exists(const char *path) {
    struct stat stats;

    stat(path, &stats);

    // Check for file existence
    if (S_ISDIR(stats.st_mode))
        return 1;

    return 0;
}

// Trims whitespaces to be used for field buffers
static char* trim_whitespaces(char *str) {
	char *end;

	// trim leading space
	while(isspace(*str))
		str++;

	if(*str == 0) // all spaces?
		return str;

	// trim trailing space
	end = str + strnlen(str, 128) - 1;

	while(end > str && isspace(*end))
		end--;

	// write new null terminator
	*(end+1) = '\0';

	return str;
}

// Returns the file name extension
char *get_filename_ext(char *filename) {
    char *dot = strrchr(filename, '.');
    if(!dot || dot == filename) {
        return "";
    }
    return dot + 1;
}

// Clears the config file
static void clear_config_file(void) {

    FILE *fptr;

    fptr = fopen("config.conf", "w");
    if (fptr == NULL) {
        printf("error!");
        exit(EXIT_FAILURE);
    }
    fflush(fptr);
    fclose(fptr);
}

// Appends the given key-value pair into the config file
static void write_config_file(char *key, char *value) {

    FILE *fptr;

    fptr = fopen("config.conf", "a");
    if (fptr == NULL) {
        printf("error!");
        exit(EXIT_FAILURE);
    }
    int invalid = 0; // is 1 if input is invalid

     // checking if input is a valid port number
    int length = strlen (value);
    if (strcmp(key, "port") == 0) {
        // if length is of a valid port number 
        if (length == 4 || length == 5) {

            for (int i=0; i < length; i++) {
                // if we see a non-digit character, it's an invalid port
                if (!isdigit(value[i])){
                    invalid = 1;
                    break;
                }
            }
        // if length is too short/too long
        } else {
            invalid = 1;
        }

        if (invalid) {
            printw ("Invalid port number. Setting port to 49157.\n");
            strcpy(value, "49157");
        }

    } else if (strcmp(key, "subprocess") == 0) {
        if (length == 1) {
            // if character is not in any valid inputs
            if ((strcmp(value, "t") != 0) && (strcmp(value, "p") != 0)) {
                invalid = 1;
            }
        } else {
            invalid = 1;
        }
        
        if (invalid) {
            printw ("Invalid subprocess character. Setting it to threads.\n");
            strcpy(value, "t");
        }

    } else if (strcmp(key, "root") == 0) {

        if (!is_directory_exists(value)) {
            printw ("Invalid directory. Setting it to current directory.\n");
            strcpy(value, ".");
        }

    } else if (strcmp(key, "error") == 0) {

        if(access(value, F_OK) != -1) {
            char *ext = get_filename_ext(value);
            if ((strcmp(ext, "txt") != 0) && (strcmp(ext, "html") != 0)) {
                invalid = 1;
            } 
        } else {
            invalid = 1;
        }   
        
        if (invalid) {
            printw ("Invalid error file. Setting it to 404.html.\n");
            strcpy(value, "t");
        }
    }
           
    fputs(key, fptr);
    fputs("=", fptr);
    fprintf(fptr, "%s\n", value);
    fflush(fptr);
    fclose(fptr);
}

// Executes specific actions depending on user's keystroke
static void driver(int ch) {
	int i;
    int label_count = 0;

	switch (ch) {
		case KEY_F(4):
			// Or the current field buffer won't be sync with what is displayed
			form_driver(form, REQ_NEXT_FIELD);
			form_driver(form, REQ_PREV_FIELD);
			move(LINES-3, 2);

            clear_config_file();
			for (i = 0; field[i]; i++) {
                // display-only fields are at an even index
                // input-only fields are at an odd index
                // if we're at an input label, call write function
                if (i % 2 != 0) { 
                    char *current = trim_whitespaces(field_buffer(field[i], 0));
                    write_config_file(keys[label_count++], current);
                }

			}

            printw("\nConfigurations written in config.conf");

			refresh();
			pos_form_cursor(form);
			break;

		case KEY_DOWN:
			form_driver(form, REQ_NEXT_FIELD);
			form_driver(form, REQ_END_LINE);
			break;

		case KEY_UP:
			form_driver(form, REQ_PREV_FIELD);
			form_driver(form, REQ_END_LINE);
			break;

		case KEY_LEFT:
			form_driver(form, REQ_PREV_CHAR);
			break;

		case KEY_RIGHT:
			form_driver(form, REQ_NEXT_CHAR);
			break;

		// Delete the char before cursor
		case KEY_BACKSPACE:
		case 127:
			form_driver(form, REQ_DEL_PREV);
			break;

		// Delete the char under the cursor
		case KEY_DC:
			form_driver(form, REQ_DEL_CHAR);
			break;

		default:
			form_driver(form, ch);
			break;
	}

	wrefresh(win_form);
}


int main() {

    int ch;
    
    // initializing ncurses
    initscr();
    noecho();
    cbreak();
    keypad(stdscr, TRUE); // toggling arrows, other keypad btns are usable

    win_body = newwin(24, 80, 0, 0);
    win_form = derwin(win_body, 18, 78, 4, 1);

    // printing basic info about gui program
    mvwprintw(win_body, 1, 2, "Configuration File Editor");
    mvwprintw(win_body, 2, 2, "Use arrow keys to navigate between fields.");
    mvwprintw(win_body, 3, 2, "Press F2 to quit and F4 to confirm server configurations.");

    // making fields
    field[0] = new_field(1, 25, 0, 0, 0, 0); // port number label
	field[1] = new_field(1, 40, 0, 30, 0, 0);
	field[2] = new_field(1, 25, 2, 0, 0, 0); // thread/process label
	field[3] = new_field(1, 40, 2, 30, 0, 0);
    field[4] = new_field(1, 25, 4, 0, 0, 0); // root directory label
	field[5] = new_field(1, 40, 4, 30, 0, 0);
    field[6] = new_field(1, 25, 6, 0, 0, 0); // 404 page label
	field[7] = new_field(1, 40, 6, 30, 0, 0);
    field[8] = NULL; // declaring last field in array as null because it's like a linkedlist

    // setting the field buffers
    set_field_buffer(field[0], 0, "Port Number: ");
    set_field_buffer(field[1], 0, "49157");
    set_field_buffer(field[2], 0, "Thread or Process (t/p): ");
    set_field_buffer(field[3], 0, "t");
    set_field_buffer(field[4], 0, "Root Directory: ");
    set_field_buffer(field[5], 0, ".");
    set_field_buffer(field[6], 0, "404 Page: ");
    set_field_buffer(field[7], 0, "404.html");

    set_field_opts(field[0], O_VISIBLE | O_PUBLIC | O_AUTOSKIP);
	set_field_opts(field[1], O_VISIBLE | O_PUBLIC | O_EDIT | O_ACTIVE);
	set_field_opts(field[2], O_VISIBLE | O_PUBLIC | O_AUTOSKIP);
	set_field_opts(field[3], O_VISIBLE | O_PUBLIC | O_EDIT | O_ACTIVE);
    set_field_opts(field[4], O_VISIBLE | O_PUBLIC | O_AUTOSKIP);
	set_field_opts(field[5], O_VISIBLE | O_PUBLIC | O_EDIT | O_ACTIVE);
    set_field_opts(field[6], O_VISIBLE | O_PUBLIC | O_AUTOSKIP);
	set_field_opts(field[7], O_VISIBLE | O_PUBLIC | O_EDIT | O_ACTIVE);

    // adding underlines on the fill-able fields for readability
    set_field_back(field[1], A_UNDERLINE); 
	set_field_back(field[3], A_UNDERLINE);
    set_field_back(field[5], A_UNDERLINE);
    set_field_back(field[7], A_UNDERLINE);

    // setting form into window
    form = new_form(field);
    set_form_win(form, win_form);
    set_form_sub(form, derwin(win_form, 18, 76, 1, 1));
    post_form(form);

    refresh(); // refresh for the stdscr itself 
    wrefresh(win_body); // wrefresh for the windows
    wrefresh(win_form);

    // read loop
    while ((ch = getch()) != KEY_F(2)) {
        driver(ch);
    }

    // form and window clean up
    unpost_form(form);
    free_form(form);
    // freeing each field
    for (int i = 0; i < FIELD_COUNT; i++) {
        free_field(field[i]);
    }
    delwin(win_form);
    delwin(win_body);
    endwin();

    return 0;

}


