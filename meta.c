#include "c_defs.h"

/************************************************************************
 *
 * meta server routines
 *
 * $Id$
 *
 * Copyright 2004 Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 ***********************************************************************/

#include "global.h"
#include "conqnet.h"
#include "conf.h"
#include "meta.h"
#include "conqcom.h"


/* convert any pipe chars in a string to underlines */
void pipe2ul(char *str)
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
 * vers|addr|port|name|srvrvers|motd|totsh|actsh|vacsh|robsh|flags|
 * 11 fields
 */
int str2srec(metaSRec_t *srec, char *buf)
{
  const int numfields = 11;
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
      case 0:                   /* version - ignored for now */
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
        srec->port = (Unsgn16)atoi(chs);

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

        srec->numtotal = (Unsgn8)atoi(chs);

        chs = ch + 1;
        fieldno++;
        break;

      case 7:                   /* active ships */
        *ch = 0;

        srec->numactive = (Unsgn8)atoi(chs);

        chs = ch + 1;
        fieldno++;
        break;

      case 8:                   /* vacant */
        *ch = 0;

        srec->numvacant = (Unsgn8)atoi(chs);

        chs = ch + 1;
        fieldno++;
        break;

      case 9:                   /* robot */
        *ch = 0;

        srec->numrobot = (Unsgn8)atoi(chs);

        chs = ch + 1;
        fieldno++;

        break;

      case 10:                   /* flags */
        *ch = 0;

        srec->flags = (Unsgn32)atol(chs);

        chs = ch + 1;
        fieldno++;

        break;

      }
  }

  free(tbuf);

  if (fieldno != numfields)
    return FALSE;             /* something went wrong */
  
  return TRUE;
}

/* returns a string in the same format as above. */
void srec2str(char *buf, metaSRec_t *srec)
{

  sprintf(buf, "%u|%s|%d|%s|%s|%s|%d|%d|%d|%d|%u|\n",
          META_VERSION, 
          srec->altaddr,
          srec->port,
          srec->servername,
          srec->serverver,
          srec->motd,
          srec->numtotal,
          srec->numactive,
          srec->numvacant,
          srec->numrobot,
          srec->flags);

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

  if (!remotehost)
    return FALSE;

  if (!name)
    strcpy(myname, "");
  else
    strncpy(myname, name, CONF_SERVER_NAME_SZ);
  myname[CONF_SERVER_NAME_SZ - 1] = 0;

  memset((void *)&sRec, 0, sizeof(metaSRec_t));

  /* count ships */
  for ( i = 1; i <= MAXSHIPS; i++ )
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
  strcat(sRec.serverver, " ");
  strncat(sRec.serverver, ConquestDate,
          (CONF_SERVER_NAME_SZ - strlen(ConquestVersion)) - 2);
  pipe2ul(sRec.serverver);
  strncpy(sRec.motd, SysConf.ServerMotd, CONF_SERVER_MOTD_SZ);
  pipe2ul(sRec.motd);

  /* all loaded up, convert it and send it off */
  srec2str(msg, &sRec);
  
  if ((hp = gethostbyname(remotehost)) == NULL) 
    {
      clog("metaUpdateServer: %s: no such host", remotehost);
      return FALSE;
    }

  /* put host's address and address type into socket structure */
  memcpy((char *)&sa.sin_addr, (char *)hp->h_addr, hp->h_length);

  sa.sin_family = hp->h_addrtype;

  sa.sin_port = htons(META_DFLT_PORT);

  if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP )) < 0) 
    {
      clog("metaUpdateServer: socket failed: %s", strerror(errno));
      return FALSE;
    }

  if (sendto(s, msg, strlen(msg), 0, &sa, sizeof(struct sockaddr_in)) < 0)
    {
      clog("metaUpdateServer: sento failed: %s", strerror(errno));
      return FALSE;
    }

  close(s);

  return TRUE;
}


/* contact a meta server, and return a pointer to a static array of
   metaSRec_t's coresponding to the server list.  returns number
   of servers found, or ERR if error */
int metaGetServerList(char *remotehost, metaSRec_t **srvlist)
{
  static metaSRec_t servers[META_MAXSERVERS];
  struct sockaddr_in sa;
  struct hostent *hp;
  char buf[1024];               /* server buffer */
  int off;
  int firsttime = TRUE;
  int s;                        /* socket */
  int nums;                     /* number of servers found */
  char c;

  nums = 0;

  if (!remotehost || !srvlist)
    return ERR;

  if (firsttime)
    {
      firsttime = FALSE;
      memset((void *)&servers, 0, (sizeof(metaSRec_t) * META_MAXSERVERS));
    }

  if ((hp = gethostbyname(remotehost)) == NULL) 
    {
      clog("metaGetServerList: %s: no such host", remotehost);
      return ERR;
    }

  /* put host's address and address type into socket structure */
  memcpy((char *)&sa.sin_addr, (char *)hp->h_addr, hp->h_length);

  sa.sin_family = hp->h_addrtype;

  sa.sin_port = htons(META_DFLT_PORT);

  if ((s = socket(AF_INET, SOCK_STREAM, 0 )) < 0) 
    {
      clog("metaGetServerList: socket failed: %s", strerror(errno));
      return ERR;
    }

  /* connect to the remote server */
  if ( connect ( s, &sa, sizeof ( sa ) ) < 0 ) 
    {
      clog("metaGetServerList: connect failed: %s", strerror(errno));
      return ERR;
  }


  off = 0;
  while (read(s, &c, 1) > 0)
    {
      if (c != '\n')
        {
          buf[off++] = c;
        }
      else
        {                       /* we got one */
          buf[off] = 0;

          /* convert to a metaSRec_t */
          if (str2srec(&servers[nums], buf))
            nums++;
          else
            clog("metaGetServerList: str2srec(%s) failed, skipping", buf);
          
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

