AUTOMAKE_OPTIONS = foreign
ACLOCAL_AMFLAGS = -I m4 --install


# User defineable options

.rc.o:
	@target@-windres $^ -o $@

CXXFLAGS = @CONQ_CFLAGS@ $(SDL_CFLAGS)
CONQGROUP = @CONQGROUP@
CONQLOCALSTATEDIR = @localstatedir@/conquest
CONQLIBEXECDIR = @libexecdir@
CONQDATADIR = @datadir@
CONQSYSCONFDIR = @sysconfdir@/conquest
CONQIMGDIR = $(CONQDATADIR)/conquest/img
CONQSNDDIR = $(CONQDATADIR)/conquest/sound
CONQDOCDIR = $(CONQDATADIR)/doc/conquest

CONQDOCS = ../docs/HISTORY.txt ../docs/conquest-guide.md ../docs/conqinit.txt \
	../robot-rules/conqrule ../robot-rules/conqrule.easy \
	../docs/server-guide.md ../LICENSE.txt ../docs/CREDITS.txt ../README.md

IMAGES = ../img/*.png ../img/*.txf
SOUNDS = ../sound/*.ogg

TCPW_H = tcpwrap.h
TCPW_C = tcpwrap.cxx

man6_MANS = ../man/conqdriv.man ../man/conqmetad.man ../man/conqoper.man \
	../man/conquest.man ../man/conquestd.man ../man/conqai.man \
	../man/conqstrat.man ../man/conqinit.man

if ISMINGW

XLIBS = -lfreeglut -lopengl32 -lglu32 $(SDL_LIBS) -lws2_32
#-lwsock32

else !ISMINGW

XLIBS = @X_LIBS@ @X_EXTRA_LIBS@ -lglut -lGL -lGLU -lXt $(SDL_LIBS) @X_PRE_LIBS@ -lXext -lXi -lX11 -lXmu

endif !ISMINGW

AM_CPPFLAGS = -DCONQUEST_GROUP=\"$(CONQGROUP)\" \
	-DCONQSTATE=\"$(CONQLOCALSTATEDIR)\" -DCONQLIBEXEC=\"$(CONQLIBEXECDIR)\" \
	-DCONQSHARE=\"$(CONQDATADIR)/conquest\" -DCONQETC=\"$(CONQSYSCONFDIR)\" \
	-DCONQDOCDIR=\"$(CONQDATADIR)\" \
	@X_CFLAGS@

AM_YFLAGS = -d

if ISMINGW
noinst_LTLIBRARIES = libconquest.la libconqclnt.la libconqserv.la \
	libconqinit.la libUi.la libUiGL.la libcqsound.la libfmt.la
else !ISMINGW

if GLCLIENT
noinst_LTLIBRARIES = libconquest.la libconqclnt.la libconqserv.la \
	libconqinit.la libUi.la libUiGL.la libcqsound.la libfmt.la
else !GLCLIENT
noinst_LTLIBRARIES = libconquest.la libconqclnt.la libconqserv.la \
	libconqinit.la libUi.la libUiCU.la libfmt.la
endif !GLCLIENT

endif !ISMINGW

libfmt_la_SOURCES = format.h format.cxx

libconquest_la_SOURCES = conqai.cxx conqai.h cb.cxx cb.h \
	conqlb.h conqlb.cxx rndlb.cxx rndlb.h conqutil.cxx conqutil.h \
	conqunix.cxx conqunix.h ibuf.cxx version.cxx conf.cxx sem.cxx color.cxx \
	userauth.cxx record.cxx color.h user.h shiptype.h \
	ship.h planet.h team.h msg.h doomsday.h driver.h conqinfo.h \
	history.h robot.h global.h torp.h ibuf.h conf.h  display.h \
	protocol.h packet.h packet.cxx userauth.h meta.cxx meta.h udp.cxx udp.h \
	rb.h rb.cxx cbMaxLimits.h proto_0006.h

libconquest_la_LIBADD = @LTLIBOBJS@

libconqclnt_la_SOURCES = client.cxx client.h cproc.h cproc.cxx clientlb.cxx \
	clientlb.h playback.h playback.cxx cproc_0006.h cproc_0006.cxx \
	ping.cxx ping.h

libconqserv_la_SOURCES = servauth.cxx server.cxx servauth.h server.h serverpkt.cxx \
	serverpkt.h servercmd.h servercmd.cxx sproc.h sproc.cxx

libUi_la_SOURCES = ui.h uimisc.cxx

libUiGL_la_SOURCES = GL.cxx texfont.cxx glfont.cxx glfont.h \
	glmisc.cxx glmisc.h nConsvr.cxx nConsvr.h nMeta.h nMeta.cxx cqkeys.h \
        prm.h prm.cxx nAuth.h nAuth.cxx nWelcome.h nWelcome.cxx nMenu.h nMenu.cxx \
	nPlay.h nPlay.cxx nCP.h nCP.cxx nDead.h nDead.cxx nCPHelp.h nCPHelp.cxx \
	nShipl.h nShipl.cxx node.h node.cxx nPlanetl.h nPlanetl.cxx nTeaml.h \
	nTeaml.cxx nUserl.h nUserl.cxx nHistl.h nHistl.cxx cprintf.h glcprintf.cxx \
	gldisplay.h gldisplay.cxx disputil.h render.h render.cxx \
	nPlayBMenu.h nPlayBMenu.cxx nPlayB.h nPlayB.cxx nPlayBHelp.h \
	nPlayBHelp.cxx nOptions.h nOptions.cxx colorGL.cxx ui.h anim.cxx anim.h \
	GL.h hud.h hud.cxx blinker.h blinker.cxx

libUiCU_la_SOURCES = display.cxx cprintf.h cprintf.cxx disputil.h colorCU.cxx \
	cd2lb.h cd2lb.cxx iolb.h iolb.cxx cumisc.h cumisc.cxx cuclient.h \
	cuclient.cxx options.h options.cxx clntauth.cxx ui.h display.h

libconqinit_la_SOURCES = conqinit-yacc.yxx conqinit-lex.lxx conqinit.h planinit.cxx

libcqsound_la_SOURCES = cqsound.cxx cqsound.h

# for now, conquest is the only program we build for windows

if ISMINGW

bin_PROGRAMS = conquest

else !ISMINGW

libexec_PROGRAMS = conqdriv

if GLCLIENT

bin_PROGRAMS = conquest conqoper conqai conqstrat \
	conquestd conqmetad conqinit
else !GLCLIENT

bin_PROGRAMS = conqoper conqai conqstrat \
	conquestd conqmetad conqinit

endif !GLCLIENT

endif !ISMINGW

if ISMINGW
conquest_SOURCES = conquest.cxx c_defs.h defs.h conqdef.h cb.h \
	context.h icon.rc
else !ISMINGW
conquest_SOURCES = conquest.cxx c_defs.h defs.h conqdef.h cb.h \
	context.h
endif !ISMINGW

conquest_LDADD = libUiGL.la libUi.la libconqclnt.la libcqsound.la \
	libconquest.la libconqinit.la $(XLIBS) $(LEXLIB) $(YACCLIB)

if TCPW
conquestd_SOURCES = conquestd.cxx c_defs.h defs.h conqdef.h cb.h \
	context.h conqnet.h $(TCPW_H) $(TCPW_C)
conquestd_LDADD = libconqserv.la libconquest.la libconqinit.la $(LEXLIB) $(YACCLIB) $(TCPWLIB) $(NSLLIB)
else
conquestd_SOURCES = conquestd.cxx c_defs.h defs.h conqdef.h cb.h \
	context.h conqnet.h $(TCPW_H)
conquestd_LDADD = libconqserv.la libconquest.la libconqinit.la $(LEXLIB) $(YACCLIB)
endif

conqai_SOURCES = conqai_main.cxx c_defs.h defs.h conqdef.h cb.h \
                 context.h conqdata.h

conqai_LDADD = libconquest.la libconqinit.la libfmt.la $(LEXLIB) $(YACCLIB)

conqoper_SOURCES = conqoper.cxx c_defs.h defs.h conqdef.h cb.h context.h
conqoper_LDADD = libUiCU.la libUi.la libconqclnt.la libconquest.la libconqinit.la libfmt.la $(LEXLIB) $(YACCLIB)

conqdriv_SOURCES = conqdriv.cxx c_defs.h defs.h conqdef.h cb.h context.h
conqdriv_LDADD = libconquest.la libconqinit.la libfmt.la $(LEXLIB) $(YACCLIB)

# we are very careful here not to depend on or use any conquest
# libraries with conqstrat.
conqstrat_SOURCES = conqstrat-yacc.yxx conqstrat-lex.lxx conqstrat.h
conqstrat_LDADD = libfmt.la $(LEXLIB) $(YACCLIB)

conqinit_SOURCES = conqinit.cxx conqinit.h
conqinit_LDADD = libconquest.la libconqinit.la libfmt.la $(LEXLIB) $(YACCLIB)

if TCPW
conqmetad_SOURCES = conqmetad.cxx meta.h $(TCPW_H) $(TCPW_C)
conqmetad_LDADD = libconquest.la libconqinit.la libfmt.la $(LEXLIB) $(YACCLIB) $(TCPWLIB) $(NSLLIB)
else
conqmetad_SOURCES = conqmetad.cxx meta.h $(TCPW_H)
conqmetad_LDADD = libconquest.la libconqinit.la libfmt.la $(LEXLIB) $(YACCLIB)
endif

EXTRA_DIST = $(man6_MANS)

install-exec-hook:
	$(mkinstalldirs) -m 755 $(DESTDIR) $(DESTDIR)$(localstatedir) \
		$(DESTDIR)$(CONQSYSCONFDIR) \
		$(DESTDIR)$(CONQLOCALSTATEDIR) \
		$(DESTDIR)$(CONQDATADIR) \
		$(DESTDIR)$(CONQDATADIR)/doc \
		$(DESTDIR)$(CONQDATADIR)/doc/conquest \
		$(DESTDIR)$(CONQDATADIR)/conquest \
		$(DESTDIR)$(CONQIMGDIR) \
		$(DESTDIR)$(CONQSNDDIR) \
		$(DESTDIR)$(bindir) \
		$(DESTDIR)$(libexecdir) \
		$(DESTDIR)$(mandir) \
		$(DESTDIR)$(mandir)/man6
	chmod 775 $(DESTDIR)$(CONQLOCALSTATEDIR)
	chgrp $(CONQGROUP) $(DESTDIR)$(CONQLOCALSTATEDIR)
	cp -f $(IMAGES) $(DESTDIR)$(CONQIMGDIR)/
	chmod 644 $(DESTDIR)$(CONQIMGDIR)/*.png $(DESTDIR)$(CONQIMGDIR)/*.txf
	cp -f $(SOUNDS) $(DESTDIR)$(CONQSNDDIR)/
	chmod 644 $(DESTDIR)$(CONQSNDDIR)/*.ogg
	cp -f $(CONQDOCS) $(DESTDIR)$(CONQDOCDIR)/
	chmod 644 $(DESTDIR)$(CONQDOCDIR)/*
	cp ../initfiles/conqinitrc $(DESTDIR)$(CONQSYSCONFDIR)/conqinitrc
	chgrp $(CONQGROUP) $(DESTDIR)$(CONQSYSCONFDIR)/conqinitrc
	chmod 664 $(DESTDIR)$(CONQSYSCONFDIR)/conqinitrc
	cp ../initfiles/texturesrc $(DESTDIR)$(CONQSYSCONFDIR)/texturesrc
	chgrp $(CONQGROUP) $(DESTDIR)$(CONQSYSCONFDIR)/texturesrc
	chmod 664 $(DESTDIR)$(CONQSYSCONFDIR)/texturesrc
	cp ../initfiles/soundrc $(DESTDIR)$(CONQSYSCONFDIR)/soundrc
	chgrp $(CONQGROUP) $(DESTDIR)$(CONQSYSCONFDIR)/soundrc
	chmod 664 $(DESTDIR)$(CONQSYSCONFDIR)/soundrc
	cp ../initfiles/00_plantex.trc $(DESTDIR)$(CONQSYSCONFDIR)/00_plantex.trc
	chgrp $(CONQGROUP) $(DESTDIR)$(CONQSYSCONFDIR)/00_plantex.trc
	chmod 664 $(DESTDIR)$(CONQSYSCONFDIR)/00_plantex.trc
	chgrp $(CONQGROUP) $(DESTDIR)$(bindir)/conqoper$(EXEEXT) \
		$(DESTDIR)$(libexecdir)/conqdriv$(EXEEXT) \
		$(DESTDIR)$(bindir)/conqmetad$(EXEEXT) \
		$(DESTDIR)$(bindir)/conquestd$(EXEEXT)
	chmod g+s $(DESTDIR)$(bindir)/conqoper$(EXEEXT) \
		$(DESTDIR)$(libexecdir)/conqdriv$(EXEEXT) \
		$(DESTDIR)$(bindir)/conqmetad$(EXEEXT) \
		$(DESTDIR)$(bindir)/conquestd$(EXEEXT)
	-$(bindir)/conqoper$(EXEEXT) -C
	-chown root:$(CONQGROUP) $(DESTDIR)$(CONQLOCALSTATEDIR)
