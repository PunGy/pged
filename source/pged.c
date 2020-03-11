/** includes **/

#define _DEFAULT_SOURCE
#define _GNU_SOURCE
#define _BSD_SOURCE

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>

#include "pged.h"
#include "editor_core.h"
#include "user_io.h"
#include "text_operations.h"
#include "file.h"


void initEditor(void)
{
    E.cx = 0;
    E.cy = 0;
    E.rx = 0;
    E.rowoff = 0;
    E.coloff = 0;
    E.numrows = 0;
    E.dirty = 0;
    E.rows = NULL;
    E.filename = NULL;
    E.file_exist = true; // if file name not provided - it's exits
    E.file_write_rights = true;
    E.statusmsg[0] = '\0';
    E.statusmsg_time = 0;

    if (getWindowSize(&E.screenrows, &E.screencols) == -1) die("getWindowSize: can't get window size");
    E.screenrows -= 2;
}

int main(int argc, char *argv[])
{
    enableRawMode();
    initEditor();
    if (argc >= 2) {
        free(E.filename);
        E.filename = strdup(argv[1]);
        
        if (access(E.filename, F_OK) != -1) { // if file exist
            if (access(E.filename, R_OK) == -1) die("Reading of file is not allowed");
            if (access(E.filename, W_OK) == -1) E.file_write_rights = false;

            editorOpen();
        } else {
            char *folder = strrchr(E.filename, '/');
            if (access(folder, W_OK) == -1) E.file_write_rights = false;
            E.file_exist = false;
        }

    }

    editorSetStatusMessage("HELP: Ctrl-S = save | Ctrl-F = find | Ctrl-Q = quit");

    while (1) {
        editorRefreshScreen();
        editorProcessKeypress();
    }
    return 0;
}
