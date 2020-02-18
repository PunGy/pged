/** includes **/

#define _DEFAULT_SOURCE
#define _GNU_SOURCE
#define _BSD_SOURCE

#include <ctype.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

/** defines **/

#define LTED_VERSION "0.0.1"
#define TAB_SIZE 4
#define CTRL_KEY(k) ((k) & 0x1f)

// 4 byte size
#define C_ERASE_DISPLAY "\x1b[2J"
// 3 byte size
#define C_START_CURSOR_POS "\x1b[H"

enum editorKey {
    ARROW_LEFT = 1000,
    ARROW_RIGHT,
    ARROW_UP,
    ARROW_DOWN,
    DEL_KEY,
    HOME_KEY,
    END_KEY,
    PAGE_UP,
    PAGE_DOWN,
};

/** data **/

typedef struct erow {
    int size;
    int rsize;
    char *chars;
    char *render;
} erow;
struct editorConfig {
    int cx, cy;
    int rx;
    int coloff; // column offset
    int rowoff; // row offset
    int screenrows; // count of screen rows
    int screencols; // count of screen cols
    int numrows; // count of file rows
    erow *frows; // array of file rows
    char *filename;
    struct termios orig_termios;
};

struct editorConfig E;

/** terminal **/

void die(const char *s)
{
    write(STDOUT_FILENO, C_ERASE_DISPLAY, 4);
    write(STDOUT_FILENO, C_START_CURSOR_POS, 3);
    perror(s);
    exit(1);
}
void disableRawMode(void)
{
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.orig_termios) == -1)
        die("tcsetattr");
}
void enableRawMode(void)
{
    if (tcgetattr(STDIN_FILENO, &E.orig_termios) == -1) die("tcgetattr");
    atexit(disableRawMode);

    struct termios raw = E.orig_termios;

    raw.c_iflag &= ~(BRKINT | INPCK | ISTRIP | ICRNL | IXON);
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag &= ~(CS8);
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) die("tcsetattr");
}
int editorReadKey(void)
{
    int nread; // number of bytes read
    char c;
    while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
        if (nread == -1 && errno != EAGAIN) die("read");
    }

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

/** row operations **/

int edtiorRowCxToRx(erow *row, int cx)
{
    int rx = 0;
    for (int i = 0; i < cx; i++) {
        if (row->chars[i] == '\t')
            rx += (TAB_SIZE - 1) - (rx % TAB_SIZE);
        rx++;
    }
    return rx;
} 

// function for updating row via rendering special symbols
void editorUpdateRow(erow *row)
{
    int tabs = 0;
    int rowIdx; // current position of row
    int renderIdx = 0; // current position of render string
    for (rowIdx = 0; rowIdx < row->size; rowIdx++)
        if (row->chars[rowIdx] == '\t') tabs++;
    
    free(row->render);
    row->render = malloc(row->size + tabs * (TAB_SIZE - 1) + 1);

    for (rowIdx = 0; rowIdx < row->size; rowIdx++) {
        if (row->chars[rowIdx] == '\t') {
            row->render[renderIdx++] = ' '; // add first space, because if we will start from zero, 0 % 0 != 0 will be false
            while (renderIdx % TAB_SIZE != 0) row->render[renderIdx++] = ' ';
        } else {
            row->render[renderIdx++] = row->chars[rowIdx];
        }        
    }
    row->render[renderIdx] = '\0';
    row->rsize = renderIdx;
}
void editorAppendRow(char *str, size_t len)
{
    E.frows = realloc(E.frows, sizeof(erow) * (E.numrows + 1));

    int at = E.numrows;
    E.frows[at].size = len;
    E.frows[at].chars = malloc(len + 1);
    memcpy(E.frows[at].chars, str, len);
    E.frows[at].chars[len] = '\0';

    E.frows[at].rsize = 0;
    E.frows[at].render = NULL;
    editorUpdateRow(&E.frows[at]);

    E.numrows++;
}

/** file i/o **/

void editorOpen(char *filename)
{
    free(E.filename);
    E.filename = strdup(filename);

    FILE *fp = fopen(filename, "r");
    if (!fp) die("fopen");

    char *line = NULL;
    size_t linecap = 0;
    ssize_t linelen;
    while ((linelen = getline(&line, &linecap, fp)) != -1) {
        while (linelen > 0 && (line[linelen - 1] == '\n' ||
                               line[linelen - 1] == '\r'))
            linelen--;
        editorAppendRow(line, linelen);
    }
    free(line);
    fclose(fp);
}

/** append buffer **/

struct abuf {
    char *b;
    int len;
};

#define ABUF_INIT {NULL, 0}

void abAppend(struct abuf *ab, const char *s, int len)
{
    char *new = realloc(ab->b, ab->len + len);

    if (new == NULL) return;
    memcpy(&new[ab->len], s, len);
    ab->b = new;
    ab->len += len;
}
void abFree(struct abuf *ab)
{
    free(ab->b);
}


/** input **/

void editorMoveCursor(int key)
{
    erow *row = (E.cy >= E.numrows) ? NULL : &E.frows[E.cy];

    switch (key)
    {
        case ARROW_LEFT:
            if (E.cx != 0) {
                E.cx--;
            } else if (E.cy > 0) {
                E.cy--;
                E.cx = E.frows[E.cy].size;
            }
            break;
        case ARROW_RIGHT:
            if (row && E.cx < row->size) {
                E.cx++;
            } else if (row && E.cx == row->size) {
                E.cy++;
                E.cx = 0;
            }
            break;
        case ARROW_UP:
            if (E.cy != 0)
                E.cy--;
            break;
        case ARROW_DOWN:
            if (E.cy < E.numrows)
                E.cy++;
            break;
    }

    row = (E.cy >= E.numrows) ? NULL : &E.frows[E.cy];
    int rowlen = row ? row->size : 0;
    if (E.cx > rowlen) {
        E.cx = rowlen;
    }
}
void editorProcessKeypress(void)
{
    int c = editorReadKey();

    switch (c) {
        case CTRL_KEY('q'):
            write(STDOUT_FILENO, C_ERASE_DISPLAY, 4);
            write(STDOUT_FILENO, C_START_CURSOR_POS, 3);
            exit(0);
            break;

        case PAGE_UP:
        case PAGE_DOWN:
            {
                if (c == PAGE_UP) {
                    E.cy = E.rowoff;
                } else if (c == PAGE_DOWN) {
                    E.cy = E.rowoff + E.screenrows - 1;
                    if (E.cy > E.numrows) E.cy = E.numrows;
                }
            }
            break;
        case HOME_KEY:
            E.cx = 0;
            break;
        case END_KEY:
            if (E.cy < E.numrows)
                E.cx = E.frows[E.cy].size;
            break;

        case ARROW_UP:
        case ARROW_DOWN:
        case ARROW_LEFT:
        case ARROW_RIGHT:
            editorMoveCursor(c);
            break;
    }
}

/** output **/

void editorScroll(void)
{
    E.rx = E.cx;
    if (E.cy < E.numrows)
        E.rx = edtiorRowCxToRx(&E.frows[E.cy], E.cx);

    if (E.cy < E.rowoff) {
        E.rowoff = E.cy;
    }
    if (E.cy >= E.rowoff + E.screenrows) {
        E.rowoff = E.cy - E.screenrows + 1;
    }
    if (E.rx < E.coloff) {
        E.coloff = E.rx;
    }
    if (E.rx >= E.coloff + E.screencols) {
        E.coloff = E.rx - E.screencols + 1;
    }
}
void editorDrawRows(struct abuf *ab)
{
    int rowi; // index of current row
    for (rowi = 0; rowi < E.screenrows; rowi++) {
        int presrow = rowi + E.rowoff; // offsetted row
        if (presrow >= E.numrows) {
            if (E.numrows == 0 && rowi == E.screenrows - 1) {
                char welcome[80];

                int welcomelen = snprintf(welcome, sizeof(welcome),
                    "LTED -- version %s", LTED_VERSION);
                if (welcomelen > E.screencols) welcomelen = E.screencols;

                int padding = (E.screencols - welcomelen) / 2;
                if (padding) {
                    abAppend(ab, "~", 1);
                    padding--;
                }
                while (padding--) abAppend(ab, " ", 1);

                abAppend(ab, welcome, welcomelen);
            } else {
                abAppend(ab, "~", 1);
            }
        } else {
            int len = E.frows[presrow].rsize - E.coloff;
            if (len < 0) len = 0;
            if (len > E.screencols) len = E.screencols;
            abAppend(ab, &E.frows[presrow].render[E.coloff], len);
        }

        abAppend(ab, "\x1b[K", 3); // erase text from active cursor position to end of line
        abAppend(ab, "\r\n", 2);
    }
}
void editorDrawStatusBar(struct abuf *ab)
{
    abAppend(ab, "\x1b[7m", 4); // reverse color( from black to white, for example )
    
    char leftStatus[80],
         rightStatus[80];
    int leftLen = snprintf(leftStatus, sizeof(leftStatus), "%.20s - %d lines",
        E.filename ? E.filename : "[No Name]", E.numrows);

    int rightLen = snprintf(rightStatus, sizeof(rightStatus), "%d/%d",
        E.cy + 1, E.numrows);

    if (leftLen > E.screencols) leftLen = E.screencols;
    abAppend(ab, leftStatus, leftLen);

    while (leftLen < E.screencols) {
        if (E.screencols - leftLen == rightLen) {
            abAppend(ab, rightStatus, rightLen);
            break;
        }
        abAppend(ab, " ", 1); // fill last row with spaces
        leftLen++;
    }
    
    abAppend(ab, "\x1b[m", 3); // off color changes
}
void editorRefreshScreen(void)
{
    editorScroll();
    struct abuf ab = ABUF_INIT;

    abAppend(&ab, "\x1b[?25l", 6); // make the cursor invisible
    abAppend(&ab, C_START_CURSOR_POS, 3);

    editorDrawRows(&ab);
    editorDrawStatusBar(&ab);

    char buf[32];
    // move cursor position to cy and cx
    snprintf(buf, sizeof(buf), "\x1b[%d;%dH", (E.cy - E.rowoff) + 1, 
                                              (E.rx - E.coloff) + 1);
    abAppend(&ab, buf, strlen(buf));

    abAppend(&ab, "\x1b[?25h", 6); // make the cursor visible

    write(STDOUT_FILENO, ab.b, ab.len);
    abFree(&ab);
}

/** init **/

void initEditor(void)
{
    E.cx = 0;
    E.cy = 0;
    E.rx = 0;
    E.rowoff = 0;
    E.coloff = 0;
    E.numrows = 0;
    E.frows = NULL;
    E.filename = NULL;

    if (getWindowSize(&E.screenrows, &E.screencols) == -1) die("getWindowSize");
    E.screenrows -= 1;
}

int main(int argc, char *argv[])
{
    enableRawMode();
    initEditor();
    if (argc >= 2) {
        editorOpen(argv[1]);
    }

    while (1) {
        editorRefreshScreen();
        editorProcessKeypress();
    }
    return 0;
}