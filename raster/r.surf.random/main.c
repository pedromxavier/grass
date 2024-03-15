/****************************************************************************
 *
 * MODULE:       r.surf.random
 * AUTHOR(S):    Jo Wood, 19th October, 23rd October 1991 (original contributor)
 *               Midlands Regional Research Laboratory (ASSIST)
 * AUTHOR(S):    Markus Neteler <neteler itc.it> (original contributor)
 *               Vaclav Petras <wenzeslaus gmail com>
 * PURPOSE:      produces a raster map layer of uniform random deviates
 * COPYRIGHT:    (C) 1999-2020 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#include <stdlib.h>
#include <string.h>

#include <grass/gis.h>
#include <grass/glocale.h>
#include <grass/raster.h>
#include "local_proto.h"

/** Return TRUE if text contains only an integer number
 *
 * @param buffer text with the number or other content
 * @returns TRUE if all chars are read as an int, FALSE otherwise
 */
int is_int_only(const char *buffer)
{
    int unused_value;
    int num_items;
    int num_characters;

    /* %n is the number of characters read so far */
    num_items = sscanf(buffer, "%d%n", &unused_value, &num_characters);
    /* strlen returns size_t, but %n is int */
    if (num_items == 1 && num_characters == (int)strlen(buffer)) {
        return TRUE;
    }
    return FALSE;
}

/** Issue a fatal error if the option value is not integer
 *
 * This catches the cases when option is readable as integer,
 * but there would be additional characters left.
 * For example, when a number with a decimal point is read by C
 * functions, the decimal part is simply truncated and an integer is
 * read without an error. Although this might be fine in some cases,
 * here it can lead to different results as well as to unclear metadata.
 */
void option_must_be_int(struct Option *option)
{
    if (!is_int_only(option->answer))
        G_fatal_error(_("Option %s must be an integer, <%s> provided"),
                      option->key, option->answer);
}

int main(int argc, char *argv[])
{

    /****** INITIALISE ******/

    struct GModule *module;
    struct Option *out;
    struct Option *min;
    struct Option *max;
    struct Flag *i_flag;
    double min_value;
    double max_value;

    struct History history;
    char title[64];

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("raster"));
    G_add_keyword(_("surface"));
    G_add_keyword(_("random"));
    module->description = _("Produces a raster surface map of uniform random "
                            "deviates with defined range.");

    out = G_define_standard_option(G_OPT_R_OUTPUT);

    min = G_define_option();
    min->key = "min";
    min->description = _("Minimum random value");
    min->type = TYPE_DOUBLE;
    min->answer = "0";

    max = G_define_option();
    max->key = "max";
    max->description = _("Maximum random value");
    max->type = TYPE_DOUBLE;
    max->answer = "100";

    i_flag = G_define_flag();
    i_flag->key = 'i';
    i_flag->description = _("Create an integer raster map");

    if (G_parser(argc, argv))
        exit(EXIT_FAILURE);

    min_value = atof(min->answer);
    max_value = atof(max->answer);

    /* We disallow max=5.5 for integer output since there are unclear
     * expectations on what it should do. */
    if (i_flag->answer) {
        option_must_be_int(min);
        option_must_be_int(max);
    }

    /* We disallow min > max as a likely mistake, but we allow
     * min == max as a possible extreme case. */
    if (min_value > max_value) {
        /* showing the not parsed numbers to show exactly what user
         * provided and to avoid any issues with formatting %f vs %d */
        G_fatal_error(_("Minimum %s should be higher than maximum %s,"
                        " but %s > %s"),
                      min->key, max->key, min->answer, max->answer);
    }

    randsurf(out->answer, min_value, max_value, i_flag->answer);

    /* Using user-provided strings instead of attempting to guess the
     * right formatting. */
    if (i_flag->answer) {
        sprintf(title, _("Uniform random integer values in range [%s, %s]"),
                min->answer, max->answer);
    }
    else {
        sprintf(title, _("Uniform random float values in range [%s, %s)"),
                min->answer, max->answer);
    }
    Rast_put_cell_title(out->answer, title);
    Rast_short_history(out->answer, "raster", &history);
    Rast_command_history(&history);
    Rast_write_history(out->answer, &history);

    G_done_msg(_("Raster map <%s> created."), out->answer);

    exit(EXIT_SUCCESS);
}
