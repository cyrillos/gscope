#
# Trivial makefile for gScope
#

ifeq ($(strip $(V)),)
	E = @echo
	Q = @
else
	E = @\#
	Q =
endif
export E Q

PROGRAM	= gscope
FIND = find
MKID = mkid
CSCOPE = cscope

uname_M      := $(shell uname -m | sed -e s/i.86/i386/)
ifeq ($(uname_M),i386)
DEFINES      += -DCONFIG_X86_32
ifeq ($(uname_M),x86_64)
DEFINES      += -DCONFIG_X86_64
endif
endif

GTK_CFLAGS = `pkg-config --cflags gtk+-2.0 gtksourceview-2.0`
GTK_LIBS = `pkg-config --libs gtk+-2.0 gtksourceview-2.0`

GTK_CFLAGS += `pkg-config --cflags pango`
GTK_LIBS += `pkg-config --libs pango`

CFLAGS	+= $(CPPFLAGS) $(DEFINES) $(GTK_CFLAGS) -I.

WARNINGS += -Wall
WARNINGS += -Wcast-align
WARNINGS += -Wformat=2
WARNINGS += -Winit-self
WARNINGS += -Wmissing-declarations
WARNINGS += -Wmissing-prototypes
WARNINGS += -Wnested-externs
WARNINGS += -Wno-system-headers
WARNINGS += -Wold-style-definition
WARNINGS += -Wredundant-decls
WARNINGS += -Wsign-compare
WARNINGS += -Wstrict-prototypes
WARNINGS += -Wundef
WARNINGS += -Wvolatile-register-var
WARNINGS += -Wwrite-strings

CFLAGS	+= $(WARNINGS)

GCCOPT	+= -ggdb3
#GCCOPT	+= -O2
#-std=c99 -pedantic
GCCOPT	+= -DGTK_DISABLE_DEPRECATED

CFLAGS	+= $(GCCOPT)

LIBS	+= $(GTK_LIBS) -lc -pthread

OBJS	+= backend-proto.o
OBJS	+= cscope.o
OBJS	+= id-utils.o
OBJS	+= tags.o
OBJS	+= main.o
OBJS	+= notebook.o
OBJS	+= queries.o
OBJS	+= sourceview.o
OBJS	+= util.o
OBJS	+= view.o
OBJS	+= config.o
OBJS	+= mark.o

all: $(PROGRAM)

$(PROGRAM): $(OBJS)
	$(E) "  LINK    " $@
	$(Q) $(CC) $(LIBS) $(OBJS) -o $@

$(OBJS):
%.o: %.c
	$(E) "  CC      " $@
	$(Q) $(CC) -c $(CFLAGS) $< -o $@

clean:
	$(E) " CLEAN *.o"
	$(Q) rm -f ./*.o
	$(E) " CLEAN $(PROGRAM)"
	$(Q) rm -f ./$(PROGRAM)
	$(Q) rm -f ./tags ./TAGS

TAGS:
	$(RM) -f TAGS
	$(FIND) . -name '*.[hcS]' -print | xargs etags -a

tags:
	$(RM) -f tags
	$(FIND) . -name '*.[hcS]' -print | xargs ctags -a

cscope:
	$(FIND) . -name '*.[hcS]' -print > cscope.files
	$(CSCOPE) -bkqu

id:
	$(FIND) . -name '*.[hcS]' -print | $(MKID)

.PHONY: TAGS tags clean cscope id
# DO NOT DELETE THIS LINE -- mkdep uses it.
# DO NOT PUT ANYTHING AFTER THIS LINE, IT WILL GO AWAY.

backend-proto.o: backend-proto.c backend-proto.h list.h util.h
config.o: config.c util.h
cscope.o: cscope.c cscope.h list.h backend-proto.h util.h
id-utils.o: id-utils.c id-utils.h list.h backend-proto.h util.h
main.o: main.c main.h view.h id-utils.h list.h cscope.h queries.h \
 backend-proto.h notebook.h tags.h sourceview.h util.h \
 img/gScope-32.png.img
notebook.o: notebook.c notebook.h list.h util.h sourceview.h \
 backend-proto.h
queries.o: queries.c queries.h list.h backend-proto.h notebook.h tags.h \
 id-utils.h cscope.h view.h sourceview.h util.h
sourceview.o: sourceview.c sourceview.h list.h tags.h backend-proto.h \
 util.h queries.h notebook.h id-utils.h cscope.h view.h
tags.o: tags.c tags.h list.h backend-proto.h util.h
util.o: util.c util.h
version.o: version.c
view.o: view.c view.h id-utils.h list.h cscope.h queries.h \
 backend-proto.h notebook.h tags.h sourceview.h util.h

# IF YOU PUT ANYTHING HERE IT WILL GO AWAY
