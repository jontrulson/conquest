AUTOMAKE_OPTIONS = foreign

# User defineable options
CONQGROUP = @CONQGROUP@
CONQLOCALSTATEDIR = @localstatedir@/conquest
CONQLIBEXECDIR = @libexecdir@
CONQDATADIR = @datadir@
CONQSYSCONFDIR = @sysconfdir@

INCLUDES = -DCONQUEST_GROUP=\"$(CONQGROUP)\" \
	-DCONQSTATE=\"$(CONQLOCALSTATEDIR)\" -DCONQLIBEXEC=\"$(CONQLIBEXECDIR)\" \
	-DCONQSHARE=\"$(CONQDATADIR)\" -DCONQETC=\"$(CONQSYSCONFDIR)\"

AM_YFLAGS = -d

lib_LTLIBRARIES = libconquest.la
libconquest_la_SOURCES = conqai.c ratfor.c cd2lb.c display.c iolb.c  \
	conqlb.c rndlb.c conqmisc.c conqutil.c conqcm.c \
	conqsvr42.c global.c ibuf.c version.c conf.c sem.c color.c \
	userauth.c options.c record.c color.h user.h shiptype.h \
	ship.h planet.h team.h msg.h doomsday.h driver.h conqinfo.h \
	history.h robot.h global.h torp.h ibuf.h conf.h
libconquest_la_LDFLAGS = -version-info 0:0:0

bin_PROGRAMS = conquest conqai conqoper conqstrat conqreplay
libexec_PROGRAMS = conqdriv
          
conquest_SOURCES = conquest.c c_defs.h defs.h conqdef.h conqcom.h context.h
conquest_LDADD = libconquest.la

conqai_SOURCES = conqai_main.c c_defs.h defs.h conqdef.h conqcom.h \
                 context.h conqdata.h

conqai_LDADD = libconquest.la

conqoper_SOURCES = conqoper.c c_defs.h defs.h conqdef.h conqcom.h context.h
conqoper_LDADD = libconquest.la

conqdriv_SOURCES = conqdriv.c c_defs.h defs.h conqdef.h conqcom.h context.h
conqdriv_LDADD = libconquest.la

conqstrat_SOURCES = conqstrat-yacc.y conqstrat-lex.l conqstrat.h
conqstrat_LDADD = libconquest.la $(LEXLIB) $(YACCLIB)

conqreplay_SOURCES = conqreplay.c c_defs.h defs.h conqdef.h conqcom.h context.h record.h
conqreplay_LDADD = libconquest.la

data_DATA = conquest.doc conqnews.doc README conqrule
#sysconf_DATA = conquestrc

EXTRA_DIST = conquest.doc conqnews.doc README conqrule # conquestrc

install-exec-hook:
	chgrp $(CONQGROUP) $(DESTDIR)$(bindir)/conquest \
		$(DESTDIR)$(bindir)/conqoper \
		$(DESTDIR)$(libexecdir)/conqdriv
	chmod g+s $(DESTDIR)$(bindir)/conquest \
		$(DESTDIR)$(bindir)/conqoper \
		$(DESTDIR)$(libexecdir)/conqdriv
		$(DESTDIR)$(bindir)/conqoper -C
	$(FILEPRIV) $(DESTDIR)$(bindir)/conquest \
		$(DESTDIR)$(bindir)/conqoper \
		$(DESTDIR)$(libexecdir)/conqdriv
