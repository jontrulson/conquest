#include "c_defs.h"

/************************************************************************
 *
 * $Id$
 *
 * Copyright 1999-2004 Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 ***********************************************************************/

/*                               C O N Q U E S T */
/*            Copyright (C)1983-1986 by Jef Poskanzer and Craig Leres */
/*    Permission to use, copy, modify, and distribute this software and */
/*    its documentation for any purpose and without fee is hereby granted, */
/*    provided that this copyright notice appear in all copies and in all */
/*    supporting documentation. Jef Poskanzer and Craig Leres mak2003 */
/*    representations about the suitability of this software for any */
/*    purpose. It is provided "as is" without express or implied warranty. */

/**********************************************************************/
/* Unix/C specific porting and supporting code Copyright (C)1994-1996 */
/* by Jon Trulson <jon@radscan.com> under the same terms and          */
/* conditions of the original copyright by Jef Poskanzer and Craig    */
/* Leres.                                                             */
/*                                                                    */
/**********************************************************************/

#define NOEXTERN
#include "conqdef.h"
#include "conqcom.h"
#include "context.h"
#include "conf.h"
#include "global.h"
#include "color.h"
#include "record.h"
#include "ibuf.h"
#include "gldisplay.h"
#include "conqnet.h"
#include "packet.h"
#include "client.h"
#include "clientlb.h"
#include "clntauth.h"

#include "meta.h"

#include "nPlayBMenu.h"
#include "nConsvr.h"
#include "nMeta.h"

#include "conqinit.h"
#include "cqsound.h"

void catchSignals(void);
void handleSignal(int sig);
void conqend(void);

void printUsage()
{
  printf("Usage: conquestgl [-m ][-s server[:port]] [-r recfile] [ -t ]\n");
  printf("                  [-f ][-g <geometry>] [ -u ]\n");
  printf("                  [ -M <metaserver> ] [ -B ]\n\n");
  printf("    -f               run in fullscreen mode\n");
  printf("    -g <geometry>    specify intial window width/height.\n");
  printf("                      Format is WxH (ex: 1024x768).\n");
  printf("                      Default is 1024x768.\n");
  printf("    -m               query the metaserver\n");
  printf("    -s server[:port] connect to <server> at <port>\n");
  printf("                      default: localhost:1701\n");
  printf("    -r recfile       Record game to <recfile>\n");
  printf("                      recfile will be in compressed format\n");
  printf("                      if conquest was compiled with libz\n");
  printf("                      support\n");
  printf("    -t               telnet mode (no user conf load/save)\n");
  
  printf("    -M metaserver    specify alternate <metaserver> to contact.\n");
  printf("                      default: %s\n", META_DFLT_SERVER);

                                 
  printf("    -P <cqr file>    Play back a Conquest recording (.cqr)\n");
  printf("    -B               Benchmark mode.  When playing back a recording,\n");
  printf("                      the default playback speed will be as fast as possible.\n");
  printf("    -u               do not attempt to use UDP from server.\n");
  printf("    -S               disable sound support.\n");
  printf("    -v               be more verbose.\n");
  
                                 
  return;
}

int getLocalhost(char *buf, int len)
{
  struct hostent *hp;

  gethostname ( buf, len );
  if ((hp = gethostbyname(buf)) == NULL) 
    {
      fprintf(stderr, "conquest: gethostbyname(): cannot get localhost info.\n");
      return FALSE;
    }

  return TRUE;
}

/* parse the geometry arg.  Ensure that dConf init w/h are only
 *  set if the geom arg was reasonably valid. Format WxH (ex: 1024x768)
 */
static void parseGeometry(char *geom)
{
  char geomcpy[32];
  char *ch;
  int w, h;

  if (!geom || !*geom)
    return;

  memset((void *)geomcpy, 0, 32);
  strncpy(geomcpy, geom, 32 - 1);

  if ((ch = strchr(geomcpy, 'x')) == NULL)
    return;                     /* invalid */

  *ch = 0;
  ch++;

  if (!*ch)
    return;

  w = abs(atoi(geomcpy));
  h = abs(atoi(ch));

  if (!w || !h)
    return;

  /* set it up */
  dConf.initWidth = w;
  dConf.initHeight = h;

  return;
}

static int _cmpfile(void *cmp1, void *cmp2)
{
  char *str1 = *(char **)cmp1;
  char *str2 = *(char **)cmp2;
  return(strcmp(str1, str2));
}

static void _loadRCFiles(int type, char *cqdir, char *suffix)
{
  char filenm[BUFFER_SIZE];
  DIR *dirp;
  struct dirent *direntp;
  char **filelist = NULL;
  char **tmplist = NULL;
  int numfiles = 0;
  int i;

  if (!cqdir || !suffix)
    return;


  if ((dirp = opendir(cqdir)))
    {
      while ((direntp = readdir(dirp)) != NULL)
        {                       /* here, we will just check out the .trc
                                   files */
          int len;
          int suff_len = strlen(suffix); 

          if ((!strcmp(direntp->d_name, "..") ||
               !strcmp(direntp->d_name, ".")))
            continue;

          len = strlen(direntp->d_name);
          if (len < suff_len)
            continue;
          
          if (!strncmp(&(direntp->d_name[len - suff_len]), suffix, suff_len)) 
            {                   /* found one */
              char *ptr;

              snprintf(filenm, sizeof(filenm)-1, "%s/%s", 
                       cqdir, direntp->d_name);

              if (!(ptr = strdup(filenm)))
                {
                  clog("%s: Could not strdup %s, ignoring.",
                       __FUNCTION__,
                       filenm);

                }

              /* add it to the list */

              tmplist = (char **)realloc((void *)filelist, 
                                         sizeof(char *) * 
                                         (numfiles + 1));
              
              if (!tmplist)
                {  
                  clog("%s: Could not realloc %d files, ignoring '%s'",
                       __FUNCTION__,
                       numfiles + 1,
                       filenm);
                  if (ptr)
                    free(ptr);

                  continue;
                }
              
              filelist = tmplist;
              filelist[numfiles] = ptr;
              numfiles++;
            }
        }

      /* now sort it */
      if (!numfiles)
        return;

      qsort(filelist, numfiles, sizeof(char *),  
            (int (*)(const void *, const void *))_cmpfile); 

      /* print them out for testing */
      for(i=0; i <numfiles; i++)
        {
          cqiLoadRC(type, filelist[i], 1, 0);
          free(filelist[i]);
        }
      free(filelist);
      filelist = NULL;
      numfiles = 0;
    }

  return;
}

/* first load the main texturesrc and *.trc files.  Then look for and
   load any ~/.conquest/ *.trc files */
void loadTextureRCFiles()
{
  char cqdir[BUFFER_SIZE];
  char *homevar;

  /* load the main texturesrc file first.  It's fatal if this fails. */
  if (!cqiLoadRC(CQI_FILE_TEXTURESRC, NULL, 1, 0))
      exit(1);

  /* now load any .trc files in there (CONQETC) */
  _loadRCFiles(CQI_FILE_TEXTURESRC_ADD, CONQETC, ".trc");

  /* now load any in the users own ~/.conquest/ dir */
  if ((homevar = getenv("HOME")) == NULL)
    return;

  snprintf(cqdir, sizeof(cqdir)-1, "%s/.conquest", 
           homevar);

  _loadRCFiles(CQI_FILE_TEXTURESRC_ADD, cqdir, ".trc");

  return;
}

void loadSoundRCFiles()
{
  char cqdir[BUFFER_SIZE];
  char *homevar;

  /* load the main sound file first */
  cqiLoadRC(CQI_FILE_SOUNDRC, NULL, 1, 0);

  /* now load any .src files in there (CONQETC) */
  _loadRCFiles(CQI_FILE_SOUNDRC_ADD, CONQETC, ".src");

  /* now load any in the users own ~/.conquest/ dir */
  if ((homevar = getenv("HOME")) == NULL)
    return;

  snprintf(cqdir, sizeof(cqdir)-1, "%s/.conquest", 
           homevar);

  _loadRCFiles(CQI_FILE_SOUNDRC_ADD, cqdir, ".src");

  return;
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
  int dosound = TRUE;

  Context.entship = FALSE;
  Context.recmode = RECMODE_OFF;
  Context.updsec = 10;		/* dflt - 10/sec */
  Context.msgrand = getnow(NULL, 0);

  cInfo.sock = -1;
  cInfo.usock = -1;
  cInfo.doUDP = FALSE;
  cInfo.tryUDP = TRUE;
  cInfo.state = CLT_STATE_PREINIT;
  cInfo.serverDead = TRUE;
  cInfo.isLoggedIn = FALSE;
  cInfo.remoteport = CN_DFLT_PORT;

  setSystemLog(FALSE, TRUE);	/* use $HOME for logfile */
  if (!getLocalhost(cInfo.localhost, MAXHOSTNAME))
    return(1);

  cInfo.remotehost = strdup("localhost"); /* default to your own server */

  dspInitData();

  /* check options */
  while ((i = getopt(argc, argv, "fmM:s:r:tP:Bug:Sv")) != EOF)    
    switch (i)
      {
      case 'B':                 /* Benchmark mode, set frameDelay to 0.0 */
        frameDelay = 0.0;
        break;
      case 'f':
        dConf.fullScreen = TRUE;
        break;
      case 'g':                 
        parseGeometry(optarg);
        break;
      case 'm':
        wantMetaList = TRUE;
        break;
      case 'M':
        metaServer = optarg;
        break;
      case 's':                 /* [host[:port]] */
	cInfo.remotehost = (Unsgn8 *)strdup(optarg);
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
              cInfo.remotehost = "localhost";
          }
        else
          cInfo.remoteport = CN_DFLT_PORT;
        
        serveropt = TRUE;

	break;
      case 'r': 
        /* don't want to do this if we've already seen -P */
        if (Context.recmode != RECMODE_PLAYING)
          {
            if (recordOpenOutput(optarg, FALSE))
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

      case 't':
        confSetTelnetClientMode(TRUE);
        break;

      case 'P':
        rfname = optarg;
        Context.recmode = RECMODE_PLAYING;
        dosound = FALSE;        /* no sound during playback */
        break;

      case 'u':
        cInfo.tryUDP = FALSE;
        break;
 
      case 'S':
        dosound = FALSE;
        break;

      case 'v':
        cqDebug++;
        break;

     default:
	printUsage();
	exit(1);
      }

  rndini( 0, 0 );		/* initialize random numbers */
  
  
#ifdef DEBUG_CONFIG
  clog("%s@%d: main() Reading Configuration files.", __FILE__, __LINE__);
#endif
  
  if (GetConf(0) == ERR)	
    {
#ifdef DEBUG_CONFIG
      clog("%s@%d: main(): GetConf() returned ERR.", __FILE__, __LINE__);
#endif
	exit(1);
      }

  Context.updsec = UserConf.UpdatesPerSecond;
  
  if (Context.recmode == RECMODE_PLAYING) 
    {
      if (serveropt || wantMetaList)
        printf("-P option specified.  All other options ignored.\n");

      serveropt = wantMetaList = FALSE;
      printf("Scanning file %s...\n", rfname);
      if (!initReplay(rfname, &totElapsed))
        exit(1);

      /* now init for real */
      if (!initReplay(rfname, NULL))
        exit(1);

      Context.unum = MSG_GOD;       /* stow user number */
      Context.snum = ERR;           /* don't display in cdgetp - JET */
      Context.entship = FALSE;      /* never entered a ship */
      Context.histslot = ERR;       /* useless as an op */
      Context.lasttdist = Context.lasttang = 0;
      Context.lasttarg[0] = EOS;

      /* turn off annoying beeps */
      UserConf.DoAlarms = FALSE;
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

  /* load the globals/planets, etc */
  cqiLoadRC(CQI_FILE_CONQINITRC, NULL, 1, 0);

  /* load the main texturesrc file and any user supplied ~/.conquest/ *.trc
     files */
  loadTextureRCFiles();

  /* a parallel universe, it is */
  map_lcommon();

  /* init and load the sounds */
  if (dosound)
    {
      loadSoundRCFiles();
      cqsInitSound();
    }
  else
    cqsSoundAvailable = FALSE;

#ifdef DEBUG_FLOW
  clog("%s@%d: main() starting conqinit().", __FILE__, __LINE__);
#endif
  
  conqinit();			/* machine dependent initialization */
  iBufInit();
  
#ifdef DEBUG_FLOW
  clog("%s@%d: main() starting cdinit().", __FILE__, __LINE__);
#endif

  
  uiGLInit(&argc, argv);
  
  cqsMusicPlay(cqsFindMusic("intro"), FALSE);

  Context.maxlin = 25;
  Context.maxcol = 80;
  Context.snum = 0;		/* force menu to get a new ship */
  Context.histslot = ERR;
  Context.lasttang = Context.lasttdist = 0;
  Context.lasttarg[0] = EOS;
  catchSignals();       /* enable trapping of interesting signals */

  /* which node to start from... */

  if (Context.recmode == RECMODE_PLAYING)
    nPlayBMenuInit();
  else if (wantMetaList)
    nMetaInit(metaServerList, nums);
  else
    nConsvrInit(cInfo.remotehost, cInfo.remoteport);

#ifdef DEBUG_FLOW
  clog("%s@%d: main() welcoming player.", __FILE__, __LINE__);
#endif

  /* start the fun! */
  glutMainLoop();
  
  exit(0);
  
}

void catchSignals(void)
{
#ifdef DEBUG_SIG
  clog("catchSignals() ENABLED");
#endif
  
  signal(SIGHUP, (void (*)(int))handleSignal);
  /*  signal(SIGTSTP, SIG_IGN);*/
  signal(SIGTERM, (void (*)(int))handleSignal);  
  signal(SIGINT, SIG_IGN);
  signal(SIGQUIT, (void (*)(int))handleSignal);
  
  return;
}

void handleSignal(int sig)
{
  
#ifdef DEBUG_SIG
  clog("handleSignal() got SIG %d", sig);
#endif
  
  switch(sig)
    {
    case SIGQUIT:
    case SIGINT:
    case SIGTERM:
    case SIGHUP:
      conqend();		/* sends a disconnect packet */
      exit(0);
      break;

    default:
      break;
    }

  catchSignals();	/* reset */
  return;
}

/*  conqend - machine dependent clean-up */
/*  SYNOPSIS */
/*    conqend */
void conqend(void)
{
  if (cInfo.sock != -1)
    sendCommand(CPCMD_DISCONNECT, 0); /* tell the server */
  recordCloseOutput();

  return;
  
}

