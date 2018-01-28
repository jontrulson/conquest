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


#ifndef _INITVEC_H
#define _INITVEC_H

// Some helpers for freeing, and initializing some of the 1D and 2D
// static vectors we use.
#define _INIT_VEC1D(_myvec, _mytype, _mysize)   \
    {                                           \
        _myvec.clear();                         \
        for (int i=0; i<_mysize; i++)             \
            _myvec.push_back(_mytype());        \
    }

#define _INIT_VEC2D(_myvec, _mytype, _mysize, _mysize2)         \
    {                                                           \
        _myvec.clear();                                         \
        for (int i=0; i<_mysize; i++)                             \
            _myvec.push_back(vector<_mytype>(_mysize2));        \
    }

#endif // _INITVEC_H
