/************************************************************************
 * Copyright Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 ***********************************************************************/

#ifndef _HUD_H
#define _HUD_H

/* cockpit hud data items */

#define HUD_STR_SZ          64
#define HUD_INFO_STR_SZ     256
#define HUD_PROMPT_SZ       256

/* an enum for the various types of alert conditions one can be in */
typedef enum {
  GREEN_ALERT = 0,
  PROXIMITY_ALERT,
  YELLOW_ALERT,
  TORP_ALERT,
  RED_ALERT,                  /* close enough to be concerned */
  PHASER_ALERT,               /* close enough to be in phaser range */
} alertLevel_t;

/* alert/warning/critical limits */

#define HUD_E_ALRT     50
#define HUD_E_WARN     80
#define HUD_E_CRIT     80

#define HUD_W_ALRT     50
#define HUD_W_WARN     80
#define HUD_W_CRIT     80

#define HUD_SH_CRIT    20

#define HUD_F_ALRT     500
#define HUD_F_WARN     200
#define HUD_F_CRIT     100

#define HUD_HULL_ALRT  10
#define HUD_HULL_WARN  65
#define HUD_HULL_CRIT  75



struct _warp {
  real     warp;
  cqColor  color;
  char     str[HUD_STR_SZ];
};

struct _heading {
  int     head;
  cqColor color;
  char    str[HUD_STR_SZ];
};

struct _kills {
  real    kills;
  cqColor color;
  char    str[HUD_STR_SZ];
};

struct _alertStatus {
  alertLevel_t  alertLevel;
  int           aShip;          /* hostile ship causing the alert, if any */
  cqColor       color;          /* alert text and alert border color */
  char          str[HUD_STR_SZ];
};

struct _shields {
  int     shields;
  cqColor color;
  char    str[HUD_STR_SZ];
};

struct _damage {
  real    damage;
  cqColor color;
  char    str[HUD_STR_SZ];
};

struct _fuel {
  real    fuel;
  cqColor color;
  char    str[HUD_STR_SZ];
};

struct _alloc {
  int     walloc;
  int     ealloc;
  cqColor color;
  char    str[HUD_STR_SZ];
};

struct _temp {
  real    temp;
  cqColor color;
  int     overl;                    /* overloaded */
  char    str[HUD_STR_SZ];
};

struct _tow {
  int     towstat;              /* 0 = no towing, < 0 towing, > 0 towed by */
  cqColor color;
  char    str[HUD_STR_SZ];
};

struct _armies {
  cqColor color;
  int     armies;
  char    str[HUD_STR_SZ];
};

struct _raction {               /* robot action */
  cqColor color;
  int     action;
  char    str[HUD_STR_SZ];
};

struct _destruct {              /* destruct msg */
  cqColor color;
  int     fuse;
  char    str[HUD_STR_SZ];
};

/* prompt areas */

struct _prompt_lin {
  char str[HUD_PROMPT_SZ];
};

/* data from last info command for the iconic ship */
struct _info {
  real lastblast;

  int  lastang;
  int  lastdist;
  int  lasttarget;

  char lastblaststr[HUD_INFO_STR_SZ];  /* last "FA:" blast (firing) angle */
  char lasttargetstr[HUD_INFO_STR_SZ]; /* last target in char *form */
  char lasttadstr[HUD_INFO_STR_SZ];    /* full "TA/D" target:ang/dist str */
};

struct _recId {
  char str[HUD_INFO_STR_SZ];
};

struct _recTime {
  char str[HUD_INFO_STR_SZ];
};


/* This holds all of the info for the cockpit display. */
typedef struct _dspData {
  struct _warp              warp;
  struct _heading           heading;
  struct _kills             kills;
  struct _alertStatus       aStat;
  struct _shields           sh;
  struct _damage            dam;
  struct _fuel              fuel;
  struct _alloc             alloc;
  struct _temp              etemp;
  struct _temp              wtemp;
  struct _tow               tow;
  struct _armies            armies;
  struct _raction           raction;
  struct _destruct          destruct;
  struct _prompt_lin        p1;
  struct _prompt_lin        p2;
  struct _prompt_lin        msg;
  struct _info              info;
  struct _recId             recId;
  struct _recTime           recTime;
} hudData_t;


/* global hud data */
#if defined(NOEXTERN_HUD)
hudData_t hudData = {};
#else
extern hudData_t hudData;
#endif /* NOEXTERN_HUD */


void hudInitData(void);

void hudSetWarp(int snum);
void hudSetHeading(int snum);
void hudSetAlertStatus(int snum, int asnum, alertLevel_t astatus);
void hudSetKills(int snum);
void hudSetShields(int snum, int *dobeep);
void hudSetDamage(int snum, real *lastdamage);
void hudSetFuel(int snum);
void hudSetAlloc(int snum);
void hudSetTemps(int snum);
void hudSetTow(int snum);
void hudSetArmies(int snum);
void hudSetRobotAction(int snum);
void hudSetDestruct(int snum);
void hudSetPrompt(int line, char *prompt, int pcolor, 
                  char *buf, int color);
void hudClearPrompt(int line);
void hudSetInfoFiringAngle(real blastang);
void hudSetInfoTarget(int tnum);
void hudSetInfoTargetAngle(int ang);
void hudSetInfoTargetDist(int tdist);
void hudSetRecId(char *str);
void hudSetRecTime(char *str);

#endif /* _HUD_H */
