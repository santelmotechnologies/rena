/*****************************************************************************/
/* Copyright (C) 2024 Santelmo Technologies <santelmotechnologies@gmail.com> */
/*                                                                           */
/* This program is free software: you can redistribute it and/or modify      */
/* it under the terms of the GNU General Public License as published by      */
/* the Free Software Foundation, either version 3 of the License, or         */
/* (at your option) any later version.                                       */
/*                                                                           */
/* This program is distributed in the hope that it will be useful,           */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of            */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             */
/* GNU General Public License for more details.                              */
/*                                                                           */
/* You should have received a copy of the GNU General Public License         */
/* along with this program.  If not, see <http://www.gnu.org/licenses/>.     */
/*****************************************************************************/

#if HAVE_CONFIG_H
#include <config.h>
#endif

#include "rena.h"

#if defined(GETTEXT_PACKAGE)
#include <glib/gi18n-lib.h>
#else
#include <glib/gi18n.h>
#endif

#include <glib.h>
#include <locale.h> /* require LC_ALL */
#include <libintl.h>
#include <tag_c.h>

#ifdef DEBUG
GThread *rena_main_thread = NULL;
#endif

gint main(gint argc, gchar *argv[])
{
    RenaApplication *rena;
    int status;
#ifdef DEBUG
    g_print ("debug enabled\n");
    rena_main_thread = g_thread_self ();
#endif
    debug_level = 0;

    /* setup translation domain */
    setlocale (LC_ALL, "");
    bindtextdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
    bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
    textdomain (GETTEXT_PACKAGE);

    /* Force unicode to taglib. */
    taglib_set_strings_unicode(TRUE);
    taglib_set_string_management_enabled(FALSE);

    /* Setup application name and pulseaudio role */
    g_set_application_name(_("Rena Music Player"));
    g_setenv("PULSE_PROP_media.role", "audio", TRUE);

    rena = rena_application_new ();
    status = g_application_run (G_APPLICATION (rena), argc, argv);
    g_object_run_dispose (G_OBJECT (rena));
    g_object_unref (rena);

    return status;
}
