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

#ifndef _HUD_H
#define _HUD_H

#include <string>

/* cockpit hud data items */

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
    std::string str;
};

struct _heading {
    int     head;
    cqColor color;
    std::string str;
};

struct _kills {
    real    kills;
    cqColor color;
    std::string str;
};

struct _alertStatus {
    alertLevel_t  alertLevel;
    int           aShip;          /* hostile ship causing the alert, if any */
    cqColor       color;          /* alert text and alert border color */
    std::string str;
};

struct _shields {
    int     shields;
    cqColor color;
    std::string str;
};

struct _damage {
    real    damage;
    cqColor color;
    std::string str;
};

struct _fuel {
    real    fuel;
    cqColor color;
    std::string str;
};

struct _alloc {
    int     walloc;
    int     ealloc;
    cqColor color;
    std::string str;
};

struct _temp {
    real    temp;
    cqColor color;
    int     overl;                    /* overloaded */
    std::string str;
};

struct _tow {
    bool     towstat;              /* true if towing/towedby */
    cqColor color;
    std::string str;
};

struct _armies {
    cqColor color;
    int     armies;
    std::string str;
};

struct _raction {               /* robot action */
    cqColor color;
    int     action;
    std::string str;
};

struct _destruct {              /* destruct msg */
    cqColor color;
    int     fuse;
    std::string str;
};

/* prompt areas */

struct _prompt_lin {
    std::string str;
};

/* data from last info command for the iconic ship */
struct _info {
    real lastblast;

    int  lastang;
    int  lastdist;
    int  lasttarget;

    std::string lastblaststr;  /* last "FA:" blast (firing) angle */
    std::string lasttargetstr; /* last target in char *form */
    std::string lasttadstr;    /* full "TA/D" target:ang/dist str */
};

struct _recId {
    std::string str;
};

struct _recTime {
    std::string str;
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
void hudSetShields(int snum, bool *dobeep);
void hudSetDamage(int snum, real *lastdamage);
void hudSetFuel(int snum);
void hudSetAlloc(int snum);
void hudSetTemps(int snum);
void hudSetTow(int snum);
void hudSetArmies(int snum);
void hudSetRobotAction(int snum);
void hudSetDestruct(int snum);
void hudSetPrompt(int line, const std::string& prompt, int pcolor,
                  const std::string& buf, int color);
void hudClearPrompt(int line);
void hudSetInfoFiringAngle(real blastang);
void hudSetInfoTarget(int tnum, bool isShip);
void hudSetInfoTargetAngle(int ang);
void hudSetInfoTargetDist(int tdist);
void hudSetRecId(const std::string& str);
void hudSetRecTime(const std::string& str);

#endif /* _HUD_H */
