# check pre-requisites
EXECUTABLES = install strip ln tar $(CC) $(AR) fakeroot mkdir git echo cat
K := $(foreach exec,$(EXECUTABLES),\
        $(if $(shell which $(exec)),some string,$(error "No $(exec) in PATH")))

# tools used
INSTALL = $(shell which install)
STRIP = $(shell which strip)
LN = $(shell which ln)
TAR = $(shell which tar)
FAKEROOT = $(shell which fakeroot)
MKDIR = $(shell which mkdir)
GIT = $(shell which git)
ECHO = $(shell which echo)
CAT = $(shell which cat)

# the current version
VERSION ?= $(subst v,,$(strip $(if $(shell git describe --tags 2> /dev/null),$(shell git describe --tags),v0.0.0)))
VERSION_PARTS = $(subst ., ,$(VERSION))
VMAJOR = $(word 1,$(VERSION_PARTS))
VMINOR = $(word 2,$(VERSION_PARTS))
VBUILD_FULL = $(word 3,$(VERSION_PARTS))
VBUILD = $(word 1,$(subst -, ,$(VBUILD_FULL)))

# the processor architecture 
MACHINE = $(shell $(CC) -dumpmachine)

# Target
TARGET_NAME = amxo
TARGET = lib$(TARGET_NAME)

# install directories
PREFIX = /usr/
INSTALL_INC_DIR = include/$(TARGET_NAME)
INSTALL_LIB_DIR ?= lib/$(MACHINE)
