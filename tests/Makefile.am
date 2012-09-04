## Process this file with automake to produce Makefile.in

INCLUDES = $(RTCOM_EVENTLOGGER_ASYNC_CFLAGS) \
           -I../src

bin_PROGRAMS = test-get-event-by-id test-get-event-list

test_get_event_by_id_SOURCES = \
	test_id.c

test_get_event_by_id_CFLAGS = $(CHECK_FLAGS)
test_get_event_by_id_LDADD = \
	$(RTCOM_EVENTLOGGER_ASYNC_LIBS) \
	$(CHECK_LIBS) ${top_builddir}/src/librtcom-eventlogger-async.la

test_get_event_list_SOURCES = \
	test_query.c

test_get_event_list_CFLAGS = $(CHECK_FLAGS)
test_get_event_list_LDADD = \
	$(RTCOM_EVENTLOGGER_ASYNC_LIBS) \
	$(CHECK_LIBS) ${top_builddir}/src/librtcom-eventlogger-async.la

COMMON_TESTS_ENVIRONMENT = \
	RTCOM_EL_PLUGINS_PATH=.libs \
	RTCOM_EL_HOME=@abs_top_builddir@/tests \
	G_DEBUG=fatal-criticals
TESTS_ENVIRONMENT = \
	$(COMMON_TESTS_ENVIRONMENT) \
	sh @top_srcdir@/tools/with-session-bus.sh --session --

TESTS = test-get-event-by-id test-get-event-list

check-valgrind:
	$(MAKE) check-TESTS TESTS_ENVIRONMENT='$$(VALGRIND_TESTS_ENVIRONMENT)'

VALGRIND_TESTS_ENVIRONMENT = \
	$(COMMON_TESTS_ENVIRONMENT) \
	G_SLICE=always-malloc \
	G_DEBUG=gc-friendly,fatal-criticals \
	CK_FORK=no \
	sh @top_srcdir@/tools/with-session-bus.sh --session -- \
	$(top_builddir)/libtool --mode=execute \
	valgrind --tool=memcheck --leak-check=full --show-reachable=yes -v \
		--suppressions=$(top_srcdir)/data/rtcom-eventlogger.supp \
		--num-callers=20 --error-exitcode=42 --gen-suppressions=all \
		$(VALGRIND_FLAGS)
