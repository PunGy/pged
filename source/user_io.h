#ifndef USER_IO_H
#define USER_IO_H

/* Output buffer */

/**
 * Buffer for storing characters, which will append to user output
 * OB - output buffer
 */
struct obuf {
    char *b;
    int len;
};
#define OBUF_INIT {NULL, 0}

// Append string to output buffer
void obAppend(struct obuf *ob, const char *str, int len);

// Clear output buffer
void obFree(struct obuf *ob);

// Ask user and return his answer
char *editorPrompt(char *prompt);

// Process moving cursor and change E.cx/E.cy depend on input key
void editorMoveCursor(int key);

// Process user input
void editorProcessKeypress(void);

// Set offsetting from current E.cx/E.cy
void editorScroll(void);

// Write rows from E.rows( with offest ), and append it to output buffer
void editorDrawRows(struct obuf *ob);

// Fill status bar
void editorDrawStatusBar(struct obuf *ob);

// Fill message bar
void editorDrawMessageBar(struct obuf *ob);

// Draw updates on screen
void editorRefreshScreen(void);

// Set formated status message
void editorSetStatusMessage(const char *fmt, ...);

#endif
