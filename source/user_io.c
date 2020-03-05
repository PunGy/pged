#define _DEFAULT_SOURCE
#define _GNU_SOURCE
#define _BSD_SOURCE

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <ctype.h>
#include <stdio.h>

#include "pged.h"
#include "editor_core.h"
#include "user_io.h"
#include "text_operations.h"
#include "file.h"

/* OB data */


void obAppend(struct obuf *ob, const char *str, int len)
{
    char *new = realloc(ob->b, ob->len + len);

    if (new == NULL) return;
    memcpy(&new[ob->len], str, len);
    ob->b = new;
    ob->len += len;
}


void obFree(struct obuf *ob)
{
    free(ob->b);
}


/** input **/

char *editorPrompt(char *prompt)
{
    size_t bufsize = 128;
    char *buf = malloc(bufsize);

    size_t buflen = 0;
    buf[0] = '\0';

    while (1) {
        editorSetStatusMessage(prompt, buf);
        editorRefreshScreen();

        int c = editorReadKey();
        if (c == DEL_KEY || c == CTRL_KEY('h') || c == BACKSPACE) {
            if (buflen != 0) buf[--buflen] = '\0';
        } else if (c == '\x1b') {
            editorSetStatusMessage("");
            free(buf);
            return NULL;
        } else if (c == '\r') {
            if (buflen != 0) {
                editorSetStatusMessage("");
                return buf;
            }
        } else if (!iscntrl(c) && c < 128) {
            if (buflen == bufsize - 1) {
                bufsize *= 2;
                buf = realloc(buf, bufsize);
            }
            buf[buflen++] = c;
            buf[buflen] = '\0';
        }
    }
}


void editorMoveCursor(int key)
{
    erow *row = (E.cy >= E.numrows) ? NULL : &E.rows[E.cy];

    switch (key)
    {
        case ARROW_LEFT:
            if (E.cx != 0) {
                E.cx--;
            } else if (E.cy > 0) {
                E.cy--;
                E.cx = E.rows[E.cy].size;
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

    row = (E.cy >= E.numrows) ? NULL : &E.rows[E.cy];
    int rowlen = row ? row->size : 0;
    if (E.cx > rowlen) {
        E.cx = rowlen;
    }
}


void editorProcessKeypress(void)
{
    static int quit_times = QUIT_TIMES;

    int c = editorReadKey();

    switch (c) {
        case '\r':
            editorInsertNewLine();
            break;

        case CTRL_KEY('q'):
            if (E.dirty && quit_times > 0) {
                editorSetStatusMessage(
                    "File has unsaved changes. "
                    "Press Ctrl-Q %d more times to quit.", quit_times
                );
                quit_times--;
                return;
            }
            write(STDOUT_FILENO, C_ERASE_DISPLAY, 4);
            write(STDOUT_FILENO, C_START_CURSOR_POS, 3);
            exit(0);
            break;
        case CTRL_KEY('s'):
            editorSave();
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
                E.cx = E.rows[E.cy].size;
            break;
        
        case BACKSPACE:
        case CTRL_KEY('h'):
        case DEL_KEY:
            if (c == DEL_KEY) editorMoveCursor(ARROW_RIGHT);
            editorDelChar();
            break;

        case ARROW_UP:
        case ARROW_DOWN:
        case ARROW_LEFT:
        case ARROW_RIGHT:
            editorMoveCursor(c);
            break;

        case CTRL_KEY('l'):
        case '\x1b':
            /* TODO */
            break;
        
        default:
            editorInsertChar(c);
            break;
    }

    quit_times = QUIT_TIMES;
}

/** output **/

void editorScroll(void)
{
    E.rx = E.cx;
    if (E.cy < E.numrows)
        E.rx = editorRowCxToRx(&E.rows[E.cy], E.cx);

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


void editorDrawRows(struct obuf *ob)
{
    int rowi; // index of current row

    for (rowi = 0; rowi < E.screenrows; rowi++) {
        int presrow = rowi + E.rowoff; // offsetted row

        if (presrow >= E.numrows) {
            // if end of file

            if (E.numrows == 0 && rowi == E.screenrows - 1) {
                // if file is empty
                char welcome[80];

                int welcomelen = snprintf(welcome, sizeof(welcome),
                    "PGED -- version %s", PGED_VERSION);
                if (welcomelen > E.screencols) welcomelen = E.screencols;

                int padding = (E.screencols - welcomelen) / 2;
                if (padding) {
                    obAppend(ob, "~", 1);
                    padding--;
                }

                while (padding--) obAppend(ob, " ", 1);

                obAppend(ob, welcome, welcomelen);
            } else {
                obAppend(ob, "~", 1);
            }

        } else {
            int len = E.rows[presrow].rsize - E.coloff;
            if (len < 0) len = 0;
            if (len > E.screencols) len = E.screencols;
            obAppend(ob, &E.rows[presrow].render[E.coloff], len);
        }

        obAppend(ob, "\x1b[K", 3); // erase text from active cursor position to end of line
        obAppend(ob, "\r\n", 2);
    }
}


void editorDrawStatusBar(struct obuf *ob)
{
    obAppend(ob, "\x1b[7m", 4); // reverse color( from black to white, for example )
    
    char leftStatus[80],
         rightStatus[80];
    int leftLen = snprintf(leftStatus, sizeof(leftStatus), "%.20s - %d lines %s",
        E.filename ? E.filename : "[No Name]", E.numrows,
        E.dirty ? "(modified)" : "");

    int rightLen = snprintf(rightStatus, sizeof(rightStatus), "Ln %d, Col%d",
        E.cy + 1, E.cx + 1);

    if (leftLen > E.screencols) leftLen = E.screencols;
    obAppend(ob, leftStatus, leftLen);

    while (leftLen < E.screencols) {
        if (E.screencols - leftLen == rightLen) {
            obAppend(ob, rightStatus, rightLen);
            break;
        }
        obAppend(ob, " ", 1); // fill last row with spaces
        leftLen++;
    }
    
    obAppend(ob, "\x1b[m", 3); // off color changes
    obAppend(ob, "\r\n", 2);
}


void editorDrawMessageBar(struct obuf *ob)
{
    obAppend(ob, "\x1b[K", 3);
    int msglen = strlen(E.statusmsg);

    if (msglen > E.screencols) msglen = E.screencols;
    if (msglen && time(NULL) - E.statusmsg_time < 5)
        obAppend(ob, E.statusmsg, msglen);
}


void editorRefreshScreen(void)
{
    editorScroll();
    struct obuf ob = OBUF_INIT;

    obAppend(&ob, "\x1b[?25l", 6); // make the cursor invisible
    obAppend(&ob, C_START_CURSOR_POS, 3);

    editorDrawRows(&ob);
    editorDrawStatusBar(&ob);
    editorDrawMessageBar(&ob);

    char buf[32];
    // move cursor position to cy and cx
    snprintf(buf, sizeof(buf), "\x1b[%d;%dH", (E.cy - E.rowoff) + 1, 
                                              (E.rx - E.coloff) + 1);
    obAppend(&ob, buf, strlen(buf));

    obAppend(&ob, "\x1b[?25h", 6); // make the cursor visible

    write(STDOUT_FILENO, ob.b, ob.len);
    obFree(&ob);
}


void editorSetStatusMessage(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(E.statusmsg, sizeof(E.statusmsg), fmt, ap);
    va_end(ap);
    E.statusmsg_time = time(NULL);
}