## Process this file with automake to produce Makefile.in

ACLOCAL_AMFLAGS = -I tools

SUBDIRS = tools src tests

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = rtcom-eventlogger-async.pc

service_in_files = rtcomeventlogger.events.EventsBook.service.in
servicedir = $(datadir)/dbus-1/services
service_DATA = $(service_in_files:.service.in=.service)
@EVO_SUBST_SERVICE_RULE@

eventloggerasyncdocdir = ${docdir}
eventloggerasyncdoc_DATA = \
	README\
	COPYING\
	AUTHORS\
	ChangeLog\
	INSTALL\
	NEWS

CLEANFILES = $(service_DATA)
DISTCLEANFILES = $(pkgconfig_DATA)

EXTRA_DIST = 						\
	src/rtcom-eventlogger-async.xml			\
	$(service_in_files)				\
	$(libeventloggerdoc_DATA)

LCOV_DIRS = src
include tools/lcov.am
