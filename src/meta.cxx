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
#include "conqnet.h"
#include "conf.h"
#include "meta.h"
#include "cb.h"
#include "conqutil.h"
#include "protocol.h"

#include <string>
#include <algorithm>
#include "format.h"

/* convert any pipe chars in a string to underlines */
static void pipe2ul(std::string& str)
{
    std::replace(str.begin(), str.end(), '|', '_');
    return;
}

/* format: (recieved from a server)
 *
 * version 0x0001
 * vers|addr|port|name|srvrvers|motd|totsh|actsh|vacsh|robsh|flags|
 * 11 fields
 *
 * version 0x0002
 * vers|addr|port|name|srvrvers|motd|totsh|actsh|vacsh|robsh|flags|
 *     protovers|contact|walltime|
 * 14 fields
 *
 */
int metaBuffer2ServerRec(metaSRec_t *srec, const char *buf)
{
    const int numfields = 14;     /* ver 2 */
    char *tbuf;                   /* copy of buf */
    char *ch, *chs;
    int fieldno;

    if (!buf)
        return false;

    // duplicate it so we can modify it as we parse
    if ((tbuf = strdup(buf)) == NULL)
        return false;

    *srec = {};

    fieldno = 0;
    chs = tbuf;
    while ((ch = strchr(chs, '|')) && fieldno < numfields)
    {
        switch (fieldno)
        {
        case 0:                   /* meta protocol version */
            *ch = 0;
            srec->version = atoi(chs);
            chs = ch + 1;
            fieldno++;
            break;

        case 1:                   /* address if specified */
            *ch = 0;
            srec->altaddr = chs;

            chs = ch + 1;
            fieldno++;
            break;

        case 2:                   /* server port */
            *ch = 0;
            srec->port = (uint16_t)atoi(chs);

            chs = ch + 1;
            fieldno++;
            break;

        case 3:                   /* server name */
            *ch = 0;
            srec->servername = chs;

            chs = ch + 1;
            fieldno++;
            break;

        case 4:                   /* server version */
            *ch = 0;
            srec->serverver = chs;

            chs = ch + 1;
            fieldno++;
            break;


        case 5:                   /* motd */
            *ch = 0;
            srec->motd = chs;

            chs = ch + 1;
            fieldno++;
            break;

        case 6:                   /* total ships */
            *ch = 0;

            srec->numtotal = (uint8_t)atoi(chs);

            chs = ch + 1;
            fieldno++;
            break;

        case 7:                   /* active ships */
            *ch = 0;

            srec->numactive = (uint8_t)atoi(chs);

            chs = ch + 1;
            fieldno++;
            break;

        case 8:                   /* vacant */
            *ch = 0;

            srec->numvacant = (uint8_t)atoi(chs);

            chs = ch + 1;
            fieldno++;
            break;

        case 9:                   /* robot */
            *ch = 0;

            srec->numrobot = (uint8_t)atoi(chs);

            chs = ch + 1;
            fieldno++;

            break;

        case 10:                   /* flags */
            *ch = 0;

            srec->flags = (uint32_t)atol(chs);

            chs = ch + 1;
            fieldno++;

            break;

            /* meta version 0x0002+ */
        case 11:                  /* server protocol version */
            *ch = 0;
            srec->protovers = (uint16_t)atoi(chs);
            chs = ch + 1;
            fieldno++;

            break;

        case 12:                  /* contact (email/http/whatever) */
            *ch = 0;
            srec->contact = chs;

            chs = ch + 1;
            fieldno++;
            break;

        case 13:                  /* server localtime */
            *ch = 0;
            srec->walltime = chs;

            chs = ch + 1;
            fieldno++;
            break;
        }
    }

    free(tbuf);

    switch (srec->version)
    {
    case 1:
        if (fieldno < 11)
            return false;             /* something went wrong */
        break;

    case 2:
        if (fieldno != 14)
            return false;             /* something went wrong */
        break;

    default:
        return false;
    }

    return true;
}

/* returns a string in the same format as above. */
void metaServerRec2Buffer(std::string& buf, const metaSRec_t& srec)
{
    buf = fmt::format("{}|{}|{}|{}|{}|{}|{}|{}|{}|{}|{}|{}|{}|{}|\n",
                      srec.version,
                      srec.altaddr,
                      srec.port,
                      srec.servername,
                      srec.serverver,
                      srec.motd,
                      srec.numtotal,
                      srec.numactive,
                      srec.numvacant,
                      srec.numrobot,
                      srec.flags,
                      /* meta vers 0x0002+ */
                      srec.protovers,
                      srec.contact,
                      srec.walltime);

    return;
}

/* update the meta server 'remotehost' */
int metaUpdateServer(const char *remotehost, const char *name, int port)
{
    metaSRec_t sRec;
    int s;
    struct sockaddr_in sa;
    struct hostent *hp;
    std::string msg;
    std::string myname;
    int i;
    extern char *ConquestVersion, *ConquestDate;
    int numshipsactive = 0;
    int numshipsvacant = 0;
    int numshipsrobot = 0;
    struct tm *thetm;
    time_t thetimet = time(0);

    if (!remotehost)
        return false;

    if (!name)
        myname = remotehost;
    else
        myname = name;

    /* count ships */
    for ( i = 0; i < cbLimits.maxShips(); i++ )
    {
        if ( cbShips[i].status == SS_LIVE )
        {
            if (SROBOT(i))
                numshipsrobot++;
            else
            {
                if (SVACANT(i))
                    numshipsvacant++;
                else
                    numshipsactive++;

            }
        }
    }

    // clear it out
    sRec = {};
    // load it up
    sRec.version = META_VERSION;
    sRec.numactive = numshipsactive;
    sRec.numvacant = numshipsvacant;
    sRec.numrobot = numshipsrobot;
    sRec.numtotal = cbLimits.maxShips();
    sRec.flags = getServerFlags();
    sRec.port = port;

    sRec.altaddr = myname;
    pipe2ul(sRec.altaddr);
    sRec.servername = SysConf.ServerName;
    pipe2ul(sRec.servername);

    sRec.serverver = ConquestVersion;
    sRec.serverver += " ";
    sRec.serverver += ConquestDate;

    pipe2ul(sRec.serverver);
    sRec.motd = SysConf.ServerMotd;
    pipe2ul(sRec.motd);

    /* meta ver 0x0002+ */
    sRec.protovers = PROTOCOL_VERSION;

    sRec.contact = SysConf.ServerContact;
    pipe2ul(sRec.contact);

    thetm = localtime(&thetimet);
    std::string walltime(asctime(thetm));
    // remove newline if present.  Not sure why older servers didn't
    // seem to add one in asctime, but current ones sure do.
    walltime.erase(std::remove(walltime.begin(), walltime.end(), '\n'),
                   walltime.end());
    sRec.walltime = walltime;

    /* all loaded up, convert it and send it off */
    metaServerRec2Buffer(msg, sRec);

    if ((hp = gethostbyname(remotehost)) == NULL)
    {
        utLog("metaUpdateServer: %s: no such host", remotehost);
        return false;
    }

    /* put host's address and address type into socket structure */
    memcpy((char *)&sa.sin_addr, (char *)hp->h_addr, hp->h_length);

    sa.sin_family = hp->h_addrtype;

    sa.sin_port = htons(META_DFLT_PORT);

    if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP )) < 0)
    {
        utLog("metaUpdateServer: socket failed: %s", strerror(errno));
        return false;
    }

    if (sendto(s, msg.c_str(), msg.size(), 0,
               (const struct sockaddr *)&sa, sizeof(struct sockaddr_in)) < 0)
    {
        close(s);
        utLog("metaUpdateServer: sento failed: %s", strerror(errno));
        return false;
    }

    close(s);

    return true;
}


/* contact a meta server, and return a pointer to a static array of
   metaSRec_t's coresponding to the server list.  returns number
   of servers found, or -1 if error */

int metaGetServerList(const char *remotehost, metaServerVec_t& srvlist)
{
    struct sockaddr_in sa;
    struct hostent *hp;

    if (!remotehost)
        return -1;

    if ((hp = gethostbyname(remotehost)) == NULL)
    {
        utLog("metaGetServerList: %s: no such host", remotehost);
        return -1;
    }

    /* put host's address and address type into socket structure */
    memcpy((char *)&sa.sin_addr, (char *)hp->h_addr, hp->h_length);

    sa.sin_family = hp->h_addrtype;

    // set the port!
    sa.sin_port = htons(META_DFLT_PORT);

    int s;                      /* socket */
    if ((s = socket(AF_INET, SOCK_STREAM, 0 )) < 0)
    {
        utLog("metaGetServerList: socket failed: %s", strerror(errno));
        return -1;
    }

    /* connect to the remote server */
    if ( connect ( s, (const  struct sockaddr *)&sa, sizeof ( sa ) ) < 0 )
    {
        close(s);
        utLog("metaGetServerList: connect failed: %s", strerror(errno));
        return -1;
    }


    std::string buf;               /* server buffer */
    buf.clear();
    char c;

    while (recv(s, &c, 1, 0) > 0)
        {
            if (c != '\n' && buf.size() < (META_MAX_PKT_SIZE - 1))
            {
                buf += c;
            }
            else
            {                       /* we got one line */
                /* convert to a metaSRec_t */
                metaSRec_t sRec = {};

                if (srvlist.size() < META_MAXSERVERS)
                {
                    if (metaBuffer2ServerRec(&sRec, buf.c_str()))
                        srvlist.push_back(sRec);
                    else
                        utLog("metaGetServerList: metaBuffer2ServerRec(%s) "
                              "failed, skipping", buf.c_str());
                }
                else
                {
                    utLog("metaGetServerList: num servers exceeds %d, skipping",
                          META_MAXSERVERS);
                }

                // reset for next line
                buf.clear();
            }
        }

    /* done. */
    close(s);

    return srvlist.size();
}
