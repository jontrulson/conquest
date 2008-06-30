/************************************************************************
 *
 * $Id$
 *
 * Copyright 2008 Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 ***********************************************************************/

#ifndef _HUD_H
#define _HUD_H

/* cockpit hud data items */

#define HUD_STR_SZ          64
#define HUD_INFO_STR_SZ     256
#define HUD_PROMPT_SZ       1024

/* an enum for the various types of alert conditions one can be in */
typedef enum {
  GREEN_ALERT = 0,
  PROXIMITY_ALERT,
  YELLOW_ALERT,
  TORP_ALERT,
  RED_ALERT,                  /* close enough to be concerned */
  PHASER_ALERT,               /* close enough to be in phaser range */
} alertLevel_t;



struct _warp {
  real warp;
  int  color;
  char warpstr[HUD_STR_SZ];
};

struct _heading {
  int  head;
  int  color;
  char headstr[HUD_STR_SZ];
};

struct _kills {
  real kills;
  int  color;
  char killstr[HUD_STR_SZ];
};

struct _alertStatus {
  alertLevel_t  alertLevel;
  int           aShip;          /* hostile ship causing the alert, if any */
  int           color;          /* alert text and alert border color */
  char          alertstr[HUD_STR_SZ];
};

#if 0
struct _alertBorder {
  int alertColor;
};
#endif

struct _shields {
  int  shields;
  int  color;
  char shieldstr[HUD_STR_SZ];
};

struct _damage {
  real damage;
  int  color;
  char damagestr[HUD_STR_SZ];
};

struct _fuel {
  real fuel;
  int  color;
  char fuelstr[HUD_STR_SZ];
};

struct _alloc {
  int walloc, ealloc;
  int color;
  char allocstr[HUD_STR_SZ];
};

struct _temp {
  real temp;
  int  color;
  int  overl;                    /* overloaded */
  char tempstr[HUD_STR_SZ];
};

struct _tow {
  char str[HUD_STR_SZ];
  int color;
};

struct _armies {
  char str[HUD_STR_SZ];
  char label[HUD_STR_SZ];
  int color;
};

struct _cloakdest {             /* cloak OR destruct msg */
  char str[HUD_STR_SZ];
  int color;
  int bgcolor;
};

/* prompt areas */

struct _prompt_lin {
  char str[HUD_PROMPT_SZ];
};

struct _xtrainfo {
  char str[HUD_INFO_STR_SZ];
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
  struct _cloakdest         cloakdest;
  struct _prompt_lin        p1;
  struct _prompt_lin        p2;
  struct _prompt_lin        msg;
  struct _xtrainfo          xtrainfo;
  struct _recId             recId;
  struct _recTime           recTime;
} hudData_t;


/* global hud data */
#if defined(NOEXTERN_HUD)
hudData_t hudData;
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



#endif /* _HUD_H */
