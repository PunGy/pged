#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "pged.h"
#include "user_io.h"
#include "text_operations.h"

void editorFindCallback(char *query, int key)
{
  if (key == '\r' || key == '\x1b') {
    return;
  }
  
  for (int i = 0; i < E.numrows; i++) {
    erow *row = &E.rows[i];
    char *match = strstr(row->render, query);

    if (match) {
      E.cy = i;
      E.cx = editorRowRxToCx(row, match - row->render);
      E.rowoff = E.numrows;
      break;
    }
  }
}

void editorFind(void)
{
  int saved_cx = E.cx;
  int saved_cy = E.cy;
  int saved_coloff = E.coloff;
  int saved_rowoff = E.rowoff;

  char *query = editorPrompt("Search: %s (ESC to cancel)", editorFindCallback);
  
  if (query) {
    free(query);
  } else {
    E.cx = saved_cx;
    E.cy = saved_cy;
    E.coloff = saved_coloff;
    E.rowoff = saved_rowoff;
  }
}
