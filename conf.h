/************************************************************************
 *
 * conf.h - header for conf.c
 *
 * $Id$
 *
 * Copyright 1999 Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 ***********************************************************************/

/**********************************************************************/
/* Unix/C specific porting and supporting code Copyright (C)1994-1996 */
/* by Jon Trulson <jon@radscan.com> under the same terms and          */
/* conditions of the original copyright by Jef Poskanzer and Craig    */
/* Leres.                                                             */
/*                                                                    */
/**********************************************************************/

#ifndef CONF_H
#define CONF_H

#ifdef NOEXTERN
# define CEXTERN
#else
# define CEXTERN extern
#endif

#define SYSCONFIG_FILE "conquestrc"	/* relative to CONQETC */
#define CONFIG_FILE ".conquestrc"

#define CONF_MAXCOMMENTS 50

				/* config types */
#define CTYPE_NULL (0)
#define CTYPE_BOOL (1)
#define CTYPE_STRING (2)
#define CTYPE_MACRO (3)	
#define CTYPE_NUMERIC (4)

struct Conf 
{
  int Found;
  int ConfType;
  char *ConfName;
  void *ConfValue;
  char *OneLineDesc;
  char *ConfComment[CONF_MAXCOMMENTS];
};

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

typedef struct _userConf {
				/* Whether to beep on an incoming message */
  int MessageBell;
				/* Whether or not to disable color */
  int NoColor;
				/* whether or not you want to see robot msgs */
  int NoRobotMsgs;
				/* whether or not to send announcements on */
				/* player entries/deaths to you. */
  int RecPlayerMsgs;
				/* whether to update the screen twice per */
				/* second instead of once per second. */
  int DoFastUpdate;
				/* whether to restrict bells to no more than */
				/* one per second.  Handy when do_fastupdate */
				/* is set. */
  int DoLimitBell;
				/* whether or not to clear out old msgs after
				   a reincarnate */
  int ClearOldMsgs;
				/* whether to send emergency distress calls
				   to all friendly ships, rather than just
				   your team mates */
  int DistressToFriendly;
				/* whether to use alt HUD.  no effect if
				   allow_alt_hud is false */
  int AltHUD;
				/* Macro keys - F1-F<MAX_MACROS> */
  char MacrosF[MAX_MACROS][MAX_MACRO_LEN];

} UserConf_t;

typedef struct _sysConf {
				/* can see friendly torps on LR scan? */
  int DoLRTorpScan;
				/* (M)ap is actually LongeRange scan? */
  int DoLocalLRScan;
				/* (I)nfo cmds return ETA data? */
  int DoETAStats;
				/* randomly gen a doomsday machine? */
  int NoDoomsday;
				/* allow users to use do_fastupdate */
  int AllowFastUpdate;
				/* allow randomized robot strengths */
  int DoRandomRobotKills;
				/* allow non gods to SIGQUIT out */
  int AllowSigquit;
				/* allow users to (s)witchteams from main
				   menu */
  int AllowSwitchteams;
				/* number of days of inactivity before
				   expiring a user */
  int UserExpiredays;

				/* whether to log all messages into
				   the logfile */
  int LogMessages;

				/* whether to allow refitting */
  int AllowRefits;
				/* whether to allow alternate HUD */
  int AllowAltHUD;
} SysConf_t;


				/* Revision on .conquestrc file */
				/* - for updating .conquestrc   */
				/*   when something changes */
#ifndef NOEXTERN
CEXTERN char ConfigVersion[];

#else
char ConfigVersion[] = "$Revision$";
#endif /* NOEXTERN */

/* Config's */
CEXTERN UserConf_t UserConf;
CEXTERN SysConf_t SysConf;

				/* local function declarations */
CEXTERN char *process_macrostr(char *str);
CEXTERN int process_bool(char *bufptr);
CEXTERN int MakeConf(char *filename);


				/* Initialize the system configurables */
#ifndef NOEXTERN
CEXTERN struct Conf SysConfData[];
#else
struct Conf SysConfData[] =
{
  {			/* The current conquestrc (conquest) version */
    FALSE,
    CTYPE_NULL,
    "SysConqfigVersion=",
    ConfigVersion,
    "System Config Version",
    {
      NULL
    }
  },
  {
    TRUE,			/* always found the header */
    CTYPE_NULL,
    NULL,
    NULL,
    "System Config File Header",
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
      NULL
    }
  },
  {
    FALSE,
    CTYPE_BOOL,
    "do_lr_torpscan=",
    &SysConf.DoLRTorpScan,
    "Show friendly torps on a long-range scan",
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
    &SysConf.DoLocalLRScan,
    "Long Range Scan is centered on ship, not Murisak",
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
    &SysConf.DoETAStats,
    "Compute and display Estimated Time of Arrival (ETA) stats",
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
    &SysConf.NoDoomsday,
    "Disable the Doomsday machine",
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
    &SysConf.AllowFastUpdate,
    "Allow 2 screen updates per second rather than 1",
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
    &SysConf.DoRandomRobotKills,
    "Robots have randomized kills when created",
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
    &SysConf.AllowSigquit,
    "Allow users to exit Conquest immediately with a QUIT signal",
    {
      "# define this as 'true' if you want to allow non Conquest Gods to",
      "#  be able to use SIGQUIT (normally ^\\) to leave conquest.",
      "#  Default: false",
      NULL
    }
  },
  {
    FALSE,
    CTYPE_BOOL,
    "allow_switchteams=",
    &SysConf.AllowSwitchteams,
    "Allow users to (s)witch teams at the main menu",
    {
      "# define this as 'false' if you want to prevent users from being",
      "#  able to (s)witch teams from the main menu.",
      "#  Default: true",
      NULL
    }
  },
  {
    FALSE,
    CTYPE_NUMERIC,
    "user_expiredays=",
    &SysConf.UserExpiredays,
    "Number of days after which to expire inactive users",
    {
      "# number of days of inactivity, after which a user is expired.",
      "#  Set this equal to 0 to disable expiration of inactive users.",
      "#  Default: 120   ",
      NULL
    }
  },
  {
    FALSE,
    CTYPE_BOOL,
    "log_messages=",
    &SysConf.LogMessages,
    "Log all player messages into the log file",
    {
      "# Whether or not to log all player messages into the logfile.",
      "#  Default: false   ",
      NULL
    }
  },
  {
    FALSE,
    CTYPE_BOOL,
    "allow_alt_hud=",
    &SysConf.AllowAltHUD,
    "Allow users to use an alternate (experimental) HUD",
    {
      "# Whether or not to allow users to enable an alternate and ",
      "# experimental HUD that provides some more info along",
      "# the bottom view border.",
      "#  Default: false   ",
      NULL
    }
  },
  {
    FALSE,
    CTYPE_BOOL,
    "allow_refits=",
    &SysConf.AllowRefits,
    "Allow players to refit their ships",
    {
      "# Whether or not to allow players to refit their ships to a ",
      "#  different ship type - if they have a kill, and are orbiting",
      "#  a team owned planet.",
      "#  Default: true   ",
      NULL
    }
  }
};
#endif /* NOEXTERN */
				/* Initialize the user configurables */
#ifndef NOEXTERN
CEXTERN struct Conf ConfData[];
#else
struct Conf ConfData[] =
{
  {				/* The current .conquestrc (conf.h) version */
    FALSE,
    CTYPE_NULL,
    "ConqfigVersion=",
    ConfigVersion,
    "User Config Version",
    {
      NULL
    }
  },
  {
    TRUE,			/* always found the header */
    CTYPE_NULL,
    NULL,
    NULL,
    "User Config File Header",
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
      "#        <+>     = raise shields",
      "#  The conquest commands defined in an FKey, will be fed to",
      "#  conquest just as if they were typed from the keyboard.",
      "#",
      "",
      NULL
    }
  },
  {
    FALSE,
    CTYPE_BOOL,
    "do_msg_alarm=",
    &UserConf.MessageBell,
    "Beep when a message arrives",
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
    &UserConf.NoColor,
    "Disable color, even if your terminal/terminfo entry supports it",
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
    &UserConf.NoRobotMsgs,
    "Disable messages from robots to your ship",
    {
      "# define this as 'true' if you don't want to recieve messages",
      "#  from any robots.  This can be handy when sending messages",
      "#  to All, and you don't want alot of replies from robots. NOTE:",
      "#  Obviously if you set this to true, don't expect replies from",
      "#  the robots - some of which can be useful.",
      "#  Default: false",
      NULL
    }
  },
  {
    FALSE,
    CTYPE_BOOL,
    "rcv_player_msgs=",
    &UserConf.RecPlayerMsgs,
    "Recieve messages indicating when a player enters or dies",
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
    &UserConf.DoFastUpdate,
    "Update the display twice per second (if system allows it)",
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
    &UserConf.DoLimitBell,
    "Limit beeping to no more than 1 per second",
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
    CTYPE_BOOL,
    "clear_old_msgs=",
    &UserConf.ClearOldMsgs,
    "When re-incarnating to a ship, discard any old messages",
    {
      "# define this as true if you want old msgs cleared out when you",
      "#  reincarnate to another ship.  If this is defined as false, then",
      "#  any messages sent to you while you were gone from your ship",
      "#  are displayed as usual when you enter the game.",
      "#  Default: true",
      NULL,
    }
  },
  {
    FALSE,
    CTYPE_BOOL,
    "distress_to_friendly=",
    &UserConf.DistressToFriendly,
    "Send emergency distress calls to friendly ships",
    {
      "# define this as true if you want emergency distress calls to be",
      "#  sent to all friendly ships rather than just to your teammates.",
      "#  Default: false",
      NULL,
    }
  },
  {
    FALSE,
    CTYPE_BOOL,
    "alt_hud=",
    &UserConf.AltHUD,
    "Use an alternate, experimental HUD",
    {
      "# define this as true if you want to use a new, experimental",
      "#  HUD.  This option has no effect if allow_alt_hud is not ",
      "#  enabled in the sys-wide config file.  This option allows the",
      "#  display of certain information on the bottom view line."
      "#  Default: false",
      NULL,
    }
  },
  {				/* Macros are special - these should be last */
    FALSE,
    CTYPE_MACRO,
    "macro_f",
    UserConf.MacrosF,
    "Macro Keys",
    {
      "# Macro definitions.  The format in the ~/.conquestrc file is",
      "#  'macro_fN=<string>'",
      "#  'N' is a number between 1 and MAX_MACROS, inclusive.",
      "#  <string> is a sequence of Conquest commands to execute when",
      "#  that particular Function Key is hit.",
      "#",
      "# The following special sequences are recognized:",
      "#  \\r = [RETURN], \\t = [TAB]",
      "#",
      "# Here are some example entries as they might appear in the",
      "#  ~/.conquestrc file:",
      "#",
      "#  # fires full spread of torps at last dir when F1 key is pressed:",
      "#  macro_f1=P\\rP\\rP\\r",
      "#  # raises shields and detonates enemy torps when F15 is pressed:",
      "#  macro_f15=+d",
      "#",
      "#  Obviously if you are changing your macros via the ",
      "#  (O)ptions Menu->View/Edit Macros screen from within Conquest, ",
      "#  you won't need to type the 'macro_fN=' part ;-)",
      "",
      NULL
    }
  }
};
#endif /* NOEXTERN */

#ifndef NOEXTERN
CEXTERN int CfEnd;
#else
int CfEnd = CF_END;
#endif /* NOEXTERN */

#ifndef NOEXTERN
CEXTERN int SysCfEnd;
#else
int SysCfEnd = SYSCF_END;
#endif /* NOEXTERN */

#undef CEXTERN			/* cleanup */

#endif /* CONF_H */
