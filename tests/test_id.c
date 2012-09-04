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

static void get_event_cb(GArray *intvals, char **strvals, GError *error, gpointer userdata)
{
	guint index;

	if (error)
		DBG("error: %s", error->message);

	DBG("intvals %p", intvals);
	if (intvals) {
		for (index = 0; index < intvals->len; ++index)
			DBG("intvals[%d] = %d", index, g_array_index(intvals,gint,index));
	}

	DBG("strvals %p", strvals);
	if (strvals) {
		for (index = 0; strvals[index] != NULL; ++index)
			DBG("strvals[%d] = %s", index, strvals[index]);
	}

	DBG("userdata %p", userdata);
	if (userdata)
		DBG("userdata %s", (gchar *) userdata);

	g_main_loop_quit(loop);
}

static gboolean
timeoutCallback(DBusGProxy* proxy)
{
	DBG("test_id timeouted");
	g_main_loop_quit(loop);

	return TRUE;
}

int
main(int argc, char **argv)
{
	gint ret = 0;
	unsigned int id = 1;

	g_type_init();

	loop = g_main_loop_new (NULL, TRUE);

	if (argc == 2)
		id = atoi(argv[1]);

	DBG("rtcomel_get_event_async");
	if ((ret = rtcomel_get_event_async(get_event_cb, id,
			"id,service-id,event-type-id,storage-time,start-time,end-time,flags,outgoing",
			"service,event-type,local-uid,local-name,group-uid,remote-ebook-uid,remote-uid,remote-name,channel",
			text))) {
		g_error("Unable to get event id = 1 with: %d", ret);
		exit(1);
	}

	g_timeout_add(60000, (GSourceFunc)timeoutCallback, NULL);

	g_main_loop_run(loop);
	return 1;
}
