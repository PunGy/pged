#define _DEFAULT_SOURCE
#define _GNU_SOURCE
#define _BSD_SOURCE

#include <stdlib.h>
#include <string.h>

#include "user_io.h"
#include "pged.h"


/**
 * Convert rows from E.rows to string and return it
 */
char *editorRowsToString(int *buflen)
{
    int totlen = 0;
    int i;
    for (i = 0; i < E.numrows; i++)
        totlen += E.rows[i].size + 1;
    *buflen = totlen;

    char *buf = malloc(totlen);
    char *p = buf;
    for (i = 0; i < E.numrows; i++) {
        memcpy(p, E.rows[i].chars, E.rows[i].size);
        p += E.rows[i].size;
        *p = '\n';
        p++;
    }

    return buf;
}

/**
 * Return render x axis from row x axis
 */
int editorRowCxToRx(erow *row, int cx)
{
    int rx = 0;
    for (int i = 0; i < cx; i++) {
        if (row->chars[i] == '\t')
            rx += (TAB_SIZE - 1) - (rx % TAB_SIZE);
        rx++;
    }
    return rx;
} 

/**
 * Process row for genering render row to E.render
 */
void editorUpdateRow(erow *row)
{
    int tabs = 0;
    int rowIdx; // current position of row
    int renderIdx = 0; // current position of render string
    for (rowIdx = 0; rowIdx < row->size; rowIdx++)
        if (row->chars[rowIdx] == '\t') tabs++;
    
    free(row->render);
    row->render = (char *)malloc(row->size + tabs * (TAB_SIZE - 1) + 1);
    if (tabs == 0) {
        memcpy(row->render, row->chars, row->size);
        row->rsize = row->size;
        row->render[row->rsize] = '\0';
        return;
    }

    for (rowIdx = 0; rowIdx < row->size; rowIdx++) {
        if (row->chars[rowIdx] == '\t') {
            // add first space, because if we will start from zero, 0 % 0 != 0 will be false
            row->render[renderIdx++] = ' ';

            while (renderIdx % TAB_SIZE != 0) row->render[renderIdx++] = ' ';
        } else {
            row->render[renderIdx++] = row->chars[rowIdx];
        }        
    }
    row->render[renderIdx] = '\0';
    row->rsize = renderIdx;
}

/**
 * Insert new row to E.rows 
 */
void editorInsertRow(int at, char *str, size_t len)
{
    if (at < 0 || at > E.numrows) return;

    // Allocate memory for one more row
    E.rows = realloc(E.rows, sizeof(erow) * (E.numrows + 1));
    
    // Make room for new row, if it is'nt a last row
    if (E.numrows != at) {
        memmove(&E.rows[at + 1], &E.rows[at], sizeof(erow) * (E.numrows - at));
    }

    E.rows[at].size = len;
    E.rows[at].chars = malloc(len + 1);
    memcpy(E.rows[at].chars, str, len);
    E.rows[at].chars[len] = '\0';

    E.rows[at].rsize = 0;
    E.rows[at].render = NULL;
    editorUpdateRow(&E.rows[at]);

    E.numrows++;
    E.dirty++;
}

/**
 * Free memory of row
 */ 
void editorFreeRow(erow *row)
{
    free(row->render);
    free(row->chars);
}

/**
 * Delete row at( x axis )
 */ 
void editorDelRow(int at)
{
    if (at < 0 || at >= E.numrows) return;
    
    editorFreeRow(&E.rows[at]);
    memmove(&E.rows[at], &E.rows[at + 1], sizeof(erow) * (E.numrows - at - 1));

    E.numrows--;
    E.dirty++;
}

/**
 * Append string to end of row
 */
void editorRowAppendString(erow *row, char *s, size_t len)
{
    row->chars = realloc(row->chars, row->size + len + 1);
    memcpy(&row->chars[row->size], s, len);

    row->size += len;
    row->chars[row->size] = '\0';
    
    editorUpdateRow(row);
    E.dirty++;
}

/**
 * Insert character to row at( x axis )
 */
void editorRowInsertChar(erow *row, int at, int c)
{
    if (at < 0 || at > row->size) at = row->size;

    // Allocate memory for new character and null byte
    row->chars = realloc(row->chars, row->size + 2);
    
    /**
     * Displace string located on cursor to right
     * For making place for new character
     */
    memmove(
        &row->chars[at + 1],
        &row->chars[at],
        row->size - at + 1
    );
    row->size++;
    row->chars[at] = c;

    editorUpdateRow(row);
    E.dirty++;
}

/**
 * Delete character from row at( x axis )
 */
void editorRowDelChar(erow *row, int at)
{
    if (at < 0 || at >= row->size) return;

    /**
     * Displace string located at cursor to left
     * For removing current character
     */
    memmove(
        &row->chars[at],
        &row->chars[at + 1],
        row->size - at
    );
    row->size--;
    editorUpdateRow(row);
    E.dirty++;
}

/**
 * Insert new char to row on which cursor located
 */
void editorInsertChar(int c)
{
    if (E.cy == E.numrows) {
        editorInsertRow(E.numrows, "", 0);
    }
    editorRowInsertChar(&E.rows[E.cy], E.cx, c);
    E.cx++;
}

/**
 * Insert new line below cursor location
 */
void editorInsertNewLine()
{
    if (E.cx == 0) {
        editorInsertRow(E.cy, "", 0);
    } else {
        erow *row = &E.rows[E.cy];

        editorInsertRow(
            E.cy + 1,
            &row->chars[E.cx],
            row->size - E.cx
        );
        
        row = &E.rows[E.cy];
        row->size = E.cx;
        row->chars[row->size] = '\0';
        
        editorUpdateRow(row);
    }
    E.cy++;
    E.cx = 0;
}

/*
 * Delete character at cursor position
 */
void editorDelChar()
{
    if (E.cy == E.numrows) return;
    if (E.cx == 0 && E.cy == 0) return;

    erow *row = &E.rows[E.cy];
    if (E.cx > 0) {
        editorRowDelChar(row, E.cx - 1);
        E.cx--;
    } else {
        E.cx = E.rows[E.cy - 1].size;
        editorRowAppendString(&E.rows[E.cy - 1], row->chars, row->size);
        editorDelRow(E.cy);
        E.cy--;
    }
}
