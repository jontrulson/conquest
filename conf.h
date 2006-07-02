/************************************************************************
 *
 * conf.h - header for conf.c
 *
 * $Id$
 *
 * Copyright 1999-2004 Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
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

#include "datatypes.h"
#include "cqkeys.h"
#ifdef CONF_NOEXTERN
# define CEXTERN
#else
# define CEXTERN extern
#endif

#define SYSCONFIG_FILE "conquestrc"	/* relative to CONQETC */
#define CONFIG_FILE "conquestrc"

#define CONF_MAXCOMMENTS   50

#define CONF_MAXBUTTONS    32     /* max # of mouse buttons we can handle
                                   (mouse macros) */
#define CONF_MAXMODIFIERS  (((CQ_KEY_MOD_SHIFT | CQ_KEY_MOD_ALT | \
                              CQ_KEY_MOD_CTRL) >> CQ_MODIFIER_SHIFT) + 1)


				/* config types */
#define CTYPE_NULL (0)
#define CTYPE_BOOL (1)
#define CTYPE_STRING (2)
#define CTYPE_MACRO (3)	
#define CTYPE_NUMERIC (4)
#define CTYPE_MOUSE (5)

/* limits for some options */
#define CONF_SERVER_NAME_SZ   70
#define CONF_SERVER_MOTD_SZ   70

#include "meta.h"

struct Conf 
{
  int Found;
  int ConfType;
  char *ConfName;
  void *ConfValue;
  int min, max;			/* for CTYPE_NUMERIC, CTYPE_STRING */
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
				/* whether to update the screen twice per */
				/* second instead of once per second. */
  int UpdatesPerSecond;
				/* whether to send emergency distress calls
				   to all friendly ships, rather than just
				   your team mates */
  int DistressToFriendly;
				/* whether to use alt HUD.  no effect if
				   allow_alt_hud is false */
  int AltHUD;
				/* Macro keys - F1-F<MAX_MACROS> */

  int ShowPhasers;		/* want phaser graphics? */

  int ShowPlanNames;		/* show planet names? */

  int DoAlarms;			/* want alarm bell? */

  int DoIntrudeAlert;		/* want intruder alerts */

  int DoNumMap;			/* want numeric map */

  int Terse;			/* be terse? */
 
  int DoExplode;		/* show explosions? */

				/* can see friendly torps on LR scan? */
  int DoLRTorpScan;
                              /* (M)ap is actually LongeRange scan? */
  int DoLocalLRScan;
                                /* (I)nfo cmds return ETA data? */
  int DoETAStats;
     /* for the GL client only - draws a red box around enemy ships */
  int EnemyShipBox;

  int doVBG;                    /* draw the background in the viewer? */

  int DoShields;                /* Cataboligne - enhanced colored shield
                                   indicator */

  int DoTacBkg;                 /* tactical background graphic */

  int DoTacShade;               /* tactical background alpha */

  char MacrosF[MAX_MACROS][MAX_MACRO_LEN];

  /* mouse macros, indexed by [max buttons][modifiers (8)]  */
  char Mouse[CONF_MAXBUTTONS][CONF_MAXMODIFIERS][MAX_MACRO_LEN];

} UserConf_t;

typedef struct _sysConf {
				/* randomly gen a doomsday machine? */
  int NoDoomsday;
				/* allow randomized robot strengths */
  int DoRandomRobotKills;
				/* allow vacant ships */
  int AllowVacant;
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

  int AllowSlingShot;           /* slingshot bug enabled? */

  /* server name */
  char ServerName[CONF_SERVER_NAME_SZ];
  /* server motd */
  char ServerMotd[CONF_SERVER_MOTD_SZ];
  /* server owner contact info */
  char ServerContact[META_GEN_STRSIZE];

} SysConf_t;


				/* Revision on conquestrc file */
				/* - for updating conquestrc   */
				/*   when something changes */
#ifndef CONF_NOEXTERN
CEXTERN char ConfigVersion[];

#else
char ConfigVersion[] = "$Revision$";
#endif /* CONF_NOEXTERN */

/* Config's */
CEXTERN UserConf_t UserConf;
CEXTERN SysConf_t SysConf;

				/* local function declarations */
void confSetTelnetClientMode(int telnetc);
CEXTERN int GetSysConf(int checkonly);
CEXTERN int GetConf(int usernum);
CEXTERN int MakeSysConf(void);
CEXTERN int SaveUserConfig(void);
CEXTERN int SaveSysConfig(void);
CEXTERN char *Str2Macro(char *str);
CEXTERN char *Macro2Str(char *str);
CEXTERN char *process_macrostr(char *str);
CEXTERN int process_bool(char *bufptr);
CEXTERN int MakeConf(char *filename);
CEXTERN Unsgn32 getServerFlags(void);

				/* Initialize the system configurables */
#ifndef CONF_NOEXTERN
CEXTERN struct Conf SysConfData[];
#else
struct Conf SysConfData[] =
{
  {			/* The current conquestrc (conquest) version */
    FALSE,
    CTYPE_NULL,
    "SysConqfigVersion=",
    ConfigVersion,
    0, 0,			/* mix/max */
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
    0, 0,			/* mix/max */
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
    "no_doomsday=",
    &SysConf.NoDoomsday,
    0, 0,			/* mix/max */
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
    "do_random_robotkills=",
    &SysConf.DoRandomRobotKills,
    0, 0,			/* mix/max */
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
    "allow_vacant=",
    &SysConf.AllowVacant,
    0, 0,			/* mix/max */
    "Allow users to exit Conquest leaving their ship vacant",
    {
      "# define this as 'true' if you want to allow users to",
      "#  be able to leave their ship vacant if they use SIGQUIT",
      "#  (normally ^\\) to leave conquest without self-destructing first.",
      "#  Otherwise their ship will be destroyed by a lightning bolt.",
      "#  Default: false",
      NULL
    }
  },
  {
    FALSE,
    CTYPE_BOOL,
    "allow_switchteams=",
    &SysConf.AllowSwitchteams,
    0, 0,			/* mix/max */
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
    0, 4000,			/* mix/max */
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
    0, 0,			/* mix/max */
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
    "allow_refits=",
    &SysConf.AllowRefits,
    0, 0,			/* mix/max */
    "Allow players to refit their ships",
    {
      "# Whether or not to allow players to refit their ships to a ",
      "#  different ship type - if they have a kill, and are orbiting",
      "#  a team owned planet.",
      "#  Default: true   ",
      NULL
    }
  },
  {
    FALSE,
    CTYPE_BOOL,
    "allow_slingshot=",
    &SysConf.AllowSlingShot,
    0, 0,			/* mix/max */
    "Enable the Slingshot bug.",
    {
      "# Whether or not to enable the slingshot bug.",
      "#  This is a towing bug in the original conquest. Good fun!",
      "#  Default: false",
      NULL
    }
  },
  {
    FALSE,
    CTYPE_STRING,
    "server_name=",
    SysConf.ServerName,
    0, CONF_SERVER_NAME_SZ,	/* mix/max */
    "Server's Name",
    {
      "# The name of your server, displayed on the client login",
      "#  screen",
      "#  Default: Generic Conquest Server",
      NULL
    }
  },
  {
    FALSE,
    CTYPE_STRING,
    "server_motd=",
    SysConf.ServerMotd,
    0, CONF_SERVER_MOTD_SZ,	/* mix/max */
    "Server MOTD",
    {
      "# Message of the day (MOTD) for your server",
      "#  Default: Keep your shields up in battle.",
      NULL
    }
  },
  {
    FALSE,
    CTYPE_STRING,
    "server_contact=",
    SysConf.ServerContact,
    0, META_GEN_STRSIZE,	/* mix/max */
    "Server Contact Info",
    {
      "# Email address or URL to use to contact the server owner.",
      "#  Default: root@localhost",
      NULL
    }
  }
};
#endif /* CONF_NOEXTERN */
				/* Initialize the user configurables */
#ifndef CONF_NOEXTERN
CEXTERN struct Conf ConfData[];
#else
struct Conf ConfData[] =
{
  {				/* The current conquestrc (conf.h) version */
    FALSE,
    CTYPE_NULL,
    "ConqfigVersion=",
    ConfigVersion,
    0, 0,			/* mix/max */
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
    0, 0,			/* mix/max */
    "User Config File Header",
    {
      "###################################################################",
      "#",
      "# ~/.conquest/conquestrc - define some configuration values",
      "#",
      "# SYNTAX - <variable>=<value>",
      "#",
      "#  No spaces on either side of the '=' sign, no leading white space.",
      "#",
      "# BOOLEANS - use 'true', 'false', 'yes', or 'no' in upper/lower",
      "#  case.",
      "#",
      "",
      NULL
    }
  },
  {
    FALSE,
    CTYPE_BOOL,
    "do_alarm_bell=",
    &UserConf.DoAlarms,
    0, 0,			/* mix/max */
    "Beep when something important is happening",
    {
      "# define this as 'true' if you want beeps when under attack, ",
      "# taking damage, etc.",
      "# Default: true",
      NULL
    }
  },
  {
    FALSE,
    CTYPE_BOOL,
    "show_phaser_graphics=",
    &UserConf.ShowPhasers,
    0, 0,			/* mix/max */
    "Display phaser graphics",
    {
      "# define this as 'true' if you want to see phasers",
      "# Default: true",
      NULL
    }
  },
  {
    FALSE,
    CTYPE_BOOL,
    "show_planet_names=",
    &UserConf.ShowPlanNames,
    0, 0,			/* mix/max */
    "Show Planet names on the scanners",
    {
      "# define this as 'true' if you want planets labeled with their",
      "# names on your scanners.",
      "# Default: true",
      NULL
    }
  },
  {
    FALSE,
    CTYPE_BOOL,
    "do_intruder_alerts=",
    &UserConf.DoIntrudeAlert,
    0, 0,			/* mix/max */
    "Receive Intruder alerts",
    {
      "# define this as 'true' if you want to receive a message when",
      "# someone attacks one of your team's planets.",
      "# Default: true",
      NULL
    }
  },
  {
    FALSE,
    CTYPE_BOOL,
    "numeric_planet_map=",
    &UserConf.DoNumMap,
    0, 0,			/* mix/max */
    "Show army counts for planets on LR scan",
    {
      "# define this as 'true' if you want to see the number of armies",
      "# on each (scanned) planet in the long range map.",
      "# Default: true",
      NULL
    }
  },
  {
    FALSE,
    CTYPE_BOOL,
    "terse=",
    &UserConf.Terse,
    0, 0,			/* mix/max */
    "Be Terse",
    {
      "# define this as 'true' if you don't want to receive certain",
      "# mundane messages like 'Entering orbit', etc...",
      "# Default: false",
      NULL
    }
  },
  {
    FALSE,
    CTYPE_BOOL,
    "show_explosions=",
    &UserConf.DoExplode,
    0, 0,			/* mix/max */
    "Show torpedo and ship explosions",
    {
      "# define this as 'true' if you want to see exploding torps and",
      "# ships.",
      "# Default: true",
      NULL
    }
  },
  {
    FALSE,
    CTYPE_BOOL,
    "do_msg_alarm=",
    &UserConf.MessageBell,
    0, 0,			/* mix/max */
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
    0, 0,			/* mix/max */
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
    0, 0,			/* mix/max */
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
    CTYPE_NUMERIC,
    "updates_per_sec=",
    &UserConf.UpdatesPerSecond,
    1, 10,			/* mix/max */
    "Updates per second (1-10)",
    {
      "# define this as the number of updates per second in which to",
      "#  recieve packets and update the display. Number must be between",
      "#  1 and 10 inclusive.",
      "#  Default: 5",
      NULL
    }
  },
  {
    FALSE,
    CTYPE_BOOL,
    "distress_to_friendly=",
    &UserConf.DistressToFriendly,
    0, 0,			/* mix/max */
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
    0, 0,			/* mix/max */
    "Use an alternate, experimental HUD",
    {
      "# define this as true if you want to use a new, experimental",
      "#  HUD.  This option allows the display of certain information",
      "#  on the bottom view line.  There may be more in the future.",
      "#  Default: false",
      NULL,
    }
  },
  {
    FALSE,
    CTYPE_BOOL,
    "do_lr_torpscan=",
    &UserConf.DoLRTorpScan,
    0, 0,			/* mix/max */
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
    &UserConf.DoLocalLRScan,
    0, 0,			/* mix/max */
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
    &UserConf.DoETAStats,
    0, 0,			/* mix/max */
    "Compute and display Estimated Time of Arrival (ETA) stats",
    {
      "# define this as 'true' if you want Estimated Time of Arrival (ETA)",
      "#  information to be computed and displayed when getting (I)nfo on",
      "#  another object.  Default: true",
      NULL
    }
  },
  {
    TRUE,
    CTYPE_BOOL,
    "do_enemyshipbox=",
    &UserConf.EnemyShipBox,
    0, 0,			/* mix/max */
    "Draw a red box around enemy ships (GL client only)",
    {
      "# define this as 'true' if you want a red box drawn around",
      "#  enemy ships on the viewer.  Valid for GL client only.",
      "#  Default: true",
      NULL
    }
  },
  {
    TRUE,
    CTYPE_BOOL,
    "do_draw_viewerbg=",
    &UserConf.doVBG,
    0, 0,			/* mix/max */
    "Draw the background image in the viewer(GL client only)",
    {
      "# define this as 'true' if you want to see the rather attractive",
      "#  background image displayed in the viewer.  ",
      "#  Valid for GL client only.",
      "#  Default: true",
      NULL
    }
  },
  {
    FALSE,
    CTYPE_BOOL,
    "do_shields=",
    &UserConf.DoShields,
    0, 0,
    "Display shield graphics",
    {
      "# define this as 'true' if you want a color coded shield strength",
      "#  graphic displayed around the ships.  Default: true",
      NULL
    }
  },
  {
    FALSE,
    CTYPE_BOOL,
    "do_tac_bkg=",
    &UserConf.DoTacBkg,
    0, 0,
    "Display tactical display background image",
    {
      "# define this as 'true' if you want a tactical LR gauge",
      "#  Default: false",
      NULL
    }
  },
  {
    FALSE,
    CTYPE_NUMERIC,
    "do_tac_shade=",
    &UserConf.DoTacShade,
    1, 100,                      /* min/max */
    "Tactical display background shade (1-100)",
    {
      "# If tactical background is true, this will be the shade alpha",
      "#  valid values 1 - 100",
      "#  Default: 50",
      NULL
    }
  },
  {				/* mouse macros */
    FALSE,
    CTYPE_MOUSE,
    "mouse_",
    &UserConf.Mouse,
    0, 0,			/* mix/max */
    "Mouse macros",
    {
      "# Mouse macro definitions.  Same general format as Macros.",
      "# These only work when in the game and you click within the viewer.",
      "# GL client only.",
      "#",
      "#  'mouse_[s|c|a]N=<string>'",
      "#  s, c, and a are optional modifiers and represent:",
      "#  s = Shift key, a = Alt key, c = Control key",
      "#",
      "#  'N' is the button number between 0 and 31, inclusive.",
      "#",
      "#  <string> is a sequence of Conquest commands to execute when",
      "#  that particular Button and Modifier combination is pressed.",
      "#",
      "# The following special sequences are recognized:",
      "#  \\r = [RETURN], \\t = [TAB] \\a = [Clicked Angle]",
      "#",
      "# Here are some example entries as they might appear in the",
      "#  ~/.conquest/conquestrc file:",
      "#",
      "#  # mouse button 0, fire phaser at angle:",
      "#  mouse_0=f\\a\\r",
      "#  # sets course when middle button pressed:",
      "#  mouse_1=k\\a\\r",
      "#  # fire spread at angle on Control-Button 0:",
      "#  mouse_c0=P\\a\\r",
      "#  # Start self destuct on Control-Shift button 2:",
      "#  mouse_cs2=Q\\t",
      "#",
      "",
      NULL
    }
  },
  {				/* Macros are special - these should be last */
    FALSE,
    CTYPE_MACRO,
    "macro_f",
    UserConf.MacrosF,
    0, 0,			/* mix/max */
    "Macro Keys",
    {
      "# Macro definitions.  The format in the ~/.conquest/conquestrc file is",
      "#  'macro_fN=<string>'",
      "#  'N' is a number between 1 and MAX_MACROS, inclusive.",
      "#  <string> is a sequence of Conquest commands to execute when",
      "#  that particular Function Key is hit.",
      "#",
      "# The following special sequences are recognized:",
      "#  \\r = [RETURN], \\t = [TAB]",
      "#",
      "# Here are some example entries as they might appear in the",
      "#  ~/.conquest/conquestrc file:",
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
#endif /* CONF_NOEXTERN */

#ifndef CONF_NOEXTERN
CEXTERN int CfEnd;
#else
int CfEnd = CF_END;
#endif /* CONF_NOEXTERN */

#ifndef CONF_NOEXTERN
CEXTERN int SysCfEnd;
#else
int SysCfEnd = SYSCF_END;
#endif /* CONF_NOEXTERN */

#undef CEXTERN			/* cleanup */

#endif /* CONF_H */
