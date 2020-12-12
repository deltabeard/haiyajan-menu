# This file is for use with GNU Make only.
# Project details
NAME		:= Haiyajan-UI
DESCRIPTION	:= UI toolkit for Haiyajan
COMPANY		:= Deltabeard
AUTHOR		:= Mahyar Koshkouei
LICENSE_SPDX	:= All Rights Reserved

# Default configurable build options
BUILD	:= DEBUG

# Build help text shown using `make help`.
define help_txt
$(NAME) - $(DESCRIPTION)

Available options and their descriptions when enabled:
  BUILD=$(BUILD)
    The type of build configuration to use.
    This is one of DEBUG, RELEASE, RELDEBUG and RELMINSIZE.
      DEBUG: All debugging symbols; No optimisation.
      RELEASE: No debugging symbols; Optimised for speed.
      RELDEBUG: All debugging symbols; Optimised for speed.
      RELMINSIZE: No debugging symbols; Optimised for size.

  EXTRA_CFLAGS=$(EXTRA_CFLAGS)
    Extra CFLAGS to pass to C compiler.

  EXTRA_LDFLAGS=$(EXTRA_LDFLAGS)
    Extra LDFLAGS to pass to the C compiler.

Example: make BUILD=RELEASE EXTRA_CFLAGS="-march=native"

$(LICENSE)
endef

ifdef VSCMD_VER
	# Default compiler options for Microsoft Visual C++ (MSVC)
	CC	:= cl
	OBJEXT	:= obj
	RM	:= del
	EXEOUT	:= /Fe
	CFLAGS	:= /nologo /analyze /diagnostics:caret /utf-8 /std:c11 /W3 /Iext\inc
	LDFLAGS := /link /SUBSYSTEM:CONSOLE SDL2main.lib SDL2.lib shell32.lib /LIBPATH:ext\lib_$(VSCMD_ARG_TGT_ARCH)
	ICON_FILE := icon.ico
	RES	:= meta\winres.res
else
	# Default compiler options for GCC and Clang
	CC	:= cc
	OBJEXT	:= o
	RM	:= rm -f
	EXEOUT	:= -o
	CFLAGS	:= -std=c99 -pedantic -Wall -Wextra $(shell sdl2-config --cflags)
	LDFLAGS	:= $(shell sdl2-config --libs)
endif

# Options specific to 32-bit platforms
ifeq ($(VSCMD_ARG_TGT_ARCH),x32)
	# Use SSE instructions (since Pentium III).
	CFLAGS += /arch:SSE

	# Add support for ReactOS and Windows XP.
	CFLAGS += /Fdvc141.pdb
endif

#
# No need to edit anything past this line.
#
EXE	:= $(NAME)
LICENSE := (C) $(AUTHOR). $(LICENSE_SPDX).
GIT_VER := $(shell git describe --dirty --always --tags --long)

SRCS := $(wildcard src/*.c)
OBJS := $(SRCS:.c=.$(OBJEXT))

# Use a fallback git version string if build system does not have git.
ifeq ($(GIT_VER),)
	GIT_VER := LOCAL
endif

# Function to check if running within MSVC Native Tools Command Prompt.
MSVC_GCC = $(if $(VSCMD_VER),$(1),$(2))

# Select default build type
ifndef BUILD
	BUILD := DEBUG
endif

# Apply build type settings
ifeq ($(BUILD),DEBUG)
	CFLAGS += $(call MSVC_GCC,/Zi /MDd /RTC1 /sdl,-O0 -g3)
else ifeq ($(BUILD),RELEASE)
	CFLAGS += $(call MSVC_GCC,/MD /O2 /fp:fast,-Ofast -s)
else ifeq ($(BUILD),RELDEBUG)
	CFLAGS += $(call MSVC_GCC,/MDd /O2 /fp:fast,-Ofast -g3)
else ifeq ($(BUILD),RELMINSIZE)
	CFLAGS += $(call MSVC_GCC,/MD /O1 /fp:fast,-Os -ffast-math -s)
else
	err := $(error Unknown build configuration '$(BUILD)')
endif

# WHen compiling with MSVC, check if SDL2 has been expanded from prepared cab file.
ifeq ($(CC)$(wildcard SDL2.dll),cl)
    $(info Preparing SDL2 development libraries)
    EXPAND_CMD := expand ext/SDL2-2.0.12-VC.cab -F:* ext
    UNUSED := $(shell $(EXPAND_CMD))

    # Copy SDL2.DLL to output EXE directory.
    UNUSED := $(shell COPY ext\lib_$(VSCMD_ARG_TGT_ARCH)\SDL2.dll SDL2.dll)
endif

# Add UI example application to target.
TARGET += $(EXE)

# Add UI test to target
TEST_EXE := test/ui-test
TARGET += $(TEST_EXE)
TEST_SRCS := $(wildcard test/*.c) src/ui.c src/font.c src/menu.c
TEST_OBJS := $(TEST_SRCS:.c=.$(OBJEXT))

# File extension ".exe" is automatically appended on MinGW and MSVC builds, even
# if we don't ask for it.
# Append ".exe" for all targets.
ifeq ($(OS),Windows_NT)
	TARGET := $(addsuffix .exe,$(TARGET))
endif

override CFLAGS += -Iinc $(EXTRA_CFLAGS)
override LDFLAGS += $(EXTRA_LDFLAGS)

all: $(TARGET)
$(EXE): $(OBJS) $(RES)
	$(info LINK $@)
	$(CC) $(CFLAGS) $(EXEOUT)$@ $^ $(LDFLAGS)

$(TEST_EXE): $(TEST_OBJS)
	$(info LINK $@)
	$(CC) $(CFLAGS) $(EXEOUT)$@ $^ $(LDFLAGS)

%.o: %.c
	$(info CC $@)
	@$(CC) -c $(CFLAGS) $(CPPFLAGS) -o $@ $<

# cl always prints the source file name, so we just add the CC suffix.
%.obj: %.c
	$(info CC $<)
	@$(CC) $(CFLAGS) /Fo$@ /c /TC $^

%.res: %.rc
	$(info RC $@)
	@rc /nologo /DCOMPANY="$(COMPANY)" /DDESCRIPTION="$(DESCRIPTION)" \
		/DLICENSE="$(LICENSE)" /DGIT_VER="$(GIT_VER)" \
		/DNAME="$(NAME)" /DICON_FILE="$(ICON_FILE)" $^

clean:
	$(RM) $(EXE) $(RES)
	cd src && $(RM) *.$(OBJEXT)

help:
	@exit
	$(info $(help_txt))
