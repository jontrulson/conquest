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
#include "client.h"
#include "clientlb.h"
#include "conqcom.h"
#include "context.h"
#include "msg.h"
#include "record.h"

/* send an auth packet and wait for a response (if not CHPWD).
   returns the error code from the ack, or error from write/read */
int sendAuth(int sock, Unsgn8 flag, Unsgn8 *login, Unsgn8 *pw)
{
  Unsgn8 buf[PKT_MAXSIZE];
  int rv;
  spAck_t *sack;
  cpAuthenticate_t cauth;

  memset((void *)&cauth, 0, sizeof(cauth));

  cauth.type = CP_AUTHENTICATE;
  cauth.flag = flag;

  if (login)
    strncpy(cauth.login, login, MAXUSERNAME - 1);
  if (pw)
    strncpy(cauth.pw, pw, MAXUSERNAME - 1);

#if defined(DEBUG_CLIENTSEND)
  clog("sendAuth: LOGIN = '%s' PW = '%s'\n",
       login, pw);
#endif

  if ((rv = writePacket(PKT_TOSERVER, cInfo.sock, (Unsgn8 *)&cauth)) <= 0)
    {
      clog("sendAuth: writePacket returned %d\n", rv);
      return rv;
    }

  /* now look for an SP_ACK, if we're not changeing pw's */

  if (flag == CPAUTH_CHGPWD)
    return PERR_OK;

  rv = waitForPacket(PKT_FROMSERVER, cInfo.sock, SP_ACK, buf, PKT_MAXSIZE, 
		     15, NULL);

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

  if (writePacket(PKT_TOSERVER, sock, (Unsgn8 *)&csc) <= 0)
    return FALSE;
  else
    return TRUE;
}

int procUser(Unsgn8 *buf)
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

  strncpy(Users[unum].username, suser->username, MAXUSERNAME - 1);
  strncpy(Users[unum].alias, suser->alias, MAXUSERPNAME - 1);
 
  return TRUE;
}
  
int procShip(Unsgn8 *buf)
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
  strncpy(Ships[snum].alias, sship->alias, MAXUSERPNAME - 1);
  
  return TRUE;
}

int procShipSml(Unsgn8 *buf)
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

int procShipLoc(Unsgn8 *buf)
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

int procPlanet(Unsgn8 *buf)
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
  strncpy(Planets[pnum].name, splan->name, MAXPLANETNAME);

  return TRUE;
}

int procPlanetSml(Unsgn8 *buf)
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

  return TRUE;
}

int procPlanetLoc(Unsgn8 *buf)
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

int procTorp(Unsgn8 *buf)
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

int procTorpLoc(Unsgn8 *buf)
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


int procMessage(Unsgn8 *buf)
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
    clntDisplayFeedback(smsg->msg);
  else
    clntStoreMessage(smsg);

  return TRUE;
}

int procTeam(Unsgn8 *buf)
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

  strncpy(Teams[team].name, steam->name, MAXTEAMNAME - 1);

  return TRUE;
}

int procServerStat(Unsgn8 *buf)
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

int procConqInfo(Unsgn8 *buf)
{
  spConqInfo_t *spci = (spConqInfo_t *)buf;

  if (!validPkt(SP_CONQINFO, buf))
    return FALSE;

  strncpy(ConqInfo->conqueror, spci->conqueror, MAXUSERPNAME);
  ConqInfo->conqueror[MAXUSERPNAME - 1] = 0;
  strncpy(ConqInfo->conqteam, spci->conqteam, MAXTEAMNAME);
  ConqInfo->conqteam[MAXTEAMNAME - 1] = 0;
  strncpy(ConqInfo->conqtime, spci->conqtime, DATESIZE);
  ConqInfo->conqtime[DATESIZE - 1] = 0;
  strncpy(ConqInfo->lastwords, spci->lastwords, MAXLASTWORDS);
  ConqInfo->lastwords[MAXLASTWORDS - 1] = 0;

  return TRUE;
}

int procHistory(Unsgn8 *buf)
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

/* send a command to server to change name */
int sendSetName(Unsgn8 *name)
{
  cpSetName_t sname;

  if (!name)
    return FALSE;

  memset((void *)&sname, 0, sizeof(cpSetName_t));
  sname.type = CP_SETNAME;
  strncpy(sname.alias, name, MAXUSERPNAME - 1);

  if (writePacket(PKT_TOSERVER, cInfo.sock, (Unsgn8 *)&sname) <= 0)
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

  if (writePacket(PKT_TOSERVER, cInfo.sock, (Unsgn8 *)&ccmd) <= 0)
    return FALSE;
  else 
    return TRUE;
}

int sendFireTorps(int num, real dir)
{
  cpFireTorps_t ftorps;

  memset((void *)&ftorps, 0, sizeof(cpFireTorps_t));

  ftorps.type = CP_FIRETORPS;
  ftorps.num = (Unsgn8)num;
  ftorps.dir = htons((Unsgn16)(dir * 100.0));

  if (writePacket(PKT_TOSERVER, cInfo.sock, (Unsgn8 *)&ftorps) <= 0)
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
  strncpy(cmsg.msg, msg, MESSAGE_SIZE - 1);

  if (writePacket(PKT_TOSERVER, cInfo.sock, (Unsgn8 *)&cmsg) <= 0)
    return FALSE;
  else 
    return TRUE;
}

