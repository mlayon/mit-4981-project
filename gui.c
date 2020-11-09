// https://tldp.org/HOWTO/NCURSES-Programming-HOWTO/forms.html
// second example 
#include <form.h>

int main() {
    FIELD *field[3];
    FORM *my_form;
    int ch;

    // init curses
    initscr();
    start_color();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);

    // init color pairs
    init_pair(1, COLOR_WHITE, COLOR_BLUE);
    init_pair(2, COLOR_WHITE, COLOR_BLUE);

    // init the fields
    field[0] = new_field(1, 10, 4, 18, 0, 0);
    field[1] = new_field(1, 10, 6, 18, 0, 0);
    field[2] = NULL;

    // set field options
    set_field_fore(field[0], COLOR_PAIR(1)); // put field with white foreground -> chars printed in white
    set_field_back(field[0], COLOR_PAIR(2)); // put field with blue background

    field_opts_off(field[0], O_AUTOSKIP); // don't go to next field when this field is filled up

    set_field_back(field[1], A_UNDERLINE);
    field_opts_off(field[1], O_AUTOSKIP);

    // create the form and post it
    my_form = new_form(field);
    post_form(my_form);
    refresh();

    set_current_field(my_form, field[0]); // set focus to the colored field
    mvprintw(4, 10, "Value 1:");
    mvprintw(6, 10, "Value 2:");
    mvprintw(LINES - 2, 0, "Use UP, DOWN arrow keys to switch between fields");
    refresh();

    // loop through to get user requests
    while ((ch = getch()) != KEY_F(1)) {

        switch(ch) {
            case KEY_DOWN: // go to next field
                form_driver(my_form, REQ_NEXT_FIELD);
                // go to the end of the present buffer
                // leaves nicely at the last chracter
                form_driver(my_form, REQ_END_LINE);
                break;
            case KEY_UP: // go to prev field
                form_driver(my_form, REQ_PREV_FIELD);
                form_driver(my_form, REQ_END_LINE);
                break;
            default: // if this is a normal character it gets printed
                form_driver(my_form, ch);
                break;
        }
    }

    // unpost form and free the memory
    unpost_form(my_form);
    free_form(my_form);
    free_field(field[0]);
    free_field(field[1]);

    endwin();
    return 0;
}