/* 
 * node
 *
 * Copyright Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 */

#include "c_defs.h"
#include "context.h"
#include "global.h"

#include "color.h"
#include "conf.h"
#include "gldisplay.h"
#include "client.h"
#include "node.h"
#include "ship.h"
#include "prm.h"
#include "conqcom.h"
#include "conqlb.h"
#include "conqutil.h"
#include "nDead.h"
#include "nMenu.h"
#include "nPlay.h"
#include "cqkeys.h"
#include "cqsound.h"
#include "glmisc.h"

static int snum;
static int kb;                  /* killed by... */
string ywkb="You were killed by ";
static char buf[128], junk[128], cbuf[MID_BUFFER_SIZE];
static char lastwords[MAXLASTWORDS];
static Ship_t eShip = {}; /* copy of killers ship, if killed by ship */

#define S_PRESSANY  0           /* press any key... */
#define S_LASTWORDS 1           /* you won, get conquer msg */
#define S_LASTWCONF 2           /* confirm last words */
static int state;

static prm_t prm;

static int nDeadDisplay(dspConfig_t *);
static int nDeadIdle(void);
static int nDeadInput(int ch);

static scrNode_t nDeadNode = {
  nDeadDisplay,               /* display */
  nDeadIdle,                  /* idle */
  nDeadInput,                 /* input */
  NULL,                       /* minput */
  NULL                        /* animQue */
};


void nDeadInit(void)
{
  state = S_PRESSANY;
  snum = Context.snum;

  /* If something is wrong, don't do anything. */
  if ( snum < 1 || snum > MAXSHIPS )
    {
      utLog("nDead: nDeadInit: snum < 1 || snum > MAXSHIPS (%d)", snum);
      nMenuInit();
    }
  
  kb = Ships[snum].killedby;

  if (kb >= 1 && kb <= MAXSHIPS)
    eShip = Ships[kb];        /* get copy of killers ship */
  else
    memset((void *)&eShip, 0, sizeof(Ship_t));

  if (clientFlags & SPCLNTSTAT_FLAG_CONQUER)
    {
      state = S_LASTWORDS;

      prm.preinit = FALSE;
      prm.buf = lastwords;
      prm.buflen = MAXLASTWORDS - 1;
      prm.terms = TERMS;
      prm.buf[0] = 0;
    }


  setONode(&nDeadNode);

  return;
}


static int nDeadDisplay(dspConfig_t *dsp)
{
  int i;

  /* Figure out why we died. */
  switch ( kb )
    {
    case KB_SELF:
      cprintf(8,0,ALIGN_CENTER,"#%d#%s", RedLevelColor, 
              "You scuttled yourself.");
      
      break;
    case KB_NEGENB:
      cprintf(8,0,ALIGN_CENTER,"#%d#%s", RedLevelColor, 
              "You were destroyed by the negative energy barrier.");
      
      break;
    case KB_CONQUER:
      cprintf(8,0,ALIGN_CENTER,"#%d#%s", RedLevelColor, 
              "Y O U   C O N Q U E R E D   T H E   U N I V E R S E ! ! !");
      break;
    case KB_NEWGAME:
      cprintf(8,0,ALIGN_CENTER,"#%d#%s", RedLevelColor, 
              "N E W   G A M E !");
      break;
    case KB_EVICT:
      cprintf(8,0,ALIGN_CENTER,"#%d#%s", RedLevelColor, 
              "Closed for repairs.");
      break;
    case KB_SHIT:
      cprintf(8,0,ALIGN_CENTER,"#%d#%s", RedLevelColor, 
              "You are no longer allowed to play.");
      break;
    case KB_GOD:
      cprintf(8,0,ALIGN_CENTER,"#%d#%s", RedLevelColor, 
              "You were killed by an act of GOD.");
      
      break;
    case KB_DOOMSDAY:
      cprintf(8,0,ALIGN_CENTER,"#%d#%s", RedLevelColor, 
              "You were eaten by the doomsday machine.");
      
      break;
    case KB_GOTDOOMSDAY:
      cprintf(8,0,ALIGN_CENTER,"#%d#%s", RedLevelColor, 
              "You destroyed the doomsday machine!");
      break;
    case KB_DEATHSTAR:
      cprintf(8,0,ALIGN_CENTER,"#%d#%s", RedLevelColor, 
              "You were vaporized by the Death Star.");
      
      break;
    case KB_LIGHTNING:
      cprintf(8,0,ALIGN_CENTER,"#%d#%s", RedLevelColor, 
              "You were destroyed by a lightning bolt.");
      
      break;
    default:
      cbuf[0] = 0;
      buf[0] = 0;
      junk[0] = 0;
      if ( kb > 0 && kb <= MAXSHIPS )
	{
	  utAppendShip( kb, cbuf );
	  if ( eShip.status != SS_LIVE )
	    appstr( ", who also died.", buf );
	  else
	    appchr( '.', buf );
	  cprintf( 8,0,ALIGN_CENTER, 
		   "#%d#You were kill number #%d#%.1f #%d#for #%d#%s #%d#(#%d#%s#%d#)%s",
		   InfoColor, CQC_A_BOLD, eShip.kills, 
		   InfoColor, CQC_A_BOLD, eShip.alias, 
		   InfoColor, CQC_A_BOLD, cbuf, 
		   InfoColor, buf );
	}
      else if ( -kb > 0 && -kb <= NUMPLANETS )
	{
	  if ( Planets[-kb].type == PLANET_SUN )
            strcpy(cbuf, "solar radiation.");
	  else
            strcpy(cbuf, "planetary defenses.");

	  cprintf(8,0,ALIGN_CENTER,"#%d#%s#%d#%s%s#%d#%s", 
                  InfoColor, ywkb, CQC_A_BOLD, Planets[-kb].name, "'s ",
                  InfoColor, cbuf);
	}
      else
	{
	  /* We were unable to determine the cause of death. */
	  buf[0] = 0;
	  utAppendShip( snum, buf );
	  sprintf(cbuf, "dead: %s was killed by %d.", buf, kb);
	  utError( cbuf );
	  utLog(cbuf);
	  
	  cprintf(8,0,ALIGN_CENTER,"#%d#%s%s", 
                  RedLevelColor, ywkb, "nothing in particular.  (How strange...)");
	}
    }
  
  if ( kb == KB_NEWGAME )
    {
      cprintf( 10,0,ALIGN_CENTER,
		"#%d#Universe conquered by #%d#%s #%d#for the #%d#%s #%d#team.",
		 InfoColor, CQC_A_BOLD, ConqInfo->conqueror, 
		 InfoColor, CQC_A_BOLD, ConqInfo->conqteam, LabelColor );
    }
  else if ( kb == KB_SELF )
    {
      i = Ships[snum].armies;
      if ( i > 0 )
	{
	  junk[0] = 0; 
	  if ( i == 1 )
	    strcpy( cbuf, "army" );
	  else
	    {
	      if ( i < 100 )
			utAppendNumWord( i, junk );
	      else
			utAppendInt( i, junk );
	      strcpy( cbuf, "armies" );
	    }
	  if ( i == 1 )
	    strcpy( buf, "was" );
	  else
	    strcpy( buf, "were");
	  if ( i == 1 )
		cprintf(10,0,ALIGN_CENTER,
		"#%d#The #%d#%s #%d#you were carrying %s not amused.",
			LabelColor, CQC_A_BOLD, cbuf, LabelColor, buf);
	  else
		cprintf(10,0,ALIGN_CENTER,
		"#%d#The #%d#%s %s #%d#you were carrying %s not amused.",
			LabelColor, CQC_A_BOLD, junk, cbuf, LabelColor, buf);
	}
    }
  else if ( kb >= 0 )
    {
      if ( eShip.status == SS_LIVE )
	{
	  cprintf( 10,0,ALIGN_CENTER,
		"#%d#He had #%d#%d%% #%d#shields and #%d#%d%% #%d#damage.",
		InfoColor, CQC_A_BOLD, round(eShip.shields), 
		InfoColor, CQC_A_BOLD, round(eShip.damage),InfoColor );
	}
    }

  cprintf(12,0,ALIGN_CENTER,
	"#%d#You got #%d#%.1f #%d#this time.", 
	InfoColor, CQC_A_BOLD, oneplace(Ships[snum].kills), InfoColor );

  if (state == S_PRESSANY)
    cprintf(MSG_LIN1, 0, ALIGN_CENTER, 
            "[ESC] for Main Menu, [TAB] to re-enter the game.");
  else
    {
      if (state == S_LASTWORDS)
        {
          cprintf(14, 0, ALIGN_LEFT, "#%d#Any last words? #%d#%s",
                  CyanColor, NoColor, prm.buf);
        }
      if (state == S_LASTWCONF)
        {
          if (prm.buf[0] != 0)
            {
              cprintf( 13,0,ALIGN_CENTER, "#%d#%s",
                       InfoColor, "Your last words are entered as:");
              cprintf( 14,0,ALIGN_CENTER, "#%d#%c%s%c",
                       YellowLevelColor, '"', prm.buf, '"' );
            }
          else
            cprintf( 14,0,ALIGN_CENTER,"#%d#%s", InfoColor,
                     "You have chosen to NOT leave any last words:" );

          cprintf( 16,0,ALIGN_CENTER, "Press [TAB] to confirm");
        }
    }

  return NODE_OK;
}  
  
static int nDeadIdle(void)
{
  return NODE_OK;
}

static int nDeadInput(int ch)
{
  int irv;

  ch = CQ_CHAR(ch);

  switch (state)
    {
    case S_PRESSANY:
      if (ch == TERM_ABORT || ch == TERM_EXTRA) /* ESC or TAB */
        {
          setONode(NULL);
          /* turn off any running effects */
          cqsEffectStop(CQS_INVHANDLE, TRUE);

          switch (ch)
            {
            case TERM_ABORT:    /* ESC */
              nMenuInit();      /* go to menu */
              return NODE_OK;
            case TERM_EXTRA:
              /* since the server is already in the 'menu' waiting for us
               * just go for it.
               */
              nPlayInit();
              return NODE_OK;
            }
        }
      else
        mglBeep(MGL_BEEP_ERR);

      break;
    case S_LASTWORDS:
      irv = prmProcInput(&prm, ch); 
      if (irv > 0)
        state = S_LASTWCONF;

      break;

    case S_LASTWCONF:
      if (ch == TERM_EXTRA)
        {                       /* we are done */
          sendMessage(MSG_GOD, lastwords);
          setONode(NULL);
          nMenuInit();
        }
      else
        {                       /* back to the drawing board */
          state = S_LASTWORDS;
          prm.buf[0] = 0;
        }

      break;
    }

  return NODE_OK;
}

