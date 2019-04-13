ACLOCAL_AMFLAGS = -I m4 --install

# User defineable options

CXXFLAGS = @CONQ_CFLAGS@ $(SDL_CFLAGS)
CONQGROUP = @CONQGROUP@
CONQLOCALSTATEDIR = @localstatedir@/conquest
CONQLIBEXECDIR = @libexecdir@
CONQDATADIR = @datadir@
CONQSYSCONFDIR = @sysconfdir@/conquest
CONQIMGDIR = $(CONQDATADIR)/conquest/img
CONQSNDDIR = $(CONQDATADIR)/conquest/sound
CONQDOCDIR = $(CONQDATADIR)/doc/conquest

noinst_LTLIBRARIES = libfmt.la
libfmt_la_SOURCES = format.h format.cxx


