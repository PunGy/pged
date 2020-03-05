/** includes **/

#define _DEFAULT_SOURCE
#define _GNU_SOURCE
#define _BSD_SOURCE

#include <string.h>
#include <stdlib.h>

#include "pged.h"
#include "editor_core.h"
#include "user_io.h"
#include "text_operations.h"
#include "file.h"

/** find **/

void editorFind()
{
    char *query = editorPrompt("Search: %s (ESC to cancel)");
    if (query == NULL) return;

    int i;
    for (i = 0; i < E.numrows; i++) {
        erow *row = &E.rows[i];
        char *match = strstr(row->render, query);
        if (match) {
            E.cy = i;
            E.cx = match - row->render;
            E.rowoff = E.numrows;
            break;
        }
    }

    free(query);
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
    E.dirty = 0;
    E.rows = NULL;
    E.filename = NULL;
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
        editorOpen(argv[1]);
    }

    editorSetStatusMessage("HELP: Ctrl-S = save | Ctrl-Q = quit");

    while (1) {
        editorRefreshScreen();
        editorProcessKeypress();
    }
    return 0;
}
