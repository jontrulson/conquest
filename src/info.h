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

#ifndef _INFO_H
#define _INFO_H

#include <string>

/* Return values for utIsSpecial(). */
enum InfoNear {
    NEAR_SHIP = 1,
    NEAR_PLANET = 2,
    NEAR_NONE = 3,
    NEAR_DIRECTION = 4,
    NEAR_ERROR = 5,
};

/* Tokens for utIsSpecial() and clbFindSpecial() */
enum InfoSpecial {
    SPECIAL_NOTSPECIAL = 1,
    SPECIAL_ENEMYSHIP = 2,
    SPECIAL_FUELPLANET = 3,
    SPECIAL_ENEMYPLANET = 4,
    SPECIAL_SHIP = 5,
    SPECIAL_PLANET = 6,
    SPECIAL_TEAMSHIP = 7,
    SPECIAL_ARMYPLANET = 8,
    SPECIAL_WEAKPLANET = 9,
    SPECIAL_TEAMPLANET = 10,
    SPECIAL_REPAIRPLANET = 11,
    SPECIAL_HOMEPLANET = 12,
};


void info( const std::string& inbuf, char ch, bool doOutput );
void infoPlanet( const std::string& str, int pnum, int snum, bool doOutput );
void infoShip( int snum, int scanner, bool doOutput );

#endif /* _INFO_H */
