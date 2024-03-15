"""
@package tools.build_modules_xml

@brief Builds XML metadata of GRASS modules. Runs only during compilation.

(C) 2013 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Vaclav Petras <wenzeslaus gmail.com>
@author Anna Petrasova <kratochanna gmail.com>
"""

import sys

import grass.script.core as gcore
import grass.script.task as gtask
from grass.exceptions import ScriptError


def escapeXML(text):
    """This is a duplicate of function in core/toolboxes.

    >>> escapeXML('<>&')
    '&amp;lt;&gt;&amp;'
    """
    return text.replace("<", "&lt;").replace("&", "&amp;").replace(">", "&gt;")


def do_doctest_gettext_workaround():
    """This is a duplicate of function in core/toolboxes."""

    def new_displayhook(string):
        """A replacement for default `sys.displayhook`"""
        if string is not None:
            sys.stdout.write("%r\n" % (string,))

    def new_translator(string):
        """A fake gettext underscore function."""
        return string

    sys.displayhook = new_displayhook
    sys.__displayhook__ = new_displayhook

    import builtins

    builtins.__dict__["_"] = new_translator


def parse_modules(fd):
    """Writes metadata to xml file."""
    # TODO: what about ms windows? does gtask handle this?
    mlist = list(gcore.get_commands()[0])
    indent = 4
    for m in sorted(mlist):
        # TODO: get rid of g.mapsets_picker.py
        if m == "g.mapsets_picker.py" or m == "g.parser":
            continue
        desc, keyw = get_module_metadata(m)
        fd.write('%s<module-item name="%s">\n' % (" " * indent, m))
        indent += 4
        fd.write("%s<module>%s</module>\n" % (" " * indent, m))
        fd.write("%s<description>%s</description>\n" % (" " * indent, escapeXML(desc)))
        fd.write(
            "%s<keywords>%s</keywords>\n" % (" " * indent, escapeXML(",".join(keyw)))
        )
        indent -= 4
        fd.write("%s</module-item>\n" % (" " * indent))


def get_module_metadata(name):
    """

    >>> get_module_metadata('g.region')
    ('Manages the boundary definitions for the geographic region.', ['general', 'settings'])
    >>> get_module_metadata('m.proj')
    ('Converts coordinates from one projection to another (cs2cs frontend).', ['miscellaneous', 'projection'])
    """
    try:
        task = gtask.parse_interface(name)
    except ScriptError as exc:
        sys.stderr.write(
            "Cannot parse interface for module %s. Empty strings"
            " will be placed instead of description and keywords."
            " Reason: %s\n" % (name, str(exc))
        )
        return "", ""

    return task.get_description(full=True), task.get_keywords()


def header(fd):
    fd.write('<?xml version="1.0" encoding="UTF-8"?>\n')
    fd.write('<!DOCTYPE module-items SYSTEM "module_items.dtd">\n')
    fd.write("<!--This file is automatically generated using %s-->\n" % sys.argv[0])
    # g.version -r is crashing, commenting this block for now
    #    vInfo = gcore.version()
    #    fd.write('<!--version="%s" revision="%s" date="%s"-->\n' % \
    #                 (vInfo['version'].split('.')[0],
    #                  vInfo['revision'],
    #                  datetime.now()))
    fd.write("<module-items>\n")


def footer(fd):
    fd.write("</module-items>\n")


def doc_test():
    """Tests the module using doctest

    :return: a number of failed tests
    """
    import doctest

    do_doctest_gettext_workaround()
    return doctest.testmod().failed


def module_test():
    grass_commands = gcore.get_commands()[0]
    if "g.region" not in grass_commands:
        print("No g.region")
        return 1
    if "m.proj" not in grass_commands:
        print("No m.proj")
        return 1
    if "t.rast.univar" not in grass_commands:
        print("No t.rast.univar")
        return 1
    print(get_module_metadata("g.region"))
    print(get_module_metadata("m.proj"))
    print(get_module_metadata("t.rast.univar"))


def main():
    fh = sys.stdout

    header(fh)
    parse_modules(fh)
    footer(fh)

    return 0


if __name__ == "__main__":
    if len(sys.argv) > 1:
        if sys.argv[1] == "doctest":
            sys.exit(doc_test())
        elif sys.argv[1] == "test":
            sys.exit(module_test())
        else:
            gcore.fatal("Unrecognized parameter.")
    sys.exit(main())
