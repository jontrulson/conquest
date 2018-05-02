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

#include "prm.h"

int prmProcInput(prm_t& prm, int ch)
{
    char c = (ch & 0xff);         /* 8bit equiv */

    if (strchr(TERMS, ch ))
        return ch;                  /* you're terminated */

    bool isPrintable = isprint(c);
    if ((prm.buf.size() >= prm.buflen) && isPrintable)
        return PRM_MAXLEN;           /* buf is full */

    /* check for preinit */

    // TODO - detect difference between typing a char on a pre-init,
    // vs. deleting (editing) and typing a char.  Right now, any
    // printable on a preinit prompt clears the whole buffer.
    if (prm.preinit && ch != TERM_NORMAL && ch != TERM_EXTRA && isPrintable)
    {
        prm.buf = c;
        prm.preinit = false;

        return PRM_OK;
    }

    /* editing */
    if ( ch == '\b' || ch == 0x7f )
    {
        if ( !prm.buf.empty() )
        {
            prm.buf.pop_back();

            return PRM_OK;
        }
    }
    else if ( ch == 0x17 )	/* ^W */
    {
        /* Delete the last word. */
        if ( !prm.buf.empty() )
        {
            // delete any trailing spaces...
            while ( !prm.buf.empty() && prm.buf[prm.buf.size() - 1] == ' ')
                prm.buf.pop_back();

            // if the string is empty, we're done
            if (prm.buf.empty())
                return PRM_OK;

            // now look for the last space in the string
            size_t found = prm.buf.rfind(' ');

            if (found == std::string::npos)
            {
                // not found, so clear the whole string
                prm.buf.clear();
            }
            else
            {
                // delete everything at and after that point...
                prm.buf.erase(found);
            }

            return PRM_OK;
        }
    }
    else if ( ch == 0x15 || ch == 0x18 ) /* ^U || ^X  - clear line */
    {
        prm.buf.clear();
    }
    else
    {
        // add the new character if it's a printable...
        if (isPrintable)
            prm.buf += c;
    }

    return PRM_OK;
}
