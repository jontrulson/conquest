#include "c_defs.h"

/************************************************************************
 *
 * client specific stuff
 *
 * $Id$
 *
 * Copyright 2003 Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 ***********************************************************************/

#include "conf.h"
#include "conqnet.h"
#include "protocol.h"
#include "packet.h"

#define CLIENT_NOEXTERN
#include "client.h"
#undef CLIENT_NOEXTERN

#include "conqlb.h"
#include "clientlb.h"
#include "conqcom.h"
#include "context.h"
#include "msg.h"
#include "record.h"
#include "udp.h"
#include "color.h"
#include "ui.h"

/* send an auth packet and wait for a response (if not CHPWD).
   returns the error code from the ack, or error from write/read */
int sendAuth(int sock, Unsgn8 flag, char *login, char *pw)
{
  char buf[PKT_MAXSIZE];
  int rv;
  spAck_t *sack;
  cpAuthenticate_t cauth;
  int sockl[2] = {cInfo.sock, cInfo.usock};

  memset((void *)&cauth, 0, sizeof(cauth));

  cauth.type = CP_AUTHENTICATE;
  cauth.flag = flag;

  if (login)
    strncpy((char *)cauth.login, (char *)login, MAXUSERNAME - 1);
  if (pw)
    strncpy((char *)cauth.pw, (char *)pw, MAXUSERNAME - 1);

#if defined(DEBUG_CLIENTSEND)
  clog("sendAuth: LOGIN = '%s' PW = '%s'\n",
       login, pw);
#endif

  if ((rv = writePacket(PKT_TOSERVER, cInfo.sock, &cauth)) <= 0)
    {
      clog("sendAuth: writePacket returned %d\n", rv);
      return rv;
    }

  /* now look for an SP_ACK, if we're not changeing pw's */

  if (flag == CPAUTH_CHGPWD)
    return PERR_OK;

  rv = waitForPacket(PKT_FROMSERVER, sockl, SP_ACK, buf, PKT_MAXSIZE, 
		     60, NULL);

  if (rv <= 0)			/* error or timeout (0) */
    {
      clog("sendAuth: waitForPacket = %d", rv);
      return -1;
    }

  /* now we should have our ACK... */

  sack = (spAck_t *)buf;

  return sack->code;
}

int sendSetCourse(int sock, Sgn8 lock, real head)
{
  cpSetCourse_t csc;

  memset((void *)&csc, 0, sizeof(cpSetCourse_t));

  csc.type = CP_SETCOURSE;
  csc.lock = lock;
  csc.head = (Unsgn16)htons((Unsgn16)(head * 100.0));

  if (writePacket(PKT_TOSERVER, sock, &csc) <= 0)
    return FALSE;
  else
    return TRUE;
}

int procUser(char *buf)
{
  int i;
  int unum;
  spUser_t *suser = (spUser_t *)buf;

  if (!validPkt(SP_USER, buf))
    return FALSE;

  unum = (int)((Unsgn16)ntohs(suser->unum));

#if defined(DEBUG_CLIENTPROC)
  clog("PROC USER: unum = %d", unum);
#endif

  if (unum < 0 || unum >= MAXUSERS)
    return FALSE;

  if (Context.recmode == RECMODE_ON)
    recordWriteEvent(buf);

  Users[unum].team = suser->team;

  if (suser->flags & SPUSER_FLAGS_LIVE)
    Users[unum].live = TRUE;

  for (i=0; i<NUMPLAYERTEAMS; i++)
    if ((suser->war & (1 << i)))
      Users[unum].war[i] = TRUE;

  Users[unum].rating = (real)((real)((Sgn16)ntohs(suser->rating)) / 100.0);
  Users[unum].lastentry = (time_t)ntohl(suser->lastentry);

  for (i=0; i<OOPT_TOTALOOPTION; i++)
    Users[unum].ooptions[i] = (Sgn32)ntohl(suser->ooptions[i]);

  for (i=0; i<USTAT_TOTALSTATS; i++)
    Users[unum].stats[i] = (Sgn32)ntohl(suser->stats[i]);

  strncpy(Users[unum].username, (char *)suser->username, MAXUSERNAME - 1);
  strncpy(Users[unum].alias, (char *)suser->alias, MAXUSERPNAME - 1);
 
  return TRUE;
}
  
int procShip(char *buf)
{
  int i;
  spShip_t *sship = (spShip_t *)buf;
  int snum;

  if (!validPkt(SP_SHIP, buf))
    return FALSE;

  snum = sship->snum;
  if (snum <= 0 || snum > MAXSHIPS)
    return FALSE;

#if defined(DEBUG_CLIENTPROC)
  clog("PROC SHIP: snum = %d", snum);
#endif
  if (Context.recmode == RECMODE_ON)
    recordWriteEvent(buf);

  Ships[snum].status = sship->status;
  Ships[snum].team = sship->team;
  Ships[snum].unum = ntohs(sship->unum);
  Ships[snum].shiptype = sship->shiptype;
  Ships[snum].towing = sship->towing;
  Ships[snum].towedby = sship->towedby;

  for (i=0; i<NUMPLAYERTEAMS; i++)
    if (sship->war & (1 << i))
      Ships[snum].war[i] = TRUE;
    else
      Ships[snum].war[i] = FALSE;

  for (i=0; i<NUMPLAYERTEAMS; i++)
    if (sship->rwar & (1 << i))
      Ships[snum].rwar[i] = TRUE;
    else
      Ships[snum].rwar[i] = FALSE;

  Ships[snum].killedby = (int)((Sgn16)ntohs(sship->killedby));
  Ships[snum].kills = (real)((real)ntohl(sship->kills) / 10.0);

  for (i=1; i<=NUMPLANETS; i++)
    Ships[snum].srpwar[i] = (int)sship->srpwar[i];

  for (i=0; i<NUMPLAYERTEAMS; i++)
    Ships[snum].scanned[i] = (int)sship->scanned[i];

  sship->alias[MAXUSERPNAME - 1] = 0;
  strncpy(Ships[snum].alias, (char *)sship->alias, MAXUSERPNAME - 1);
  
  return TRUE;
}

int procShipSml(char *buf)
{
  int snum;
  spShipSml_t *sshipsml = (spShipSml_t *)buf;

  if (!validPkt(SP_SHIPSML, buf))
    return FALSE;

  snum = sshipsml->snum;
  
  if (snum <= 0 || snum > MAXSHIPS)
    return FALSE;

#if defined(DEBUG_CLIENTPROC)
  clog("PROC SHIPSML: snum = %d", snum);
#endif

  if (Context.recmode == RECMODE_ON)
    recordWriteEvent(buf);

  /* we need to mask out map since it's always local */
  Ships[snum].flags = ((((Unsgn16)ntohs(sshipsml->flags)) & ~SHIP_F_MAP) | SMAP(snum));

  Ships[snum].action = sshipsml->action;
  Ships[snum].shields = sshipsml->shields;
  Ships[snum].damage = sshipsml->damage;
  Ships[snum].armies = sshipsml->armies;
  Ships[snum].sdfuse = (int)((Sgn16)ntohs(sshipsml->sdfuse));

  Ships[snum].wfuse = (int)sshipsml->wfuse;
  Ships[snum].efuse = (int)sshipsml->efuse;

  Ships[snum].weapalloc = sshipsml->walloc;
  Ships[snum].engalloc = 100 - Ships[snum].weapalloc;

  Ships[snum].pfuse = (int)sshipsml->pfuse;

  Ships[snum].etemp = (real)sshipsml->etemp;
  Ships[snum].wtemp = (real)sshipsml->wtemp;
  Ships[snum].fuel = (real)((Unsgn16)ntohs(sshipsml->fuel));
  Ships[snum].lock = (int)((Sgn16)ntohs(sshipsml->lock));
  Ships[snum].lastphase = (real)((Unsgn16)ntohs(sshipsml->lastphase)) / 100.0;
  Ships[snum].lastblast = (real)((Unsgn16)ntohs(sshipsml->lastblast)) / 100.0;

  return TRUE;
}

int procShipLoc(char *buf)
{
  int snum;
  spShipLoc_t *sshiploc = (spShipLoc_t *)buf;

  if (!validPkt(SP_SHIPLOC, buf))
    return FALSE;

  snum = sshiploc->snum;
  
  if (snum <= 0 || snum > MAXSHIPS)
    return FALSE;

#if defined(DEBUG_CLIENTPROC)
  clog("PROC SHIPLOC: snum = %d", snum);
#endif

  if (Context.recmode == RECMODE_ON)
    recordWriteEvent(buf);

  Ships[snum].head = (real)((real)ntohs(sshiploc->head) / 10.0);
  Ships[snum].warp = (real)((real)sshiploc->warp / 10.0);

  Ships[snum].x = (real)((real)((Sgn32)ntohl(sshiploc->x)) / 1000.0);
  Ships[snum].y = (real)((real)((Sgn32)ntohl(sshiploc->y)) / 1000.0);

  return TRUE;
}

int procPlanet(char *buf)
{
  spPlanet_t *splan = (spPlanet_t *)buf;
  int pnum;

  if (!validPkt(SP_PLANET, buf))
    return FALSE;

  pnum = splan->pnum;
  
  if (pnum <= 0 || pnum > NUMPLANETS)
    return FALSE;

  if (Context.recmode == RECMODE_ON)
    recordWriteEvent(buf);

  Planets[pnum].type = splan->ptype;
  Planets[pnum].team = splan->team;

  splan->name[MAXPLANETNAME - 1] = 0;
  strncpy(Planets[pnum].name, (char *)splan->name, MAXPLANETNAME);

  uiUpdatePlanet(pnum);

  return TRUE;
}

int procPlanetSml(char *buf)
{
  int i;
  spPlanetSml_t *splansml = (spPlanetSml_t *)buf;
  int pnum;

  if (!validPkt(SP_PLANETSML, buf))
    return FALSE;
  
  pnum = splansml->pnum;
  
  if (pnum <= 0 || pnum > NUMPLANETS)
    return FALSE;
  
  if (Context.recmode == RECMODE_ON)
    recordWriteEvent(buf);

  for (i=0; i<NUMPLAYERTEAMS; i++)
    if (splansml->scanned & (1 << i))  
      Planets[pnum].scanned[i] = TRUE;
    else
      Planets[pnum].scanned[i] = FALSE;

  Planets[pnum].uninhabtime = (int)splansml->uninhabtime;

#if 0
  clog("%s: %d scanned = %x", __FUNCTION__, pnum, splansml->scanned);
#endif

  return TRUE;
}

int procPlanetLoc(char *buf)
{
  spPlanetLoc_t *splanloc = (spPlanetLoc_t *)buf;
  int pnum;

  if (!validPkt(SP_PLANETLOC, buf))
    return FALSE;

  pnum = splanloc->pnum;
  
  if (pnum <= 0 || pnum > NUMPLANETS)
    return FALSE;

  if (Context.recmode == RECMODE_ON)
    recordWriteEvent(buf);

  Planets[pnum].armies = (int)((Sgn16)ntohs(splanloc->armies));
  Planets[pnum].x = (real)((real)((Sgn32)ntohl(splanloc->x)) / 1000.0);
  Planets[pnum].y = (real)((real)((Sgn32)ntohl(splanloc->y)) / 1000.0);

  return TRUE;
}

int procPlanetLoc2(char *buf)
{
  spPlanetLoc2_t *splanloc2 = (spPlanetLoc2_t *)buf;
  int pnum;

  if (!validPkt(SP_PLANETLOC2, buf))
    return FALSE;

  pnum = splanloc2->pnum;
  
  if (pnum <= 0 || pnum > NUMPLANETS)
    return FALSE;

  if (Context.recmode == RECMODE_ON)
    recordWriteEvent(buf);

  Planets[pnum].armies = (int)((Sgn16)ntohs(splanloc2->armies));
  Planets[pnum].x = (real)((real)((Sgn32)ntohl(splanloc2->x)) / 1000.0);
  Planets[pnum].y = (real)((real)((Sgn32)ntohl(splanloc2->y)) / 1000.0);
  Planets[pnum].orbang = (real)((real)((Unsgn16)ntohs(splanloc2->orbang)) / 100.0);

  return TRUE;
}

int procPlanetInfo(char *buf)
{
  spPlanetInfo_t *splaninfo = (spPlanetInfo_t *)buf;
  int pnum;
  int primary;

  if (!validPkt(SP_PLANETINFO, buf))
    return FALSE;

  pnum = splaninfo->pnum;
  
  if (pnum <= 0 || pnum > NUMPLANETS)
    return FALSE;

  primary = splaninfo->primary;

  /* Roy fix - 10/17/2005 - let mur data be sent. */
  if (primary < 0 || primary > NUMPLANETS)
    return FALSE;

  /* we will record them if we get them */
  if (Context.recmode == RECMODE_ON)
    recordWriteEvent(buf);

  /* in protocol 6, we 'forgot' planet realness.  To avoid breaking
     protocol again, and allow unpatched clients and/or servers to
     work we check to see if SPPLANETINFO_FLAGS_FVALID is set.  If so,
     _then_ we pay attn to any other flags present. Else we ignore
     them. */
  if (splaninfo->flags & SPPLANETINFO_FLAGS_FVALID)
    {                           /* we have valid flags */

      if (splaninfo->flags & SPPLANETINFO_FLAGS_REAL)
        Planets[pnum].real = TRUE;
      else
        Planets[pnum].real = FALSE;
    }

  Planets[pnum].primary = primary;
  Planets[pnum].orbrad = (real)((real)((Unsgn32)ntohl(splaninfo->orbrad)) / 10.0);
  Planets[pnum].orbvel = (real)((real)((Sgn32)ntohl(splaninfo->orbvel)) / 100.0);

  return TRUE;
}


int procTorp(char *buf)
{
  int snum, tnum;
  spTorp_t *storp = (spTorp_t *)buf;

  if (!validPkt(SP_TORP, buf))
    return FALSE;

  snum = storp->snum;
  tnum = storp->tnum;
  
  if (snum <= 0 || snum > MAXSHIPS)
    return FALSE;

  if (tnum < 0 || tnum >= MAXTORPS)
    return FALSE;

  if (Context.recmode == RECMODE_ON)
    recordWriteEvent(buf);

  Ships[snum].torps[tnum].status = (int)storp->status;

  return TRUE;
}

int procTorpLoc(char *buf)
{
  int snum, tnum, i;
  spTorpLoc_t *storploc = (spTorpLoc_t *)buf;

  if (!validPkt(SP_TORPLOC, buf))
    return FALSE;

  snum = storploc->snum;
  tnum = storploc->tnum;
  
  if (snum <= 0 || snum > MAXSHIPS)
    return FALSE;

  if (tnum < 0 || tnum >= MAXTORPS)
    return FALSE;

  if (Context.recmode == RECMODE_ON)
    recordWriteEvent(buf);

  for (i=0; i<NUMPLAYERTEAMS; i++)
    if (storploc->war & (1 << i))
      Ships[snum].torps[tnum].war[i] = TRUE;
    else 
      Ships[snum].torps[tnum].war[i] = FALSE;
  
  Ships[snum].torps[tnum].x = (real)((real)((Sgn32)ntohl(storploc->x)) / 1000.0);
  Ships[snum].torps[tnum].y = (real)((real)((Sgn32)ntohl(storploc->y)) / 1000.0);

  return TRUE;
}


int procTorpEvent(char *buf)
{
  int snum, tnum, i;
  spTorpEvent_t *storpev = (spTorpEvent_t *)buf;

  if (!validPkt(SP_TORPEVENT, buf))
    return FALSE;

  snum = storpev->snum;
  tnum = storpev->tnum;
  
  if (snum <= 0 || snum > MAXSHIPS)
    return FALSE;

  if (tnum < 0 || tnum >= MAXTORPS)
    return FALSE;

  Ships[snum].torps[tnum].status = (int)storpev->status;

  if (Context.recmode == RECMODE_ON)
    recordWriteEvent(buf);

  for (i=0; i<NUMPLAYERTEAMS; i++)
    if (storpev->war & (1 << i))
      Ships[snum].torps[tnum].war[i] = TRUE;
    else 
      Ships[snum].torps[tnum].war[i] = FALSE;
  
  Ships[snum].torps[tnum].x = 
    (real)((real)((Sgn32)ntohl(storpev->x)) / 1000.0);
  Ships[snum].torps[tnum].y = 
    (real)((real)((Sgn32)ntohl(storpev->y)) / 1000.0);

  Ships[snum].torps[tnum].dx = 
    (real)((real)((Sgn32)ntohl(storpev->dx)) / 1000.0);
  Ships[snum].torps[tnum].dy = 
    (real)((real)((Sgn32)ntohl(storpev->dy)) / 1000.0);

  uiUpdateTorpDir(snum, tnum);

  return TRUE;
}


int procMessage(char *buf)
{
  spMessage_t *smsg = (spMessage_t *)buf;
  
  if (!validPkt(SP_MESSAGE, buf))
    return FALSE;

  smsg->msg[MESSAGE_SIZE - 1] = 0;

  /* terse? */
  if (UserConf.Terse && (smsg->flags & MSG_FLAGS_TERSABLE))
    return TRUE;

  /* robot response/insult? */
  if (UserConf.NoRobotMsgs && (smsg->flags & MSG_FLAGS_ROBOT))
    return TRUE;

  /* intruder alert? */
  if (!UserConf.DoIntrudeAlert && (smsg->flags & MSG_FLAGS_INTRUDER))
    return TRUE;

  /* don't record feedbacks */
  if ((Context.recmode == RECMODE_ON) && !(smsg->flags & MSG_FLAGS_FEEDBACK) )
    recordWriteEvent(buf);

  if (smsg->flags & MSG_FLAGS_FEEDBACK)
    clntDisplayFeedback((char *)smsg->msg);
  else
    clntStoreMessage(smsg);

  return TRUE;
}

int procTeam(char *buf)
{
  int team, i;
  spTeam_t *steam = (spTeam_t *)buf;

  if (!validPkt(SP_TEAM, buf))
    return FALSE;

  team = steam->team;

  if (team < 0 || team >= NUMALLTEAMS)
    return FALSE;

  if (Context.recmode == RECMODE_ON)
    recordWriteEvent(buf);

  Teams[team].homesun = steam->homesun;

  if (steam->flags & SPTEAM_FLAGS_COUPINFO)
    Teams[team].coupinfo = TRUE;
  else 
    Teams[team].coupinfo = FALSE;

  Teams[team].couptime = steam->couptime;

  for (i=0; i<3; i++)
    Teams[team].teamhplanets[i] = steam->teamhplanets[i];

  Teams[team].homeplanet = steam->homeplanet;

  for (i=0; i<MAXTSTATS; i++)
    Teams[team].stats[i] = (int)ntohl(steam->stats[i]);

  strncpy(Teams[team].name, (char *)steam->name, MAXTEAMNAME - 1);

  return TRUE;
}

int procServerStat(char *buf)
{
  spServerStat_t *sstat = (spServerStat_t *)buf;

  if (!validPkt(SP_SERVERSTAT, buf))
    return FALSE;

  sStat = *sstat;               /* client.h - clients keep a copy. */

  /* now fixup the byte order */
  sStat.numusers = ntohs(sStat.numusers);
  sStat.flags = ntohl(sStat.flags);
  sStat.servertime = ntohl(sStat.servertime);

  return TRUE;
}

int procConqInfo(char *buf)
{
  spConqInfo_t *spci = (spConqInfo_t *)buf;

  if (!validPkt(SP_CONQINFO, buf))
    return FALSE;

  strncpy(ConqInfo->conqueror, (char *)spci->conqueror, MAXUSERPNAME);
  ConqInfo->conqueror[MAXUSERPNAME - 1] = 0;
  strncpy(ConqInfo->conqteam, (char *)spci->conqteam, MAXTEAMNAME);
  ConqInfo->conqteam[MAXTEAMNAME - 1] = 0;
  strncpy(ConqInfo->conqtime, (char *)spci->conqtime, DATESIZE);
  ConqInfo->conqtime[DATESIZE - 1] = 0;
  strncpy(ConqInfo->lastwords, (char *)spci->lastwords, MAXLASTWORDS);
  ConqInfo->lastwords[MAXLASTWORDS - 1] = 0;

  return TRUE;
}

int procHistory(char *buf)
{ 
  spHistory_t *hist = (spHistory_t *)buf;
  int hnum;

  if (!validPkt(SP_HISTORY, buf))
    return FALSE;

  hnum = hist->hnum;

  ConqInfo->histptr = hist->histptr;
  History[hnum].histunum = (int)ntohs(hist->unum);

  History[hnum].elapsed = (time_t)ntohl((Unsgn32)hist->elapsed);
  History[hnum].histlog = (time_t)ntohl((Unsgn32)hist->histlog);

  return TRUE;
}

int procDoomsday(char *buf)
{ 
  spDoomsday_t *dd = (spDoomsday_t *)buf;

  if (!validPkt(SP_DOOMSDAY, buf))
    return FALSE;

  if (Context.recmode == RECMODE_ON)
    recordWriteEvent(buf);

  Doomsday->status = dd->status;
  Doomsday->heading =(real)((real)ntohs(dd->heading) / 10.0);
  Doomsday->x = (real)((real)((Sgn32)ntohl(dd->x)) / 1000.0);
  Doomsday->y = (real)((real)((Sgn32)ntohl(dd->y)) / 1000.0);

  return TRUE;
}

/* send a command to server to change name */
int sendSetName(char *name)
{
  cpSetName_t sname;

  if (!name)
    return FALSE;

  memset((void *)&sname, 0, sizeof(cpSetName_t));
  sname.type = CP_SETNAME;
  strncpy((char *)sname.alias, name, MAXUSERPNAME - 1);

  if (writePacket(PKT_TOSERVER, cInfo.sock, &sname) <= 0)
    return FALSE;
  else 
    return TRUE;
}

int sendCommand(Unsgn8 cmd, Unsgn16 detail)
{
  cpCommand_t ccmd;

  memset((void *)&ccmd, 0, sizeof(cpCommand_t));
  ccmd.type = CP_COMMAND;
  ccmd.cmd = cmd;
  ccmd.detail = htons(detail);

  if (cmd == CPCMD_KEEPALIVE && cInfo.usock != -1)
    {
      writePacket(PKT_TOSERVER, cInfo.usock, &ccmd);
      return TRUE;
    }
  else
    {

      if (writePacket(PKT_TOSERVER, cInfo.sock, &ccmd) <= 0)
        return FALSE;
      else 
        return TRUE;
    }
}

int sendFireTorps(int num, real dir)
{
  cpFireTorps_t ftorps;

  memset((void *)&ftorps, 0, sizeof(cpFireTorps_t));

  ftorps.type = CP_FIRETORPS;
  ftorps.num = (Unsgn8)num;
  ftorps.dir = htons((Unsgn16)(dir * 100.0));

  if (writePacket(PKT_TOSERVER, cInfo.sock, &ftorps) <= 0)
    return FALSE;
  else 
    return TRUE;

}

/* send a message */
int sendMessage(int to, char *msg)
{
  cpMessage_t cmsg;

  if (!msg)
    return FALSE;

  memset((void *)&cmsg, 0, sizeof(cpMessage_t));
  cmsg.type = CP_MESSAGE;
  cmsg.to = (Sgn16)htons(to);
  strncpy((char *)cmsg.msg, msg, MESSAGE_SIZE - 1);

  if (writePacket(PKT_TOSERVER, cInfo.sock, &cmsg) <= 0)
    return FALSE;
  else 
    return TRUE;
}

int clientHello(char *clientname)
{
  cpHello_t chello;
  spAckMsg_t *sackmsg;
  char buf[PKT_MAXSIZE];
  int pkttype;
  extern char *ConquestVersion, *ConquestDate;
  int rv;
  struct timeval tv;
  fd_set readfds;
  int sockl[2] = {cInfo.sock, cInfo.usock};

  /* there should be a server hello waiting for us */
  if ((pkttype = readPacket(PKT_FROMSERVER, sockl, 
			    buf, PKT_MAXSIZE, 60)) < 0)
  {
    clog("clientHello: read server hello failed\n");
    return FALSE;
  }

  if (pkttype == 0)
  {
    clog("clientHello: read server hello: timeout.\n");
    return FALSE;
  }

  /* we only get this if there's problem (server denied access, usually) */
  if (pkttype == SP_ACKMSG || pkttype == SP_ACK)
    {
      if (pkttype == SP_ACKMSG)
        {
          sackmsg = (spAckMsg_t *)buf;
          if (sackmsg->txt)
            {
              clog("clientHello: %s '%s'",
                   psev2String(sackmsg->severity),
                   sackmsg->txt);
            }
        }
      return FALSE;
    }

  if (pkttype != SP_HELLO)
  {
    clog("clientHello: read server hello: wrong packet type %d\n", pkttype);
    return FALSE;
  }

  sHello = *(spHello_t *)buf;

  /* fix up byte ordering */
  sHello.protover = (Unsgn16)ntohs(sHello.protover);
  sHello.cmnrev = (Unsgn32)ntohl(sHello.cmnrev);

  sHello.servername[CONF_SERVER_NAME_SZ - 1] = 0;
  sHello.serverver[CONF_SERVER_NAME_SZ - 1] = 0;
  sHello.motd[CONF_SERVER_MOTD_SZ - 1] = 0;

  clog("SERVERID:%s:%s:0x%04hx:%d:0x%02x:%s",
       sHello.servername,
       sHello.serverver,
       sHello.protover,
       sHello.cmnrev,
       sHello.flags,
       sHello.motd);

  if (cInfo.tryUDP)
    {
      if (connect(cInfo.usock, (const struct sockaddr *)&cInfo.servaddr, 
                  sizeof(cInfo.servaddr)) < 0)
        {
          clog("NET: clientHello: udp connect() failed: %s", strerror(errno));
          cInfo.tryUDP = FALSE;
        }
      else
        {
          /* see if this will succeed in setting up a NAT tunnel
             to the server */
          clog("NET: clientHello: send udp to server.");
          write(cInfo.usock, "Open Me", 7);
        }
    }

  /* now send a client hello */
  chello.type = CP_HELLO;
  chello.updates = Context.updsec;
  chello.protover = htons(PROTOCOL_VERSION);
  chello.cmnrev = htonl(COMMONSTAMP);

  strncpy((char *)chello.clientname, clientname, CONF_SERVER_NAME_SZ);
  strncpy((char *)chello.clientver, ConquestVersion, CONF_SERVER_NAME_SZ);

  strcat((char *)chello.clientver, " ");
  strncat((char *)chello.clientver, ConquestDate, 
	  (CONF_SERVER_NAME_SZ - strlen(ConquestVersion)) - 2);

  if (!writePacket(PKT_TOSERVER, cInfo.sock, &chello))
    {
      clog("clientHello: write client hello failed\n");
      return FALSE;
    }

  clog("clientHello: sent hello to server");

  if (cInfo.tryUDP)
    {
      /* see if we get an ack back from the server via udp */
      /* this is kind of weak and probably needs more work.  As it is, the client needs
         to receive a udp from the server within 5 seconds, or UDP will not be used.  If the
         inbound packet gets lost.... Oh well. */
      tv.tv_sec = 5;            /* 5 secs */
      tv.tv_usec = 0;
      FD_ZERO(&readfds);
      FD_SET(cInfo.usock, &readfds);
      if ((rv = select(cInfo.usock+1, &readfds, NULL, NULL, &tv)) <= 0)
        {
          clog("CLIENT: hello: select udp failed: %s", strerror(errno));
          cInfo.tryUDP = FALSE;
        }
      else
        {
          if (rv > 0 && FD_ISSET(cInfo.usock, &readfds))
            {
              if ((rv = udpRecv(cInfo.usock, buf, PKT_MAXSIZE, &cInfo.servaddr)) >= 0 )
                {
                  clog("NET: got (%d) UDP bytes from server, will ACK for server UDP", rv);
                  cInfo.doUDP = TRUE;
                }
            }
        }
    }
  /* now we need a server stat or a Nak */

  if ((pkttype = readPacket(PKT_FROMSERVER, sockl, 
			    buf, PKT_MAXSIZE, 60)) < 0)
  {
    clog("clientHello: read of SP_ACK or SP_SERVERSTAT failed\n");
    return FALSE;
  }

  if (pkttype == SP_ACKMSG || pkttype == SP_ACK)/* we only get this if problem */
    {
      if (pkttype == SP_ACKMSG)
	{
	  sackmsg = (spAckMsg_t *)buf;
	  if (sackmsg->txt)
	    {
	      clog("clientHello: %s '%s'\n", 
		   psev2String(sackmsg->severity), 
		   sackmsg->txt);
	      printf("clientHello: %s '%s'\n", 
		   psev2String(sackmsg->severity), 
		   sackmsg->txt);

	    }
	}
      return FALSE;
    }

  if (pkttype == SP_SERVERSTAT)
    {
      procServerStat(buf);
# if defined(DEBUG_CLIENTPROC)
      clog("clientHello: recv SP_SERVERSTAT: ships = %d, na = %d, nv = %d, nr = %d\n"
           " nu = %d flags = 0x%08x",
	   sStat.numtotal,
	   sStat.numactive,
	   sStat.numvacant,
	   sStat.numrobot,
	   sStat.numusers,
	   sStat.flags);
#endif
    }
  else
    {
      clog("clientHello: pkttype = %d, was waiting for SP_SERVERSTAT", pkttype);
      return FALSE;
    }

  if (cInfo.doUDP)
    sendAck(cInfo.sock, PKT_TOSERVER, PSEV_INFO, PERR_DOUDP, NULL);
  else
    sendAck(cInfo.sock, PKT_TOSERVER, PSEV_INFO, PERR_OK, NULL);

  return TRUE;
}

void processPacket(char *buf)
{
  int pkttype;
  spClientStat_t *scstat;
  spAck_t *sack;
  spAckMsg_t *sackm;
  spFrame_t *frame;

  if (!buf)
    return;

  pkttype = (int)buf[0];

  switch (pkttype)
    {
    case SP_ACK:
      sack = (spAck_t *)buf;
      lastServerError = sack->code;
      break;
    case SP_ACKMSG:
      sackm = (spAckMsg_t *)buf;
      sackm->txt[MESSAGE_SIZE - 1] = 0;
      lastServerError = sackm->code;
      break;
    case SP_SHIP:
      procShip(buf);
      break;
    case SP_SHIPSML:
      procShipSml(buf);
      break;
    case SP_SHIPLOC:
      procShipLoc(buf);
      break;
    case SP_USER:
      procUser(buf);
      break;
    case SP_PLANET:
      procPlanet(buf);
      break;
    case SP_PLANETSML:
      procPlanetSml(buf);
      break;
    case SP_PLANETLOC:
      procPlanetLoc(buf);
      break;
    case SP_PLANETLOC2:
      procPlanetLoc2(buf);
      break;
    case SP_PLANETINFO:
      procPlanetInfo(buf);
      break;
    case SP_TORP:
      procTorp(buf);
      break;
    case SP_TORPLOC:
      procTorpLoc(buf);
      break;

    case SP_TORPEVENT:
      procTorpEvent(buf);
      break;

    case SP_TEAM:
      procTeam(buf);
      break;

    case SP_CLIENTSTAT:
      if ((scstat = chkClientStat(buf)))
        {
          Context.snum = scstat->snum;
          Context.unum = scstat->unum;
          Ships[Context.snum].team = scstat->team;
          clientFlags = scstat->flags;
        }
      break;

    case SP_MESSAGE:
      procMessage(buf);
      break;
      
    case SP_SERVERSTAT:
      procServerStat(buf);
      break;

    case SP_CONQINFO:
      procConqInfo(buf);
      break;

    case SP_HISTORY:
      procHistory(buf);
      break;

    case SP_DOOMSDAY:
      procDoomsday(buf);
      break;

    case SP_FRAME:              /* playback */
      frame = (spFrame_t *)buf;
      /* endian correction*/
      frame->time = (Unsgn32)ntohl(frame->time);
      frame->frame = (Unsgn32)ntohl(frame->frame);

      if (startTime == (time_t)0)
        startTime = (time_t)frame->time;
      currTime = (time_t)frame->time;

      frameCount = (Unsgn32)frame->frame;

      break;

    default:
      clog("conquest:processPacket: got unexpected packet type %d",
	   pkttype);
      break;
    }

  return;
}

/* Some routers/firewalls seem to disconnect a UDP conncetion if there haven't
 * been any outgoing (client->server) packets for awhile.  Normally, conquest
 * never sends UDP packets except during UDP negotiation at the start of a
 * game.  This routine will send a CPCMD_KEEPALIVE packet every 60 seconds
 * if a UDP connection is enabled.  Seems to solve clute's problem.
 */
void sendUDPKeepAlive(Unsgn32 timebase)
{
  static Unsgn32 katime = 0;     /* UDP keepalive packets */
  static const Unsgn32 kawait = 30000;  /* ms (30 seconds) */
  Unsgn32 iternow;

  if (!cInfo.doUDP)
    return;                     /* no point */

  if (timebase)                 /* don't query the clock */
    iternow = timebase;
  else
    iternow = clbGetMillis();  

  /* send a UDP keepalive packet if it's time */
  if (((iternow - katime) > kawait) && cInfo.usock != -1)
    {
#if 0
      clog("%s: Sending CPCMD_KEEPALIVE\n", __FUNCTION__);
#endif
      sendCommand(CPCMD_KEEPALIVE, 0);
      katime = iternow;
    }

  return;
}

/* this function accepts a character buffer representing a clientstat packet
   and validates it.  It return a pointer to a static spClientStat_t
   packet if everything is in order, NULL otherwise. */
spClientStat_t *chkClientStat(char *buf)
{
  static spClientStat_t scstat;

  if (!buf)
    return NULL;

  scstat = *(spClientStat_t *)buf;
  
  scstat.unum = (Unsgn16)ntohs(scstat.unum);

  if (scstat.unum >= MAXUSERS)
    {
#if defined(DEBUG_PKT)
      clog("%s: unum not in valid range", __FUNCTION__);
#endif
      return NULL;
    }

  if (scstat.snum < 1 || scstat.snum > MAXSHIPS)
    {
#if defined(DEBUG_PKT)
      clog("%s: snum not in valid range", __FUNCTION__);
#endif
      return NULL;
    }

  if (scstat.team >= NUMALLTEAMS)
    {
#if defined(DEBUG_PKT)
      clog("%s: team not in valid range", __FUNCTION__);
#endif
      return NULL;
    }

  return &scstat;
}
  
