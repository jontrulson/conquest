/************************************************************************
 *
 * conf.h - header for conf.c
 *
 * $Id$
 *
 ***********************************************************************/

/**********************************************************************/
/* Unix/C specific porting and supporting code Copyright (C)1994-1996 */
/* by Jon Trulson <jon@radscan.com> under the same terms and          */
/* conditions of the original copyright by Jef Poskanzer and Craig    */
/* Leres.                                                             */
/* Have Phun!                                                         */
/**********************************************************************/

#ifndef CONF_H
#define CONF_H


#define SYSCONFIG_FILE "etc/conquestrc"	/* relative to CONQHOME */
#define CONFIG_FILE ".conquestrc"

#define MAXCOMMENTS 50

				/* config types */
#define CTYPE_NULL (0)
#define CTYPE_BOOL (1)
#define CTYPE_STRING (2)
#define CTYPE_MACRO (3)	

				/* Config ID's can be used as offset into */
                                /* ConfData. The ConfData array MUST be */
				/* initialized in the same order as these */
                                /* #define's */


				/* These *must* be 0 and 1 */
#define CF_VERSION        (0)	/* Not really a config option but... */
#define CF_HEADER         (1)
#define CF_START          (CF_HEADER + 1) /* First option (alias) */
#define CF_END (sizeof(ConfData)/sizeof(struct Conf))

				/* These *must* be 0 and 1 */
				/* These are for SysConfData[] */
#define SYSCF_VERSION     (0)	/* Not really a config option but... */
#define SYSCF_HEADER      (1)
#define SYSCF_START       (SYSCF_HEADER + 1) /* First option (alias) */
#define SYSCF_END (sizeof(SysConfData)/sizeof(struct Conf))

				/* Revision on .conquestrc file */
				/* - for updating .conquestrc   */
				/*   when something changes */

				/* to test against .conquestrc */
static char ConfigVersion[] = "$Revision$";

				/* local function declarations */
static char *process_macrostr(char *str);
static char *string_to_macro(char *str);
static int process_bool(char *bufptr);
static int MakeConf(char *filename);


struct Conf 
{
  int Found;
  int ConfType;
  char *ConfName;
  void *ConfValue;
  char *ConfComment[MAXCOMMENTS];
};

				/* Initialize the system configurables */
static struct Conf SysConfData[] =
{
  {			/* The current conquestrc (conquest) version */
    FALSE,
    CTYPE_NULL,
    "SysConqfigVersion=",
    ConfigVersion,
    {
      NULL
    }
  },
  {
    TRUE,			/* always found the header */
    CTYPE_NULL,
    NULL,
    NULL,
    {
      "###################################################################",
      "#",
      "# conquestrc - Conquest system-wide defaults",
      "#",
      "# SYNTAX - <variable>=<value>",
      "#",
      "#  No spaces on either side of the '=' sign, no leading white space.",
      "#",
      "# BOOLEANS - use 'true', 'false', 'yes', or 'no' in upper/lower",
      "#  case.",
      "#",
      "#",
      "",
      "",
      NULL
    }
  },
  {
    FALSE,
    CTYPE_BOOL,
    "do_lr_torpscan=",
    &sysconf_DoLRTorpScan,
    {
      "# define this as 'true' if you want to be able to see friendly",
      "#  torps (including yours) on a long-range scan.  Default: true",
      NULL
    }
  },
  {
    FALSE,
    CTYPE_BOOL,
    "do_local_lrscan=",
    &sysconf_DoLocalLRScan,
    {
      "# define this as 'false' if you want the (M)ap command to have",
      "#  it's original meaning - a static map of the universe with",
      "#  Murisak at the center.  Setting to true makes the (M)ap",
      "#  command have more 'realism' (IMHO) by providing a dynamic",
      "#  map of the universe centered on your ship's current location.",
      "#  Default: true",
      NULL
    }
  },
  {
    FALSE,
    CTYPE_BOOL,
    "do_etastats=",
    &sysconf_DoETAStats,
    {
      "# define this as 'true' if you want Estimated Time of Arrival (ETA)",
      "#  information to be computed and displayed when getting (I)nfo on",
      "#  another object.  Default: true",
      NULL
    }
  },
  {
    FALSE,
    CTYPE_BOOL,
    "no_doomsday=",
    &sysconf_NoDoomsday,
    {
      "# define this as 'true' if you never want the Doomsday Machine to",
      "#  randomly startup (except manually via conqoper).  Default: false",
      NULL
    }
  },
  {
    FALSE,
    CTYPE_BOOL,
    "allow_fastupdate=",
    &sysconf_AllowFastUpdate,
    {
      "# define this as 'true' if you want to allow users to enable the",
      "#  'do_fastupdate' option, which gives users 2 screen updates per",
      "#  second instead of one.  This can provide a smoother game, but",
      "#  requires more CPU and better bandwith to be useful. This option",
      "#  has no effect (nor does do_fastupdate) if Conquest was compiled",
      "#  with HAS_SETITIMER undefined in defs.h.",
      "#  Default: true",
      NULL
    }
  },
  {
    FALSE,
    CTYPE_BOOL,
    "do_random_robotkills=",
    &sysconf_DoRandomRobotKills,
    {
      "# define this as 'true' if you want robots to be created with a",
      "#  randomized kill point value.  A ship's kill points influences how ",
      "#  'efficient' that ship is.  This means that when robots are created,",
      "#  with a randomized kill point value, they can be much more deadly ",
      "#  than if the default kill point value of 0.0 is used.",
      "#  It also makes them a little less predictable.  NOTE: When this",
      "#  is enabled, the extra kills given to a robot are not used in ",
      "#  calculating Team or User statistics.  It just makes them a little",
      "#  tougher... probably not a good idea for new players though ;-)",
      "#  Default: false",
      NULL
    }
  },
  {
    FALSE,
    CTYPE_BOOL,
    "allow_sigquit=",
    &sysconf_AllowSigquit,
    {
      "# define this as 'true' if you want to allow non Conquest Gods to",
      "#  be able to use SIGQUIT (normally ^\\) to leave conquest.",
      "#  Default: false",
      NULL
    }
  }
};
				/* Initialize the user configurables */
static struct Conf ConfData[] =
{
  {				/* The current .conquestrc (conf.h) version */
    FALSE,
    CTYPE_NULL,
    "ConqfigVersion=",
    ConfigVersion,
    {
      NULL
    }
  },
  {
    TRUE,			/* always found the header */
    CTYPE_NULL,
    NULL,
    NULL,
    {
      "###################################################################",
      "#",
      "# .conquestrc - define some configuration values",
      "#",
      "# SYNTAX - <variable>=<value>",
      "#",
      "#  No spaces on either side of the '=' sign, no leading white space.",
      "#",
      "# BOOLEANS - use 'true', 'false', 'yes', or 'no' in upper/lower",
      "#  case.",
      "#",
      "# MACRO KEYS - program an Fkey to send a series of conquest commands",
      "#  ALL characters after the '=' up to the end of line are programmed",
      "#  into the respective FKey.  The following special sequences are ",
      "#  recognized:  \\r = [RETURN], \\t = [TAB]",
      "#",
      "#  An example - make f1 a 'red alert' key:",
      "#  macro_f1=A60\\r+",
      "#  Where <A60\\r> = set weapons to 60% power allocation, and",
      "#        <+>      = raise shields",
      "#  The conquest commands defined in an FKey, will be fed to",
      "#  conquest just as if they were typed from the keyboard.",
      "#",
      "",
      "",
      NULL
    }
  },
  {
    FALSE,
    CTYPE_BOOL,
    "do_msg_alarm=",
    &conf_MessageBell,
    {
      "# define this as 'true' if you want beeps when a message is",
      "#  delivered to your ship.   Default: true",
      NULL
    }
  },
  {
    FALSE,
    CTYPE_BOOL,
    "no_color=",
    &conf_NoColor,
    {
      "# define this as 'true' if you never want to see color,",
      "#  even if your terminal supports it.  Default: false",
      NULL
    }
  },
  {
    FALSE,
    CTYPE_BOOL,
    "no_robot_msgs=",
    &conf_NoRobotMsgs,
    {
      "# define this as 'true' if you don't want to recieve messages",
      "#  from friendly robots.  This can be handy when sending messages",
      "#  to All, and you don't want alot of replies from robots you",
      "#  don't care about.",
      "#  Default: false",
      NULL
    }
  },
  {
    FALSE,
    CTYPE_BOOL,
    "rcv_player_msgs=",
    &conf_RecPlayerMsgs,
    {
      "# define this as 'true' if you want to recieve a message",
      "#  whenever someone enters the game, or dies in it.",
      "#  Default: true",
      NULL
    }
  },
  {
    FALSE,
    CTYPE_BOOL,
    "do_fastupdate=",
    &conf_DoFastUpdate,
    {
      "# define this as 'true' if you want 2 screen updates per second",
      "#  instead of the usual 1 per second.  This option has NO EFFECT",
      "#  if 'allow_fastupdate' is set to 'false' in the system-wide",
      "#  conquestrc file, (by default /opt/conquest/etc/conquestrc) OR",
      "#  HAS_SETITIMER was not defined (in defs.h) when Conquest was",
      "#  compiled.",
      "#  Default: true",
      NULL
    }
  },
  {
    FALSE,
    CTYPE_BOOL,
    "do_limitbell=",
    &conf_DoLimitBell,
    {
      "# define this as 'true' if you want no more than one 'beep'",
      "#  maximum per second.  Additional beeps that occur in the same",
      "#  second as the last beep are ignored.  This means you might",
      "#  miss beeps, but is real nice when do_fastupdate is enabled.",
      "#  Default: true",
      NULL
    }
  },
  {
    FALSE,
    CTYPE_MACRO,
    "macro_f1=",
    conf_MacrosF[0],
    {
      "# F1 Key",
      NULL
    }
  },
  {
    FALSE,
    CTYPE_MACRO,
    "macro_f2=",
    conf_MacrosF[1],
    {
      "# F2 Key",
      NULL
    }
  },
  {
    FALSE,
    CTYPE_MACRO,
    "macro_f3=",
    conf_MacrosF[2],
    {
      "# F3 Key",
      NULL
    }
  },
  {
    FALSE,
    CTYPE_MACRO,
    "macro_f4=",
    conf_MacrosF[3],
    {
      "# F4 Key",
      NULL
    }
  },
  {
    FALSE,
    CTYPE_MACRO,
    "macro_f5=",
    conf_MacrosF[4],
    {
      "# F5 Key",
      NULL
    }
  },
  {
    FALSE,
    CTYPE_MACRO,
    "macro_f6=",
    conf_MacrosF[5],
    {
      "# F6 Key",
      NULL
    }
  },
  {
    FALSE,
    CTYPE_MACRO,
    "macro_f7=",
    conf_MacrosF[6],
    {
      "# F7 Key",
      NULL
    }
  },
  {
    FALSE,
    CTYPE_MACRO,
    "macro_f8=",
    conf_MacrosF[7],
    {
      "# F8 Key",
      NULL
    }
  },
  {
    FALSE,
    CTYPE_MACRO,
    "macro_f9=",
    conf_MacrosF[8],
    {
      "# F9 Key",
      NULL
    }
  },
  {
    FALSE,
    CTYPE_MACRO,
    "macro_f10=",
    conf_MacrosF[9],
    {
      "# F10 Key",
      NULL
    }
  },
  {
    FALSE,
    CTYPE_MACRO,
    "macro_f11=",
    conf_MacrosF[10],
    {
      "# F11 Key",
      NULL
    }
  },
  {
    FALSE,
    CTYPE_MACRO,
    "macro_f12=",
    conf_MacrosF[11],
    {
      "# F12 Key",
      NULL
    }
  },
  {
    FALSE,
    CTYPE_MACRO,
    "macro_f13=",
    conf_MacrosF[12],
    {
      "# F13 Key (SHIFT F1)",
      NULL
    }
  },
  {
    FALSE,
    CTYPE_MACRO,
    "macro_f14=",
    conf_MacrosF[13],
    {
      "# F14 Key (SHIFT F2)",
      NULL
    }
  },
  {
    FALSE,
    CTYPE_MACRO,
    "macro_f15=",
    conf_MacrosF[14],
    {
      "# F15 Key (SHIFT F3)",
      NULL
    }
  },
  {
    FALSE,
    CTYPE_MACRO,
    "macro_f16=",
    conf_MacrosF[15],
    {
      "# F16 Key (SHIFT F4)",
      NULL
    }
  },
  {
    FALSE,
    CTYPE_MACRO,
    "macro_f17=",
    conf_MacrosF[16],
    {
      "# F17 Key (SHIFT F5)",
      NULL
    }
  },
  {
    FALSE,
    CTYPE_MACRO,
    "macro_f18=",
    conf_MacrosF[17],
    {
      "# F18 Key (SHIFT F6)",
      NULL
    }
  },
  {
    FALSE,
    CTYPE_MACRO,
    "macro_f19=",
    conf_MacrosF[18],
    {
      "# F19 Key (SHIFT F7)",
      NULL
    }
  },
  {
    FALSE,
    CTYPE_MACRO,
    "macro_f20=",
    conf_MacrosF[19],
    {
      "# F20 Key (SHIFT F8)",
      NULL
    }
  },
  {
    FALSE,
    CTYPE_MACRO,
    "macro_f21=",
    conf_MacrosF[20],
    {
      "# F21 Key (SHIFT F9)",
      NULL
    }
  },
  {
    FALSE,
    CTYPE_MACRO,
    "macro_f22=",
    conf_MacrosF[21],
    {
      "# F22 Key (SHIFT F10)",
      NULL
    }
  },
  {
    FALSE,
    CTYPE_MACRO,
    "macro_f23=",
    conf_MacrosF[22],
    {
      "# F23 Key (SHIFT F11)",
      NULL
    }
  },
  {
    FALSE,
    CTYPE_MACRO,
    "macro_f24=",
    conf_MacrosF[23],
    {
      "# F24 Key (SHIFT F12)",
      NULL
    }
  }
};

#endif /* CONF_H */
