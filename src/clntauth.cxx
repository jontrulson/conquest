//
// Author: Jon Trulson <jon@radscan.com>
// Copyright (c) 1994-2018 Jon Trulson
//
// The MIT License
//
// Permission is hereby granted, free of charge, to any person
// obtaining a copy of this software and associated documentation
// files (the "Software"), to deal in the Software without
// restriction, including without limitation the rights to use, copy,
// modify, merge, publish, distribute, sublicense, and/or sell copies
// of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
// BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
// ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//

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
#include "cprintf.h"

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
    char pw[MAX_USERNAME], pwr[MAX_USERNAME], epw[MAX_USERNAME];
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
            TERMS, pw, MAX_USERNAME - 1, false );

    if (isoper == false)
    {
        pwr[0] = 0;
        cdclrl( MSG_LIN1, 2  );
        cdputs("Use any printable characters.", MSG_LIN2, 1);
        cdgetx( "Retype Password: ", MSG_LIN1, 1,
                TERMS, pwr, MAX_USERNAME - 1, false );

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

        utStrncpy(epw, (char *)crypt(pw, salt), MAX_USERNAME);
        utStrncpy(cbUsers[unum].pw, epw, MAX_USERNAME);
    }
    else				/* send a packet */
        sendAuth(cInfo.sock, CPAUTH_CHGPWD, "", pw);

    cdclrl(MSG_LIN1, 2);

    return;
}
