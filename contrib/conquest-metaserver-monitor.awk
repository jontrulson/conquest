#!/usr/bin/awk -f
# 
# This script requires GNU AWK (gawk)
#
# Matt in Canada
#
# Queries a Conquest metaserver for games with active players
# See http://conquest.radscan.com/conquest.html for more info
# 
# Accepts the following -v parameters
#   server (name of metaserver to query)
#   port   (port of metaserver to query)
#   email  (email addresses to notify, separated by commas)
#
# Fields
# =====================================================================
#  ?  SERVER_HOST  PORT  NAME  VERSION  MOTD  MAX_SLOTS  ACTIVE#  VACANT#  ROBOT#  ?  ?  CONTACT_EMAIL  LOCALTIME
#  1  2            3     4     5        6     7          8        9        10      11 12 13             14
#

BEGIN {
    if (!server)
        server = "conquest.radscan.com"
    if (!port)
        port = "1700"
    
    if (!email) {
        printf "%s: You must set the email addresses to notify using `-v email=addr1,addr2'\n", ARGV[0] > "/dev/stderr"
        exit
    }
        
    FS="|"
    NetService = "/inet/tcp/0/" server "/" port
    print "name" |& NetService

    while ((NetService |& getline) > 0) {
        if ($8 > 0) {
            printf "echo \"Conquest server has %d/%d ACTIVE players (%d VACANT, %d ROBOT)\nconquestgl -f -s %s:%d\n\n\" | mail -s \"ACTIVE CONQUEST GAME\" %s", 
                $8, $7, $9, $10, $2, $3, email | "/bin/sh"
            close("/bin/sh")
        }
    }

    close(NetService)
}
