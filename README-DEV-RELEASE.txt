11/20/2003
7.2d

        - merge of CS-12 into main branch (7.2d)

        - removed '-p <port>' option from conquest.  To specify a port
          other than the default, add it to the server name seperated
          by a colon.  For example, to connect to a server at
          conquest.radscan.com, port 1702, use:

          conquest -s conquest.radscan.com:1702

        - added meta server (conqmetad) and functionality to conquestd.

          - meta server listens on TCP and UDP port 1700.  The UDP
            port is incoming only, and is used to recieve updates from
            conquestd's.  TCP is outbound only, and will dump the
            current server list and their stats ('|' delimited) upon
            connect.

            Try 'telnet conquest.radscan.com 1700' . :)

          - new options added to conquestd:

            '-m' update the meta server (default: conquest.radscan.com)

            '-M <metaserver>' use metaserver <metaserver>

            '-N <myname>' tell the metaserver to use <myname> as the
            server address.  Otherwise conqmetad will use the src
            address on the incoming packet to determine the conquestd
            server address.

        - still need to add ability in conquest to get server list
          from meta server and allow the user to select one.

        - fixed conquestd and conquest to improve responsivness.
          conquest will now process packets as soon as they are
          received regardless of the update rate, instead of wating
          for a timer cycle.  

          conquestd updates the client whenever the client does
          something (raise shields, etc).  

          These changes made a huge diff in responsiveness to
          commands, especially at lower update rates. Sorry about that
          oversight :)

        - client now processes incoming packets while in help, userlist,
          etc, rather than only when at the battle screen.


10/25/2003
7.2c-cs-12

        - feedback messages are no longer recorded.  Tersables
          are only recorded if the user has them enabled when the
          recording was made.  Server recordings never contain
          Tersables. 

        - notify opers when someone enters conqoper

        - opers get all messages sent to GOD

        - applied patch from Josef Jahn that corrected a problem where
          conquestd would only bind to the IP address the host
          resolved to.  Now it will bind to all interfaces.

          Now that this bug is fixed, the conquest client now defaults
          to 'localhost' if no server was specified on the command line.

        - fixed problem with reserved ship slots and no valid process
          controlling them in newship().  Also, when disconnecting
          from the main menu, turns off the ship before exiting. 

        - fixed problem where the default update rate was being set to
          2 by default, rather than the 5 it should have been.

        - clean up of the conf stuff

        - some cleanup of recording code

        - send relevant User packet *before* sending a Hist packet.

        - new options for conquestd 

          -d

            When this option is specified to conquestd, conquestd will
            run in daemon mode (fork itself into the background and
            detach).

          -l 

            When this option is specified to conquestd, conquestd will
            only accept connections from local transports (loopback).
            This is useful if you want to run a server, but only want
            to accept connections from clients running on the same
            machine as the server.

          -u user 

            When this option is specified to conquestd, conquestd will
            do a setuid() to this user before beginning operations.

          Options -u and -d are handy for starting up a server
          at system boot time.  I use something like:

          <path-to-conquest>/bin/conquestd -d -u nobody

          in an /etc/rc.d/init.d/conquestd script.

        - new '-t' option for conquest.

          when conquest is run with the '-t' flag, then no attempt
          will be made to read or write a user config file.  This is
          useful for people who still want to run telnetable servers,
          yet prevent such users from stomping over each other's
          settings.

          Such users always use the default options, which means they
          will need to set them to their preferences whenever they
          enter the game, since they are never saved.  If you (the
          user) find this annoying, then download the src, and use the
          client to connect to the server properly :).

        - new 'conquestsh' program for telnetable servers.  When
          setting up a telnetable server, use 'conquestsh' as the
          user's shell program.  conquestsh exec's the conquest client
          bianary, passing the '-t' flag, and the '-s localhost'
          flag.  Feel free to edit this program for your site, for
          example if you want the telnetable client to connect to a
          different server than 'localhost'.


        - removed GL/X11 dependancies that snuck in.

10/11/2003
7.2c-cs-11

	This is the first release of a development version of the
	client/server conquest.  There have been quite a few changes from v7.x!
	Please read this document for the juicy details ;-)

        The common block revision has changed, making this version
        incompatible with previous versions.  There may be more
        changes on the way to a final release, but I will try to limit
        them as much as possible.

	To build it, I would use something like (I'll assume below
	that you will install conquest in /opt/conqnet):

	$ ./configure --prefix=/opt/conqnet --enable-static=no
	$ make
	
	Then the usual 'make install' as root.

	Run conqoper, init the universe and open the game.  

        Please feel free to contact me with any problems, patches,
        etc.          
        
	Of course the major change in this release is that conquest
	uses a network based client server model.  There is now a
	seperate conquestd program that listens to network requests
	from clients (conquest).  If you want to run a game on your
	machine, you will need to start conquestd like so:

	/opt/conqnet/bin/conquestd &

        I would do this from a seperate non-privileged account of
        course, if you intend to invite the unwashed internet
        masses onto your server to play.  

        The final version of conquestd should allow you to specify the
        uid to run under (like 'nobody') for internet servers.

	Then if you just type 'conquest', it will try to attach to the
	conquest daemon on your local host.  To connect to a remote
	host, use the '-s' switch.  

	This will take you to the login screen.  Choose a username,
	password etc, login, and kick some ass.

        I have setup a public testing server here for this release
	that you can connect to like so:

	/opt/conqnet/bin/conquest -s conquest.radscan.com

        Note, as this is a dev release, there may be changes made to
        the version of the public server I have running at
        conquest.radscan.com that might require you to download a new
        src tree in order to connect (if the common block or protocol
        revision changes), or the universe might get reset,
        klingons and romulans might start dating each other, etc.  I
        will try to keep these changes to a minimum, but expect the
        unexpected until the stable version is released.

        Once an official 'stable' release is made, the old telnet
        server there (v7.2) will probably go away.

        There may eventually be multiple servers running (one for
        experienced playas, for example, running on a different port.)

	There is still much work to be done.  The final release will
	be called v8.0 and probably will not include a graphical
	client.  That will come later ;-)

        Here is a (big) list of the most significant changes and other
        information you may find useful.  The conquest documentation
        itself will be updated when final release approaches, for now
        it's essentially untouched from the last stable release (7.2).

        - conquest is now client/server.  In the old days, the
          conquest binary had to be run on the same machine as the
          common block (universe).  In order to run a 'server', a telnet
          interface was used so remote players could connect.

          Now, conquest is a client that can connect over the network
          to any conquest server (conquestd).  In the future, there
          will be a graphical client, using OpenGL to render the main
          battle screen.  That will probably be my next big priority
          when this version is fully released in stable form as
          version 8.

          The protocol is endian safe of course.

        - The conquest login screen provides information about the
          server you are attached to - number of active ships, server
          name, enabled flags (see below), MOTD, etc.

        - The daemon (conquestd) listens on TCP port 1701 by default.
          You can change this by supplying '-p <port>' to the
          conquestd command line.

	  If you want to run your own public internet server, you need
          to allow tcp/1701 into your firewall (or whatever port you
          configured your server to use).  I would recomend running
          the daemon as an unprivileged user.  Oh, and please let me
          know about it - I'll add it to the list on the Conquest
          homepage.  Also, I may drop in for a little ass-kicking
          (mine or yours - you decide ;-)

	- Client Updates.  From the options menu, a player can set the
          update interval anywhere from 1 to 10 updates per second.
          The default is currently 5.  There is no AllowFastUpdates
          capability anymore.  I would not recommend 10 per second on
          a modem. Try different ones until you find one that works
          best for your connection.  In the old conquest, 1 and 2 were
          the only options.

        - Conquest operators are now defined as any user with the
          OOPT_OPER option set in his or her user profile.  They will
          be listed in the (U)ser list with a '+' to the left of their
          name.  

          Currently a user marked with OPER can only start and stop
          server recordings from within the game using 'EXEC' messages
          sent to GOD from within the game.  Other than that, there is
          no difference compared to a normal user.

          Of course, only someone with shell access on the server
          machine, and who is a member of the conquest group can run
          the conqoper program.  Root can always run conqoper.

          In the future, more EXEC commands may be provided to
          conquest operators so they can become the tin plated
          dictators with delusions of godhood most of them secretly
          want to be. :)

        - Previous versions of conquest (v7.0+) used to distinguish
          between 'local' and 'remote' users.  Local users were users
          who were playing conquest from a shell account on the
          machine where conquest was installed.  Remote users were
          users playing from a generic conquest account, usually setup
          by the system administrator, that had the conquest
          executable as the shell.  In this way a 'server' could be run
          by providing telnet access to this special account from the
          internet.  

          'Remote' users were denoted by a '@' next to their usernames
          in conquest.

          Since Conquest now uses a client server model, all users are
          remote.  The '@' tag is gone.  Even playing on a local
          machine is still accomplished by connecting to a local
          conquest server via TCP, so there is no such thing as a
          'local' user anymore.

          Also for this reason, the conquest client is no longer
          set-groupid conquest - no need, since no special privileges
          or access is required.  If you can connect to a server, you
          can play.

        - user and system configuration (conquestrc) have changed
          significantly.  Some options are gone now, others moved into
          user config, etc.

          Many of the Sysconf options have been moved into User
          options (ETA stats, LR torpscan, etc)

          Some user options like clear_old_msgs have been removed.

          The 'compile options' are no longer viewable, since they
          don't matter much from the player perspective, and are of
          no use to a client anyway.

          The server (conquestd) advertises certain game flags enabled
          by the conquest operator (using conqoper) to the client.
          These flags are displayed on the client login screen, as well
          as from within the client's (O)ptions Menu.  The flags are
          configured by the operator using conqoper.

          The current flags are:

          Refit              - refits are allowed.  You can refit to
                               one of the 3 types of conquest ships
                               when you have at least one kill, and
                               are orbiting a team owned planet.

          Vacant             - vacant ships are allowed.  A ship goes
                               vacant when a client exits the game
                               without self destructing first, while
                               flying a ship.

                               When vacant ships are disabled by the
                               oper, such ships are immediately killed
                               by a 'lightning bolt'.

                               NOTE: In previous versions of conquest,
                               oper's would always go vacant in this
                               case, regardless of the vacant option
                               setting.  In this version, every user
                               is subject to this option.


          SlingShot          - the SlingShot bug is enabled.  This is
                               a towing bug that existed in the
                               original VMS/RatFor version of
                               conquest.  It's a fun bug, and I
                               recieved several emails about it when I
                               inadvertantly fixed it in a later
                               version without understanding it's
                               significance. 

                               If you don't know what it does,
                               well... figure it out!  You'll need a
                               buddy to make it work though ;-)

          NoDoomsday         - when set, the doomsday machine is
                               prevented from randomly starting up and
                               trashing the universe. 

          Killbots           - when this option is enabled, robots are
                               created with a random kill point
                               value.  The kill points a ship has
                               affects the efficiency of it's engines,
                               the strength of it's weapons, etc.

                               Normally when a robot is created (by
                               attacking an unguarded homeplanet, etc),
                               it is initialized to 0 kills.  When
                               this option is enabled, a kills value
                               is randomly selected.  This means that
                               a new robot might prove to be much more
                               deadly than the default 0 kill robot.

                               This may not be a good option to enable
                               on a public server where newcomers play.
                               Fighting a 25 kill robot takes some
                               skill ;-)

          SwitchTeam         - when enabled, users can switch teams at
                               the conquest main menu.

	- the 'I' command is no longer provided.  All of those options
          have been added to the User options menu.

	- Users can no longer have multiple ships.  If you have a
          vacant ship, you will be automatically attached to it.  The
          game still has most of this code present, so this feature could
          be added back in the future if enough people whine.

        - When using the (U)sers command in the client, only those
          users attached to currently active ships, or attached to
          entries in the History ring will be listed.  There is no
          point in checking 500 user slots on each client update ;-)

          For this reason, only the conqoper program will be able to
          list *all* users.

	- the client creates it's logfile in your home directory
          (~/.conquest.log).  The conquest server, driver, etc, will
          still use the system log.  Check these for any errors or
          other oddities.

        - When you die, you now get to see your ship explode first
          before being taken to the 'dead' screen.

	- There are still some debugging messages printed out logged.
          These will be removed in a released version.

        - added new 'Friend' target for messages.  You can send a
          message to all friendly ships by specifying 'fr' as the
          destination for a message.

        - new AltHUD user option.  This is like the regular HUD, but
          adds some of the information returned from infoship and
          infoplanet calls to the lower alert line on the main
          display.  This can be useful in battle when messages or
          other commands remove the info after requesting it.

          Current info presented is:

            FA: (Firing Angle).  This lets you know the direction that
                any further firing requests will use, unless a
                direction is explicitly specified to the 'p' or 'f'
                commands.

            TA/D: (Target Angle / Distance). This item displays the
                  Target name, Target Angle and Target Distance
                  obtained in the last infoship/infoplanet request,
                  and is updated whenever you do an info command on
                  any ship or planet.

          It is disabled by default.

        - Recording.

          - Recording has been completely reworked.  Unfortunately,
            any previous CQR files are incompatible with this release.

          - you can now select the long range or short range view of a
            ship you are watching in conqreplay.

          - The new format uses the same packet protocol that the
            clients use.  This means recording files (cqr's) are
            *much* smaller and much more efficient (cpu-wise) during the
            recording process.

            Another benefit is endian safety.  With the old format,
            recordings could only be played back on the same machine
            architecture the cqr was created on.  Since the recording
            data is now based on the packet protocol, recordings are
            now completely endian safe (network byte order is used).
            So now, some poor slob on his Sun SPARC can play back a
            recording I make on a lintel machine.

          - There are two types of recordings: client and server.

            - A client recording is made in the usual way - by calling
              conquest with the '-r <recfile>' option.

              Client recordings do not contain full information on
              other ships (fuel, etc) since, in order to limit the
              ability to cheat by hacking up the client, only pertinet
              data about another ship is ever sent to the client. 

              For this reason, client recordings are identified by the
              ship that made the recording.  Although you can still
              try to watch other ships in conqreplay than the one that
              made the recording, the experience will probably not be
              too rewarding, since much data about the ship will never
              have been sent to the client to be recorded in the first
              place. 

              For client recordings, conqreplay will always use the
              recorder's ship number as the default to the (w)atch
              command, though you can certainly select other ones,
              keeping the caveat's above in mind.

              Of course, full information is always recorded for the
              ship that made the recording.

            - Server recordings are complete (like the old recordings
              made in previous versions of conquest), since the server
              has full access to the common block (unlike clients).
             
              All active ship/planet/etc data is stored in server
              recordings.  You can watch any ship in conqreply with a
              server recording and get all info on the ship (heading,
              fuel, temps, etc).

              Server recordings are created in
              PREFIX/var/conquest/conquest-rec-<timestamp>.cqr on the
              server machine.

              Server recordings are made when an oper (anyone who
              has the ooption OOPT_OPER set in their user record)
              sends a specially crafted message to GOD from within the
              game. 

              Currently understood messages are:

               exec recon

               - turns recording on.

               exec recoff

               - turns recording off

               Other oper options will probably be added in the future.


        - Thanks to Clint Adams, the autoconf stuff has been
          significantly updated.

        - there are a whole buttload of other changes, most of them
          internal.  Let me know if I forgot to mention any important
          ones. 
          
        - I looked upon my creation, and saw that it was good. :)

