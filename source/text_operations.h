#ifndef TEXT_OPERATIONS_H
#define TEXT_OPERATIONS_H

#include "pged.h"

// Convert rows from E.rows to string and return it
char *editorRowsToString(int *buflen);

// Return render x from plain row and row x
int editorRowCxToRx(erow *row, int cx);

// Return row x axis from render x axis 
int editorRowRxToCx(erow *row, int rx);

// Process row for genering render row to E.render
void editorUpdateRow(erow *row);

// Insert new row to E.rows
void editorInsertRow(int at, char *str, size_t len);

// Free memory of row
void editorFreeRow(erow *row);

// Delete row at( y axis )
void editorDelRow(int at);

// Append string to end of row
void editorRowAppendString(erow *row, char *s, size_t len);

// Insert character to row at( x axis )
void editorRowInsertChar(erow *row, int at, int c);

// Delete character from row at( x axis )
void editorRowDelChar(erow *row, int at);

// Insert new char to row on which cursor located
void editorInsertChar(int c);

// Insert new line below cursor location
void editorInsertNewLine();

// Delete character at cursor position
void editorDelChar();

#endif
