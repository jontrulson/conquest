#include "c_defs.h"

/************************************************************************
 *
 * client packet processing (from server - SP_*)
 *
 * Copyright Jon Trulson under the MIT License. (See LICENSE).
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
#include "cb.h"
#include "context.h"
#include "msg.h"
#include "record.h"
#include "udp.h"
#include "color.h"
#include "ui.h"

/* disptach init */
int procDispatchInit(uint16_t vers, packetEnt_t *pktList, int numpkts)
{
    int i;
    dispatchProc_t *procs = NULL;
    int numprocs = 0;

    switch (vers)
    {
    case 0x0006: // backwards compat for recordings (.cqr files)
        procs = cprocDispatchTable_0006;
        numprocs = CPROCDISPATCHTABLENUM_0006;
        break;

    case 0x0007: // current
        procs = cprocDispatchTable_0007;
        numprocs = CPROCDISPATCHTABLENUM_0007;
        break;
    }

    if (procs == NULL || numprocs == 0)
    {
        utLog("%s: Could not find dispatch table for vers %h.",
              __FUNCTION__, vers);
        return false;
    }

    if (numprocs != numpkts)
    {
        utLog("%s: numprocs does not equal numpkts for vers %h.",
              __FUNCTION__, vers);
        return false;
    }

    /* now init the dispatch entries */
    for (i=0; i < numpkts; i++)
        pktList[i].dispatch = procs[i];

    return true;
}

/* packet processing for current protocol */
int procUser(char *buf)
{
    int i;
    int unum;
    spUser_t *suser = (spUser_t *)buf;

    if (!pktIsValid(SP_USER, buf))
        return false;

    unum = (int)((uint16_t)ntohs(suser->unum));

#if defined(DEBUG_CLIENTPROC)
    utLog("%s: unum = %d", __FUNCTION__, unum);
#endif

    if (unum < 0 || unum >= MAXUSERS)
        return false;

    if (Context.recmode == RECMODE_ON)
        recWriteEvent(buf);

    cbUsers[unum].team = suser->team;
    cbUsers[unum].type = (userTypes_t)suser->userType;

    cbUsers[unum].flags = ntohs(suser->flags);
    cbUsers[unum].opFlags = ntohs(suser->opFlags);

    for (i=0; i<NUMPLAYERTEAMS; i++)
        if ((suser->war & (1 << i)))
            cbUsers[unum].war[i] = true;

    cbUsers[unum].rating = (real)((real)((int16_t)ntohs(suser->rating)) / 10.0);
    cbUsers[unum].lastentry = (time_t)ntohl(suser->lastentry);

    for (i=0; i<USTAT_TOTALSTATS; i++)
        cbUsers[unum].stats[i] = (int32_t)ntohl(suser->stats[i]);

    utStrncpy(cbUsers[unum].username, (char *)suser->username, MAXUSERNAME);
    utStrncpy(cbUsers[unum].alias, (char *)suser->alias, MAXUSERNAME);

#if defined(DEBUG_CLIENTPROC)
    utLog("\t%s: name: %s (%s)", __FUNCTION__, cbUsers[unum].username, cbUsers[unum].alias);
#endif

    return true;
}

int procShip(char *buf)
{
    int i;
    spShip_t *sship = (spShip_t *)buf;
    int snum;

    if (!pktIsValid(SP_SHIP, buf))
        return false;

    snum = sship->snum;
    if (snum < 0 || snum >= MAXSHIPS)
        return false;

#if defined(DEBUG_CLIENTPROC)
    utLog("PROC SHIP: snum = %d", snum);
#endif
    if (Context.recmode == RECMODE_ON)
        recWriteEvent(buf);

    cbShips[snum].status = sship->status;
    cbShips[snum].team = sship->team;
    cbShips[snum].unum = ntohs(sship->unum);
    cbShips[snum].shiptype = sship->shiptype;

    for (i=0; i<NUMPLAYERTEAMS; i++)
    {
        if (sship->war & (1 << i))
            cbShips[snum].war[i] = true;
        else
            cbShips[snum].war[i] = false;

        if (sship->rwar & (1 << i))
            cbShips[snum].rwar[i] = true;
        else
            cbShips[snum].rwar[i] = false;
    }

    cbShips[snum].killedBy = (killedBy_t)sship->killedBy;
    cbShips[snum].killedByDetail =
        (unsigned int)((uint16_t)ntohs(sship->killedByDetail));

    cbShips[snum].kills = (real)((real)ntohl(sship->kills) / 10.0);

    // srpwar - convert to host order
    for (i=0; i<PROTO_SRPWAR_BIT_WORDS; i++)
        sship->srpwar[i] = ntohl(sship->srpwar[i]);

    // decode the bit encoded representation
    for (i=0; i<cbLimits.maxPlanets(); i++)
    {
        int word = i / sizeof(uint32_t);
        int bit = i % sizeof(uint32_t);

        if (sship->srpwar[word] & (1 << bit))
            cbShips[snum].srpwar[i] = true;
        else
            cbShips[snum].srpwar[i] = false;
    }

    for (i=0; i<NUMPLAYERTEAMS; i++)
        cbShips[snum].scanned[i] = (int)sship->scanned[i];

    sship->alias[MAXUSERNAME - 1] = 0;
    utStrncpy(cbShips[snum].alias, (char *)sship->alias, MAXUSERNAME);

    return true;
}

int procShipSml(char *buf)
{
    int snum;
    spShipSml_t *sshipsml = (spShipSml_t *)buf;

    if (!pktIsValid(SP_SHIPSML, buf))
        return false;

    snum = sshipsml->snum;

    if (snum < 0 || snum >= MAXSHIPS)
        return false;

#if defined(DEBUG_CLIENTPROC)
    utLog("PROC SHIPSML: snum = %d", snum);
#endif

    if (Context.recmode == RECMODE_ON)
        recWriteEvent(buf);

    /* we need to mask out map since it's always local */
    cbShips[snum].flags =
        (ntohl(sshipsml->flags) & ~SHIP_F_MAP) | SMAP(snum);

    cbShips[snum].action = sshipsml->action;
    cbShips[snum].shields = sshipsml->shields;
    cbShips[snum].damage = sshipsml->damage;
    cbShips[snum].armies = sshipsml->armies;
    cbShips[snum].sdfuse = (int)((int16_t)ntohs(sshipsml->sdfuse));

    cbShips[snum].wfuse = (int)sshipsml->wfuse;
    cbShips[snum].efuse = (int)sshipsml->efuse;

    cbShips[snum].towing = sshipsml->towing;
    cbShips[snum].towedby = sshipsml->towedby;

    cbShips[snum].weapalloc = sshipsml->walloc;
    cbShips[snum].engalloc = 100 - cbShips[snum].weapalloc;

    cbShips[snum].pfuse = (int)sshipsml->pfuse;

    cbShips[snum].etemp = (real)sshipsml->etemp;
    cbShips[snum].wtemp = (real)sshipsml->wtemp;
    cbShips[snum].fuel = (real)((uint16_t)ntohs(sshipsml->fuel));
    // a check...
    if (sshipsml->lock < LOCK_MAX)
        cbShips[snum].lock = (courseLock_t)sshipsml->lock;
    else
        utLog("%s: got invalid courseLock enueration %d, ignoring",
              __FUNCTION__, (int)sshipsml->lock);

    cbShips[snum].lockDetail = ntohs(sshipsml->lockDetail);
    cbShips[snum].lastphase = (real)((uint16_t)ntohs(sshipsml->lastphase)) / 100.0;
    cbShips[snum].lastblast = (real)((uint16_t)ntohs(sshipsml->lastblast)) / 100.0;

    return true;
}

int procShipLoc(char *buf)
{
    int snum;
    spShipLoc_t *sshiploc = (spShipLoc_t *)buf;

    if (!pktIsValid(SP_SHIPLOC, buf))
        return false;

    snum = sshiploc->snum;

    if (snum < 0 || snum >= MAXSHIPS)
        return false;

#if defined(DEBUG_CLIENTPROC)
    utLog("PROC SHIPLOC: snum = %d", snum);
#endif

    if (Context.recmode == RECMODE_ON)
        recWriteEvent(buf);

    cbShips[snum].head = (real)((real)ntohs(sshiploc->head) / 10.0);
    cbShips[snum].warp = (real)((real)sshiploc->warp / 10.0);

    cbShips[snum].x = (real)((real)((int32_t)ntohl(sshiploc->x)) / 1000.0);
    cbShips[snum].y = (real)((real)((int32_t)ntohl(sshiploc->y)) / 1000.0);

    return true;
}

int procPlanet(char *buf)
{
    spPlanet_t *splan = (spPlanet_t *)buf;
    int pnum;

    if (!pktIsValid(SP_PLANET, buf))
        return false;

    pnum = splan->pnum;

    if (pnum < 0 || pnum >= cbLimits.maxPlanets())
        return false;

    if (Context.recmode == RECMODE_ON)
        recWriteEvent(buf);

    cbPlanets[pnum].type = splan->ptype;
    cbPlanets[pnum].team = splan->team;
    cbPlanets[pnum].defendteam = splan->defendteam;
    cbPlanets[pnum].size = ntohs(splan->size);

    utStrncpy(cbPlanets[pnum].name, (char *)splan->name, MAXPLANETNAME);

    uiUpdatePlanet(pnum);

    return true;
}

int procPlanetSml(char *buf)
{
    int i;
    spPlanetSml_t *splansml = (spPlanetSml_t *)buf;
    int pnum;

    if (!pktIsValid(SP_PLANETSML, buf))
        return false;

    pnum = splansml->pnum;

    if (pnum < 0 || pnum >= cbLimits.maxPlanets())
        return false;

    if (Context.recmode == RECMODE_ON)
        recWriteEvent(buf);

    for (i=0; i<NUMPLAYERTEAMS; i++)
        if (splansml->scanned & (1 << i))
            cbPlanets[pnum].scanned[i] = true;
        else
            cbPlanets[pnum].scanned[i] = false;

    cbPlanets[pnum].uninhabtime = (int)splansml->uninhabtime;

#if 0
    utLog("%s: %d scanned = %x", __FUNCTION__, pnum, splansml->scanned);
#endif

    return true;
}

int procPlanetLoc(char *buf)
{
    spPlanetLoc_t *splanloc = (spPlanetLoc_t *)buf;
    int pnum;

    if (!pktIsValid(SP_PLANETLOC, buf))
        return false;

    pnum = splanloc->pnum;

    if (pnum < 0 || pnum >= cbLimits.maxPlanets())
        return false;

    if (Context.recmode == RECMODE_ON)
        recWriteEvent(buf);

    cbPlanets[pnum].armies = (int)((int16_t)ntohs(splanloc->armies));
    cbPlanets[pnum].x = (real)((real)((int32_t)ntohl(splanloc->x)) / 1000.0);
    cbPlanets[pnum].y = (real)((real)((int32_t)ntohl(splanloc->y)) / 1000.0);

    return true;
}

int procPlanetLoc2(char *buf)
{
    spPlanetLoc2_t *splanloc2 = (spPlanetLoc2_t *)buf;
    int pnum;

    if (!pktIsValid(SP_PLANETLOC2, buf))
        return false;

    pnum = splanloc2->pnum;

    if (pnum < 0 || pnum >= cbLimits.maxPlanets())
        return false;

    if (Context.recmode == RECMODE_ON)
        recWriteEvent(buf);

    cbPlanets[pnum].armies = (int)((int16_t)ntohs(splanloc2->armies));
    cbPlanets[pnum].x = (real)((real)((int32_t)ntohl(splanloc2->x)) / 1000.0);
    cbPlanets[pnum].y = (real)((real)((int32_t)ntohl(splanloc2->y)) / 1000.0);
    cbPlanets[pnum].orbang = (real)((real)((uint16_t)ntohs(splanloc2->orbang)) / 100.0);

    return true;
}

int procPlanetInfo(char *buf)
{
    spPlanetInfo_t *splaninfo = (spPlanetInfo_t *)buf;
    int pnum;
    int primary;

    if (!pktIsValid(SP_PLANETINFO, buf))
        return false;

    pnum = splaninfo->pnum;

    if (pnum < 0 || pnum >= cbLimits.maxPlanets())
        return false;

    primary = splaninfo->primary;

    /* Roy fix - 10/17/2005 - let mur data be sent. */
    if (primary < 0 || primary >= cbLimits.maxPlanets())
        return false;

    /* we will record them if we get them */
    if (Context.recmode == RECMODE_ON)
        recWriteEvent(buf);

    cbPlanets[pnum].primary = primary;
    cbPlanets[pnum].flags = ntohl(splaninfo->flags);
    cbPlanets[pnum].orbrad = (real)((real)((uint32_t)ntohl(splaninfo->orbrad)) / 10.0);
    cbPlanets[pnum].orbvel = (real)((real)((int32_t)ntohl(splaninfo->orbvel)) / 100.0);

    return true;
}


int procTorp(char *buf)
{
    int snum, tnum;
    spTorp_t *storp = (spTorp_t *)buf;

    if (!pktIsValid(SP_TORP, buf))
        return false;

    snum = storp->snum;
    tnum = storp->tnum;

    if (snum < 0 || snum >= MAXSHIPS)
        return false;

    if (tnum < 0 || tnum >= MAXTORPS)
        return false;

    if (Context.recmode == RECMODE_ON)
        recWriteEvent(buf);

    cbShips[snum].torps[tnum].status = (int)storp->status;

    return true;
}

int procTorpLoc(char *buf)
{
    int snum, tnum, i;
    spTorpLoc_t *storploc = (spTorpLoc_t *)buf;

    if (!pktIsValid(SP_TORPLOC, buf))
        return false;

    snum = storploc->snum;
    tnum = storploc->tnum;

    if (snum < 0 || snum >= MAXSHIPS)
        return false;

    if (tnum < 0 || tnum >= MAXTORPS)
        return false;

    if (Context.recmode == RECMODE_ON)
        recWriteEvent(buf);

    for (i=0; i<NUMPLAYERTEAMS; i++)
        if (storploc->war & (1 << i))
            cbShips[snum].torps[tnum].war[i] = true;
        else
            cbShips[snum].torps[tnum].war[i] = false;

    cbShips[snum].torps[tnum].x = (real)((real)((int32_t)ntohl(storploc->x)) / 1000.0);
    cbShips[snum].torps[tnum].y = (real)((real)((int32_t)ntohl(storploc->y)) / 1000.0);

    return true;
}


int procTorpEvent(char *buf)
{
    int snum, tnum, i;
    spTorpEvent_t *storpev = (spTorpEvent_t *)buf;

    if (!pktIsValid(SP_TORPEVENT, buf))
        return false;

    snum = storpev->snum;
    tnum = storpev->tnum;

    if (snum < 0 || snum >= MAXSHIPS)
        return false;

    if (tnum < 0 || tnum >= MAXTORPS)
        return false;

    cbShips[snum].torps[tnum].status = (int)storpev->status;

    if (Context.recmode == RECMODE_ON)
        recWriteEvent(buf);

    for (i=0; i<NUMPLAYERTEAMS; i++)
        if (storpev->war & (1 << i))
            cbShips[snum].torps[tnum].war[i] = true;
        else
            cbShips[snum].torps[tnum].war[i] = false;

    cbShips[snum].torps[tnum].x =
        (real)((real)((int32_t)ntohl(storpev->x)) / 1000.0);
    cbShips[snum].torps[tnum].y =
        (real)((real)((int32_t)ntohl(storpev->y)) / 1000.0);

    cbShips[snum].torps[tnum].dx =
        (real)((real)((int32_t)ntohl(storpev->dx)) / 1000.0);
    cbShips[snum].torps[tnum].dy =
        (real)((real)((int32_t)ntohl(storpev->dy)) / 1000.0);

    uiUpdateTorpDir(snum, tnum);

    return true;
}


int procMessage(char *buf)
{
    spMessage_t *smsg = (spMessage_t *)buf;

    if (!pktIsValid(SP_MESSAGE, buf))
        return false;

    smsg->msg[MESSAGE_SIZE - 1] = 0;
    smsg->fromDetail = ntohs(smsg->fromDetail);
    smsg->toDetail = ntohs(smsg->toDetail);

    /* special handling when playing back a recording */
    if (Context.recmode == RECMODE_PLAYING)
    {
        /* if we aren't interested in robot msgs, skip it */
        if (!(smsg->flags & MSG_FLAGS_ROBOT) ||
            ((smsg->flags & MSG_FLAGS_ROBOT) && !UserConf.NoRobotMsgs))
        {
            memset((void *)&recMsg, 0, sizeof(Msg_t));
            utStrncpy(recMsg.msgbuf, (char *)smsg->msg, MESSAGE_SIZE);

            // do a check on the enums...
            if (smsg->to > MSG_TO_MAX || smsg->from > MSG_FROM_MAX)
            {
                // log it and bail
                utLog("%s: invalid to or from in message: to %d from %d",
                      __FUNCTION__, (int)smsg->to, (int)smsg->from);
                return true;
            }

            recMsg.from = static_cast<msgFrom_t>(smsg->from);
            recMsg.fromDetail = smsg->fromDetail;
            recMsg.to = static_cast<msgTo_t>(smsg->to);
            recMsg.toDetail = smsg->toDetail;
            recMsg.flags = smsg->flags;
        }
        return true;
    }

    /* terse? */
    if (UserConf.Terse && (smsg->flags & MSG_FLAGS_TERSABLE))
        return true;

    /* robot response/insult? */
    if (UserConf.NoRobotMsgs && (smsg->flags & MSG_FLAGS_ROBOT))
        return true;

    /* intruder alert? */
    if (!UserConf.DoIntrudeAlert && (smsg->flags & MSG_FLAGS_INTRUDER))
        return true;

    /* don't record feedbacks */
    if ((Context.recmode == RECMODE_ON) && !(smsg->flags & MSG_FLAGS_FEEDBACK) )
        recWriteEvent(buf);

    if (smsg->flags & MSG_FLAGS_FEEDBACK)
        clntDisplayFeedback((char *)smsg->msg);
    else
        clntStoreMessage(smsg);

    return true;
}

int procTeam(char *buf)
{
    int team, i;
    spTeam_t *steam = (spTeam_t *)buf;

    if (!pktIsValid(SP_TEAM, buf))
        return false;

    team = steam->team;

    if (team < 0 || team >= NUMALLTEAMS)
        return false;

    if (Context.recmode == RECMODE_ON)
        recWriteEvent(buf);

    if (steam->flags & SPTEAM_FLAGS_COUPINFO)
        cbTeams[team].coupinfo = true;
    else
        cbTeams[team].coupinfo = false;

    cbTeams[team].homeplanet = (int)steam->homeplanet;

    cbTeams[team].couptime = steam->couptime;

    for (i=0; i<MAXTSTATS; i++)
        cbTeams[team].stats[i] = (int)ntohl(steam->stats[i]);

    utStrncpy(cbTeams[team].name, (char *)steam->name, MAXTEAMNAME);

    return true;
}

int procServerStat(char *buf)
{
    spServerStat_t *sstat = (spServerStat_t *)buf;

    if (!pktIsValid(SP_SERVERSTAT, buf))
        return false;

    sStat = *sstat;               /* client.h - clients keep a copy. */

    /* now fixup the byte order */
    sStat.numusers = ntohs(sStat.numusers);
    sStat.serverFlags = ntohl(sStat.serverFlags);
    sStat.servertime = ntohl(sStat.servertime);

    return true;
}

int proccbConqInfo(char *buf)
{
    spcbConqInfo_t *spci = (spcbConqInfo_t *)buf;

    if (!pktIsValid(SP_CONQINFO, buf))
        return false;

    utStrncpy(cbConqInfo->conqueror, (char *)spci->conqueror, MAXUSERNAME);
    utStrncpy(cbConqInfo->conqteam, (char *)spci->conqteam, MAXTEAMNAME);
    utStrncpy(cbConqInfo->conqtime, (char *)spci->conqtime, MAXDATESIZE);
    utStrncpy(cbConqInfo->lastwords, (char *)spci->lastwords, MAXLASTWORDS);

    return true;
}

int procHistory(char *buf)
{
    spHistory_t *hist = (spHistory_t *)buf;
    int hnum;

    if (!pktIsValid(SP_HISTORY, buf))
        return false;

    hnum = hist->hnum;

    cbConqInfo->histptr = hist->histptr;
    cbHistory[hnum].unum = (int)((int16_t)ntohs(hist->unum));

    cbHistory[hnum].elapsed = (time_t)ntohl((uint32_t)hist->elapsed);
    cbHistory[hnum].enterTime = (time_t)ntohl((uint32_t)hist->enterTime);

    utStrncpy(cbHistory[hnum].username, (char *)hist->username, MAXUSERNAME);

    return true;
}

int procDoomsday(char *buf)
{
    spDoomsday_t *dd = (spDoomsday_t *)buf;

    if (!pktIsValid(SP_DOOMSDAY, buf))
        return false;

    if (Context.recmode == RECMODE_ON)
        recWriteEvent(buf);

    cbDoomsday->flags = dd->flags;
    cbDoomsday->heading =(real)((real)ntohs(dd->heading) / 10.0);
    cbDoomsday->x = (real)((real)((int32_t)ntohl(dd->x)) / 1000.0);
    cbDoomsday->y = (real)((real)((int32_t)ntohl(dd->y)) / 1000.0);

    cbDoomsday->eaterType = static_cast<eaterType_t>(dd->eaterType);

    return true;
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

        return true;
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
        utStrncpy((char *)sAckMsg.txt, (char *)sackm->txt, MESSAGE_SIZE);

        return true;
    }

    return false;
}

int procClientStat(char *buf)
{
    spClientStat_t *scstat;

    /* chkClientStat will handle the endian conversion(s) */
    if ((scstat = chkClientStat(buf)))
    {
        Context.snum = scstat->snum;
        Context.unum = scstat->unum;
        cbShips[Context.snum].team = scstat->team;
        clientFlags = scstat->flags;

        /* save a copy in the global variant */
        sClientStat.type = scstat->type;
        sClientStat.flags = scstat->flags;
        sClientStat.snum = scstat->snum;
        sClientStat.team = scstat->team;
        sClientStat.unum = scstat->unum;
        sClientStat.esystem = scstat->esystem;

        return true;
    }

    return false;
}

int procFrame(char *buf)
{
    spFrame_t *frame;

    if (pktIsValid(SP_FRAME, buf))
    {
        frame = (spFrame_t *)buf;
        /* endian correction*/
        frame->time = (uint32_t)ntohl(frame->time);
        frame->frame = (uint32_t)ntohl(frame->frame);

        if (recStartTime == (time_t)0)
            recStartTime = (time_t)frame->time;
        recCurrentTime = (time_t)frame->time;

        recFrameCount = (uint32_t)frame->frame;

        /* save a copy in the global variant */
        sFrame.type = frame->type;
        sFrame.frame = frame->frame;
        sFrame.time = frame->time;

        return true;
    }

    return false;
}

void processPacket(char *buf)
{
    int pkttype;

    if (!buf)
        return;

    pkttype = (int)buf[0];

    if (pkttype < 0 || pkttype >= serverPktMax)
    {
        utLog("%s: got invalid packet type %d", __FUNCTION__,
              pkttype);
    }
    else
    {
        /* dispatch it */
        PKT_PROCSP(buf);
    }

    return;
}
