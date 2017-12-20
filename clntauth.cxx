/*
 * clntauth.c - user stuff
 *
 * Copyright Jon Trulson under the MIT License. (See LICENSE).
 */

#include "c_defs.h"
#include "global.h"
#include "conqdef.h"
#include "cb.h"
#include "context.h"
#include "conf.h"
#include "color.h"
#include "ui.h"
#include "cd2lb.h"
#include "cumisc.h"

#include "conqnet.h"
#include "packet.h"
#include "userauth.h"
#include "client.h"
#include "clientlb.h"
#include "conqutil.h"

/* ChangePassword() - change a users password - called from UserOptsMenu() */
void ChangePassword(int unum, int isoper)
{
    static const char *header = "Change Password";
    char pw[MAXUSERNAME], pwr[MAXUSERNAME], epw[MAXUSERNAME];
    char salt[3];
    int lin = 0, col = 0;

    if (isoper == false)
        cdclear();
    else
    {
        cdclrl(MSG_LIN1, 2);
    }

    if (isoper == false)
    {
        lin = 1;
        col = ((Context.maxcol / 2) - (strlen(header) / 2));

        cprintf(lin, col, ALIGN_NONE, "#%d#%s", NoColor, header);
    } /* ! isoper */
    /* get and recheck new passwd */
    pw[0] = 0;
    cdclrl( MSG_LIN1, 2  );
    cdputs("Use any printable characters.", MSG_LIN2, 1);

    cdgetx( "New Password: ", MSG_LIN1, 1,
            TERMS, pw, MAXUSERNAME - 1, false );

    if (isoper == false)
    {
        pwr[0] = 0;
        cdclrl( MSG_LIN1, 2  );
        cdputs("Use any printable characters.", MSG_LIN2, 1);
        cdgetx( "Retype Password: ", MSG_LIN1, 1,
                TERMS, pwr, MAXUSERNAME - 1, false );

        if (strcmp(pw, pwr) != 0)
	{			/* pw's don't match, start over */
            cdbeep();
            cdclrl( MSG_LIN2, 1  );
            uiPutColor(RedLevelColor);
            cdputs("Passwords don't match.", MSG_LIN2, 1);
            uiPutColor(NoColor);
            cdrefresh();
            sleep(2);
            return;
	}
    } /* ! isoper */
    /* ok, we have a new password -
       make it so */

    if (isoper)
    {				/* do it locally */
        salt[0] = (cbUsers[unum].username[0] != 0) ? cbUsers[unum].username[0] : 'J';
        salt[1] = (cbUsers[unum].username[1] != 0) ? cbUsers[unum].username[1] : 'T';
        salt[2] = 0;

        utStrncpy(epw, (char *)crypt(pw, salt), MAXUSERNAME);
        utStrncpy(cbUsers[unum].pw, epw, MAXUSERNAME);
    }
    else				/* send a packet */
        sendAuth(cInfo.sock, CPAUTH_CHGPWD, "", pw);

    cdclrl(MSG_LIN1, 2);

    return;
}
