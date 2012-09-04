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
#include <string.h>
#include <unistd.h>

#include "rtcom-eventlogger/eventlogger-async.h"
#include "eventlogger-async-client-bindings.h"

static DBusGConnection *connection = NULL;
static DBusGProxy *proxy;

typedef struct {
	gpointer callback;
	gpointer closure;
} AsyncData;

/* helper functions for allocating and freeing AsyncData structs that
 * automatically ref and unref the book and query attributes */
static AsyncData *
async_data_new (gpointer callback, gpointer closure)
{
	AsyncData* data = g_slice_new0(AsyncData);
	data->callback = callback;
	data->closure = closure;

	return data;
}

static void
async_data_free (AsyncData *data)
{
	g_slice_free(AsyncData, data);
}

/* one-time start up for rtcomm-eventlogger-async */
static gboolean
rtcomel_async_activate(GError **error)
{
	DBusError derror;

	if (G_LIKELY(proxy))
		return TRUE;

	if (!connection) {
		connection = dbus_g_bus_get(DBUS_BUS_SESSION, error);
		if (!connection)
			return FALSE;
	}

	dbus_error_init(&derror);
	if (!dbus_bus_start_service_by_name(dbus_g_connection_get_connection(connection),
				RTCOM_EVENTLOGGER_ASYNC_SERVICE_NAME, 0, NULL, &derror)) {
		dbus_set_g_error(error, &derror);
		dbus_error_free(&derror);
		return FALSE;
	}

	if (!proxy) {
		proxy = dbus_g_proxy_new_for_name_owner(connection,
				RTCOM_EVENTLOGGER_ASYNC_SERVICE_NAME, 
				RTCOM_EVENTLOGGER_ASYNC_PATH,
				RTCOM_EVENTLOGGER_ASYNC_INTERFACE,
				error);
		if (!proxy)
			return FALSE;
		g_object_add_weak_pointer(G_OBJECT(proxy), (gpointer) &proxy);
	}

	return TRUE;
}

#if 0
/* Called when the rtcom_eventlogger server dies. */
static void
proxy_destroyed (gpointer data, GObject *object)
{
	g_warning(G_STRLOC ": rtcom-eventlogger proxy died");

	/* Ensure that everything relevant is NULL */
	proxy = NULL;
}

/* one-time finalize for rtcomm-eventlogger-async */
static void
rtcomel_async_dispose (GObject *object)
{
	if (proxy) {
		g_object_weak_unref(G_OBJECT(proxy), proxy_destroyed, object);
		g_object_unref(proxy);
		proxy = NULL;
	}
}
#endif

/*
 * API functions
 */

static void
rtcomel_get_event_reply (DBusGProxy *proxy, GArray *intvals, char **strvals,
			GError *error, gpointer user_data)
{
	AsyncData *data = user_data;
	RTCOMELEventCallback cb = data->callback;

	if (error != NULL) {
		g_warning(G_STRLOC ": cannot get event: %s", error->message);
		intvals = NULL;
		strvals = NULL;
	}

	if (cb)
		cb(intvals, strvals, error, data->closure);
	else
		g_warning(G_STRLOC ": No callback provided for rtcom_get_event_async()");

	if (error == NULL) {
		g_array_free(intvals, TRUE);
		g_strfreev(strvals);
	} else {
		g_error_free (error);
	}
	async_data_free(data);
}

gint
rtcomel_get_event_async(RTCOMELEventCallback cb, const guint id,
			const gchar *intcols, const gchar *strcols,
			gpointer closure)
{
	AsyncData *data;
	GError *err = NULL;

	if (intcols == NULL || *intcols == '\0' ||
	    strcols == NULL || *strcols == '\0'
	)
		return RTCOMEL_ERROR_INVALID_ARG;

	if (rtcomel_async_activate(&err) == FALSE) {
		g_warning(G_STRLOC ": cannot activate rtcom-eventlogger: %s\n", err->message);
		g_error_free (err);
		return RTCOMEL_ERROR_OFFLINE;
	}

	data = async_data_new(cb, closure);

	if (rtcomeventlogger_events_EventsList_get_event_async(proxy, id, intcols, strcols, rtcomel_get_event_reply, data) == FALSE) {
		return RTCOMEL_ERROR_CALL_FAILED;
	}

	return RTCOMEL_ERROR_OK;
}

static void
rtcomel_get_event_list_reply (DBusGProxy *proxy, GPtrArray *intvals, GPtrArray *strvals,
			GError *error, gpointer user_data)
{
	AsyncData *data = user_data;
	RTCOMELEventListCallback cb = data->callback;

	if (error) {
		g_warning(G_STRLOC ": cannot get event list: %s", error->message);
		intvals = NULL;
		strvals = NULL;
	}

	if (cb)
		cb(intvals, strvals, error, data->closure);
	else
		g_warning(G_STRLOC ": No callback provided for rtcom_get_event_list_async()");

	if (error == NULL) {
		g_ptr_array_foreach(intvals, (GFunc) g_array_free, (gpointer) TRUE);
		g_ptr_array_free(intvals, TRUE);
		g_ptr_array_foreach(strvals, (GFunc) g_strfreev, NULL);
		g_ptr_array_free(strvals, TRUE);
	} else {
		g_error_free (error);
	}
	async_data_free(data);
}

gint
rtcomel_get_event_list_async(RTCOMELEventListCallback cb, const gchar *query,
			const gchar *intcols, const gchar *strcols,
			const guint offset, const gint limit, gpointer closure)
{
	AsyncData *data;
	GError *err = NULL;

	if (query == NULL ||
	    intcols == NULL || *intcols == '\0' ||
	    strcols == NULL || *strcols == '\0' ||
	    limit == 0
	)
		return RTCOMEL_ERROR_INVALID_ARG;

	if (rtcomel_async_activate(&err) == FALSE) {
		g_warning(G_STRLOC ": cannot activate rtcom-eventlogger: %s\n", err->message);
		g_error_free (err);
		return RTCOMEL_ERROR_OFFLINE;
	}

	data = async_data_new(cb, closure);

	if (rtcomeventlogger_events_EventsList_get_event_list_async(proxy, query, intcols, strcols, offset, limit, rtcomel_get_event_list_reply, data) == FALSE) {
		return RTCOMEL_ERROR_CALL_FAILED;
	}

	return RTCOMEL_ERROR_OK;
}

static void
rtcomel_get_event_list_count_reply (DBusGProxy *proxy, guint count, GError *error, gpointer user_data)
{
	AsyncData *data = user_data;
	RTCOMELEventListCountCallback cb = data->callback;

	if (error) {
		g_warning(G_STRLOC ": cannot get event list count: %s", error->message);
		count = 0;
	}

	if (cb)
		cb(count, error, data->closure);
	else
		g_warning(G_STRLOC ": No callback provided for rtcom_get_event_list_count_async()");

	if (error)
		g_error_free (error);

	async_data_free(data);
}

gint
rtcomel_get_event_list_count_async(RTCOMELEventListCountCallback cb, const gchar *query,
			const guint offset, const gint limit, gpointer closure)
{
	AsyncData *data;
	GError *err = NULL;

	if (query == NULL ||
	    limit == 0
	)
		return RTCOMEL_ERROR_INVALID_ARG;

	if (rtcomel_async_activate(&err) == FALSE) {
		g_warning(G_STRLOC ": cannot activate rtcom-eventlogger: %s\n", err->message);
		g_error_free (err);
		return RTCOMEL_ERROR_OFFLINE;
	}

	data = async_data_new(cb, closure);

	if (rtcomeventlogger_events_EventsList_get_event_list_count_async(proxy, query, offset, limit, rtcomel_get_event_list_count_reply, data) == FALSE) {
		return RTCOMEL_ERROR_CALL_FAILED;
	}

	return RTCOMEL_ERROR_OK;
}
