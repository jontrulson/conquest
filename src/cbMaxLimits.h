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


/************************************************************************
 * This is a simple class (cause I'm a simple guy) to store the global
 * limits for the maximum sizes of various object arrays in the CB.
 ***********************************************************************/

#ifndef _CBMAXLIMITS_H
#define _CBMAXLIMITS_H

#include "conqdef.h"

class cbMaxLimits {

public:
    cbMaxLimits()
    {
        m_maxplanets = 0;
        m_maxships = 0;
        m_maxusers = 0;
        m_maxhist = 0;
        m_maxmsgs = 0;
        // This is hardcoded to 9 for now.  Someday when we complete
        // the shiptypes work, we can then allow this one to be
        // dynamic too.
        m_maxtorps = 9;
        // we hardcode this to 3 for now, until it's actually
        // implemented in some fashion
        m_maxshiptypes = 3;
    }

    // getters
    const unsigned int maxPlanets() const
    {
        return m_maxplanets;
    }
    const unsigned int maxShips() const
    {
        return m_maxships;
    }
    const unsigned int maxUsers() const
    {
        return m_maxusers;
    }
    const unsigned int maxHist() const
    {
        return m_maxhist;
    }
    const unsigned int maxMsgs() const
    {
        return m_maxmsgs;
    }
    const unsigned int maxTorps() const
    {
        // This is hardcoded to 9 for now.  Someday when we complete
        // the shiptypes work, we can then allow this one to be
        // dynamic too.
        return 9;
    }
    const unsigned int maxShiptypes() const
    {
        // This is hardcoded to 3 for now.  Someday when we complete
        // the shiptypes work, we can then allow this one to be
        // dynamic too.
        return 3;
    }

    // setters
    void setMaxPlanets(unsigned int max)
    {
        m_maxplanets = CLAMP(1, ABS_MAXPLANETS, max);
    }
    void setMaxShips(unsigned int max)
    {
        m_maxships = CLAMP(1, ABS_MAXSHIPS, max);
    }
    void setMaxUsers(unsigned int max)
    {
        m_maxusers = CLAMP(1, ABS_MAXUSERS, max);
    }
    void setMaxHist(unsigned int max)
    {
        m_maxhist = CLAMP(1, ABS_MAXHIST, max);
    }
    void setMaxMsgs(unsigned int max)
    {
        m_maxmsgs = CLAMP(1, ABS_MAXMSGS, max);
    }
    void setMaxTorps(unsigned int max)
    {
        // This is hardcoded to 9 for now.  Someday when we complete
        // the shiptypes work, we can then allow this one to be
        // dynamic too.
        m_maxtorps = 9;
    }
    void setMaxShiptypes(unsigned int max)
    {
        // This is hardcoded to 3 for now.  Someday when we complete
        // the shiptypes work, we can then allow this one to be
        // dynamic too.
        m_maxshiptypes = 3;
    }

private:

    // current limits for Universe building (CB array maximums)
    unsigned int m_maxplanets;
    unsigned int m_maxships;
    unsigned int m_maxusers;
    unsigned int m_maxhist;
    unsigned int m_maxmsgs;
    unsigned int m_maxtorps;
    unsigned int m_maxshiptypes;
};

#endif // _CBMAXLIMITS_H
