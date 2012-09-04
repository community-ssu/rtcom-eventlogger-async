/*
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of version 2 of the GNU Lesser General Public License as published
 * by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * Author: Ludek Finstrle <luf@pzkagis.cz>
 */

#include <config.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "rtcom-eventlogger/eventlogger-async.h"

typedef struct {
	GObject parent;
	RTComEl *el;
	RTComElQuery *el_query;
} RTCOMELAsyncFactory;

typedef struct {
	GObjectClass parent;
} RTCOMELAsyncFactoryClass;

GType rtcomel_async_factory_get_type(void);
#define RTCOMEL_TYPE_ASYNC_FACTORY	(rtcomel_async_factory_get_type())
#define RTCOMEL_ASYNC_FACTORY(object)				\
	(G_TYPE_CHECK_INSTANCE_CAST ((object),			\
	 RTCOMEL_TYPE_ASYNC_FACTORY, RTCOMELAsyncFactory))
#define RTCOMEL_ASYNC_FACTORY_CLASS(klass)			\
	(G_TYPE_CHECK_CLASS_CAST ((klass),			\
	 RTCOMEL_TYPE_ASYNC_FACTORY, RTCOMELAsyncFactoryClass))
#define RTCOMEL_IS_ASYNC_FACTORY(object)			\
	(G_TYPE_CHECK_INSTANCE_TYPE ((object),			\
	 RTCOMEL_TYPE_ASYNC_FACTORY))
#define RTCOMEL_IS_ASYNC_FACTORY_CLASS(klass)			\
	(G_TYPE_CHECK_CLASS_TYPE ((klass),			\
	 RTCOMEL_TYPE_ASYNC_FACTORY))
#define RTCOMEL_ASYNC_FACTORY_GET_CLASS(obj)			\
	(G_TYPE_INSTANCE_GET_CLASS ((obj),			\
	 RTCOMEL_TYPE_ASYNC_FACTORY, RTCOMELAsyncFactoryClass))

G_DEFINE_TYPE(RTCOMELAsyncFactory, rtcomel_async_factory, G_TYPE_OBJECT)

static void impl_EventsList_getEvent(RTCOMELAsyncFactory *factory, const guint IN_id, const char *IN_intcols, const char *IN_strcols, DBusGMethodInvocation *context);
static void impl_EventsList_getEventList(RTCOMELAsyncFactory *factory, const char *IN_query, const char *IN_intcols, const char *IN_strcols, const guint IN_offset, const gint IN_limit, DBusGMethodInvocation *context);
static void impl_EventsList_getEventListCount(RTCOMELAsyncFactory *factory, const char *IN_query, const guint IN_offset, const gint IN_limit, DBusGMethodInvocation *context);

GQuark
rtcomel_async_error_quark(void)
{
	static GQuark quark = 0;

	if (G_UNLIKELY (quark == 0))
		quark = g_quark_from_static_string ("rtcomel-async-error");

	return quark;
}

#define RTCOMEL_ASYNC_ERROR	rtcomel_async_error_quark()

#include "eventlogger-async-server-glue.h"

#define RTCOMEL_QUERY_PARTS_MAX 20
#define RTCOMEL_USE_QUERY_PARTS(parts)	parts[0].name, parts[0].value, parts[0].op,		\
					parts[1].name, parts[1].value, parts[1].op,		\
					parts[2].name, parts[2].value, parts[2].op,		\
					parts[3].name, parts[3].value, parts[3].op,		\
					parts[4].name, parts[4].value, parts[4].op,		\
					parts[5].name, parts[5].value, parts[5].op,		\
					parts[6].name, parts[6].value, parts[6].op,		\
					parts[7].name, parts[7].value, parts[7].op,		\
					parts[8].name, parts[8].value, parts[8].op,		\
					parts[9].name, parts[9].value, parts[9].op,		\
					parts[10].name, parts[10].value, parts[10].op,	\
					parts[11].name, parts[11].value, parts[11].op,	\
					parts[12].name, parts[12].value, parts[12].op,	\
					parts[13].name, parts[13].value, parts[13].op,	\
					parts[14].name, parts[14].value, parts[14].op,	\
					parts[15].name, parts[15].value, parts[15].op,	\
					parts[16].name, parts[16].value, parts[16].op,	\
					parts[17].name, parts[17].value, parts[17].op,	\
					parts[18].name, parts[18].value, parts[18].op,	\
					parts[19].name, parts[19].value, parts[19].op

typedef struct {
	char *name;
	gpointer value;
	gboolean is_string;
	RTComElOp op;
} RTCOMEL_Query_Part;

static GMainLoop *loop;
DBusGConnection *connection;
RTCOMELAsyncFactory *factory;

/* support functions */

static void
die(const char *prefix, GError *error)
{
	g_error("%s: %s", prefix, error->message);
	g_error_free(error);
	exit(1);
}

static gchar *
get_query_part(char **query, gboolean *is_string)
{
	char *part = *query;
	char *end;
	char save;
	gboolean is_str = FALSE;

	if (part == NULL || *part == '\0')
		return NULL;

	if (*part == '"') {
		char *test;

		is_str = TRUE;

		end = part + 1;
		while (*end != '"') {
			end = strstr(end, "\";");
			if (end == NULL)
				break;
			test = end - 1;
			while (*test == '\\')
				--test;
			if (G_UNLIKELY(((end - test) % 2) == 0))
				end += 2;
		}
	} else {
		end =  strchr(*query, ';');
	}
	if (end != NULL) {
		save = *end;
		*end = '\0';
	} else {
		save = '\0';
	}
	part = g_strdup(part + ( is_str ? 1 : 0 ));
	if (G_UNLIKELY(save == '\0')) {
		*query = end;
	} else {
		*end = save;
		*query = end + ( is_str ? 2 : 1 );
	}

	if (is_string)
		*is_string = is_str;

	return part;
}

static gboolean
prepare_el(RTCOMELAsyncFactory* obj)
{
	if (obj->el != NULL)
		return TRUE;

	obj->el = rtcom_el_new();
	if (!RTCOM_IS_EL(obj->el)) {
		obj->el = NULL;
		return FALSE;
	}

	return TRUE;
}

static gboolean
prepare_el_query(RTCOMELAsyncFactory* obj)
{
	if (prepare_el(obj) != TRUE)
		return FALSE;

	if (obj->el_query == NULL)
		obj->el_query = rtcom_el_query_new(obj->el);

	if (!RTCOM_IS_EL_QUERY(obj->el_query)) {
		obj->el_query = NULL;
		return FALSE;
	}

	return TRUE;
}

static gboolean
define_el_query_id(RTCOMELAsyncFactory* obj, const guint value)
{
	if (prepare_el_query(obj) != TRUE)
		return FALSE;

	rtcom_el_query_set_offset(obj->el_query, 0);
	rtcom_el_query_set_limit(obj->el_query, 1);

	if (!rtcom_el_query_prepare(obj->el_query, "id", value, RTCOM_EL_OP_EQUAL, NULL))
		return FALSE;

	return TRUE;
}

static gboolean
define_el_query_query(RTCOMELAsyncFactory* obj, const char *query, guint offset, gint limit)
{
	if (prepare_el_query(obj) != TRUE)
		return FALSE;

	rtcom_el_query_set_offset(obj->el_query, offset);
	rtcom_el_query_set_limit(obj->el_query, limit);

	/* some obscurd dance around va_list */
	if (query != NULL && *query != '\0') {
		gint count = 0;
		char *part = (char *) query;
		char *value;
		gboolean is_str;
		RTCOMEL_Query_Part parts[RTCOMEL_QUERY_PARTS_MAX];

		bzero(parts, RTCOMEL_QUERY_PARTS_MAX * sizeof(RTCOMEL_Query_Part));
		while (part != NULL && *part != '\0' && count < RTCOMEL_QUERY_PARTS_MAX) {
			value = get_query_part(&part, NULL);
			if (G_UNLIKELY(part == NULL || *part == '\0')) {
				g_free(value);
				break;
			}
			parts[count].name = value;
			value = get_query_part(&part, &is_str);
			if (G_UNLIKELY(part == NULL || *part == '\0')) {
				g_free(parts[count].name);
				parts[count].name = NULL;
				g_free(value);
				break;
			}
			if (is_str) {
				parts[count].value = value;
			} else {
				parts[count].value = (gpointer) atoi(value);
				g_free(value);
			}
			parts[count].is_string = is_str;
			value = get_query_part(&part, NULL);
			parts[count].op = (RTComElOp) atoi(value);
			g_free(value);
			++count;
		}
		if (part != NULL && *part != '\0')
			g_warning("Maximum query parts are %d, ignoring rest: %s", RTCOMEL_QUERY_PARTS_MAX, part);
	
		if (!rtcom_el_query_prepare(obj->el_query, RTCOMEL_USE_QUERY_PARTS(parts), NULL))
			return FALSE;

		for (--count; count >= 0; --count) {
			if (parts[count].name != NULL)
				g_free(parts[count].name);
			if (parts[count].is_string && parts[count].value != NULL)
				g_free(parts[count].value);
		}
	} else {
		if (!rtcom_el_query_prepare(obj->el_query, NULL))
			return FALSE;
	}

	return TRUE;
}

static RTComElIter *
prepare_el_iter(RTCOMELAsyncFactory* obj)
{
	if (obj->el == NULL || obj->el_query == NULL)
		return NULL;

	RTComElIter *iter = rtcom_el_get_events(obj->el, obj->el_query);
	if (!RTCOM_IS_EL_ITER(iter))
		return NULL;

	return iter;
}

static gboolean
el_get_row_data(RTComElIter *iter, char **intcols, char **strcols,
		GArray *intvals, char **strvals)
{
	while (*intcols != NULL) {
		gint ival;
		rtcom_el_iter_get_values(iter, *intcols, &ival, NULL);
		g_array_append_val(intvals, ival);
		++intcols;
	}

	while (*strcols != NULL) {
		char *sval;
		rtcom_el_iter_get_values(iter, *strcols, &sval, NULL);
		if (sval == NULL)
			sval = g_strdup("");
		*strvals = sval;
		++strcols;
		++strvals;
	}

	return TRUE;		
}

static guint
parse_col(const char *col, char ***cols)
{
	if (col == NULL)
		return 0;

	*cols = g_strsplit(col, ",", -1);
	return g_strv_length(*cols);
}

static gboolean
parse_cols(const char *intcols, const char *strcols, guint *intcount, guint *strcount,
		char ***aintcols, char ***astrcols)
{
	if (intcols == NULL || strcols == NULL ||
	    intcount == NULL || strcount == NULL ||
	    aintcols == NULL || astrcols == NULL)
		return FALSE;

	*intcount  = parse_col(intcols, aintcols);
	if (*aintcols == NULL)
		return FALSE;
	*strcount = parse_col(strcols, astrcols);
	if (*astrcols == NULL) {
		g_strfreev(*aintcols);
		return FALSE;
	}

	return TRUE;
}

static gboolean
create_row_structures(guint intcount, guint strcount,
		GArray **intvals, char ***strvals)
{
	if (intcount == 0)
		intcount = 1;
	*intvals = g_array_sized_new(FALSE, TRUE, sizeof(gint), intcount);
	if (*intvals == NULL)
		return FALSE;
	size_t size = (strcount + 1) * sizeof(char *); 
	*strvals = (char **) malloc(size);
	if (*strvals == NULL) {
		g_array_free(*intvals, TRUE);
		*intvals = NULL;
		return FALSE;
	} else {
		bzero(*strvals, size);
	}

	return TRUE;
}

static gboolean
create_table_structures(GPtrArray **intvals, GPtrArray **strvals)
{
	*intvals = g_ptr_array_new();
	if (*intvals == NULL)
		return FALSE;
	*strvals = g_ptr_array_new();
	if (*strvals == NULL) {
		g_ptr_array_free(*intvals, TRUE);
		*intvals = NULL;
		return FALSE;
	}

	return TRUE;
}

/* GObject RTCOMELAsyncFactory */
static void rtcomel_async_factory_init(RTCOMELAsyncFactory *obj)
{
	g_assert(obj != NULL);

	obj->el = NULL;
	obj->el_query = NULL;
}

static void rtcomel_async_factory_dispose(GObject *obj)
{
	RTCOMELAsyncFactory *self = RTCOMEL_ASYNC_FACTORY(obj);

	if (self->el_query) {
		g_object_unref(self->el_query);
		self->el_query = NULL;
	}
	if (self->el) {
		g_object_unref(self->el);
		self->el = NULL;
	}

	if (G_OBJECT_CLASS(rtcomel_async_factory_parent_class)->dispose)
		G_OBJECT_CLASS(rtcomel_async_factory_parent_class)->dispose(obj);
}

static void rtcomel_async_factory_class_init(RTCOMELAsyncFactoryClass *rtcomel_async_factory_class)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS(rtcomel_async_factory_class);

	gobject_class->dispose = rtcomel_async_factory_dispose;
	dbus_g_object_type_install_info(G_TYPE_FROM_CLASS(rtcomel_async_factory_class),
			&dbus_glib_rtcomeventlogger_events_EventsList_object_info);
}

/* DBus calls */

static void impl_EventsList_getEvent(RTCOMELAsyncFactory *factory, const guint IN_id, const char *IN_intcols, const char *IN_strcols, DBusGMethodInvocation *context)
{
	guint intcount, strcount;
	char **intcols = NULL;
	char **strcols = NULL;
	GArray *intvals;
	char **strvals;
	RTComElIter *iter = NULL;
	GError *error = NULL;

	if (parse_cols(IN_intcols, IN_strcols, &intcount, &strcount, &intcols, &strcols) == FALSE) {
		error = g_error_new(RTCOMEL_ASYNC_ERROR, RTCOMEL_ERROR_PARSECOLS, "Unable to create structures for results");
		goto fail;
	}
	if (define_el_query_id(factory, IN_id) == FALSE) {
		error = g_error_new(RTCOMEL_ASYNC_ERROR, RTCOMEL_ERROR_ELCREATE, "Unable to prepare id query");
		goto fail;
	}
	if ((iter = prepare_el_iter(factory)) == NULL) {
		error = g_error_new(RTCOMEL_ASYNC_ERROR, RTCOMEL_ERROR_ELPREPARE, "Unable to prepare iteration");
		goto fail;
	}

	if (create_row_structures(intcount, strcount, &intvals, &strvals) == FALSE) {
		error = g_error_new(RTCOMEL_ASYNC_ERROR, RTCOMEL_ERROR_CREATEDATA, "Unable to create structures for row data");
		goto fail;
	}

	if (rtcom_el_iter_first(iter))
		el_get_row_data(iter, intcols, strcols, intvals, strvals);

	g_object_unref(iter);
	g_strfreev(intcols);
	g_strfreev(strcols);

	dbus_g_method_return(context, intvals, strvals);

	g_array_free(intvals, TRUE);
	g_strfreev(strvals);

	return;

fail:
	if (iter)
		g_object_unref(iter);
	if (intcols)
		g_strfreev(intcols);
	if (strcols)
		g_strfreev(strcols);

	dbus_g_method_return_error(context, error);
	g_error_free(error);
}

static void impl_EventsList_getEventList(RTCOMELAsyncFactory *factory, const char *IN_query, const char *IN_intcols, const char *IN_strcols, const guint IN_offset, const gint IN_limit, DBusGMethodInvocation *context)
{
	guint intcount, strcount;
	char **intcols = NULL;
	char **strcols = NULL;
	GArray *intvals;
	char **strvals;
	GPtrArray *inttable = NULL;
	GPtrArray *strtable = NULL;
	RTComElIter *iter = NULL;
	GError *error = NULL;

	if (parse_cols(IN_intcols, IN_strcols, &intcount, &strcount, &intcols, &strcols) == FALSE) {
		error = g_error_new(RTCOMEL_ASYNC_ERROR, RTCOMEL_ERROR_PARSECOLS, "Unable to create structures for results");
		goto fail;
	}
	if (define_el_query_query(factory, IN_query, IN_offset, IN_limit) == FALSE) {
		error = g_error_new(RTCOMEL_ASYNC_ERROR, RTCOMEL_ERROR_ELCREATE, "Unable to prepare custom query");
		goto fail;
	}
	if ((iter = prepare_el_iter(factory)) == NULL) {
		error = g_error_new(RTCOMEL_ASYNC_ERROR, RTCOMEL_ERROR_ELPREPARE, "Unable to prepare iteration");
		goto fail;
	}

	if (create_table_structures(&inttable, &strtable) == FALSE) {
		error = g_error_new(RTCOMEL_ASYNC_ERROR, RTCOMEL_ERROR_CREATEDATA, "Unable to create structures for table data");
		goto fail;
	}
	if (rtcom_el_iter_first(iter)) {
		do {
			intvals = NULL;
			strvals = NULL;

			if (create_row_structures(intcount, strcount, &intvals, &strvals) == FALSE) {
				error = g_error_new(RTCOMEL_ASYNC_ERROR, RTCOMEL_ERROR_CREATEDATA, "Unable to create structures for row data");
				goto fail;
			}

			el_get_row_data(iter, intcols, strcols, intvals, strvals);

			g_ptr_array_add(inttable, intvals);
			g_ptr_array_add(strtable, strvals);
		} while (rtcom_el_iter_next(iter));
	}

	g_object_unref(iter);
	g_strfreev(intcols);
	g_strfreev(strcols);

	dbus_g_method_return(context, inttable, strtable);

	g_ptr_array_foreach(inttable, (GFunc) g_array_free, (gpointer) TRUE);
	g_ptr_array_free(inttable, TRUE);
	g_ptr_array_foreach(strtable, (GFunc) g_strfreev, NULL);
	g_ptr_array_free(strtable, TRUE);

	return;

fail:
	if (iter)
		g_object_unref(iter);
	if (intcols)
		g_strfreev(intcols);
	if (strcols)
		g_strfreev(strcols);
	if (inttable) {
		g_ptr_array_foreach(inttable, (GFunc) g_array_free, (gpointer) TRUE);
		g_ptr_array_free(inttable, TRUE);
	}
	if (strtable) {
		g_ptr_array_foreach(inttable, (GFunc) g_strfreev, NULL);
		g_ptr_array_free(strtable, TRUE);
	}

	dbus_g_method_return_error(context, error);
	g_error_free(error);
}

static void impl_EventsList_getEventListCount(RTCOMELAsyncFactory *factory, const char *IN_query, const guint IN_offset, const gint IN_limit, DBusGMethodInvocation *context)
{
	RTComElIter *iter = NULL;
	GError *error = NULL;
	guint count = 0;

	if (define_el_query_query(factory, IN_query, IN_offset, IN_limit) == FALSE) {
		error = g_error_new(RTCOMEL_ASYNC_ERROR, RTCOMEL_ERROR_ELCREATE, "Unable to prepare custom query");
		goto fail;
	}
	if ((iter = prepare_el_iter(factory)) == NULL) {
		error = g_error_new(RTCOMEL_ASYNC_ERROR, RTCOMEL_ERROR_ELPREPARE, "Unable to prepare iteration");
		goto fail;
	}
 
	if (rtcom_el_iter_first(iter)) {
		++count;
		while (rtcom_el_iter_next(iter))
			++count;
	}

	g_object_unref(iter);

	dbus_g_method_return(context, count);

	return;

fail:
	if (iter)
		g_object_unref(iter);

	dbus_g_method_return_error(context, error);
	g_error_free(error);
}

/* Main function */

int
main(int argc, char **argv)
{
	GError *error = NULL;
	DBusGProxy *bus_proxy;
	guint32 request_name_ret;

	g_type_init();

	loop = g_main_loop_new(NULL, FALSE);
	if (loop == NULL) {
		g_error("Failed to initialize GMainLoop");
		exit(1);
	}

	/* Obtain a connection to the session bus */
	connection = dbus_g_bus_get(DBUS_BUS_SESSION, &error);
	if (connection == NULL)
		die("Failed to open connection to bus", error);

	bus_proxy = dbus_g_proxy_new_for_name(connection,
			DBUS_SERVICE_DBUS,
			DBUS_PATH_DBUS,
			DBUS_INTERFACE_DBUS);

	if (!org_freedesktop_DBus_request_name(bus_proxy, RTCOM_EVENTLOGGER_ASYNC_SERVICE_NAME,
						0, &request_name_ret, &error))
		die ("Failed to get name", error);

	g_object_unref(bus_proxy);

	if (request_name_ret != DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER) {
		g_error("Got result code %u from requesting name", request_name_ret);
		exit(1);
	}

	factory = g_object_new(RTCOMEL_TYPE_ASYNC_FACTORY, NULL);
	dbus_g_connection_register_g_object(connection,
			RTCOM_EVENTLOGGER_ASYNC_PATH,
			G_OBJECT(factory));

	g_main_loop_run(loop);

	dbus_g_connection_unref(connection);

	return 0;
}
