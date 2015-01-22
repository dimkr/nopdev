PROG = nopdev

CC ?= cc
CFLAGS ?= -O2
LDFLAGS ?=
DESTDIR ?= /
SBIN_DIR ?= sbin
DOC_DIR ?= usr/share/doc/$(PROG)
MAN_DIR ?= usr/share/man

CFLAGS += -Wall -pedantic -DPROG=\"$(PROG)\"

SRCS = $(wildcard *.c)
OBJECTS = $(SRCS:.c=.o)

all: $(PROG)

config.h: config.def.h
	$(warning Using the default configuration)
	cp config.def.h config.h

%.o: %.c config.h
	$(CC) -c -o $@ $< $(CFLAGS)

$(PROG): $(OBJECTS)
	$(CC) -o $@ $^ $(LDFLAGS)

clean:
	rm -f $(PROG) $(OBJECTS) config.h

install: $(PROG)
	install -D -m 755 $(PROG) $(DESTDIR)/$(SBIN_DIR)/$(PROG)
	install -D -m 644 $(PROG).8 $(DESTDIR)/$(MAN_DIR)/man8/$(PROG).8
	install -D -m 644 README $(DESTDIR)/$(DOC_DIR)/README
	install -m 644 AUTHORS $(DESTDIR)/$(DOC_DIR)/AUTHORS
	install -m 644 COPYING $(DESTDIR)/$(DOC_DIR)/COPYING
