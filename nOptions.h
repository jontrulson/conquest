/*
 * Copyright Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 */

#ifndef _NOPTIONS_H
#define _NOPTIONS_H

#define NOPT_USER    0          /* user opts */
#define NOPT_SYS     1          /* sysopts */

scrNode_t *nOptionsInit(int what, int setmode, int rnode);

#endif /* _NOPTIONS_H */
