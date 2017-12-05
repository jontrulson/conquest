#!/usr/bin/perl -w
#
#
# $Id$
#

use IO::Socket::INET;
use HTML::Template;
use strict;

my $host = "conquest.radscan.com";
my $port = "1700";
my $tmpl = new HTML::Template( filename => "$ENV{DOCUMENT_ROOT}/templates/conqserv.tmpl",
                               no_includes => 1 );

my @srvlist;

my $sock = IO::Socket::INET->new("$host:$port")
    or die "Could not connect to $host:$port: $!\n";

while (<$sock>)
{
    chomp;

    my ($ver, $server, $port, $name, $sver, $motd, $tots, $act, $vac, 
        $rob, $flags, $servproto, $contact, $ltime) = split(/\|/, $_);


#    print "server = $server, prot = $port, nm = $name sver = $sver, motd = $motd tots = $tots act = $act vac = $vac rob = $rob flags = $flags";

    my $flying = $act + $vac + $rob;
    my $sful = "$server" . ":" . "$port";
    my $sstat = sprintf( "(%d/%d) %d-active %d-vacant %d-robot", 
                         $flying, $tots, $act, $vac, $rob );
    my $flagstr;

    $flagstr .= "Refit " if ( $flags & 1 );
    $flagstr .= "Vacant " if ( $flags & 2 );
    $flagstr .= "SlingShot " if ( $flags & 4 );
    $flagstr .= "NoDoomsday " if ( $flags & 8 );
    $flagstr .= "KillBots " if ( $flags & 16 );
    $flagstr .= "SwitchTeam " if ( $flags & 32 );

    push @srvlist, { server => "$sful",
                     vers => $sver,
                     name => $name,
                     motd => $motd,
                     flags => $flagstr,
                     status => $sstat,
                     contact => $contact,
                     ltime => $ltime };
}

$tmpl->param( srvlist => \@srvlist );

print "Content-type: text/html\n\n", $tmpl->output;

close($sock);



