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

#ifndef RTCOMEL_ASYNC_H
#define RTCOMEL_ASYNC_H

#include <glib-object.h>
#include <dbus/dbus.h>
#include <dbus/dbus-glib-bindings.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>

#include <rtcom-eventlogger/eventlogger.h>

#define RTCOM_EVENTLOGGER_ASYNC_SERVICE_NAME	"rtcomeventlogger.events.EventsBook"
#define RTCOM_EVENTLOGGER_ASYNC_PATH		"/rtcomeventlogger/events/EventsList"
#define RTCOM_EVENTLOGGER_ASYNC_INTERFACE	"rtcomeventlogger.events.EventsList"

G_BEGIN_DECLS

typedef void (*RTCOMELEventCallback) (GArray *intvals, char **strvals, GError *error, gpointer userdata);
typedef void (*RTCOMELEventListCallback) (GPtrArray *intvals, GPtrArray *strvals, GError *error, gpointer userdata);
typedef void (*RTCOMELEventListCountCallback) (guint count, GError *error, gpointer userdata);

#define RTCOMEL_ERROR_OK		0
#define RTCOMEL_ERROR_INVALID_ARG	1
#define RTCOMEL_ERROR_OFFLINE		2
#define RTCOMEL_ERROR_CALL_FAILED	3
#define RTCOMEL_ERROR_PARSECOLS		4
#define RTCOMEL_ERROR_ELCREATE		5
#define RTCOMEL_ERROR_ELPREPARE		6
#define RTCOMEL_ERROR_CREATEDATA	7

gint
rtcomel_get_event_async(RTCOMELEventCallback cb, const guint id,
			const gchar *intcols, const gchar *strcols,
			gpointer closure);

gint
rtcomel_get_event_list_async(RTCOMELEventListCallback cb, const gchar *query,
			const gchar *intcols, const gchar *strcols,
			const guint offset, const gint limit, gpointer closure);

gint
rtcomel_get_event_list_count_async(RTCOMELEventListCountCallback cb, const gchar *query,
			const guint offset, const gint limit, gpointer closure);

#endif
