/*
 * prompt utils
 *
 * Copyright Jon Trulson under the MIT License. (See LICENSE).
 */

#include "c_defs.h"
#include "global.h"

#include "color.h"
#include "gldisplay.h"
#include "glmisc.h"
#include "prm.h"

int prmProcInput(prm_t *prm, int ch)
{
    char c = (ch & 0xff);         /* 8bit equiv */
    int clen = strlen(prm->buf);

    if (strchr(prm->terms, ch ))
        return ch;                  /* you're terminated */

    if ((clen >= (prm->buflen - 1)) && isprint(c))
        return PRM_MAXLEN;           /* buf is full */

    /* check for preinit */
    if (prm->preinit && ch != TERM_NORMAL && ch != TERM_EXTRA && isprint(c))
    {
        prm->buf[0] = c;
        prm->buf[1] = 0;
        prm->preinit = FALSE;

        return PRM_OK;
    }

    /* editing */
    if ( ch == '\b' || ch == 0x7f )
    {
        if ( clen > 0 )
        {
            clen--;
            prm->buf[clen] = 0;

            return PRM_OK;
        }
    }
    else if ( ch == 0x17 )	/* ^W */
    {
        /* Delete the last word. */
        if ( clen > 0 )
        {
            /* Back up over blanks. */
            while ( clen >= 0 )
                if ( prm->buf[clen] == ' ' )
                    clen--;
                else
                    break;

            /* Back up over non-blanks. */
            while ( clen >= 0 )
                if ( prm->buf[clen] == ' ' )
                    break;
                else
                    clen--;

            if (clen < 0 )
            {
                clen = 0;
            }

            prm->buf[clen] = 0;
        }
    }
    else if ( ch == 0x15 || ch == 0x18 ) /* ^U || ^X  - clear line */
    {
        if ( clen > 0 )
        {
            clen = 0;
            prm->buf[clen] = 0;
        }
    }
    else if (!isprint(c))
        mglBeep(MGL_BEEP_ERR);
    else
    {
        prm->buf[clen] = c;
        prm->buf[clen + 1] = 0;
    }

    return PRM_OK;
}
