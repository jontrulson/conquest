/* 
 * recordclnt.c - client playback
 *
 * $Id$
 *
 * Copyright 1999-2004 Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 */

#include "c_defs.h"
#include "global.h"
#include "conqdef.h"
#include "conqcom.h"
#include "context.h"
#include "conf.h"
#include "global.h"
#include "color.h"
#include "datatypes.h"
#include "protocol.h"
#include "packet.h"

#include "protocol.h"
#include "client.h"
#include "record.h"

#define NOEXTERN_PLAYBACK
#include "playback.h"
#undef NOEXTERN_PLAYBACK

/* read in a header/data packet pair, and add them to our cmb.  return
   the packet type processed or RDATA_NONE if there is no more data or
   other error. */
int pbProcessPackets(void)
{
  char buf[PKT_MAXSIZE];
  spFrame_t *frame;
  int pkttype;
  spMessage_t *smsg;

#if defined(DEBUG_REC)
  clog("conqreply: processPacket ENTER");
#endif

  if ((pkttype = recordReadPkt(buf, PKT_MAXSIZE)) != SP_NULL)
    {
      switch(pkttype)
        {
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
        case SP_TORPEVENT:
          procTorpEvent(buf);
          break;
        case SP_TORP:
          procTorp(buf);
          break;
        case SP_TORPLOC:
          procTorpLoc(buf);
          break;
        case SP_TEAM:
          procTeam(buf);
          break;
        case SP_MESSAGE:
          smsg = (spMessage_t *)buf;
          /* if we aren't interested in robot msgs, skip it */
          if (!(smsg->flags & MSG_FLAGS_ROBOT) ||
              ((smsg->flags & MSG_FLAGS_ROBOT) && !UserConf.NoRobotMsgs))
            {
              memset((void *)&recMsg, 0, sizeof(Msg_t));
              strncpy(recMsg.msgbuf, (char *)smsg->msg, MESSAGE_SIZE);
              recMsg.msgfrom = (int)((Sgn16)ntohs(smsg->from));
              recMsg.msgto = (int)((Sgn16)ntohs(smsg->to));
              recMsg.flags = smsg->flags;
            }

          break;

        case SP_FRAME:
          frame = (spFrame_t *)buf;
          /* endian correction*/
          frame->time = (Unsgn32)ntohl(frame->time);
          frame->frame = (Unsgn32)ntohl(frame->frame);

          if (startTime == (time_t)0)
            startTime = (time_t)frame->time;
          currTime = (time_t)frame->time;

          frameCount = (Unsgn32)frame->frame;

          break;
          
        case SP_DOOMSDAY:
          procDoomsday(buf);
          break;

        default:
#ifdef DEBUG_REC
          fprintf(stderr, "processPacket: Invalid rtype %d\n", pkttype);
#endif
          break;          
        }
    }

  return pkttype;
}


/* seek around in a game.  backwards seeks will be slooow... */
void pbFileSeek(time_t newtime)
{
  if (newtime == currTime)
    return;			/* simple case */

  if (newtime < currTime)
    {				/* backward */
      /* we have to reset everything and start from scratch. */

      recordCloseInput();

      if (!initReplay(rfname, NULL))
	return;			/* bummer */
      
      currTime = startTime;
    }

  /* start searching */

  /* just read packets until 1. currTime exceeds newtime, or 2. no
     data is left */
  Context.display = FALSE; /* don't display things while looking */
  
  while (currTime < newtime)
    if ((pbProcessPackets() == SP_NULL))
      break;		/* no more data */
  
  Context.display = TRUE;

  return;
}
	  
/* read and process packets until a FRAME packet or EOD is found */
int pbProcessIter(void)
{
  int rtype;

  while(((rtype = pbProcessPackets()) != SP_NULL) && rtype != SP_FRAME)
    ;

  return(rtype);
}

/* This function accepts a playback speed, checks the limits, and sets
 * the frameDelay appropriately.
 */
void pbSetPlaybackSpeed(int speed, int samplerate)
{
  /* first check the limits and adjust accordingly */

  if (speed > PB_SPEED_INFINITE)
    speed = PB_SPEED_INFINITE;

  if (speed < -PB_SPEED_MAX_TIMES)
    speed = -PB_SPEED_MAX_TIMES;

  /* now we need to check for and 'skip' speeds of 0 and -1 */

  /* going slower (can only get there from 1x incrementally) */
  if (speed == 0)
    speed = -2;

  /* going faster (can only get there from -2x incrementally) */
  if (speed == -1)
    speed = 1;
    
  /* save the new speed */
  pbSpeed = speed;

  /* now compute the new frameDelay */
  
  if (pbSpeed == PB_SPEED_INFINITE)
    frameDelay = 0.0;
  else if (pbSpeed > 0)
    frameDelay = (1.0 / (real)samplerate) / (real)pbSpeed;
  else if (pbSpeed < 0)
    frameDelay = -(real)pbSpeed * (1.0 / (real)samplerate);

  return;
}
