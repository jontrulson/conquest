/************************************************************************
 * Copyright Jon Trulson under the MIT License. (See LICENSE).
 ***********************************************************************/

#ifndef _INITVEC_H
#define _INITVEC_H

// Some helpers for freeing, and initializing some of the 1D and 2D
// static vectors we use.
#define _INIT_VEC1D(_myvec, _mysize)            \
    {                                           \
        _myvec.clear();                         \
        _myvec.reserve(_mysize);                \
        for (int i; i<_mysize; i++)             \
            _myvec.push_back({});               \
    }

#define _INIT_VEC2D(_myvec, _mysize, _mysize2)  \
    {                                           \
        _myvec.clear();                         \
        _myvec.reserve(_mysize);                \
        for (int i; i<_mysize; i++)             \
            _myvec.push_back({});               \
        for (auto &j : _myvec)                  \
        {                                       \
            j.reserve(_mysize2);                \
            for (int i=0; i<_mysize2; i++)      \
                j.push_back({});                \
        }                                       \
    }

#endif // _INITVEC_H
