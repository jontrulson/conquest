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

#include <string>
#include <algorithm>
#include "fmt/format.h"

/* Original Copyright (C)1983-1986 by Jef Poskanzer and Craig Leres */

#include "global.h"

#include "conf.h"

#include "conqdef.h"
#include "cb.h"
#include "conqlb.h"
#include "rndlb.h"
#include "conqutil.h"
#include "conqai.h"
#include "conqunix.h"

#include "context.h"

#include "global.h"
#include "sem.h"
#include "color.h"
#include "ui.h"
#include "info.h"
#include "options.h"
#include "cd2lb.h"
#include "iolb.h"
#include "cumisc.h"
#include "record.h"
#include "clientlb.h"
#include "cuclient.h"
#include "clntauth.h"
#include "display.h"
#include "conqinit.h"
#include "cprintf.h"

#include "gameDir.h"

/* option masks */
#define OP_NONE (unsigned int)(0x00000000)
#define OP_REGENSYSCONF (unsigned int)(0x00000001)
#define OP_INITSTUFF (unsigned int)(0x00000002)
#define OP_DISABLEGAME (unsigned int)(0x00000004)
#define OP_ENABLEGAME (unsigned int)(0x00000008)

static bool doDisplay;
static std::string operUsername;

void DoConqoperSig(int sig);
void astoperservice(int sig);
void EnableConqoperSignalHandler(void);
void operStopTimer(void);
void operSetTimer(void);

void bigbang(void);
void debugdisplay( int snum );
void debugplan(void);
int opPlanetMatch( char str[], int *pnum );
void kiss(int snum, int prompt_flg);
void opback( int lastrev, int *savelin );
void operate(void);
void opinfo(void);
void opinit(void);
void opPlanetList(void);
void opresign(void);
void oprobot(void);
void opstats(void);
void opTeamList(void);
void opuadd(void);
void oppedit(void);
void opuedit(void);
void watch(void);
int prompt_ship(char buf[], int *snum, int *normal);
void dowatchhelp(void);
void toggle_line(int snum, int old_snum);
char *build_toggle_str(char snum_str[], int snum);
void menu_item( char *option, char *msg_line, int lin, int col );
int DoInit(char InitChar, int cmdline);


/*  conqoper - main program */
int main(int argc, char *argv[])
{
    int i;
    unsigned int OptionAction;

    char InitStuffChar = '\0';

    utSetLogProgramName(argv[0]);
    OptionAction = OP_NONE;

    if (!isagod(-1) )
    {
        printf("Access denied.\n\nIn order to run conqoper, you must:\n");
        printf("- be the root user, or\n");
        printf("- be a member of the conquest group (default \"conquest\")\n");
        printf("  or whatever group was chosen as the conquest group when\n");
        printf("  conquest was built.\n");

        // printf("Poor cretins such as yourself lack the "
        //       "skills necessary to use this program.\n");
        exit(1);
    }

    rndini();		/* initialize random numbers */

    while ((i = getopt(argc, argv, "CDEG:I:v")) != EOF)    /* get command args */
        switch (i)
        {
        case 'C':
            OptionAction |= OP_REGENSYSCONF;
            break;

        case 'D':
            OptionAction |= OP_DISABLEGAME;
            break;

        case 'E':
            OptionAction |= OP_ENABLEGAME;
            break;

        case 'G':
            gameSubdirectory.set(optarg);
            break;

        case 'I':
            OptionAction |= OP_INITSTUFF;
            InitStuffChar = *optarg; /* first character */
            break;

        case 'v':
            cqDebug++;
            break;

        case '?':
        default:
            fprintf(stderr, "usage: %s [-C] [-D] [-E] [-I <what>] [-v]\n",
                    argv[0]);
            fprintf(stderr, "       -C \t\trebuild systemwide conquest.conf file\n");
            fprintf(stderr, "       -D \t\tdisable the game\n");
            fprintf(stderr, "       -E \t\tenable the game\n");
            fprintf(stderr, "       -G \t\tspecify alternate game subdirectory\n");
            fprintf(stderr, "       -I <what> \tInitialize <what>, where <what> is:\n");
            fprintf(stderr, "          e - everything\n");
            fprintf(stderr, "          g - game\n");
            fprintf(stderr, "          l - lockwords\n");
            fprintf(stderr, "          m - messages\n");
            fprintf(stderr, "          p - planets\n");
            fprintf(stderr, "          r - robots\n");
            fprintf(stderr, "          s - ships\n");
            fprintf(stderr, "          u - universe\n");
            fprintf(stderr, "          z - zero common block\n");
            fprintf(stderr, "       -v \t\tincrease verbosity\n");

            exit(1);
            break;
        }

    if ((ConquestGID = getConquestGID()) == -1)
    {
        fprintf(stderr, "conqoper: getConquestGID() failed\n");
        exit(1);
    }


    if (OptionAction != OP_NONE)
    {
        /* need to be conq grp for these */
        if (setgid(ConquestGID) == -1)
	{
            fprintf(stderr, "conqoper: setgid(): failed\n");
            exit(1);
	}

        GetSysConf(true);		/* init defaults... */

        if (semInit() == -1)
	{
            fprintf(stderr, "semInit() failed to get semaphores. exiting.\n");
            exit(1);
	}

        /* load conqinitrc */
        if (cqiLoadRC(CQI_FILE_CONQINITRC, NULL, cqDebug))
	{
            fprintf(stderr, "FATAL: can't load conqinitrc file.\n");
            exit(1);
	}

        cbMap();		/* Map the conquest universe common block */

				/* regen sysconf file? */
        if ((OptionAction & OP_REGENSYSCONF) != 0)
	{
            if (MakeSysConf() == -1)
                exit(1);

	}

        /* initialize something? */
        if ((OptionAction & OP_INITSTUFF) != 0)
	{
            int rv;

            rv = DoInit(InitStuffChar, true);

            if (rv != 0)
                exit(1);
	}

        /* turn the game on */
        if ((OptionAction & OP_ENABLEGAME) != 0)
	{
            SysConf.Closed = false;
            SaveSysConfig();
            fprintf(stdout, "Game enabled.\n");
	}

        /* turn the game off */
        if ((OptionAction & OP_DISABLEGAME) != 0)
	{
            SysConf.Closed = true;
            SaveSysConfig();
            fprintf(stdout, "Game disabled.\n");
	}

        /* the process *must* exit in this block. */
        exit(0);

    }

    if (GetSysConf(false) == -1)
    {
#ifdef DEBUG_CONFIG
        utLog("%s@%d: main(): GetSysConf() returned -1.", __FILE__, __LINE__);
#endif
        /* */
        ;
    }

    Context.updsec = 2;		/* default upd per sec */

    if (GetConf(0) == -1)	/* use one if there, else defaults
				   A missing or out-of-date conquest.conf file
				   will be ignored */
    {
#ifdef DEBUG_CONFIG
        utLog("%s@%d: main(): GetSysConf() returned -1.", __FILE__, __LINE__);
#endif
        /* */
        ;
    }

    if (setgid(ConquestGID) == -1)
    {
        utLog("conqoper: setgid(%d): %s",
              ConquestGID,
              strerror(errno));
        fprintf(stderr, "conqoper: setgid(): failed\n");
        exit(1);
    }

    if (semInit() == -1)
    {
        fprintf(stderr, "semInit() failed to get semaphores. exiting.\n");
        exit(1);
    }

    /* load conqinitrc */
    if (cqiLoadRC(CQI_FILE_CONQINITRC, NULL, cqDebug))
    {
        fprintf(stderr, "FATAL: can't load conqinitrc file.\n");
        exit(1);
    }

    cbMap();			/* Map the conquest universe common block */
    cdinit();			/* initialize display environment */

    Context.unum = -1;          /* stow user number */
    Context.snum = -1;		/* don't display in cdgetp - JET */
    historyCurrentSlot = -1;	/* useless as an op */
    Context.lasttang = Context.lasttdist = 0;
    Context.lastInfoTarget.clear();
    doDisplay = true;
    Context.maxlin = cdlins();	/* number of lines */

    Context.maxcol = cdcols();	/* number of columns */
    Context.recmode = RECMODE_OFF;

    operUsername = clbGetUserLogname();
    std::string msgbuf = fmt::format("OPER: User {} has entered conqoper.",
                                     operUsername);
    utLog("%s", msgbuf.c_str());        /* log it too... */
    clbStoreMsg( MSG_FROM_COMP, 0, MSG_TO_GOD, 0, msgbuf );

    operate();

    cdend();

    exit(0);

}


/*  bigbang - fire a torp from every ship */
/*  SYNOPSIS */
/*    bigbang */
void bigbang(void)
{
    int i, snum, cnt;
    real dir;

    dir = 0.0;
    cnt = 0;
    for ( snum = 0; snum < cbLimits.maxShips(); snum++ )
        if ( cbShips[snum].status == SS_LIVE )
        {
            for ( i = 0; i < cbLimits.maxTorps(); i = i + 1 )
                if ( ! clbLaunch( snum, dir, 1, LAUNCH_NORMAL ) )
                    break;
                else
                {
                    dir = utMod360( dir + 40.0 );
                    cnt = cnt + 1;
                }
        }
    cprintf(MSG_LIN1,0,ALIGN_CENTER,
            "#%d#bigbang: Fired #%d#%d #%d#torpedos, hoo hah won't they be surprised!",
            InfoColor,SpecialColor,cnt,InfoColor );

    utLog("OPER: %s fired BigBang - %d torpedos",
          operUsername.c_str(), cnt);

    return;

}


/*  debugdisplay - verbose and ugly version of display */
/*  SYNOPSIS */
/*    int snum */
/*    debugdisplay( snum ) */
void debugdisplay( int snum )
{

    /* The following aren't displayed by this routine... */
    /* int ssrpwar(snum,MAXPLANETS)	# self-ruled planets s/he is at war */
    /* int slastmsg(snum)		# last message seen */
    /* int salastmsg(snum)		# last message allowed to be seen */
    /* int smap(snum)			# strategic map or not */
    /* int spfuse(snum)			# tenths until phasers can be fired */
    /* int sscanned(snum,NUMPLAYERTEAMS)	# fuse for which ships have been */
    /* int stalert(snum)			# torp alert! */
    /* int sctime(snum)			# cpu hundreths at last check */
    /* int setime(snum)			# elapsed hundreths at last check */
    /* int scacc(snum)			# accumulated cpu time */
    /* int seacc(snum)			# accumulated elapsed time */

    int i, j, unum, lin, tcol, dcol;
    real x;
    char buf[MSGMAXLINE];
    const char *torpstr = "???";

#define TOFF "OFF"
#define TLAUNCHING "LAUNCHING"
#define TLIVE "LIVE"
#define TDETONATE "DETONATE"
#define TFIREBALL "EXPLODING"


    // can't do squat with a negative ship number...
    if (snum < 0)
    {
        utLog("%s: snum is negative (%d), returning", __FUNCTION__, snum);
        return;
    }

    cdclrl( 1, MSG_LIN1 - 1 ); 	/* don't clear the message lines */
    unum = cbShips[snum].unum;

    lin = 1;
    tcol = 1;
    dcol = tcol + 10;
    cprintf(lin,tcol,ALIGN_NONE,"#%d#%s",LabelColor, "    ship:");
    buf[0] = 0;
    utAppendShip(buf , snum) ;
    if ( SROBOT(snum) )
        strcat(buf, " (ROBOT)");
    cprintf( lin, dcol,ALIGN_NONE,"#%d#%s",InfoColor,buf );
    lin++;
    cprintf(lin,tcol,ALIGN_NONE,"#%d#%s",LabelColor, "      sx:");
    cprintf(lin,dcol,ALIGN_NONE,"#%d#%0g",InfoColor, oneplace(cbShips[snum].x));
    lin++;
    cprintf(lin,tcol,ALIGN_NONE,"#%d#%s",LabelColor, "      sy:");
    cprintf(lin,dcol,ALIGN_NONE,"#%d#%0g",InfoColor, oneplace(cbShips[snum].y));
    lin++;
    cprintf(lin,tcol,ALIGN_NONE,"#%d#%s",LabelColor, "     sdx:");
    cprintf(lin,dcol,ALIGN_NONE,"#%d#%0g",InfoColor, oneplace(cbShips[snum].dx));
    lin++;
    cprintf(lin,tcol,ALIGN_NONE,"#%d#%s",LabelColor, "     sdy:");
    cprintf(lin,dcol,ALIGN_NONE,"#%d#%0g",InfoColor, oneplace(cbShips[snum].dy));
    lin++;
    cprintf(lin,tcol,ALIGN_NONE,"#%d#%s",LabelColor, "  skills:");
    cprintf(lin,dcol,ALIGN_NONE,"#%d#%0g",InfoColor,
            (oneplace(cbShips[snum].kills + cbShips[snum].strkills)));
    lin++;
    cprintf(lin,tcol,ALIGN_NONE,"#%d#%s",LabelColor, "   swarp:");
    x = oneplace(cbShips[snum].warp);
    if ( x == ORBIT_CW )
        cprintf(lin,dcol,ALIGN_NONE,"#%d#%s",InfoColor, "ORBIT_CW");
    else if ( x == ORBIT_CCW )
        cprintf(lin,dcol,ALIGN_NONE,"#%d#%s",InfoColor, "ORBIT_CCW");
    else
  	cprintf(lin,dcol,ALIGN_NONE,"#%d#%0g",InfoColor, x);
    lin++;
    cprintf(lin,tcol,ALIGN_NONE,"#%d#%s",LabelColor, "  sdwarp:");
    cprintf(lin,dcol,ALIGN_NONE,"#%d#%0g",InfoColor, oneplace(cbShips[snum].dwarp));
    lin++;
    cprintf(lin,tcol,ALIGN_NONE,"#%d#%s",LabelColor, "   shead:");
    cprintf(lin,dcol,ALIGN_NONE,"#%d#%0d",InfoColor, iround(cbShips[snum].head));
    lin++;
    cprintf(lin,tcol,ALIGN_NONE,"#%d#%s",LabelColor, "  sdhead:");
    cprintf(lin,dcol,ALIGN_NONE,"#%d#%0d",InfoColor, iround(cbShips[snum].dhead));
    lin++;
    cprintf(lin,tcol,ALIGN_NONE,"#%d#%s",LabelColor, " sarmies:");
    cprintf(lin,dcol,ALIGN_NONE,"#%d#%0d",InfoColor, iround(cbShips[snum].armies));

    lin = 1;
    tcol = 23;
    dcol = tcol + 12;
    cprintf(lin,tcol,ALIGN_NONE,"#%d#%s",LabelColor, "      name:");
    if ( cbShips[snum].alias[0] != 0 )
  	cprintf(lin,dcol,ALIGN_NONE,"#%d#%s",InfoColor, cbShips[snum].alias);
    lin++;
    cprintf(lin,tcol,ALIGN_NONE,"#%d#%s",LabelColor, "  username:");
    buf[0] = 0;
    if ( unum >= 0 && unum < cbLimits.maxUsers() )
    {
        strcpy(buf , cbUsers[unum].username) ;
        if ( buf[0] != 0 )
            utAppendChar(buf , ' ') ;
    }
    utAppendChar(buf , '(') ;
    utAppendInt(buf , unum) ;
    utAppendChar(buf, ')');
    cprintf(lin,dcol,ALIGN_NONE,"#%d#%s",InfoColor, buf);
    lin++;
    cprintf(lin,tcol,ALIGN_NONE,"#%d#%s",LabelColor, "     slock:");
    cprintf(lin+1,tcol,ALIGN_NONE,"#%d#%s",LabelColor, "       dtt:");

    if (cbShips[snum].lock != LOCK_NONE)
    {
        i = (int)cbShips[snum].lockDetail;
        if ( cbShips[snum].lock == LOCK_PLANET && i < cbLimits.maxPlanets() )
        {
            cprintf(lin,dcol,ALIGN_NONE,"#%d#%s",InfoColor, cbPlanets[i].name);
            cprintf(lin+1,dcol,ALIGN_NONE,"#%d#%d",InfoColor,
                    iround( dist( cbShips[snum].x, cbShips[snum].y, cbPlanets[i].x, cbPlanets[i].y ) ));
        }
        else if (cbShips[snum].lock == LOCK_SHIP && i < cbLimits.maxShips())
        {
            cprintf(lin,dcol,ALIGN_NONE,"#%d#%d",InfoColor, i);
        }
        else
        {
            // should never happen unless a new lock type is added
            cprintf(lin,dcol,ALIGN_NONE,"#%d#%d(%d)",InfoColor,
                    cbShips[snum].lock, (int)cbShips[snum].lockDetail);
        }
    }

    lin+=2;
    cprintf(lin,tcol,ALIGN_NONE,"#%d#%s",LabelColor, "     sfuel:");
    cprintf(lin,dcol,ALIGN_NONE,"#%d#%0d",InfoColor, iround(cbShips[snum].fuel));
    lin++;
    cprintf(lin,tcol,ALIGN_NONE,"#%d#%s",LabelColor, "       w/e:");
    sprintf( buf, "%d/%d", cbShips[snum].weapalloc, cbShips[snum].engalloc );
    if ( cbShips[snum].wfuse > 0 || cbShips[snum].efuse > 0 )
    {
        strcat(buf , " (") ;
        utAppendInt(buf , cbShips[snum].wfuse) ;
        utAppendChar(buf , '/') ;
        utAppendInt(buf , cbShips[snum].efuse) ;
        utAppendChar(buf, ')');
    }
    cprintf(lin,dcol,ALIGN_NONE,"#%d#%s",InfoColor, buf);
    lin++;
    cprintf(lin,tcol,ALIGN_NONE,"#%d#%s",LabelColor, "      temp:");
    sprintf( buf, "%d/%d", iround(cbShips[snum].wtemp), iround(cbShips[snum].etemp) );
    cprintf(lin,dcol,ALIGN_NONE,"#%d#%s",InfoColor, buf);
    lin++;
    cprintf(lin,tcol,ALIGN_NONE,"#%d#%s",LabelColor, "   ssdfuse:");
    i = cbShips[snum].sdfuse;
    buf[0] = 0;
    if ( i != 0 )
    {
        sprintf( buf, "%d ", i );
    }
    if ( SCLOAKED(snum) )
        strcat(buf, "(CLOAKED)");
    cprintf(lin,dcol,ALIGN_NONE,"#%d#%s",LabelColor, buf);
    lin++;
    cprintf(lin,tcol,ALIGN_NONE,"#%d#%s",LabelColor, "      spid:");
    i = cbShips[snum].pid;
    if ( i != 0 )
    {
/*      sprintf( buf, "%d", i ); */
        cprintf(lin,dcol,ALIGN_NONE,"#%d#%d",InfoColor, i);
    }
    lin++;
    cprintf(lin,tcol,ALIGN_NONE,"#%d#%s",LabelColor, "slastblast:");
    cprintf(lin,dcol,ALIGN_NONE,"#%d#%0g",InfoColor, oneplace(cbShips[snum].lastblast));
    lin++;
    cprintf(lin,tcol,ALIGN_NONE,"#%d#%s",LabelColor, "slastphase:");
    cprintf(lin,dcol,ALIGN_NONE,"#%d#%0g",InfoColor, oneplace(cbShips[snum].lastphase));

    lin = 1;
    tcol = 57;
    dcol = tcol + 12;
    cprintf(lin,tcol,ALIGN_NONE,"#%d#%s",LabelColor, "   sstatus:");
    buf[0] = 0;
    utAppendShipStatus(buf , cbShips[snum].status) ;
    cprintf(lin,dcol,ALIGN_NONE,"#%d#%s",InfoColor, buf);
    lin++;
    cprintf(lin,tcol,ALIGN_NONE,"#%d#%s",LabelColor, " skilledby:");
    if ( cbShips[snum].killedBy != KB_NONE )
    {
        buf[0] = 0;
        utAppendKilledBy(buf, cbShips[snum].killedBy,
                         cbShips[snum].killedByDetail);
        cprintf(lin,dcol,ALIGN_NONE,"#%d#%s",InfoColor, buf);
    }
    lin++;
    cprintf(lin,tcol,ALIGN_NONE,"#%d#%s",LabelColor, "   shields:");
    cprintf(lin,dcol,ALIGN_NONE,"#%d#%0d",InfoColor, iround(cbShips[snum].shields));
    if ( ! SSHUP(snum) )
  	cprintf(lin,dcol+5,ALIGN_NONE,"#%d#%c",InfoColor, 'D');
    else
  	cprintf(lin,dcol+5,ALIGN_NONE,"#%d#%c",InfoColor, 'U');
    lin++;
    cprintf(lin,tcol,ALIGN_NONE,"#%d#%s",LabelColor, "   sdamage:");
    cprintf(lin,dcol,ALIGN_NONE,"#%d#%0d",InfoColor, iround(cbShips[snum].damage));
    if ( SREPAIR(snum) )
  	cprintf(lin,dcol+5,ALIGN_NONE,"#%d#%c",InfoColor, 'R');
    lin++;
    cprintf(lin,tcol,ALIGN_NONE,"#%d#%s",LabelColor, "  stowedby:");

    if ( STOWEDBY(snum) )
    {
        buf[0] = 0;
        utAppendShip(buf, cbShips[snum].towedby) ;
        cprintf(lin,dcol,ALIGN_NONE,"#%d#%s",InfoColor, buf);
    }
    lin++;
    cprintf(lin,tcol,ALIGN_NONE,"#%d#%s",LabelColor, "   stowing:");

    if ( STOWING(snum) )
    {
        buf[0] = 0;
        utAppendShip(buf, cbShips[snum].towing);
        cprintf(lin,dcol,ALIGN_NONE,"#%d#%s",InfoColor, buf);
    }

    lin++;
    cprintf(lin,tcol,ALIGN_NONE,"#%d#%s",LabelColor, "      swar:");
    buf[0] = '(';
    for ( i = 0; i < NUMPLAYERTEAMS; i = i + 1 )
        if ( cbShips[snum].war[i] )
            buf[i+1] = cbTeams[i].teamchar;
        else
            buf[i+1] = '-';
    buf[NUMPLAYERTEAMS+1] = ')';
    buf[NUMPLAYERTEAMS+2] = 0;
    cprintf(lin,dcol,ALIGN_NONE,"#%d#%s",InfoColor, buf);

    lin++;
    cprintf(lin,tcol,ALIGN_NONE,"#%d#%s",LabelColor, "     srwar:");
    buf[0] = '(';
    for ( i = 0; i < NUMPLAYERTEAMS; i = i + 1 )
        if ( cbShips[snum].rwar[i] )
            buf[i+1] = cbTeams[i].teamchar;
        else
            buf[i+1] = '-';
    buf[NUMPLAYERTEAMS+1] = ')';
    buf[NUMPLAYERTEAMS+2] = 0;
    cprintf(lin,dcol,ALIGN_NONE,"#%d#%s",InfoColor, buf);

    lin ++;

    cprintf(lin,tcol,ALIGN_NONE,"#%d#%s",LabelColor, "     flags:");
    cprintf(lin,dcol,ALIGN_NONE,"#%d#0x%04x",InfoColor, cbShips[snum].flags);

    lin++;

    cprintf(lin,tcol,ALIGN_NONE,"#%d#%s",LabelColor, "   saction:");
    i = cbShips[snum].action;
    if ( i != 0 )
    {
        std::string abuf;
        robstr( i, abuf );
        cprintf(lin,dcol,ALIGN_NONE,"#%d#%s",InfoColor, abuf.c_str());
    }


    // We need to limit this display to 9 torps, the old hardcoded
    // limit, on this logic for now since we can't display more than
    // that anyway.
    static const int maxDisplayableTorps = 9;
    lin = (Context.maxlin - (Context.maxlin - MSG_LIN1))
        - (maxDisplayableTorps+1);  /* dwp */
    cprintf(lin,3,ALIGN_NONE,"#%d#%s",LabelColor,
            "tstatus    tfuse    tmult       tx       ty      tdx      tdy     twar");
    for ( i=0; i<cbLimits.maxTorps() && i<maxDisplayableTorps; i++ )
    {
        lin++;
        switch(cbShips[snum].torps[i].status)
	{
	case TS_OFF:
            torpstr = TOFF;
            break;
	case TS_LAUNCHING:
            torpstr = TLAUNCHING;
            break;
	case TS_LIVE:
            torpstr = TLIVE;
            break;
	case TS_DETONATE:
            torpstr = TDETONATE;
            break;
	case TS_FIREBALL:
            torpstr = TFIREBALL;
            break;
	}
        cprintf(lin,3,ALIGN_NONE,"#%d#%s",InfoColor, torpstr);
        if ( cbShips[snum].torps[i].status != TS_OFF )
	{
            cprintf(lin,13,ALIGN_NONE,"#%d#%6d",InfoColor, cbShips[snum].torps[i].fuse);
            cprintf(lin,19,ALIGN_NONE,"#%d#%9g",InfoColor, oneplace(cbShips[snum].torps[i].mult));
            cprintf(lin,28,ALIGN_NONE,"#%d#%9g",InfoColor, oneplace(cbShips[snum].torps[i].x));
            cprintf(lin,37,ALIGN_NONE,"#%d#%9g",InfoColor, oneplace(cbShips[snum].torps[i].y));
            cprintf(lin,46,ALIGN_NONE,"#%d#%9g",InfoColor, oneplace(cbShips[snum].torps[i].dx));
            cprintf(lin,55,ALIGN_NONE,"#%d#%9g",InfoColor, oneplace(cbShips[snum].torps[i].dy));
            buf[0] = '(';
            for ( j = 0; j < NUMPLAYERTEAMS; j++ )
                if ( cbShips[snum].torps[i].war[j] )
                    buf[j+1] = cbTeams[j].teamchar;
                else
                    buf[j+1] = '-';
            buf[NUMPLAYERTEAMS+1] = ')';
            buf[NUMPLAYERTEAMS+2] = 0;
            cprintf(lin,67,ALIGN_NONE,"#%d#%s",InfoColor, buf);
	}
    }

    cdmove( 1, 1 );
    cdrefresh();

    return;

}


/*  debugplan - debugging planet list */
/*  SYNOPSIS */
/*    debugplan */
void debugplan(void)
{

    int i, j, k, cmd, lin, col, olin;
    int outattr;
    char junk[10], uninhab[20];
    char hd0[MSGMAXLINE*4];
    const char *hd1="D E B U G G I N G  P L A N E T   L I S T";
    const char *hd2="planet        C T arm uih scan        planet        C T arm uih scan";
    char hd3[BUFFER_SIZE_256];
    static bool firstTime = true;
    int PlanetOffset;             /* offset into cbLimits.maxPlanets() for this page */
    int PlanetIdx = 0;
    bool Done;


    if (firstTime)
    {
        firstTime = false;
        sprintf(hd0,
                "#%d#%s#%d#%s#%d#%s#%d#%s",
                LabelColor,
                hd1,
                InfoColor,
                "  ('",
                SpecialColor,
                "-",
                InfoColor,
                "' = hidden)");

    }

    /* sort the planets */
    std::vector<int> sv;
    for ( i = 0; i < cbLimits.maxPlanets(); i++ )
        sv.push_back(i);
    clbSortPlanets( sv );

    strcpy( hd3, hd2 );
    for ( i = 0; hd3[i] != 0; i++ )
        if ( hd3[i] != ' ' )
            hd3[i] = '-';

    PlanetOffset = 0;
    cdclear();

    Done = false;
    do
    {
        cdclra(0, 0, MSG_LIN1 + 2, Context.maxcol - 1);

        lin = 1;
        cprintf(lin,0,ALIGN_CENTER,"%s", hd0 );

        lin++;
        cprintf( lin,0,ALIGN_CENTER,"#%d#%s", LabelColor, hd2 );
        lin++;
        cprintf( lin,0,ALIGN_CENTER,"#%d#%s", LabelColor, hd3 );
        lin++;
        olin = lin;
        col = 6;

        PlanetIdx = 0;

        if (PlanetOffset < cbLimits.maxPlanets())
        {
            while ((PlanetOffset + PlanetIdx) < cbLimits.maxPlanets())
            {
                i = PlanetOffset + PlanetIdx;
                PlanetIdx++;
                k = sv[i];

                for ( j = 0; j < NUMPLAYERTEAMS; j++ )
                    if ( cbPlanets[k].scanned[j] )
                        junk[j] = cbTeams[j].teamchar;
                    else
                        junk[j] = '-';
                junk[j] = 0;
                j = cbPlanets[k].uninhabtime;
                if ( j != 0 )
                    sprintf( uninhab, "%d", j );
                else
                    uninhab[0] = 0;

                switch(cbPlanets[k].type)
		{
		case PLANET_SUN:
                    outattr = RedLevelColor;
                    break;
		case PLANET_CLASSM:
                    outattr = GreenLevelColor;
                    break;
		case PLANET_DEAD:
                    outattr = YellowLevelColor;
                    break;
		case PLANET_CLASSA:
		case PLANET_CLASSO:
		case PLANET_CLASSZ:
                    outattr = CQC_A_BOLD;
                    break;
		case PLANET_GHOST:
                    outattr = NoColor;
                    break;
		default:
                    outattr = SpecialColor;
                    break;
		}

                cprintf(lin, col, ALIGN_NONE,
                        "#%d#%-13s %c %c %3d %3s %4s",
                        outattr,
                        cbPlanets[k].name,
                        cbConqInfo->chrplanets[cbPlanets[k].type],
                        cbTeams[cbPlanets[k].team].teamchar,
                        cbPlanets[k].armies,
                        uninhab,
                        junk );

                if ( ! PVISIBLE(k) )
                    cprintf(lin,col-2,ALIGN_NONE, "#%d#%c", SpecialColor, '-');

                lin++;
                if ( lin == MSG_LIN2 )
		{
                    if (col == 44)
                        break;	/* out of while */
                    lin = olin;
                    col = 44;
		}
                uiPutColor(0);
	    } /* while */

            if ((PlanetOffset + PlanetIdx) >= cbLimits.maxPlanets())
                mcuPutPrompt( MTXT_DONE, MSG_LIN2 ); /* last page? */
            else
                mcuPutPrompt( MTXT_MORE, MSG_LIN2 );

            cdrefresh();

            if (iogtimed( &cmd, 1.0 ))
            {                   /* got a char */
                if (cmd != ' ')
                {               /* quit */
                    Done = true;
                }
                else
                {               /* some other key... */
                    /* setup for new page */
                    PlanetOffset += PlanetIdx;
                    if (PlanetOffset >= cbLimits.maxPlanets())
                    {           /* pointless to continue */
                        Done = true;
                    }
                }
            }

            /* didn't get a char, update */
        } /* if PlanetOffset <= cbLimits.maxPlanets() */
        else
            Done = true;            /* else PlanetOffset > cbLimits.maxPlanets() */

    } while(!Done); /* do */

    return;

}


/*  gplanmatch - GOD's check if a string matches a planet name */
/*  SYNOPSIS */
/*    int gplanmatch, pnum */
/*    char str() */
/*    int status */
/*    status = opPlanetMatch( str, pnum ) */
int opPlanetMatch( char str[], int *pnum )
{
    if ( utIsDigits( str ) )
    {
        if ( ! utSafeCToI( pnum, str, 0 ) )
            return ( false );
        if ( *pnum < 0 || *pnum >= cbLimits.maxPlanets() )
            return ( false );
    }
    else
        return ( clbPlanetMatch( str, pnum, true ) );

    return ( true );

}


/*  kiss - give the kiss of death */
/*  SYNOPSIS */
/*    kiss */
void kiss(int snum, int prompt_flg)
{
    int unum;
    char ch, buf[MSGMAXLINE], mbuf[MSGMAXLINE], ssbuf[MSGMAXLINE];
    bool didany;
    static const char *prompt_str = "Kill what (<cr> for driver)? ";
    static const char *kill_driver_str = "Killing the driver.";
    static const char *cant_kill_ship_str = "You can't kill ship %c%d (%s) status (%s).";
    static const char *kill_ship_str1 = "Killing ship %c%d (%s).";
    static const char *kill_ship_str2 = "Killing ship %c%d (%s) user (%s).";
    static const char *nobody_str = "Nobody here but us GODs.";
    static const char *no_user_str = "No such user.";
    static const char *no_ship_str = "No such ship.";
    static const char *not_flying_str = "User %s (%s) isn't flying right now.";

    /* Find out what to kill. */
    if (prompt_flg)
    {
        cdclrl( MSG_LIN1, 2 );
        if (snum < 0)
            buf[0] = 0;
        else
            sprintf(buf, "%d", snum);

        ch = (char)cdgetx( prompt_str, MSG_LIN1, 1, TERMS, buf, MSGMAXLINE,
                           true);
        if ( ch == TERM_ABORT )
	{
            cdclrl( MSG_LIN1, 1 );
            cdmove( 1, 1 );
            return;
	}
        utDeleteBlanks( buf );
    }
    else
    {
        if (snum < 0)
            return;
        else
            sprintf(buf,"%d",snum);
    }

    /* Kill the driver? */
    if ( buf[0] == 0 )
    {
        cdclrl( MSG_LIN1, 1 );
        sprintf(mbuf,"%s", kill_driver_str);
        cdputs( mbuf, MSG_LIN1, 1 );
        if ( mcuConfirm() )
            if ( cbDriver->drivstat == DRS_RUNNING )
                cbDriver->drivstat = DRS_KAMIKAZE;
        cdclrl( MSG_LIN1, 2 );
        cdmove( 1, 1 );
        utLog("OPER: %s killed the driver", operUsername.c_str());
        return;
    }

    /* Kill a ship? */
    if ( utIsDigits( buf ) )
    {
        utSafeCToI( &snum, buf, 0 );		/* ignore status */
        if ( snum < 0 || snum >= cbLimits.maxShips() )
            cdputs( no_ship_str, MSG_LIN2, 1 );
        else if ( cbShips[snum].status != SS_LIVE ) {
            cdclrl( MSG_LIN1, 1 );
            ssbuf[0] = 0;
            utAppendShipStatus(ssbuf, cbShips[snum].status) ;
            sprintf(mbuf, cant_kill_ship_str,
                    cbTeams[cbShips[snum].team].teamchar,
                    snum,
                    cbShips[snum].alias,
                    ssbuf);
            cdputs( mbuf, MSG_LIN1, 1 );
        }
        else {
            cdclrl( MSG_LIN1, 1 );
            sprintf(mbuf, kill_ship_str1,
                    cbTeams[cbShips[snum].team].teamchar, snum, cbShips[snum].alias);
            cdputs( mbuf, MSG_LIN1, 1 );
            if ( mcuConfirm() )
            {
                clbKillShip( snum, KB_GOD, 0 );
                cdclrl( MSG_LIN2, 1 );
                utLog("OPER: %s killed ship %d",
                      operUsername.c_str(), snum);
            }
            cdclrl( MSG_LIN1, 1 );
        }
        cdmove( 1, 1 );
        return;
    }

    /* Kill EVERYBODY? */
    if ( utStringMatch( buf, "all", false ) )
    {
        didany = false;
        for ( snum = 0; snum < cbLimits.maxShips(); snum++ )
            if ( cbShips[snum].status == SS_LIVE )
            {
                didany = true;
                cdclrl( MSG_LIN1, 1 );
                sprintf(buf, kill_ship_str1,
                        cbTeams[cbShips[snum].team].teamchar,
                        snum,
                        cbShips[snum].alias);
                cdputs( buf, MSG_LIN1, 1 );
                if ( mcuConfirm() )
                {
                    utLog("OPER: %s killed ship %d",
                          operUsername.c_str(), snum);
                    clbKillShip( snum, KB_GOD, 0 );
                }
            }
        if ( didany )
            cdclrl( MSG_LIN1, 2 );
        else
            cdputs( nobody_str, MSG_LIN2, 1 );
        cdmove( 1, 1 );
        return;
    }

    /* Kill a user? */
    if ( ! clbGetUserNum( &unum, buf, USERTYPE_ANY ) )
    {
        cdputs( no_user_str, MSG_LIN2, 1 );
        cdmove( 0, 0 );
        return;
    }

    /* Yes. */
    didany = false;
    for ( snum = 0; snum < cbLimits.maxShips(); snum++ )
        if ( cbShips[snum].status == SS_LIVE )
            if ( cbShips[snum].unum == unum )
            {
                didany = true;
                cdclrl( MSG_LIN1, 1 );
                sprintf(mbuf, kill_ship_str2,
                        cbTeams[cbShips[snum].team].teamchar,
                        snum,
                        cbShips[snum].alias,
                        buf);
                cdputs( mbuf, MSG_LIN1, 1 );
                if ( mcuConfirm() )
                {
                    clbKillShip( snum, KB_GOD, 0 );
                    cdclrl( MSG_LIN2, 1 );
                    utLog("OPER: %s killed ship %d",
                          operUsername.c_str(), snum);
                }
            }
    if ( ! didany ) {
	cdclrl( MSG_LIN1, 1 );
	sprintf(mbuf, not_flying_str, cbUsers[unum].username,
		cbUsers[unum].alias);
	cdputs( mbuf, MSG_LIN1, 1 );
    }
    else
        cdclrl( MSG_LIN1, 2 );

    return;

}


/*  opback - put up the background for the operator program */
/*  SYNOPSIS */
/*    int lastrev, savelin */
/*    opback( lastrev, savelin ) */
void opback( int lastrev, int *savelin )
{
    int i, lin, col;
    extern char *ConquestVersion;
    extern char *ConquestDate;

    static bool firstTime = true;
    static std::string sfmt;

    if (firstTime)
    {
        firstTime = false;
        sfmt = fmt::format("#{}#(#{}#%c#{}#)#{}# - %s",
                LabelColor,
                InfoColor,
                LabelColor,
                NoColor);
    }

    cdclear();

    lin = 1;
    std::string cbuf;
    if ( lastrev == COMMONSTAMP )
    {
        uiPutColor(NoColor|CQC_A_BOLD);
        cdputc( "CONQUEST OPERATOR PROGRAM", lin );
        uiPutColor(YellowLevelColor);
        cbuf = fmt::format("{} ({})",
                           ConquestVersion, ConquestDate);
        cdputc( cbuf.c_str(), lin+1 );
    }
    else
    {
        uiPutColor(RedLevelColor);
        cbuf = fmt::format("CONQUEST COMMON BLOCK MISMATCH {} != {}",
                           lastrev, COMMONSTAMP );
        cdputc( cbuf.c_str(), lin );
        uiPutColor(0);
    }

    EnableConqoperSignalHandler(); /* enable trapping of interesting signals */

    lin++;

    lin+=2;
    *savelin = lin;
    lin+=3;

    cprintf(lin,0,ALIGN_CENTER,"#%d#%s",NoColor, "Options:");
    lin+=2;
    i = lin;

    col = 5;
    cprintf(lin,col,ALIGN_NONE,sfmt.c_str(), 'f', "flip the open/closed flag");
    lin++;
    cprintf(lin,col,ALIGN_NONE,sfmt.c_str(), 'd', "flip the doomsday machine!");
    lin++;
    cprintf(lin,col,ALIGN_NONE,sfmt.c_str(), 'h', "hold the driver");
    lin++;
    cprintf(lin,col,ALIGN_NONE,sfmt.c_str(), 'I', "initialize");
    lin++;
    cprintf(lin,col,ALIGN_NONE,sfmt.c_str(), 'b', "big bang");
    lin++;
    cprintf(lin,col,ALIGN_NONE,sfmt.c_str(), 'H', "user history");
    lin++;
    cprintf(lin,col,ALIGN_NONE,sfmt.c_str(), '/', "player list");
    lin++;
    cprintf(lin,col,ALIGN_NONE,sfmt.c_str(), '\\', "full player list");
    lin++;
    cprintf(lin,col,ALIGN_NONE,sfmt.c_str(), '?', "planet list");
    lin++;
    cprintf(lin,col,ALIGN_NONE,sfmt.c_str(), '$', "debugging planet list");
    lin++;
    cprintf(lin,col,ALIGN_NONE,sfmt.c_str(), 'p', "edit a planet");
    lin++;
    cprintf(lin,col,ALIGN_NONE,sfmt.c_str(), 'w', "watch a ship");
    lin++;
    cprintf(lin,col,ALIGN_NONE,sfmt.c_str(), 'i', "info");

    lin = i;
    col = 45;
    cprintf(lin,col,ALIGN_NONE,sfmt.c_str(), 'r', "create a robot ship");
    lin++;
    cprintf(lin,col,ALIGN_NONE,sfmt.c_str(), 'L', "review messages");
    lin++;
    cprintf(lin,col,ALIGN_NONE,sfmt.c_str(), 'm', "message from GOD");
    lin++;
    cprintf(lin,col,ALIGN_NONE,sfmt.c_str(), 'O', "options menu");
    lin++;
    cprintf(lin,col,ALIGN_NONE,sfmt.c_str(), 'T', "team stats");
    lin++;
    cprintf(lin,col,ALIGN_NONE,sfmt.c_str(), 'U', "user stats");
    lin++;
    cprintf(lin,col,ALIGN_NONE,sfmt.c_str(), 'S', "more user stats");
    lin++;
    cprintf(lin,col,ALIGN_NONE,sfmt.c_str(), 's', "special stats page");
    lin++;
    cprintf(lin,col,ALIGN_NONE,sfmt.c_str(), 'a', "add a user");
    lin++;
    cprintf(lin,col,ALIGN_NONE,sfmt.c_str(), 'e', "edit a user");
    lin++;
    cprintf(lin,col,ALIGN_NONE,sfmt.c_str(), 'R', "resign a user");
    lin++;
    cprintf(lin,col,ALIGN_NONE,sfmt.c_str(), 'k', "kiss of death");
    lin++;
    cprintf(lin,col,ALIGN_NONE,sfmt.c_str(), 'q', "exit");

    return;

}

/*  operate - main part of the conquest operator program */
/*  SYNOPSIS */
/*    operate */
void operate(void)
{

    int lin, savelin;
    int redraw, readone;
    int lastrev;
    uint64_t msgrand, now;
    char buf[MSGMAXLINE], junk[MSGMAXLINE];
    char xbuf[MSGMAXLINE];
    int ch;

    cbConqInfo->glastmsg = cbConqInfo->lastmsg;

    lastrev = *cbRevision;
    utGrand( &msgrand );

    redraw = true;
    while (true)      /* repeat */
    {
        if ( redraw || lastrev != *cbRevision )
	{
            lastrev = *cbRevision;
            opback( lastrev, &savelin );
            redraw = false;
	}
        /* Line 1. */

        if (*cbRevision == COMMONSTAMP)
	{
            /* game status */
            if ( SysConf.Closed )
                strcpy(junk , "CLOSED") ;
            else
                strcpy(junk , "open") ;

            /* driver status */

            switch ( cbDriver->drivstat )
	    {
	    case DRS_OFF:
                strcpy(xbuf , "OFF") ;
                break;
	    case DRS_RESTART:
                strcpy(xbuf , "RESTART") ;
                break;
	    case DRS_STARTING:
                strcpy(xbuf , "STARTING") ;
                break;
	    case DRS_HOLDING:
                strcpy(xbuf , "HOLDING") ;
                break;
	    case DRS_RUNNING:
                strcpy(xbuf , "on") ;
                break;
	    case DRS_KAMIKAZE:
                strcpy(xbuf , "KAMIKAZE") ;
                break;
	    default:
                strcpy(xbuf , "???") ;
	    }

            /* eater status */
            if ( !DOOM_LIVE() )
                strcpy(buf , "off") ;
            else
	    {
                // it's alive!
                strcpy(buf , "ON (") ;
                if (cbDoomsday->lock == LOCK_NONE)
                    strcat(buf , "NONE") ;
                else if (cbDoomsday->lock == LOCK_PLANET
                         && cbDoomsday->lockDetail < cbLimits.maxPlanets())
                    strcat(buf , cbPlanets[cbDoomsday->lockDetail].name);
                else if (cbDoomsday->lock == LOCK_SHIP
                         && cbDoomsday->lockDetail < cbLimits.maxShips())
                    utAppendShip(buf, cbDoomsday->lockDetail);

                utAppendChar(buf, ')');
	    }

            lin = savelin;
            cdclrl( lin, 1 );
            cprintf(lin,0,ALIGN_CENTER,"#%d#%s#%d#%s#%d#%s#%d#%s#%d#%s#%d#%s",
                    LabelColor,"game ",InfoColor,junk,LabelColor,", driver ",InfoColor,
                    xbuf,LabelColor,", eater ",InfoColor,buf);

            /* Line 2. */
            strcpy(buf, semGetStatusStr());

            lin++;
            cdclrl( lin, 1 );
            uiPutColor(SpecialColor);
            cdputc( buf, lin );
            uiPutColor(0);


            /* Display a new message, if any. */
            readone = false;
            if ( utDeltaGrand( msgrand, &now ) >= NEWMSG_GRAND )
                if ( utGetMsg( -1, &cbConqInfo->glastmsg ) )
                {
                    mcuReadMsg( cbConqInfo->glastmsg, MSG_MSG );

#if defined(OPER_MSG_BEEP)
                    if (cbMsgs[cbConqInfo->glastmsg].msgfrom != MSG_GOD)
                        cdbeep();
#endif
                    readone = true;
                    msgrand = now;
                }
            cdmove( 1, 1 );
            cdrefresh();
            /* Un-read message, if there's a chance it got garbaged. */
            if ( readone )
                if ( iochav() )
                    cbConqInfo->glastmsg = mod( cbConqInfo->glastmsg - 1, cbLimits.maxMsgs() );

	} /* *cbRevision != COMMONSTAMP */
        else
	{ /* COMMONBLOCK MISMATCH */

            cprintf(4, 0, ALIGN_CENTER, "#%d#You must (I)nitialize (e)verything.",
                    RedLevelColor);
            cdmove( 1, 1 );
            cdrefresh();

	}


        /* Get a char with timeout. */
        if ( ! iogtimed( &ch, 1.0 ) )
            continue;		/* next */
        switch ( ch )
	{
	case 'a':
            opuadd();
            redraw = true;
            break;
	case 'b':
            if ( mcuConfirm() )
                bigbang();
            break;
	case 'd':
            if ( DOOM_LIVE() )
	    {
                DOOMCLR(DOOM_F_LIVE);
                utLog("OPER: %s deactivated the Doomsday machine",
                      operUsername.c_str());
	    }
            else
	    {
                clbDoomsday();
                utLog("OPER: %s has ACTIVATED the Doomsday machine",
                      operUsername.c_str());
	    }
            break;
	case 'e':
            opuedit();
            redraw = true;
            break;
	case 'f':
            if ( SysConf.Closed )
	    {
                SysConf.Closed = false;
                SaveSysConfig();

                utLog("OPER: %s has enabled the game",
                      operUsername.c_str());
	    }
            else if ( mcuConfirm() )
	    {
                SysConf.Closed = true;
                SaveSysConfig();
                utLog("OPER: %s has disabled the game",
                      operUsername.c_str());
	    }
            break;
	case 'h':
            if ( cbDriver->drivstat == DRS_HOLDING )
                cbDriver->drivstat = DRS_RUNNING;
            else
                cbDriver->drivstat = DRS_HOLDING;
            break;
	case 'H':
            mcuHistList();
            redraw = true;
            break;
	case 'i':
            opinfo();
            break;
	case 'I':
            opinit();
            redraw = true;
            break;
	case 'k':
            kiss(0,true);
            break;
	case 'L':
            mcuReviewMsgs( cbConqInfo->glastmsg );
            break;
	case 'm':
            cucSendMsg( MSG_FROM_GOD, 0, true, false );
            break;
	case 'O':
            SysOptsMenu();
            redraw = true;
            break;
	case 'p':
            oppedit();
            redraw = true;
            break;
	case 'q':
	case 'Q':
            return;
            break;
	case 'r':
            oprobot();
            break;
	case 'R':
            opresign();
            redraw = true;
            break;
	case 's':
            opstats();
            redraw = true;
            break;
	case 'S':
            mcuUserStats();
            redraw = true;
            break;
	case 'T':
            opTeamList();
            redraw = true;
            break;
	case 'U':
            mcuUserList();
            redraw = true;
            break;
	case 'w':
            watch();
            operStopTimer();		/* to be sure */
            redraw = true;
            break;
	case '/':
            mcuPlayList( false );
            redraw = true;
            break;
	case '\\':
            mcuPlayList( true );
            redraw = true;
            break;
	case '?':
            opPlanetList();
            redraw = true;
            break;
	case '$':
            debugplan();
            redraw = true;
            break;
	case 0x0c:
            cdredo();
            redraw = true;
            break;
	case ' ':
	case TERM_NORMAL:
	case 0:
            /* do nothing */
	default:
            cdbeep();
	}
        /* Disable messages for awhile. */
        utGrand( &msgrand );
    } /* repeat */

    /* NOTREACHED */

}


/*  opinfo - do an operator info command */
/*  SYNOPSIS */
/*    int snum */
/*    opinfo( snum ) */
void opinfo()
{
    char ch;
    char cbuf[MSGMAXLINE];

    cdclrl( MSG_LIN1, 2 );

    cbuf[0] = 0;
    ch = (char)cdgetx( "Information on: ", MSG_LIN1, 1, TERMS,
                       cbuf, MSGMAXLINE, true );

    info(cbuf, ch, true);

    return;
}


/*  opinit - handle the various kinds of initialization */
/*  SYNOPSIS */
/*    opinit */
void opinit(void)
{

    int i, lin, col, icol;
    char ch, buf[MSGMAXLINE];
    static const char *pmt="Initialize what: ";


    cdclear();
    cdredo();

    lin = 2;
    icol = 11;
    uiPutColor(LabelColor);
    cdputc( "Conquest Initialization", lin );

    lin+=3;
    col = icol - 2;
    strcpy(buf, "(r)obots");
    i = strlen( buf );
    uiPutColor(InfoColor);
    cdputs( buf, lin, col+1 );
    uiPutColor(CQC_A_BOLD);
    cdbox( lin-1, col, lin+1, col+i+1 );
    col = col + i + 4;
    uiPutColor(LabelColor);
    cdputs( "<-", lin, col );
    col = col + 4;
    strcpy(buf, "(e)verything");
    i = strlen( buf );
    uiPutColor(InfoColor);
    cdputs( buf, lin, col+1 );
    uiPutColor(CQC_A_BOLD);
    cdbox( lin-1, col, lin+1, col+i+1 );
    col = col + i + 4;
    uiPutColor(LabelColor);
    cdputs( ".", lin, col );
    col = col + 4;
    strcpy(buf, "(z)ero everything");
    i = strlen( buf );
    uiPutColor(InfoColor);
    cdputs( buf, lin, col+1 );
    uiPutColor(CQC_A_BOLD);
    cdbox( lin-1, col, lin+1, col+i+1 );

    col = icol + 20;
    lin+=3;
    uiPutColor(LabelColor);
    cdput( '|', lin, col );
    lin++;
    cdput( 'v', lin, col );
    lin+=3;

    col = icol;
    strcpy(buf, "(s)hips");
    i = strlen( buf );
    uiPutColor(InfoColor);
    cdputs( buf, lin, col+1 );
    uiPutColor(CQC_A_BOLD);
    cdbox( lin-1, col, lin+1, col+i+1 );
    col = col + i + 4;
    uiPutColor(LabelColor);
    cdputs( "<-", lin, col );
    col = col + 4;
    strcpy(buf, "(u)niverse");
    i = strlen( buf );
    uiPutColor(InfoColor);
    cdputs( buf, lin, col+1 );
    uiPutColor(CQC_A_BOLD);
    cdbox( lin-1, col, lin+1, col+i+1 );
    col = col + i + 4;
    uiPutColor(LabelColor);
    cdputs( ".", lin, col );
    col = col + 4;
    strcpy(buf, "(g)ame");
    i = strlen( buf );
    uiPutColor(InfoColor);
    cdputs( buf, lin, col+1 );
    uiPutColor(CQC_A_BOLD);
    cdbox( lin-1, col, lin+1, col+i+1 );
    col = col + i + 4;
    uiPutColor(LabelColor);
    cdputs( ".", lin, col );
    col = col + 4;
    strcpy(buf, "(p)lanets");
    i = strlen( buf );
    uiPutColor(InfoColor);
    cdputs( buf, lin, col+1 );
    uiPutColor(CQC_A_BOLD);
    cdbox( lin-1, col, lin+1, col+i+1 );

    col = icol + 20;
    lin+=3;
    uiPutColor(LabelColor);
    cdput( '|', lin, col );
    lin++;
    cdput( 'v', lin, col );
    lin+=3;

    col = icol + 15;
    strcpy(buf, "(m)essages");
    i = strlen( buf );
    uiPutColor(InfoColor);
    cdputs( buf, lin, col+1 );
    uiPutColor(CQC_A_BOLD);
    cdbox( lin-1, col, lin+1, col+i+1 );
    col = col + i + 8;
    strcpy(buf, "(l)ockwords");
    i = strlen( buf );
    uiPutColor(InfoColor);
    cdputs( buf, lin, col+1 );
    uiPutColor(CQC_A_BOLD);
    cdbox( lin-1, col, lin+1, col+i+1 );

    while (true)  /*repeat */
    {
        lin = MSG_LIN1;
        col = 30;
        cdclrl( lin, 1 );
        uiPutColor(InfoColor);
        buf[0] = 0;
        ch = (char)cdgetx( pmt, lin, col, TERMS, buf, MSGMAXLINE, true );
        cdclrl( lin, 1 );
        cdputs( pmt, lin, col );
        uiPutColor(0);
        col = col + strlen( pmt );
        if ( ch == TERM_ABORT || buf[0] == 0 )
            break;
        switch ( buf[0] )
	{
	case 'e':
            cdputs( "everything", lin, col );
            if ( mcuConfirm() )
	    {
                DoInit('e', false);
	    }
            break;
	case 'z':
            cdputs( "zero everything", lin, col );
            if ( mcuConfirm() )
                DoInit('z', false);
            break;
	case 'u':
            cdputs( "universe", lin, col );
            if ( mcuConfirm() )
	    {
                DoInit('u', false);
	    }
            break;
	case 'g':
            cdputs( "game", lin, col );
            if ( mcuConfirm() )
	    {
                DoInit('g', false);
	    }
            break;
	case 'p':
            cdputs( "planets", lin, col );
            if ( mcuConfirm() )
	    {
                DoInit('p', false);
	    }
            break;
	case 's':
            cdputs( "ships", lin, col );
            if ( mcuConfirm() )
	    {
                DoInit('s', false);
	    }
            break;
	case 'm':
            cdputs( "messages", lin, col );
            if ( mcuConfirm() )
	    {
                DoInit('m', false);
	    }
            break;
	case 'l':
            cdputs( "lockwords", lin, col );
            if ( mcuConfirm() )
	    {
                DoInit('l', false);
	    }
            break;
	case 'r':
            cdputs( "robots", lin, col );
            if ( mcuConfirm() )
	    {
                DoInit('r', false);
	    }
            break;
	default:
            cdbeep();
	}
    }

    return;

}


/*  oppedit - edit a planet's characteristics */
/*  SYNOPSIS */
/*    oppedit */
void oppedit(void)
{

    int i, j, lin, col, datacol;
    static int pnum = 0;
    real x;
    int ch;
    char buf[MSGMAXLINE];
    int attrib;

    static bool firstTime = true;
    static char sfmt[MSGMAXLINE * 2];

    if (firstTime)
    {
        firstTime = false;
        sprintf(sfmt,
                "#%d#(#%d#%%s#%d#)   %%s",
                LabelColor,
                InfoColor,
                LabelColor);
    }


    col = 4;
    datacol = col + 28;

    cdredo();
    cdclear();
    while (true) /*repeat*/
    {

        /* Display the planet. */
        i = 20;			/* i = 10 */
        j = 57;


        /* colorize planet name according to type */
        if ( cbPlanets[pnum].type == PLANET_SUN )
            attrib = RedLevelColor;
        else if ( cbPlanets[pnum].type == PLANET_CLASSM )
            attrib = GreenLevelColor;
        else if ( cbPlanets[pnum].type == PLANET_DEAD )
            attrib = YellowLevelColor;
        else if ( cbPlanets[pnum].type == PLANET_CLASSA ||
                  cbPlanets[pnum].type == PLANET_CLASSO ||
                  cbPlanets[pnum].type == PLANET_CLASSZ ||
                  cbPlanets[pnum].type == PLANET_GHOST )   /* white as a ghost ;-) */
            attrib = CQC_A_BOLD;
        else
            attrib = SpecialColor;

        /* if we're doing a sun, use yellow
           else use attrib set above */
        if (cbPlanets[pnum].type == PLANET_SUN)
            uiPutColor(YellowLevelColor);
        else
            uiPutColor(attrib);
        mcuPutThing(cbPlanets[pnum].type, i, j );
        uiPutColor(0);

        /* suns have red cores, others are cyan. */
        if (cbPlanets[pnum].type == PLANET_SUN)
            uiPutColor(RedLevelColor);
        else
            uiPutColor(InfoColor);
        cdput( cbConqInfo->chrplanets[cbPlanets[pnum].type], i, j + 1);
        uiPutColor(0);

        sprintf(buf, "%s\n", cbPlanets[pnum].name);
        uiPutColor(attrib);
        cdputs( buf, i + 1, j + 2 ); /* -[] */
        uiPutColor(0);

        /* Display info about the planet. */
        lin = 4;
        i = pnum;
        cprintf(lin,col,ALIGN_NONE,sfmt, "p", "  Planet:\n");
        cprintf( lin,datacol,ALIGN_NONE,"#%d#%s (%d)",
                 attrib, cbPlanets[pnum].name, pnum );

        lin++;
        i = cbPlanets[pnum].primary;
        if ( i == pnum ) // orbiting itself
	{
            lin++;

            x = cbPlanets[pnum].orbvel;
            if ( x == 0.0 )
                cprintf(lin,col,ALIGN_NONE,sfmt, "v", "  Stationary\n");
            else
	    {
                cprintf(lin,col,ALIGN_NONE,sfmt, "v", "Velocity:\n");
                cprintf( lin,datacol,ALIGN_NONE,"#%d#Warp %f",
                         InfoColor, oneplace(x) );
	    }
	}
        else
	{
            cprintf(lin,col,ALIGN_NONE,sfmt, "o", "  Orbiting:\n");
            cprintf( lin,datacol,ALIGN_NONE,"#%d#%s (%d)",
                     InfoColor, cbPlanets[i].name, i );

            lin++;
            cprintf(lin,col,ALIGN_NONE,sfmt, "v", "  Orbit velocity:\n");
            cprintf( lin,datacol,ALIGN_NONE,"#%d#%.1f degrees/minute",
                     InfoColor, cbPlanets[pnum].orbvel );
	}

        lin++;
        cprintf(lin,col,ALIGN_NONE,sfmt, "r", "  Radius:\n");
        cprintf( lin,datacol,ALIGN_NONE, "#%d#%.1f", InfoColor, cbPlanets[pnum].orbrad );

        lin++;
        cprintf(lin,col,ALIGN_NONE,sfmt, "a", "  Angle:\n");
        cprintf( lin,datacol,ALIGN_NONE, "#%d#%.1f", InfoColor, cbPlanets[pnum].orbang );

        lin++;
        cprintf(lin,col,ALIGN_NONE,sfmt, "S", "  Size:\n");
        cprintf( lin,datacol,ALIGN_NONE, "#%d#%hu", InfoColor, cbPlanets[pnum].size );

        lin++;
        i = cbPlanets[pnum].type;
        cprintf(lin,col,ALIGN_NONE,sfmt, "t", "  Type:\n");
        cprintf( lin, datacol, ALIGN_NONE,
                 "#%d#%s (%d)", InfoColor, cbConqInfo->ptname[i], i );

        lin++;
        i = cbPlanets[pnum].team;
        cprintf(lin,col,ALIGN_NONE,sfmt, "T", "  Owner team:\n");
        cprintf( lin,datacol,ALIGN_NONE, "#%d#%s (%d)",
                 InfoColor,cbTeams[i].name, i );

        lin++;
        cprintf(lin,col,ALIGN_NONE,sfmt, "x,y", "Position:\n");
        cprintf( lin,datacol,ALIGN_NONE, "#%d#%.1f, %.1f",
                 InfoColor, cbPlanets[pnum].x, cbPlanets[pnum].y );

        lin++;
        cprintf(lin,col,ALIGN_NONE,sfmt, "A", "  Armies:\n");
        cprintf( lin,datacol,ALIGN_NONE, "#%d#%d", InfoColor, cbPlanets[pnum].armies );

        lin++;
        cprintf(lin,col,ALIGN_NONE,sfmt, "s", "  Scanned by:\n");
        buf[0] = '(';
        for ( i = 1; i <= NUMPLAYERTEAMS; i = i + 1 )
            if ( cbPlanets[pnum].scanned[i - 1] )
                buf[i] = cbTeams[i - 1].teamchar;
            else
                buf[i] = '-';
        buf[NUMPLAYERTEAMS+1] = ')';
        buf[NUMPLAYERTEAMS+2] = '\0';
        cprintf( lin, datacol,ALIGN_NONE, "#%d#%s",InfoColor, buf);

        lin++;
        cprintf(lin,col,ALIGN_NONE,sfmt, "u", "  Uninhabitable time:\n");
        cprintf( lin,datacol,ALIGN_NONE, "#%d#%d",
                 InfoColor, cbPlanets[pnum].uninhabtime );

        lin++;
        if ( PVISIBLE(pnum) )
            cprintf(lin,col,ALIGN_NONE,sfmt, "-", "  Visible\n");
        else
            cprintf(lin,col,ALIGN_NONE,sfmt, "+", "  Hidden\n");

        lin++;
        cprintf(lin,col,ALIGN_NONE,sfmt, "n", "  Change planet name\n");

        lin++;
        cprintf(lin,col,ALIGN_NONE,sfmt, "<>",
		" decrement/increment planet number\n");

        cdclra(MSG_LIN1, 0, MSG_LIN1 + 2, Context.maxcol - 1);

        cdmove( 0, 0 );
        cdrefresh();

        if ( ! iogtimed( &ch, 1.0 ) )
            continue;		/* next */
        switch ( ch )
	{
	case 'a':
            /* Angle. */
            ch = mcuGetCX( "New angle? ", MSG_LIN1, 0,
                           TERMS, buf, MSGMAXLINE );
            if ( ch == TERM_ABORT || buf[0] == 0 )
                continue;	/* next */
            utDeleteBlanks( buf );
            if ( ! utSafeCToI( &j, buf, 0 ) )
                continue;	/* next */

            x = atof( buf);
            if ( x < 0.0 || x > 360.0 )
                continue;	/* next */
            cbPlanets[pnum].orbang = x;
            break;
	case 'A':
            /* Armies. */
            ch = mcuGetCX( "New number of armies? ",
                           MSG_LIN1, 0, TERMS, buf, MSGMAXLINE );
            if ( ch == TERM_ABORT || buf[0] == 0 )
                continue;
            utDeleteBlanks( buf );
            if ( ! utSafeCToI( &j, buf, 0 ) )
                continue;
            cbPlanets[pnum].armies = j;
            break;
	case 'n':
            /* New planet name. */
            ch = mcuGetCX( "New name for this planet? ",
                           MSG_LIN1, 0, TERMS, buf, MAXPLANETNAME );
            if ( ch != TERM_ABORT && ( ch == TERM_EXTRA || buf[0] != 0 ) )
                utStrncpy( cbPlanets[pnum].name, buf, MAXPLANETNAME );
            break;
	case 'o':
            /* New primary. */
            ch = mcuGetCX( "New planet to orbit? ",
                           MSG_LIN1, 0, TERMS, buf, MAXPLANETNAME );
            if ( ch == TERM_ABORT || buf[0] == 0 )
                continue;	/* next */
            if ( opPlanetMatch( buf, &i ) )
                cbPlanets[pnum].primary = i;
            break;
	case 'v':
            /* Velocity. */
            ch = mcuGetCX( "New velocity? ",
                           MSG_LIN1, 0, TERMS, buf, MSGMAXLINE );
            if ( ch == TERM_ABORT || buf[0] == 0 )
                continue;	/* next */
            utDeleteBlanks( buf );
            if ( ! utSafeCToI( &j, buf, 0 ) )
                continue;	/* next */

            cbPlanets[pnum].orbvel = atof( buf );
            break;
	case 'S':
            /* Size. */
            ch = mcuGetCX( "New size? ", MSG_LIN1, 0,
                           TERMS, buf, MSGMAXLINE );
            if ( ch == TERM_ABORT || buf[0] == 0 )
                continue;	/* next */
            utDeleteBlanks( buf );
            if ( ! utSafeCToI( &j, buf, 0 ) )
                continue;	/* next */

            x = atof( buf);
            if ( x < 1.0 )
                continue;	/* next */
            cbPlanets[pnum].size = x;
            break;

	case 'T':
            /* Rotate owner team. */
            cbPlanets[pnum].team =
                mod( cbPlanets[pnum].team + 1, NUMALLTEAMS );
            break;
	case 't':
            /* Rotate planet type. */
            cbPlanets[pnum].type =
                static_cast<PlanetType>(mod( cbPlanets[pnum].type + 1,
                                             MAXPLANETTYPES ));
            break;
	case 'x':
            /* X coordinate. */
            ch = mcuGetCX( "New X coordinate? ",
                           MSG_LIN1, 0, TERMS, buf, MSGMAXLINE );
            if ( ch == TERM_ABORT || buf[0] == 0 )
                continue;	/* next */
            utDeleteBlanks( buf );

            if ( ! utSafeCToI( &j, buf, 0 ) )
                continue;	/* next */

            cbPlanets[pnum].x = atof( buf );
            break;
	case 'y':
            /* Y coordinate. */
            ch = mcuGetCX( "New Y coordinate? ",
                           MSG_LIN1, 0, TERMS, buf, MSGMAXLINE );
            if ( ch == TERM_ABORT || buf[0] == 0 )
                continue;	/* next */
            utDeleteBlanks( buf );
            if ( ! utSafeCToI( &j, buf, 0 ) )
                continue;	/* next */

            cbPlanets[pnum].y = atof( buf );
            break;
	case 's':
            /* Scanned. */
            cdputs( "Toggle which team? ", MSG_LIN1, 1 );
            cdmove( MSG_LIN1, 20 );
            cdrefresh();
            ch = (char)toupper( iogchar() );
            for ( i = 0; i < NUMPLAYERTEAMS; i = i + 1 )
                if ( ch == cbTeams[i].teamchar )
                {
                    cbPlanets[pnum].scanned[i] = ! cbPlanets[pnum].scanned[i];
                    break;
                }
            break;
            /* Uninhabitable minutes */
	case 'u':
            ch = mcuGetCX( "New uninhabitable minutes? ",
                           MSG_LIN1, 0, TERMS, buf, MSGMAXLINE );
            if ( ch == TERM_ABORT || buf[0] == 0 )
                continue;
            utDeleteBlanks( buf );
            if ( ! utSafeCToI( &j, buf, 0 ) )
                continue;
            cbPlanets[pnum].uninhabtime = j;
            break;
	case 'p':
            ch = mcuGetCX( "New planet to edit? ",
                           MSG_LIN1, 0, TERMS, buf, MAXPLANETNAME );
            if ( ch == TERM_ABORT || buf[0] == 0 )
                continue;	/* next */
            if ( opPlanetMatch( buf, &i ) )
                pnum = i;
            break;
	case 'r':
            /* Radius. */
            ch = mcuGetCX( "New radius? ",
                           MSG_LIN1, 0, TERMS, buf, MSGMAXLINE );
            if ( ch == TERM_ABORT || buf[0] == 0 )
                continue;	/* next */
            utDeleteBlanks( buf );
            if ( ! utSafeCToI( &j, buf, 0 ) )
                continue;	/* next */

            cbPlanets[pnum].orbrad = atof( buf );
            break;
	case '+':
            /* Now you see it... */
            PFSET(pnum, PLAN_F_VISIBLE);
            break;
	case '-':
            /* Now you don't */
            PFCLR(pnum, PLAN_F_VISIBLE);
            break;
	case '>': /* forward rotate planet number - dwp */
	case KEY_RIGHT:
	case KEY_UP:
            pnum = mod(pnum + 1, cbLimits.maxPlanets());
            break;
	case '<':  /* reverse rotate planet number - dwp */
	case KEY_LEFT:
	case KEY_DOWN:
            pnum = ((pnum == 0) ? cbLimits.maxPlanets() - 1 : pnum - 1);
            pnum = mod(pnum, cbLimits.maxPlanets());
            break;
	case ' ':
            /* do no-thing */
            break;
	case 0x0c:
            cdredo();
            break;
	case TERM_NORMAL:
	case TERM_ABORT:
	case 'q':
	case 'Q':
            return;
            break;
	default:
            cdbeep();
	}
    }/* while (true) */

    /* NOTREACHED */

}


/*  opplanlist - display the planet list for an operator */
/*  SYNOPSIS */
/*    opplanlist */
void opPlanetList(void)
{
    mcuPlanetList( );
}


/*  opresign - resign a user */
/*  SYNOPSIS */
/*    opresign */
void opresign(void)
{

    int unum;
    char ch, buf[MSGMAXLINE];

    cdclrl( MSG_LIN1, 2 );
    buf[0] = 0;
    ch = (char)cdgetx( "Resign user: ", MSG_LIN1, 1, TERMS, buf, MSGMAXLINE,
                       true);
    if ( ch == TERM_ABORT )
    {
        cdclrl( MSG_LIN1, 1 );
        return;
    }

    if ( ! clbGetUserNum( &unum, buf, USERTYPE_ANY ) )
    {
        cdputs( "No such user.", MSG_LIN2, 1 );
        cdmove( 1, 1 );
        cdrefresh();
        utSleep( 1.0 );
    }
    else if ( mcuConfirm() )
    {
        utLog("OPER: %s has resigned %s (%s)",
              operUsername.c_str(),
              cbUsers[unum].username,
              cbUsers[unum].alias);

        clbResign( unum, true );
    }
    cdclrl( MSG_LIN1, 2 );

    return;

}


/*  oprobot - handle gratuitous robot creation */
/*  SYNOPSIS */
/*    oprobot */
void oprobot(void)
{

    int i, j, snum, unum, num, anum;
    char ch, buf[MSGMAXLINE];
    int warlike;
    char xbuf[MSGMAXLINE];

    cdclrl( MSG_LIN1, 2 );
    buf[0] = 0;
    ch = (char)cdgetx( "Enter username for new robot (Orion, Federation, etc): ",
                       MSG_LIN1, 1, TERMS, buf, MAX_USERNAME, true );
    if ( ch == TERM_ABORT || buf[0] == 0 )
    {
        cdclrl( MSG_LIN1, 1 );
        return;
    }
    /* catch lowercase, uppercase typos - dwp */
    strcpy(xbuf, buf);
    j = strlen(xbuf);
    buf[0] = (char)toupper(xbuf[0]);
    if (j>1)
  	for (i=1;i<j && xbuf[i] != 0;i++)
            buf[i] = (char)tolower(xbuf[i]);

    if ( ! clbGetUserNum( &unum, buf, USERTYPE_BUILTIN ) )
    {
        char *uptr = buf;
        /* un-upper case first char and
           try again */
        if (*uptr == '@')
            uptr++;
        uptr[0] = (char)tolower(uptr[0]);
        if ( ! clbGetUserNum( &unum, buf, USERTYPE_BUILTIN ) )
	{
            cdputs( "No such user.", MSG_LIN2, 1 );
            return;
	}
    }

    /* Defaults. */
    num = 1;
    warlike = false;

    if ( ch == TERM_EXTRA )
    {
        buf[0] = 0;
        ch = (char)cdgetx( "Enter number desired ([TAB] for warlike): ",
                           MSG_LIN2, 1, TERMS, buf, MAX_USERNAME, true );
        if ( ch == TERM_ABORT )
	{
            cdclrl( MSG_LIN1, 2 );
            return;
	}
        warlike = ( ch == TERM_EXTRA );
        utDeleteBlanks( buf );
        utSafeCToI( &num, buf, 0 );
        if ( num <= 0 )
            num = 1;
    }

    anum = 0;
    for ( i = 1; i <= num; i = i + 1 )
    {
        if ( ! newrob( &snum, unum ) )
	{
            uiPutMsg( "Failed to create robot ship.", MSG_LIN1 );
            break;
	}

        anum = anum + 1;

        /* If requested, make the robot war-like. */
        if ( warlike )
	{
            for ( j = 0; j < NUMPLAYERTEAMS; j = j + 1 )
                cbShips[snum].war[j] = true;
            cbShips[snum].war[cbShips[snum].team] = false;
	}
    }

    /* Report the good news. */
    utLog("OPER: %s created %d %s%s (%s) robot(s)",
          operUsername.c_str(),
          anum,
          (warlike == true) ? "WARLIKE " : "",
          cbUsers[unum].alias,
          cbUsers[unum].username);

    sprintf( buf, "Automation %s (%s) is now flying ",
             cbUsers[unum].alias, cbUsers[unum].username );
    if ( anum == 1 )
        utAppendShip(buf , snum) ;
    else
    {
        utAppendInt(buf , anum) ;
        strcat(buf , " new ships.") ;
    }
    cdclrl( MSG_LIN2, 1 );
    i = MSG_LIN1;
    if ( anum != num )
        i = i + 1;
    uiPutMsg( buf, i );

    return;

}


/*  opstats - display operator statistics */
/*  SYNOPSIS */
/*    opstats */
void opstats(void)
{

    int i, lin, col;
    char buf[MSGMAXLINE], junk[MSGMAXLINE*2], timbuf[32];
    int ch;
    real x;
    static const char *sfmt="#%d#%32s #%d#%12s\n";
    static const char *tfmt="#%d#%32s #%d#%20s\n";
    static const char *pfmt="#%d#%32s #%d#%11.1f%%\n";

    col = 8;
    cdclear();
    do /*repeat*/
    {
        lin = 2;
        utFormatSeconds( cbConqInfo->ccpuseconds, timbuf );
        cprintf( lin,col,ALIGN_NONE,sfmt,
                 LabelColor,"Conquest cpu time:", InfoColor,timbuf );

        lin++;
        i = cbConqInfo->celapsedseconds;
        utFormatSeconds( i, timbuf );
        cprintf( lin,col,ALIGN_NONE,sfmt,
                 LabelColor,"Conquest elapsed time:", InfoColor,timbuf );

        lin++;
        if ( i == 0 )
            x = 0.0;
        else
            x = oneplace( 100.0 * (real)cbConqInfo->ccpuseconds / (real)i );
        cprintf( lin,col,ALIGN_NONE,pfmt,
                 LabelColor,"Conquest cpu usage:", InfoColor,x);

        lin+=2;
        utFormatSeconds( cbConqInfo->dcpuseconds, timbuf );
        cprintf( lin,col,ALIGN_NONE,sfmt,
                 LabelColor,"Conqdriv cpu time:", InfoColor,timbuf );

        lin++;
        i = cbConqInfo->delapsedseconds;
        utFormatSeconds( i, timbuf );
        cprintf( lin,col,ALIGN_NONE,sfmt,
                 LabelColor,"Conqdriv elapsed time:", InfoColor,timbuf );

        lin++;
        if ( i == 0 )
            x = 0.0;
        else
            x = oneplace( 100.0 * (real)cbConqInfo->dcpuseconds / (real)i );
        cprintf( lin,col,ALIGN_NONE,pfmt,
                 LabelColor,"Conqdriv cpu usage:", InfoColor,x);

        lin+=2;
        utFormatSeconds( cbConqInfo->rcpuseconds, timbuf );
        cprintf( lin,col,ALIGN_NONE,sfmt,
                 LabelColor,"Robot cpu time:", InfoColor,timbuf );

        lin++;
        i = cbConqInfo->relapsedseconds;
        utFormatSeconds( i, timbuf );
        cprintf( lin,col,ALIGN_NONE,sfmt,
                 LabelColor,"Robot elapsed time:", InfoColor,timbuf );

        lin++;
        if ( i == 0 )
            x = 0.0;
        else
            x = ( 100.0 * (real)cbConqInfo->rcpuseconds / (real)i );
        cprintf( lin, col, ALIGN_NONE, pfmt,
                 LabelColor, "Robot cpu usage:", InfoColor, x);

        lin+=2;
        cprintf( lin, col, ALIGN_NONE, tfmt,
                 LabelColor, "Last initialize:", InfoColor, cbConqInfo->inittime);

        lin++;
        cprintf( lin, col, ALIGN_NONE, tfmt,
                 LabelColor, "Last conquer:", InfoColor, cbConqInfo->conqtime);

        lin++;
        utFormatSeconds( cbDriver->drivtime, timbuf );
        cprintf( lin, col, ALIGN_NONE, sfmt,
                 LabelColor, "UTC Driver time:", InfoColor, timbuf);

        lin++;
        utFormatSeconds( cbDriver->playtime, timbuf );
        cprintf( lin, col, ALIGN_NONE, sfmt,
                 LabelColor, "UTC Play time:", InfoColor, timbuf);

        lin++;
        cprintf( lin, col, ALIGN_NONE, tfmt,
                 LabelColor, "Last upchuck:", InfoColor, cbConqInfo->lastupchuck);

        lin++;
        utFormatTime( timbuf, 0 );
        cprintf( lin, col, ALIGN_NONE, tfmt,
                 LabelColor, "Current time:", InfoColor, timbuf);

        lin+=2;
        if ( cbDriver->drivowner[0] != 0 )
            sprintf( junk, "%d #%d#(#%d#%s#%d#)",
                     cbDriver->drivpid, LabelColor, SpecialColor,
                     cbDriver->drivowner, LabelColor );
        else if ( cbDriver->drivpid != 0 )
            sprintf( junk, "%d", cbDriver->drivpid );
        else
            junk[0] = 0;

        if (junk[0] == 0)
            cprintf( lin,col,ALIGN_NONE,
                     "#%d#drivsecs = #%d#%03d#%d#, drivcnt = #%d#%d\n",
                     LabelColor, InfoColor,
                     cbDriver->drivsecs,LabelColor,InfoColor,cbDriver->drivcnt);
        else
            cprintf( lin,col,ALIGN_NONE,
                     "#%d#%s#%d#, drivsecs = #%d#%03d#%d#, drivcnt = #%d#%d\n",
                     InfoColor,junk,LabelColor,InfoColor,
                     cbDriver->drivsecs,LabelColor,InfoColor,cbDriver->drivcnt);

        lin++;
        cprintf( lin,col,ALIGN_NONE,
                 "#%d#%u #%d#bytes in the common block.\n",
                 InfoColor,
                 cbGetSize(),
                 LabelColor);

        lin++;
        sprintf( buf, "#%d#Common ident is #%d#%d",
                 LabelColor,InfoColor,*cbRevision);
        if ( *cbRevision != COMMONSTAMP )
	{
            snprintf( junk, sizeof(junk),
                      " #%d#(binary ident is #%d#%d#%d#)\n",
                     LabelColor,InfoColor,COMMONSTAMP,LabelColor );
            utStrncat(buf, junk, sizeof(buf));
	}
        cprintf( lin,col,ALIGN_NONE, "%s",buf);

        cdmove( 1, 1 );
        uiPutColor(0);
        cdrefresh();
    }
    while ( !iogtimed( &ch, 1.0 ) ); /* until */

    return;

}


/*  opteamlist - display the team list for an operator */
/*  SYNOPSIS */
/*    opteamlist */
void opTeamList(void)
{

    int ch;

    cdclear();
    do /* repeat*/
    {
        mcuTeamList();
        mcuPutPrompt( MTXT_DONE, MSG_LIN2 );
        cdrefresh();
    }
    while ( !iogtimed( &ch, 1.0 ) ); /* until */

}


/*  opuadd - add a user */
/*  SYNOPSIS */
/*    opuadd */
void opuadd(void)
{

    int i, unum, team;
    char ch;
    char buf[MSGMAXLINE], junk[MSGMAXLINE], name[MSGMAXLINE];

    cdclrl( MSG_LIN1, 2 );
    name[0] = 0;
    ch = (char)cdgetx( "Add user: ", MSG_LIN1, 1, TERMS, name, MAX_USERNAME,
                       true);
    /*  utDeleteBlanks( name );*/

    if ( ch == TERM_ABORT || name[0] == 0 )
    {
        cdclrl( MSG_LIN1, 1 );
        return;
    }
    if ( clbGetUserNum( &unum, name, USERTYPE_NORMAL ) )
    {
        cdputs( "That user is already enrolled.", MSG_LIN2, 1 );
        cdmove( 1, 1 );
        cdrefresh();
        utSleep( 1.0 );
        cdclrl( MSG_LIN1, 2 );
        return;
    }
    for ( team = -1; team == -1; )
    {
        sprintf(junk, "Select a team (%c%c%c%c): ",
                cbTeams[TEAM_FEDERATION].teamchar,
                cbTeams[TEAM_ROMULAN].teamchar,
                cbTeams[TEAM_KLINGON].teamchar,
                cbTeams[TEAM_ORION].teamchar);

        cdclrl( MSG_LIN1, 1 );
        buf[0] = 0;
        ch = (char)cdgetx( junk, MSG_LIN1, 1, TERMS, buf, MSGMAXLINE, true );
        if ( ch == TERM_ABORT )
	{
            cdclrl( MSG_LIN1, 1 );
            return;
	}
        else if ( ch == TERM_EXTRA && buf[0] == 0 )
            team = rndint( 0, NUMPLAYERTEAMS - 1);
        else
	{
            ch = (char)toupper( buf[0] );
            for ( i = 0; i < NUMPLAYERTEAMS; i++ )
                if ( cbTeams[i].teamchar == ch )
                {
                    team = i;
                    break;
                }
	}
    }


    std::string regBuf, nameBuf(name);

    nameBuf[0] = ::toupper(name[0]);

    utAppendTitle(regBuf, team);
    regBuf += ' ';
    regBuf += nameBuf;

    if ( ! clbRegister( name, regBuf.c_str(), team, &unum ) )
    {
        cdputs( "Error adding new user.", MSG_LIN2, 1 );
        cdmove( 0, 0 );
        cdrefresh();
        utSleep( 1.0 );
    }
    else
    {
        utLog("OPER: %s added user '%s'.",
              operUsername.c_str(), name);

    }
    cdclrl( MSG_LIN1, 2 );


    ChangePassword(unum, true);

    return;

}


/*  opuedit - edit a user */
/*  SYNOPSIS */
/*    opuedit */
void opuedit(void)
{

#define MAXUEDITROWS (12+2)

    int i, unum, row = 1, lin, olin, tcol, dcol, lcol, rcol;
    char buf[MSGMAXLINE];
    int ch, left = true;
    char datestr[MAXDATESIZE];
    static const char *prompt2 = "any other key to quit.";
    static const char *rprompt =
        "Use arrow keys to position, [SPACE] to modify, [TAB] to change password";

    cdclrl( MSG_LIN1, 2 );
    uiPutColor(InfoColor);
    ch = mcuGetCX( "Edit which user: ", MSG_LIN1, 0, TERMS, buf, MAX_USERNAME );
    if ( ch == TERM_ABORT )
    {
        cdclrl( MSG_LIN1, 2 );
        uiPutColor(0);
        return;
    }
    /*  utDeleteBlanks( buf );*/

    if ( ! clbGetUserNum( &unum, buf, USERTYPE_ANY ) )
    {
        cdclrl( MSG_LIN1, 2 );
        cdputs( "Unknown user.", MSG_LIN1, 1 );
        uiPutColor(0);
        cdmove( 1, 1 );
        cdrefresh();
        utSleep( 1.0 );
        return;
    }
    cdclear();
    cdclrl( MSG_LIN1, 2 );

    while (true) /* repeat */
    {
        /* Do the right side first. */
        cdclrl( 1, MSG_LIN1 -1 );

        lin = 1;
        tcol = 43;
        dcol = 62;
        rcol = dcol - 1;

        cprintf(lin,tcol,ALIGN_NONE,"#%d#%s", LabelColor,"         Username:");
        cprintf(lin,dcol,ALIGN_NONE,"#%d#%s", InfoColor, cbUsers[unum].username);

        lin++;
        // was multiple count, reuse someday...
        cprintf(lin,tcol,ALIGN_NONE,"#%d#%s", LabelColor,"                 ");
        //cprintf(lin,dcol,ALIGN_NONE,"#%d#%0d", InfoColor, 0);

        lin++;
        // room for 12
        for ( i = 0; i < 12; i++ )
	{
            cprintf(lin+i,tcol,ALIGN_NONE,"#%d#%17d:", LabelColor,i);
            cprintf(lin+i,dcol,ALIGN_NONE,"#%d#%c", RedLevelColor,'F');
	}

        lin+=(12 + 1);
        cprintf(lin,tcol,ALIGN_NONE,"#%d#%s", LabelColor,"          Urating:");
        cprintf(lin,dcol,ALIGN_NONE,"#%d#%0g",InfoColor,
                oneplace(cbUsers[unum].rating));

        lin++;
        cprintf(lin,tcol,ALIGN_NONE,"#%d#%s", LabelColor,"             Uwar:");
        buf[0] = '(';
        for ( i = 0; i < NUMPLAYERTEAMS; i = i + 1 )
            if ( cbUsers[unum].war[i] )
                buf[i+1] = cbTeams[i].teamchar;
            else
                buf[i+1] = '-';
        buf[NUMPLAYERTEAMS+1] = ')';
        buf[NUMPLAYERTEAMS+2] = 0;
        cprintf(lin,dcol,ALIGN_NONE,"#%d#%s",InfoColor, buf);

        lin++;
        cprintf(lin,tcol,ALIGN_NONE,"#%d#%s", LabelColor,"           Urobot:");
        if ( UROBOT(unum) )
            cprintf(lin,dcol,ALIGN_NONE,"#%d#%c",GreenLevelColor, 'T');
        else
            cprintf(lin,dcol,ALIGN_NONE,"#%d#%c",RedLevelColor, 'F');

        /* Now the left side. */
        lin = 1;
        tcol = 3;
        dcol = 22;
        lcol = dcol - 1;

        cprintf(lin,tcol,ALIGN_NONE,"#%d#%s", LabelColor,"             Name:");


        cprintf(lin,dcol,ALIGN_NONE,"#%d#%s",InfoColor,
                cbUsers[unum].alias);

        lin++;
        cprintf(lin,tcol,ALIGN_NONE,"#%d#%s", LabelColor,"             Team:");
        i = cbUsers[unum].team;
        if ( i < 0 || i >= NUMPLAYERTEAMS )
            cprintf(lin,dcol,ALIGN_NONE,"#%d#%0d",InfoColor, i);
        else
            cprintf(lin,dcol,ALIGN_NONE,"#%d#%s",InfoColor, cbTeams[i].name);

        lin++;
        // room for 12 operator options
        for ( i = 0; i < 12; i++ )
	{
            switch(i) {
            case 0: // play when closed
                if (UPLAYWHENCLOSED(unum))
                    cprintf(lin+i,dcol,ALIGN_NONE,"#%d#%c",
                            GreenLevelColor,'T');
                else
                    cprintf(lin+i,dcol,ALIGN_NONE,"#%d#%c",
                            RedLevelColor,'F');
                break;
            case 1: // banned
                if (UBANNED(unum))
                    cprintf(lin+i,dcol,ALIGN_NONE,"#%d#%c",
                            GreenLevelColor,'T');
                else
                    cprintf(lin+i,dcol,ALIGN_NONE,"#%d#%c",
                            RedLevelColor,'F');
                break;
            case 2: // Operator
                if (UISOPER(unum))
                    cprintf(lin+i,dcol,ALIGN_NONE,"#%d#%c",
                            GreenLevelColor,'T');
                else
                    cprintf(lin+i,dcol,ALIGN_NONE,"#%d#%c",
                            RedLevelColor,'F');
                break;
            case 3: // autopilot
                if (UAUTOPILOT(unum))
                    cprintf(lin+i,dcol,ALIGN_NONE,"#%d#%c",
                            GreenLevelColor,'T');
                else
                    cprintf(lin+i,dcol,ALIGN_NONE,"#%d#%c",
                            RedLevelColor,'F');
                break;
            default:
                // do nothing
                break;
            }
	}

        // operator settable options...

        // line offset 0 - play when closed
        cprintf(lin+0,tcol,ALIGN_NONE,"#%d#%s",
		LabelColor," Play when closed:");
        // 1 - banned?
        cprintf(lin+1,tcol,ALIGN_NONE,"#%d#%s",
		LabelColor,"           Banned:");
        // 2 - operator
        cprintf(lin+2,tcol,ALIGN_NONE,"#%d#%s",
		LabelColor,"Conquest Operator:");
        // 3 - autopilot
        cprintf(lin+3,tcol,ALIGN_NONE,"#%d#%s",
		LabelColor,"        Autopilot:");

        lin+=(12 + 1); /* 12 == old maxooptions */
        cprintf(lin,tcol,ALIGN_NONE,"#%d#%s", LabelColor,"       Last entry:");

        if (cbUsers[unum].lastentry == 0)
            strcpy(datestr, "never");
        else
            utFormatTime(datestr, cbUsers[unum].lastentry);

        cprintf(lin,dcol,ALIGN_NONE,"#%d#%s",InfoColor,
                datestr);

        lin++;
        cprintf(lin,tcol,ALIGN_NONE,"#%d#%s", LabelColor,"  Elapsed seconds:");
        utFormatSeconds( cbUsers[unum].stats[TSTAT_SECONDS], buf );
        i = dcol + 11 - strlen( buf );
        cprintf(lin,i,ALIGN_NONE,"#%d#%s",InfoColor, buf);

        lin++;
        cprintf(lin,tcol,ALIGN_NONE,"#%d#%s", LabelColor,"      Cpu seconds:");
        utFormatSeconds( cbUsers[unum].stats[TSTAT_CPUSECONDS], buf );
        i = dcol + 11 - strlen ( buf );
        cprintf(lin,i,ALIGN_NONE,"#%d#%s",InfoColor, buf);

        lin++;

        /* Do column 4 of the bottom stuff. */
        olin = lin;
        tcol = 62;
        dcol = 72;
        cprintf(lin,tcol,ALIGN_NONE,"#%d#%s", LabelColor, "Maxkills:");
        cprintf(lin,dcol,ALIGN_NONE,"#%d#%0d",InfoColor,
                cbUsers[unum].stats[USTAT_MAXKILLS]);

        lin++;
        cprintf(lin,tcol,ALIGN_NONE,"#%d#%s", LabelColor, "Torpedos:");
        cprintf(lin,dcol,ALIGN_NONE,"#%d#%0d",InfoColor,
                cbUsers[unum].stats[USTAT_TORPS]);

        lin++;
        cprintf(lin,tcol,ALIGN_NONE,"#%d#%s", LabelColor, " Phasers:");
        cprintf(lin,dcol,ALIGN_NONE,"#%d#%0d",InfoColor,
                cbUsers[unum].stats[USTAT_PHASERS]);

        /* Do column 3 of the bottom stuff. */
        lin = olin;
        tcol = 35;
        dcol = 51;
        cprintf(lin,tcol,ALIGN_NONE,"#%d#%s", LabelColor, " Planets taken:");
        cprintf(lin,dcol,ALIGN_NONE,"#%d#%0d",InfoColor,
                cbUsers[unum].stats[USTAT_CONQPLANETS]);

        lin++;
        cprintf(lin,tcol,ALIGN_NONE,"#%d#%s", LabelColor, " Armies bombed:");
        cprintf(lin,dcol,ALIGN_NONE,"#%d#%0d",InfoColor,
                cbUsers[unum].stats[USTAT_ARMBOMB]);

        lin++;
        cprintf(lin,tcol,ALIGN_NONE,"#%d#%s", LabelColor, "   Ship armies:");
        cprintf(lin,dcol,ALIGN_NONE,"#%d#%0d",InfoColor,
                cbUsers[unum].stats[USTAT_ARMSHIP]);

        /* Do column 2 of the bottom stuff. */
        lin = olin;
        tcol = 18;
        dcol = 29;
        cprintf(lin,tcol,ALIGN_NONE,"#%d#%s", LabelColor, " Conquers:");
        cprintf(lin,dcol,ALIGN_NONE,"#%d#%0d",InfoColor,
                cbUsers[unum].stats[USTAT_CONQUERS]);

        lin++;
        cprintf(lin,tcol,ALIGN_NONE,"#%d#%s", LabelColor,"    Coups:");
        cprintf(lin,dcol,ALIGN_NONE,"#%d#%0d",InfoColor,
                cbUsers[unum].stats[USTAT_COUPS]);

        lin++;
        cprintf(lin,tcol,ALIGN_NONE,"#%d#%s", LabelColor, "Genocides:");
        cprintf(lin,dcol,ALIGN_NONE,"#%d#%0d",InfoColor,
                cbUsers[unum].stats[USTAT_GENOCIDE]);

        /* Do column 1 of the bottom stuff. */
        lin = olin;
        tcol = 1;
        dcol = 10;
        cprintf(lin,tcol,ALIGN_NONE,"#%d#%s", LabelColor, "   Wins:");
        cprintf(lin,dcol,ALIGN_NONE,"#%d#%0d",InfoColor,
                cbUsers[unum].stats[USTAT_WINS]);

        lin++;
        cprintf(lin,tcol,ALIGN_NONE,"#%d#%s", LabelColor, " Losses:");
        cprintf(lin,dcol,ALIGN_NONE,"#%d#%0d",InfoColor,
                cbUsers[unum].stats[USTAT_LOSSES]);

        lin++;
        cprintf(lin,tcol,ALIGN_NONE,"#%d#%s", LabelColor, "Entries:");
        cprintf(lin,dcol,ALIGN_NONE,"#%d#%0d",InfoColor,
                cbUsers[unum].stats[USTAT_ENTRIES]);

        cprintf(MSG_LIN1,0,ALIGN_CENTER,"#%d#%s", InfoColor,
                rprompt);
        cprintf(MSG_LIN2,0,ALIGN_CENTER,"#%d#%s", InfoColor,
                prompt2);

        if ( left )
            i = lcol;
        else
            i = rcol;

        cdput( '+', row, i );
        cdmove( row, i );
        cdrefresh();

        /* Now, get a char and process it. */
        if ( ! iogtimed( &ch, 1.1 ) )
            continue;		/* next */
        switch ( ch )
	{
	case 'a':
	case 'A':
	case 'h':
	case 'H':
	case KEY_LEFT:
            /* Left. */
            left = true;
            break;
	case 'd':
	case 'D':
	case 'l':
	case 'L':
	case KEY_RIGHT:
            /* Right. */
            left = false;
            break;
	case 'w':
	case 'W':
	case 'k':
	case 'K':
	case KEY_UP:
            /* Up. */
            row = std::max( row - 1, 1 );
            break;
	case 'x':
	case  'X':
	case 'j':
	case  'J':
	case KEY_DOWN:
            /* Down. */
            row = std::min( row + 1, MAXUEDITROWS );
            break;
	case 'y':
	case 'Y':
	case KEY_A1:
	case KEY_HOME:
            /* Up and left. */
            left = true;
            row = std::max( row - 1, 1 );
            break;
	case 'e':
	case 'E':
	case 'u':
	case 'U':
	case KEY_PPAGE:
	case KEY_A3:
            /* Up and right. */
            left = false;
            row = std::max( row - 1, 1 );
            break;
	case 'z':
	case 'Z':
	case 'b':
	case 'B':
	case KEY_END:
	case KEY_C1:
            /* Down and left. */
            left = true;
            row = std::min( row + 1, MAXUEDITROWS );
            break;
	case 'c':
	case 'C':
	case 'n':
	case 'N':
	case KEY_NPAGE:
	case KEY_C3:
            /* Down and right. */
            left = false;
            row = std::min( row + 1, MAXUEDITROWS );
            break;
	case ' ':
            /* Modify the current entry. */
            if ( left && row == 1 )
	    {
                /* Pseudonym. */
                cdclrl( MSG_LIN2, 1 );
                ch = mcuGetCX( "Enter a new pseudonym: ",
                               MSG_LIN2, 0, TERMS, buf, MAX_USERNAME );
                if ( ch != TERM_ABORT &&
                     ( buf[0] != 0 || ch == TERM_EXTRA ) )
                    utStrncpy( cbUsers[unum].alias, buf, MAX_USERNAME ); /* -[] */
	    }
            else if ( ! left && row == 1 )
	    {
                /* Username. */
                cdclrl( MSG_LIN2, 1 );
                ch = mcuGetCX( "Enter a new username: ",
                               MSG_LIN2, 0, TERMS, buf, MAX_USERNAME );
                if ( ch != TERM_ABORT && buf[0] != 0)
                {
                    utDeleteBlanks( buf );
                    if ( ! clbGetUserNum( &i, buf, USERTYPE_ANY ) )
                        utStrncpy( cbUsers[unum].username, buf, MAX_USERNAME );
                    else
                    {
                        cdclrl( MSG_LIN1, 2 );
                        cdputc( "That username is already in use.",
                                MSG_LIN2 );
                        cdmove( 1, 1 );
                        cdrefresh();
                        utSleep( 1.0 );
                    }
                }
	    }
            else if ( left && row == 2 )
	    {
                /* Team. */
                cbUsers[unum].team = mod( cbUsers[unum].team + 1, NUMPLAYERTEAMS );
	    }
            else if ( ! left && row == 2 )
	    {
                /* was Multiple count.  Available for future use... */
	    }
            else
	    {
                // the options - detect which ones (only the "left"
                // side for now
                i = row - 3;
                if ( left )
                {
                    // 0 - play when closed
                    if (i == 0)
                    {
                        if (UPLAYWHENCLOSED(unum))
                            UOPCLR(unum, USER_OP_PLAYWHENCLOSED);
                        else if (!UPLAYWHENCLOSED(unum))
                            UOPSET(unum, USER_OP_PLAYWHENCLOSED);
                        break;
                    }

                    // 1 - banned
                    if (i == 1)
                    {
                        if (UBANNED(unum))
                            UOPCLR(unum, USER_OP_BANNED);
                        else if (!UBANNED(unum))
                            UOPSET(unum, USER_OP_BANNED);
                        break;
                    }

                    // 2 - operator
                    if (i == 2)
                    {
                        if (UISOPER(unum))
                            UOPCLR(unum, USER_OP_ISOPER);
                        else if (!UISOPER(unum))
                            UOPSET(unum, USER_OP_ISOPER);
                        break;
                    }

                    // 3 - autopilot
                    if (i == 3)
                    {
                        if (UAUTOPILOT(unum))
                            UOPCLR(unum, USER_OP_AUTOPILOT);
                        else if (!UAUTOPILOT(unum))
                            UOPSET(unum, USER_OP_AUTOPILOT);
                        break;
                    }

                    cdbeep();
                }
                else // not left
                    cdbeep();
	    }
            break;

	case TERM_EXTRA:	/* change passwd */
            ChangePassword(unum, true);
            break;

	case 0x0c:
            cdredo();
            break;

	case TERM_NORMAL:
	case TERM_ABORT:

	default:
            return;
	}
    }

    /* NOTREACHED */

}


/*  watch - peer over someone's shoulder */
/*  SYNOPSIS */
/*    watch */
void watch(void)
{

    int snum, tmp_snum, old_snum;
    int ch, normal, readone;
    uint64_t msgrand, now;
    char buf[MSGMAXLINE];
    int live_ships = true;
    int toggle_flg = false;   /* jon no like the toggle line ... :-) */

    normal = true;

    if (!prompt_ship(buf, &snum, &normal))
        return;
    else
    {
        old_snum = tmp_snum = snum;
        Context.redraw = true;
        cdclear();
        cdredo();
        utGrand( &msgrand );

        Context.snum = snum;		/* so display knows what to display */
        operSetTimer();

        while (true)	/* repeat */
        {
            if (!normal)
		doDisplay = false; /* can't use it to display debugging */
            else
		doDisplay = true;

            /* set up toggle line display */
            /* cdclrl( MSG_LIN1, 1 ); */
            if (toggle_flg)
                toggle_line(snum,old_snum);

	    /* Try to display a new message. */
	    readone = false;
	    if ( utDeltaGrand( msgrand, &now ) >= NEWMSG_GRAND )
		if ( utGetMsg( -1, &cbConqInfo->glastmsg ) )
                {
		    mcuReadMsg( cbConqInfo->glastmsg, MSG_MSG );
#if defined(OPER_MSG_BEEP)
		    if (cbMsgs[cbConqInfo->glastmsg].msgfrom != MSG_GOD)
                        cdbeep();
#endif
		    msgrand = now;
		    readone = true;
                }

            if ( !normal )
            {
                debugdisplay( snum );
            }

            /* Un-read message, if there's a chance it got garbaged. */
            if ( readone )
		if ( iochav() )
                    cbConqInfo->glastmsg = mod( cbConqInfo->glastmsg - 1, cbLimits.maxMsgs() );

            /* Get a char with timeout. */
            if ( ! iogtimed( &ch, 1.0 ) )
		continue; /* next */
            cdclrl( MSG_LIN1, 2 );
            switch ( ch )
            {
            case 'd':    /* flip the doomsday machine (only from doomsday window) */
                if (snum == DISPLAY_DOOMSDAY)
                {
                    if ( DOOM_LIVE() )
			DOOMCLR(DOOM_F_LIVE);
                    else
			clbDoomsday();
                }
                else
		    cdbeep();
                break;
            case 'h':
                operStopTimer();
                dowatchhelp();
                Context.redraw = true;
                operSetTimer();
                break;
            case 'i':
                opinfo();
                break;
            case 'k':
                kiss(Context.snum, true);
                break;
            case 'M':		/* strategic/tactical map */
                if (SMAP(Context.snum))
                    SFCLR(Context.snum, SHIP_F_MAP);
                else
                    SFSET(Context.snum, SHIP_F_MAP);

                break;
            case 'm':
                cucSendMsg( MSG_FROM_GOD, 0, true, false );
                break;
            case 'r':  /* just for fun - dwp */
                oprobot();
                break;
            case 'L':
                mcuReviewMsgs( cbConqInfo->glastmsg );
                break;
            case 0x0c:
                operStopTimer();
                cdredo();
                Context.redraw = true;
                operSetTimer();
                break;
            case 'q':
            case 'Q':
                operStopTimer();
                return;
                break;
            case 'w': /* look at any ship (live or not) if specifically asked for */
                tmp_snum = snum;
                if (prompt_ship(buf, &snum, &normal))
                {
                    if (tmp_snum != snum)
                    {
                        old_snum = tmp_snum;
                        Context.redraw = true;
                    }
                    Context.snum = snum;
                    if (normal)
                    {
                        operStopTimer();
                        display( Context.snum );
                        operSetTimer();
                    }
                }
                break;
            case 'W': /* toggle live_ships flag */
                if (live_ships)
		    live_ships = false;
                else
		    live_ships = true;
                break;
            case '`':                 /* toggle between two ships */
                if (normal || (!normal && old_snum >= 0))
                {
                    if (old_snum != snum)
                    {
                        tmp_snum = snum;
                        snum = old_snum;
                        old_snum = tmp_snum;

                        Context.snum = snum;
                        Context.redraw = true;
                        if (normal)
                        {
                            operStopTimer();
                            display( Context.snum );
                            operSetTimer();
                        }
                    }
                }
                else
		    cdbeep();
                break;
            case '~':                 /* toggle debug display */
                if (Context.snum >= 0)
                {
                    if (normal)
			normal = false;
                    else
			normal = true;
                    Context.redraw = true;
                    cdclear();
                }
                else
		    cdbeep();
                break;
            case '/':                /* ship list - dwp */
                operStopTimer();
                mcuPlayList( false );
                Context.redraw = true;
                operSetTimer();
                break;
            case '\\':               /* big ship list - dwp */
                operStopTimer();
                mcuPlayList( true );
                Context.redraw = true;
                operSetTimer();
                break;
            case '!':
                if (toggle_flg)
                    toggle_flg = false;
                else
                    toggle_flg = true;
                break;
            case '>':  /* forward rotate ship numbers (including doomsday) - dwp */
            case KEY_RIGHT:
            case KEY_UP:
                while (true)
                {
                    int i;

                    if (live_ships)
                    {	/* we need to make sure that there is
                           actually something alive or an
                           infinite loop will result... */
                        int foundone = false;

                        for (i=0; i<cbLimits.maxShips(); i++)
                        {
                            if (clbStillAlive(i))
                            {
                                foundone = true;
                            }
                        }
                        if (foundone == false)
                        {	/* check the doomsday machine */
                            if (DOOM_LIVE())
				foundone = true;
                        }

                        if (foundone == false)
                        {
                            cdbeep();
                            break; /* didn't find one, beep, leave everything
				      alone*/
                        }
                    }

                    if (snum == DISPLAY_DOOMSDAY)
                    {	  /* doomsday - wrap around to first ship */
                        i = 0;
                    }
                    else
			i = snum + 1;

                    if (i >= cbLimits.maxShips())
                    {	/* if we're going past
                           now loop thu specials (only doomsday for
                           now... ) */
                        if (normal)
			    i = DISPLAY_DOOMSDAY;
                        else
			    i = 0;
                    }

                    snum = i;

                    Context.redraw = true;

                    if (live_ships)
			if ((snum >= 0 && clbStillAlive(snum)) ||
			    (snum == DISPLAY_DOOMSDAY && DOOM_LIVE()))
                        {
			    Context.snum = snum;
			    break;
                        }
			else
                            continue;
                    else
                    {
                        Context.snum = snum;
                        break;
                    }
                }
                if (normal)
                {
                    operStopTimer();
                    display( Context.snum );
                    operSetTimer();
                }
                break;
            case '<':  /* reverse rotate ship numbers (including doomsday)  - dwp */
            case KEY_LEFT:
            case KEY_DOWN:
                while (true)
                {
                    int i;

                    if (live_ships)
                    {	/* we need to make sure that there is
                           actually something alive or an
                           infinite loop will result... */
                        int foundone = false;

                        for (i=0; i < cbLimits.maxShips(); i++)
                        {
                            if (clbStillAlive(i))
                            {
                                foundone = true;
                            }
                        }
                        if (foundone == false)
                        {	/* check the doomsday machine */
                            if (DOOM_LIVE())
				foundone = true;
                        }

                        if (foundone == false)
                        {
                            cdbeep();
                            break; /* didn't find one, beep, leave everything
				      alone*/
                        }
                    }


                    if (snum == DISPLAY_DOOMSDAY)
                    {	  /* doomsday - wrap around to last ship */
                        i = cbLimits.maxShips() - 1;
                    }
                    else
			i = snum - 1;

                    if (i < 0)
                    {	/* if we're going past
                           now loop thu specials (only doomsday for
                           now... )*/
                        if (normal)
			    i = DISPLAY_DOOMSDAY;
                        else
			    i = cbLimits.maxShips() - 1;
                    }

                    snum = i;

                    Context.redraw = true;

                    if (live_ships)
			if ((snum >= 0 && clbStillAlive(snum)) ||
			    (snum == DISPLAY_DOOMSDAY && DOOM_LIVE()))
                        {
			    Context.snum = snum;
			    break;
                        }
			else
                            continue;
                    else
                    {
                        Context.snum = snum;
                        break;
                    }
                }
                if (normal)
                {
                    operStopTimer();
                    display( Context.snum );
                    operSetTimer();
                }

                break;
            case TERM_ABORT:
                return;
                break;
            default:
                cdbeep();
                uiPutMsg( "Type h for help.", MSG_LIN2 );
                break;
            }
            /* Disable messages for awhile. */
            utGrand( &msgrand );
        }
    } /* end else */

    /* NOTREACHED */

}

int prompt_ship(char buf[], int *snum, int *normal)
{
    int tch;
    int tmpsnum = -1;
    static const char *pmt="Watch which ship (<cr> for doomsday)? ";
    static const char *nss="No such ship.";

    cdclrl( MSG_LIN1, 2 );
    buf[0] = 0;
    tch = cdgetx( pmt, MSG_LIN1, 1, TERMS, buf, MSGMAXLINE, true );
    cdclrl( MSG_LIN1, 1 );

    if ( tch == TERM_ABORT )
    {
        return(false); /* dwp */
    }

    /*
     * DWP - Removed old code to watch doomsday machine.
     * Watch it with conqlb.c display() now.
     */

    *normal = ( tch != TERM_EXTRA );		/* line feed means debugging */

    utDeleteBlanks( buf );

    if ( strlen( buf ) == 0 )
    {              /* watch doomsday machine */
        tmpsnum = DISPLAY_DOOMSDAY;
        *normal = true;		/* doomsday doesn't have a debugging view */
    }
    else
    {
        if ( !utIsDigits( buf ) )
	{
            cdputs( nss, MSG_LIN2, 1 );
            cdmove( 1, 1 );
            cdrefresh();
            utSleep( 1.0 );
            return(false); /* dwp */
	}
        utSafeCToI( &tmpsnum, buf, 0 );	/* ignore return status */
    }

    if ( (tmpsnum < 0 || tmpsnum >= cbLimits.maxShips()) && tmpsnum != DISPLAY_DOOMSDAY )
    {
        cdputs( nss, MSG_LIN2, 1 );
        cdmove( 1, 1 );
        cdrefresh();
        utSleep( 1.0 );
        return(false); /* dwp */
    }

    *snum = tmpsnum;
    return(true);

} /* end prompt_ship() */


/*  dowatchhelp - display a list of commands while watching a ship */
/*  SYNOPSIS */
/*    dowatchhelp( void ) */
void dowatchhelp(void)
{
    int lin, col, tlin;
    int ch;
    static bool firstTime = true;
    static char sfmt[MSGMAXLINE * 2];

    if (firstTime)
    {
        firstTime = false;
        sprintf(sfmt,
                "#%d#%%-9s#%d#%%s",
                InfoColor,
                LabelColor);
    }

    cdclear();
    cprintf(1,0,ALIGN_CENTER,"#%d#%s", LabelColor, "WATCH WINDOW COMMANDS");

    lin = 4;

    /* Display the left side. */
    tlin = lin;
    col = 4;
    tlin++;
    cprintf(tlin,col,ALIGN_NONE,sfmt,
            "d", "flip the doomsday machine - doomsday window only");
    tlin++;
    cprintf(tlin,col,ALIGN_NONE,sfmt, "m", "message from GOD");
    tlin++;
    cprintf(tlin,col,ALIGN_NONE,sfmt, "L", "review messages");
    tlin++;
    cprintf(tlin,col,ALIGN_NONE,sfmt, "k", "kill a ship");
    tlin++;
    cprintf(tlin,col,ALIGN_NONE,sfmt, "h", "this message");
    tlin++;
    cprintf(tlin,col,ALIGN_NONE,sfmt, "w", "watch a ship");
    tlin++;
    cprintf(tlin,col,ALIGN_NONE,sfmt,
            "W", "toggle between (live status) and (any status) ships");
    tlin++;
    cprintf(tlin,col,ALIGN_NONE,sfmt,
            "<>", "decrement/increment ship number\n");
    tlin++;
    cprintf(tlin,col,ALIGN_NONE,sfmt, "`", "toggle between two ships");
    tlin++;
    cprintf(tlin,col,ALIGN_NONE,sfmt,
            "~", "toggle between window and debug ship screen");
    tlin++;
    cprintf(tlin,col,ALIGN_NONE,sfmt, "!", "display toggle line");
    tlin++;
    cprintf(tlin,col,ALIGN_NONE,sfmt, "/", "player list");

    mcuPutPrompt( MTXT_DONE, MSG_LIN2 );
    cdrefresh();
    while ( ! iogtimed( &ch, 1.0 ) )
        ;
    cdclrl( MSG_LIN2, 1 );

    return;

}


void toggle_line(int snum, int old_snum)
{

    static const char *frmt_str = "(') = Toggle %s:%s";
    char snum_str[MSGMAXLINE];
    char old_snum_str[MSGMAXLINE];
    char buf[MSGMAXLINE];

    build_toggle_str(snum_str,snum);
    build_toggle_str(old_snum_str,old_snum);

    sprintf(buf,frmt_str,snum_str, old_snum_str);
    cdputs( buf, MSG_LIN1, (Context.maxcol-(strlen(buf))));  /* at end of line */

    return;

}

char *build_toggle_str(char *snum_str, int snum)
{

    static const char *doomsday_str = "DM";
    static const char *unknown_str = "n/a";

    if (snum >= 0 && snum < cbLimits.maxShips())
    {          /* ship */
        sprintf(snum_str,"%c%d", cbTeams[cbShips[snum].team].teamchar, snum);
    }
    else if (snum == DISPLAY_DOOMSDAY)          /* specials */
        strcpy(snum_str,doomsday_str);
    else                                        /* should not get here */
        strcpy(snum_str,unknown_str);

    return(snum_str);

}

/* DoInit(InitChar, cmdline) - Based on InitChar, init something (like in
 *   the (I)nitialize screen).  if cmdline is true, output status on stdout.
 */

int DoInit(char InitChar, int cmdline)
{

    if (cmdline == true)
    {
        fprintf(stdout, "Initialized ");
        fflush(stdout);
    }

    /*  perform an init on something */
    switch(InitChar)
    {
    case 'e':
        clbInitEverything(false);
        *cbRevision = COMMONSTAMP;

        if (cmdline == true)
	{
            fprintf(stdout, "everything.\n");
            fflush(stdout);
	}

        break;

    case 'z':
        clbZeroEverything();

        if (cmdline == true)
	{
            fprintf(stdout, "common block (zeroed).\n");
            fflush(stdout);
	}

        break;

    case 'u':
        clbInitUniverse(false);
        *cbRevision = COMMONSTAMP;

        if (cmdline == true)
	{
            fprintf(stdout, "universe.\n");
            fflush(stdout);
	}

        break;

    case 'g':
        clbInitGame(false);
        *cbRevision = COMMONSTAMP;

        if (cmdline == true)
	{
            fprintf(stdout, "game.\n");
            fflush(stdout);
	}

        break;

    case 'p':
        cqiInitPlanets();
        *cbRevision = COMMONSTAMP;

        if (cmdline == true)
	{
            fprintf(stdout, "planets.\n");
            fflush(stdout);
	}

        break;

    case 's':
        clbClearShips();
        *cbRevision = COMMONSTAMP;

        if (cmdline == true)
	{
            fprintf(stdout, "ships.\n");
            fflush(stdout);
	}

        break;

    case 'm':
        clbInitMsgs();
        *cbRevision = COMMONSTAMP;

        if (cmdline == true)
	{
            fprintf(stdout, "messages.\n");
            fflush(stdout);
	}

        break;

    case 'l':
        cbUnlock(&cbConqInfo->lockword);
        cbUnlock(&cbConqInfo->lockmesg);
        *cbRevision = COMMONSTAMP;

        if (cmdline == true)
	{
            fprintf(stdout, "lockwords.\n");
            fflush(stdout);
	}

        break;

    case 'r':
        clbInitRobots();
        *cbRevision = COMMONSTAMP;

        if (cmdline == true)
	{
            fprintf(stdout, "robots.\n");
            fflush(stdout);
	}

        break;

    default:
        if (cmdline == true)
	{
            fprintf(stdout, "nothing. '%c' unrecognized.\n", InitChar);
            fflush(stdout);
	}
        break;
    }

    utLog("OPER: %s initialized '%c'",
          operUsername.c_str(), InitChar);

    return true;
}

void EnableConqoperSignalHandler(void)
{
#ifdef DEBUG_SIG
    utLog("EnableConquestSignalHandler() ENABLED");
#endif

    signal(SIGHUP, (void (*)(int))DoConqoperSig);
    signal(SIGTSTP, SIG_IGN);
    signal(SIGTERM, (void (*)(int))DoConqoperSig);
    signal(SIGINT, (void (*)(int))DoConqoperSig);
    signal(SIGQUIT, (void (*)(int))DoConqoperSig);

    return;
}

void DoConqoperSig(int sig)
{

#ifdef DEBUG_SIG
    utLog("DoSig() got SIG %d", sig);
#endif

    switch(sig)
    {
    case SIGTERM:
    case SIGINT:
    case SIGHUP:
    case SIGQUIT:
        operStopTimer();
        cdrefresh();
        cdend();
        exit(0);			/* WE EXIT HERE */
        break;
    default:
        break;
    }

    EnableConqoperSignalHandler();	/* reset */
    return;
}


/*  astoperservice - ast service routine for conqoper */
/*  SYNOPSIS */
/*    astservice */
/* This routine gets called from a sys$setimr ast. Normally, it outputs */
/* one screen update and then sets up another timer request. */
void astoperservice(int sig)
{
    /* Don't do anything if we're not supposed to. */
    if ( ! doDisplay )
        return;

    operStopTimer();

    /* Perform one ship display update. */
    display( Context.snum );

    /* Schedule for next time. */
    operSetTimer();

    return;

}

/*  operSetTimer - set timer to display() for conqoper...*/
/*  SYNOPSIS */
void operSetTimer(void)
{
    static struct sigaction Sig;

#ifdef HAVE_SETITIMER
    struct itimerval itimer;
#endif

    Sig.sa_handler = (void (*)(int))astoperservice;

    Sig.sa_flags = 0;

    if (sigaction(SIGALRM, &Sig, NULL) == -1)
    {
        utLog("clntSetTimer():sigaction(): %s\n", strerror(errno));
        exit(errno);
    }

#ifdef HAVE_SETITIMER
    if (Context.updsec < 1 || Context.updsec > MAX_UPDATE_PER_SEC)
        Context.updsec = 1;

    if (Context.updsec == 1)
    {
        itimer.it_value.tv_sec = 1;
        itimer.it_value.tv_usec = 0;
    }
    else
    {
        itimer.it_value.tv_sec = 0;
        itimer.it_value.tv_usec = (1000000 / Context.updsec);
    }

    itimer.it_interval.tv_sec = itimer.it_value.tv_sec;
    itimer.it_interval.tv_usec = itimer.it_value.tv_usec;

    setitimer(ITIMER_REAL, &itimer, NULL);
#else
    alarm(1);			/* set alarm() */
#endif
    return;

}



/*  operStopTimer - cancel timer */
/*  SYNOPSIS */
/*    operStopTimer */
void operStopTimer(void)
{
#ifdef HAVE_SETITIMER
    struct itimerval itimer;
#endif

    doDisplay = false;


    signal(SIGALRM, SIG_IGN);

#ifdef HAVE_SETITIMER
    itimer.it_value.tv_sec = itimer.it_interval.tv_sec = 0;
    itimer.it_value.tv_usec = itimer.it_interval.tv_usec = 0;

    setitimer(ITIMER_REAL, &itimer, NULL);
#else
    alarm(0);
#endif


    doDisplay = true;

    return;

}
