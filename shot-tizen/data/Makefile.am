MAINTAINERCLEANFILES = Makefile.in

SUBDIRS = sounds

EDJE_CC = @EDJE_CC@
EDJE_FLAGS = -v \
-id $(top_srcdir)/data/images

filesdir = /usr/share/enlightenment/data/themes
files_DATA = \
shot_effect.edj

EXTRA_DIST = \
shot_effect.edc

shot_effect.edj: Makefile $(EXTRA_DIST)
	$(EDJE_CC) $(EDJE_FLAGS) \
	$(top_srcdir)/data/shot_effect.edc \
	$(top_builddir)/data/shot_effect.edj

clean-local:
	rm -f *.edj
