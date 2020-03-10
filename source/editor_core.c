#define _DEFAULT_SOURCE
#define _GNU_SOURCE
#define _BSD_SOURCE

#include <termio.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#include "editor_core.h"
#include "pged.h"
#include "user_io.h"


void die(const char *error)
{
    write(STDOUT_FILENO, C_ERASE_DISPLAY, 4);
    write(STDOUT_FILENO, C_START_CURSOR_POS, 3);
    
    perror(error);
    exit(1);
}


void disableRawMode(void)
{
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.orig_termios) == -1)
        die("tcsetattr");
}


void enableRawMode(void)
{
    if (tcgetattr(STDIN_FILENO, &E.orig_termios) == -1) die("tcgetattr: Error with coping default setting");
    atexit(disableRawMode);

    struct termios raw = E.orig_termios;

    raw.c_iflag &= ~(BRKINT | INPCK | ISTRIP | ICRNL | IXON);
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag |= ~(CS8);
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) die("tcsetattr: Error with writing setting for raw mode");
}


int editorReadKey(void)
{
    int nread; // number of bytes read
    char c;
    while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
        if (nread == -1 && errno != EAGAIN) die("read: Error in reading key from STDIN");
    }
    editorSetStatusMessage("%d, printed", c);

    if (c == '\x1b') {
        char seq[3]; // array for sequance keys

        if (read(STDIN_FILENO, &seq[0], 1) != 1) return '\x1b';
        if (read(STDIN_FILENO, &seq[1], 1) != 1) return '\x1b';

        // in different systems sequances of HOME and END can be different

        if (seq[0] == '[') {
            
            if (seq[1] >= '0' && seq[1] <= '9') {
                if (read(STDIN_FILENO, &seq[2], 1) != 1) return '\x1b';
                if (seq[2] == '~') {
                    switch (seq[1]) {
                        case '1': return HOME_KEY;
                        case '3': return DEL_KEY;
                        case '4': return END_KEY;
                        case '5': return PAGE_UP; // PgUp key is "\x1b[5~"
                        case '6': return PAGE_DOWN; // PgUp key is "\x1b[6~"
                        case '7': return HOME_KEY;
                        case '8': return END_KEY;
                    }
                }
            }

            switch (seq[1]) {
                case 'A': return ARROW_UP;
                case 'B': return ARROW_DOWN;
                case 'C': return ARROW_RIGHT;
                case 'D': return ARROW_LEFT;
                case 'H': return HOME_KEY;
                case 'F': return END_KEY;
            }
        } else if (seq[0] == 'O') {
            switch (seq[1])
            {
                case 'H': return HOME_KEY;
                case 'F': return END_KEY;
            }
        }
        return '\x1b';
    }

    return c;
}


int getCursorPosition(int *rows, int *cols)
{
    char curpos[32]; // string for cursor position

    unsigned int i = 0;

    // ask shell cursor position, it will be printed to stdout
    if (write(STDOUT_FILENO, "\x1b[6n", 4) != 4) return -1;

    while (i < sizeof(curpos) - 1) {
        if (read(STDIN_FILENO, &curpos[i], 1) != 1) break;
        if (curpos[i] == 'R') break;
        i++;
    }
    curpos[i] = '\0';

    if (curpos[0] != '\x1b' || curpos[1] != '[') return -1;
    if (sscanf(&curpos[2], "%d;%d", rows, cols) != 2) return -1;

    return 0;
}


int getWindowSize(int *rows, int *cols)
{
    struct winsize ws;

    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
        // if TIOCGWINSZ is not supported, get size of windows via cursors position

        // send cursor to right bottom corner
        if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12) return -1;
        return getCursorPosition(rows, cols);
    } else {
        *cols = ws.ws_col;
        *rows = ws.ws_row;
        return 0;
    }
}
