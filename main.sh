#!/bin/bash

CFLAGS="-ggdb3 -mms-bitfields -IE:/gtk/include/gtk-2.0 -IE:/gtk/lib/gtk-2.0/include -IE:/gtk/include/atk-1.0 -IE:/gtk/include/cairo -IE:/gtk/include/pango-1.0 -IE:/gtk/include/glib-2.0 -IE:/gtk/lib/glib-2.0/include -IE:/gtk/include/libpng12"
LIBS="-ggdb3 -LE:/gtk/lib -lgtk-win32-2.0 E:/gtk/lib/libgdk-win32-2.0.dll.a -lglib-2.0 E:/gtk/lib/libglib-2.0.dll.a -lgmodule-2.0 E:/gtk/lib/libgmodule-2.0.dll.a -lgobject-2.0 E:/gtk/lib/libgobject-2.0.dll.a -lgthread-2.0 E:/gtk/lib/libgthread-2.0.dll.a -lgdk_pixbuf-2.0 E:/gtk/lib/libgdk_pixbuf-2.0.dll.a -lpng12 E:/gtk/lib/libpng12.dll.a -lpango-1.0 E:/gtk/lib/libpango-1.0.dll.a -lpangocairo-1.0 E:/gtk/lib/libpangocairo-1.0.dll.a -lpangoft2-1.0 E:/gtk/lib/libpangoft2-1.0.dll.a -lpangowin32-1.0 E:/gtk/lib/libpangowin32-1.0.dll.a"

CMD_C="gcc -c $CFLAGS"
CMD_L="gcc $LIBS"

# gdk-pixbuf-csource --raw --name=nb_close_12_inline nb-close-12.png > nb-close-12.dump
# gdk-pixbuf-csource --raw --name=gScope_32_inline gScope-32.png > gScope-32.dump

$CMD_C main.c
$CMD_C util.c
$CMD_C notebook.c
$CMD_C tags.c
$CMD_C sourceview.c
gcc -o gScope main.o util.o notebook.o tags.o sourceview.o $LIBS
