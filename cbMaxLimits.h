/************************************************************************
 * This is a simple class (cause I'm a simple guy) to store the global
 * limits for the maximum sizes of various object arrays in the CB.
 *
 * Copyright Jon Trulson under the MIT License. (See LICENSE).
 ***********************************************************************/

#ifndef _CBMAXLIMITS_H
#define _CBMAXLIMITS_H

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

    // setters
    void setMaxPlanets(unsigned int max)
    {
        m_maxplanets = max;
    }
    void setMaxShips(unsigned int max)
    {
        m_maxships = max;
    }
    void setMaxUsers(unsigned int max)
    {
        m_maxusers = max;
    }
    void setMaxHist(unsigned int max)
    {
        m_maxhist = max;
    }
    void setMaxMsgs(unsigned int max)
    {
        m_maxmsgs = max;
    }
    void setMaxTorps(unsigned int max)
    {
        // This is hardcoded to 9 for now.  Someday when we complete
        // the shiptypes work, we can then allow this one to be
        // dynamic too.
        m_maxtorps = 9;
    }

private:

    // current limits for Universe building (CB array maximums)
    unsigned int m_maxplanets;
    unsigned int m_maxships;
    unsigned int m_maxusers;
    unsigned int m_maxhist;
    unsigned int m_maxmsgs;
    unsigned int m_maxtorps;
};

#endif // _CBMAXLIMITS_H
