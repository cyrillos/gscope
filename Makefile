.PHONY: all .FORCE
.DEFAULT_GOAL := all

ifeq ($(strip $(V)),)
        E := @echo
        Q := @
else
        E := @\#
        Q :=
endif

export E Q

define msg-gen
        $(E) "  GEN     " $(1)
endef

define msg-clean
        $(E) "  CLEAN   " $(1)
endef

export msg-gen msg-clean

MAKEFLAGS += --no-print-directory
export MAKEFLAGS

RM		?= rm -f
MAKE		?= make
GIT		?= git
CP		?= cp -f
CTAGS		?= ctags
PYTHON		?= python3

export RM MAKE GIT C

src-y	+= src/gscope
src-y	+= src/gscope.py
src-y	+= setup.py

tags: $(src-y)
	$(call msg-gen,$@)
	$(Q) $(CTAGS) --language-force=python $(src-y)

sdist: $(src-y) .FORCE
	$(call msg-gen,$@)
	$(Q) $(PYTHON) setup.py sdist
.PHONY: sdist

all-y += sdist

# Default target
all: $(all-y)

help:
	@echo '    Targets:'
	@echo '      all             - Build all [*] targets'
	@echo '     *sdist           - Prepare source distro'
	@echo '      tags            - Build tags'
.PHONY: help

archive:
	$(Q) $(GIT) archive --format=tar --prefix=gscope/ HEAD > gscope.tar
.PHONY: archive

clean:
	$(call msg-clean,tags)
	$(Q) $(RM) tags
	$(call msg-clean,sdist)
	$(Q) $(RM) -r dist
.PHONY: clean

.SUFFIXES:
