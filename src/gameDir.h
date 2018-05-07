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

#ifndef _GAMEDIR_H
#define _GAMEDIR_H

#include "string"

// A simple class to store and retrieve a game subdirectory

class gameDir {
public:
    gameDir() { m_gameDir.clear(); }

    // check the characters provided - they must be [a-z] [A-Z] [0-9]
    // and '-' or '_'.  Invalid chars will be converted to
    // underscores.
    void set(const char *name)
    {
        if (!name)
            return;

        std::string tempstr(name);

        for (int i=0; i<tempstr.size(); i++)
        {
            if ( !((tempstr[i] >= 'A' && tempstr[i] <= 'Z')
                   || (tempstr[i] >= 'a' && tempstr[i] <= 'z')
                   || (tempstr[i] >= '0' && tempstr[i] <= '9')
                   || (tempstr[i] == '_')
                   || (tempstr[i] == '-')) )
                tempstr[i] = '_'; // replace bad chars with _
        }
        m_gameDir = tempstr;
    }

    const std::string& get() const
    {
        return m_gameDir;
    }

private:
    std::string m_gameDir;
};

#endif /* _GAMEDIR_H */
