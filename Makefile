#
# Import the build engine first
__nmk_dir=$(CURDIR)/scripts/nmk/scripts/
export __nmk_dir

include $(__nmk_dir)/include.mk
include $(__nmk_dir)/macro.mk

SRC_DIR	:= $(CURDIR)
export SRC_DIR

PROGRAM		:= gscope

DEFINES		+= -D_FILE_OFFSET_BITS=64
DEFINES		+= -D_GNU_SOURCE

ifneq ($(WERROR),0)
        WARNINGS	+= -Werror
endif

ifeq ($(DEBUG),1)
        DEFINES		+= -DCR_DEBUG
        CFLAGS		+= -O0 -ggdb3
else
        CFLAGS		+= -O2 -g
endif

CFLAGS		+= $(WARNINGS) $(DEFINES)

CFLAGS		+= $(shell pkg-config --cflags gtk+-2.0 gtksourceview-2.0)
CFLAGS		+= $(shell pkg-config --cflags pango)
CFLAGS		+= -DGTK_DISABLE_DEPRECATED
CFLAGS		+= -iquote $(SRC_DIR)/include

export CFLAGS

LIBS		+= $(shell pkg-config --libs gtk+-2.0 gtksourceview-2.0)
LIBS		+= $(shell pkg-config --libs pango)
LIBS		+= -lc -pthread

export LIBS

$(eval $(call gen-built-in,src))

$(PROGRAM): src/built-in.o
	$(E) "  LINK    " $@
	$(Q) $(CC) $(LIBS) src/built-in.o -o $@

all: $(PROGRAM)
	@true
.PHONY: all

clean:
	$(Q) $(MAKE) $(build)=src $@
	$(Q) $(RM) $(PROGRAM)
.PHONY: clean

tags:
	$(call msg-gen, $@)
	$(Q) $(RM) tags
	$(Q) $(FIND) . -name '*.[hcS]' ! -path './.*' ! -path './test/*' -print | xargs $(CTAGS) -a
.PHONY: tags

etags:
	$(call msg-gen, $@)
	$(Q) $(RM) TAGS
	$(Q) $(FIND) . -name '*.[hcS]' ! -path './.*' ! -path './test/*' -print | xargs $(ETAGS) -a
.PHONY: etags

cscope:
	$(call msg-gen, $@)
	$(Q) $(FIND) . -name '*.[hcS]' ! -path './.*' ! -path './test/*' ! -type l -print > cscope.files
	$(Q) $(CSCOPE) -bkqu
.PHONY: cscope

.DEFAULT_GOAL := all
