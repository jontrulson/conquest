/* testmeta */

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
        return FALSE;
    }

    /* put host's address and address type into socket structure */
    memcpy((char *)&sa.sin_addr, (char *)hp->h_addr, hp->h_length);

    sa.sin_family = hp->h_addrtype;

    sa.sin_port = htons(1700);

    if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP )) < 0)
    {
        perror("socket");
        return FALSE;
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
