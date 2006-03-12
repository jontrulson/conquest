/* quick hack program to include texdata.h and emit the filenames
   to make it easy to be sure we have all the textures
*/

#include <stdio.h>

typedef struct {
  char *name;
  char *file;
  unsigned int col;
}  cqiTextureInitRec_t;

#include "texdata.h"


int main()
{
  int i;

  for (i=0; i<defaultNumTextures; i++)
    {
      printf("%s\n", defaultTextures[i].file);
    }

  return 0;
}
