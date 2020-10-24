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
#include <vector>
#include <algorithm>

#include "conqdef.h"
#include "global.h"

#include "cb.h"

#include "conqutil.h"
#include "rndlb.h"
#include "conqunix.h"

#include "context.h"

#include "conf.h"

#include "color.h"
#include "record.h"
#include "ibuf.h"
#include "gldisplay.h"
#include "GL.h"
#include "conqnet.h"
#include "packet.h"
#include "client.h"
#include "clientlb.h"
#include "conqlb.h"

#include "meta.h"

#include "nPlayBMenu.h"
#include "nConsvr.h"
#include "nMeta.h"

#include "conqinit.h"
#include "cqsound.h"
#include "hud.h"

#include "playback.h"

void catchSignals(void);
void handleSignal(int sig);
void conqend(void);

void printUsage()
{
    printf("Usage: conquest   [-s server[:port]] [-f] [-g <geometry>] \n");
    printf("                  [-u] [-S] [-v] [-M <metaserver>] \n");
    printf("                  [-B]\n\n");
    printf("    -f               run in fullscreen mode\n");
    printf("    -g <geometry>    specify intial window width/height.\n");
    printf("                      Format is WxH (ex: 1024x768).\n");
    printf("                      Default is 1280x720.\n");
    printf("    -s server[:port] connect to <server> at <port>\n");
    printf("                      default port: 1701\n");

    printf("    -M metaserver    specify alternate <metaserver> to contact.\n");
    printf("                      default: %s\n", META_DFLT_SERVER);


    printf("    -P <cqr file>    Play back a Conquest recording (.cqr)\n");
    printf("    -B               Benchmark mode.  When playing back a recording,\n");
    printf("                      the default playback speed will be as fast as possible.\n");
    printf("                      NOTE: most systems limit refresh to 60Hz, so it may\n");
    printf("                      not really be \"as fast as possible\".\n");
    printf("    -u               do not attempt to establish a UDP link to the server.\n");
    printf("    -S               disable sound support.\n");
    printf("    -v               be more verbose.  More '-v's increase verbosity\n\n");

    printf("The default action without arguments is to query the metaserver\n");
    printf("for a list of servers to connect to.\n");

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
        return false;
    }

    return true;
}

/* parse the geometry arg.  Ensure that dConf init w/h are only
 *  set if the geom arg was reasonably valid. Format WxH (ex: 1024x768)
 */
static void parseGeometry(char *geom)
{
    const int geoSize = 32;
    char geomcpy[geoSize];
    char *ch;
    int w, h;

    if (!geom || !*geom)
        return;

    memset((void *)geomcpy, 0, geoSize);
    utStrncpy(geomcpy, geom, geoSize);

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

static void _loadRCFiles(int type, const char *cqdir, const char *suffix)
{
    DIR *dirp;
    struct dirent *direntp;
    std::vector<std::string> filelist;

    if (!cqdir || !suffix)
        return;


    if ((dirp = opendir(cqdir)))
    {
        while ((direntp = readdir(dirp)) != NULL)
        {
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
                std::string filenm = cqdir + std::string("/") + direntp->d_name;
                filelist.push_back(filenm);
            }
        }

        // nothing to do
        if (filelist.empty())
        {
            closedir(dirp);
            return;
        }

        // now sort it
        sort(filelist.begin(), filelist.end());

        /* load them up */
        for (std::string filename : filelist)
        {
            if (cqiLoadRC(type, filename.c_str(), cqDebug))
            {
                utLog("%s: cqiLoadRC(%s) failed, ignoring.",
                      __FUNCTION__, filename.c_str());
            }
        }

        closedir(dirp);
    }

    return;
}

/* first load the main texturesrc and *.trc files.  Then look for and
   load any ~/.conquest/ *.trc files */
void loadTextureRCFiles()
{
    char cqdir[PATH_MAX];
    char *homevar;

    /* load the main texturesrc file first.  It's fatal if this fails. */
    if (cqiLoadRC(CQI_FILE_TEXTURESRC, NULL, cqDebug))
    {
        utLog("%s: FATAL: cannot load texturesrc file.",
              __FUNCTION__);

        exit(1);
    }

    /* now load any .trc files in there (CONQETC) */
    _loadRCFiles(CQI_FILE_TEXTURESRC_ADD, utGetPath(CONQETC), ".trc");

    /* now load any in the users own ~/.conquest/ dir */
    if ((homevar = getenv(CQ_USERHOMEDIR)) == NULL)
        return;

    snprintf(cqdir, PATH_MAX, "%s/%s",
             homevar, CQ_USERCONFDIR);

    _loadRCFiles(CQI_FILE_TEXTURESRC_ADD, cqdir, ".trc");

    return;
}

void loadSoundRCFiles()
{
    char cqdir[PATH_MAX];
    char *homevar;

    /* load the main sound file first */
    if (cqiLoadRC(CQI_FILE_SOUNDRC, NULL, cqDebug))
    {
        utLog("%s: FATAL: cannot load soundrc file.",
              __FUNCTION__);

        exit(1);
    }

    /* now load any .src files in there (CONQETC) */
    _loadRCFiles(CQI_FILE_SOUNDRC_ADD, utGetPath(CONQETC), ".src");

    /* now load any in the users own ~/.conquest/ dir */
    if ((homevar = getenv(CQ_USERHOMEDIR)) == NULL)
        return;

    snprintf(cqdir, PATH_MAX, "%s/%s",
             homevar, CQ_USERCONFDIR);

    _loadRCFiles(CQI_FILE_SOUNDRC_ADD, cqdir, ".src");

    return;
}


/*  conquest - main program */
int main(int argc, char *argv[])
{
    int i;
    char *ch;
    int wantMetaList = true;     /* default is metaserver list */
    int serveropt = false;        /* specified a server with '-s' */
    int dosound = true;
#if defined(MINGW)
    WSADATA wsaData;
    int rv;
#endif


    /* tell the packet routines that we are a client */
    pktSetClientMode(true);
    pktSetClientProtocolVersion(PROTOCOL_VERSION);

    Context.recmode = RECMODE_OFF;
    Context.updsec = MAX_UPDATE_PER_SEC;		/* dflt - 10/sec */
    Context.msgrand = time(0);

    utStrncpy(cInfo.metaServer, META_DFLT_SERVER, MAXHOSTNAME);
    cInfo.sock = -1;
    cInfo.usock = -1;
    cInfo.doUDP = false;
    cInfo.tryUDP = true;
    cInfo.remoteport = CN_DFLT_PORT;
    cInfo.state = CLIENT_STATE_INIT;

    utSetLogConfig(false, true);	/* use CQ_USERHOMEDIR for logfile */
    utSetLogProgramName(argv[0]);

    cInfo.remotehost = strdup("localhost"); /* default to your own server */

    // updated per-frame in renderNode()
    cInfo.nodeMillis = clbGetMillis();

    dspInitData();

#if defined(MINGW)
    /* init windows sockets (version 2.0) */
    rv = WSAStartup(MAKEWORD(2, 0), &wsaData);
    if (rv != 0) {
        fprintf(stderr, "main: WSAStartup failed: %d\n", rv);
        utLog("main: WSAStartup failed: %d\n", rv);
        return 1;
    }
#endif

    /* check options */
    while ((i = getopt(argc, argv, "fM:s:P:Bug:Sv")) != EOF)
        switch (i)
        {
        case 'B':                 /* Benchmark mode, set recFrameDelay to 0.0 */
            pbSpeed = PB_SPEED_INFINITE;
            recFrameDelay = 0.0;
            break;
        case 'f':
            DSPFSET(DSP_F_FULLSCREEN);
            break;
        case 'g':
            parseGeometry(optarg);
            break;
        case 'M':
            utStrncpy(cInfo.metaServer, optarg, MAXHOSTNAME);

            break;
        case 's':                 /* [host[:port]] */
            wantMetaList = false;
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

            serveropt = true;

            break;

        case 'P':
            wantMetaList = false;
            recFilename = optarg;
            Context.recmode = RECMODE_PLAYING;
            dosound = false;        /* no sound during playback */
            break;

        case 'u':
            cInfo.tryUDP = false;
            break;

        case 'S':
            dosound = false;
            break;

        case 'v':
            cqDebug++;
            break;

        default:
            printUsage();
            exit(1);
        }

    rndini();		/* initialize random numbers */

    if (!pktInit())
    {
        fprintf(stderr, "pktInit failed, exiting\n");
        utLog("pktInit failed, exiting");
    }
    pktSetSocketFds(cInfo.sock, cInfo.usock);

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

        printf("Scanning file %s...\n", recFilename);

        /* On windows, over a shared folder in VBox, this will be way slow */
        if (!pbInitReplay(recFilename, &recTotalElapsed))
            exit(1);

        /* now init for real */
        if (!pbInitReplay(recFilename, NULL))
            exit(1);

        Context.unum = -1;       /* stow user number */

        /* turn off annoying beeps */
        UserConf.DoAlarms = false;
        /* turn off hudInfo */
        UserConf.hudInfo = false;
    }

    /* load the main texturesrc file and any user supplied ~/.conquest/ *.trc
       files */
    loadTextureRCFiles();

    /* init and load the sounds */
    if (dosound)
    {
        loadSoundRCFiles();
        cqsInitSound();
    }
    else
        cqsSoundAvailable = false;

#ifdef DEBUG_FLOW
    utLog("%s@%d: main() starting conqinit().", __FILE__, __LINE__);
#endif

    conqinit();			/* machine dependent initialization */
    ibufInit();

#ifdef DEBUG_FLOW
    utLog("%s@%d: main() starting cdinit().", __FILE__, __LINE__);
#endif


    uiGLInit(&argc, argv);

    cqsMusicPlay(cqsFindMusic("intro"), false);

    Context.maxlin = 25;
    Context.maxcol = 80;
    Context.snum = -1;
    Context.unum = -1;
    historyCurrentSlot = -1; // useless to a client
    Context.lasttang = 0;
    Context.lasttdist = 0;
    Context.lastInfoTarget.clear();

    catchSignals();       /* enable trapping of interesting signals */

    /* which node to start from... */
    /* a parallel universe, it is */
    // For now, we hardcode cb limits to the minimum.  We will end up
    // unmapping and remapping based on limits received from the
    // server later on anyway.

    if (Context.recmode == RECMODE_PLAYING)
        nPlayBMenuInit();
    else if (wantMetaList)
        nMetaInit();
    else
        nConsvrInit(cInfo.remotehost, cInfo.remoteport);

#ifdef DEBUG_FLOW
    utLog("%s@%d: main() welcoming player.", __FILE__, __LINE__);
#endif

    /* start the fun! */
    glutMainLoop();

    exit(0);

}

void catchSignals(void)
{
#ifdef DEBUG_SIG
    utLog("catchSignals() ENABLED");
#endif
#if !defined(MINGW)
    signal(SIGHUP, (void (*)(int))handleSignal);
    /*  signal(SIGTSTP, SIG_IGN);*/
    signal(SIGTERM, (void (*)(int))handleSignal);
    signal(SIGINT, SIG_IGN);
    signal(SIGQUIT, (void (*)(int))handleSignal);
    signal(SIGPIPE, (void (*)(int))handleSignal);
#endif  /* MINGW */
    return;
}

void handleSignal(int sig)
{

#ifdef DEBUG_SIG
    utLog("handleSignal() got SIG %d", sig);
#endif
#if defined(MINGW)
    utLog("handleSignal() (MINGW) got a signal??? %d", sig);
    exit(1);
#else

    switch(sig)
    {
    case SIGQUIT:
    case SIGINT:
    case SIGTERM:
    case SIGHUP:
    case SIGPIPE:
        utLog("handleSignal: Exiting on signal %d", sig);
        conqend();		/* sends a disconnect packet */
        exit(0);
        break;

    default:
        break;
    }

    catchSignals();	/* reset */
#endif                  /* MINGW */
    return;
}

/*  conqend - machine dependent clean-up */
/*  SYNOPSIS */
/*    conqend */
void conqend(void)
{
    if (!pktNoNetwork())
        sendCommand(CPCMD_DISCONNECT, 0); /* tell the server */

    return;

}
