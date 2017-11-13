#include "c_defs.h"

/************************************************************************
 *
 * meta server routines
 *
 * Copyright Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 ***********************************************************************/

#include "global.h"
#include "conqnet.h"
#include "conf.h"
#include "meta.h"
#include "conqcom.h"
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
        return FALSE;

    if ((tbuf = strdup(buf)) == NULL)
        return FALSE;

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
            strncpy(srec->altaddr, chs, CONF_SERVER_NAME_SZ - 1);

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
            strncpy(srec->servername, chs, CONF_SERVER_NAME_SZ - 1);

            chs = ch + 1;
            fieldno++;
            break;

        case 4:                   /* server version */
            *ch = 0;
            strncpy(srec->serverver, chs, CONF_SERVER_NAME_SZ - 1);

            chs = ch + 1;
            fieldno++;
            break;


        case 5:                   /* motd */
            *ch = 0;
            strncpy(srec->motd, chs, CONF_SERVER_MOTD_SZ - 1);

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
            strncpy(srec->contact, chs, META_GEN_STRSIZE - 1);

            chs = ch + 1;
            fieldno++;
            break;

        case 13:                  /* server localtime */
            *ch = 0;
            strncpy(srec->walltime, chs, META_GEN_STRSIZE - 1);

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
            return FALSE;             /* something went wrong */
        break;

    case 2:
        if (fieldno != 14)
            return FALSE;             /* something went wrong */
        break;

    default:
        return FALSE;
    }

    return TRUE;
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
int metaUpdateServer(char *remotehost, char *name, int port)
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
    char tmbuf[META_GEN_STRSIZE];

    if (!remotehost)
        return FALSE;

    if (!name)
        strcpy(myname, "");
    else
        strncpy(myname, name, CONF_SERVER_NAME_SZ);
    myname[CONF_SERVER_NAME_SZ - 1] = 0;

    memset((void *)&sRec, 0, sizeof(metaSRec_t));

    /* count ships */
    for ( i = 0; i < MAXSHIPS; i++ )
    {
        if ( Ships[i].status == SS_LIVE )
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
    sRec.numtotal = MAXSHIPS;
    sRec.flags = getServerFlags();
    sRec.port = port;

    strncpy(sRec.altaddr, myname, CONF_SERVER_NAME_SZ);
    pipe2ul(sRec.altaddr);
    strncpy(sRec.servername, SysConf.ServerName, CONF_SERVER_NAME_SZ);
    pipe2ul(sRec.servername);
    strncpy(sRec.serverver, ConquestVersion, CONF_SERVER_NAME_SZ);
    strcat((char *)sRec.serverver, " ");
    strncat((char *)sRec.serverver, ConquestDate,
            (CONF_SERVER_NAME_SZ - strlen(ConquestVersion)) - 2);
    pipe2ul(sRec.serverver);
    strncpy(sRec.motd, SysConf.ServerMotd, CONF_SERVER_MOTD_SZ);
    pipe2ul(sRec.motd);

    /* meta ver 0x0002+ */
    sRec.protovers = PROTOCOL_VERSION;

    strncpy(sRec.contact, SysConf.ServerContact, META_GEN_STRSIZE - 1);
    pipe2ul(sRec.altaddr);

    thetm = localtime(&thetimet);
    snprintf(tmbuf, META_GEN_STRSIZE - 1, "%s", asctime(thetm));
    i = strlen(tmbuf);
    if (i > 0)
        tmbuf[i - 1] = 0;
    strncpy(sRec.walltime, tmbuf, META_GEN_STRSIZE - 1);

    /* all loaded up, convert it and send it off */
    metaServerRec2Buffer(msg, &sRec);

    if ((hp = gethostbyname(remotehost)) == NULL)
    {
        utLog("metaUpdateServer: %s: no such host", remotehost);
        return FALSE;
    }

    /* put host's address and address type into socket structure */
    memcpy((char *)&sa.sin_addr, (char *)hp->h_addr, hp->h_length);

    sa.sin_family = hp->h_addrtype;

    sa.sin_port = htons(META_DFLT_PORT);

    if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP )) < 0)
    {
        utLog("metaUpdateServer: socket failed: %s", strerror(errno));
        return FALSE;
    }

    if (sendto(s, msg, strlen(msg), 0, (const struct sockaddr *)&sa, sizeof(struct sockaddr_in)) < 0)
    {
        utLog("metaUpdateServer: sento failed: %s", strerror(errno));
        return FALSE;
    }

    close(s);

    return TRUE;
}


/* contact a meta server, and return a pointer to a static array of
   metaSRec_t's coresponding to the server list.  returns number
   of servers found, or -1 if error */
#define SERVER_BUFSIZE 1024

int metaGetServerList(char *remotehost, metaSRec_t **srvlist)
{
    static metaSRec_t servers[META_MAXSERVERS];
    struct sockaddr_in sa;
    struct hostent *hp;
    char buf[SERVER_BUFSIZE];               /* server buffer */
    int off;
    int firsttime = TRUE;
    int s;                        /* socket */
    int nums;                     /* number of servers found */
    char c;

    nums = 0;

    if (!remotehost || !srvlist)
        return -1;

    if (firsttime)
    {
        firsttime = FALSE;
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
