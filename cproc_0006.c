#include "c_defs.h"

/************************************************************************
 *
 * client packet processing (from server - SP_*)
 *
 * Copyright Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
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

    Users[unum].team = suser->team;

    if (suser->flags & SP_0006_USER_FLAGS_LIVE)
        Users[unum].live = TRUE;

    for (i=0; i<NUMPLAYERTEAMS; i++)
        if ((suser->war & (1 << i)))
            Users[unum].war[i] = TRUE;

    Users[unum].rating = (real)((real)((int16_t)ntohs(suser->rating)) / 100.0);
    Users[unum].lastentry = (time_t)ntohl(suser->lastentry);

    for (i=0; i<OOPT_TOTALOOPTION; i++)
        Users[unum].ooptions[i] = (int32_t)ntohl(suser->ooptions[i]);

    for (i=0; i<USTAT_TOTALSTATS; i++)
        Users[unum].stats[i] = (int32_t)ntohl(suser->stats[i]);

    strncpy(Users[unum].username, (char *)suser->username, 32 /*MAXUSERNAME*/ - 1);
    strncpy(Users[unum].alias, (char *)suser->alias, 24 /*MAXUSERPNAME*/ - 1);

#if defined(DEBUG_CLIENTPROC)
    utLog("\t%s: name: %s (%s)", __FUNCTION__, Users[unum].username, Users[unum].alias);
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

    snum = sship->snum;
// FIXME/CHECKME - is this (MAXSHIPS) logic orrect for the new world
// order?  It will be in a new common block with 0-based MAXSHIP (and
// eventually planets)...
    if (snum <= 0 || snum > 20 /*MAXSHIPS*/)
        return FALSE;

#if defined(DEBUG_CLIENTPROC)
    utLog("PROC SHIP: snum = %d", snum);
#endif

    Ships[snum].status = sship->status;
    Ships[snum].team = sship->team;
    Ships[snum].unum = ntohs(sship->unum);
    Ships[snum].shiptype = sship->shiptype;
    Ships[snum].towing = sship->towing;
    Ships[snum].towedby = sship->towedby;

    for (i=0; i<4 /*NUMPLAYERTEAMS*/; i++)
        if (sship->war & (1 << i))
            Ships[snum].war[i] = TRUE;
        else
            Ships[snum].war[i] = FALSE;

    for (i=0; i<4 /*NUMPLAYERTEAMS*/; i++)
        if (sship->rwar & (1 << i))
            Ships[snum].rwar[i] = TRUE;
        else
            Ships[snum].rwar[i] = FALSE;

    // Don't bother decoding properly for this - not necessary when
    // just playing back recordings...
    //    Ships[snum].killedby = (int)((int16_t)ntohs(sship->killedby));
    Ships[snum].kills = (real)((real)ntohl(sship->kills) / 10.0);

    // FIXME - check logic here... This will be broken I think.
    for (i=0; i<=(40 + 20)/*MAXPLANETS*/; i++)
        Ships[snum].srpwar[i] = (int)sship->srpwar[i];

    for (i=0; i<4 /*NUMPLAYERTEAMS*/; i++)
        Ships[snum].scanned[i] = (int)sship->scanned[i];

    sship->alias[24 /*MAXUSERPNAME*/ - 1] = 0;
    strncpy(Ships[snum].alias, (char *)sship->alias, 24 /*MAXUSERPNAME*/ - 1);

    return TRUE;
}

int proc_0006_ShipSml(char *buf)
{
    int snum;
    sp_0006_ShipSml_t *sshipsml = (sp_0006_ShipSml_t *)buf;

    if (!pktIsValid(SP_0006_SHIPSML, buf))
        return FALSE;

    snum = sshipsml->snum;

    if (snum <= 0 || snum > 20 /*MAXSHIPS*/)
        return FALSE;

#if defined(DEBUG_CLIENTPROC)
    utLog("PROC SHIPSML: snum = %d", snum);
#endif

    /* we need to mask out map since it's always local */
    Ships[snum].flags = ((((uint16_t)ntohs(sshipsml->flags)) & ~0x0040 /*SHIP_F_MAP*/) | SMAP(snum));

    Ships[snum].action = sshipsml->action;
    Ships[snum].shields = sshipsml->shields;
    Ships[snum].damage = sshipsml->damage;
    Ships[snum].armies = sshipsml->armies;
    Ships[snum].sdfuse = (int)((int16_t)ntohs(sshipsml->sdfuse));

    Ships[snum].wfuse = (int)sshipsml->wfuse;
    Ships[snum].efuse = (int)sshipsml->efuse;

    Ships[snum].weapalloc = sshipsml->walloc;
    Ships[snum].engalloc = 100 - Ships[snum].weapalloc;

    Ships[snum].pfuse = (int)sshipsml->pfuse;

    Ships[snum].etemp = (real)sshipsml->etemp;
    Ships[snum].wtemp = (real)sshipsml->wtemp;
    Ships[snum].fuel = (real)((uint16_t)ntohs(sshipsml->fuel));

    // fixups for the current CB/protocol:
    // if lock == 0, no lock, if < 0 planet, if > 0 ship
    // we do not support ship lockons, especially in this version, so
    // ignore.
    int temp = (int)((int16_t)ntohs(sshipsml->lock));

    if (temp == 0)
    {
        Ships[snum].lock = LOCK_NONE;
        Ships[snum].lockDetail = 0;
    }
    else if (temp < 0) // planet
    {
        Ships[snum].lock = LOCK_PLANET;
        // FIXME adjust planet number when ready
        Ships[snum].lockDetail = -temp;
    }
    Ships[snum].lastphase = (real)((uint16_t)ntohs(sshipsml->lastphase)) / 100.0;
    Ships[snum].lastblast = (real)((uint16_t)ntohs(sshipsml->lastblast)) / 100.0;

    return TRUE;
}

int proc_0006_ShipLoc(char *buf)
{
    int snum;
    sp_0006_ShipLoc_t *sshiploc = (sp_0006_ShipLoc_t *)buf;

    if (!pktIsValid(SP_0006_SHIPLOC, buf))
        return FALSE;

    snum = sshiploc->snum;

    if (snum <= 0 || snum > 20 /*MAXSHIPS*/)
        return FALSE;

#if defined(DEBUG_CLIENTPROC)
    utLog("PROC SHIPLOC: snum = %d", snum);
#endif

    Ships[snum].head = (real)((real)ntohs(sshiploc->head) / 10.0);
    Ships[snum].warp = (real)((real)sshiploc->warp / 10.0);

    Ships[snum].x = (real)((real)((int32_t)ntohl(sshiploc->x)) / 1000.0);
    Ships[snum].y = (real)((real)((int32_t)ntohl(sshiploc->y)) / 1000.0);

    return TRUE;
}

int proc_0006_Planet(char *buf)
{
    sp_0006_Planet_t *splan = (sp_0006_Planet_t *)buf;
    int pnum;

    if (!pktIsValid(SP_0006_PLANET, buf))
        return FALSE;

    pnum = splan->pnum;

    if (pnum <= 0 || pnum > (40 + 20) /*MAXPLANETS*/)
        return FALSE;

    Planets[pnum].type = splan->ptype;
    Planets[pnum].team = splan->team;

    splan->name[12 /*MAXPLANETNAME*/ - 1] = 0;
    strncpy(Planets[pnum].name, (char *)splan->name, 12 /*MAXPLANETNAME*/);

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

    pnum = splansml->pnum;

    if (pnum <= 0 || pnum > (40 + 20) /*MAXPLANETS*/)
        return FALSE;

    for (i=0; i<4 /*NUMPLAYERTEAMS*/; i++)
        if (splansml->scanned & (1 << i))
            Planets[pnum].scanned[i] = TRUE;
        else
            Planets[pnum].scanned[i] = FALSE;

    Planets[pnum].uninhabtime = (int)splansml->uninhabtime;

    return TRUE;
}

int proc_0006_PlanetLoc(char *buf)
{
    sp_0006_PlanetLoc_t *splanloc = (sp_0006_PlanetLoc_t *)buf;
    int pnum;

    if (!pktIsValid(SP_0006_PLANETLOC, buf))
        return FALSE;

    pnum = splanloc->pnum;

    if (pnum <= 0 || pnum > (40 + 20) /*MAXPLANETS*/)
        return FALSE;

    Planets[pnum].armies = (int)((int16_t)ntohs(splanloc->armies));
    Planets[pnum].x = (real)((real)((int32_t)ntohl(splanloc->x)) / 1000.0);
    Planets[pnum].y = (real)((real)((int32_t)ntohl(splanloc->y)) / 1000.0);

    return TRUE;
}

int proc_0006_PlanetLoc2(char *buf)
{
    sp_0006_PlanetLoc2_t *splanloc2 = (sp_0006_PlanetLoc2_t *)buf;
    int pnum;

    if (!pktIsValid(SP_0006_PLANETLOC2, buf))
        return FALSE;

    pnum = splanloc2->pnum;

    if (pnum <= 0 || pnum > 60 /*MAXPLANETS*/)
        return FALSE;

    Planets[pnum].armies = (int)((int16_t)ntohs(splanloc2->armies));
    Planets[pnum].x = (real)((real)((int32_t)ntohl(splanloc2->x)) / 1000.0);
    Planets[pnum].y = (real)((real)((int32_t)ntohl(splanloc2->y)) / 1000.0);
    Planets[pnum].orbang = (real)((real)((uint16_t)ntohs(splanloc2->orbang)) / 100.0);

    return TRUE;
}

int proc_0006_PlanetInfo(char *buf)
{
    sp_0006_PlanetInfo_t *splaninfo = (sp_0006_PlanetInfo_t *)buf;
    int pnum;
    int primary;

    if (!pktIsValid(SP_0006_PLANETINFO, buf))
        return FALSE;

    pnum = splaninfo->pnum;

    if (pnum <= 0 || pnum > (40 + 20) /*MAXPLANETS*/)
        return FALSE;

    primary = splaninfo->primary;

    /* Roy fix - 10/17/2005 - let mur data be sent. */
    if (primary < 0 || primary > (40 + 20) /*MAXPLANETS*/)
        return FALSE;

    /* in protocol 6, we 'forgot' planet realness.  To avoid breaking
       protocol again, and allow unpatched clients and/or servers to
       work we check to see if SPPLANETINFO_FLAGS_FVALID is set.  If so,
       _then_ we pay attn to any other flags present. Else we ignore
       them. */

    /* FIXME - with new CB, and later, new protocol, we will send these
       flags directly.  Right? */
    if (splaninfo->flags & SP_0006_PLANETINFO_FLAGS_FVALID)
    {                           /* we have valid flags */

        if (splaninfo->flags & SP_0006_PLANETINFO_FLAGS_REAL)
            PFSET(pnum, PLAN_F_VISIBLE);
        else
            PFCLR(pnum, PLAN_F_VISIBLE);
    }

    Planets[pnum].primary = primary;
    Planets[pnum].orbrad = (real)((real)((uint32_t)ntohl(splaninfo->orbrad)) / 10.0);
    Planets[pnum].orbvel = (real)((real)((int32_t)ntohl(splaninfo->orbvel)) / 100.0);

    return TRUE;
}


int proc_0006_Torp(char *buf)
{
    int snum, tnum;
    sp_0006_Torp_t *storp = (sp_0006_Torp_t *)buf;

    if (!pktIsValid(SP_0006_TORP, buf))
        return FALSE;

    snum = storp->snum;
    tnum = storp->tnum;

    if (snum <= 0 || snum > 20 /*MAXSHIPS*/)
        return FALSE;

    if (tnum < 0 || tnum >= 9 /*MAXTORPS*/)
        return FALSE;

    Ships[snum].torps[tnum].status = (int)storp->status;

    return TRUE;
}

int proc_0006_TorpLoc(char *buf)
{
    int snum, tnum, i;
    sp_0006_TorpLoc_t *storploc = (sp_0006_TorpLoc_t *)buf;

    if (!pktIsValid(SP_0006_TORPLOC, buf))
        return FALSE;

    snum = storploc->snum;
    tnum = storploc->tnum;

    if (snum <= 0 || snum > 20 /*MAXSHIPS*/)
        return FALSE;

    if (tnum < 0 || tnum >= 12 /*MAXTORPS*/)
        return FALSE;

    for (i=0; i<4 /*NUMPLAYERTEAMS*/; i++)
        if (storploc->war & (1 << i))
            Ships[snum].torps[tnum].war[i] = TRUE;
        else
            Ships[snum].torps[tnum].war[i] = FALSE;

    Ships[snum].torps[tnum].x = (real)((real)((int32_t)ntohl(storploc->x)) / 1000.0);
    Ships[snum].torps[tnum].y = (real)((real)((int32_t)ntohl(storploc->y)) / 1000.0);

    return TRUE;
}


int proc_0006_TorpEvent(char *buf)
{
    int snum, tnum, i;
    sp_0006_TorpEvent_t *storpev = (sp_0006_TorpEvent_t *)buf;

    if (!pktIsValid(SP_0006_TORPEVENT, buf))
        return FALSE;

    snum = storpev->snum;
    tnum = storpev->tnum;

    if (snum <= 0 || snum > 20 /*MAXSHIPS*/)
        return FALSE;

    if (tnum < 0 || tnum >= 9 /*MAXTORPS*/)
        return FALSE;

    Ships[snum].torps[tnum].status = (int)storpev->status;

    for (i=0; i<4 /*NUMPLAYERTEAMS*/; i++)
        if (storpev->war & (1 << i))
            Ships[snum].torps[tnum].war[i] = TRUE;
        else
            Ships[snum].torps[tnum].war[i] = FALSE;

    Ships[snum].torps[tnum].x =
        (real)((real)((int32_t)ntohl(storpev->x)) / 1000.0);
    Ships[snum].torps[tnum].y =
        (real)((real)((int32_t)ntohl(storpev->y)) / 1000.0);

    Ships[snum].torps[tnum].dx =
        (real)((real)((int32_t)ntohl(storpev->dx)) / 1000.0);
    Ships[snum].torps[tnum].dy =
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
        realFromDetail = (uint16_t)smsg->from;
// FIXME       realFromDetail--; // compensate for 0-based ships[] in current version
    }
    else if (smsg->from == 0) // god
    {
        realFrom = MSG_FROM_GOD;
        realFromDetail = 0;
    }
    else if (smsg->from < 0) // planet
    {
        realFrom = MSG_FROM_PLANET;
        realFromDetail = (uint16_t)(smsg->from * -1);
        // FIXME, compensate when MAXPLANETS work done
        // realFromDetail--;
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
        realTo = MSG_TO_TEAM;
        realToDetail = (uint16_t)(smsg->to * -1);
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
        strncpy(recMsg.msgbuf, (char *)smsg->msg, 70 /*MESSAGE_SIZE*/);
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

    Teams[team].homesun = steam->homesun;

    if (steam->flags & SP_0006_TEAM_FLAGS_COUPINFO)
        Teams[team].coupinfo = TRUE;
    else
        Teams[team].coupinfo = FALSE;

    Teams[team].couptime = steam->couptime;

    for (i=0; i<3; i++)
        Teams[team].teamhplanets[i] = steam->teamhplanets[i];

    Teams[team].homeplanet = steam->homeplanet;

    for (i=0; i<20 /*MAXTSTATS*/; i++)
        Teams[team].stats[i] = (int)ntohl(steam->stats[i]);

    strncpy(Teams[team].name, (char *)steam->name, 12 /*MAXTEAMNAME*/ - 1);

    return TRUE;
}

int proc_0006_ServerStat(char *buf)
{
    // no need to do anything here...
    return TRUE;
}

int proc_0006_ConqInfo(char *buf)
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

    Doomsday->status = dd->status;
    Doomsday->heading =(real)((real)ntohs(dd->heading) / 10.0);
    Doomsday->x = (real)((real)((int32_t)ntohl(dd->x)) / 1000.0);
    Doomsday->y = (real)((real)((int32_t)ntohl(dd->y)) / 1000.0);

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

