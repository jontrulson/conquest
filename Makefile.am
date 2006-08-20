AUTOMAKE_OPTIONS = foreign

# User defineable options

CFLAGS = @CONQ_CFLAGS@
CONQGROUP = @CONQGROUP@
CONQLOCALSTATEDIR = @localstatedir@/conquest
CONQLIBEXECDIR = @libexecdir@
CONQDATADIR = @datadir@
CONQSYSCONFDIR = @sysconfdir@
CONQIMGDIR = $(CONQDATADIR)/img
CONQDOCDIR = $(CONQDATADIR)/doc/conquest

CONQDOCS = HISTORY README CONQINIT.TXT conqrule conqrule.easy

IMAGES = img/*.tga img/*.txf

man6_MANS = conqdriv.man conqmetad.man conqoper.man conquest.man \
	conquestd.man conquestsh.man conqai.man conqstrat.man \
	conqinit.man conquestgl.man

XLIBS = @X_LIBS@ @X_EXTRA_LIBS@ -lglut -lGL -lGLU -lXt @X_PRE_LIBS@ -lXext -lXi -lX11 -lXmu  

INCLUDES = -DCONQUEST_GROUP=\"$(CONQGROUP)\" \
	-DCONQSTATE=\"$(CONQLOCALSTATEDIR)\" -DCONQLIBEXEC=\"$(CONQLIBEXECDIR)\" \
	-DCONQSHARE=\"$(CONQDATADIR)\" -DCONQETC=\"$(CONQSYSCONFDIR)\" \
	-DCONQUESTSH_EXEC=\"$(DESTDIR)$(bindir)/conquest$(EXEEXT)\" \
	-DCONQDOCDIR=\"$(CONQDATADIR)\" \
	@X_CFLAGS@

AM_YFLAGS = -d

if GLCLIENT
lib_LTLIBRARIES = libconquest.la libconqclnt.la libconqserv.la \
	libconqinit.la libUi.la libUiGL.la libUiCU.la 
else
lib_LTLIBRARIES = libconquest.la libconqclnt.la libconqserv.la \
	libconqinit.la libUi.la libUiCU.la
endif

libconquest_la_SOURCES = conqai.c ratfor.c  \
	conqlb.h conqlb.c rndlb.c conqutil.c conqcm.c \
	conqsvr42.c global.c ibuf.c version.c conf.c sem.c color.c \
	userauth.c record.c color.h user.h shiptype.h \
	ship.h planet.h team.h msg.h doomsday.h driver.h conqinfo.h \
	history.h robot.h global.h torp.h ibuf.h conf.h  display.h \
	protocol.h packet.h packet.c userauth.h meta.c meta.h udp.c udp.h

libconquest_la_LIBADD = @LTLIBOBJS@

libconquest_la_LDFLAGS = -version-info 1:0:0

libconqclnt_la_SOURCES = client.c client.h clientlb.c clientlb.h \
	playback.h playback.c

libconqclnt_la_LDFLAGS = -version-info 1:0:0

libconqserv_la_SOURCES = servauth.c server.c servauth.h server.h serverpkt.c \
	serverpkt.h servercmd.h servercmd.c

libconqserv_la_LDFLAGS = -version-info 1:0:0

libUi_la_SOURCES = ui.h uimisc.c

libUi_la_LDFLAGS = -version-info 1:0:0

libUiGL_la_SOURCES = GL.c texfont.c glfont.c glfont.h \
	glmisc.c glmisc.h nConsvr.c nConsvr.h nMeta.h nMeta.c cqkeys.h \
        prm.h prm.c nAuth.h nAuth.c nWelcome.h nWelcome.c nMenu.h nMenu.c \
	nPlay.h nPlay.c nCP.h nCP.c nDead.h nDead.c nCPHelp.h nCPHelp.c \
	nShipl.h nShipl.c node.h node.c nPlanetl.h nPlanetl.c nTeaml.h \
	nTeaml.c nUserl.h nUserl.c nHistl.h nHistl.c cprintf.h glcprintf.c \
	gldisplay.h gldisplay.c disputil.h render.h render.c \
	nPlayBMenu.h nPlayBMenu.c nPlayB.h nPlayB.c nPlayBHelp.h \
	nPlayBHelp.c nOptions.h nOptions.c colorGL.c ui.h anim.c anim.h \
	GL.h

libUiGL_la_LDFLAGS = -version-info 1:0:0

libUiCU_la_SOURCES = display.c cprintf.h cprintf.c disputil.h colorCU.c \
	cd2lb.h cd2lb.c iolb.h iolb.c cumisc.h cumisc.c cuclient.h \
	cuclient.c options.h options.c clntauth.c ui.h display.h 

libUiCU_la_LDFLAGS = -version-info 1:0:0

libconqinit_la_SOURCES = conqinit-yacc.y conqinit-lex.l conqinit.h initdata.h planinit.c

libconqinit_la_LDFLAGS = -version-info 1:0:0

if GLCLIENT
bin_PROGRAMS = conquest conquestgl conqai conqoper conqstrat \
	conquestd conquestsh conqmetad conqinit
else
bin_PROGRAMS = conquest conqai conqoper conqstrat \
	conquestd conquestsh conqmetad conqinit
endif

libexec_PROGRAMS = conqdriv

conquest_SOURCES = conquest.c c_defs.h defs.h conqdef.h conqcom.h context.h \
	conqreplay.c

conquest_LDADD = libUiCU.la libUi.la libconqclnt.la libconquest.la libconqinit.la $(LEXLIB) $(YACCLIB)

conquestgl_SOURCES = conquestgl.c c_defs.h defs.h conqdef.h conqcom.h context.h
conquestgl_LDADD = libUiGL.la libUi.la libconqclnt.la libconquest.la libconqinit.la $(XLIBS) $(LEXLIB) $(YACCLIB)

conquestd_SOURCES = conquestd.c c_defs.h defs.h conqdef.h conqcom.h \
	context.h conqnet.h 
conquestd_LDADD = libconqserv.la libconquest.la libconqinit.la $(LEXLIB) $(YACCLIB) 

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

conquestsh_SOURCES = conquestsh.c

conqmetad_SOURCES = conqmetad.c meta.h
conqmetad_LDADD = libconquest.la libconqinit.la $(LEXLIB) $(YACCLIB)

# we install conqinitrc in the intall-exec-hook, so that we only install
#  conqinitrc if it does not already exist.
sysconf_DATA = texturesrc

EXTRA_DIST = $(man6_MANS)

install-exec-hook:
	$(mkinstalldirs) -m 755 $(DESTDIR)$(localstatedir) \
		$(DESTDIR)$(CONQSYSCONFDIR) \
		$(DESTDIR)$(CONQLOCALSTATEDIR) \
		$(DESTDIR)$(CONQDATADIR) \
		$(DESTDIR)$(CONQDATADIR)/doc \
		$(DESTDIR)$(CONQDATADIR)/doc/conquest \
		$(DESTDIR)$(CONQIMGDIR) \
		$(DESTDIR)$(bindir) \
		$(DESTDIR)$(libexecdir) \
		$(DESTDIR)$(libdir) \
		$(DESTDIR)/$(mandir) \
		$(DESTDIR)/$(mandir)/man6 
	chmod 775 $(DESTDIR)$(CONQLOCALSTATEDIR) 
	chgrp $(CONQGROUP) $(DESTDIR)$(CONQLOCALSTATEDIR)
	cp -f $(IMAGES) $(DESTDIR)$(CONQIMGDIR)/
	chmod 644 $(DESTDIR)$(CONQIMGDIR)/*.tga $(DESTDIR)$(CONQIMGDIR)/*.txf
	cp -f $(CONQDOCS) $(DESTDIR)$(CONQDOCDIR)/
	chmod 644 $(DESTDIR)$(CONQDOCDIR)/*
	if [ ! -f $(DESTDIR)$(CONQSYSCONFDIR)/conqinitrc ] ; \
	then \
		cp conqinitrc $(DESTDIR)$(CONQSYSCONFDIR)/conqinitrc; \
	fi
	chgrp $(CONQGROUP) $(DESTDIR)$(CONQSYSCONFDIR)/*rc
	chmod 664 $(DESTDIR)$(CONQSYSCONFDIR)/*rc
	chmod 755 $(prefix) $(DESTDIR)$(datadir)
	chgrp $(CONQGROUP) $(DESTDIR)$(bindir)/conqoper$(EXEEXT) \
		$(DESTDIR)$(libexecdir)/conqdriv$(EXEEXT) \
		$(DESTDIR)$(bindir)/conquestd$(EXEEXT)
	chmod g+s $(DESTDIR)$(bindir)/conqoper$(EXEEXT) \
		$(DESTDIR)$(libexecdir)/conqdriv$(EXEEXT) \
		$(DESTDIR)$(bindir)/conquestd$(EXEEXT)
	-$(bindir)/conqoper$(EXEEXT) -C
	chmod 775 $(DESTDIR)$(CONQLOCALSTATEDIR)
	-chown root:$(CONQGROUP) $(DESTDIR)$(CONQLOCALSTATEDIR)
	$(FILEPRIV) $(DESTDIR)$(bindir)/conqoper$(EXEEXT) \
		$(DESTDIR)$(libexecdir)/conqdriv$(EXEEXT) \
		$(DESTDIR)$(bindir)/conquestd$(EXEEXT)

