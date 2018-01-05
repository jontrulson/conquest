/************************************************************************
 * This is a simple class (cause I'm a simple guy) to store the global
 * limits for the maximum sizes of various object arrays in the CB.
 *
 * Copyright Jon Trulson under the MIT License. (See LICENSE).
 ***********************************************************************/

#ifndef _CBMAXLIMITS_H
#define _CBMAXLIMITS_H

#include <stdint.h>

class cbMaxLimits {

public:
    cbMaxLimits()
    {
        m_maxplanets = 0;
        m_maxships = 0;
        m_maxusers = 0;
        m_maxhist = 0;
        m_maxmsgs = 0;
    }

    // getters
    const int maxPlanets() const
    {
        return m_maxplanets;
    }
    const int maxShips() const
    {
        return m_maxships;
    }
    const int maxUsers() const
    {
        return m_maxusers;
    }
    const int maxHist() const
    {
        return m_maxhist;
    }
    const int maxMsgs() const
    {
        return m_maxmsgs;
    }

    // setters
    void setMaxPlanets(int max)
    {
        m_maxplanets = max;
    }
    void setMaxShips(int max)
    {
        m_maxships = max;
    }
    void setMaxUsers(int max)
    {
        m_maxusers = max;
    }
    void setMaxHist(int max)
    {
        m_maxhist = max;
    }
    void setMaxMsgs(int max)
    {
        m_maxmsgs = max;
    }

private:

    // current limits for Universe building (CB array maximums)
    uint8_t m_maxplanets;
    uint8_t m_maxships;
    uint8_t m_maxusers;
    uint8_t m_maxhist;
    uint8_t m_maxmsgs;
};

#endif // _CBMAXLIMITS_H
