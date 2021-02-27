#ifndef PGED_H
#define PGED_H

#include <termios.h>
#include <time.h>
#include <stdbool.h>

#define PGED_VERSION "0.0.1"
#define QUIT_TIMES 1
#define TAB_SIZE 4

typedef struct erow {
    int size;
    int rsize;
    char *chars; // plain string
    char *render; // processed string which will be rendered on screen
} erow;
struct editorConfig {
    int cx, cy;
    int rx; // x of redender row
    int coloff; // column offset
    int rowoff; // row offset
    int screenrows; // count of screen rows
    int screencols; // count of screen cols
    int numrows; // count of file rows
    erow *rows; // array of file rows
    int dirty; // flag
    
    char *filename;
    bool file_exist; // is file exist
    bool file_write_rights;

    char statusmsg[80];
    time_t statusmsg_time;
    struct termios orig_termios;
};

extern struct editorConfig E;

#endif
