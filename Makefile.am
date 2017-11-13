AUTOMAKE_OPTIONS = foreign
ACLOCAL_AMFLAGS = -I m4 --install


# User defineable options

.rc.o:
	@target@-windres $^ -o $@

CFLAGS = @CONQ_CFLAGS@ $(SDL_CFLAGS)
CONQGROUP = @CONQGROUP@
CONQLOCALSTATEDIR = @localstatedir@/conquest
CONQLIBEXECDIR = @libexecdir@
CONQDATADIR = @datadir@
CONQSYSCONFDIR = @sysconfdir@
CONQIMGDIR = $(CONQDATADIR)/img
CONQSNDDIR = $(CONQDATADIR)/sound
CONQDOCDIR = $(CONQDATADIR)/doc/conquest

TEXTURESRC = $(DESTDIR)$(CONQSYSCONFDIR)/texturesrc
CONQINITRC = $(DESTDIR)$(CONQSYSCONFDIR)/conqinitrc
SOUNDRC    = $(DESTDIR)$(CONQSYSCONFDIR)/soundrc

CONQDOCS = HISTORY README CONQINIT.TXT conqrule conqrule.easy

IMAGES = img/*.tga img/*.txf
SOUNDS = sound/*.ogg 
TRC = *.trc
SRC = *.src

TCPW_H = tcpwrap.h
TCPW_C = tcpwrap.c

man6_MANS = conqdriv.man conqmetad.man conqoper.man conquest.man \
	conquestd.man conqai.man conqstrat.man \
	conqinit.man conquestgl.man

if ISMINGW

XLIBS = -lfreeglut -lopengl32 -lglu32 $(SDL_LIBS) -lws2_32 
#-lwsock32 

else !ISMINGW

XLIBS = @X_LIBS@ @X_EXTRA_LIBS@ -lglut -lGL -lGLU -lXt $(SDL_LIBS) @X_PRE_LIBS@ -lXext -lXi -lX11 -lXmu  

endif !ISMINGW

AM_CPPFLAGS = -DCONQUEST_GROUP=\"$(CONQGROUP)\" \
	-DCONQSTATE=\"$(CONQLOCALSTATEDIR)\" -DCONQLIBEXEC=\"$(CONQLIBEXECDIR)\" \
	-DCONQSHARE=\"$(CONQDATADIR)\" -DCONQETC=\"$(CONQSYSCONFDIR)\" \
	-DCONQDOCDIR=\"$(CONQDATADIR)\" \
	@X_CFLAGS@

AM_YFLAGS = -d

if ISMINGW
noinst_LTLIBRARIES = libconquest.la libconqclnt.la libconqserv.la \
	libconqinit.la libUi.la libUiGL.la libcqsound.la
else !ISMINGW

if GLCLIENT
noinst_LTLIBRARIES = libconquest.la libconqclnt.la libconqserv.la \
	libconqinit.la libUi.la libUiGL.la libcqsound.la
else !GLCLIENT
noinst_LTLIBRARIES = libconquest.la libconqclnt.la libconqserv.la \
	libconqinit.la libUi.la libUiCU.la
endif !GLCLIENT

endif !ISMINGW

libconquest_la_SOURCES = conqai.c conqai.h conqcm.c conqcom.h \
	conqlb.h conqlb.c rndlb.c rndlb.h conqutil.c conqutil.h \
	conqunix.c conqunix.h ibuf.c version.c conf.c sem.c color.c \
	userauth.c record.c color.h user.h shiptype.h \
	ship.h planet.h team.h msg.h doomsday.h driver.h conqinfo.h \
	history.h robot.h global.h torp.h ibuf.h conf.h  display.h \
	protocol.h packet.h packet.c userauth.h meta.c meta.h udp.c udp.h \
	rb.h rb.c cbglobal.h proto_0006.h conqcom.h

libconquest_la_LIBADD = @LTLIBOBJS@

libconqclnt_la_SOURCES = client.c client.h cproc.h cproc.c clientlb.c \
	clientlb.h playback.h playback.c cproc_0006.h cproc_0006.c

libconqserv_la_SOURCES = servauth.c server.c servauth.h server.h serverpkt.c \
	serverpkt.h servercmd.h servercmd.c sproc.h sproc.c

libUi_la_SOURCES = ui.h uimisc.c 

libUiGL_la_SOURCES = GL.c texfont.c glfont.c glfont.h \
	glmisc.c glmisc.h nConsvr.c nConsvr.h nMeta.h nMeta.c cqkeys.h \
        prm.h prm.c nAuth.h nAuth.c nWelcome.h nWelcome.c nMenu.h nMenu.c \
	nPlay.h nPlay.c nCP.h nCP.c nDead.h nDead.c nCPHelp.h nCPHelp.c \
	nShipl.h nShipl.c node.h node.c nPlanetl.h nPlanetl.c nTeaml.h \
	nTeaml.c nUserl.h nUserl.c nHistl.h nHistl.c cprintf.h glcprintf.c \
	gldisplay.h gldisplay.c disputil.h render.h render.c \
	nPlayBMenu.h nPlayBMenu.c nPlayB.h nPlayB.c nPlayBHelp.h \
	nPlayBHelp.c nOptions.h nOptions.c colorGL.c ui.h anim.c anim.h \
	GL.h hud.h hud.c blinker.h blinker.c

libUiCU_la_SOURCES = display.c cprintf.h cprintf.c disputil.h colorCU.c \
	cd2lb.h cd2lb.c iolb.h iolb.c cumisc.h cumisc.c cuclient.h \
	cuclient.c options.h options.c clntauth.c ui.h display.h 

libconqinit_la_SOURCES = conqinit-yacc.y conqinit-lex.l conqinit.h initdata.h planinit.c

libcqsound_la_SOURCES = cqsound.c cqsound.h

# for now, conquestgl is the only program we build for windows

if ISMINGW

bin_PROGRAMS = conquestgl 

else !ISMINGW

libexec_PROGRAMS = conqdriv

if GLCLIENT

bin_PROGRAMS = conquestgl conquest conqoper conqai conqstrat \
	conquestd conqmetad conqinit
else !GLCLIENT

bin_PROGRAMS = conquest conqoper conqai conqstrat \
	conquestd conqmetad conqinit

endif !GLCLIENT

endif !ISMINGW

conquest_SOURCES = conquest.c c_defs.h defs.h conqdef.h conqcom.h context.h \
	conqreplay.c

conquest_LDADD = libUiCU.la libUi.la libconqclnt.la libconquest.la libconqinit.la $(LEXLIB) $(YACCLIB)

if ISMINGW
conquestgl_SOURCES = conquestgl.c c_defs.h defs.h conqdef.h conqcom.h \
	context.h icon.rc
else !ISMINGW
conquestgl_SOURCES = conquestgl.c c_defs.h defs.h conqdef.h conqcom.h \
	context.h
endif !ISMINGW

conquestgl_LDADD = libUiGL.la libUi.la libconqclnt.la libcqsound.la \
	libconquest.la libconqinit.la $(XLIBS) $(LEXLIB) $(YACCLIB)

if TCPW
conquestd_SOURCES = conquestd.c c_defs.h defs.h conqdef.h conqcom.h \
	context.h conqnet.h $(TCPW_H) $(TCPW_C) 
conquestd_LDADD = libconqserv.la libconquest.la libconqinit.la $(LEXLIB) $(YACCLIB) $(TCPWLIB) $(NSLLIB)
else
conquestd_SOURCES = conquestd.c c_defs.h defs.h conqdef.h conqcom.h \
	context.h conqnet.h $(TCPW_H)
conquestd_LDADD = libconqserv.la libconquest.la libconqinit.la $(LEXLIB) $(YACCLIB)
endif

conqai_SOURCES = conqai_main.c c_defs.h defs.h conqdef.h conqcom.h \
                 context.h conqdata.h

conqai_LDADD = libconquest.la libconqinit.la  $(LEXLIB) $(YACCLIB)

conqoper_SOURCES = conqoper.c c_defs.h defs.h conqdef.h conqcom.h context.h 
conqoper_LDADD = libUiCU.la libUi.la libconqclnt.la libconquest.la libconqinit.la $(LEXLIB) $(YACCLIB) 

conqdriv_SOURCES = conqdriv.c c_defs.h defs.h conqdef.h conqcom.h context.h
conqdriv_LDADD = libconquest.la libconqinit.la $(LEXLIB) $(YACCLIB)

conqstrat_SOURCES = conqstrat-yacc.y conqstrat-lex.l conqstrat.h
conqstrat_LDADD = libconquest.la $(LEXLIB) $(YACCLIB) 

conqinit_SOURCES = conqinit.c conqinit.h 
conqinit_LDADD = libconquest.la libconqinit.la $(LEXLIB) $(YACCLIB) 

if TCPW
conqmetad_SOURCES = conqmetad.c meta.h $(TCPW_H) $(TCPW_C) 
conqmetad_LDADD = libconquest.la libconqinit.la $(LEXLIB) $(YACCLIB) $(TCPWLIB) $(NSLLIB)
else
conqmetad_SOURCES = conqmetad.c meta.h $(TCPW_H) 
conqmetad_LDADD = libconquest.la libconqinit.la $(LEXLIB) $(YACCLIB) 
endif

# we install conqinitrc in the intall-exec-hook, so that we only install
#  conqinitrc if it does not already exist.
sysconf_DATA = texturesrc soundrc $(TRC) #$(SRC)

EXTRA_DIST = $(man6_MANS)

install-exec-hook:
	$(mkinstalldirs) -m 755 $(DESTDIR) $(DESTDIR)$(localstatedir) \
		$(DESTDIR)$(CONQSYSCONFDIR) \
		$(DESTDIR)$(CONQLOCALSTATEDIR) \
		$(DESTDIR)$(CONQDATADIR) \
		$(DESTDIR)$(CONQDATADIR)/doc \
		$(DESTDIR)$(CONQDATADIR)/doc/conquest \
		$(DESTDIR)$(CONQIMGDIR) \
		$(DESTDIR)$(CONQSNDDIR) \
		$(DESTDIR)$(bindir) \
		$(DESTDIR)$(libexecdir) \
		$(DESTDIR)/$(mandir) \
		$(DESTDIR)/$(mandir)/man6 
	chmod 775 $(DESTDIR)$(CONQLOCALSTATEDIR) 
	chgrp $(CONQGROUP) $(DESTDIR)$(CONQLOCALSTATEDIR)
	cp -f $(IMAGES) $(DESTDIR)$(CONQIMGDIR)/
	chmod 644 $(DESTDIR)$(CONQIMGDIR)/*.tga $(DESTDIR)$(CONQIMGDIR)/*.txf
	cp -f $(SOUNDS) $(DESTDIR)$(CONQSNDDIR)/
	chmod 644 $(DESTDIR)$(CONQSNDDIR)/*.ogg 
	cp -f $(CONQDOCS) $(DESTDIR)$(CONQDOCDIR)/
	chmod 644 $(DESTDIR)$(CONQDOCDIR)/*
	if [ ! -f $(DESTDIR)$(CONQSYSCONFDIR)/conqinitrc ] ; \
	then \
		cp conqinitrc $(DESTDIR)$(CONQSYSCONFDIR)/conqinitrc; \
	fi
	chgrp $(CONQGROUP) $(TEXTURESRC) $(CONQINITRC) $(SOUNDRC) 
	chmod 664 $(TEXTURESRC) $(CONQINITRC) $(SOUNDRC)
	chgrp $(CONQGROUP) $(DESTDIR)$(bindir)/conqoper$(EXEEXT) \
		$(DESTDIR)$(libexecdir)/conqdriv$(EXEEXT) \
		$(DESTDIR)$(bindir)/conquestd$(EXEEXT)
	chmod g+s $(DESTDIR)$(bindir)/conqoper$(EXEEXT) \
		$(DESTDIR)$(libexecdir)/conqdriv$(EXEEXT) \
		$(DESTDIR)$(bindir)/conquestd$(EXEEXT)
	-$(bindir)/conqoper$(EXEEXT) -C
	chmod 775 $(DESTDIR)$(CONQLOCALSTATEDIR)
	-chown root:$(CONQGROUP) $(DESTDIR)$(CONQLOCALSTATEDIR)

