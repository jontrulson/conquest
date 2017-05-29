#include "c_defs.h"

/************************************************************************
 * Copyright Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 ***********************************************************************/

/*                               C O N Q U E S T */
/*            Copyright (C)1983-1986 by Jef Poskanzer and Craig Leres */
/*    Permission to use, copy, modify, and distribute this software and */
/*    its documentation for any purpose and without fee is hereby granted, */
/*    provided that this copyright notice appear in all copies and in all */
/*    supporting documentation. Jef Poskanzer and Craig Leres mak2003 */
/*    representations about the suitability of this software for any */
/*    purpose. It is provided "as is" without express or implied warranty. */

#define NOEXTERN_GLOBALS
#include "global.h"

#define NOEXTERN_CONF
#include "conf.h"

#include "conqdef.h"
#include "conqcom.h"
#include "conqlb.h"
#include "rndlb.h"
#include "conqutil.h"
#include "conqunix.h"

#define NOEXTERN_CONTEXT
#include "context.h"


#include "color.h"
#include "ui.h"
#include "options.h"
#include "cd2lb.h"
#include "iolb.h"
#include "cumisc.h"
#include "cuclient.h"
#include "record.h"
#include "ibuf.h"
#include "conqnet.h"
#include "packet.h"
#include "display.h"
#include "client.h"
#include "clientlb.h"
#include "clntauth.h"
#include "cuclient.h"
#include "udp.h"
#include "meta.h"
#include "conqinit.h"
#include "playback.h"
#include "conquest.h"

struct _srvvec {
    uint16_t vers;
    char hostname[MAXHOSTNAME + 10];
};

static struct _srvvec servervec[META_MAXSERVERS] = {};


static char cbuf[BUFFER_SIZE_1024]; /* general purpose buffer */

void catchSignals(void);
void handleSignal(int sig);

void astservice(int sig);
void stopTimer(void);
void startTimer(void);
void conqend(void);
int selectServer(metaSRec_t *metaServerList, int nums);

/* conqreplay.c */
extern void conquestReplay(void);

/* conquest.c */

int welcome(void);
void doalloc( int snum);
void doautopilot( int snum );
void dobeam( int snum );
void dobomb( int snum );
void doburst( int snum );
void docloak( int snum );
void dorefit( int snum, int dodisplay );
void docoup( int snum );
void docourse( int snum );
void dodet( int snum );
void dodistress( int snum );
void dohelp( void );
void doinfo( int snum );
void dolastphase( int snum );
void domydet( int snum );
void dooption( int snum, int dodisplay );
void doorbit( int snum );
void dophase( int snum );
void doPlanetList( int snum );
void doReviewMsgs( int snum );
void doselfdest(int snum);
void doshields( int snum, int up );
void doTeamList( int team );
void dotorp( int snum );
void dotow( int snum );
void dountow( int snum );
void dowar( int snum );
void dowarp( int snum, real warp );
int getoption( char ch, int *tok );
void menu(void);
int play(void);
int capentry( int snum, int *system );
int newship( int unum, int *snum );
void dead( int snum, int leave );

void printUsage()
{
    printf("Usage: conquest [-m ][-s server[:port]] [-r recfile] [ -t ]\n");
    printf("                [ -M <metaserver> ] [ -u ] [ -B ]\n\n");
    printf("    -m               query the metaserver\n");
    printf("    -s server[:port] connect to <server> at <port>\n");
    printf("                      default: localhost:1701\n");
    printf("    -r recfile       Record game to <recfile>\n");
    printf("                      recfile will be in compressed format\n");
    printf("                      if conquest was compiled with libz\n");
    printf("                      support\n");
    printf("    -M metaserver   specify alternate <metaserver> to contact.\n");
    printf("                     default: %s\n", META_DFLT_SERVER);
    printf("    -P <cqr file>   Play back a Conquest recording (.cqr)\n");
    printf("    -B              Benchmark mode.  When playing back a recording,\n");
    printf("                    the default playback speed will be as fast as possible.\n");
    printf("    -u              do not attempt to use UDP from server.\n");
    printf("    -v              be more verbose.\n");
    return;
}

int getLocalhost(char *buf, int len)
{
    struct hostent *hp;

    gethostname ( buf, len );
    if ((hp = gethostbyname(buf)) == NULL)
    {
        if (buf)
            fprintf(stderr, "conquest: gethostbyname('%s'): cannot get localhost info.\n", buf);
        return FALSE;
    }

    return TRUE;
}

int connectServer(char *remotehost, uint16_t remoteport)
{
    int s;
    struct sockaddr_in sa;
    struct hostent *hp;
    int lin;

    if (!remotehost)
        return FALSE;

    /* display the logo */
    lin = mcuConqLogo();

    lin += 5;

    if ((hp = gethostbyname(remotehost)) == NULL)
    {
        utLog("conquest: %s: no such host\n", remotehost);

        cprintf(lin, 0, ALIGN_CENTER, "conquest: %s: no such host",
                remotehost);
        mcuPutPrompt(MTXT_DONE, MSG_LIN2 );
        iogchar();

        return FALSE;
    }

    /* put host's address and address type into socket structure */
    memcpy((char *)&sa.sin_addr, (char *)hp->h_addr, hp->h_length);
    sa.sin_family = hp->h_addrtype;

    sa.sin_port = htons(remoteport);

    if ((s = socket(AF_INET, SOCK_STREAM, 0 )) < 0)
    {
        utLog("socket: %s", strerror(errno));
        cprintf(lin, 0, ALIGN_CENTER, "socket: %s",
                remotehost);
        mcuPutPrompt(MTXT_DONE, MSG_LIN2 );
        iogchar();
        return FALSE;
    }

    if (cInfo.tryUDP)
    {
        utLog("NET: Opening UDP...");
        if ((cInfo.usock = udpOpen(0, &cInfo.servaddr)) < 0)
        {
            utLog("NET: udpOpen: %s", strerror(errno));
            cInfo.tryUDP = FALSE;
        }
    }

    utLog("Connecting to host: %s, port %d\n",
          remotehost, remoteport);

    cprintf(lin++, 0, ALIGN_CENTER, "Connecting to host: %s, port %d\n",
            remotehost, remoteport);
    cdrefresh();

    /* connect to the remote server */
    if (connect(s, (const struct sockaddr *)&sa, sizeof(sa)) < 0)
    {
        utLog("connect: %s", strerror(errno));
        utLog("Cannot connect to host %s:%d\n",
              remotehost, remoteport);

        cprintf(lin++, 0, ALIGN_CENTER, "connect: %s", strerror(errno));
        cprintf(lin++, 0, ALIGN_CENTER, "Cannot connect to host %s:%d\n",
                remotehost, remoteport);
        cprintf(lin++, 0, ALIGN_CENTER,
                "Is there a conquestd server running there?\n");
        mcuPutPrompt(MTXT_DONE, MSG_LIN2 );
        iogchar();

        return FALSE;
    }

    cprintf(lin++, 0, ALIGN_CENTER, "Connected!");
    cdrefresh();

    cInfo.serverDead = FALSE;
    cInfo.sock = s;
    cInfo.servaddr = sa;

    pktSetSocketFds(cInfo.sock, PKT_SOCKFD_NOCHANGE);
    /* we won't setup the udp fd until we've negotiated successfully */
    pktSetNodelay();

    return TRUE;
}

/*  conquest - main program */
int main(int argc, char *argv[])
{
    int i;
    char *ch;
    int wantMetaList = FALSE;     /* wants to see a list from metaserver */
    int serveropt = FALSE;        /* specified a server with '-s' */
    int nums = 0;                     /* num servers from metaGetServerList() */
    char *metaServer = META_DFLT_SERVER;
    metaSRec_t *metaServerList;   /* list of servers */

    /* tell the packet routines that we are a client */
    pktSetClientMode(TRUE);
    pktSetClientProtocolVersion(PROTOCOL_VERSION);

    Context.entship = FALSE;
    Context.recmode = RECMODE_OFF;
    Context.updsec = 5;		/* dflt - 5/sec */
    Context.msgrand = time(0);

    cInfo.sock = -1;
    cInfo.usock = -1;

    utSetLogConfig(FALSE, FALSE);	/* use $HOME for logfile */

    cInfo.doUDP = FALSE;
    cInfo.tryUDP = TRUE;

    cInfo.state = CLT_STATE_PREINIT;
    cInfo.serverDead = TRUE;
    cInfo.isLoggedIn = FALSE;
    cInfo.remoteport = CN_DFLT_PORT;

    cInfo.remotehost = strdup("localhost"); /* default to your own server */

    /* check options */
    while ((i = getopt(argc, argv, "mM:s:r:P:Buv")) != EOF)    /* get command args */
        switch (i)
        {
        case 'B':                 /* Benchmark mode, set recFrameDelay to 0.0 */
            pbSpeed = PB_SPEED_INFINITE;
            recFrameDelay = 0.0;
            break;
        case 'm':
            wantMetaList = TRUE;
            break;
        case 'M':
            metaServer = optarg;
            break;
        case 's':                 /* [host[:port]] */
            free(cInfo.remotehost);
            cInfo.remotehost = strdup(optarg);
            if (!cInfo.remotehost)
            {
                printf("strdup failed\n");
                exit(1);
            }
            if ((ch = strchr(cInfo.remotehost, ':')) != NULL)
            {
                *ch = 0;
                ch++;
                if ((cInfo.remoteport = atoi(ch)) == 0)
                    cInfo.remoteport = CN_DFLT_PORT;

                /* if no host was specified (only the :port), then set to
                   localhost */
                if (strlen(cInfo.remotehost) == 0)
                {
                    free(cInfo.remotehost);
                    cInfo.remotehost = strdup("localhost");
                }
            }
            else
                cInfo.remoteport = CN_DFLT_PORT;

            serveropt = TRUE;

            break;
        case 'r':
            /* don't want to do this if we've already seen -P */
            if (Context.recmode != RECMODE_PLAYING)
            {
                if (recOpenOutput(optarg, FALSE))
                {			/* we are almost ready... */
                    Context.recmode = RECMODE_STARTING;
                    printf("Recording game to %s...\n", optarg);
                }
                else
                {
                    Context.recmode = RECMODE_OFF;
                    printf("Cannot record game to %s... terminating\n", optarg);
                    exit(1);
                }
            }
            break;

        case 'P':
            recFilename = optarg;
            Context.recmode = RECMODE_PLAYING;
            break;

        case 'u':
            cInfo.tryUDP = FALSE;
            break;

        case 'v':
            cqDebug++;
            break;

        default:
            printUsage();
            exit(1);
        }

    rndini( 0, 0 );		/* initialize random numbers */

    if (!pktInit())
    {
        fprintf(stderr, "pktInit failed, exiting\n");
        utLog("pktInit failed, exiting");
    }
    pktSetSocketFds(cInfo.sock, cInfo.usock);

    cqiLoadRC(CQI_FILE_CONQINITRC, NULL, 1, 0);


#ifdef DEBUG_CONFIG
    utLog("%s@%d: main() Reading Configuration files.", __FILE__, __LINE__);
#endif

    if (GetConf(0) == -1)
    {
#ifdef DEBUG_CONFIG
        utLog("%s@%d: main(): GetConf() returned -1.", __FILE__, __LINE__);
#endif
	exit(1);
    }

    Context.updsec = UserConf.UpdatesPerSecond;

    if (Context.recmode == RECMODE_PLAYING)
    {
        if (serveropt || wantMetaList)
            printf("-P option specified.  All other options ignored.\n");

        serveropt = wantMetaList = FALSE;
        printf("Scanning file %s...\n", recFilename);
        if (!pbInitReplay(recFilename, &recTotalElapsed))
            exit(1);

        /* now init for real */
        if (!pbInitReplay(recFilename, NULL))
            exit(1);

        Context.unum = MSG_GOD;       /* stow user number */
        Context.snum = -1;           /* don't display in cdgetp - JET */
        Context.entship = FALSE;      /* never entered a ship */
        Context.histslot = -1;       /* useless as an op */
        Context.lasttdist = Context.lasttang = 0;
        Context.lasttarg[0] = 0;

        /* turn off annoying beeps */
        UserConf.DoAlarms = FALSE;
        /* turn off hudInfo */
        UserConf.hudInfo = FALSE;
    }

    if (serveropt && wantMetaList)
    {
        printf("-m ignored, since -s was specified\n");
        wantMetaList = FALSE;
    }
    else if (wantMetaList)
    {                           /* get the metalist and display */
        printf("Querying metaserver at %s\n",
               metaServer);
        nums = metaGetServerList(metaServer, &metaServerList);

        if (nums < 0)
        {
            printf("metaGetServerList() failed\n");
            return 1;
        }

        if (nums == 0)
        {
            printf("metaGetServerList() reported 0 servers online\n");
            return 1;
        }

        printf("Found %d server(s)\n",
               nums);
    }


    /* a parallel universe, it is */
    map_lcommon();

#ifdef DEBUG_FLOW
    utLog("%s@%d: main() starting conqinit().", __FILE__, __LINE__);
#endif

    conqinit();			/* machine dependent initialization */
    ibufInit();

#ifdef DEBUG_FLOW
    utLog("%s@%d: main() starting cdinit().", __FILE__, __LINE__);
#endif

    cdinit();			/* set up display environment */

    Context.maxlin = cdlins();
    Context.maxcol = cdcols();
    Context.snum = 0;		/* force menu to get a new ship */
    Context.histslot = -1;
    Context.lasttang = Context.lasttdist = 0;
    Context.lasttarg[0] = 0;

    /* If we are playing back a recording (-P)... */
    if (Context.recmode == RECMODE_PLAYING)
    {                           /* here, we will just do the replay stuff
                                   and exit */
        conquestReplay();
        cdend();
        return 0;
    }

    if (wantMetaList)
    {                           /* list the servers */
        i = selectServer(metaServerList, nums);

        if (i < 0)
        {                       /* didn't pick one */
            cdend();
            return 1;
        }

        if (cInfo.remotehost)
            free(cInfo.remotehost);

        if ((cInfo.remotehost = strdup(metaServerList[i].altaddr)) == NULL)
        {
            utLog("strdup(metaServerList[i]) failed");
            cdend();
            return 1;
        }

        cInfo.remoteport = metaServerList[i].port;

    }


    /* connect to the host */
    if (!connectServer(cInfo.remotehost, cInfo.remoteport))
    {
        cdend();
        return(1);
    }

    /* now we need to negotiate. */
    if (!clientHello(CONQUEST_NAME))
    {
        cdend();
        utLog("conquest: clientHello() failed\n");
        printf("conquest: clientHello() failed, check log\n");
        exit(1);
    }

#ifdef DEBUG_FLOW
    utLog("%s@%d: main() welcoming player.", __FILE__, __LINE__);
#endif

    if ( welcome() )
    {
        menu();
    }

    cdend();			/* clean up display environment */
    conqend();			/* machine dependent clean-up */

#ifdef DEBUG_FLOW
    utLog("%s@%d: main() *EXITING*", __FILE__, __LINE__);
#endif

    exit(0);

}


/*  capentry - captured system entry for a new ship */
/*  SYNOPSIS */
/*    int flag, capentry */
/*    int capentry, snum, system */
/*    system = capentry( snum, system ) */
int selectentry( uint8_t esystem )
{
    int i;
    int ch;
    int owned[NUMPLAYERTEAMS];

    /* First figure out which systems we can enter from. */
    for ( i = 0; i < NUMPLAYERTEAMS; i++ )
        if (esystem & (1 << i))
        {
            owned[i] = TRUE;
        }
        else
            owned[i] = FALSE;

    /* Prompt for a decision. */
    strcpy(cbuf , "Enter which system") ;
    for ( i = 0; i < NUMPLAYERTEAMS; i++ )
        if ( owned[i] )
        {
            strcat(cbuf,  ", ");
            strcat(cbuf , Teams[i].name) ;
        }

    /* Change first comma to a colon. */
    char *sptr = strchr(cbuf, ',');
    if (sptr)
        *sptr = ':';

    cdclrl( MSG_LIN1, 2 );
    cdputc( cbuf, MSG_LIN1 );
    cdmove( 1, 1 );
    cdrefresh();

    while ( clbStillAlive( Context.snum ) )
    {
        if ( ! iogtimed( &ch, 1.0 ) )
            continue;
        switch  ( ch )
	{
	case TERM_NORMAL:
	case TERM_ABORT:	/* doesn't like the choices ;-) */
            sendCommand(CPCMD_ENTER, 0);
            return ( FALSE );
            break;
	case TERM_EXTRA:
            /* Enter the home system. */
            sendCommand(CPCMD_ENTER, (uint16_t)(1 << Ships[Context.snum].team));
            return ( TRUE );
            break;
	default:
            for ( i = 0; i < NUMPLAYERTEAMS; i++ )
                if ( Teams[i].teamchar == (char)toupper( ch ) && owned[i] )
                {
                    /* Found a good one. */
                    sendCommand(CPCMD_ENTER, (uint16_t)(1 << i));
                    return ( TRUE );
                }
            /* Didn't get a good one; complain and try again. */
            cdbeep();
            cdrefresh();
            break;
	}
    }

    return ( FALSE );	    /* can get here because of clbStillAlive() */

}


/*  command - execute a user's command */
/*  SYNOPSIS */
/*    char ch */
/*    command( ch ) */
void command( int ch )
{
    int i;
    real x;
    if (mcuKPAngle(ch, &x) == TRUE)	/* hit a keypad key */
    {				/* change course */
        cdclrl( MSG_LIN1, 1 );
        cdclrl( MSG_LIN2, 1 );

        sendSetCourse(cInfo.sock, 0, x);
        return;
    }

    switch ( ch )
    {
    case '0':           /* - '9', '=' :set warp factor */
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
    case '=':
        if ( ch == '=' )
            x = 10.0;
        else
	{
            i = ch - '0';
            x = (real) (i);
	}
        dowarp( Context.snum, x );
        break;
    case 'a':				/* autopilot */
        if ( Users[Ships[Context.snum].unum].ooptions[ OOPT_AUTOPILOT] )
	{
            doautopilot( Context.snum );
	}
        else
	{
            goto label1;
	}
        break;
    case 'A':				/* change allocation */
        doalloc( Context.snum );
        break;
    case 'b':				/* beam armies */
        dobeam( Context.snum );
        break;
    case 'B':				/* bombard a planet */
        dobomb( Context.snum );
        break;
    case 'C':				/* cloak control */
        docloak( Context.snum );
        break;
    case 'd':				/* detonate enemy torps */
    case '*':
        dodet( Context.snum );
        break;
    case 'D':				/* detonate own torps */
        domydet( Context.snum );
        break;
    case 'E':				/* emergency distress call */
        dodistress( Context.snum );
        break;
    case 'f':				/* phasers */
        dophase( Context.snum );
        break;
    case 'F':				/* phasers, same direction */
        dolastphase( Context.snum );
        break;
    case 'h':
        Context.redraw = TRUE;
        Context.display = FALSE;
        dohelp();
        Context.display = TRUE;
        if ( clbStillAlive( Context.snum ) )
        {
            stopTimer();
            display( Context.snum );
            startTimer();
        }
        break;
    case 'H':
        Context.redraw = TRUE;
        Context.display = FALSE;
        mcuHistList( FALSE );
        Context.display = TRUE;
        if ( clbStillAlive( Context.snum ) )
        {
            stopTimer();
            display( Context.snum );
            startTimer();
        }
        break;
    case 'i':				/* information */
        doinfo( Context.snum );
        break;
    case 'k':				/* set course */
        docourse( Context.snum );
        break;
    case 'K':				/* coup */
        docoup( Context.snum );
        break;
    case 'L':				/* review old messages */
        doReviewMsgs( Context.snum );
        break;
    case 'm':				/* send a message */
        cucSendMsg( Context.snum, UserConf.Terse,
                    TRUE);
        break;
    case 'M':				/* strategic/tactical map */
        if (SMAP(Context.snum))
            SFCLR(Context.snum, SHIP_F_MAP);
        else
            SFSET(Context.snum, SHIP_F_MAP);
        stopTimer();
        display( Context.snum );
        startTimer();
        break;
    case 'N':				/* change pseudonym */
        cucPseudo( Context.unum, Context.snum );
        break;

    case 'O':
        Context.redraw = TRUE;
        Context.display = FALSE;
        UserOptsMenu(Context.unum);
        Context.display = TRUE;
        if ( clbStillAlive( Context.snum ) )
        {
            stopTimer();
            display( Context.snum );
            startTimer();
        }
        break;
    case 'o':				/* orbit nearby planet */
        doorbit( Context.snum );
        break;
    case 'P':				/* photon torpedo burst */
        doburst( Context.snum );
        break;
    case 'p':				/* photon torpedoes */
        dotorp( Context.snum );
        break;
    case 'Q':				/* self destruct */
        doselfdest( Context.snum );
        break;
    case 'r':				/* refit */
        if (sStat.flags & SPSSTAT_FLAGS_REFIT)
            dorefit( Context.snum, TRUE );
        else
            cdbeep();
        break;
    case 'R':				/* repair mode */
        if ( ! SCLOAKED(Context.snum) )
	{
            cdclrl( MSG_LIN1, 2 );
            sendCommand(CPCMD_REPAIR, 0);
	}
        else
	{
            cdclrl( MSG_LIN2, 1 );
            mcuPutMsg(
                "You cannot repair while the cloaking device is engaged.",
                MSG_LIN1 );
	}
        break;
    case 't':				/* tow */
        dotow( Context.snum );
        break;
    case 'S':				/* more user stats */
        Context.redraw = TRUE;
        Context.display = FALSE;
        mcuUserStats( FALSE, Context.snum );
        Context.display = TRUE;
        if ( clbStillAlive( Context.snum ) )
        {
            stopTimer();
            display( Context.snum );
            startTimer();
        }
        break;
    case 'T':				/* team list */
        Context.redraw = TRUE;
        Context.display = FALSE;
        doTeamList( Ships[Context.snum].team );
        Context.display = TRUE;
        if ( clbStillAlive( Context.snum ) )
        {
            stopTimer();
            display( Context.snum );
            startTimer();
        }
        break;
    case 'u':				/* un-tractor */
        sendCommand(CPCMD_UNTOW, 0);
        break;
    case 'U':				/* user stats */
        Context.redraw = TRUE;
        Context.display = FALSE;
        mcuUserList( FALSE, Context.snum );
        Context.display = TRUE;
        if ( clbStillAlive( Context.snum ) )
        {
            stopTimer();
            display( Context.snum );
            startTimer();
        }
        break;
    case 'W':				/* war and peace */
        cucDoWar( Context.snum );
        break;
    case '-':				/* shields down */
        doshields( Context.snum, FALSE );
        stopTimer();
        display( Context.snum );
        startTimer();
        break;
    case '+':				/* shields up */
        doshields( Context.snum, TRUE );
        stopTimer();
        display( Context.snum );
        startTimer();
        break;
    case '/':				/* player list */
        Context.redraw = TRUE;
        Context.display = FALSE;
        mcuPlayList( FALSE, FALSE, Context.snum );
        Context.display = TRUE;
        if ( clbStillAlive( Context.snum ) )
        {
            stopTimer();
            display( Context.snum );
            startTimer();
        }
        break;
    case '?':				/* planet list */
        Context.redraw = TRUE;
        Context.display = FALSE;
        doPlanetList( Context.snum );
        Context.display = TRUE;
        if ( clbStillAlive( Context.snum ) )
        {
            stopTimer();
            display( Context.snum );
            startTimer();
        }
	display( Context.snum );
        break;
    case TERM_REDRAW:			/* clear and redisplay */
        stopTimer();
        cdredo();
        Context.redraw = TRUE;
        display( Context.snum );
        startTimer();
        break;

    case TERM_NORMAL:		/* Have [ENTER] act like 'I[ENTER]'  */
    case KEY_ENTER:
    case '\n':
        ibufPut("i\r");		/* (get last info) */
        break;

    case ' ':
        UserConf.DoLocalLRScan = !UserConf.DoLocalLRScan;
        break;

    case TERM_EXTRA:		/* Have [TAB] act like 'i\t' */
        ibufPut("i\t");		/* (get next last info) */
        break;

    case TERM_RELOAD:		/* have server resend current universe */
        sendCommand(CPCMD_RELOAD, 0);
        utLog("client: sent CPCMD_RELOAD");
        break;

    case -1:			/* really nothing, move along */
        break;

        /* nothing. */
    default:
    label1:
        cdbeep();
#ifdef DEBUG_IO
        utLog("command(): got 0%o, KEY_A1 =0%o", ch, KEY_A1);
#endif
        mcuPutMsg( "Type h for help.", MSG_LIN2 );
    }

    return;

}


/*  conqds - display background for Conquest */
/*  SYNOPSIS */
/*    int multiple, switchteams */
/*    conqds( multiple, switchteams ) */
void conqds( int multiple, int switchteams )
{
    int i, col, lin;
    extern char *ConquestVersion;
    extern char *ConquestDate;
    static int FirstTime = TRUE;
    static char sfmt[MSGMAXLINE * 2];

    if (FirstTime == TRUE)
    {
        FirstTime = FALSE;
        sprintf(sfmt,
                "#%d#(#%d#%%c#%d#) - %%s",
                LabelColor,
                InfoColor,
                LabelColor);
    }

    /* First clear the display. */
    cdclear();

    /* Display the logo. */
    lin = mcuConqLogo();

    if ( ConqInfo->closed )
        cprintf(lin,0,ALIGN_CENTER,"#%d#%s",RedLevelColor,"The game is closed.");
    else
        cprintf( lin,0,ALIGN_CENTER,"#%d#%s (%s)",YellowLevelColor,
                 ConquestVersion, ConquestDate);

    lin++;
    lin++;
    cprintf(lin,0,ALIGN_CENTER,"#%d#%s", MagentaColor, CONQ_HTTP);
    lin++;
    lin++;
    lin++;
    cprintf(lin,0,ALIGN_CENTER,"#%d#%s",NoColor, "Options:");

    col = 13;
    lin++;
    i = lin;
    cprintf(lin,col,ALIGN_NONE,sfmt, 'e', "enter the game");
    if ( Context.hasnewsfile )
    {
        lin++;
        cprintf(lin,col,ALIGN_NONE,sfmt, 'n', "read the news");
    }
    lin++;
    cprintf(lin,col,ALIGN_NONE,sfmt, 'h', "read the help lesson");
    lin++;
    cprintf(lin,col,ALIGN_NONE,sfmt, 'S', "more user statistics");
    lin++;
    cprintf(lin,col,ALIGN_NONE,sfmt, 'T', "team statistics");
    lin++;
    cprintf(lin,col,ALIGN_NONE,sfmt, 'U', "user statistics");
    lin++;
    cprintf(lin,col,ALIGN_NONE,sfmt, 'L', "review messages");
    lin++;
    cprintf(lin,col,ALIGN_NONE,sfmt, 'W', "set war or peace");

    col = 48;
    lin = i;
    cprintf(lin,col,ALIGN_NONE,sfmt, 'N', "change your name");
    lin++;
    cprintf(lin,col,ALIGN_NONE,sfmt, 'O', "options menu");

    if ( ! multiple )
    {
        lin++;
        cprintf(lin,col,ALIGN_NONE,sfmt, 'r', "resign your commission");
    }
    if ( multiple || switchteams )
    {
        lin++;
        cprintf(lin,col,ALIGN_NONE,sfmt, 's', "switch teams");
    }
    lin++;
    cprintf(lin,col,ALIGN_NONE,sfmt, 'H', "user history");
    lin++;
    cprintf(lin,col,ALIGN_NONE,sfmt, '/', "player list");
    lin++;
    cprintf(lin,col,ALIGN_NONE,sfmt, '?', "planet list");
    lin++;
    cprintf(lin,col,ALIGN_NONE,sfmt, 'q', "exit the program");

    return;

}


/*  dead - announce to a user that s/he is dead (DOES LOCKING) */
/*  SYNOPSIS */
/*    int snum */
/*    int leave */
/*    dead( snum, leave ) */
void dead( int snum, int leave )
{
    int i, kb;
    int ch;
    char *ywkb="You were killed by ";
    char buf[128], junk[128];

    /* (Quickly) clear the screen. */
    cdclear();
    cdredo();
    cdrefresh();

    /* If something is wrong, don't do anything. */
    if ( snum < 1 || snum > MAXSHIPS )
    {
        utLog("dead: snum < 1 || snum > MAXSHIPS (%d)", snum);
        return;
    }

    kb = Ships[snum].killedby;

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
            if ( Ships[kb].status != SS_LIVE )
                strcat(buf, ", who also died.");
            else
                utAppendChar(buf , '.') ;
            cprintf( 8,0,ALIGN_CENTER,
                     "#%d#You were kill number #%d#%.1f #%d#for #%d#%s #%d#(#%d#%s#%d#)%s",
                     InfoColor, CQC_A_BOLD, Ships[kb].kills,
                     InfoColor, CQC_A_BOLD, Ships[kb].alias,
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
            if ( i == 1 )
                strcpy( cbuf, "army" );
            else
                strcpy( cbuf, "armies" );

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
                        "#%d#The #%d#%d %s #%d#you were carrying %s not amused.",
			LabelColor, CQC_A_BOLD, i, cbuf, LabelColor, buf);
	}
    }
    else if ( kb >= 0 )
    {
        if ( Ships[kb].status == SS_LIVE )
	{
            cprintf( 10,0,ALIGN_CENTER,
                     "#%d#He had #%d#%d%% #%d#shields and #%d#%d%% #%d#damage.",
                     InfoColor, CQC_A_BOLD, round(Ships[kb].shields),
                     InfoColor, CQC_A_BOLD, round(Ships[kb].damage),InfoColor );
	}
    }
    cprintf(12,0,ALIGN_CENTER,
            "#%d#You got #%d#%.1f #%d#this time.",
            InfoColor, CQC_A_BOLD, oneplace(Ships[snum].kills), InfoColor );
    cdmove( 1, 1 );
    cdrefresh();

    /* check the clientstat's flags to see if last words are needed */

    if (clientFlags & SPCLNTSTAT_FLAG_CONQUER)
    {
        utSleep(4.0);
        do
	{
            cdclear();
            cdredo();
            buf[0] = 0;

            ch = cdgetx( "Any last words? ",
                         14, 1, TERMS, buf, MAXLASTWORDS, TRUE );

            cdclear();
            cdredo();

            if ( buf[0] != 0 )
	    {
                cprintf( 13,0,ALIGN_CENTER, "#%d#%s",
                         InfoColor, "You last words are entered as:");
                cprintf( 14,0,ALIGN_CENTER, "#%d#%c%s%c",
                         YellowLevelColor, '"', buf, '"' );
	    }
            else
                cprintf( 14,0,ALIGN_CENTER,"#%d#%s", InfoColor,
                         "You have chosen to NOT leave any last words:" );
            ch = mcuGetCX( "Press [TAB] to confirm:", 16, 0,
                           TERMS, cbuf, 10 );
	}
        while ( ch != TERM_EXTRA ); /* until . while */

        /* now we just send a message */
        sendMessage(MSG_GOD, buf);
    }

    /* set the ship reserved (locally).  The server has already done this
       but we have not processed any packets since we died.  This keeps
       menu() from booting us out of the game. */
    Ships[Context.snum].status = SS_RESERVED;

    ioeat();
    mcuPutPrompt( MTXT_DONE, MSG_LIN2 );
    cdrefresh();
    while ( ! iogtimed( &ch, 1.0 ) )
        ;

    cdmove( 1, 1 );

    return;

}

/*  doalloc - change weapon/engine allocations */
/*  SYNOPSIS */
/*    int snum */
/*    doalloc( snum ) */
void doalloc( int snum )
{
    char ch;
    int i, alloc;
    int dwalloc = 0;
    char *pmt="New weapons allocation: (30-70) ";

    cdclrl( MSG_LIN1, 2 );
    cbuf[0] = 0;
    ch = (char)cdgetx( pmt, MSG_LIN1, 1, TERMS, cbuf, MSGMAXLINE, TRUE );

    switch (ch)
    {
    case TERM_EXTRA:
        dwalloc = Ships[snum].engalloc;
        break;

    case TERM_NORMAL:
        i = 0;
        utSafeCToI( &alloc, cbuf, i );			/* ignore status */
        if ( alloc != 0 )
	{
            if ( alloc < 30 )
                alloc = 30;
            else if ( alloc > 70 )
                alloc = 70;
            dwalloc = alloc;
	}
        else
	{
            cdclrl( MSG_LIN1, 1 );
            return;
	}

        break;

    default:
        return;
    }

    sendCommand(CPCMD_ALLOC, (uint16_t)dwalloc);

    cdclrl( MSG_LIN1, 1 );

    return;

}


/*  doautopilot - handle the autopilot */
/*  SYNOPSIS */
/*    int snum */
/*    doautopilot( snum ) */
void doautopilot( int snum )
{
    int ch;
    char *conf="Press [TAB] to engage autopilot:";

    cdclrl( MSG_LIN1, 2 );
    cbuf[0] = 0;
    if ( cdgetx( conf, MSG_LIN1, 1, TERMS, cbuf, MSGMAXLINE,
                 TRUE ) != TERM_EXTRA )
    {
        cdclrl( MSG_LIN1, 1 );
        return;
    }

    lastServerError = 0;
    sendCommand(CPCMD_AUTOPILOT, 1); /* turn over command to a machine */

    while ( clbStillAlive( Context.snum ) )
    {
        if (lastServerError)
            break;

        /* Get a character. */
        if ( ! iogtimed( &ch, 1.0 ) )
            continue;		/* next . echo */
        Context.msgok = FALSE;
        utGrand( &Context.msgrand );
        switch ( ch )
	{
	case TERM_ABORT:
            break;
	case TERM_REDRAW:	/* ^L */
            cdredo();
            break;
	default:
            mcuPutMsg( "Press [ESC] to abort autopilot.", MSG_LIN1 );
            cdbeep();
            cdrefresh();
	}
        Context.msgok = TRUE;
        if (ch == TERM_ABORT)
            break;
    }

    sendCommand(CPCMD_AUTOPILOT, 0);

    cdclrl( MSG_LIN1, 2 );

    return;
}


/*  dobeam - beam armies up or down (DOES LOCKING) */
/*  SYNOPSIS */
/*    int snum */
/*    dobeam( snum ) */
void dobeam( int snum )
{
    int pnum, num, upmax, downmax, capacity, beamax, i;
    int dirup = FALSE;
    int ch;
    char buf[MSGMAXLINE];
    real rkills;
    int done = FALSE;
    char *lastfew="Fleet orders prohibit removing the last three armies.";
    char *abt="...aborted...";

    cdclrl( MSG_LIN1, 2 );

    /* all of these checks are performed server-side as well, but we also
       check here for the obvious problems so we can save time without
       bothering the server for something it will refuse anyway. */

    /* at least the basic checks could be split into a seperate func that
       could be used by both client and server */

    /* Check for allowability. */
    if ( Ships[snum].warp >= 0.0 )
    {
        mcuPutMsg( "We must be orbiting a planet to use the transporter.",
                   MSG_LIN1 );
        return;
    }
    pnum = -Ships[snum].lock;
    if ( Ships[snum].armies > 0 )
    {
        if ( Planets[pnum].type == PLANET_SUN )
	{
            mcuPutMsg( "Idiot!  Our armies will fry down there!", MSG_LIN1 );
            return;
	}
        else if ( Planets[pnum].type == PLANET_MOON )
	{
            mcuPutMsg( "Fool!  Our armies will suffocate down there!",
                       MSG_LIN1 );
            return;
	}
        else if ( Planets[pnum].team == TEAM_GOD )
	{
            mcuPutMsg(
                "GOD->you: YOUR ARMIES AREN'T GOOD ENOUGH FOR THIS PLANET.",
                MSG_LIN1 );
            return;
	}
    }

    i = Planets[pnum].uninhabtime;
    if ( i > 0 )
    {
        sprintf( cbuf, "This planet is uninhabitable for %d more minute",
                 i );
        if ( i != 1 )
            utAppendChar(cbuf , 's') ;
        utAppendChar(cbuf , '.') ;
        mcuPutMsg( cbuf, MSG_LIN1 );
        return;
    }

    /* can take empty planets */
    if ( Planets[pnum].team != Ships[snum].team &&
         Planets[pnum].team != TEAM_SELFRULED &&
         Planets[pnum].team != TEAM_NOTEAM )
        if ( ! Ships[snum].war[Planets[pnum].team] && Planets[pnum].armies != 0)
        {
            mcuPutMsg( "But we are not at war with this planet!", MSG_LIN1 );
            return;
        }

    if ( Ships[snum].armies == 0 &&
         Planets[pnum].team == Ships[snum].team && Planets[pnum].armies <= MIN_BEAM_ARMIES )
    {
        mcuPutMsg( lastfew, MSG_LIN1 );
        return;
    }

    rkills = Ships[snum].kills;

    if ( rkills < (real)1.0 )
    {
        mcuPutMsg(
            "Fleet orders prohibit beaming armies until you have a kill.",
            MSG_LIN1 );
        return;
    }

    /* Figure out what can be beamed. */
    downmax = Ships[snum].armies;
    if ( clbSPWar(snum,pnum) ||
         Planets[pnum].team == TEAM_SELFRULED ||
         Planets[pnum].team == TEAM_NOTEAM ||
         Planets[pnum].team == TEAM_GOD ||
         Planets[pnum].armies == 0 )
    {
        upmax = 0;
    }
    else
    {
        capacity = min( (int)rkills * 2,
                        ShipTypes[Ships[snum].shiptype].armylim );
        upmax = min( Planets[pnum].armies - MIN_BEAM_ARMIES,
                     capacity - Ships[snum].armies );
    }

    /* If there are armies to beam but we're selfwar... */
    if ( upmax > 0 && selfwar(snum) && Ships[snum].team == Planets[pnum].team )
    {
        if ( downmax <= 0 )
	{
            strcpy(cbuf , "The arm") ;
            if ( upmax == 1 )
                strcat(cbuf , "y is") ;
            else
                strcat(cbuf , "ies are") ;
            strcat(cbuf , " reluctant to beam aboard a pirate vessel.") ;
            mcuPutMsg( cbuf, MSG_LIN1 );
            return;
	}
        upmax = 0;
    }

    /* Figure out which direction to beam. */
    if ( upmax <= 0 && downmax <= 0 )
    {
        mcuPutMsg( "There is no one to beam.", MSG_LIN1 );
        return;
    }
    if ( upmax <= 0 )
        dirup = FALSE;
    else if ( downmax <= 0 )
        dirup = TRUE;
    else
    {
        mcuPutMsg( "Beam [up or down] ", MSG_LIN1 );
        cdrefresh();
        done = FALSE;
        while ( clbStillAlive( Context.snum ) && done == FALSE)
	{
            if ( ! iogtimed( &ch, 1.0 ) )
	    {
                continue;	/* next */
	    }
            switch ( (char)tolower( ch ) )
	    {
	    case 'u':
	    case 'U':
                dirup = TRUE;
                done = TRUE;
                break;
	    case 'd':
	    case 'D':
	    case TERM_EXTRA:
                dirup = FALSE;
                done = TRUE;
                break;
	    default:
                mcuPutMsg( abt, MSG_LIN1 );
                return;
	    }
	}
    }

    if ( dirup )
        beamax = upmax;
    else
        beamax = downmax;

    /* Figure out how many armies should be beamed. */
    if ( dirup )
        strcpy(buf , "up") ;
    else
        strcpy(buf , "down") ;
    sprintf( cbuf, "Beam %s [1-%d] ", buf, beamax );
    cdclrl( MSG_LIN1, 1 );
    buf[0] = 0;
    ch = cdgetx( cbuf, MSG_LIN1, 1, TERMS, buf, MSGMAXLINE, TRUE );
    if ( ch == TERM_ABORT )
    {
        mcuPutMsg( abt, MSG_LIN1 );
        return;
    }
    else if ( ch == TERM_EXTRA && buf[0] == 0 )
        num = beamax;
    else
    {
        utDeleteBlanks( buf );
        if (!utIsDigits(buf))
	{
            mcuPutMsg( abt, MSG_LIN1 );
            return;
	}
        i = 0;
        utSafeCToI( &num, buf, i );			/* ignore status */
        if ( num < 1 || num > beamax )
	{
            mcuPutMsg( abt, MSG_LIN1 );
            return;
	}
    }

    /* now we start the phun. */

    lastServerError = 0;

    /* detail is (armies & 0x00ff), 0x8000 set if beaming down */

    sendCommand(CPCMD_BEAM,
                (dirup) ? (uint16_t)(num & 0x00ff):
                (uint16_t)((num & 0x00ff) | 0x8000));

    while(TRUE)			/* repeat infloop */
    {
        if ( ! clbStillAlive( Context.snum ) )
            return;

        if ( iochav() )
	{
            mcuPutMsg( abt, MSG_LIN1 );
            sendCommand(CPCMD_BEAM, 0); /* stop! */
            break;
	}

        if (lastServerError)
	{
            sendCommand(CPCMD_BEAM, 0); /* make sure */
            break;
	}

        utSleep( ITER_SECONDS );
    }

    /* Try to display the last beaming message. */
    cdrefresh();

    return;

}


/*  dobomb - bombard a planet (DOES LOCKING) */
/*  SYNOPSIS */
/*    int snum */
/*    dobomb( snum ) */
void dobomb( int snum )
{
    int pnum;
    char  buf[MSGMAXLINE];
    char *abt="...aborted...";

    SFCLR(snum, SHIP_F_REPAIR);;

    cdclrl( MSG_LIN2, 1 );
    cdclrl(MSG_LIN1, 1);

    /* Check for allowability. */
    if ( Ships[snum].warp >= 0.0 )
    {
        mcuPutMsg( "We must be orbiting a planet to bombard it.", MSG_LIN1 );
        return;
    }
    pnum = -Ships[snum].lock;
    if ( Planets[pnum].type == PLANET_SUN || Planets[pnum].type == PLANET_MOON ||
         Planets[pnum].team == TEAM_NOTEAM || Planets[pnum].armies == 0 )
    {
        mcuPutMsg( "There is no one there to bombard.", MSG_LIN1 );
        return;
    }
    if ( Planets[pnum].team == Ships[snum].team )
    {
        mcuPutMsg( "We can't bomb our own armies!", MSG_LIN1 );
        return;
    }
    if ( Planets[pnum].team != TEAM_SELFRULED && Planets[pnum].team != TEAM_GOD )
        if ( ! Ships[snum].war[Planets[pnum].team] )
        {
            mcuPutMsg( "But we are not at war with this planet!", MSG_LIN1 );
            return;
        }

    /* Confirm. */
    sprintf( cbuf, "Press [TAB] to bombard %s, %d armies:",
             Planets[pnum].name, Planets[pnum].armies );
    cdclrl( MSG_LIN1, 1 );
    cdclrl( MSG_LIN2, 1 );
    buf[0] = 0;
    if ( cdgetx( cbuf, MSG_LIN1, 1, TERMS, buf, MSGMAXLINE,
                 TRUE) != TERM_EXTRA )
    {
        cdclrl( MSG_LIN1, 1 );
        cdclrl( MSG_LIN2, 1 );
        return;
    }

    lastServerError = 0;		/* quit if something happens server-side */

    sendCommand(CPCMD_BOMB, 1);	/* start bombing */

    while(TRUE)
    {
        if ( ! clbStillAlive( Context.snum ) )
            break;

        if ( iochav() )
	{
            mcuPutMsg( abt, MSG_LIN1 );
            sendCommand(CPCMD_BOMB, 0);
            break;
	}

        if (lastServerError)
	{
            sendCommand(CPCMD_BOMB, 0); /* make sure */
            break;
	}

        utSleep( ITER_SECONDS );
    }

    cdrefresh();

    return;

}


/*  doburst - launch a burst of three torpedoes */
/*  SYNOPSIS */
/*    int snum */
/*    doburst( snum ) */
void doburst( int snum )
{
    real dir;

    cdclrl( MSG_LIN2, 1 );

    if ( mcuGetTarget( "Torpedo burst: ", MSG_LIN1, 1, &dir, Ships[snum].lastblast ) )
    {
        sendFireTorps(3, dir);
        cdclrl( MSG_LIN1, 1 );
    }
    else
    {
        mcuPutMsg( "Invalid targeting information.", MSG_LIN1 );
    }


    return;

}


/*  docloak - cloaking device control */
/*  SYNOPSIS */
/*    int snum */
/*    docloak( snum ) */
void docloak( int snum )
{
    char *pmt="Press [TAB] to engage cloaking device: ";

    cdclrl( MSG_LIN1, 1 );
    cdclrl( MSG_LIN2, 1 );

    if ( SCLOAKED(snum) )
    {
        sendCommand(CPCMD_CLOAK, 0);
        return;
    }

    cdclrl( MSG_LIN1, 1 );
    cbuf[0] = 0;
    if ( cdgetx( pmt, MSG_LIN1, 1, TERMS, cbuf, MSGMAXLINE,
                 TRUE) == TERM_EXTRA )
    {
        sendCommand(CPCMD_CLOAK, 0);
    }
    cdclrl( MSG_LIN1, 1 );

    return;

}

/*  dorefit - attempt to change the nature of things */
/*  SYNOPSIS */
/*    int snum */
/*    dorefit( snum ) */
void dorefit( int snum, int dodisplay )
{
    int ch, pnum, now, entertime, leave;
    char buf1[128];
    int oldstype = 0, stype = 0;
    char *ntp="We must be orbiting a team owned planet to refit.";
    char *nek="You must have at least one kill to refit.";
    char *conf="Press [TAB] to change, [ENTER] to accept: ";
    char *cararm="You cannot refit while carrying armies";

    cdclrl( MSG_LIN2, 1 );

    /* Check for allowability. */
    if ( oneplace( Ships[snum].kills ) < MIN_REFIT_KILLS )
    {
        mcuPutMsg( nek, MSG_LIN1 );
        return;
    }

    pnum = -Ships[snum].lock;

    if (Planets[pnum].team != Ships[snum].team || Ships[snum].warp >= 0.0)
    {
        mcuPutMsg( ntp, MSG_LIN1 );
        return;
    }

    if (Ships[snum].armies != 0)
    {
        mcuPutMsg( cararm, MSG_LIN1 );
        return;
    }

    /* we have at least 1 kill, and are */
    /* orbiting a team owned planet */

    stype = oldstype = Ships[snum].shiptype;
    leave = FALSE;

    while ( clbStillAlive( snum ) && !leave)
    {
        /* Display the current options. */

        cdclrl( MSG_LIN1, 1 );
        cdclrl( MSG_LIN2, 1 );

        buf1[0] = '\0';
        strcat(buf1, "Refit ship type: ") ;
        strcat(buf1, ShipTypes[stype].name) ;

        mcuPutMsg(buf1, MSG_LIN1);
        mcuPutMsg(conf, MSG_LIN2);

        cdrefresh();

        /* Get a character. */
        if ( ! iogtimed( &ch, 1.0 ) )
            continue; /* next; */
        switch ( ch )
        {
        case TERM_EXTRA:
            stype = utModPlusOne( stype + 1, MAXNUMSHIPTYPES );
            leave = FALSE;

            if ( dodisplay )
            {
                /* Force an update. */
                stopTimer();
                display( snum );
                startTimer();
            }

            break;

        case TERM_NORMAL:
            leave = TRUE;
            break;

        case TERM_ABORT:
            /* Decided to abort; restore things. */

            stype = oldstype;
            leave = TRUE;

            if ( dodisplay )
            {
                /* Force an update. */
                stopTimer();
                display( snum );		/* update the display */
                startTimer();
            }

            break;
        default:
            cdbeep();
            leave = FALSE;
            break;
        }
    }

    if (stype == oldstype)
    {
        cdclrl( MSG_LIN1, 1 );
        cdclrl( MSG_LIN2, 1 );

        return;
    }

    /* Now wait it out... */
    cdclrl( MSG_LIN1, 1 );
    cdclrl( MSG_LIN2, 1 );
    mcuPutMsg( "Refitting ship...", MSG_LIN1 );
    sendCommand(CPCMD_REFIT, (uint16_t)stype);
    cdrefresh();
    utGrand( &entertime );
    while ( utDeltaGrand( entertime, &now ) < REFIT_GRAND )
    {
        /* See if we're still alive. */
        if ( ! clbStillAlive( snum ) )
            return;

        /* Sleep */
        utSleep( ITER_SECONDS );
    }

    cdclrl( MSG_LIN1, 1 );
    cdclrl( MSG_LIN2, 1 );

    return;

}

/*  docoup - attempt to rise from the ashes (DOES LOCKING) */
/*  SYNOPSIS */
/*    int snum */
/*    docoup( snum ) */
void docoup( int snum )
{
    int i, pnum;
    char *nhp="We must be orbiting our home planet to attempt a coup.";
    char *conf="Press [TAB] to try it: ";

    cdclrl( MSG_LIN2, 1 );

    /* some checks we will do locally, the rest will have to be handled by
       the server */
    /* Check for allowability. */
    if ( oneplace( Ships[snum].kills ) < MIN_COUP_KILLS )
    {
        mcuPutMsg(
            "Fleet orders require three kills before a coup can be attempted.",
            MSG_LIN1 );
        return;
    }
    for ( i = 1; i <= NUMPLANETS; i = i + 1 )
        if ( Planets[i].team == Ships[snum].team && Planets[i].armies > 0 )
        {
            mcuPutMsg( "We don't need to coup, we still have armies left!",
                       MSG_LIN1 );
            return;
        }
    if ( Ships[snum].warp >= 0.0 )
    {
        mcuPutMsg( nhp, MSG_LIN1 );
        return;
    }
    pnum = -Ships[snum].lock;
    if ( pnum != Teams[Ships[snum].team].homeplanet )
    {
        mcuPutMsg( nhp, MSG_LIN1 );
        return;
    }
    if ( Planets[pnum].armies > MAX_COUP_ENEMY_ARMIES )
    {
        mcuPutMsg( "The enemy is still too strong to attempt a coup.",
                   MSG_LIN1 );
        return;
    }
    i = Planets[pnum].uninhabtime;
    if ( i > 0 )
    {
        sprintf( cbuf, "This planet is uninhabitable for %d more minutes.",
                 i );
        mcuPutMsg( cbuf, MSG_LIN1 );
        return;
    }


    /* at this point, we will just send the request, and let the server
       deal with it */

    /* Confirm. */
    cdclrl( MSG_LIN1, 1 );
    cbuf[0] = 0;
    if ( cdgetx( conf, MSG_LIN1, 1, TERMS, cbuf, MSGMAXLINE,
                 TRUE) != TERM_EXTRA )
    {
        mcuPutMsg( "...aborted...", MSG_LIN1 );
        return;
    }

    mcuPutMsg( "Attempting coup...", MSG_LIN1 );
    sendCommand(CPCMD_COUP, 0);

    return;

}


/*  docourse - set course */
/*  SYNOPSIS */
/*    int snum */
/*    docourse( snum ) */
void docourse( int snum )
{
    int i, j, what, sorpnum, xsorpnum, newlock, token, count;
    real dir, appx, appy;
    int ch;

    cdclrl( MSG_LIN1, 2 );

    cbuf[0] = 0;
    ch = cdgetx( "Come to course: ", MSG_LIN1, 1, TERMS, cbuf, MSGMAXLINE, TRUE );
    utDeleteBlanks( cbuf );
    if ( ch == TERM_ABORT || cbuf[0] == 0 )
    {
        cdclrl( MSG_LIN1, 1 );
        return;
    }

    newlock = 0;				/* default to no lock */

    what = NEAR_ERROR;
    if (utIsDigits(cbuf))
    {
        /* Raw angle. */
        cdclrl( MSG_LIN1, 1 );
        i = 0;
        if ( utSafeCToI( &j, cbuf, i ) )
	{
            what = NEAR_DIRECTION;
            dir = (real)utMod360( (real)( j ) );
	}
    }
    else if ( cbuf[0] == 's' && utIsDigits(&cbuf[1]) )
    {
        /* Ship. */

        i = 1;
        if ( utSafeCToI( &sorpnum, cbuf, i ) )
            what = NEAR_SHIP;
    }
    else if ( utArrowsToDir( cbuf, &dir ) )
        what = NEAR_DIRECTION;
    else if ( utIsSpecial( cbuf, &i, &token, &count ) )
    {
        if ( clbFindSpecial( snum, token, count, &sorpnum, &xsorpnum ) )
            what = i;
    }
    else if ( clbPlanetMatch( cbuf, &sorpnum, FALSE ) )
        what = NEAR_PLANET;

    switch ( what )
    {
    case NEAR_SHIP:
        if ( sorpnum < 1 || sorpnum > MAXSHIPS )
	{
            mcuPutMsg( "No such ship.", MSG_LIN2 );
            return;
	}
        if ( sorpnum == snum )
	{
            cdclrl( MSG_LIN1, 1 );
            return;
	}
        if ( Ships[sorpnum].status != SS_LIVE )
	{
            mcuPutMsg( "Not found.", MSG_LIN2 );
            return;
	}

        if ( SCLOAKED(sorpnum) )
	{
            if ( Ships[sorpnum].warp <= 0.0 )
	    {
                mcuPutMsg( "Sensors are unable to lock on.", MSG_LIN2 );
                return;
	    }
	}

        appx = Ships[sorpnum].x;
        appy = Ships[sorpnum].y;

        dir = utAngle( Ships[snum].x, Ships[snum].y, appx, appy );

        /* Give info if he used TAB. */
        if ( ch == TERM_EXTRA )
            mcuInfoShip( sorpnum, snum );
        else
            cdclrl( MSG_LIN1, 1 );
        break;
    case NEAR_PLANET:
        dir = utAngle( Ships[snum].x, Ships[snum].y, Planets[sorpnum].x, Planets[sorpnum].y );
        if ( ch == TERM_EXTRA )
	{
            newlock = -sorpnum;
            mcuInfoPlanet( "Now locked on to ", sorpnum, snum );
	}
        else
            mcuInfoPlanet( "Setting course for ", sorpnum, snum );
        break;
    case NEAR_DIRECTION:
        cdclrl( MSG_LIN1, 1 );
        break;
    case NEAR_NONE:
        mcuPutMsg( "Not found.", MSG_LIN2 );
        return;
        break;
    default:
        /* This includes NEAR_ERROR. */
        mcuPutMsg( "I don't understand.", MSG_LIN2 );
        return;
        break;
    }

    sendSetCourse(cInfo.sock, newlock, dir);

    return;

}


/*  dodet - detonate enemy torps */
/*  SYNOPSIS */
/*    int snum */
/*    dodet( snum ) */
void dodet( int snum )
{
    cdclrl( MSG_LIN2, 1 );

    if ( Ships[snum].wfuse > 0 )
        mcuPutMsg( "Weapons are currently overloaded.", MSG_LIN1 );
    else if ( clbUseFuel( snum, DETONATE_FUEL, TRUE, FALSE ) )
    {				/* we don't really use fuel here on the
				   client*/
        mcuPutMsg( "detonating...", MSG_LIN1 );
        sendCommand(CPCMD_DETENEMY, 0);
    }
    else
        mcuPutMsg( "Not enough fuel to fire detonators.", MSG_LIN1 );

    return;

}


/*  dodistress - send an emergency distress call */
/*  SYNOPSIS */
/*    int snum */
/*    dodistress( snum ) */
void dodistress( int snum )
{
    char *pmt="Press [TAB] to send an emergency distress call: ";

    cdclrl( MSG_LIN1, 2 );

    cbuf[0] = 0;
    if ( cdgetx( pmt, MSG_LIN1, 1, TERMS, cbuf, MSGMAXLINE,
                 TRUE) == TERM_EXTRA )
        sendCommand(CPCMD_DISTRESS, (uint16_t)UserConf.DistressToFriendly);

    cdclrl( MSG_LIN1, 1 );

    return;

}


/*  dohelp - display a list of commands */
/*  SYNOPSIS */
/*    int subdcl */
/*    dohelp( subdcl ) */
void dohelp( void )
{
    int lin, col, tlin;
    int ch;
    static int FirstTime = TRUE;
    static char sfmt[MSGMAXLINE * 2];

    if (FirstTime == TRUE)
    {
        FirstTime = FALSE;
        sprintf(sfmt,
                "#%d#%%-9s#%d#%%s",
                InfoColor,
                LabelColor);
    }

    cdclear();

    cdclear();
    cprintf(1,0,ALIGN_CENTER, "#%d#%s", LabelColor, "CONQUEST COMMANDS");

    lin = 3;

    /* Display the left side. */
    tlin = lin;
    col = 4;
    cprintf(tlin,col,ALIGN_NONE,sfmt, "0-9,=", "set warp factor (= is 10)");
    tlin++;
    cprintf(tlin,col,ALIGN_NONE,sfmt, "A", "change w/e allocations");
    tlin++;
    cprintf(tlin,col,ALIGN_NONE,sfmt, "b", "beam armies");
    tlin++;
    cprintf(tlin,col,ALIGN_NONE,sfmt, "B", "bombard a planet");
    tlin++;
    cprintf(tlin,col,ALIGN_NONE,sfmt, "C", "cloaking device");
    tlin++;
    cprintf(tlin,col,ALIGN_NONE,sfmt, "d,*", "detonate enemy torpedoes");
    tlin++;
    cprintf(tlin,col,ALIGN_NONE,sfmt, "D", "detonate your own torpedoes");
    tlin++;
    cprintf(tlin,col,ALIGN_NONE,sfmt, "E", "send emergency distress call");
    tlin++;
    cprintf(tlin,col,ALIGN_NONE,sfmt, "f", "fire phasers");
    tlin++;
    cprintf(tlin,col,ALIGN_NONE,sfmt, "F", "fire phasers, same direction");
    tlin++;
    cprintf(tlin,col,ALIGN_NONE,sfmt, "h", "this");
    tlin++;
    cprintf(tlin,col,ALIGN_NONE,sfmt,
            "H", "user history");
    tlin++;
    cprintf(tlin,col,ALIGN_NONE,sfmt, "i", "information");
    tlin++;
    cprintf(tlin,col,ALIGN_NONE,sfmt, "k", "set course");
    tlin++;
    cprintf(tlin,col,ALIGN_NONE,sfmt, "K", "try a coup");
    tlin++;
    cprintf(tlin,col,ALIGN_NONE,sfmt, "L", "review old messages");
    tlin++;
    cprintf(tlin,col,ALIGN_NONE,sfmt, "m", "send a message");
    tlin++;
    cprintf(tlin,col,ALIGN_NONE,sfmt, "M", "short/long range sensor toggle");
    tlin++;
    cprintf(tlin,col,ALIGN_NONE,sfmt, "N", "change your name");
    tlin++;
    cprintf(tlin,col,ALIGN_NONE,sfmt, "[SPACE]", "toggle map/lrscan");

    /* Now do the right side. */
    tlin = lin;
    col = 44;
    cprintf(tlin,col,ALIGN_NONE,sfmt, "O", "options menu");
    tlin++;
    cprintf(tlin,col,ALIGN_NONE,sfmt, "o", "come into orbit");
    tlin++;
    cprintf(tlin,col,ALIGN_NONE,sfmt, "p", "launch photon torpedo");
    tlin++;
    cprintf(tlin,col,ALIGN_NONE,sfmt, "P", "launch photon torpedo burst");
    tlin++;
    cprintf(tlin,col,ALIGN_NONE,sfmt, "Q", "initiate self-destruct");
    tlin++;
    if (sStat.flags & SPSSTAT_FLAGS_REFIT)
    {
        cprintf(tlin,col,ALIGN_NONE,sfmt, "r", "refit ship to new type");
        tlin++;
    }
    cprintf(tlin,col,ALIGN_NONE,sfmt, "R", "enter repair mode");
    tlin++;
    cprintf(tlin,col,ALIGN_NONE,sfmt, "S", "more user statistics");
    tlin++;
    cprintf(tlin,col,ALIGN_NONE,sfmt, "t", "engage tractor beams");
    tlin++;
    cprintf(tlin,col,ALIGN_NONE,sfmt, "T", "team list");
    tlin++;
    cprintf(tlin,col,ALIGN_NONE,sfmt, "u", "un-engage tractor beams");
    tlin++;
    cprintf(tlin,col,ALIGN_NONE,sfmt, "U", "user statistics");
    tlin++;
    cprintf(tlin,col,ALIGN_NONE,sfmt, "W", "set war or peace");
    tlin++;
    cprintf(tlin,col,ALIGN_NONE,sfmt, "-", "lower shields");
    tlin++;
    cprintf(tlin,col,ALIGN_NONE,sfmt, "+", "raise shields");
    tlin++;
    cprintf(tlin,col,ALIGN_NONE,sfmt,
            "/", "player list");
    tlin++;
    cprintf(tlin,col,ALIGN_NONE,sfmt, "?", "planet list");
    tlin++;
    cprintf(tlin,col,ALIGN_NONE,sfmt,
            "^L", "refresh the screen");
    tlin++;
    cprintf(tlin,col,ALIGN_NONE,sfmt,
            "[ENTER]", "get last info");
    tlin++;
    cprintf(tlin,col,ALIGN_NONE,sfmt, "[TAB]", "get next last info");

    mcuPutPrompt( MTXT_DONE, MSG_LIN2 );
    cdrefresh();
    while ( ! iogtimed( &ch, 1.0 ) && clbStillAlive( Context.snum ) )
        ;

    return;

}


/*  doinfo - do an info command */
/*  SYNOPSIS */
/*    int snum */
/*    doinfo( snum ) */
void doinfo( int snum )
{
    char ch;
    int i, j, what, sorpnum, xsorpnum, count, token, now[NOWSIZE];
    int extra;

    cdclrl( MSG_LIN1, 2 );

    cbuf[0] = 0;
    ch = (char)cdgetx( "Information on: ", MSG_LIN1, 1, TERMS, cbuf, MSGMAXLINE,
                       TRUE);
    if ( ch == TERM_ABORT )
    {
        cdclrl( MSG_LIN1, 1 );
        return;
    }
    extra = ( ch == TERM_EXTRA );

    /* Default to what we did last time. */
    utDeleteBlanks( cbuf );
    if ( cbuf[0] == 0 )
    {
        strcpy(cbuf , Context.lastinfostr) ;
        if ( cbuf[0] == 0 )
	{
            cdclrl( MSG_LIN1, 1 );
            return;
	}
    }
    else
        strcpy(Context.lastinfostr , cbuf) ;

    if ( utIsSpecial( cbuf, &what, &token, &count ) )
    {
        if ( ! clbFindSpecial( snum, token, count, &sorpnum, &xsorpnum ) )
            what = NEAR_NONE;
        else if ( extra )
	{
            if ( xsorpnum == 0 )
                what = NEAR_NONE;
            else
                sorpnum = xsorpnum;
	}

        if ( what == NEAR_SHIP )
            mcuInfoShip( sorpnum, snum );
        else if ( what == NEAR_PLANET )
            mcuInfoPlanet( "", sorpnum, snum );
        else
            mcuPutMsg( "Not found.", MSG_LIN2 );
    }
    else if ( cbuf[0] == 's' && utIsDigits(&cbuf[1]) )
    {
        i = 1;
        utSafeCToI( &j, cbuf, i );		/* ignore status */
        mcuInfoShip( j, snum );
    }
    else if (utIsDigits(cbuf))
    {
        i = 0;
        utSafeCToI( &j, cbuf, i );		/* ignore status */
        mcuInfoShip( j, snum );
    }
    else if ( clbPlanetMatch( cbuf, &j, FALSE ) )
        mcuInfoPlanet( "", j, snum );
    else
    {
        mcuPutMsg( "I don't understand.", MSG_LIN2 );
        cdmove( MSG_LIN1, 1 );
    }

    return;

}


/*  dolastphase - do a fire phasers same direction command */
/*  SYNOPSIS */
/*    int snum */
/*    dolastphase( snum ) */
void dolastphase( int snum )
{
    cdclrl( MSG_LIN1, 1 );

    sendCommand(CPCMD_FIREPHASER, (uint16_t)(Ships[snum].lastphase * 100.0));
    cdclrl( MSG_LIN2, 1 );

    return;

}


/*  domydet - detonate your own torps */
/*  SYNOPSIS */
/*    int snum */
/*    domydet( snum ) */
void domydet( int snum )
{
    cdclrl( MSG_LIN2, 1 );

    sendCommand(CPCMD_DETSELF, 0);

    mcuPutMsg( "Detonating...", MSG_LIN1 );

    return;

}

/*  doorbit - orbit the ship and print a message */
/*  SYNOPSIS */
/*    int snum */
/*    doorbit( snum ) */
void doorbit( int snum )
{
    int pnum;

    if ( ( Ships[snum].warp == ORBIT_CW ) || ( Ships[snum].warp == ORBIT_CCW ) )
        mcuInfoPlanet( "But we are already orbiting ", -Ships[snum].lock, snum );
    else if ( ! clbFindOrbit( snum, &pnum ) )
    {
        sprintf( cbuf, "We are not close enough to orbit, %s.",
                 Ships[snum].alias );
        mcuPutMsg( cbuf, MSG_LIN1 );
        cdclrl( MSG_LIN2, 1 );
    }
    else if ( Ships[snum].warp > MAX_ORBIT_WARP )
    {
        sprintf( cbuf, "We are going too fast to orbit, %s.",
                 Ships[snum].alias );
        mcuPutMsg( cbuf, MSG_LIN1 );
        sprintf( cbuf, "Maximum orbital insertion velocity is warp %.1f.",
                 oneplace(MAX_ORBIT_WARP) );
        mcuPutMsg( cbuf, MSG_LIN2 );
    }
    else
    {
        sendCommand(CPCMD_ORBIT, 0);
        mcuInfoPlanet( "Coming into orbit around ", pnum, snum );
    }

    return;

}


/*  dophase - do a fire phasers command */
/*  SYNOPSIS */
/*    int snum */
/*    dophase( snum ) */
void dophase( int snum )
{
    real dir;

    cdclrl( MSG_LIN2, 1 );

    if ( mcuGetTarget( "Fire phasers: ", MSG_LIN1, 1, &dir, Ships[snum].lastblast ) )
    {
        mcuPutMsg( "Firing phasers...", MSG_LIN2 );
        sendCommand(CPCMD_FIREPHASER, (uint16_t)(dir * 100.0));
    }
    else
    {
        mcuPutMsg( "Invalid targeting information.", MSG_LIN1 );
    }

    return;

}


/*  doplanlist - display the planet list for a ship */
/*  SYNOPSIS */
/*    int snum */
/*    doPlanetList( snum ) */
void doPlanetList( int snum )
{

    if (snum > 0 && snum <= MAXSHIPS)
        mcuPlanetList( Ships[snum].team, snum );
    else		/* then use user team if user doen't have a ship yet */
        mcuPlanetList( Users[Context.unum].team, snum );

    return;

}


/*  doreview - review messages for a ship */
/*  SYNOPSIS */
/*    int snum */
/*    doReviewMsgs( snum ) */
void doReviewMsgs( int snum )
{
    int ch;
    int lstmsg;			/* saved last msg in case new ones come in */

    lstmsg = Ships[snum].lastmsg;	/* don't want lstmsg changing while reading old ones. */

    if ( ! mcuReviewMsgs( snum, lstmsg ) )
    {
        mcuPutMsg( "There are no old messages.", MSG_LIN1 );
        mcuPutPrompt( MTXT_MORE, MSG_LIN2 );
        cdrefresh();
        while ( ! iogtimed( &ch, 1.0 ) && clbStillAlive( Context.snum ) )
            ;
        cdclrl( MSG_LIN1, 2 );
    }

    return;

}


/*  doselfdest - execute a self-destruct command */
/*  SYNOPSIS */
/*    doselfdest */
void doselfdest(int snum)
{
    char *pmt="Press [TAB] to initiate self-destruct sequence: ";

    cdclrl( MSG_LIN1, 2 );

    if ( SCLOAKED(snum) )
    {
        mcuPutMsg( "The cloaking device is using all available power.",
                   MSG_LIN1 );
        return;
    }

    cbuf[0] = 0;
    if ( cdgetx( pmt, MSG_LIN1, 1, TERMS, cbuf, MSGMAXLINE,
                 TRUE) != TERM_EXTRA )
    {
        /* Chickened out. */
        cdclrl( MSG_LIN1, 1 );
        return;
    }

    cdclrl( MSG_LIN1, 1 );

    lastServerError = 0;
    sendCommand(CPCMD_DESTRUCT, 1); /* blow yourself up */

    while ( TRUE )
    {
        if ( ! clbStillAlive( Context.snum ) )
            return;			/* Died in the process. */

        if ( iochav() )
	{
            /* Got a new character. */
            cdclrl( MSG_LIN1, 2 );
            if ( iogchar() == TERM_ABORT )
	    {
                sendCommand(CPCMD_DESTRUCT, 0);
                mcuPutMsg( "Self destruct has been canceled.", MSG_LIN1 );
                return;
	    }
            else
	    {
                mcuPutMsg( "Press [ESC] to abort self destruct.", MSG_LIN1 );
                cdbeep();
                cdrefresh();
	    }
	}

        if (lastServerError)
        {
            sendCommand(CPCMD_DESTRUCT, 0); /* make sure */
            break;
        }


        utSleep( ITER_SECONDS );
    } /* end while */

    return;

}


/*  doshields - raise or lower shields */
/*  SYNOPSIS */
/*    int snum */
/*    int up */
/*    doshields( snum, up ) */
void doshields( int snum, int up )
{

    if (!sendCommand(CPCMD_SETSHIELDS, (uint16_t)up))
        return;

    if ( up )
    {
        SFCLR(snum, SHIP_F_REPAIR);
        mcuPutMsg( "Shields raised.", MSG_LIN1 );
    }
    else
        mcuPutMsg( "Shields lowered.", MSG_LIN1 );

    cdclrl( MSG_LIN2, 1 );

    return;

}


/*  doteamlist - display the team list for a ship */
/*  SYNOPSIS */
/*    int team */
/*    doTeamList( team ) */
void doTeamList( int team )
{
    int ch;

    cdclear();
    while ( clbStillAlive( Context.snum ) )
    {
        mcuTeamList( team );
        mcuPutPrompt( MTXT_DONE, MSG_LIN2 );
        cdrefresh();
        if ( iogtimed( &ch, 1.0 ) )
            break;
    }
    return;

}


/*  dotorp - launch single torpedoes */
/*  SYNOPSIS */
/*    int snum */
/*    dotorp( snum ) */
void dotorp( int snum )
{
    real dir;

    cdclrl( MSG_LIN2, 1 );

    if ( SCLOAKED(snum) )
    {
        mcuPutMsg( "The cloaking device is using all available power.",
                   MSG_LIN1 );
        return;
    }
    if ( Ships[snum].wfuse > 0 )
    {
        mcuPutMsg( "Weapons are currently overloaded.", MSG_LIN1 );
        return;
    }
    if ( Ships[snum].fuel < TORPEDO_FUEL )
    {
        mcuPutMsg( "Not enough fuel to launch a torpedo.", MSG_LIN1 );
        return;
    }
    if ( mcuGetTarget( "Launch torpedo: ", MSG_LIN1, 1, &dir, Ships[snum].lastblast ) )
    {
        if ( ! clbCheckLaunch( snum, 1 ) )
            mcuPutMsg( ">TUBES EMPTY<", MSG_LIN2 );
        else
	{			/* a local approx */
            sendFireTorps(1, dir);
            cdclrl( MSG_LIN1, 1 );
	}
    }
    else
    {
        mcuPutMsg( "Invalid targeting information.", MSG_LIN1 );
    }

    return;

}

/*  dotow - attempt to tow another ship (DOES LOCKING) */
/*  SYNOPSIS */
/*    int snum */
/*    dotow( snum ) */
void dotow( int snum )
{
    char ch;
    int i, other;

    cdclrl( MSG_LIN1, 2 );
    if ( Ships[snum].towedby != 0 )
    {
        strcpy(cbuf , "But we are being towed by ") ;
        utAppendShip( Ships[snum].towedby, cbuf );
        utAppendChar(cbuf , '!') ;
        mcuPutMsg( cbuf, MSG_LIN2 );
        return;
    }
    if ( Ships[snum].towing != 0 )
    {
        strcpy(cbuf , "But we're already towing ") ;
        utAppendShip( Ships[snum].towing, cbuf );
        utAppendChar(cbuf , '.') ;
        mcuPutMsg( cbuf, MSG_LIN2 );
        return;
    }
    cbuf[0] = 0;
    ch = (char)cdgetx( "Tow which ship? ", MSG_LIN1, 1, TERMS, cbuf, MSGMAXLINE,
                       TRUE);
    cdclrl( MSG_LIN1, 1 );
    if ( ch == TERM_ABORT )
        return;

    i = 0;
    utSafeCToI( &other, cbuf, i );		/* ignore status */

    sendCommand(CPCMD_TOW, (uint16_t)other);

    return;

}


/*  dowarp - set warp factor */
/*  SYNOPSIS */
/*    int snum */
/*    real warp */
/*    dowarp( snum, warp ) */
void dowarp( int snum, real warp )
{
    cdclrl( MSG_LIN2, 1 );

    /* Handle ship limitations. */

    warp = min( warp, ShipTypes[Ships[snum].shiptype].warplim );
    if (!sendCommand(CPCMD_SETWARP, (uint16_t)warp))
        return;

    sprintf( cbuf, "Warp %d.", (int) warp );
    mcuPutMsg( cbuf, MSG_LIN1 );

    return;

}

/*  gretds - block letter "greetings..." */
/*  SYNOPSIS */
/*    gretds */
void gretds()
{

    int col,lin;
    char *g1=" GGG   RRRR   EEEEE  EEEEE  TTTTT   III   N   N   GGG    SSSS";
    char *g2="G   G  R   R  E      E        T      I    NN  N  G   G  S";
    char *g3="G      RRRR   EEE    EEE      T      I    N N N  G       SSS";
    char *g4="G  GG  R  R   E      E        T      I    N  NN  G  GG      S  ..  ..  ..";
    char *g5=" GGG   R   R  EEEEE  EEEEE    T     III   N   N   GGG   SSSS   ..  ..  ..";

    col = (int)(Context.maxcol - strlen(g5)) / (int)2;
    lin = 1;
    cprintf( lin,col,ALIGN_NONE,"#%d#%s", InfoColor, g1);
    lin++;
    cprintf( lin,col,ALIGN_NONE,"#%d#%s", InfoColor, g2);
    lin++;
    cprintf( lin,col,ALIGN_NONE,"#%d#%s", InfoColor, g3);
    lin++;
    cprintf( lin,col,ALIGN_NONE,"#%d#%s", InfoColor, g4);
    lin++;
    cprintf( lin,col,ALIGN_NONE,"#%d#%s", InfoColor, g5);

    return;

}


/*  menu - main user menu (DOES LOCKING) */
/*  SYNOPSIS */
/*    menu */
void menu(void)
{

    int i, lin, col, sleepy, countdown;
    int ch;
    int lose, oclosed, switchteams, multiple, redraw;
    int playrv;
    int rv;
    int pkttype;
    struct timeval timeout;
    fd_set readfds;
    char buf[PKT_MAXSIZE];
    char *if1="Suddenly  a  sinister,  wraithlike  figure appears before you";
    char *if2="seeming to float in the air.  In a low,  sorrowful  voice  he";
    char *if3="says, \"Alas, the very nature of the universe has changed, and";
    char *if4="your ship cannot be found.  All must now pass away.\"  Raising";
    char *if5="his  oaken  staff  in  farewell,  he fades into the spreading";
    char *if6="darkness.  In his place appears a  tastefully  lettered  sign";
    char *if7="reading:";
    char *if8="INITIALIZATION FAILURE";
    char *if9="The darkness becomes all encompassing, and your vision fails.";

    catchSignals();	/* enable trapping of interesting signals */


    /* we will init some things.  Then we will look for either an
       SP_SHIP or NAK packet (if something bad happened) */

    /* Initialize statistics. */
    initstats( &Ships[Context.snum].ctime, &Ships[Context.snum].etime );

    /* Log this entry into the Game. */
    Context.histslot = clbLogHist( Context.unum );

    /* Set up some things for the menu display. */
    switchteams = Users[Context.unum].ooptions[OOPT_SWITCHTEAMS];
    multiple = Users[Context.unum].ooptions[OOPT_MULTIPLE];
    oclosed = ConqInfo->closed;
    Context.leave = FALSE;
    redraw = TRUE;
    sleepy = 0;
    countdown = 0;
    playrv = FALSE;


    /* now look for our ship packet before we get started.  It should be a
       full SP_SHIP packet for this first time */
    if (pktWaitForPacket(SP_SHIP, buf, PKT_MAXSIZE,
                         60, NULL) <= 0)
    {
        utLog("conquest:menu: didn't get initial SP_SHIP");
        return;
    }
    else
        procShip(buf);

    do
    {
        /* Make sure things are proper. */

        lose = FALSE;

        while ((pkttype = pktWaitForPacket(PKT_ANYPKT,
                                           buf, PKT_MAXSIZE, 0, NULL)) > 0)
	{			/* proc packets while we get them */
            switch (pkttype)
	    {
	    case SP_ACK:
                PKT_PROCSP(buf);
                if (sAckMsg.code == PERR_LOSE)
                    lose = TRUE;
                else
                    utLog("conquest:menu: got unexp ack code %d", sAckMsg.code);

                break;

	    default:
                processPacket(buf);
                break;
	    }
	}

        if (pkttype < 0)		/* some error */
	{
            utLog("conquest:menu:waiForPacket returned %d", pkttype);
            Ships[Context.snum].status = SS_OFF;
            return;
	}


        if ( lose )				/* again, Jorge? */
	{
            /* We reincarnated or else something bad happened. */
            lin = 7;
            col = 11;
            cdclear();;
            cdredo();;
            cprintf( lin,col,ALIGN_NONE,"#%d#%s", RedLevelColor, if1);
            lin++;
            cprintf( lin,col,ALIGN_NONE,"#%d#%s", RedLevelColor, if2);
            lin++;
            cprintf( lin,col,ALIGN_NONE,"#%d#%s", RedLevelColor, if3);
            lin++;
            cprintf( lin,col,ALIGN_NONE,"#%d#%s", RedLevelColor, if4);
            lin++;
            cprintf( lin,col,ALIGN_NONE,"#%d#%s", RedLevelColor, if5);
            lin++;
            cprintf( lin,col,ALIGN_NONE,"#%d#%s", RedLevelColor, if6);
            lin++;
            cprintf( lin,col,ALIGN_NONE,"#%d#%s", RedLevelColor, if7);
            lin+=2;
            cprintf( lin,col,ALIGN_NONE,"#%d#%s", RedLevelColor | A_BLINK, if8);
            lin+=2;
            cprintf( lin,col,ALIGN_NONE,"#%d#%s", RedLevelColor, if9);
            ioeat();
            cdmove( 1, 1 );
            cdrefresh();
            return;
	}

        /* Some simple housekeeping. */
        if ( multiple != Users[Context.unum].ooptions[OOPT_MULTIPLE] )
	{
            multiple = ! multiple;
            redraw = TRUE;
	}

        if ( switchteams != Users[Context.unum].ooptions[OOPT_SWITCHTEAMS])
	{
            switchteams = Users[Context.unum].ooptions[OOPT_SWITCHTEAMS];
            redraw = TRUE;
	}
        if ( oclosed != ConqInfo->closed )
	{
            oclosed = ! oclosed;
            redraw = TRUE;
	}
        if ( redraw )
	{
            conqds( multiple, switchteams );
            redraw = FALSE;
	}
        else
            cdclrl( MSG_LIN1, 2 );

        clbUserline( -1, -1, cbuf, FALSE, TRUE );
        uiPutColor(LabelColor);
        cdputs( cbuf, MSG_LIN1, 1 );
        clbUserline( Context.unum, 0, cbuf, FALSE, TRUE );
        uiPutColor(CQC_A_BOLD);
        cdputs( cbuf, MSG_LIN2, 1 );
        uiPutColor(0);

        cdmove( 1, 1 );
        cdrefresh();

        /*  we've been in the menu long enough? */
        if ( countdown > 0 )
            countdown--;

        /* wait up to a second for something to happen */
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;

        FD_ZERO(&readfds);
        FD_SET(cInfo.sock, &readfds);
        FD_SET(iolbStdinFD, &readfds);

        if ((rv=select((max(cInfo.sock, iolbStdinFD) + 1), &readfds, NULL,
                       NULL, &timeout)) > 0)
        {                           /* we have activity */
            if (FD_ISSET(cInfo.sock, &readfds))
            {                   /* we have a packet */
                continue;            /* go process them at the top */
            }

            if (FD_ISSET(iolbStdinFD, &readfds))
            {          /* got a char */
                ch = iogchar();

                /* Got a character, zero timeout. */
                sleepy = 0;
                switch ( ch )
                {
                case 'e':
                    if (!sendCommand(CPCMD_ENTER, 0))
                        playrv = FALSE;
                    else
                    {
                        playrv = play();
                        countdown = 15;
                        redraw = TRUE;
                    }
                    if (playrv == FALSE)
                        Context.leave = TRUE; /* something didn't work right */

                    Context.display = FALSE;
                    break;
                case 'h':
                    mcuHelpLesson();
                    redraw = TRUE;
                    break;
                case 'H':
                    mcuHistList( FALSE );
                    redraw = TRUE;
                    break;
                case 'L':
                    doReviewMsgs( Context.snum );
                    break;
                case 'n':
                    if ( ! Context.hasnewsfile )
                        cdbeep();
                    else
                    {
                        mcuNews();
                        redraw = TRUE;
                    }
                    break;
                case 'N':
                    /*	  pseudo( Context.unum, Context.snum );*/
                    cucPseudo( Context.unum, Context.snum );
                    break;
                case 'O':
                    UserOptsMenu(Context.unum);
                    redraw = TRUE;
                    break;
                case 'r':
                    if ( multiple )
                        cdbeep();
                    else
                    {
                        for ( i = 1; i <= MAXSHIPS; i = i + 1 )
                            if ( Ships[i].status == SS_LIVE ||
                                 Ships[i].status == SS_ENTERING )
                                if ( Ships[i].unum == Context.unum )
                                    break;
                        if ( i <= MAXSHIPS )
                            cdbeep();
                        else
                        {
                            cdclrl( MSG_LIN1, 2 );
                            cdrefresh();
                            if ( mcuConfirm() )
                            {
                                /* should exit here */
                                sendCommand(CPCMD_RESIGN, 0);
                                cdend();
                                exit(0);
                                break;
                            }
                        }
                    }
                    break;
                case 's':
                    if ( ! multiple && ! switchteams )
                        cdbeep();
                    else
                    {
                        /* we'll update local data here anyway, even though it will be
                           overwritten on the next ship update.  Improves perceived
                           response time. */
                        Ships[Context.snum].team =
                            utModPlusOne( Ships[Context.snum].team+1, NUMPLAYERTEAMS );
                        Ships[Context.snum].shiptype =
                            Teams[Ships[Context.snum].team].shiptype;
                        Users[Context.unum].team = Ships[Context.snum].team;
                        Ships[Context.snum].war[Ships[Context.snum].team] = FALSE;
                        Users[Context.unum].war[Users[Context.unum].team] = FALSE;

                        sendCommand(CPCMD_SWITCHTEAM, (uint16_t)Ships[Context.snum].team);
                    }
                    break;
                case 'S':
                    mcuUserStats( FALSE, 0 ); /* we're never really neutral ;-) - dwp */
                    redraw = TRUE;
                    break;
                case 'T':
                    doTeamList( Ships[Context.snum].team );
                    redraw = TRUE;
                    break;
                case 'U':
                    mcuUserList( FALSE, 0 );
                    redraw = TRUE;
                    break;
                case 'W':
                    /*	  dowar( Context.snum );*/
                    cucDoWar( Context.snum );
                    redraw = TRUE;
                    break;
                case 'q':
                case 'Q':
                    Context.leave = TRUE;
                    break;
                case '/':
                    mcuPlayList( FALSE, FALSE, 0 );
                    redraw = TRUE;
                    break;
                case '?':
                    doPlanetList( 0 );
                    redraw = TRUE;
                    break;
                case TERM_REDRAW:	/* ^L */
                    cdredo();
                    redraw = TRUE;
                    break;
                case ' ':
                case TERM_NORMAL:
                    /* Do nothing. */
                    break;
                default:
                    cdbeep();
                    break;
                }
            }
        }

        if (rv < 0 && errno != EINTR)
        {
            utLog("conquest:menu: select returned %d, %s\n",
                  rv, strerror(errno));
            Context.leave = TRUE;
        }

        /* We get here if a char hasn't been typed. */
        sleepy++;
        if ( sleepy > 300 )
            break;
    }
    while ( clbStillAlive( Context.snum ) &&  !Context.leave );

    return;

}


/*  newship - here we will await a ClientStat from the server (indicating
    our possibly new ship), or a NAK indicating a problem.
*/
/*  SYNOPSIS */
/*    int status, newship, unum, snum */
/*    int flag, newship */
/*    flag = newship( unum, snum ) */
int newship( int unum, int *snum )
{
    int i, j;
    char cbuf[MSGMAXLINE];
    int pkttype;
    char buf[PKT_MAXSIZE];

    /* here we will wait for ack's or a clientstat pkt. Acks indicate an
       error.  If the clientstat pkt's esystem is !0, we need to prompt
       for the system to enter and send it in a CP_COMMAND:CPCMD_ENTER
       pkt. */


    while (TRUE)
    {
        if ((pkttype = pktWaitForPacket(PKT_ANYPKT,
                                        buf, PKT_MAXSIZE, 60, NULL)) < 0)
	{
            utLog("conquest:newship: waitforpacket returned %d", pkttype);
            return FALSE;
	}

        switch (pkttype)
	{
	case 0:			/* timeout */
            return FALSE;
            break;

	case SP_ACK:		/* bummer */
            PKT_PROCSP(buf);
            switch (sAckMsg.code)
	    {
	    case PERR_FLYING:
                cdclear();
                cdredo();
                sprintf(cbuf, "You're already playing on another ship.");
                cprintf(5,0,ALIGN_CENTER,"#%d#%s",InfoColor, cbuf);
                Ships[*snum].status = SS_RESERVED;
                mcuPutPrompt( "--- press any key ---", MSG_LIN2 );
                cdrefresh();

                iogchar();

                break;

	    case PERR_TOOMANYSHIPS:
                cdclear();
                cdredo();
                i = MSG_LIN2/2;
                cdputc(
                    "I'm sorry, but your playing on too many ships right now.", i );
                i = i + 1;
                strcpy(cbuf , "You are only allowed to fly ") ;
                j = Users[unum].multiple;
                utAppendInt(cbuf , j) ;
                strcat(cbuf , " ship") ;
                if ( j != 1 )
                    utAppendChar(cbuf , 's') ;
                strcat(cbuf , " at one time.") ;
                cdputc( cbuf, i );
                cdrefresh();
                utSleep( 2.0 );
                Ships[*snum].status = SS_RESERVED;

                break;

	    default:
                utLog("newship: unexpected ack code %d",
                      sAckMsg.code);
                break;
	    }

            return FALSE;		/* always a failure */
            break;

	case SP_CLIENTSTAT:
            if (PKT_PROCSP(buf))
            {
                /* Context.snum,unum & team will be set by the dispatch routine */

                if (sClientStat.esystem == 0)	/* we are done */
                    return TRUE;

                /* otherwise, need to prompt for system to enter */
                if (selectentry(sClientStat.esystem))
                    return TRUE;		/* done */
                else
                    return FALSE;
            }
            else
            {
                utLog("newship: invalid CLIENTSTAT\n");
                return FALSE;       /* don't want to hang if a bad pkt */
            }

            break;

            /* we might get other packets too */
	default:
            processPacket(buf);

            break;
	}
    }

    /* if we are here, something unexpected happened */
    return FALSE;			/* NOTREACHED */


}


/*  play - play the game  ( PLAY ) */
/*  SYNOPSIS */
/*    play */
int play()
{
    int ch, rv;
    struct timeval timeout;
    fd_set readfds;

    /* Can't carry on without a vessel. */

    if ( (rv = newship( Context.unum, &Context.snum )) != TRUE)
        return(rv);
    else
        Context.entship = TRUE;

    Ships[Context.snum].sdfuse = 0;	/* zero self destruct fuse */
    utGrand( &Context.msgrand );		/* initialize message timer */
    Context.leave = FALSE;		/* assume we won't want to bail */
    Context.redraw = TRUE;		/* want redraw first time */
    Context.msgok = TRUE;		/* ok to get messages */
    cdclear();			/* clear the display */
    cdredo();			/*  (quickly) */
    stopTimer();			/* stop the display interrupt */
    display( Context.snum );	/* update the screen manually */
    Context.display = TRUE;		/* ok to display */
    clientFlags = 0;
    startTimer();			/* setup for next second */


    /* start recording if neccessary */
    if (Context.recmode == RECMODE_STARTING)
    {
        if (recInitOutput(Context.unum, time(0), Context.snum,
                          FALSE))
        {
            Context.recmode = RECMODE_ON;
        }
        else
            Context.recmode = RECMODE_OFF;
    }

    /* need to tell the server to resend all the data it already
       sent in menu, since our ship might have changed. */
    sendCommand(CPCMD_RELOAD, 0);

    /* While we're alive, field commands and process them. Inbound
       packets are handled by astservice() */
    /* when we become 'killed', then dead processing can begin. */
    while ( !(clientFlags & SPCLNTSTAT_FLAG_KILLED) )
    {

        /* Get a packet or char with one second timeout. */
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;

        FD_ZERO(&readfds);
        FD_SET(cInfo.sock, &readfds);
        FD_SET(iolbStdinFD, &readfds);

        if ((rv=select((max(cInfo.sock, iolbStdinFD) + 1), &readfds, NULL,
                       NULL, &timeout)) > 0)
        {                       /* we have activity */
            if (FD_ISSET(cInfo.sock,&readfds))
            {                   /* we have a packet */
                /* call astservice.  it will get any packets,
                   and update the display */
                stopTimer();      /* to be sure */
                astservice(0);    /* will restart the timer */
            }

            /* Get a char */

            if (FD_ISSET(iolbStdinFD, &readfds))
            {
                ch = iogchar();
                /* only process commands if we are live (the server will ignore
                   them anyway) */
                if (Ships[Context.snum].status == SS_LIVE)
                {
                    if (ibufExpandMacro(((ch - KEY_F(0)) - 1)) == TRUE)
                    {
                        while (ibufCount())
                        {
                            ch = ibufGetc();
                            command( ch );
                        }
                    }
                    else
                    {
                        do
                        {
                            command( ch );
                        } while (ibufCount() && (ch = ibufGetc()));
                    }
                }
            }
        }


        if (rv < 0 && errno != EINTR)
        {
            utLog("conquest:play: select returned %d, %s\n",
                  rv, strerror(errno));
            Context.display = FALSE;
            return FALSE;
        }
        else
        {
            Context.msgok = TRUE;
            cdrefresh();
        }

        /* send a UDP keepalive packet if it's time */
        sendUDPKeepAlive(0);

    } /* while stillalive */

    Context.display = FALSE;

    /* Asts are still enabled, simply cancel the next screen update. */
    stopTimer();

    utSleep( 1.0 );
    dead( Context.snum, Context.leave );

    return(TRUE);

}


/*  welcome - entry routine */
/*  SYNOPSIS */
/*    int flag, welcome */
/*    int unum */
/*    flag = welcome( unum ) */
int welcome(void)
{
    int i, team, col;
    char name[MAXUSERNAME];
    char *sorry1="I'm sorry, but the game is closed for repairs right now.";
    char *sorry2="I'm sorry, but there is no room for a new player right now.";
    char *sorryn="Please try again some other time.  Thank you.";
    char * selected_str="You have been selected to command a";
    char * starship_str=" starship.";
    char * prepare_str="Prepare to be beamed aboard...";
    int pkttype;
    char buf[PKT_MAXSIZE];
    int done = FALSE;

    col=0;

    if (!Logon(name))
    {
        utLog("conquest: Logon failed.");
        return FALSE;
    }

    while (!done)
    {
        /* now look for SP_CLIENTSTAT or SP_ACK */
        if ((pkttype =
             pktRead(buf, PKT_MAXSIZE, 60)) <= 0)
        {
            utLog("welcome: read SP_CLIENTSTAT or SP_ACK failed: %d\n", pkttype);
            return FALSE;
        }

        switch (pkttype)
        {
        case SP_CLIENTSTAT:
            if (PKT_PROCSP(buf))
            {
                /* Context.snum,unum & team will be set by the dispatch routine */
                done = TRUE;
            }
            else
            {
                utLog("welcome: invalid CLIENTSTAT\n");
                return FALSE;
            }

            break;
        case SP_ACK:
            PKT_PROCSP(buf);
            done = TRUE;
            break;
        default:
            utLog("welcome: got unexpected packet type %d. Ignoring.",
                  pkttype);
            done = FALSE;         /* redundant, but it gets the point across */
            break;
        }
    }

    if ( pkttype == SP_CLIENTSTAT && (sClientStat.flags & SPCLNTSTAT_FLAG_NEW) )
    {
        /* Must be a new player. */
        cdclear();
        cdredo();
        if ( ConqInfo->closed )
	{
            /* Can't enroll if the game is closed. */
            cprintf(MSG_LIN2/2,col,ALIGN_CENTER,"#%d#%s", InfoColor, sorry1 );
            cprintf(MSG_LIN2/2+1,col,ALIGN_CENTER,"#%d#%s", InfoColor, sorryn );
            cdmove( 1, 1 );
            cdrefresh();
            utSleep( 2.0 );
            return ( FALSE );
	}
        team = sClientStat.team;
        cbuf[0] = 0;
        utAppendTitle(cbuf , team) ;
        utAppendChar(cbuf , ' ') ;
        i = strlen( cbuf );
        strcat(cbuf , name) ;
        cbuf[i] = (char)toupper( cbuf[i] );

        gretds();			/* 'GREETINGS' */

        if ( vowel( Teams[team].name[0] ) )
            cprintf(MSG_LIN2/2,0,ALIGN_CENTER,"#%d#%s%c #%d#%s #%d#%s",
                    InfoColor,selected_str,'n',CQC_A_BOLD,Teams[team].name,
                    InfoColor,starship_str);
        else
            cprintf(MSG_LIN2/2,0,ALIGN_CENTER,"#%d#%s #%d#%s #%d#%s",
                    InfoColor,selected_str,CQC_A_BOLD,Teams[team].name,
                    InfoColor,starship_str);
        cprintf(MSG_LIN2/2+1,0,ALIGN_CENTER,"#%d#%s",
                InfoColor, prepare_str );
        cdmove( 1, 1 );
        cdrefresh();
        utSleep( 3.0 );
    }


    if (pkttype == SP_ACK)	/* some problem was detected */
    {
        switch (sAckMsg.code)
	{
	case PERR_CLOSED:
            cdclear();
            cdredo();
            cprintf(MSG_LIN2/2,col,ALIGN_CENTER,"#%d#%s", InfoColor, sorry1 );
            cprintf(MSG_LIN2/2+1,col,ALIGN_CENTER,"#%d#%s", InfoColor, sorryn );
            cdmove( 1, 1 );
            cdrefresh();
            utSleep( 2.0 );
            return ( FALSE );

            break;

	case PERR_REGISTER:
            cprintf(MSG_LIN2/2,col,ALIGN_CENTER,"#%d#%s", InfoColor, sorry2 );
            cprintf(MSG_LIN2/2+1,col,ALIGN_CENTER,"#%d#%s", InfoColor, sorryn );
            cdmove( 1, 1 );
            cdrefresh();
            utSleep( 2.0 );
            return ( FALSE );

            break;

	case PERR_NOSHIP:
            cdclear();
            cdredo();
            cdputc( "I'm sorry, but there are no ships available right now.",
                    MSG_LIN2/2 );
            cdputc( sorryn, MSG_LIN2/2+1 );
            cdmove( 1, 1 );
            cdrefresh();
            utSleep( 2.0 );
            return ( FALSE );

            break;

	default:
            utLog("welcome: unexpected ACK code %d\n", sAckMsg.code);
            return FALSE;
            break;

	}

        return FALSE;		/* NOTREACHED */
    }

    /* if we are here, then we recieved a client stat pkt, and now must wait
       for a user packet for this user. */


    if (pktWaitForPacket(SP_USER, buf, PKT_MAXSIZE,
                         60, NULL) <= 0)
    {
        utLog("conquest:welcome: waitforpacket SP_USER returned error");
        return FALSE;
    }
    else
        procUser(buf);


    /* ready to rock. */
    return ( TRUE );
}

void catchSignals(void)
{
#ifdef DEBUG_SIG
    utLog("catchSignals() ENABLED");
#endif

    signal(SIGHUP, (void (*)(int))handleSignal);
    signal(SIGTSTP, SIG_IGN);
    signal(SIGTERM, (void (*)(int))handleSignal);
    signal(SIGINT, SIG_IGN);
    signal(SIGQUIT, (void (*)(int))handleSignal);

    return;
}


void handleSignal(int sig)
{

#ifdef DEBUG_SIG
    utLog("handleSignal() got SIG %d", sig);
#endif

    switch(sig)
    {
    case SIGQUIT:
    case SIGINT:
    case SIGTERM:
    case SIGHUP:
        stopTimer();
        cdclear();
        cdrefresh();
        conqend();		/* sends a disconnect packet */
        cdend();

        exit(0);
        break;

    default:
        break;
    }

    catchSignals();	/* reset */
    return;
}

/*  astservice - ast service routine for conquest */
/*  SYNOPSIS */
/*    astservice */
/* This routine gets called from a sys$setimr ast. Normally, it */
/* checks for new packet and outputs one screen update and then sets up */
/* another timer request. */
void astservice(int sig)
{
    int now;
    int difftime;
    char buf[PKT_MAXSIZE];
    static uint32_t iterstart = 0;
    uint32_t iternow = clbGetMillis();
    const uint32_t iterwait = 50.0; /* ms */
    real tdelta = (real)iternow - (real)iterstart;

    stopTimer();

    /* good time to look for packets... We do this here so the display
       doesn't stop when executing various commands */


    while (pktRead(buf, PKT_MAXSIZE, 0) > 0)
        processPacket(buf); /* process them */

    /* drive the local universe */
    if (tdelta > iterwait)
    {
        clbPlanetDrive(tdelta / 1000.0);
        clbTorpDrive(tdelta / 1000.0);
        iterstart = iternow;
        recGenTorpLoc();
    }

    /* Don't do anything if we're not supposed to. */
    if ( ! Context.display )
    {
        startTimer();
        return;
    }

    /* See if we can display a new message. */

    /* for people with 25 lines, we
       use a different timer so that
       NEWMSG_GRAND intervals will determine
       whether it's time to display a new
       message... Otherwise, Context.msgrand
       is used - which means that NEWMSG_GRAND
       interval will have to pass after issuing
       any command before a new msg will disp
       12/28/98 */
    if ( Context.msgok )
    {
        difftime = utDeltaGrand( Context.msgrand, &now );

        if ( difftime >= NEWMSG_GRAND )
            if ( utGetMsg( Context.snum, &Ships[Context.snum].lastmsg ) )
            {
                if (mcuReadMsg( Context.snum, Ships[Context.snum].lastmsg,
                                MSG_MSG ) == TRUE)
                {
                    if (Msgs[Ships[Context.snum].lastmsg].msgfrom !=
                        Context.snum)
                        if (UserConf.MessageBell)
                            cdbeep();
                    /* set both timers, regardless of which
                       one we're actally concerned with */
                    Context.msgrand = now;
                }
            }
    }

    /* Perform one ship display update. */
    display( Context.snum );

    recUpdateFrame();          /* update recording */

    /* Schedule for next time. */
    startTimer();

    return;

}

/*  stopTimer - cancel timer */
/*  SYNOPSIS */
/*    stopTimer */
void stopTimer(void)
{
    int old_disp;                 /* whether we should re-enable
                                     Context.display */
#ifdef HAVE_SETITIMER
    struct itimerval itimer;
#endif

    /* we need to turn off display while doing this.  Preserve the previous
       value on return */
    old_disp = Context.display;
    if (Context.display)
        Context.display = FALSE;


    signal(SIGALRM, SIG_IGN);

#ifdef HAVE_SETITIMER
    itimer.it_value.tv_sec = itimer.it_interval.tv_sec = 0;
    itimer.it_value.tv_usec = itimer.it_interval.tv_usec = 0;

    setitimer(ITIMER_REAL, &itimer, NULL);
#else
    alarm(0);
#endif

    Context.display = old_disp;

    return;

}


/*  startTimer - set timer to display() */
/*  SYNOPSIS */
/*    csetimer */
void startTimer(void)
{
    static struct sigaction Sig;

#ifdef HAVE_SETITIMER
    struct itimerval itimer;
#endif

    Sig.sa_handler = (void (*)(int))astservice;

    Sig.sa_flags = 0;

    if (sigaction(SIGALRM, &Sig, NULL) == -1)
    {
        utLog("startTimer():sigaction(): %s\n", strerror(errno));
        exit(errno);
    }

#ifdef HAVE_SETITIMER

    if (Context.updsec >= 1 && Context.updsec <= 10)
    {
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
    }
    else
    {
        itimer.it_value.tv_sec = 0;
        itimer.it_value.tv_usec = (1000000 / 2); /* 2/sec */
    }

    itimer.it_interval.tv_sec = itimer.it_value.tv_sec;
    itimer.it_interval.tv_usec = itimer.it_value.tv_usec;

    setitimer(ITIMER_REAL, &itimer, NULL);
#else
    alarm(1);			/* set alarm() */
#endif
    return;

}

/*  conqend - machine dependent clean-up */
/*  SYNOPSIS */
/*    conqend */
void conqend(void)
{
    sendCommand(CPCMD_DISCONNECT, 0); /* tell the server */
    recCloseOutput();

    return;

}

void dispServerInfo(int lin, metaSRec_t *metaServerList, int num)
{
    char buf[BUFFER_SIZE_1024];

    cdline ( lin, 0, lin, Context.maxcol );
    lin++;

    sprintf(buf, "#%d#Server: #%d#  %%s", MagentaColor, NoColor);
    cprintf(lin++, 0, ALIGN_NONE, buf, metaServerList[num].servername);

    sprintf(buf, "#%d#Version: #%d# %%s", MagentaColor, NoColor);
    cprintf(lin++, 0, ALIGN_NONE, buf, metaServerList[num].serverver);

    sprintf(buf,
            "#%d#Status: #%d#  Ships #%d#%%d/%%d #%d#"
            "(#%d#%%d #%d#active, #%d#%%d #%d#vacant, "
            "#%d#%%d #%d#robot)",
            MagentaColor, NoColor, CyanColor, NoColor,
            CyanColor, NoColor, CyanColor, NoColor,
            CyanColor, NoColor);

    cprintf(lin++, 0, ALIGN_NONE, buf,
            (metaServerList[num].numactive + metaServerList[num].numvacant +
             metaServerList[num].numrobot),
            metaServerList[num].numtotal,
            metaServerList[num].numactive,
            metaServerList[num].numvacant, metaServerList[num].numrobot);

    sprintf(buf, "#%d#Flags: #%d#   %%s", MagentaColor, NoColor);
    cprintf(lin++, 0, ALIGN_NONE, buf,
            clntServerFlagsStr(metaServerList[num].flags));

    sprintf(buf, "#%d#MOTD: #%d#    %%s", MagentaColor, NoColor);
    cprintf(lin++, 0, ALIGN_NONE, buf, metaServerList[num].motd);

    sprintf(buf, "#%d#Contact: #%d# %%s", MagentaColor, NoColor);
    cprintf(lin++, 0, ALIGN_NONE, buf, metaServerList[num].contact);

    sprintf(buf, "#%d#Time: #%d#    %%s", MagentaColor, NoColor);
    cprintf(lin++, 0, ALIGN_NONE, buf, metaServerList[num].walltime);

    cdline ( lin, 0, lin, Context.maxcol );
    return;
}


int selectServer(metaSRec_t *metaServerList, int nums)
{
    int i, k;
    static char *header = "Server List";
    static char *header2fmt = "(Page %d of %d)";
    static char headerbuf[BUFFER_SIZE];
    static char header2buf[BUFFER_SIZE];
    static char *eprompt = "Arrow keys to select, [TAB] or [ENTER] to accept, any other key to quit.";
    int Done = FALSE;
    int ch;
    char *dispmac;
    int lin = 0, col = 0, flin, llin, clin, pages, curpage;
    const int servers_per_page = 8;

    /* this is the number of required pages,
       though page accesses start at 0 */
    if (nums >= servers_per_page)
    {
        pages = nums / servers_per_page;
        if ((nums % servers_per_page) != 0)
            pages++;		/* for runoff */
    }
    else
        pages = 1;


    /* init the servervec array */
    for (i=0; i < nums; i++)
    {
        if (metaServerList[i].version >= 2) /* valid for newer meta protocols */
            servervec[i].vers = metaServerList[i].protovers;
        else
            servervec[i].vers = PROTOCOL_VERSION; /* always 'compatible' */

        snprintf(servervec[i].hostname, (MAXHOSTNAME + 10) - 1, "%s:%hu",
                 metaServerList[i].altaddr,
                 metaServerList[i].port);
    }

    curpage = 0;

    cdclear();			/* First clear the display. */

    flin = 11;			/* first server line */
    llin = 0;			/* last server line on this page */
    clin = 0;			/* current server line */


    while (Done == FALSE)
    {
        sprintf(header2buf, header2fmt, curpage + 1, pages);
        sprintf(headerbuf, "%s %s", header, header2buf);

        cdclrl( 1, MSG_LIN2);	/* clear screen area */
        lin = 1;
        col = ((int)(Context.maxcol - strlen(headerbuf)) / 2);

        cprintf(lin, col, ALIGN_NONE, "#%d#%s", NoColor, headerbuf);

        lin = flin;
        col = 1;

        i = 0;			/* start at index 0 */

				/* figure out the last editable line on
				   this page */

        if (curpage == (pages - 1)) /* last page - might be less than full */
            llin = (nums % servers_per_page);	/* ..or more than empty? ;-) */
        else
            llin = servers_per_page;

        i = 0;
        while (i < llin)
	{			/* display this page */
				/* get the server number for this line */
            k = (curpage * servers_per_page) + i;

            dispmac = servervec[k].hostname;

            /* highlight the currently selected line */
            if (i == clin)
            {
                if (servervec[k].vers == PROTOCOL_VERSION)
                    cprintf(lin, col, ALIGN_NONE, "#%d#%s#%d#",
                            RedLevelColor, dispmac, NoColor);
                else
                    cprintf(lin, col, ALIGN_NONE,
                            "#%d#%s#%d# (unavailable - incompatible protocol)",
                            BlueColor |CQC_A_BOLD, dispmac, NoColor);

            }
            else
            {
                if (servervec[k].vers == PROTOCOL_VERSION)
                    cprintf(lin, col, ALIGN_NONE, "#%d#%s#%d#",
                            InfoColor, dispmac, NoColor);
                else
                    cprintf(lin, col, ALIGN_NONE,
                            "#%d#%s#%d# (unavailable - incompatible protocol)",
                            BlueColor, dispmac, NoColor);
            }

            lin++;
            i++;
	}

        /* now the editing phase */
        cdclrl( MSG_LIN1, 2  );
        cdputs(eprompt, MSG_LIN1, 1);

        if (clin >= llin)
            clin = llin - 1;

        dispServerInfo(2, metaServerList, clin);
        cdmove(flin + clin, 1);

        /* Get a char */
        ch = iogchar();


        switch(ch)
	{
	case KEY_UP:		/* up */
	case KEY_LEFT:
	case 'w':
	case 'k':
            clin--;
            if (clin < 0)
	    {
                if (pages != 1)
		{
                    curpage--;
                    if (curpage < 0)
		    {
                        curpage = pages - 1;
		    }
		}

                /* setup llin  for current page */
                if (curpage == (pages - 1))
                    llin = (nums % servers_per_page);
                else
                    llin = servers_per_page;

                clin = llin - 1;
	    }
            break;

	case KEY_DOWN:		/* down */
	case KEY_RIGHT:
	case 'x':
	case 'j':
            clin++;
            if (clin >= llin)
	    {
                if (pages != 1)
		{
                    curpage++;
                    if (curpage >= pages)
		    {
                        curpage = 0;
		    }
		}

                clin = 0;
	    }
            break;

	case KEY_PPAGE:		/* prev page */
            if (pages != 1)
	    {
                curpage--;
                if (curpage < 0)
		{
                    curpage = pages - 1;
		}
	    }

            break;

	case KEY_NPAGE:		/* next page */
            if (pages != 1)
	    {
                curpage++;
                if (curpage >= pages)
		{
                    curpage = 0;
		}
	    }

            break;

	case TERM_EXTRA:        /* selected one */
        case TERM_NORMAL:

            i = ((curpage * servers_per_page) + clin);

            if ((metaServerList[i].protovers == PROTOCOL_VERSION) ||
                metaServerList[i].version < 2) /* too old to know for sure */
            {
                return i;
            }
            else
                cdbeep();

            break;

	default:		/* everything else */
            return -1;
            break;
	}

    }

    return TRUE;
}
