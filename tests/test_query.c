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

#include <stdlib.h>
#include <unistd.h>

#include <rtcom-eventlogger/eventlogger-async.h>
#include "test.h"

static GMainLoop *loop;
static gchar *text = "Test string for closure";

static void get_event_list_cb(GPtrArray *intvals, GPtrArray *strvals, GError *error, gpointer userdata)
{
	guint ptrindex = 0;
 
	if (error)
		DBG("error: %s", error->message);

	if (intvals != NULL && strvals != NULL) {
		for (ptrindex = 0; ptrindex < intvals->len; ++ptrindex) {
			guint index;
			GArray *ints = (GArray *) g_ptr_array_index(intvals, ptrindex);
			char **strs = (char **) g_ptr_array_index(strvals, ptrindex);

			DBG("=== item %d ===", ptrindex);

			DBG("  ints %p", ints);
			if (ints) {
				for (index = 0; index < ints->len; ++index)
					DBG("  ints[%d] = %d", index, g_array_index(ints,gint,index)); 
			}

			DBG("  strs %p", strs);
			if (strs) {
				for (index = 0; strs[index] != NULL; ++index)
					DBG("  strs[%d] = %s", index, strs[index]);
			}
		}
	}

	DBG("item count = %d", ptrindex);

	DBG("userdata %p", userdata);
	if (userdata)
		DBG("userdata %s", (gchar *) userdata);

	g_main_loop_quit(loop);
}

static gboolean
timeoutCallback(DBusGProxy* proxy)
{
	DBG("test_query timeouted");
	g_main_loop_quit(loop);

	return TRUE;
}

int
main(int argc, char **argv)
{
	gint ret = 0;
	char *query = "";
	guint offset = 0;
	gint limit = -1;

	g_type_init();

	loop = g_main_loop_new (NULL, TRUE);

	if (argc > 1) {
		query = argv[1];
		if (argc > 2) {
			offset = atoi(argv[2]);
			if (argc == 4)
				limit = atoi(argv[3]);
		}
	}

	DBG("rtcomel_get_event_list_async query %s, offset %d limit %d", query, offset, limit);
	if ((ret = rtcomel_get_event_list_async(get_event_list_cb, query,
			"id,service-id,event-type-id,start-time,outgoing",
			"service,event-type,remote-ebook-uid,remote-uid,remote-name",
			offset, limit, text))) {
		g_error("Unable to get query '' with: %d", ret);
		exit(1);
	}

	g_timeout_add(60000, (GSourceFunc)timeoutCallback, NULL);

	g_main_loop_run(loop);
	return 1;
}
