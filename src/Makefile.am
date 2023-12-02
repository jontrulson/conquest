ACLOCAL_AMFLAGS = -I m4 --install
SUBDIRS = fmt

# User defineable options

.rc.o:
	@target@-windres $^ -o $@

CXXFLAGS = @CONQ_CFLAGS@ $(SDL_CFLAGS)
CONQGROUP = @CONQGROUP@
CONQLOCALSTATEDIR = @localstatedir@/lib/conquest
CONQLIBEXECDIR = @libexecdir@
CONQDATADIR = @datadir@
CONQSYSCONFDIR = @sysconfdir@/conquest
CONQIMGDIR = $(CONQDATADIR)/conquest/img
CONQSNDDIR = $(CONQDATADIR)/conquest/sound
CONQDOCDIR = $(CONQDATADIR)/doc/conquest

CONQDOCS = $(top_srcdir)/docs/HISTORY.txt $(top_srcdir)/docs/conquest-guide.md $(top_srcdir)/docs/conqinit.txt \
	$(top_srcdir)/robot-rules/conqrule $(top_srcdir)/robot-rules/conqrule.easy \
	$(top_srcdir)/docs/server-guide.md $(top_srcdir)/LICENSE.txt $(top_srcdir)/docs/CREDITS.txt $(top_srcdir)/README.md

IMAGES = $(top_srcdir)/img/*.png $(top_srcdir)/img/*.txf
SOUNDS = $(top_srcdir)/sound/*.ogg

man6_MANS = $(top_srcdir)/man/conqdriv.man $(top_srcdir)/man/conqmetad.man $(top_srcdir)/man/conqoper.man \
	$(top_srcdir)/man/conquest.man $(top_srcdir)/man/conquestd.man $(top_srcdir)/man/conqai.man \
	$(top_srcdir)/man/conqstrat.man $(top_srcdir)/man/conqinit.man

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
	libconqinit.la libUi.la libUiGL.la libcqsound.la
else !ISMINGW

if GLCLIENT
noinst_LTLIBRARIES = libconquest.la libconqclnt.la libconqserv.la \
	libconqinit.la libUi.la libUiCU.la libUiGL.la libcqsound.la
else !GLCLIENT
noinst_LTLIBRARIES = libconquest.la libconqclnt.la libconqserv.la \
	libconqinit.la libUi.la libUiCU.la
endif !GLCLIENT

endif !ISMINGW

libconquest_la_SOURCES = conqai.cxx conqai.h cb.cxx cb.h \
	conqlb.h conqlb.cxx rndlb.cxx rndlb.h conqutil.cxx conqutil.h \
	conqunix.cxx conqunix.h ibuf.cxx version.cxx conf.cxx sem.cxx \
	color.cxx \
	userauth.cxx record.cxx color.h user.h shiptype.h \
	ship.h planet.h team.h msg.h doomsday.h driver.h conqinfo.h \
	history.h history.cxx robot.h global.h global.cxx torp.h ibuf.h \
	conf.h  display.h \
	protocol.h packet.h packet.cxx userauth.h meta.cxx meta.h \
	udp.cxx udp.h rb.h rb.cxx cbMaxLimits.h proto_0006.h \
	c_defs.h defs.h conqdef.h cb.h context.h context.cxx conqnet.h \
	hud.h hud.cxx info.h info.cxx initvec.h

libconquest_la_LIBADD = @LTLIBOBJS@

libconqclnt_la_SOURCES = client.cxx client.h cproc.h cproc.cxx clientlb.cxx \
	clientlb.h playback.h playback.cxx cproc_0006.h cproc_0006.cxx \
	ping.cxx ping.h

libconqserv_la_SOURCES = servauth.cxx server.cxx servauth.h server.h serverpkt.cxx \
	serverpkt.h servercmd.h servercmd.cxx sproc.h sproc.cxx initvec.h

libUi_la_SOURCES = ui.h ui.cxx

libUiGL_la_SOURCES = GL.cxx texfont.cxx glfont.cxx glfont.h \
	glmisc.cxx glmisc.h nConsvr.cxx nConsvr.h nMeta.h nMeta.cxx cqkeys.h \
        prm.h prm.cxx nAuth.h nAuth.cxx nWelcome.h nWelcome.cxx nMenu.h nMenu.cxx \
	nPlay.h nPlay.cxx nCP.h nCP.cxx nDead.h nDead.cxx nCPHelp.h nCPHelp.cxx \
	nShipl.h nShipl.cxx node.h node.cxx nPlanetl.h nPlanetl.cxx nTeaml.h \
	nTeaml.cxx nUserl.h nUserl.cxx nHistl.h nHistl.cxx cprintf.h glcprintf.cxx \
	gldisplay.h gldisplay.cxx disputil.h render.h render.cxx \
	nPlayBMenu.h nPlayBMenu.cxx nPlayB.h nPlayB.cxx nPlayBHelp.h \
	nPlayBHelp.cxx nOptions.h nOptions.cxx colorGL.cxx ui.h anim.cxx anim.h \
	GL.h blinker.h blinker.cxx textures.h textures.cxx \
	uiGL.cxx initvec.h

libUiCU_la_SOURCES = display.cxx cprintf.h cprintf.cxx disputil.h colorCU.cxx \
	cd2lb.h cd2lb.cxx iolb.h iolb.cxx cumisc.h cumisc.cxx cuclient.h \
	cuclient.cxx options.h options.cxx clntauth.cxx ui.h display.h \
	uiCU.cxx

libconqinit_la_SOURCES = conqinit-yacc.yxx conqinit-lex.lxx conqinit.h planinit.cxx

libcqsound_la_SOURCES = cqsound.cxx cqsound.h cqsSetWarp.cxx

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
conquest_SOURCES = conquest.cxx icon.rc
else !ISMINGW
conquest_SOURCES = conquest.cxx
endif !ISMINGW

conquest_LDADD = libUiGL.la libUi.la libconqclnt.la libcqsound.la \
	libconquest.la libconqinit.la fmt/libfmt.la $(XLIBS) $(LEXLIB) \
	$(YACCLIB)

conquestd_SOURCES = conquestd.cxx
conquestd_LDADD = libconqserv.la libconquest.la libconqinit.la fmt/libfmt.la \
	$(LEXLIB) $(YACCLIB)

conqai_SOURCES = conqai_main.cxx

conqai_LDADD = libconquest.la libconqinit.la fmt/libfmt.la $(LEXLIB) $(YACCLIB)

conqoper_SOURCES = conqoper.cxx
conqoper_LDADD = libUiCU.la libUi.la libconqclnt.la libconquest.la \
	libconqinit.la fmt/libfmt.la $(LEXLIB) $(YACCLIB)

conqdriv_SOURCES = conqdriv.cxx
conqdriv_LDADD = libconquest.la libconqinit.la fmt/libfmt.la $(LEXLIB) \
	$(YACCLIB)

# we are very careful here not to depend on or use any conquest
# libraries with conqstrat.
conqstrat_SOURCES = conqstrat-yacc.yxx conqstrat-lex.lxx conqstrat.h
conqstrat_LDADD = fmt/libfmt.la $(LEXLIB) $(YACCLIB)

conqinit_SOURCES = conqinit.cxx conqinit.h
conqinit_LDADD = libconquest.la libconqinit.la fmt/libfmt.la $(LEXLIB) \
	$(YACCLIB)

conqmetad_SOURCES = conqmetad.cxx meta.h
conqmetad_LDADD = libconquest.la libconqinit.la fmt/libfmt.la $(LEXLIB) \
	$(YACCLIB)

EXTRA_DIST = $(man6_MANS)

# clean out the yacc/bison flex/lex generated files
CLEANFILES = conqinit-lex.cxx \
 conqinit-yacc.cxx \
 conqinit-yacc.hxx \
 conqstrat-lex.cxx \
 conqstrat-yacc.cxx \
 conqstrat-yacc.hxx

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
	cp $(top_srcdir)/initfiles/conqinitrc $(DESTDIR)$(CONQSYSCONFDIR)/conqinitrc
	chgrp $(CONQGROUP) $(DESTDIR)$(CONQSYSCONFDIR)/conqinitrc
	chmod 664 $(DESTDIR)$(CONQSYSCONFDIR)/conqinitrc
	cp $(top_srcdir)/initfiles/texturesrc $(DESTDIR)$(CONQSYSCONFDIR)/texturesrc
	chgrp $(CONQGROUP) $(DESTDIR)$(CONQSYSCONFDIR)/texturesrc
	chmod 664 $(DESTDIR)$(CONQSYSCONFDIR)/texturesrc
	cp $(top_srcdir)/initfiles/soundrc $(DESTDIR)$(CONQSYSCONFDIR)/soundrc
	chgrp $(CONQGROUP) $(DESTDIR)$(CONQSYSCONFDIR)/soundrc
	chmod 664 $(DESTDIR)$(CONQSYSCONFDIR)/soundrc
	cp $(top_srcdir)/initfiles/00_plantex.trc $(DESTDIR)$(CONQSYSCONFDIR)/00_plantex.trc
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
