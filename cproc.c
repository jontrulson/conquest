#include "c_defs.h"

/************************************************************************
 *
 * client packet processing (from server - SP_*)
 *
 * $Id$
 *
 * Copyright 2003 Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 ***********************************************************************/

#include "conf.h"
#include "conqnet.h"
#include "protocol.h"
#include "packet.h"

#define NOEXTERN_CPROC
#include "cproc.h"
#undef NOEXTERN_CPROC

#include "client.h"

#include "conqlb.h"
#include "conqutil.h"
#include "clientlb.h"
#include "conqcom.h"
#include "context.h"
#include "msg.h"
#include "record.h"
#include "udp.h"
#include "color.h"
#include "ui.h"

/* disptach init */
int procDispatchInit(Unsgn16 vers, packetEnt_t *pktList, int numpkts)
{
  int i;
  dispatchProc_t *procs = NULL;
  int numprocs = 0;

  switch (vers)
    {
    case 0x0006:
      procs = cprocDispatchTable_0006;
      numprocs = CPROCDISPATCHTABLENUM_0006;
      break;
    default:
      return FALSE;
    }

  if (procs == NULL || numprocs == 0)
    {
      utLog("%s: Could not find dispatch table for vers %h.", 
            __FUNCTION__, vers);
      return FALSE;
    }

  if (numprocs != numpkts)
    {
      utLog("%s: numprocs does not equal numpkts for vers %h.", 
            __FUNCTION__, vers);
      return FALSE;
    }
  
  /* now init the dispatch entries */
  for (i=0; i < numpkts; i++)
    pktList[i].dispatch = procs[i];

  return TRUE;
}

/* packet processing for current protocol */
int procUser(char *buf)
{
  int i;
  int unum;
  spUser_t *suser = (spUser_t *)buf;

  if (!pktIsValid(SP_USER, buf))
    return FALSE;

  unum = (int)((Unsgn16)ntohs(suser->unum));

#if defined(DEBUG_CLIENTPROC)
  utLog("%s: unum = %d", __FUNCTION__, unum);
#endif

  if (unum < 0 || unum >= MAXUSERS)
    return FALSE;

  if (Context.recmode == RECMODE_ON)
    recWriteEvent(buf);

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
 
#if defined(DEBUG_CLIENTPROC)
  utLog("\t%s: name: %s (%s)", __FUNCTION__, Users[unum].username, Users[unum].alias);
#endif

  return TRUE;
}
  
int procShip(char *buf)
{
  int i;
  spShip_t *sship = (spShip_t *)buf;
  int snum;

  if (!pktIsValid(SP_SHIP, buf))
    return FALSE;

  snum = sship->snum;
  if (snum <= 0 || snum > MAXSHIPS)
    return FALSE;

#if defined(DEBUG_CLIENTPROC)
  utLog("PROC SHIP: snum = %d", snum);
#endif
  if (Context.recmode == RECMODE_ON)
    recWriteEvent(buf);

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

  if (!pktIsValid(SP_SHIPSML, buf))
    return FALSE;

  snum = sshipsml->snum;
  
  if (snum <= 0 || snum > MAXSHIPS)
    return FALSE;

#if defined(DEBUG_CLIENTPROC)
  utLog("PROC SHIPSML: snum = %d", snum);
#endif

  if (Context.recmode == RECMODE_ON)
    recWriteEvent(buf);

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

  if (!pktIsValid(SP_SHIPLOC, buf))
    return FALSE;

  snum = sshiploc->snum;
  
  if (snum <= 0 || snum > MAXSHIPS)
    return FALSE;

#if defined(DEBUG_CLIENTPROC)
  utLog("PROC SHIPLOC: snum = %d", snum);
#endif

  if (Context.recmode == RECMODE_ON)
    recWriteEvent(buf);

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

  if (!pktIsValid(SP_PLANET, buf))
    return FALSE;

  pnum = splan->pnum;
  
  if (pnum <= 0 || pnum > NUMPLANETS)
    return FALSE;

  if (Context.recmode == RECMODE_ON)
    recWriteEvent(buf);

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

  if (!pktIsValid(SP_PLANETSML, buf))
    return FALSE;
  
  pnum = splansml->pnum;
  
  if (pnum <= 0 || pnum > NUMPLANETS)
    return FALSE;
  
  if (Context.recmode == RECMODE_ON)
    recWriteEvent(buf);

  for (i=0; i<NUMPLAYERTEAMS; i++)
    if (splansml->scanned & (1 << i))  
      Planets[pnum].scanned[i] = TRUE;
    else
      Planets[pnum].scanned[i] = FALSE;

  Planets[pnum].uninhabtime = (int)splansml->uninhabtime;

#if 0
  utLog("%s: %d scanned = %x", __FUNCTION__, pnum, splansml->scanned);
#endif

  return TRUE;
}

int procPlanetLoc(char *buf)
{
  spPlanetLoc_t *splanloc = (spPlanetLoc_t *)buf;
  int pnum;

  if (!pktIsValid(SP_PLANETLOC, buf))
    return FALSE;

  pnum = splanloc->pnum;
  
  if (pnum <= 0 || pnum > NUMPLANETS)
    return FALSE;

  if (Context.recmode == RECMODE_ON)
    recWriteEvent(buf);

  Planets[pnum].armies = (int)((Sgn16)ntohs(splanloc->armies));
  Planets[pnum].x = (real)((real)((Sgn32)ntohl(splanloc->x)) / 1000.0);
  Planets[pnum].y = (real)((real)((Sgn32)ntohl(splanloc->y)) / 1000.0);

  return TRUE;
}

int procPlanetLoc2(char *buf)
{
  spPlanetLoc2_t *splanloc2 = (spPlanetLoc2_t *)buf;
  int pnum;

  if (!pktIsValid(SP_PLANETLOC2, buf))
    return FALSE;

  pnum = splanloc2->pnum;
  
  if (pnum <= 0 || pnum > NUMPLANETS)
    return FALSE;

  if (Context.recmode == RECMODE_ON)
    recWriteEvent(buf);

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

  if (!pktIsValid(SP_PLANETINFO, buf))
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
    recWriteEvent(buf);

  /* in protocol 6, we 'forgot' planet realness.  To avoid breaking
     protocol again, and allow unpatched clients and/or servers to
     work we check to see if SPPLANETINFO_FLAGS_FVALID is set.  If so,
     _then_ we pay attn to any other flags present. Else we ignore
     them. */

  /* FIXME - with new CB, and later, new protocol, we will send these
     flags directly.  Right? */
  if (splaninfo->flags & SPPLANETINFO_FLAGS_FVALID)
    {                           /* we have valid flags */

      if (splaninfo->flags & SPPLANETINFO_FLAGS_REAL)
        PFSET(pnum, PLAN_F_VISIBLE);
      else
        PFCLR(pnum, PLAN_F_VISIBLE);
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

  if (!pktIsValid(SP_TORP, buf))
    return FALSE;

  snum = storp->snum;
  tnum = storp->tnum;
  
  if (snum <= 0 || snum > MAXSHIPS)
    return FALSE;

  if (tnum < 0 || tnum >= MAXTORPS)
    return FALSE;

  if (Context.recmode == RECMODE_ON)
    recWriteEvent(buf);

  Ships[snum].torps[tnum].status = (int)storp->status;

  return TRUE;
}

int procTorpLoc(char *buf)
{
  int snum, tnum, i;
  spTorpLoc_t *storploc = (spTorpLoc_t *)buf;

  if (!pktIsValid(SP_TORPLOC, buf))
    return FALSE;

  snum = storploc->snum;
  tnum = storploc->tnum;
  
  if (snum <= 0 || snum > MAXSHIPS)
    return FALSE;

  if (tnum < 0 || tnum >= MAXTORPS)
    return FALSE;

  if (Context.recmode == RECMODE_ON)
    recWriteEvent(buf);

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

  if (!pktIsValid(SP_TORPEVENT, buf))
    return FALSE;

  snum = storpev->snum;
  tnum = storpev->tnum;
  
  if (snum <= 0 || snum > MAXSHIPS)
    return FALSE;

  if (tnum < 0 || tnum >= MAXTORPS)
    return FALSE;

  Ships[snum].torps[tnum].status = (int)storpev->status;

  if (Context.recmode == RECMODE_ON)
    recWriteEvent(buf);

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
  
  if (!pktIsValid(SP_MESSAGE, buf))
    return FALSE;


  smsg->msg[MESSAGE_SIZE - 1] = 0;
  smsg->from = (int)((Sgn16)ntohs(smsg->from));
  smsg->to = (int)((Sgn16)ntohs(smsg->to));

  /* special handling when playing back a recording */
  if (Context.recmode == RECMODE_PLAYING)
    {
      /* if we aren't interested in robot msgs, skip it */
      if (!(smsg->flags & MSG_FLAGS_ROBOT) ||
          ((smsg->flags & MSG_FLAGS_ROBOT) && !UserConf.NoRobotMsgs))
        {
          memset((void *)&recMsg, 0, sizeof(Msg_t));
          strncpy(recMsg.msgbuf, (char *)smsg->msg, MESSAGE_SIZE);
          recMsg.msgfrom = (int)smsg->from;
          recMsg.msgto = (int)smsg->to;
          recMsg.flags = smsg->flags;
        }
      return TRUE;
    }

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
    recWriteEvent(buf);

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

  if (!pktIsValid(SP_TEAM, buf))
    return FALSE;

  team = steam->team;

  if (team < 0 || team >= NUMALLTEAMS)
    return FALSE;

  if (Context.recmode == RECMODE_ON)
    recWriteEvent(buf);

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

  if (!pktIsValid(SP_SERVERSTAT, buf))
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

  if (!pktIsValid(SP_CONQINFO, buf))
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

  if (!pktIsValid(SP_HISTORY, buf))
    return FALSE;

  hnum = hist->hnum;

  ConqInfo->histptr = hist->histptr;
  History[hnum].histunum = (int)ntohs(hist->unum);

  History[hnum].elapsed = (time_t)ntohl((Unsgn32)hist->elapsed);
  History[hnum].histlog = (time_t)ntohl((Unsgn32)hist->histlog);

  /* FIXME: need to extract username from hist packet when
     new protocol. */

  return TRUE;
}

int procDoomsday(char *buf)
{ 
  spDoomsday_t *dd = (spDoomsday_t *)buf;

  if (!pktIsValid(SP_DOOMSDAY, buf))
    return FALSE;

  if (Context.recmode == RECMODE_ON)
    recWriteEvent(buf);

  Doomsday->status = dd->status;
  Doomsday->heading =(real)((real)ntohs(dd->heading) / 10.0);
  Doomsday->x = (real)((real)((Sgn32)ntohl(dd->x)) / 1000.0);
  Doomsday->y = (real)((real)((Sgn32)ntohl(dd->y)) / 1000.0);

  return TRUE;
}

int procAck(char *buf)
{
  spAck_t *sack;
  spAckMsg_t *sackm;

  if (pktIsValid(SP_ACK, buf))
    {
      sack = (spAck_t *)buf;
      lastServerError = sack->code;
      
      /* set the global variants.  We save both Ack and AckMsgs here */
      sAckMsg.type = sack->type;
      sAckMsg.severity = sack->severity;
      sAckMsg.code = sack->code;
      sAckMsg.txt[0] = 0;

      return TRUE;
    }

  if (pktIsValid(SP_ACKMSG, buf))
    {
      sackm = (spAckMsg_t *)buf;
      lastServerError = sackm->code;
      
      /* save a copy in the global variant. We save both Ack and AckMsgs here */
      sAckMsg.type = sackm->type;
      sAckMsg.severity = sackm->severity;
      sAckMsg.code = sackm->code;
      memset((void*)sAckMsg.txt, 0,  MESSAGE_SIZE);
      strncpy((char *)sAckMsg.txt, (char *)sackm->txt, MESSAGE_SIZE - 1);

      return TRUE;
    }

  return FALSE;
}

int procClientStat(char *buf)
{
  spClientStat_t *scstat;

  /* chkClientStat will handle the endian conversion(s) */
  if ((scstat = chkClientStat(buf)))
    {
      Context.snum = scstat->snum;
      Context.unum = scstat->unum;
      Ships[Context.snum].team = scstat->team;
      clientFlags = scstat->flags;

      /* save a copy in the global variant */
      sClientStat.type = scstat->type;
      sClientStat.flags = scstat->flags;
      sClientStat.snum = scstat->snum;
      sClientStat.team = scstat->team;
      sClientStat.unum = scstat->unum;
      sClientStat.esystem = scstat->esystem;

      return TRUE;
    }
  
  return FALSE;
}

int procFrame(char *buf)
{
  spFrame_t *frame;

  if (pktIsValid(SP_FRAME, buf))
    {
      frame = (spFrame_t *)buf;
      /* endian correction*/
      frame->time = (Unsgn32)ntohl(frame->time);
      frame->frame = (Unsgn32)ntohl(frame->frame);
      
      if (recStartTime == (time_t)0)
        recStartTime = (time_t)frame->time;
      recCurrentTime = (time_t)frame->time;
      
      recFrameCount = (Unsgn32)frame->frame;

      /* save a copy in the global variant */
      sFrame.type = frame->type;
      sFrame.frame = frame->frame;
      sFrame.time = frame->time;

      return TRUE;
    }

  return FALSE;
}

void processPacket(char *buf)
{
  int pkttype;

  if (!buf)
    return;

  pkttype = (int)buf[0];

  if (pkttype < 0 || pkttype >= serverPktMax)
    {
      utLog("%s: got invalid packet type %d",
            pkttype);
    }
  else
    {
      /* dispatch it */
      PKT_PROCSP(buf);
    }

  return;
}

