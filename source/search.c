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
  char *query = editorPrompt("Search: %s (ESC to cancel)", editorFindCallback);
  
  if (query) {
    free(query);
  }
}
