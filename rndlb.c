#include "c_defs.h"

/************************************************************************
 * Copyright Jon Trulson under the MIT License. (See LICENSE).
 ***********************************************************************/


/*  rndini - initialize the random number package */
void rndini(void)
{
    time_t thetime = time(0);

    /* rand() isn't all that great, but for a client on mingw, should
     * be ok */

#if !defined(MINGW)
    srand48((long) thetime);
#else
    srand((unsigned int) thetime);
#endif

    return;
}


/*  rnd - random real number in the range [0..1) */
real rnd ( void )		/* use 48bit linear congruential */
{
    real rc;

    /* rand() isn't all that great, but for a client on mingw, should be ok */
#if !defined(MINGW)
    rc = (real) drand48();
#else
    rc = (real)rand() / (real)RAND_MAX;
#endif

#ifdef DEBUG_RANDOM
    utLog("rnd(): drand48() = %f", rc);
#endif
    return(rc);
}

/*  rnduni - random real number in the specified range */
real rnduni ( real rlow, real rhigh )
{
    return(rnd() * (rhigh-rlow) + rlow);

}


/*  rndint - random int in the specified range */
int rndint ( int ilow, int ihigh )
{
    int rc;

    rc = (int)(rnd() * (real)(ihigh - ilow + 1)) + ilow;
#ifdef DEBUG_RANDOM
    utLog("rndint(): rc = %d", rc);
#endif

    return(rc);

}


/*  rndnor - normally distributed random real number */
real rndnor ( real mean, real stddev )
{
    real v1, v2, z;

    do                          /* repeat after me... */
    {
        v1 = ((real) -log((real) 1.0 - rnd()) );
        v2 = ((real) -log((real) 1.0 - rnd()) );
    }
    while ( 2.0 * v1 < pow((real)(v2-1.0), (real)2) );

    if ( rnd() > 0.5 )
        z = 1.0;
    else
        z = -1.0;

    return(stddev * z * v2 + mean);

}


