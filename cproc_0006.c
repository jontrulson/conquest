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


/* packet processing for 0x0006 protocol */

/* We hardcode everything here so as to not be dependent on the
 * current state of affairs.  We also remove any recWriteEvent() cals
 * since you will never be recording while playing back a recording.
 */

int proc_0006_User(char *buf)
{
    int i;
    int unum;
    sp_0006_User_t *suser = (sp_0006_User_t *)buf;

    if (!pktIsValid(SP_0006_USER, buf))
        return FALSE;

    unum = (int)((uint16_t)ntohs(suser->unum));

#if defined(DEBUG_CLIENTPROC)
    utLog("%s: unum = %d", __FUNCTION__, unum);
#endif

    if (unum < 0 || unum >= 500 /*MAXUSERS*/)
        return FALSE;

    cbUsers[unum].team = suser->team;

    if (suser->flags & SP_0006_USER_FLAGS_LIVE)
        UFSET(unum, USER_F_LIVE);

    for (i=0; i<NUMPLAYERTEAMS; i++)
        if ((suser->war & (1 << i)))
            cbUsers[unum].war[i] = true;

    cbUsers[unum].rating = (real)((real)((int16_t)ntohs(suser->rating)) / 100.0);
    cbUsers[unum].lastentry = (time_t)ntohl(suser->lastentry);

    // the ooptions array doesn't exist in CB anymore, so break down
    // the older options that we care about into the new world order.

    // play when closed - 0
    int32_t opt = (int32_t)ntohl(suser->ooptions[0]);

    if (opt)
        UOPSET(unum, USER_OP_PLAYWHENCLOSED);
    else
        UOPCLR(unum, USER_OP_PLAYWHENCLOSED);

    // shitlist/banned - 3
    opt = (int32_t)ntohl(suser->ooptions[3]);

    if (opt)
        UOPSET(unum, USER_OP_BANNED);
    else
        UOPCLR(unum, USER_OP_BANNED);

    // operator privs - 4
    opt = (int32_t)ntohl(suser->ooptions[4]);

    if (opt)
        UOPSET(unum, USER_OP_ISOPER);
    else
        UOPCLR(unum, USER_OP_ISOPER);

    // autopilot - 6
    opt = (int32_t)ntohl(suser->ooptions[6]);

    if (opt)
        UOPSET(unum, USER_OP_AUTOPILOT);
    else
        UOPCLR(unum, USER_OP_AUTOPILOT);

    for (i=0; i<USTAT_TOTALSTATS; i++)
        cbUsers[unum].stats[i] = (int32_t)ntohl(suser->stats[i]);

    utStrncpy(cbUsers[unum].username, (char *)suser->username,
              32 /*MAXUSERNAME*/);
    utStrncpy(cbUsers[unum].alias, (char *)suser->alias, 24 /*MAXUSERALIAS*/);

#if defined(DEBUG_CLIENTPROC)
    utLog("\t%s: name: %s (%s)", __FUNCTION__, cbUsers[unum].username, cbUsers[unum].alias);
#endif

    return TRUE;
}

int proc_0006_Ship(char *buf)
{
    int i;
    sp_0006_Ship_t *sship = (sp_0006_Ship_t *)buf;
    int snum;

    if (!pktIsValid(SP_0006_SHIP, buf))
        return FALSE;

    // we subtract one to compensate for new 0-based cbShips[]
    snum = sship->snum - 1;


    if (snum < 0 || snum >= 20 /*MAXSHIPS*/)
        return FALSE;

#if defined(DEBUG_CLIENTPROC)
    utLog("PROC SHIP: snum = %d", snum);
#endif

    cbShips[snum].status = sship->status;
    cbShips[snum].team = sship->team;
    cbShips[snum].unum = ntohs(sship->unum);
    cbShips[snum].shiptype = sship->shiptype;
    cbShips[snum].towing = sship->towing;
    cbShips[snum].towedby = sship->towedby;

    for (i=0; i<4 /*NUMPLAYERTEAMS*/; i++)
        if (sship->war & (1 << i))
            cbShips[snum].war[i] = true;
        else
            cbShips[snum].war[i] = false;

    for (i=0; i<4 /*NUMPLAYERTEAMS*/; i++)
        if (sship->rwar & (1 << i))
            cbShips[snum].rwar[i] = true;
        else
            cbShips[snum].rwar[i] = false;

    // Don't bother decoding properly for this - not necessary when
    // just playing back recordings...
    //    cbShips[snum].killedby = (int)((int16_t)ntohs(sship->killedby));
    cbShips[snum].kills = (real)((real)ntohl(sship->kills) / 10.0);

    for (i=0; i<(40 + 20)/*MAXPLANETS*/; i++)
        cbShips[snum].srpwar[i] = (bool)sship->srpwar[i+1];

    for (i=0; i<4 /*NUMPLAYERTEAMS*/; i++)
        cbShips[snum].scanned[i] = (int)sship->scanned[i];

    sship->alias[24 /*MAXUSERALIAS*/ - 1] = 0;
    utStrncpy(cbShips[snum].alias, (char *)sship->alias, 24 /*MAXUSERALIAS*/);

    return TRUE;
}

int proc_0006_ShipSml(char *buf)
{
    int snum;
    sp_0006_ShipSml_t *sshipsml = (sp_0006_ShipSml_t *)buf;

    if (!pktIsValid(SP_0006_SHIPSML, buf))
        return FALSE;

    // compensate for 0-based cbShips[]
    snum = sshipsml->snum - 1;

    if (snum < 0 || snum >= 20 /*MAXSHIPS*/)
        return FALSE;

#if defined(DEBUG_CLIENTPROC)
    utLog("PROC SHIPSML: snum = %d", snum);
#endif

    /* we need to mask out map since it's always local */
    cbShips[snum].flags = ((((uint16_t)ntohs(sshipsml->flags)) & ~0x0040 /*SHIP_F_MAP*/) | SMAP(snum));

    cbShips[snum].action = sshipsml->action;
    cbShips[snum].shields = sshipsml->shields;
    cbShips[snum].damage = sshipsml->damage;
    cbShips[snum].armies = sshipsml->armies;
    cbShips[snum].sdfuse = (int)((int16_t)ntohs(sshipsml->sdfuse));

    cbShips[snum].wfuse = (int)sshipsml->wfuse;
    cbShips[snum].efuse = (int)sshipsml->efuse;

    cbShips[snum].weapalloc = sshipsml->walloc;
    cbShips[snum].engalloc = 100 - cbShips[snum].weapalloc;

    cbShips[snum].pfuse = (int)sshipsml->pfuse;

    cbShips[snum].etemp = (real)sshipsml->etemp;
    cbShips[snum].wtemp = (real)sshipsml->wtemp;
    cbShips[snum].fuel = (real)((uint16_t)ntohs(sshipsml->fuel));

    // fixups for the current CB/protocol:
    // if lock == 0, no lock, if < 0 planet, if > 0 ship
    // we do not support ship lockons, especially in this version, so
    // ignore.
    int temp = (int)((int16_t)ntohs(sshipsml->lock));

    if (temp == 0)
    {
        cbShips[snum].lock = LOCK_NONE;
        cbShips[snum].lockDetail = 0;
    }
    else if (temp < 0) // planet
    {
        cbShips[snum].lock = LOCK_PLANET;
        // FIXME adjust planet number when ready
        cbShips[snum].lockDetail = -temp;
        // Compensate for 0-based cbPlanets[]
        cbShips[snum].lockDetail--;
    }
    cbShips[snum].lastphase =
        (real)((uint16_t)ntohs(sshipsml->lastphase)) / 100.0;
    cbShips[snum].lastblast =
        (real)((uint16_t)ntohs(sshipsml->lastblast)) / 100.0;

    return TRUE;
}

int proc_0006_ShipLoc(char *buf)
{
    int snum;
    sp_0006_ShipLoc_t *sshiploc = (sp_0006_ShipLoc_t *)buf;

    if (!pktIsValid(SP_0006_SHIPLOC, buf))
        return FALSE;

    // compensate for 0-based cbShips[]
    snum = sshiploc->snum - 1;

    if (snum < 0 || snum >= 20 /*MAXSHIPS*/)
        return FALSE;

#if defined(DEBUG_CLIENTPROC)
    utLog("PROC SHIPLOC: snum = %d", snum);
#endif

    cbShips[snum].head = (real)((real)ntohs(sshiploc->head) / 10.0);
    cbShips[snum].warp = (real)((real)sshiploc->warp / 10.0);

    cbShips[snum].x = (real)((real)((int32_t)ntohl(sshiploc->x)) / 1000.0);
    cbShips[snum].y = (real)((real)((int32_t)ntohl(sshiploc->y)) / 1000.0);

    return TRUE;
}

int proc_0006_Planet(char *buf)
{
    sp_0006_Planet_t *splan = (sp_0006_Planet_t *)buf;
    int pnum;

    if (!pktIsValid(SP_0006_PLANET, buf))
        return FALSE;

    // compensate for 0-based cbPlanets[]
    pnum = splan->pnum - 1;

    if (pnum < 0 || pnum >= (40 + 20) /*MAXPLANETS*/)
        return FALSE;

    cbPlanets[pnum].type = splan->ptype;
    cbPlanets[pnum].team = splan->team;

    utStrncpy(cbPlanets[pnum].name, (char *)splan->name, 12 /*MAXPLANETNAME*/);

    uiUpdatePlanet(pnum);

    return TRUE;
}

int proc_0006_PlanetSml(char *buf)
{
    int i;
    sp_0006_PlanetSml_t *splansml = (sp_0006_PlanetSml_t *)buf;
    int pnum;

    if (!pktIsValid(SP_0006_PLANETSML, buf))
        return FALSE;

    // compensate for 0-based cbPlanets[]
    pnum = splansml->pnum - 1;

    if (pnum < 0 || pnum >= (40 + 20) /*MAXPLANETS*/)
        return FALSE;

    for (i=0; i<4 /*NUMPLAYERTEAMS*/; i++)
        if (splansml->scanned & (1 << i))
            cbPlanets[pnum].scanned[i] = true;
        else
            cbPlanets[pnum].scanned[i] = false;

    cbPlanets[pnum].uninhabtime = (int)splansml->uninhabtime;

    return TRUE;
}

int proc_0006_PlanetLoc(char *buf)
{
    sp_0006_PlanetLoc_t *splanloc = (sp_0006_PlanetLoc_t *)buf;
    int pnum;

    if (!pktIsValid(SP_0006_PLANETLOC, buf))
        return FALSE;

    // compensate for 0-based cbPlanets[]
    pnum = splanloc->pnum - 1;

    if (pnum < 0 || pnum >= (40 + 20) /*MAXPLANETS*/)
        return FALSE;

    cbPlanets[pnum].armies = (int)((int16_t)ntohs(splanloc->armies));
    cbPlanets[pnum].x = (real)((real)((int32_t)ntohl(splanloc->x)) / 1000.0);
    cbPlanets[pnum].y = (real)((real)((int32_t)ntohl(splanloc->y)) / 1000.0);

    return TRUE;
}

int proc_0006_PlanetLoc2(char *buf)
{
    sp_0006_PlanetLoc2_t *splanloc2 = (sp_0006_PlanetLoc2_t *)buf;
    int pnum;

    if (!pktIsValid(SP_0006_PLANETLOC2, buf))
        return FALSE;

    // compensate for 0-based cbPlanets[]
    pnum = splanloc2->pnum - 1;

    if (pnum < 0 || pnum >= 60 /*MAXPLANETS*/)
        return FALSE;

    cbPlanets[pnum].armies = (int)((int16_t)ntohs(splanloc2->armies));
    cbPlanets[pnum].x = (real)((real)((int32_t)ntohl(splanloc2->x)) / 1000.0);
    cbPlanets[pnum].y = (real)((real)((int32_t)ntohl(splanloc2->y)) / 1000.0);
    cbPlanets[pnum].orbang = (real)ntohs(splanloc2->orbang) / 100.0;

    return TRUE;
}

int proc_0006_PlanetInfo(char *buf)
{
    sp_0006_PlanetInfo_t *splaninfo = (sp_0006_PlanetInfo_t *)buf;
    int pnum;
    int primary;

    if (!pktIsValid(SP_0006_PLANETINFO, buf))
        return FALSE;

    // compensate for 0-based cbPlanets[]
    pnum = splaninfo->pnum - 1;

    if (pnum < 0 || pnum >= (40 + 20) /*MAXPLANETS*/)
        return FALSE;

    primary = splaninfo->primary;

    // special case for murisak, which had a primary of 0 (invalid).
    // In the case that we run across a 0 primary, we'll assign it to
    // it's own pnum, indicating stationary
    if (primary == 0)
        primary = pnum; // already compensated
    else
        primary--; // compensate for 0-based cbPlanets[]

    if (primary < 0 || primary >= (40 + 20) /*MAXPLANETS*/)
        return FALSE;

    /* in protocol 6, we 'forgot' planet realness.  To avoid breaking
       protocol again, and allow unpatched clients and/or servers to
       work we check to see if SPPLANETINFO_FLAGS_FVALID is set.  If so,
       _then_ we pay attn to any other flags present. Else we ignore
       them. */

    if (splaninfo->flags & SP_0006_PLANETINFO_FLAGS_FVALID)
    {                           /* we have valid flags */

        if (splaninfo->flags & SP_0006_PLANETINFO_FLAGS_REAL)
            PFSET(pnum, PLAN_F_VISIBLE);
        else
            PFCLR(pnum, PLAN_F_VISIBLE);
    }

    cbPlanets[pnum].primary = primary;
    cbPlanets[pnum].orbrad = (real)((real)((uint32_t)ntohl(splaninfo->orbrad)) / 10.0);
    cbPlanets[pnum].orbvel = (real)((real)((int32_t)ntohl(splaninfo->orbvel)) / 100.0);

    return TRUE;
}


int proc_0006_Torp(char *buf)
{
    int snum, tnum;
    sp_0006_Torp_t *storp = (sp_0006_Torp_t *)buf;

    if (!pktIsValid(SP_0006_TORP, buf))
        return FALSE;

    // compensate for 0-based cbShips[]
    snum = storp->snum - 1;
    tnum = storp->tnum;

    if (snum < 0 || snum >= 20 /*MAXSHIPS*/)
        return FALSE;

    if (tnum < 0 || tnum >= 9 /*MAXTORPS*/)
        return FALSE;

    cbShips[snum].torps[tnum].status = (int)storp->status;

    return TRUE;
}

int proc_0006_TorpLoc(char *buf)
{
    int snum, tnum, i;
    sp_0006_TorpLoc_t *storploc = (sp_0006_TorpLoc_t *)buf;

    if (!pktIsValid(SP_0006_TORPLOC, buf))
        return FALSE;

    // compensate for 0-based cbShips[]
    snum = storploc->snum - 1;
    tnum = storploc->tnum;

    if (snum <= 0 || snum > 20 /*MAXSHIPS*/)
        return FALSE;

    if (tnum < 0 || tnum >= 12 /*MAXTORPS*/)
        return FALSE;

    for (i=0; i<4 /*NUMPLAYERTEAMS*/; i++)
        if (storploc->war & (1 << i))
            cbShips[snum].torps[tnum].war[i] = true;
        else
            cbShips[snum].torps[tnum].war[i] = false;

    cbShips[snum].torps[tnum].x = (real)((real)((int32_t)ntohl(storploc->x)) / 1000.0);
    cbShips[snum].torps[tnum].y = (real)((real)((int32_t)ntohl(storploc->y)) / 1000.0);

    return TRUE;
}


int proc_0006_TorpEvent(char *buf)
{
    int snum, tnum, i;
    sp_0006_TorpEvent_t *storpev = (sp_0006_TorpEvent_t *)buf;

    if (!pktIsValid(SP_0006_TORPEVENT, buf))
        return FALSE;

    // compensate for 0-based cbShips[]
    snum = storpev->snum - 1;
    tnum = storpev->tnum;

    if (snum < 0 || snum >= 20 /*MAXSHIPS*/)
        return FALSE;

    if (tnum < 0 || tnum >= 9 /*MAXTORPS*/)
        return FALSE;

    cbShips[snum].torps[tnum].status = (int)storpev->status;

    for (i=0; i<4 /*NUMPLAYERTEAMS*/; i++)
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

    return TRUE;
}


int proc_0006_Message(char *buf)
{
    sp_0006_Message_t *smsg = (sp_0006_Message_t *)buf;

    if (!pktIsValid(SP_0006_MESSAGE, buf))
        return FALSE;


    smsg->msg[70 /*MESSAGE_SIZE*/ - 1] = 0;

    msgTo_t   realTo;
    uint16_t  realToDetail;
    msgFrom_t realFrom;
    uint16_t  realFromDetail;

    smsg->from = (int)((int16_t)ntohs(smsg->from));
    smsg->to = (int)((int16_t)ntohs(smsg->to));

    // From - convert to new order
    /* >0: ship the message is from */
    /* =0: from GOD */
    /* <0: planet with intruder alert */
    if (smsg->from > 0) // ship
    {
        realFrom = MSG_FROM_SHIP;
        // compensate for 0-based cbShips[]
        realFromDetail = (uint16_t)smsg->from - 1;
    }
    else if (smsg->from == 0) // god
    {
        realFrom = MSG_FROM_GOD;
        realFromDetail = 0;
    }
    else if (smsg->from < 0) // planet
    {
        // here, it's from a planet or one of the old MSG_* defines
        // try planets first

        if (-smsg->from >= 1 && -smsg->from <= 60)
        {
            realFrom = MSG_FROM_PLANET;
            // compensate for 0-bsed cbPlanets[]
            realFromDetail = (uint16_t)(smsg->from * -1) - 1;
        }
        else if (smsg->from == -102 /*MSG_GOD*/)
        {
            realFrom = MSG_FROM_GOD;
            realFromDetail = 0;
        }
        else if (smsg->from == -103 /*MSG_DOOM*/)
        {
            realFrom = MSG_FROM_DOOM;
            realFromDetail = 0;
        }
        else if (smsg->from == -106 /*MSG_COMP*/)
        {
            realFrom = MSG_FROM_COMP;
            realFromDetail = 0;
        }
    }
    else
    {
        // don't know what it is.  log it and ignore it
        utLog("%s: unrecognized from: (%d)", __FUNCTION__, smsg->from);
        return TRUE;
    }

    // To - convert to new order
    /* >0: ship the message is to */
    /* <=0: team the message is to */

    if (smsg->to > 0) // ship
    {
        realTo = MSG_TO_SHIP;
        realToDetail = (uint16_t)smsg->to;
        realToDetail--; // compensate for 0-based MAXSHIPS
    }
    else if (smsg->to <= 0) // team
    {
        // here, it's from a team, or one of the special MSG_* in
        // older versions proto/cb. Try teams first.

        if (-smsg->to >= 0 && -smsg->to < 4 /*NUMPLAYERTEAMS*/)
        {
            realTo = MSG_TO_TEAM;
            realToDetail = (uint16_t)(smsg->to * -1);
        }
        else if (smsg->to == -100 /*MSG_ALL*/)
        {
            realTo = MSG_TO_ALL;
            realToDetail = 0;
        }
        else if (smsg->to == -102 /*MSG_GOD*/)
        {
            realTo = MSG_TO_GOD;
            realToDetail = 0;
        }
        else if (smsg->to == -105 /*MSG_IMPLEMENTORS*/)
        {
            realTo = MSG_TO_IMPLEMENTORS;
            realToDetail = 0;
        }
        else if (smsg->to == -107 /*MSG_FRIENDLY*/)
        {
            realTo = MSG_TO_FRIENDLY;
            realToDetail = 0;
        }
    }
    else
    {
        // don't know what it is.  log it and ignore it
        utLog("%s: unrecognized to: (%d)", __FUNCTION__, smsg->to);
        return TRUE;
    }

    /* special handling when playing back a recording */
    /* if we aren't interested in robot msgs, skip it */
    if (!(smsg->flags & MSG_FLAGS_ROBOT) ||
        ((smsg->flags & MSG_FLAGS_ROBOT) && !UserConf.NoRobotMsgs))
    {
        memset((void *)&recMsg, 0, sizeof(Msg_t));
        utStrncpy(recMsg.msgbuf, (char *)smsg->msg, 70 /*MESSAGE_SIZE*/);
        recMsg.from = realFrom;
        recMsg.fromDetail = realFromDetail;
        recMsg.to = realTo;
        recMsg.toDetail = realToDetail;
        recMsg.flags = smsg->flags;
    }
    return TRUE;
}

int proc_0006_Team(char *buf)
{
    int team, i;
    sp_0006_Team_t *steam = (sp_0006_Team_t *)buf;

    if (!pktIsValid(SP_0006_TEAM, buf))
        return FALSE;

    team = steam->team;

    if (team < 0 || team >= 8 /*NUMALLTEAMS*/)
        return FALSE;

    // this doesn't exist in current cb
    // cbTeams[team].homesun = steam->homesun;

    if (steam->flags & SP_0006_TEAM_FLAGS_COUPINFO)
        cbTeams[team].coupinfo = true;
    else
        cbTeams[team].coupinfo = false;

    cbTeams[team].couptime = steam->couptime;

    // this doesn't exist in current cb
    //    for (i=0; i<3; i++)
    //        cbTeams[team].teamhplanets[i] = steam->teamhplanets[i];

    cbTeams[team].homeplanet = (int)steam->homeplanet;

    for (i=0; i<20 /*MAXTSTATS*/; i++)
        cbTeams[team].stats[i] = (int)ntohl(steam->stats[i]);

    utStrncpy(cbTeams[team].name, (char *)steam->name, 12 /*MAXTEAMNAME*/);

    return TRUE;
}

int proc_0006_ServerStat(char *buf)
{
    // no need to do anything here...
    return TRUE;
}

int proc_0006_cbConqInfo(char *buf)
{
    // no need to do anything here...
    return TRUE;
}

int proc_0006_History(char *buf)
{
    // There's nothing to do here...
    return TRUE;
}

int proc_0006_Doomsday(char *buf)
{
    sp_0006_Doomsday_t *dd = (sp_0006_Doomsday_t *)buf;

    if (!pktIsValid(SP_0006_DOOMSDAY, buf))
        return FALSE;

    cbDoomsday->status = dd->status;
    cbDoomsday->heading =(real)((real)ntohs(dd->heading) / 10.0);
    cbDoomsday->x = (real)((real)((int32_t)ntohl(dd->x)) / 1000.0);
    cbDoomsday->y = (real)((real)((int32_t)ntohl(dd->y)) / 1000.0);

    return TRUE;
}

int proc_0006_Ack(char *buf)
{
    // There's nothing to do here...
    return TRUE;
}

int proc_0006_ClientStat(char *buf)
{
    // There's nothing to do here...
    return TRUE;
}

int proc_0006_Frame(char *buf)
{
    sp_0006_Frame_t *frame;

    if (pktIsValid(SP_0006_FRAME, buf))
    {
        frame = (sp_0006_Frame_t *)buf;
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

        return TRUE;
    }

    return FALSE;
}

