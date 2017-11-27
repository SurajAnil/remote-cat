CC := gcc
CFLAGS += -Wall -Wextra -Wpedantic -std=gnu11

INCDIR = include
BUILDDIR = build
SRCDIR = src
CLIENTDIR = $(SRCDIR)/client
SERVERDIR = $(SRCDIR)/server

CLIENT = $(BUILDDIR)/remcat
SERVER = $(BUILDDIR)/remcatd

CFLAGS += $(foreach d, $(INCDIR),-I$d)

DEP := $(wildcard $(INCDIR)/*.h)
CLIENTSRC := $(wildcard $(CLIENTDIR)/*.c)
SERVERSRC := $(wildcard $(SERVERDIR)/*.c)

CLIENTOBJ := $(CLIENTSRC:%.c=$(BUILDDIR)/%.o)
CLIENTOBJ := $(subst $(CLIENTDIR)/,,$(CLIENTOBJ)) # Remove directory prefix
SERVEROBJ := $(SERVERSRC:%.c=$(BUILDDIR)/%.o)
SERVEROBJ := $(subst $(SERVERDIR)/,,$(SERVEROBJ)) # Remove directory prefix

.PHONY: $(BUILDDIR) clean default all

all: $(BUILDDIR) $(CLIENT) $(SERVER)

default: all

clean:
	rm -rf $(BUILDDIR)/* $(CLIENT) $(SERVER)

$(CLIENTOBJ): $(CLIENTSRC) | $(BUILDDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(SERVEROBJ): $(SERVERSRC) | $(BUILDDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(CLIENT): $(CLIENTOBJ)
	$(CC) $(CFLAGS) $< -o $@

$(SERVER): $(SERVEROBJ)
	$(CC) $(CFLAGS) $< -o $@

$(BUILDDIR):
	@mkdir -p $@

