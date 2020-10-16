NAME		:= Haiyajan-UI
DESCRIPTION	:= UI for Haiyajan
COMPANY		:= Deltabeard
COPYRIGHT	:= Copyright (c) 2020 Mahyar Koshkouei
LICENSE_SPDX	:= All Rights Reserved

# Default compiler options for GCC and Clang
CC	:= cc
OBJEXT	:= o
RM	:= rm -f
EXEOUT	:= -o
CFLAGS	:= -std=c99 -pedantic -Wall -O0 -g3
EXE	:= $(NAME)
LICENSE := $(COPYRIGHT); $(LICENSE_SPDX).
GIT_VER := $(shell git describe --dirty --always --tags --long)

define help_txt
$(NAME) - $(DESCRIPTION)

Available options and their descriptions when enabled:
  SDL_FLAGS=$(SDL_FLAGS)
  SDL_LIBS=$(SDL_LIBS)

$(LICENSE)
endef

SDL_FLAGS := $(shell sdl2-config --cflags)
SDL_LIBS := $(shell sdl2-config --libs)

SRCS := $(wildcard src/*.c)
OBJS := $(SRCS:.c=.$(OBJEXT))

# File extension ".exe" is automatically appended on MinGW and MSVC builds, even
# if we don't ask for it.
ifeq ($(OS),Windows_NT)
	EXE := $(NAME).exe
endif

ifeq ($(GIT_VER),)
	GIT_VER := LOCAL
endif

override CFLAGS += -Iinc $(SDL_FLAGS)
override LDLIBS += $(SDL_LIBS)

all: $(NAME)
$(NAME): $(OBJS) $(RES)
	$(CC) $(CFLAGS) $(EXEOUT)$@ $^ $(LDFLAGS) $(LDLIBS)

%.obj: %.c
	$(CC) $(CFLAGS) /Fo$@ /c /TC $^

%.res: %.rc
	rc /nologo /DCOMPANY="$(COMPANY)" /DDESCRIPTION="$(DESCRIPTION)" \
		/DLICENSE="$(LICENSE)" /DGIT_VER="$(GIT_VER)" \
		/DNAME="$(NAME)" /DICON_FILE="$(ICON_FILE)" $^

clean:
	$(RM) *.$(OBJEXT) $(EXE) $(RES)

help:
	@cd
	$(info $(help_txt))
