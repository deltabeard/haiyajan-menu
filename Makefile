# This file is for use with GNU Make only.
# Project details
NAME		:= haiyajan-menu
DESCRIPTION	:= Custom UI for Haiyajan
COMPANY		:= Deltabeard
AUTHOR		:= Mahyar Koshkouei
LICENSE_SPDX	:= LGPL-3.0

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

  PLATFORM=$(PLATFORM)
    Manualy specify target platform. If unset, an attempt is made to
    automatically determine this.
    Supported platforms:
      MSVC: For Windows NT platforms compiled with Visual Studio C++ Build Tools.
            Must be compiled within the "Native Tools Command Prompt for VS" shell.
      SWITCH: For Nintendo Switch homebrew platform. (Not automatically set).
      UNIX: For all Unix-like platforms, including Linux, BSD, MacOS, and MSYS2.

  EXTRA_CFLAGS=$(EXTRA_CFLAGS)
    Extra CFLAGS to pass to C compiler.

  EXTRA_LDFLAGS=$(EXTRA_LDFLAGS)
    Extra LDFLAGS to pass to the C compiler.

Example: make BUILD=RELEASE EXTRA_CFLAGS="-march=native"

$(LICENSE)
endef

ifdef VSCMD_VER
	PLATFORM := MSVC
else
	PLATFORM := UNIX
endif

ifeq ($(PLATFORM),MSVC)
	# Default compiler options for Microsoft Visual C++ (MSVC)
	CC	:= cl
	OBJEXT	:= obj
	RM	:= del
	CFLAGS	:= /nologo /utf-8 /W1 /Iinc /Iext\inc /FS /D_CRT_SECURE_NO_WARNINGS
	LDFLAGS := /link /SUBSYSTEM:CONSOLE SDL2main.lib SDL2.lib shell32.lib /LIBPATH:ext\lib_$(VSCMD_ARG_TGT_ARCH)
	ICON_FILE := icon.ico
	OBJS	+= meta\winres.res
	EXE	:= $(NAME).exe

else ifeq ($(PLATFORM),SWITCH)
	PORTLIBSBIN := $(DEVKITPRO)/portlibs/switch/bin
	PATH	:= $(PORTLIBSBIN):$(DEVKITPRO)/tools/bin:$(DEVKITPRO)/devkitA64/bin:$(PATH)
	PREFIX	:= aarch64-none-elf-
	CC	:= $(PREFIX)gcc
	CXX	:= $(PREFIX)g++
	EXE	:= $(NAME).nro
	OBJEXT	:= o
	CFLAGS	:= -march=armv8-a+crc+crypto -mtune=cortex-a57 -fPIE -mtp=soft -D__SWITCH__ \
		  $(shell $(PORTLIBSBIN)/sdl2-config --cflags)
	LDFLAGS	= -specs=$(DEVKITPRO)/libnx/switch.specs \
		  $(shell $(PORTLIBSBIN)/sdl2-config --static-libs)
	APP_ICON := $(DEVKITPRO)/libnx/default_icon.jpg

else ifeq ($(PLATFORM),UNIX)
	# Default compiler options for GCC and Clang
	CC	:= cc
	OBJEXT	:= o
	RM	:= rm -f
	CFLAGS	:= -Wall -Wextra -D_DEFAULT_SOURCE $(shell sdl2-config --cflags)
	LDFLAGS	:= $(shell sdl2-config --libs)
	EXE	:= $(NAME)

else
	err := $(error Unsupported platform specified)
endif

# Options specific to Windows NT 32-bit platforms
ifeq ($(VSCMD_ARG_TGT_ARCH),x86)
	# Use SSE instructions (since Pentium III).
	CFLAGS += /arch:SSE

	# Add support for ReactOS and Windows XP.
	CFLAGS += /Fdvc141.pdb
endif

#
# No need to edit anything past this line.
#
LICENSE := (C) $(AUTHOR). $(LICENSE_SPDX).
GIT_VER := $(shell git describe --dirty --always --tags --long)


SRCS := $(wildcard src/*.c) $(wildcard inc/lvgl/src/**/*.c)

# If using del, use Windows style folder separator.
ifeq ($(RM),del)
	SRCS := $(subst /,\,$(SRCS))
endif

OBJS += $(SRCS:.c=.$(OBJEXT))

# Use a fallback git version string if build system does not have git.
ifeq ($(GIT_VER),)
	GIT_VER := LOCAL
endif

# Function to check if running within MSVC Native Tools Command Prompt.
ISTARGNT = $(if $(VSCMD_VER),$(1),$(2))

# Select default build type
ifndef BUILD
	BUILD := DEBUG
endif

# Apply build type settings
ifeq ($(BUILD),DEBUG)
	CFLAGS += $(call ISTARGNT,/Zi /MDd /RTC1 /sdl,-O0 -g3)
	CFLAGS += -DSDL_ASSERT_LEVEL=2
else ifeq ($(BUILD),RELEASE)
	CFLAGS += $(call ISTARGNT,/MD /O2 /fp:fast /GL /GT /Ot /O2,-Ofast -flto)
	LDFLAGS += $(call ISTARGNT,/LTCG,)
else ifeq ($(BUILD),RELDEBUG)
	CFLAGS += $(call ISTARGNT,/MDd /O2 /fp:fast,-Ofast -g3 -flto)
else ifeq ($(BUILD),RELMINSIZE)
	CFLAGS += $(call ISTARGNT,/MD /O1 /fp:fast /GL /GT /Os,-Os -ffast-math -s -flto)
else ifeq ($(BUILD),BASIC)
	CFLAGS += $(call ISTARGNT,,)
else
	err := $(error Unknown build configuration '$(BUILD)')
endif

# When compiling with MSVC, check if SDL2 has been expanded from prepared cab file.
ifeq ($(CC)$(wildcard SDL2.dll),cl)
    $(info Preparing SDL2 development libraries)
    EXPAND_CMD := ext\MSVC_DEPS.exe -o"ext" -y
    UNUSED := $(shell $(EXPAND_CMD))

    # Copy SDL2.DLL to output EXE directory.
    UNUSED := $(shell COPY ext\lib_$(VSCMD_ARG_TGT_ARCH)\SDL2.dll SDL2.dll)
endif

# Add UI example application to target.
TARGET += $(EXE)

override CFLAGS += -Iinc -DBUILD=$(BUILD) $(EXTRA_CFLAGS)
override LDFLAGS += $(EXTRA_LDFLAGS)

all: $(TARGET)

# Unix rules
$(NAME): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $<

# MSVC rules
$(NAME).exe: $(OBJS)
	$(CC) $(CFLAGS) /Fe$@ $^ $(LDFLAGS)

%.obj: %.c
	$(CC) $(CFLAGS) /Fo$@ /c /TC $^

%.res: %.rc
	rc /nologo /DCOMPANY="$(COMPANY)" /DDESCRIPTION="$(DESCRIPTION)" \
		/DLICENSE="$(LICENSE)" /DGIT_VER="$(GIT_VER)" \
		/DNAME="$(NAME)" /DICON_FILE="$(ICON_FILE)" $^

# Nintendo Switch rules for use with devkitA64
$(NAME).nro: $(NAME).elf $(NAME).nacp
	elf2nro $< $@ --icon=$(APP_ICON) --nacp=$(NAME).nacp

$(NAME).elf: $(OBJS)
	$(CXX) $(CFLAGS) $^ -o $@ $(LDFLAGS)

%.nacp:
	nacptool --create "$(DESCRIPTION)" "$(COMPANY)" "$(GIT_VER)" $@

clean:
	$(RM) $(NAME) $(NAME).elf $(NAME).nacp $(NAME).exe $(RES) $(OBJS) $(SRCS:.c=.d) $(SRCS:.c=.gcda)

help:
	@exit
	$(info $(help_txt))
