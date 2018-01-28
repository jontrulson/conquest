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

#ifndef CONQNET_H_INCLUDED
#define CONQNET_H_INCLUDED

#define CN_DFLT_PORT   1701
#define CN_DFLT_SERVER "localhost"

#define META_DFLT_PORT   1700
#define META_DFLT_SERVER "conquest.radscan.com"

#define MAXHOSTNAME 255 // According to RFC 1035
#define MAXPORTNAME 10  // ":<port number or service name>", according
                        // to The Book of Jon

#if defined(__linux__) && !defined(_old_linux_)
#define AddrLen         unsigned int

/* setsockopt incorrectly prototypes the 4th arg without const. */
#define SSOType         void*
#endif

#if defined(__FreeBSD__)
#define AddrLen         socklen_t
#endif

#if defined(sun)
/* setsockopt prototypes the 4th arg as const char*. */
# define SSOType         const char*
#endif

#if !defined(AddrLen)
# define AddrLen         int
#endif

#if !defined(SSOType)
# define SSOType         const void*
#endif
#if !defined(CNCTType)
# define CNCTType        const struct sockaddr
#endif

#if !defined(INADDR_NONE)
# define INADDR_NONE     ((in_addr_t)0xffffffff)
#endif

#endif /* CONQNET_H_INCLUDED */
