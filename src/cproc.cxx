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
#include "conf.h"
#include "conqnet.h"
#include "protocol.h"
#include "packet.h"

#define NOEXTERN_CPROC
#include "cproc.h"
#undef NOEXTERN_CPROC

#include "client.h"

#include "global.h"
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
#include "ping.h"

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
        utLog("%s: Could not find dispatch table for vers %hu.",
              __FUNCTION__, vers);
        return false;
    }

    if (numprocs != numpkts)
    {
        utLog("%s: numprocs does not equal numpkts for vers %hu.",
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

    if (unum < 0 || unum >= cbLimits.maxUsers())
        return false;

    cbUsers[unum].team = static_cast<Team::Team>(suser->team);
    cbUsers[unum].type = static_cast<UserType::UserType>(suser->userType);

    cbUsers[unum].flags = ntohs(suser->flags);
    cbUsers[unum].opFlags = ntohs(suser->opFlags);

    for (i=0; i<NUM_PLAYERTEAMS; i++)
        if ((suser->war & (1 << i)))
            cbUsers[unum].war[i] = true;

    cbUsers[unum].rating = (real)((real)((int16_t)ntohs(suser->rating)) / 10.0);
    cbUsers[unum].lastentry = (time_t)ntohl(suser->lastentry);

    for (i=0; i<UserStats::TotalStats; i++)
        cbUsers[unum].stats[i] = (int32_t)ntohl(suser->stats[i]);

    utStrncpy(cbUsers[unum].username, (char *)suser->username, MAX_USERNAME);
    utStrncpy(cbUsers[unum].alias, (char *)suser->alias, MAX_USERNAME);

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
    if (snum < 0 || snum >= cbLimits.maxShips())
        return false;

    cbShips[snum].status = sship->status;
    cbShips[snum].team = static_cast<Team::Team>(sship->team);
    cbShips[snum].unum = ntohs(sship->unum);
    cbShips[snum].shiptype = sship->shiptype;

#if defined(DEBUG_CLIENTPROC)
    utLog("PROC SHIP: snum = %d, team = %d, shiptype = %d", snum,
          cbShips[snum].team,
          cbShips[snum].shiptype);
#endif

    for (i=0; i<NUM_PLAYERTEAMS; i++)
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
        int word = i / (sizeof(uint32_t) * 8);
        int bit = i % (sizeof(uint32_t) * 8);

        if (sship->srpwar[word] & (1 << bit))
            cbShips[snum].srpwar[i] = true;
        else
            cbShips[snum].srpwar[i] = false;
    }

    for (i=0; i<NUM_PLAYERTEAMS; i++)
        cbShips[snum].scanned[i] = (int)sship->scanned[i];

    sship->alias[MAX_USERNAME - 1] = 0;
    utStrncpy(cbShips[snum].alias, (char *)sship->alias, MAX_USERNAME);

    return true;
}

int procShipSml(char *buf)
{
    int snum;
    spShipSml_t *sshipsml = (spShipSml_t *)buf;

    if (!pktIsValid(SP_SHIPSML, buf))
        return false;

    snum = sshipsml->snum;

    if (snum < 0 || snum >= cbLimits.maxShips())
        return false;

#if defined(DEBUG_CLIENTPROC)
    utLog("PROC SHIPSML: snum = %d", snum);
#endif

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

    if (snum < 0 || snum >= cbLimits.maxShips())
        return false;

#if defined(DEBUG_CLIENTPROC)
    utLog("PROC SHIPLOC: snum = %d", snum);
#endif

    cbShips[snum].head = ((real)ntohs(sshiploc->head) / 10.0);
    cbShips[snum].warp = (real)((int16_t)ntohs((uint16_t)sshiploc->warp) / 10.0);
    cbShips[snum].dwarp = (real)sshiploc->dwarp;
    // sshiploc->flags - not used yet
    cbShips[snum].x = ((real)((int32_t)ntohl(sshiploc->x)) / 10.0);
    cbShips[snum].y = ((real)((int32_t)ntohl(sshiploc->y)) / 10.0);

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

    cbPlanets[pnum].type = static_cast<PlanetType::PlanetType>(splan->ptype);
    cbPlanets[pnum].team = static_cast<Team::Team>(splan->team);
    cbPlanets[pnum].defendteam = static_cast<Team::Team>(splan->defendteam);
    cbPlanets[pnum].size = ntohs(splan->size);

    utStrncpy(cbPlanets[pnum].name, (char *)splan->name, MAX_PLANETNAME);

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

    for (i=0; i<NUM_PLAYERTEAMS; i++)
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

    cbPlanets[pnum].armies = (int)((int16_t)ntohs(splanloc->armies));
    cbPlanets[pnum].x = (real)((real)((int32_t)ntohl(splanloc->x)) / 10.0);
    cbPlanets[pnum].y = (real)((real)((int32_t)ntohl(splanloc->y)) / 10.0);
    cbPlanets[pnum].orbang = (real)((real)((uint16_t)ntohs(splanloc->orbang)) / 100.0);

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

    if (snum < 0 || snum >= cbLimits.maxShips())
        return false;

    if (tnum < 0 || tnum >= cbLimits.maxTorps())
        return false;

    cbShips[snum].torps[tnum].status = static_cast<TorpStatus>(storp->status);

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

    if (snum < 0 || snum >= cbLimits.maxShips())
        return false;

    if (tnum < 0 || tnum >= cbLimits.maxTorps())
        return false;

    for (i=0; i<NUM_PLAYERTEAMS; i++)
        if (storploc->war & (1 << i))
            cbShips[snum].torps[tnum].war[i] = true;
        else
            cbShips[snum].torps[tnum].war[i] = false;

    cbShips[snum].torps[tnum].x =
        (real)((real)((int32_t)ntohl(storploc->x)) / 10.0);
    cbShips[snum].torps[tnum].y =
        (real)((real)((int32_t)ntohl(storploc->y)) / 10.0);

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

    if (snum < 0 || snum >= cbLimits.maxShips())
        return false;

    if (tnum < 0 || tnum >= cbLimits.maxTorps())
        return false;

    cbShips[snum].torps[tnum].status = static_cast<TorpStatus>(storpev->status);

    for (i=0; i<NUM_PLAYERTEAMS; i++)
        if (storpev->war & (1 << i))
            cbShips[snum].torps[tnum].war[i] = true;
        else
            cbShips[snum].torps[tnum].war[i] = false;

    cbShips[snum].torps[tnum].x =
        (real)((real)((int32_t)ntohl(storpev->x)) / 10.0);
    cbShips[snum].torps[tnum].y =
        (real)((real)((int32_t)ntohl(storpev->y)) / 10.0);

    cbShips[snum].torps[tnum].dx =
        (real)((real)((int32_t)ntohl(storpev->dx)) / 100.0);
    cbShips[snum].torps[tnum].dy =
        (real)((real)((int32_t)ntohl(storpev->dy)) / 100.0);

    return true;
}


int procMessage(char *buf)
{
    spMessage_t *smsg = (spMessage_t *)buf;

    if (!pktIsValid(SP_MESSAGE, buf))
        return false;

    smsg->msg[MAX_MSGSIZE - 1] = 0;
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
            utStrncpy(recMsg.msgbuf, (char *)smsg->msg, MAX_MSGSIZE);

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

    if (team < 0 || team >= NUM_ALLTEAMS)
        return false;

    cbTeams[team].flags = ntohs(steam->flags);

    cbTeams[team].homeplanet = (int)steam->homeplanet;

    cbTeams[team].couptime = steam->couptime;

    for (i=0; i<MAX_TEAM_STATS; i++)
        cbTeams[team].stats[i] = (int)ntohl(steam->stats[i]);

    utStrncpy(cbTeams[team].name, (char *)steam->name, MAX_TEAMNAME);

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

int procConqInfo(char *buf)
{
    spConqInfo_t *spci = (spConqInfo_t *)buf;

    if (!pktIsValid(SP_CONQINFO, buf))
        return false;

    utStrncpy(cbConqInfo->conqueror, (char *)spci->conqueror, MAX_USERNAME);
    utStrncpy(cbConqInfo->conqteam, (char *)spci->conqteam, MAX_TEAMNAME);
    utStrncpy(cbConqInfo->conqtime, (char *)spci->conqtime, MAX_DATESIZE);
    utStrncpy(cbConqInfo->lastwords, (char *)spci->lastwords, MAX_LASTWORDS);

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

    utStrncpy(cbHistory[hnum].username, (char *)hist->username, MAX_USERNAME);

#if defined(DEBUG_CLIENTPROC)
    utLog("%s: hnum = %d, hptr = %d, unum = %d name = (%s)",
          __FUNCTION__, hnum,
          cbConqInfo->histptr,
          cbHistory[hnum].unum,
          cbHistory[hnum].username);
#endif

    return true;
}

int procDoomsday(char *buf)
{
    spDoomsday_t *dd = (spDoomsday_t *)buf;

    if (!pktIsValid(SP_DOOMSDAY, buf))
        return false;

    cbDoomsday->flags = dd->flags;
    cbDoomsday->heading =(real)((real)ntohs(dd->heading) / 10.0);
    cbDoomsday->x = (real)((real)((int32_t)ntohl(dd->x)) / 10.0);
    cbDoomsday->y = (real)((real)((int32_t)ntohl(dd->y)) / 10.0);

    cbDoomsday->eaterType = static_cast<eaterType_t>(dd->eaterType);

    return true;
}

int procAck(char *buf)
{
    spAck_t *sack;

    if (pktIsValid(SP_ACK, buf))
    {
        sack = (spAck_t *)buf;
        if (sack->code == PERR_PINGRESP)
        {
            // not an error, log the response
            pingResponse(clbGetMillis());
        }
        else
            clientLastServerAckCode = sack->code;

        /* set the global variants */
        sAck.type = sack->type;
        sAck.severity = sack->severity;
        sAck.code = sack->code;

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
        cbShips[Context.snum].team = static_cast<Team::Team>(scstat->team);
        clientStatLastFlags = scstat->flags;

        /* save a copy in the global variant */
        sClientStat.type = scstat->type;
        sClientStat.flags = scstat->flags;
        sClientStat.snum = scstat->snum;
        sClientStat.team = scstat->team;
        sClientStat.unum = scstat->unum;
        sClientStat.esystem = scstat->esystem;

        clientStatReceived = true;
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

// A helper function for procAckUDP().  Return false on success, true
// on error.
static bool _connect_udp(void)
{
    struct sockaddr_in sa;
    struct hostent *hp = NULL;

    if ((cInfo.usock = udpOpen(0, &sa)) < 0)
    {
        utLog("%s: NET: udpOpen(): %s", __FUNCTION__, strerror(errno));
        return true;
    }
    else
    {
        if ((hp = gethostbyname(cInfo.remotehost)) == NULL)
        {
            utLog("%s: gethostbyname(%s): failed",
                  __FUNCTION__, cInfo.remotehost);
            close(cInfo.sock);
            cInfo.sock = -1;
            return true;
        }

        /* put host's address and address type into socket structure */
        memcpy((char *)&sa.sin_addr, (char *)hp->h_addr, hp->h_length);
        sa.sin_family = hp->h_addrtype;
        sa.sin_port = htons(cInfo.remoteport);

        if (connect(cInfo.usock,
                    (const struct sockaddr *)&sa,
                    sizeof(struct sockaddr_in)) < 0)
        {
            utLog("%s: NET: UDP connect() failed: %s",
                  __FUNCTION__, strerror(errno));
            close(cInfo.usock);
            cInfo.usock = true;
        }
    }

    return false;
}

int procAckUDP(char *buf)
{
    spAckUDP_t *ackUDP;

    if (!pktIsValid(SP_ACKUDP, buf))
        return false;

    ackUDP = (spAckUDP_t *)buf;
    /* endian correction*/
    ackUDP->payload = ntohl(ackUDP->payload);

    switch(ackUDP->state)
    {
        case PKTUDP_STATE_SERVER_READY:
        {
            // our cue to open UDP, and send a packet.
            if (cqDebug)
                utLog("%s: NET: received SERVER_READY, starting UDP...",
                      __FUNCTION__);

            if (_connect_udp())
            {
                // bummer....
                utLog("%s: Failed to connect to remote host via UDP",
                      __FUNCTION__);

                // send error code to server
                pktSendAckUDP(PKT_SENDTCP, PKTUDP_STATE_CLIENT_ERR, 0);
                return true;
            }
            else
            {
                /* see if this will succeed in setting up a NAT tunnel
                   to the server */
                if (cqDebug)
                    utLog("%s: NET: sending CLIENT_READY UDP response to server.",
                          __FUNCTION__);
                // we need to set this so that the packet handling
                // routines can use the socket
                pktSetSocketFds(PKT_SOCKFD_NOCHANGE, cInfo.usock);

                // send back the payload (server's PID), via UDP
                pktSendAckUDP(PKT_SENDUDP, PKTUDP_STATE_CLIENT_READY,
                              ackUDP->payload);
                // reset till we are really ready...
                pktSetSocketFds(PKT_SOCKFD_NOCHANGE, -1);
            }
        }

        // now we should await a SERVER_UDP_ON
        break;

        case PKTUDP_STATE_SERVER_ERR:
        {
            // server had a failure trying to setup UDP, or after UDP
            // was established, decided to disable it for some
            // reason...
            utLog("%s: NET: received SERVER_ERR, disabling UDP",
                  __FUNCTION__);
            cInfo.doUDP = false;
            if (cInfo.usock >= 0)
                close(cInfo.usock);
            cInfo.usock = -1;
            pktSetSocketFds(PKT_SOCKFD_NOCHANGE, cInfo.usock);
        }

        break;

        case PKTUDP_STATE_SERVER_UDP_ON:
        {
            // nothing to do here, but start using UDP
            utLog("%s: NET: received SERVER_UDP_ON, UDP is ON",
                  __FUNCTION__);
            pktSetSocketFds(PKT_SOCKFD_NOCHANGE, cInfo.usock);
            cInfo.doUDP = true;
        }

        break;

        default:
            utLog("%s: NET: Unhandled state %d",
                  __FUNCTION__, (int)ackUDP->state);
    }

    return true;
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
        if (cqDebug >= 5)
            utLog("%s: Processing pkt type: %d (%s)", __FUNCTION__,
                  pkttype, serverPackets[(uint8_t)pkttype].name);

        /* dispatch it */
        PKT_PROCSP(buf);
    }

    return;
}

int procShiptype(char *buf)
{
    spShiptype_t *stype = (spShiptype_t *)buf;

    if (!pktIsValid(SP_SHIPTYPE, buf))
        return false;

    uint8_t index = stype->index;

    if (index >= MAXNUMSHIPTYPES)
    {
        utLog("%s: got invalid shiptype index %d", __FUNCTION__,
              (int)index);
        return false;
    }

    cbShipTypes[index].engfac = ((real)ntohl(stype->engfac) / 100.0);
    cbShipTypes[index].accelfac = ((real)ntohl(stype->accelfac) / 100.0);
    cbShipTypes[index].weafac = ((real)ntohl(stype->weafac) / 100.0);

    cbShipTypes[index].torpwarp = (real)stype->torpwarp;

    cbShipTypes[index].armyMax = stype->armyMax;
    cbShipTypes[index].warpMax = stype->warpMax;
    cbShipTypes[index].torpMax = stype->torpMax;

    cbShipTypes[index].shieldMax = (real)ntohs(stype->shieldMax);
    cbShipTypes[index].damageMax = (real)ntohs(stype->damageMax);
    cbShipTypes[index].fuelMax = (real)ntohs(stype->fuelMax);

    cbShipTypes[index].size = (real)ntohs(stype->size);

    utStrncpy(cbShipTypes[index].name, (char *)stype->name, MAXSTNAME);

    return true;
}

