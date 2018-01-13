/************************************************************************
 * Copyright Jon Trulson under the MIT License. (See LICENSE).
 ***********************************************************************/

#ifndef _INITVEC_H
#define _INITVEC_H

// Some helpers for freeing, and initializing some of the 1D and 2D
// static vectors we use.
#define _INIT_VEC1D(_myvec, _mytype, _mysize)   \
    {                                           \
        _myvec.clear();                         \
        for (int i; i<_mysize; i++)             \
            _myvec.push_back(_mytype());        \
    }

#define _INIT_VEC2D(_myvec, _mytype, _mysize, _mysize2)         \
    {                                                           \
        _myvec.clear();                                         \
        for (int i; i<_mysize; i++)                             \
            _myvec.push_back(vector<_mytype>(_mysize2));        \
    }

#endif // _INITVEC_H
