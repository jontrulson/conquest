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

#include "c_defs.h"
#include "conqdef.h"
#include "conqnet.h"

int main(int argc, char *argv[])
{
    int s;
    struct sockaddr_in sa;
    struct hostent *hp;
    char *remotehost = NULL;
    char *msg = NULL;
    int msglen;
    int i;

    while ((i = getopt(argc, argv, "s:m:")) != EOF)    /* get command args */
        switch (i)
        {
        case 's':
            remotehost = optarg;
            break;

        case 'm':
            msg = optarg;
            msglen = strlen(msg);
            break;

        default:
            printf("Usage: testmeta -s <server> -m <msg>\n");
            exit(1);
        }

    if (!msg || !remotehost)
    {
        printf("Usage: testmeta -s <server> -m <msg>\n");
        exit(1);
    }


    if ((hp = gethostbyname(remotehost)) == NULL)
    {
        fprintf(stderr, "testmeta: %s: no such host\n", remotehost);
        return false;
    }

    /* put host's address and address type into socket structure */
    memcpy((char *)&sa.sin_addr, (char *)hp->h_addr, hp->h_length);

    sa.sin_family = hp->h_addrtype;

    sa.sin_port = htons(1700);

    if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP )) < 0)
    {
        perror("socket");
        return false;
    }

    printf("Connecting to host: %s, udp port %d ...\n",
           remotehost, 1700);

    if (sendto(s, msg, msglen, 0, &sa, sizeof(struct sockaddr_in)) < 0)
    {
        perror("sendto");
        exit(1);
    }

    printf("msg sent\n");
    return 0;
}
