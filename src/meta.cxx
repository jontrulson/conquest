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

/* convert any pipe chars in a string to underlines */
static void pipe2ul(char *str)
{
    char *p;

    p = str;

    while (p && *p)
    {
        if (*p == '|')
            *p = '_';
        p++;
    }

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
int metaBuffer2ServerRec(metaSRec_t *srec, char *buf)
{
    const int numfields = 14;     /* ver 2 */
    char *tbuf;                   /* copy of buf */
    char *ch, *chs;
    int fieldno;

    if (!buf)
        return false;

    if ((tbuf = strdup(buf)) == NULL)
        return false;

    memset((void *)srec, 0, sizeof(metaSRec_t));

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
            utStrncpy(srec->altaddr, chs, CONF_SERVER_NAME_SZ);

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
            utStrncpy(srec->servername, chs, CONF_SERVER_NAME_SZ);

            chs = ch + 1;
            fieldno++;
            break;

        case 4:                   /* server version */
            *ch = 0;
            utStrncpy(srec->serverver, chs, CONF_SERVER_NAME_SZ);

            chs = ch + 1;
            fieldno++;
            break;


        case 5:                   /* motd */
            *ch = 0;
            utStrncpy(srec->motd, chs, CONF_SERVER_MOTD_SZ);

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
            utStrncpy(srec->contact, chs, META_GEN_STRSIZE);

            chs = ch + 1;
            fieldno++;
            break;

        case 13:                  /* server localtime */
            *ch = 0;
            utStrncpy(srec->walltime, chs, META_GEN_STRSIZE);

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
void metaServerRec2Buffer(char *buf, metaSRec_t *srec)
{

    sprintf(buf, "%u|%s|%d|%s|%s|%s|%d|%d|%d|%d|%u|%u|%s|%s|\n",
            srec->version,
            srec->altaddr,
            srec->port,
            srec->servername,
            srec->serverver,
            srec->motd,
            srec->numtotal,
            srec->numactive,
            srec->numvacant,
            srec->numrobot,
            srec->flags,
            /* meta vers 0x0002+ */
            srec->protovers,
            srec->contact,
            srec->walltime);

    return;
}

/* update the meta server 'remotehost' */
int metaUpdateServer(const char *remotehost, const char *name, int port)
{
    metaSRec_t sRec;
    int s;
    struct sockaddr_in sa;
    struct hostent *hp;
    char msg[BUFFERSZ];
    char myname[CONF_SERVER_NAME_SZ];
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
        strcpy(myname, "");
    else
        utStrncpy(myname, name, CONF_SERVER_NAME_SZ);

    memset((void *)&sRec, 0, sizeof(metaSRec_t));

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

    sRec.version = META_VERSION;
    sRec.numactive = numshipsactive;
    sRec.numvacant = numshipsvacant;
    sRec.numrobot = numshipsrobot;
    sRec.numtotal = cbLimits.maxShips();
    sRec.flags = getServerFlags();
    sRec.port = port;

    utStrncpy(sRec.altaddr, myname, CONF_SERVER_NAME_SZ);
    pipe2ul(sRec.altaddr);
    utStrncpy(sRec.servername, SysConf.ServerName, CONF_SERVER_NAME_SZ);
    pipe2ul(sRec.servername);

    utStrncpy(sRec.serverver, ConquestVersion, CONF_SERVER_NAME_SZ);
    utStrncat((char *)sRec.serverver, " ", CONF_SERVER_NAME_SZ);
    utStrncat((char *)sRec.serverver, ConquestDate,
              CONF_SERVER_NAME_SZ);

    pipe2ul(sRec.serverver);
    utStrncpy(sRec.motd, SysConf.ServerMotd, CONF_SERVER_MOTD_SZ);
    pipe2ul(sRec.motd);

    /* meta ver 0x0002+ */
    sRec.protovers = PROTOCOL_VERSION;

    utStrncpy(sRec.contact, SysConf.ServerContact, META_GEN_STRSIZE);
    pipe2ul(sRec.altaddr);

    thetm = localtime(&thetimet);
    snprintf(sRec.walltime, META_GEN_STRSIZE, "%s", asctime(thetm));
    // remove newline.  Not sure why older servers didn't seem to add
    // one in asctime, but current ones sure do.
    i = strlen(sRec.walltime);
    if (i && sRec.walltime[i - 1] == '\n')
        sRec.walltime[i - 1] = 0;

    /* all loaded up, convert it and send it off */
    metaServerRec2Buffer(msg, &sRec);

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

    if (sendto(s, msg, strlen(msg), 0, (const struct sockaddr *)&sa, sizeof(struct sockaddr_in)) < 0)
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
#define SERVER_BUFSIZE 1024

int metaGetServerList(const char *remotehost, metaSRec_t **srvlist)
{
    static metaSRec_t servers[META_MAXSERVERS];
    struct sockaddr_in sa;
    struct hostent *hp;
    char buf[SERVER_BUFSIZE];               /* server buffer */
    int off;
    static bool firstTime = true;
    int s;                        /* socket */
    int nums;                     /* number of servers found */
    char c;

    nums = 0;

    if (!remotehost || !srvlist)
        return -1;

    if (firstTime)
    {
        firstTime = false;
        memset((void *)&servers, 0, (sizeof(metaSRec_t) * META_MAXSERVERS));
    }

    if ((hp = gethostbyname(remotehost)) == NULL)
    {
        utLog("metaGetServerList: %s: no such host", remotehost);
        return -1;
    }

    /* put host's address and address type into socket structure */
    memcpy((char *)&sa.sin_addr, (char *)hp->h_addr, hp->h_length);

    sa.sin_family = hp->h_addrtype;

    sa.sin_port = htons(META_DFLT_PORT);

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


    off = 0;
#if defined(MINGW)
    while (recv(s, &c, 1, 0) > 0)
#else
        while (read(s, &c, 1) > 0)
#endif
        {
            if (c != '\n' && off < (SERVER_BUFSIZE - 1))
            {
                buf[off++] = c;
            }
            else
            {                       /* we got one */
                buf[off] = 0;

                /* convert to a metaSRec_t */
                if (nums < META_MAXSERVERS)
                {
                    if (metaBuffer2ServerRec(&servers[nums], buf))
                        nums++;
                    else
                        utLog("metaGetServerList: metaBuffer2ServerRec(%s) failed, skipping", buf);
                }
                else
                {
                    utLog("metaGetServerList: num servers exceeds %d, skipping",
                          META_MAXSERVERS);
                }

                off = 0;
            }
        }

    /* done. */
    close(s);

    if (nums)
        *srvlist = servers;
    else
        *srvlist = NULL;

    return nums;
}
