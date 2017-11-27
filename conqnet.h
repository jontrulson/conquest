/*
 * Network stuff
 *
 * Copyright Jon Trulson under the MIT License. (See LICENSE).
 */

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
