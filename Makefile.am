AUTOMAKE_OPTIONS = foreign

# User defineable options

CFLAGS = -Wall -D__NO_MATH_INLINES -fPIC -fno-builtin
CONQGROUP = @CONQGROUP@
CONQLOCALSTATEDIR = @localstatedir@/conquest
CONQLIBEXECDIR = @libexecdir@
CONQDATADIR = @datadir@
CONQSYSCONFDIR = @sysconfdir@
CONQIMGDIR = $(CONQDATADIR)/img
#IMAGES = img/classd.bmp img/classm.bmp img/doomsday.bmp img/explode.bmp \
#	img/shipfcr.bmp img/shipfde.bmp img/shipfsc.bmp img/shipkcr.bmp \
#	img/shipkde.bmp img/shipksc.bmp img/shipocr.bmp img/shipode.bmp \
#	img/shiposc.bmp img/shiprcr.bmp img/shiprde.bmp img/shiprsc.bmp \
#	img/star.bmp


#XLIBS = @X_LIBS@ @X_EXTRA_LIBS@ -lGLw -lGL -lGLU -lXm -lXp -lXt @X_PRE_LIBS@ -lX11 -lXmu  

INCLUDES = -DCONQUEST_GROUP=\"$(CONQGROUP)\" \
	-DCONQSTATE=\"$(CONQLOCALSTATEDIR)\" -DCONQLIBEXEC=\"$(CONQLIBEXECDIR)\" \
	-DCONQSHARE=\"$(CONQDATADIR)\" -DCONQETC=\"$(CONQSYSCONFDIR)\" \
	-DCONQUESTSH_EXEC=\"$(DESTDIR)$(bindir)/conquest\"

#@X_CFLAGS@

AM_YFLAGS = -d

lib_LTLIBRARIES = libconquest.la libconqclnt.la libconqserv.la
libconquest_la_SOURCES = conqai.c ratfor.c cd2lb.c display.c iolb.c  \
	conqlb.c rndlb.c conqmisc.c conqutil.c conqcm.c \
	conqsvr42.c global.c ibuf.c version.c conf.c sem.c color.c \
	userauth.c record.c color.h user.h shiptype.h \
	ship.h planet.h team.h msg.h doomsday.h driver.h conqinfo.h \
	history.h robot.h global.h torp.h ibuf.h conf.h  display.h \
	protocol.h packet.h packet.c userauth.h meta.c meta.h

libconquest_la_LDFLAGS = -version-info 1:0:0

libconqclnt_la_SOURCES = clntauth.c client.c client.h clientlb.c clientlb.h options.c 

libconqclnt_la_LDFLAGS = -version-info 1:0:0

libconqserv_la_SOURCES = servauth.c server.c servauth.h server.h serverpkt.c \
	serverpkt.h servercmd.h servercmd.c

libconqserv_la_LDFLAGS = -version-info 1:0:0

bin_PROGRAMS = conquest conqai conqoper conqstrat conqreplay conquestd \
	conquestsh conqmetad

libexec_PROGRAMS = conqdriv
          
conquest_SOURCES = conquest.c c_defs.h defs.h conqdef.h conqcom.h context.h
conquest_LDADD = libconqclnt.la libconquest.la $(XLIBS)

conquestd_SOURCES = conquestd.c c_defs.h defs.h conqdef.h conqcom.h \
	context.h conqnet.h
conquestd_LDADD = libconqserv.la libconquest.la $(XLIBS)

conqai_SOURCES = conqai_main.c c_defs.h defs.h conqdef.h conqcom.h \
                 context.h conqdata.h

conqai_LDADD = libconquest.la $(XLIBS)

conqoper_SOURCES = conqoper.c c_defs.h defs.h conqdef.h conqcom.h context.h
conqoper_LDADD = libconqclnt.la libconquest.la $(XLIBS)

conqdriv_SOURCES = conqdriv.c c_defs.h defs.h conqdef.h conqcom.h context.h
conqdriv_LDADD = libconquest.la $(XLIBS)

conqstrat_SOURCES = conqstrat-yacc.y conqstrat-lex.l conqstrat.h
conqstrat_LDADD = libconquest.la $(LEXLIB) $(YACCLIB) $(XLIBS)

conqreplay_SOURCES = conqreplay.c c_defs.h defs.h conqdef.h conqcom.h context.h record.h
conqreplay_LDADD = libconqclnt.la libconquest.la $(XLIBS)

conquestsh_SOURCES = conquestsh.c

conqmetad_SOURCES = conqmetad.c meta.h
conqmetad_LDADD = libconquest.la

data_DATA = conquest.doc conqnews.doc README conqrule 
#sysconf_DATA = conquestrc

EXTRA_DIST = conquest.doc conqnews.doc README conqrule # conquestrc

install-exec-hook:
	$(mkinstalldirs) -m 755 $(localstatedir) $(CONQSYSCONFDIR) $(CONQLOCALSTATEDIR) $(CONQIMGDIR)
	chmod 775 $(CONQLOCALSTATEDIR) 
	chgrp $(CONQGROUP) $(CONQLOCALSTATEDIR)
#	cp -f $(IMAGES) $(CONQIMGDIR)/
#	chmod 644 $(CONQIMGDIR)/*.bmp
	chmod 755 $(prefix) $(datadir)
	chgrp $(CONQGROUP) $(DESTDIR)$(bindir)/conqoper \
		$(DESTDIR)$(libexecdir)/conqdriv \
		$(DESTDIR)$(bindir)/conquestd
	chmod g+s $(DESTDIR)$(bindir)/conqoper \
		$(DESTDIR)$(libexecdir)/conqdriv \
		$(DESTDIR)$(bindir)/conquestd
	$(DESTDIR)$(bindir)/conqoper -C
	chmod 775 $(CONQLOCALSTATEDIR)
	chown root:$(CONQGROUP) $(CONQLOCALSTATEDIR)
	$(FILEPRIV) $(DESTDIR)$(bindir)/conqoper \
		$(DESTDIR)$(libexecdir)/conqdriv \
		$(DESTDIR)$(bindir)/conquestd

