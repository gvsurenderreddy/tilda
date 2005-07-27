/*
 * This is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Library General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <stdio.h>
#include <gtk/gtk.h>
#include <glib-object.h>
#include <vte/vte.h>
#include "readconf.h"
#include "config.h"
#include "../tilda-config.h"
#include "tilda.h"
#include "callback_func.h"
#include "tilda_window.h"
#include "tilda_terminal.h"
#include "key_grabber.h"

void get_defaults (tilda_window *tw)
{
    tw->tc->lines = DEFAULT_LINES;
    g_strlcpy (tw->tc->s_image, "none", sizeof (tw->tc->s_image));
    g_strlcpy (tw->tc->s_background, "white", sizeof (tw->tc->s_background));
    g_strlcpy (tw->tc->s_font, "monospace 9", sizeof (tw->tc->s_font));
    g_strlcpy (tw->tc->s_down, "TRUE", sizeof (tw->tc->s_down));
    g_strlcpy (tw->tc->s_antialias, "TRUE", sizeof (tw->tc->s_antialias));
    g_strlcpy (tw->tc->s_scrollbar, "FALSE", sizeof (tw->tc->s_scrollbar));
    g_strlcpy (tw->tc->s_use_image, "FALSE", sizeof (tw->tc->s_use_image));
    g_strlcpy (tw->tc->s_grab_focus, "TRUE", sizeof (tw->tc->s_grab_focus));
    g_strlcpy (tw->tc->s_key, "null", sizeof (tw->tc->s_key));
    tw->tc->transparency = 0;
    tw->tc->x_pos = 0;
    tw->tc->y_pos = 0;
    tw->tc->tab_pos = 0;
}

void init_tilda_window_configs (tilda_window *tw)
{
    int i ;

    CONFIG t_c[] = {
        { CF_INT,       "max_height",   &(tw->tc->max_height),    0,                      NULL, 0, NULL },
        { CF_INT,       "max_width",    &(tw->tc->max_width),     0,                      NULL, 0, NULL },
        { CF_INT,       "min_height",   &(tw->tc->min_height),    0,                      NULL, 0, NULL },
        { CF_INT,       "min_width",    &(tw->tc->min_width),     0,                      NULL, 0, NULL },
        { CF_STRING,    "notaskbar",    tw->tc->s_notaskbar,    sizeof(tw->tc->s_notaskbar),    NULL, 0, NULL },
        { CF_STRING,    "above",        tw->tc->s_above,        sizeof(tw->tc->s_above),        NULL, 0, NULL },
        { CF_STRING,    "pinned",       tw->tc->s_pinned,       sizeof(tw->tc->s_pinned),       NULL, 0, NULL },
        { CF_INT,       "scrollback",   &(tw->tc->lines),         0,                      NULL, 0, NULL },
        { CF_INT,       "transparency", &(tw->tc->transparency),  0,                      NULL, 0, NULL },
        { CF_INT,       "x_pos",        &(tw->tc->x_pos),         0,                      NULL, 0, NULL },
        { CF_INT,       "y_pos",        &(tw->tc->y_pos),         0,                      NULL, 0, NULL },
        { CF_STRING,    "image",        tw->tc->s_image,        sizeof(tw->tc->s_image),        NULL, 0, NULL },
        { CF_STRING,    "background",   tw->tc->s_background,   sizeof(tw->tc->s_background),   NULL, 0, NULL },
        { CF_STRING,    "font",         tw->tc->s_font,         sizeof(tw->tc->s_font),         NULL, 0, NULL },
        { CF_STRING,    "antialias",    tw->tc->s_antialias,    sizeof(tw->tc->s_antialias),    NULL, 0, NULL },
        { CF_STRING,    "scrollbar",    tw->tc->s_scrollbar,    sizeof(tw->tc->s_scrollbar),    NULL, 0, NULL },
        { CF_STRING,    "use_image",    tw->tc->s_use_image,    sizeof(tw->tc->s_use_image),    NULL, 0, NULL },
        { CF_STRING,    "grab_focus",   tw->tc->s_grab_focus,   sizeof(tw->tc->s_grab_focus),   NULL, 0, NULL },
        { CF_STRING,    "key",          tw->tc->s_key,          sizeof(tw->tc->s_key),          NULL, 0, NULL },
        { CF_STRING,    "down",         tw->tc->s_down,         sizeof(tw->tc->s_down),         NULL, 0, NULL },
        { CF_INT,       "tab_pos",      &(tw->tc->tab_pos),         0,                      NULL, 0, NULL }
    };

    for (i=0;i<NUM_ELEM;i++)
        tw->tilda_config[i] = t_c[i];

    get_defaults (tw);
}

void add_tab (tilda_window *tw)
{
    tilda_term *tt;

    tt = (tilda_term *) malloc (sizeof (tilda_term));

    init_tilda_terminal (tw, tt);
}

void add_tab_menu_call (gpointer data, guint callback_action, GtkWidget *w)
{
    tilda_window *tw;
    tilda_collect *tc = (tilda_collect *) data;

    tw = tc->tw;

    add_tab (tw);
}

void close_tab (gpointer data, guint callback_action, GtkWidget *w)
{
    gint pos;
    tilda_term *tt;
    tilda_window *tw;
    tilda_collect *tc = (tilda_collect *) data;

    tw = tc->tw;
    tt = tc->tt;

    pos = gtk_notebook_page_num (GTK_NOTEBOOK (tw->notebook), tt->hbox);
    gtk_notebook_remove_page (GTK_NOTEBOOK (tw->notebook), pos);

    if (gtk_notebook_get_n_pages (GTK_NOTEBOOK (tw->notebook)) == 0)
        clean_up (tw);
    else if (gtk_notebook_get_n_pages (GTK_NOTEBOOK (tw->notebook)) == 1)
        gtk_notebook_set_show_tabs (GTK_NOTEBOOK (tw->notebook), FALSE);

    g_free (tt);
}

gboolean init_tilda_window (tilda_window *tw, tilda_term *tt)
{
    GtkAccelGroup *accel_group;
    GClosure *clean, *new_tab;
    GError *error;

    /* Create a window to hold the scrolling shell, and hook its
     * delete event to the quit function.. */
    tw->window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_container_set_resize_mode (GTK_CONTAINER(tw->window), GTK_RESIZE_IMMEDIATE);
    g_signal_connect (G_OBJECT(tw->window), "delete_event", GTK_SIGNAL_FUNC(deleted_and_quit), tw->window);

    /* Create notebook to hold all terminal widgets */
    tw->notebook = gtk_notebook_new ();

    if (tw->tc->tab_pos == 0)
        gtk_notebook_set_tab_pos (GTK_NOTEBOOK (tw->notebook), GTK_POS_TOP);
    else if (tw->tc->tab_pos == 1)
        gtk_notebook_set_tab_pos (GTK_NOTEBOOK (tw->notebook), GTK_POS_BOTTOM);
    else if (tw->tc->tab_pos == 2)
        gtk_notebook_set_tab_pos (GTK_NOTEBOOK (tw->notebook), GTK_POS_LEFT);
    else if (tw->tc->tab_pos == 3)
        gtk_notebook_set_tab_pos (GTK_NOTEBOOK (tw->notebook), GTK_POS_RIGHT);

    gtk_container_add (GTK_CONTAINER(tw->window), tw->notebook);
    gtk_widget_show (tw->notebook);
    gtk_notebook_set_show_border (GTK_NOTEBOOK (tw->notebook), FALSE);

    init_tilda_terminal (tw, tt);

    /* Exit on Ctrl-Q */
    clean = g_cclosure_new (clean_up, tw, NULL);
    accel_group = gtk_accel_group_new ();
    gtk_window_add_accel_group (GTK_WINDOW (tw->window), accel_group);
    gtk_accel_group_connect (accel_group, 'q', GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE, clean);

    gtk_window_set_decorated ((GtkWindow *) tw->window, FALSE);

    gtk_widget_set_size_request ((GtkWidget *) tw->window, 0, 0);
    fix_size_settings (tw);
    gtk_window_resize ((GtkWindow *) tw->window, tw->tc->min_width, tw->tc->min_height);

    if (!g_thread_create ((GThreadFunc) wait_for_signal, tw, FALSE, &error))
       perror ("Fuck that thread!!!");

    return TRUE;
}

