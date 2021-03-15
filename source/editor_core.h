#ifndef EDITOR_CORE_H
#define EDITOR_CORE_H

enum editorKey {
    BACKSPACE = 127,
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

/**
 * All escape sequances starts with this escape code
 */
#define ESCAPE_CODE '\x1b'

#define CTRL_KEY(k) ((k) & 0x1f)
/**
 * Erase all text on display (means just visual output, no data affected)
 * Size: 4 byte
 */
#define C_ERASE_DISPLAY "\x1b[2J"
/**
 * Set cursor position to start
 * Size: 3 byte
 */
#define C_START_CURSOR_POS "\x1b[H"

// Erase screen and show error
void die(const char *error);

// Return previos terminal mode
void disableRawMode(void);

// Save current mode to cache, and enable raw mode
void enableRawMode(void);

// Get user input
int editorReadKey(void);

// Write current cursor position to rows and cols args
int getCursorPosition(int *rows, int *cols);

// Write size of window to rows and cols args
int getWindowSize(int *rows, int *cols);

#endif
