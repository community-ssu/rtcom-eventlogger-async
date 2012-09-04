## Process this file with automake to produce Makefile.in

INCLUDES = \
	$(RTCOM_EVENTLOGGER_ASYNC_CFLAGS) \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	-DPACKAGE_SRC_DIR=\""$(srcdir)"\" \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\"

AM_CFLAGS = -I$(srcdir)/..

DBUS_BINDINGS = eventlogger-async-client-bindings.h

lib_LTLIBRARIES = librtcom-eventlogger-async.la

librtcom_eventlogger_async_la_SOURCES = eventlogger-async-client.c \
                                        rtcom-eventlogger/eventlogger-async.h \
                                        $(DBUS_BINDINGS)

librtcom_eventlogger_async_la_LIBADD = $(RTCOM_EVENTLOGGER_ASYNC_LIBS)

librtcom_eventlogger_async_la_LDFLAGS = -no-undefined \
        -export-symbols-regex "^rtcomel_.*_async" \
        -version-info "$(LT_CURRENT)":"$(LT_REVISION)":"$(LT_AGE)"

BUILT_SOURCES = $(DBUS_BINDINGS)

DBUS_GLUES = eventlogger-async-server-glue.h

factorydir = $(libexecdir)
factory_PROGRAMS = rtcom-eventlogger-factory

rtcom_eventlogger_factory_SOURCES = eventlogger-async-server.c \
                                    rtcom-eventlogger/eventlogger-async.h \
                                    $(DBUS_GLUES)

rtcom_eventlogger_factory_LDADD = $(RTCOM_EVENTLOGGER_ASYNC_LIBS)

BUILT_SOURCES += $(DBUS_GLUES)

nobase_include_HEADERS = rtcom-eventlogger/eventlogger-async.h

eventlogger-async-server-glue.h: rtcom-eventlogger-async.xml
	dbus-binding-tool --mode=glib-server --output=$@ --prefix=rtcomeventlogger_events_EventsList $^

eventlogger-async-client-bindings.h: rtcom-eventlogger-async.xml
	dbus-binding-tool --mode=glib-client --output=$@ --prefix=rtcomeventlogger_events_EventsList $^
