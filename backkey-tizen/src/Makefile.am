MAINTAINERCLEANFILES = Makefile.in
MODULE = backkey-tizen

# data files for the module
filesdir = $(libdir)/enlightenment/modules/$(MODULE)

EXTRA_DIST = $(files_DATA)

# the module .so file
INCLUDES               = -I. \
                         -I$(top_srcdir) \
                         -I$(top_srcdir)/include \
                         -I$(top_srcdir)/src \
			 @ENLIGHTENMENT_CFLAGS@ @CAPI_SYSTEM_INFO_CFLAGS@

pkgdir                 = $(libdir)/enlightenment/modules/$(MODULE)/$(MODULE_ARCH)
pkg_LTLIBRARIES        = module.la
module_la_SOURCES      = e_mod_main.c \
                         e_mod_main.h \
			 e_mod_backkey_event.c \
			 e_mod_backkey_event.h

module_la_LIBADD       = @ENLIGHTENMENT_LIBS@ @dlopen_libs@ @CAPI_SYSTEM_INFO_LIBS@
module_la_LDFLAGS      = -module -avoid-version
module_la_DEPENDENCIES = $(top_builddir)/config.h

bin_PROGRAMS = e_backkey_util
e_backkey_util_LDADD = @ENLIGHTENMENT_LIBS@ @DLOG_LIBS@
e_backkey_util_c_files = e_backkey_util.c
e_backkey_util_SOURCES = $(e_backkey_util_c_files)
e_backkey_util_CFLAGS = @ENLIGHTENMENT_CFLAGS@ @DLOG_CFLAGS@

uninstall:
	rm -rf $(DESTDIR)$(libdir)/enlightenment/modules/$(MODULE)
	rm -rf $(DESTDIR)$(bindir)/$(bin_PROGRAMS)
