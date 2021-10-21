#define _DEFAULT_SOURCE
#define _GNU_SOURCE
#define _BSD_SOURCE

#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>
#include <sys/ioctl.h>

#include "editor_core.h"
#include "pged.h"
#include "user_io.h"


void die(const char *error)
{
    write(STDOUT_FILENO, C_ERASE_DISPLAY, 4);
    write(STDOUT_FILENO, C_START_CURSOR_POS, 3);
    
    perror(error);
    write(STDOUT_FILENO, "\x1b[E", 3);

    exit(1);
}


void disableRawMode(void)
{
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.orig_termios) == -1)
        die("tcsetattr");
}


void enableRawMode(void)
{
    if (tcgetattr(STDIN_FILENO, &E.orig_termios) == -1)  // Copy initial terminal state into orig_termios
        die("tcgetattr: Error with coping default setting");
    atexit(disableRawMode); // Restore settings on exit

    struct termios raw = E.orig_termios;

    raw.c_iflag &= ~( // Disable
        BRKINT        // break condition
        | INPCK       // parity checking
        | ISTRIP      // striping 8th bit
        | ICRNL       // carriage, allow Enter and CTRL-M to be read
        | IXON        // software flow control (prevent defaults for: CTRL-S, CTRL-Q)
    );
    raw.c_oflag &= ~(OPOST); // Disable post processing of output (\n, \r\n)
    raw.c_cflag |= ~(CS8); // Set character size to 8 bits
    raw.c_lflag &= ~( // Disable
        ECHO          // user input on type
        | ICANON      // cannonical mode (disable reading line-by-line)
        | IEXTEN      // prevent defaults for: CTRL-V, CTRL-O
        | ISIG        // interrupt signals (prevent defaults for: CTRL-C, CTRL-Z, CTRL-Y)
    );
    raw.c_cc[VMIN] = 0; // Set minimum bytes for reading to 0. Means, we read every produced input
    raw.c_cc[VTIME] = 1; // Set wait time for read() to 100 miliseconds

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) // Apply new setting to terminal state
        die("tcsetattr: Error with writing setting for raw mode");
}


int editorReadKey(void)
{
    int nread; // number of bytes read
    char c;
    while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
        if (nread == -1 && errno != EAGAIN) die("read: Error in reading key from STDIN");
    }

    if (c == ESCAPE_CODE) {
        char seq[3]; // array for sequance keys

        if (read(STDIN_FILENO, &seq[0], 1) != 1) return ESCAPE_CODE; // read first byte of seq
        if (read(STDIN_FILENO, &seq[1], 1) != 1) return ESCAPE_CODE; // read second byte of seq

        // in different systems sequances of HOME and END can be different

        if (seq[0] == '[') { // '[' after escape code means it's an escape sequance
            // sequnce here is kind of \x1b[ 

            if (isdigit(seq[1])) {
                // sequance here is kind of \x1b[(number)

                if (read(STDIN_FILENO, &seq[2], 1) != 1) return ESCAPE_CODE; // read third byte of seq

                if (seq[2] == '~') {
                    // sequance here is kind of \x1b[(number)~
                    switch (seq[1]) {
                        case '1': return HOME_KEY;  // \x1b[1~
                        case '3': return DEL_KEY;   // \x1b[3~
                        case '4': return END_KEY;   // \x1b[4~
                        case '5': return PAGE_UP;   // \x1b[5~
                        case '6': return PAGE_DOWN; // \x1b[6~
                        case '7': return HOME_KEY;  // \x1b[7~
                        case '8': return END_KEY;   // \x1b[8~
                    }
                }
            }

            switch (seq[1]) {
                case 'A': return ARROW_UP;    // \x1b[A~
                case 'B': return ARROW_DOWN;  // \x1b[B~
                case 'C': return ARROW_RIGHT; // \x1b[C~
                case 'D': return ARROW_LEFT;  // \x1b[D~
                case 'H': return HOME_KEY;    // \x1b[H~
                case 'F': return END_KEY;     // \x1b[F~
            }
        } else if (seq[0] == 'O') { // In some system is also can be O
            switch (seq[1])
            {
                case 'H': return HOME_KEY; // \x1bOH
                case 'F': return END_KEY;  // \x1bOF
            }
        }
        return ESCAPE_CODE;
    }

    return c;
}


int getCursorPosition(int *rows, int *cols)
{
    char curpos[32]; // string for cursor position

    unsigned int i = 0;

    // ask shell for cursor position, it will be printed to stdout
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
