/* vim:set et sts=4: */
/* 
* Copyright (C) 2009-2010 Pravin Satpute <psatpute@redhat.com>
* Copyright (C) 2009-2010 Red Hat, Inc.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; version 2 only
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU Library General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include <ibus.h>
#include <stdlib.h>
#include <locale.h>
#include "engine.h"

#define N_(text) text

static IBusBus *bus = NULL;
static IBusFactory *factory = NULL;

/* options */
static gboolean ibus = FALSE;
static gboolean verbose = FALSE;

static const GOptionEntry entries[] =
{
    { "ibus", 'i', 0, G_OPTION_ARG_NONE, &ibus, "component is executed by ibus", NULL },
    { "verbose", 'v', 0, G_OPTION_ARG_NONE, &verbose, "verbose", NULL },
    { NULL },
};


static void
ibus_disconnected_cb (IBusBus  *bus,
                      gpointer  user_data)
{
    g_debug ("bus disconnected");
    ibus_quit ();
}


static void
start_component (void)
{
    IBusComponent *component;

    ibus_init ();

    bus = ibus_bus_new ();
    g_object_ref_sink (bus);
    g_signal_connect (bus, "disconnected", G_CALLBACK (ibus_disconnected_cb), NULL);

    factory = ibus_factory_new (ibus_bus_get_connection (bus));
    g_object_ref_sink (factory);

    ibus_factory_add_engine (factory, "rawcode", IBUS_TYPE_RAWCODE_ENGINE);

    if (ibus) {
        ibus_bus_request_name (bus, "org.freedesktop.IBus.Rawcode", 0);
    }
    else {
        component = ibus_component_new ("org.freedesktop.IBus.Rawcode",
                                        N_("Rawcode input method"),
                                        "0.1.0",
                                        "GPL",
                                        "Pravin Satpute <pravin.d.s@gmail.com>",
                                        "https://fedorahosted.org/ibus-rawcode/",
                                        "",
                                        "ibus-rawcode");
        ibus_component_add_engine (component,
                                   ibus_engine_desc_new ("rawcode",
                                                         N_("Rawcode Input Method"),
                                                         N_("Rawcode Input Method"),
                                                         "other",
                                                         "GPL",
                                                         "Pravin Satpute <pravin.d.s@gmail.com>",
                                                         PKGDATADIR"/icon/ibus-rawcode.png",
                                                         "us"));
        ibus_bus_register_component (bus, component);
    }

    ibus_main ();
}

int
main (gint argc, gchar **argv)
{
    GError *error = NULL;
    GOptionContext *context;

    setlocale (LC_ALL, "");

    context = g_option_context_new ("- ibus rawcode engine component");

    g_option_context_add_main_entries (context, entries, "ibus-rawcode");

    if (!g_option_context_parse (context, &argc, &argv, &error)) {
        g_print ("Option parsing failed: %s\n", error->message);
        exit (-1);
    }

    start_component ();
    return 0;
}
